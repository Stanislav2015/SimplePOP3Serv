
#include "MailboxLock.h"
#include "MailboxServiceManager.h"

MailboxLock::MailboxLock(std::string name) : name{ name }, acquired{ false } {
	acquired = MailboxServiceManager::LockMailbox(name);
}

MailboxLock::~MailboxLock() {
	if (acquired) {
		MailboxServiceManager::UnlockMailbox(name);
	}
}