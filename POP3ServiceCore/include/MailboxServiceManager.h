#pragma once

//#include "MailStorage.h"
//#include "FileSystemMailStorage.h"
#include "Mailbox.h"
#include "UserManager.h"

#include <set>
#include <mutex>

class MailboxServiceManager
{
public:
	MailboxServiceManager() = delete;
	/// <summary>
	/// Check user
	/// </summary>
	/// <param name="mailboxName">Name of mailbox</param>
	/// <returns></returns>
	static bool IsUserRegistered(const std::string& name) { return userManager->isUserRegistered(name); }
	
	/// <summary>
	/// Connect to mailbox (inclusivelly)
	/// </summary>
	/// <param name="mailboxName"></param>
	/// <param name="password"></param>
	/// <returns></returns>
	static std::variant<mailbox_ptr, MailboxOperationError, AuthError> connect(const std::string& mailboxName, const std::string& password);
	
	static void UnlockMailbox(const std::string& name);
	static bool LockMailbox(const std::string& name);
	static void SetUserManager(std::shared_ptr<UserManager> ptr) { userManager = ptr; }
private:
	static std::shared_ptr<UserManager> userManager;
	static std::mutex m_mutex;
	static std::set<std::string> activeMailboxes;
};



