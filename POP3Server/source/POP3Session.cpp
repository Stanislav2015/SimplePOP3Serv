
#include "POP3Session.h"
#include "POP3Status.h"
#include "MailboxServiceManager.h"

#include <assert.h>
#include <variant>

std::atomic<std::size_t> POP3Session::counter = 0;
std::list<std::shared_ptr<POP3Session>> POP3Session::sessions;
std::mutex POP3Session::m_mutex;

std::shared_ptr<POP3Session> POP3Session::createSession(boost::asio::ip::tcp::socket socket) {

	auto session = std::make_shared<POP3Session>(std::move(socket));

	{
		std::lock_guard<std::mutex> lg{ m_mutex };
		sessions.push_front(session);
	}

	return session;
}

void POP3Session::deleteFromSessions() {
	std::lock_guard<std::mutex> lg{ m_mutex };
	auto it = std::find_if(sessions.cbegin(), sessions.cend(), [this](const auto& session) {
		return session->sessionId == sessionId;
		});
	assert(it != sessions.cend());
	sessions.erase(it);
}

void POP3Session::cancelAll() {
	std::lock_guard<std::mutex> lg{ m_mutex };
	std::for_each(sessions.cbegin(), sessions.cend(), [](const auto& session) {
		session->socket.cancel();
	});
}

template<typename Err>
void POP3Session::setErrorResponse(Err err) {
	response = boost::lexical_cast<std::string>(POP3Status::ERR);
	response.append(" ");
	response.append(boost::lexical_cast<std::string>(err));
	response.append("\r\n");
}

void POP3Session::putMailboxInfoToReponse() {
	std::ostringstream ss;
	ss << boost::lexical_cast<std::string>(POP3Status::OK) << " " << userName << "'s maildrop has " <<
		mailbox->getEmailsCount() << " messages (" << mailbox->getWholeMailboxSize() << " octets)\r\n";
	response = ss.str();
}

void POP3Session::setSimpleOkResponse(std::string mesg) {
	response = boost::lexical_cast<std::string>(POP3Status::OK) + (mesg.empty() ? "" : " ") + mesg + "\r\n";
}

void POP3Session::handleAnonymousCommand(const POP3Command& cmd) {
	switch (cmd.cmdType)
	{
	case POP3CommandType::USER: {
		auto username = std::get<std::string>(cmd.parameter);
		if (!MailboxServiceManager::IsUserRegistered(username)) {
			setErrorResponse(POP3SessionError::MailboxNotFound);
		}
		else {
			userName = username;
			std::string mesg = "user ";
			mesg.append(username);
			mesg.append(" exists");
			setSimpleOkResponse(mesg);
		}
		break;
	}
	case POP3CommandType::PASS: {
		auto pass = std::get<std::string>(cmd.parameter);
		auto result = MailboxServiceManager::connect(userName, pass);
		if (std::holds_alternative<mailbox_ptr>(result)) {
			mailbox = std::move(std::get<mailbox_ptr>(result));
			state = POP3SessionState::Transaction;
			putMailboxInfoToReponse();
		}
		else if (std::holds_alternative<AuthError>(result)) {
			setErrorResponse(std::get<AuthError>(result));
		}
		else {
			auto err = std::get<MailboxOperationError>(result);
			if (err == MailboxOperationError::MailboxIsBusy) {
				setErrorResponse(POP3SessionError::MailboxIsBusy);
			}
			else {
				setErrorResponse(POP3SessionError::InternalError);
			}
		}
		break;
	}
	case POP3CommandType::NOOP: {
		setSimpleOkResponse();
		break;
	}
	case POP3CommandType::QUIT: {
		quitCommandReceived = true;
		setSimpleOkResponse();
		break;
	}
	default:
		//Not allowed for anonymous
		setErrorResponse(POP3SessionError::CommandNotAllowedForAnonymous);
		return;
	}
}

