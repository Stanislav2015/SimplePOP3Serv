#pragma once

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <algorithm>
#include <functional>
#include "MailStorage.h"
#include "Enums.h"

class Mailbox
{
public:
	using storage_ptr = std::shared_ptr<MailStorage>;

	Mailbox(std::string mailboxName, std::vector<storage_ptr> storages) : 
		name{ std::move(mailboxName) }, storages{ std::move(storages) }
	{
	}

	//noncopyable
	Mailbox(const Mailbox&) = delete;
	Mailbox operator=(const Mailbox&) = delete;

	inline std::string getName() const { return name; }

	/// <summary>
	/// Get the number of emails in the mailbox
	/// </summary>
	/// <returns></returns>
	std::size_t getEmailsCount() const;

	/// <summary>
	/// Get lengths of all emails in the mailbox
	/// </summary>
	/// <returns></returns>
	std::map<std::size_t, std::size_t> getEmailsLengths() const;

	/// <summary>
	/// Get size of all mails
	/// </summary>
	/// <returns></returns>
	std::size_t getWholeMailboxSize() const;

	/// <summary>
	/// Get the length of a particular email in the mailbox
	/// </summary>
	/// <param name="emailNumber">Number of the email</param>
	/// <returns></returns>
	std::variant<std::size_t, MailboxOperationError> getEmailLength(std::size_t emailNumber) const;

	/// <summary>
	/// Get a pointer to an object representing a particular email 
	/// </summary>
	/// <param name="emailNumber">Number of the email</param>
	/// <returns></returns>
	std::variant<std::string, MailboxOperationError> getEmail(std::size_t emailNumber) const;

	/// <summary>
	/// Mark a particular email as deleted
	/// </summary>
	/// <param name="emailNumber">Number of the email</param>
	/// <returns></returns>
	MailboxOperationError deleteEmail(std::size_t emailNumber);

	/// <summary>
	/// Mark all emails as deleted
	/// </summary>
	void deleteEmails();

	/// <summary>
	/// Cancel deleting emails
	/// </summary>
	void reset();

	inline void setUpdate() { 
		std::for_each(storages.cbegin(), storages.cend(), [](const auto& storage) { storage->setUpdateFlag(); });
	}

	//destructor
	virtual ~Mailbox() {
		//child classes' destructors may do something
	}

protected:
	std::vector<storage_ptr>::const_iterator findStorage(std::size_t& emailNumber) const;
	std::string name;
	std::vector<storage_ptr> storages;
};


