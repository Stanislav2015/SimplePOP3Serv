#pragma once

#include <set>
#include <mutex>

#include "MailStorage.h"
#include "FileSystemMailStorage.h"
#include "Mailbox.h"
#include "UserManager.h"

class MailboxServiceManager : public std::enable_shared_from_this<MailboxServiceManager>
{
public:
	/*class MailboxDeleter {
	public:
		MailboxDeleter(const MailboxDeleter&) = delete;
		MailboxDeleter operator=(const MailboxDeleter&) = delete;

		MailboxDeleter(MailboxDeleter&& moved) noexcept {
			name = moved.name;
			manager = moved.manager;
			moved.manager = nullptr;
		}

		MailboxDeleter() {}

		static std::optional<MailboxDeleter> create(const std::string& name, std::shared_ptr<MailboxServiceManager> manager) {
			if (manager && manager->acquireMailboxIfNotBusy(name)) {
				return MailboxDeleter(name, manager);
			}
			return std::optional<MailboxDeleter>();
		}

		void operator()(Mailbox* ptr) {
			if (ptr) {
				std::default_delete<Mailbox> deleter{};
				deleter(ptr);
			}
		}

		~MailboxDeleter() {
			if (manager) {
				manager->releaseMailbox(name);
			}
		}
	private:
		MailboxDeleter(std::string name, std::shared_ptr<MailboxServiceManager> manager) : name{ name }, manager{manager} {}

		std::shared_ptr<MailboxServiceManager> manager;
		std::string name;
	};*/

	struct busy_mailbox_guard {

		std::shared_ptr<MailboxServiceManager> manager;
		std::string name;
		bool acquired;
		
		busy_mailbox_guard(std::shared_ptr<MailboxServiceManager> manager, std::string name) : 
			manager{ manager }, name{name}, acquired{false}
		{}

		//noncopyable
		busy_mailbox_guard(const busy_mailbox_guard&) = delete;
		busy_mailbox_guard operator=(const busy_mailbox_guard&) = delete;

		busy_mailbox_guard(busy_mailbox_guard&& moved) noexcept {
			name = moved.name;
			acquired = moved.acquired;
			manager = moved.manager;
			moved.release();
		}
		
		bool acquire() {
			if (manager) {
				acquired = manager->acquireMailboxIfNotBusy(name);
			}
			return acquired;
		}

		void release() {
			acquired = false;
		}

		~busy_mailbox_guard() {
			if (acquired && manager) {
				manager->releaseMailbox(name);
			}
		}
	};

	using Mailbox_Ptr = std::unique_ptr<Mailbox>;//, MailboxServiceManager::MailboxDeleter > ;

	MailboxServiceManager(std::shared_ptr<UserManager> userManager) : userManager{ userManager } {}

	/// <summary>
	/// Check user
	/// </summary>
	/// <param name="mailboxName">Name of mailbox</param>
	/// <returns></returns>
	bool isUserRegistered(const std::string& name) const {
		return userManager->isUserRegistered(name);
	}
	
	/// <summary>
	/// Connect to mailbox (inclusivelly)
	/// </summary>
	/// <param name="mailboxName"></param>
	/// <param name="password"></param>
	/// <returns></returns>
	std::variant<Mailbox_Ptr, MailboxOperationError, AuthError> connect(
		const std::string& mailboxName,
		const std::string& password);
	
	void releaseMailbox(const std::string& name);
private:

	bool acquireMailboxIfNotBusy(const std::string& name);
	
	std::shared_ptr<UserManager> userManager;
	std::mutex m_mutex;
	std::set<std::string> activeMailboxes;
};

