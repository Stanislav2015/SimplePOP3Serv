#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <variant>
#include <Enums.h>

class MailStorage
{
public:
	/// <summary>
	/// Getting the number of emails contained in the storage
	/// </summary>
	/// <returns></returns>
	virtual std::size_t getEmailsCount() const = 0;

	/// <summary>
	/// Getting the lengths of all emails contained in the storage
	/// </summary>
	/// <returns></returns>
	virtual std::map<std::size_t, std::size_t> getEmailsLengths(std::size_t mailNumberOffset = 0) const = 0;

	/// <summary>
	/// Get the length of a particular email in the mailbox
	/// </summary>
	/// <param name="emailNumber">Number of the email</param>
	/// <returns></returns>
	virtual std::size_t getEmailLength(std::size_t emailNumber) const = 0;

	/// <summary>
	/// Get a pointer to an object representing a particular email 
	/// </summary>
	/// <param name="emailNumber">Number of the email</param>
	/// <returns></returns>
	virtual std::variant<std::string, MailboxOperationError> getEmail(std::size_t emailNumber) const = 0;

	/// <summary>
	/// Mark mail as deleted
	/// </summary>
	/// <param name="emailNumber"></param>
	/// <returns></returns>
	MailboxOperationError deleteEmail(std::size_t emailNumber) {
		if (isMailMarkedAsDeleted(emailNumber)) {
			return MailboxOperationError::EmailAlreadyDeleted;
		}
		emailsToBeDeleted.push_back(emailNumber);
		return MailboxOperationError::NoError;
	}

	bool isMailMarkedAsDeleted(std::size_t emailNumber) const {
		if (deleteAll) {
			return true;
		}
		return std::find(emailsToBeDeleted.cbegin(), emailsToBeDeleted.cend(), emailNumber) != emailsToBeDeleted.cend();
	}

	inline void deleteEmails() {
		deleteAll = true;
	}

	/// <summary>
	/// Cancel deleting emails
	/// </summary>
	void reset() {
		deleteAll = false;
		emailsToBeDeleted.clear();
	}

	inline void setUpdateFlag() { updateAtClose = true; }

	virtual ~MailStorage() {}

protected:
	bool updateAtClose{false};
	bool deleteAll{ false };
	std::vector<std::size_t> emailsToBeDeleted;
};