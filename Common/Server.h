#pragma once

#include <memory>
#include <boost/asio.hpp>

template<typename SessionType>
class Server {
	using ServerImpl = Server<SessionType>;
public:
	Server(boost::asio::ip::tcp::acceptor acceptor) : acceptor(std::move(acceptor)) {
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
				auto session = SessionType::createSession(std::move(socket));
				session->read();
			});
	}

	void cancel() {
		SessionType::cancelAll();
		acceptor.cancel();
	}

private:
	boost::asio::ip::tcp::acceptor acceptor;
};

template<typename ServerType>
class ServerBuilder {
public:
	ServerBuilder(std::string_view _addr, unsigned short port_num) : addr(_addr), port_number(port_num) {}

	ServerType build(boost::asio::io_context &context) {
		if (!addr.empty()) {
			auto adr = boost::asio::ip::make_address(addr);
			boost::asio::ip::tcp::acceptor acceptor(context, boost::asio::ip::tcp::endpoint(adr, port_number));
			return ServerType(std::move(acceptor));
		}
		else {
			boost::asio::ip::tcp::acceptor acceptor(context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port_number));
			return ServerType(std::move(acceptor));
		}
	}
private:
	unsigned short port_number;
	std::string addr;
};

template<typename ServerType>
class ConsoleServerController
{
public:
	static void Run(std::string_view addr = "127.0.0.1", unsigned short portNumber = 110, unsigned int thread_pool_size = 0) {
		using namespace boost::asio;

		if (!verifyAddress(addr)) {
			std::cerr << "Invalid ip address: " << addr << "\n";
			return;
		}

		if (thread_pool_size == 0) {
			thread_pool_size = std::thread::hardware_concurrency() * 2;
			thread_pool_size = std::max(static_cast<unsigned int>(2), thread_pool_size);
		}
		
		try {
			io_context io_context;
			ServerBuilder<ServerType> builder(addr, portNumber);
			auto server = builder.build(io_context);
			server.serve();

			std::vector<std::future<void>> futures;
			std::generate_n(std::back_inserter(futures), thread_pool_size, [&io_context]() {
				return std::async(std::launch::async, [&io_context] { io_context.run(); });
				});

			while (true) {
				std::string command;
				std::cin >> command;
				try {
					boost::algorithm::to_lower(command);
					if (command == "quit") {
						server.cancel();
						break;
					}
				}
				catch (...) {
				}
			}

			for (auto& future : futures) {
				try {
					future.get();
				}
				catch (const std::exception& e) {
					std::cerr << e.what() << std::endl;
				}
			}
		}
		catch (std::exception& e) {
			std::cerr << e.what() << std::endl;
		}
	}
private:
	static bool verifyAddress(std::string_view addr) {
		try {
			boost::asio::ip::make_address(addr);
			return true;
		}
		catch (boost::system::error_code) {
			return false;
		}
	}
};
