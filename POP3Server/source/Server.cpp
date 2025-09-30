
#include <iostream>
#include "Enums.h"
#include "common_headers.h"
#include "AuthorizationManager.h"
#include "MailboxServiceManager.h"
#include "POP3Server.h"
#include "Info.h"

#include <boost/asio.hpp>
#include <thread>

using namespace boost::asio;

int main()
{
	auto stor = FileSystemConsumerInfoStorage::createDefault();
	/*for (int i = 0; i < 10; i++) {
		using namespace std::string_literals;
		ConsumerInfo settingsInstance("user"s.append(boost::lexical_cast<std::string>(i + 1)), "11111111");
		settingsInstance.addStorage(StorageType::FileSystemMailStorage);
		stor->addConsumerInfo(settingsInstance);
	}*/
	std::shared_ptr<AuthorizationManager> AuthorizationManager{ new SingleConsumerInfoStorageAuthorizationManager(stor) };
	MailboxServiceManager::SetAuthorizationManager(AuthorizationManager);

	unsigned int thread_pool_size = std::thread::hardware_concurrency() * 2;
	thread_pool_size = std::max(static_cast<unsigned int>(2), thread_pool_size);

	try {
		io_context io_context;
		std::string addr = "127.0.0.1";
		int port_number = 110;
		POP3ServerBuilder builder(addr, port_number);
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
	return 0;
}