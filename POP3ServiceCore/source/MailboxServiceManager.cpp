
#include "MailboxServiceManager.h"
#include "common_headers.h"

std::shared_ptr<AuthorizationManager> MailboxServiceManager::AuthorizationManager;
std::mutex MailboxServiceManager::m_mutex;
std::set<std::string> MailboxServiceManager::activeMailboxes;

bool MailboxServiceManager::LockMailbox(std::string_view _name) {
	std::lock_guard<std::mutex> _lock{ m_mutex };
	const std::string name(_name);
	auto it = activeMailboxes.find(name);
	if (it != activeMailboxes.end()) {
		return false;
	}
	else {
		activeMailboxes.insert(name);
		return true;
	}
}

void MailboxServiceManager::UnlockMailbox(std::string_view name) {
	std::lock_guard<std::mutex> _lock{ m_mutex };
	activeMailboxes.erase(name.data());
}


#include "MailboxLock.h"
#include "FileSystemMailStorage.h"

std::variant<mailbox_ptr, MailboxOperationError, AuthError> MailboxServiceManager::VerifyCredentialsAndConnect(
	std::string_view mailboxName,
	std::string_view password)
{
	MailboxLock lock{ mailboxName };
	if (lock()) {
		auto result = AuthorizationManager->logon(mailboxName, password);
		if (std::holds_alternative<std::vector<MailStorageInfo>>(result)) {
			auto vec = std::get<std::vector<MailStorageInfo>>(result);
			if (vec.empty()) {
				return AuthError::ConsumerHasNoAssociatedMailStorage;
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