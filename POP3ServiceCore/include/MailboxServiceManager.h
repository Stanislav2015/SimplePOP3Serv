#pragma once

#include "Mailbox.h"
#include "AuthorizationManager.h"

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
	static bool VerifyName(const std::string& name) { return AuthorizationManager->verifyName(name); }
	
	/// <summary>
	/// VerifyCredentialsAndConnect to mailbox (inclusivelly)
	/// </summary>
	/// <param name="mailboxName"></param>
	/// <param name="password"></param>
	/// <returns></returns>
	static std::variant<mailbox_ptr, MailboxOperationError, AuthError> VerifyCredentialsAndConnect(const std::string& mailboxName, const std::string& password);
	
	static void UnlockMailbox(const std::string& name);
	static bool LockMailbox(const std::string& name);
	static void SetAuthorizationManager(std::shared_ptr<AuthorizationManager> ptr) { AuthorizationManager = ptr; }
private:
	static std::shared_ptr<AuthorizationManager> AuthorizationManager;
	static std::mutex m_mutex;
	static std::set<std::string> activeMailboxes;
};



