#pragma once

#include <memory>
#include <atomic>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include "POP3Command.h"
#include "Enums.h"
#include "Mailbox.h"

//using namespace boost::asio;

enum class POP3SessionState
{
	Authorization,
	Transaction,
	Update
};

enum class POP3SessionError {
	CommandNotAllowedForAnonymous,
	MailboxNotFound,
	MailboxIsBusy,
	InternalError,
	OtherUserIsLoggedNow,
	MailboxAlreadyUsed,
	NoSuchMessage,
	MessageAlreadyDeleted
};

inline std::ostream& operator<<(std::ostream& out, const POP3SessionError& err) {
	std::string message;
	switch (err)
	{
	case POP3SessionError::CommandNotAllowedForAnonymous: 
		message = "you must authorized before calling this command";
		break;
	case POP3SessionError::MailboxNotFound:
		message = "sorry, no mailbox for such user here";
		break;
	case POP3SessionError::MailboxIsBusy:
		message = "mailbox is busy at the moment, please try again later";
		break;
	case POP3SessionError::InternalError:
		message = "some internal error occured, please try again later";
		break;
	case POP3SessionError::OtherUserIsLoggedNow:
		message = "you have already logged using other name, please quit from mailbox and then try again";
		break;
	case POP3SessionError::MailboxAlreadyUsed:
		message = "maildrop already locked";
		break;
	case POP3SessionError::NoSuchMessage:
		message = "no such message";
		break;
	case POP3SessionError::MessageAlreadyDeleted:
		message = "no such message";
		break;
	default:
		break;
	}
	out << message;
	return out;
}

class POP3Session : public std::enable_shared_from_this<POP3Session>
{
public:

	explicit POP3Session(boost::asio::ip::tcp::socket socket) : sessionId{ counter++ }, socket{ std::move(socket) }, mailbox{} {
		//nothing
	}

	static std::shared_ptr<POP3Session> createSession(boost::asio::ip::tcp::socket socket);

	void read() {
		boost::asio::async_read_until(socket, boost::asio::dynamic_buffer(request), "\r\n", 
			[self = shared_from_this()](boost::system::error_code ec,
			std::size_t length){
			if (ec) {
				self->deleteFromSessions();
				//TODO: Что-то делать с ошибками
				return;
			}

			self->readImpl();
		});
	}

	void write() {
		boost::asio::async_write(socket, boost::asio::buffer(response), [self = shared_from_this()](boost::system::error_code ec,
			std::size_t length){
			if (ec) {
				self->deleteFromSessions();
				//TODO: Что-то делать с ошибками
				return;
			}
			if (self->quitCommandReceived) {
				if (self->mailbox) {
					self->mailbox->setUpdate();
				}
				self->deleteFromSessions();
			}
			else {
				self->request.clear();
				self->read();
			}
		});
	}

	~POP3Session() {
		if (mailbox) {
			// need to set free
			//mailboxServManager->releaseMailbox(mailbox->getName());
		}
	}

	/// <summary>
	/// Cancel all sessions
	/// </summary>
	static void cancelAll();

private:
	template<typename Err>
	void setErrorResponse(Err err);
	
	void readImpl();

	void putMailboxInfoToReponse();
	void setSimpleOkResponse(std::string = "");
	void handleList(const POP3Command& cmd);
	void handleDelete(const POP3Command& cmd);
	void handleRetr(const POP3Command& cmd);
	void handleAnonymousCommand(const POP3Command& cmd);
	void handleAuthorizedUserCommand(const POP3Command& cmd);

	//static
	static std::list<std::shared_ptr<POP3Session>> sessions;
	static std::mutex m_mutex;


	void deleteFromSessions();

	POP3SessionState state{POP3SessionState::Authorization};
	boost::asio::ip::tcp::socket socket;
	std::string request;
	std::string response;
	std::string userName;
	std::string password;
	bool quitCommandReceived{ false };
	mailbox_ptr mailbox;
	static std::atomic<std::size_t> counter;
	std::size_t sessionId;
};
