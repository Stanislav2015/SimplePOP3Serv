#pragma once

#include "MailStorage.h"
#include <filesystem>
#include <vector>
#include <memory>
#include <array>
#include <map>
#include <variant>

class FileSystemMailStorage : public MailStorage
{
public:
	FileSystemMailStorage(std::vector<std::filesystem::path> _emails) : MailStorage(), emails(std::move(_emails))
	{
	}

	std::size_t getEmailsCount() const override {
		if (deleteAll) {
			return static_cast<std::size_t>(0);
		}
		return emails.size() - emailsToBeDeleted.size();
	}

	/// <summary>
	/// Get lengths of all emails in the mailbox
	/// </summary>
	/// <returns></returns>
	std::map<std::size_t, std::size_t> getEmailsLengths(std::size_t mailNumberOffset = 0) const override;

	/// <summary>
	/// Get the length of a particular email in the mailbox
	/// </summary>
	/// <param name="emailNumber">Number of the email</param>
	/// <returns></returns>
	std::size_t getEmailLength(std::size_t emailNumber) const override;

	/// <summary>
	/// Get a pointer to an object representing a particular email 
	/// </summary>
	/// <param name="emailNumber">Number of the email</param>
	/// <returns></returns>
	std::variant<std::string, MailboxOperationError> getEmail(std::size_t emailNumber) const override;

	///
	/// Destructor
	/// 
	~FileSystemMailStorage() override;

private:
	static auto read_file(std::ifstream& stream, std::size_t len = 0)->std::string;
	const std::vector<std::filesystem::path> emails;
};

class FileSystemStorageFactory 
{
public:
	FileSystemStorageFactory() 
	{
		setDefaultPath();
		if (mailStoragePath.empty()) {
			throw std::exception("mailStoragePath is empty");
		}
		mailStoragePath /= defaultMiddlePartOfPath;
		if (!std::filesystem::exists(mailStoragePath)) {
			if (!std::filesystem::create_directory(mailStoragePath)) {
				throw std::exception{ "Failed to create mailbox directory" };
			}
		}
	}


	//TODO: Продумать возможные настройки
	FileSystemStorageFactory([[maybe_unused]] std::map<std::string, std::string> options) : FileSystemStorageFactory() {
	}

	std::shared_ptr<FileSystemMailStorage> create(std::string mailboxName) const;

private:
	void setDefaultPath();

	std::filesystem::path mailStoragePath;
	const char* defaultMiddlePartOfPath{ "mailboxes" };
};

