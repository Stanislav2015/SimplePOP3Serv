#pragma once

#include "POP3Session.h"
#include "UserManager.h"
#include "MailboxServiceManager.h"

#include <memory>
#include <boost/asio.hpp>


class POP3Server {
public:
	POP3Server(std::shared_ptr<MailboxServiceManager> manager, boost::asio::ip::tcp::acceptor acceptor) : 
		manager(manager), acceptor(std::move(acceptor))
	{
	}

	void serve() {
		acceptor.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
				if (ec && ec.value() == boost::asio::error::operation_aborted) {
					//aborted
					return;
				}
				serve(); 
				if (ec) {
					return;
				}
				auto session = POP3Session::createSession(std::move(socket), manager);
				session->read();
			});
	}

	void cancel() {
		POP3Session::cancelAll();
		acceptor.cancel();
	}

private:
	std::shared_ptr<MailboxServiceManager> manager;
	boost::asio::ip::tcp::acceptor acceptor;
};

class POP3ServerBuilder {
public:
	POP3ServerBuilder(std::string _addr, int port_num) : addr(_addr), port_number(port_num) {}

	POP3Server build(boost::asio::io_context &context) {
		//TODO: Реализовать полноценную фабрику
		std::shared_ptr<UserManager> userManager{ new DummyUserManager() };
		std::shared_ptr<MailboxServiceManager> mailboxServiceManager = std::make_shared<MailboxServiceManager>(userManager);
		if (!addr.empty()) {
			auto adr = boost::asio::ip::make_address(addr);
			boost::asio::ip::tcp::acceptor acceptor(context, boost::asio::ip::tcp::endpoint(adr, port_number));
			return POP3Server(mailboxServiceManager, std::move(acceptor));
		}
		else {
			boost::asio::ip::tcp::acceptor acceptor(context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port_number));
			return POP3Server(mailboxServiceManager, std::move(acceptor));
		}
	}
private:
	int port_number;
	std::string addr;
};
