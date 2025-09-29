
#include "MailboxServiceManager.h"
#include "FileSystemMailStorage.h"
#include "common_headers.h"

bool MailboxServiceManager::acquireMailboxIfNotBusy(const std::string& name) {
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

void MailboxServiceManager::releaseMailbox(const std::string& name) {
	std::lock_guard<std::mutex> _lock{ m_mutex };
	activeMailboxes.erase(name);
}



std::variant<MailboxServiceManager::Mailbox_Ptr, MailboxOperationError, AuthError> MailboxServiceManager::connect(
	const std::string& mailboxName,
	const std::string& password)
{
	//auto deleter_or_null = MailboxDeleter::create(mailboxName, shared_from_this());
	busy_mailbox_guard guard{ shared_from_this(), mailboxName };
	if (guard.acquire()) {
		auto result = userManager->logon(mailboxName, password);
		if (std::holds_alternative<std::vector<StorageDescription>>(result)) {
			auto vec = std::get<std::vector<StorageDescription>>(result);
			if (vec.empty()) {
				return AuthError::UserHasNoStorages;
			}
			std::vector<std::shared_ptr<MailStorage>> storages;
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
			guard.release();  
			return Mailbox_Ptr(new Mailbox(mailboxName, std::move(storages)));
		}
		else {
			return std::get<AuthError>(result);
		}
	}
	else {
		return MailboxOperationError::MailboxIsBusy;
	}
}