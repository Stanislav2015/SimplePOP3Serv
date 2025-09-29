
#include "MailboxServiceManager.h"
#include "common_headers.h"

std::shared_ptr<UserManager> MailboxServiceManager::userManager;
std::mutex MailboxServiceManager::m_mutex;
std::set<std::string> MailboxServiceManager::activeMailboxes;

bool MailboxServiceManager::LockMailbox(const std::string& name) {
	std::lock_guard<std::mutex> _lock{ m_mutex };
	auto it = activeMailboxes.find(name);
	if (it != activeMailboxes.end()) {
		return false;
	}
	else {
		activeMailboxes.insert(name);
		return true;
	}
}

void MailboxServiceManager::UnlockMailbox(const std::string& name) {
	std::lock_guard<std::mutex> _lock{ m_mutex };
	activeMailboxes.erase(name);
}


#include "MailboxLock.h"
#include "FileSystemMailStorage.h"

std::variant<mailbox_ptr, MailboxOperationError, AuthError> MailboxServiceManager::connect(
	const std::string& mailboxName,
	const std::string& password)
{
	MailboxLock lock{ mailboxName };
	if (lock()) {
		auto result = userManager->logon(mailboxName, password);
		if (std::holds_alternative<std::vector<StorageSettings>>(result)) {
			auto vec = std::get<std::vector<StorageSettings>>(result);
			if (vec.empty()) {
				return AuthError::UserHasNoStorages;
			}
			std::vector<storage_ptr> storages;
			std::for_each(vec.cbegin(), vec.cend(), [&storages, &mailboxName](const auto& storageDescription)
				{
					try {
						switch (storageDescription.storageType) {
						case StorageType::FileSystemMailStorage: {
							storages.push_back(FileSystemStorageFactory(storageDescription.options).create(mailboxName));
							break;
						}
						default:
							break;
						}
					}
					catch (...) {
						//TODO: Протоколировать
					}
				}
			);
			if (storages.empty()) {
				return MailboxOperationError::UserStoragesInstantinationFailure;
			}
			return mailbox_ptr(new Mailbox(mailboxName, std::move(storages), std::move(lock)));
		}
		else {
			return std::get<AuthError>(result);
		}
	}
	else {
		return MailboxOperationError::MailboxIsBusy;
	}
}