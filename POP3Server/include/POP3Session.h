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
	ProhibitedForAnonymous,
	NotRegistered,
	MailboxIsBusy,
	InternalError,
	OtherMailboxBeingUsed,
	AlreadyLogged,
	NoSuchMessage,
	MessageAlreadyDeleted
};

inline std::ostream& operator<<(std::ostream& out, const POP3SessionError& err) {
	std::string message;
	switch (err)
	{
	case POP3SessionError::ProhibitedForAnonymous: 
		message = "you must authorized before calling this command";
		break;
	case POP3SessionError::NotRegistered:
		message = "sorry, no mailbox for such user here";
		break;
	case POP3SessionError::MailboxIsBusy:
		message = "mailbox is busy at the moment, please try again later";
		break;
	case POP3SessionError::InternalError:
		message = "some internal error occured, please try again later";
		break;
	case POP3SessionError::OtherMailboxBeingUsed:
		message = "you have already logged using other name, please quit from mailbox and then try again";
		break;
	case POP3SessionError::AlreadyLogged:
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
	constexpr static boost::asio::chrono::minutes Timeout = boost::asio::chrono::minutes(1);

	explicit POP3Session(boost::asio::ip::tcp::socket socket, boost::asio::steady_timer timer) : sessionId{ counter++ },
		socket(std::move(socket)), timer(std::move(timer)),
		mailbox{}, lastActivityTime(boost::asio::chrono::steady_clock::now())
	{
		//nothing
	}

	static std::shared_ptr<POP3Session> CreateSession(boost::asio::ip::tcp::socket socket, boost::asio::steady_timer timer);

	void read() {
		boost::asio::async_read_until(socket, boost::asio::dynamic_buffer(request), "\r\n", 
			[self = shared_from_this()](boost::system::error_code ec,
			std::size_t length){
			if (ec) {
				self->deleteFromSessions();
				//TODO: Что-то делать с ошибками
				return;
			}
			self->prolongateLifeTime();
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

	inline void startTimer() { 
		timer.async_wait([self = shared_from_this()](boost::system::error_code ec){
			if (!ec) {
				//time expired
				if ((boost::asio::chrono::steady_clock::now() - self->lastActivityTime) > Timeout) {
					//client does not reponse too long
					self->socket.cancel();
				}
				else {
					self->timer.expires_after(Timeout);
					self->startTimer();
				}
			}
			else if (ec == boost::asio::error::operation_aborted) {
				//operation was canceled, do nothing
			}
			else {
				self->startTimer();
			}
		});
	}

	~POP3Session() {}

	/// <summary>
	/// Cancel all sessions
	/// </summary>
	static void cancelAll();
	static void cancelParticular(std::size_t id);

private:
	

	template<typename Err>
	void setErrorResponse(Err err);
	
	void readImpl();

	void putMailboxInfoToReponse();
	void setSimpleOkResponse(std::string_view = "");
	void handleList(const POP3Command& cmd);
	void handleDelete(const POP3Command& cmd);
	void handleRetr(const POP3Command& cmd);
	void handleAnonymousCommand(const POP3Command& cmd);
	void handleAuthorizedUserCommand(const POP3Command& cmd);

	inline void prolongateLifeTime() {
		lastActivityTime = boost::asio::chrono::steady_clock::now();
	}

	//static
	static std::list<std::shared_ptr<POP3Session>> sessions;
	static std::mutex m_mutex;


	void deleteFromSessions();

	POP3SessionState state{POP3SessionState::Authorization};
	boost::asio::ip::tcp::socket socket;
	boost::asio::steady_timer timer;
	std::string request;
	std::string response;
	std::string userName;
	std::string password;
	bool quitCommandReceived{ false };
	mailbox_ptr mailbox;
	static std::atomic<std::size_t> counter;
	std::size_t sessionId;
	boost::asio::chrono::steady_clock::time_point lastActivityTime;
};
