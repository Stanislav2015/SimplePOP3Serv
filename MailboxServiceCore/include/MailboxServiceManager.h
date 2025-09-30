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
	static bool VerifyName(std::string_view name) { return AuthorizationManager->verifyName(name); }
	
	/// <summary>
	/// VerifyCredentialsAndConnect to mailbox (inclusivelly)
	/// </summary>
	/// <param name="mailboxName"></param>
	/// <param name="password"></param>
	/// <returns></returns>
	static std::variant<mailbox_ptr, MailboxOperationError, AuthError> VerifyCredentialsAndConnect(std::string_view mailboxName, std::string_view password);
	
	static void UnlockMailbox(std::string_view name);
	static bool LockMailbox(std::string_view name);
	static void SetAuthorizationManager(std::unique_ptr<AuthorizationManager> ptr) { 
		AuthorizationManager = std::move(ptr); 
	}
private:
	static std::unique_ptr<AuthorizationManager> AuthorizationManager;
	static std::mutex m_mutex;
	static std::set<std::string> activeMailboxes;
};



