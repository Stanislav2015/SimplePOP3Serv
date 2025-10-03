

#include "AuthorizationManager.h"
#include "MailboxServiceManager.h"
#include "POP3Session.h"
#include "Server.h"
#include "ConsumerInfo.h"

#include <boost/lexical_cast.hpp>

typedef Server<POP3Session> POP3Server;

void dummy_generate_consumers(std::unique_ptr<HashedFileSystemConsumerInfoStorage>& storage, unsigned int count = 10) {
	std::filesystem::path p = "D:\\mailboxes";
	for (unsigned int i = 0; i < count; i++) {
		using namespace std::string_literals;
		auto name = "user"s.append(boost::lexical_cast<std::string>(i + 1));
		auto password = name + "11";
		BothNumericConsumerInfoNameConverter f1;
		BothNumericConsumerInfoPasswordConverter f2;
		BothNumericConsumerInfo settingsInstance(f1(name), f2(password));
		std::map<std::string, std::string> string_options;
		string_options.emplace("path", (p / name).string());
		std::map<std::string, unsigned int> numeric_options;
		settingsInstance.addStorage(StorageType::FileSystemMailStorage, string_options, numeric_options);
		storage->addConsumerInfo(settingsInstance);
	}
}

int main()
{
	auto stor = CreateHashedFileSystemConsumerInfoStorage();
	//dummy_generate_consumers(stor, 30000);
	std::unique_ptr<AuthorizationManager> AuthorizationManager{
		new SingleConsumerInfoStorageAuthorizationManager<BothNumericConsumerInfo>(std::move(stor))
	};
	MailboxServiceManager::SetAuthorizationManager(std::move(AuthorizationManager));
	ConsoleServerController<POP3Server>::Run();
	return 0;
}