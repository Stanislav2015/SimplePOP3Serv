#pragma once

#include "Info.h"
#include <variant>

class AuthorizationManager
{
public:
	virtual bool verifyName(std::string_view name) const = 0;
	
	std::variant<AuthError, std::vector<MailStorageInfo>> logon(std::string_view name, std::string_view password) {
		if (!verifyName(name)) {
			return AuthError::NoSuchConsumer;
		}
		if (!verifyCredentials(name, password)) {
			return AuthError::InvalidPassword;
		}
		return getMailStoragesAssociatedWithConsumer(name);
	}

protected:
	virtual std::vector<MailStorageInfo> getMailStoragesAssociatedWithConsumer(std::string_view name) const = 0;
	virtual bool verifyCredentials(std::string_view name, std::string_view password) const = 0;
};

/// <summary>
/// Dummy implementation of AuthorizationManager Interface which allows all connections. 
/// </summary>
class DummyAuthorizationManager : public AuthorizationManager
{
public:
	bool verifyName(std::string_view name) const override {
		return true; 
	}
protected:
	
	bool verifyCredentials(std::string_view name, std::string_view password) const override {
		return true; 
	}

	std::vector<MailStorageInfo> getMailStoragesAssociatedWithConsumer(std::string_view name) const override {
		std::vector<MailStorageInfo> vec;

		{
			std::map<std::string, std::string> fileSystemStorageOptions;
			vec.emplace_back(StorageType::FileSystemMailStorage, std::move(fileSystemStorageOptions));
		}

		return vec;
	}
};

class SingleConsumerInfoStorageAuthorizationManager : public AuthorizationManager
{
public:
	SingleConsumerInfoStorageAuthorizationManager(std::shared_ptr<ConsumerInfoStorage> ptr) : storage(ptr) {}

	bool verifyName(std::string_view name) const override {
		assert(storage);
		return storage->hasMailbox(name);
	}

private:
	bool verifyCredentials(std::string_view name, std::string_view password) const override {
		assert(storage);
		auto settings = storage->getConsumerInfo(name);
		return settings && settings->password == password;
	}

	std::vector<MailStorageInfo> getMailStoragesAssociatedWithConsumer(std::string_view name) const override {
		assert(storage);
		auto settings = storage->getConsumerInfo(name);
		if (!settings) {
			return std::vector<MailStorageInfo>();
		}
		return settings->storages;
	}

	std::shared_ptr<ConsumerInfoStorage> storage;
};