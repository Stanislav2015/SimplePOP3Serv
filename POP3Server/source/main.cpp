

#include "AuthorizationManager.h"
#include "MailboxServiceManager.h"
#include "POP3Session.h"
#include "Server.h"
#include "Info.h"

typedef Server<POP3Session> POP3Server;

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
	ConsoleServerController<POP3Server>::Run();
	return 0;
}