void POP3Session::handleList(const POP3Command& cmd) {
	if (std::holds_alternative<unsigned int>(cmd.parameter)) {
		auto number = std::get<unsigned int>(cmd.parameter);
		auto result = mailbox->getEmailLength(number);
		if (std::holds_alternative<MailboxOperationError>(result)) {
			setErrorResponse(POP3SessionError::NoSuchMessage);
		}
		else {
			std::ostringstream ss;
			ss << boost::lexical_cast<std::string>(POP3Status::OK) << " " <<
				number << " " << std::get<std::size_t>(result) << "\r\n";
			response = ss.str();
		}
	}
	else {
		auto lengths = mailbox->getEmailsLengths();
		std::ostringstream ss;
		ss << boost::lexical_cast<std::string>(POP3Status::OK) << " " << mailbox->getEmailsCount() << " messages (" << 
			mailbox->getWholeMailboxSize() << " octets)\r\n";
		if (!lengths.empty()) {
			std::for_each(lengths.cbegin(), lengths.cend(), [&ss](const auto& p)
				{
					ss << p.first << " " << p.second << "\r\n";
				});
			ss << ".\r\n";
		}
		response = ss.str();
	}
}

void POP3Session::handleDelete(const POP3Command& cmd) {
	if (std::holds_alternative<unsigned int>(cmd.parameter)) {
		auto result = mailbox->deleteEmail(std::get<unsigned int>(cmd.parameter));
		if (result == MailboxOperationError::EmailNotFound) {
			setErrorResponse(POP3SessionError::NoSuchMessage);
		}
		else if (result == MailboxOperationError::EmailAlreadyDeleted) {
			setErrorResponse(POP3SessionError::MessageAlreadyDeleted);
		}
		else {
			//success
			putMailboxInfoToReponse();
			//setSimpleOkResponse();
		}
	}
	else {
		mailbox->deleteEmails();
		setSimpleOkResponse();
	}
}

void POP3Session::handleRetr(const POP3Command& cmd) {
	auto number = std::get<unsigned int>(cmd.parameter);
	auto result = mailbox->getEmailLength(number);
	if (std::holds_alternative<MailboxOperationError>(result)) {
		setErrorResponse(POP3SessionError::NoSuchMessage);
		return;
	}
	std::ostringstream ss;
	ss << boost::lexical_cast<std::string>(POP3Status::OK) << " " << std::get<std::size_t>(result) << " octets\r\n";
	auto result1 = mailbox->getEmail(number);
	if (std::holds_alternative<MailboxOperationError>(result1)) {
		setErrorResponse(POP3SessionError::NoSuchMessage);
	}
	else {
		auto content = std::get<std::string>(result1);
		if (!boost::ends_with(content, "\r\n")) {
			content.append("\r\n");
		}
		ss << std::move(content) << ".\r\n";
		response = ss.str();
	}
}

void POP3Session::handleAuthorizedUserCommand(const POP3Command& cmd) {
	switch (cmd.cmdType)
	{
	case POP3CommandType::LIST: {
		handleList(cmd);
		break;
		}
	case POP3CommandType::DELE: {
		handleDelete(cmd);
		break;
	}
	case POP3CommandType::NOOP: {
		setSimpleOkResponse();
		break;
	}
	case POP3CommandType::USER: {
		if (userName == std::get<std::string>(cmd.parameter)) {
			std::ostringstream ss;
			ss << boost::lexical_cast<std::string>(POP3Status::OK) << " " << userName << "\r\n";
			response = ss.str();
		}
		else {
			setErrorResponse(POP3SessionError::OtherUserIsLoggedNow);
		}
		break;
	}
	case POP3CommandType::PASS: {
		setErrorResponse(POP3SessionError::MailboxAlreadyUsed);
		break;
	}
	case POP3CommandType::QUIT: {
		quitCommandReceived = true;
		setSimpleOkResponse("POP3 server signing off");
		break;
	}
	case POP3CommandType::RETR: {
		handleRetr(cmd);
		break;
	}
	case POP3CommandType::STAT: {
		putMailboxInfoToReponse();
		break;
	}
	case POP3CommandType::RSET : {
		mailbox->reset();
		putMailboxInfoToReponse();
		break;
	}
	default:
		break;
	}
}

void POP3Session::readImpl() {
	auto result = parsePOP3Command(request);
	response.clear();

	if (std::holds_alternative<ParsingError>(result)) {
		setErrorResponse(std::get<ParsingError>(result));
	}
	else {
		auto command = std::get<POP3Command>(result);
		if (state == POP3SessionState::Authorization) {
			handleAnonymousCommand(command);
		}
		else {
			handleAuthorizedUserCommand(command);
		}
	}

	write();
}