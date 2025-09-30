

#include "AuthorizationManager.h"
#include "MailboxServiceManager.h"
#include "POP3Session.h"
#include "Server.h"
#include "Info.h"

typedef Server<POP3Session> POP3Server;

void dummy_generate_consumers(std::unique_ptr<ConsumerInfoStorage>& storage, unsigned int count = 10) {
	for (unsigned int i = 0; i < count; i++) {
		using namespace std::string_literals;
		ConsumerInfo settingsInstance("user"s.append(boost::lexical_cast<std::string>(i + 1)), "11111111");
		settingsInstance.addStorage(StorageType::FileSystemMailStorage);
		storage->addConsumerInfo(settingsInstance);
	}
}

int main()
{
	auto stor = FileSystemConsumerInfoStorage::Default();
	std::unique_ptr<AuthorizationManager> AuthorizationManager{ new SingleConsumerInfoStorageAuthorizationManager(std::move(stor)) };
	MailboxServiceManager::SetAuthorizationManager(std::move(AuthorizationManager));
	ConsoleServerController<POP3Server>::Run();
	return 0;
}