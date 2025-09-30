#pragma once

#include "Info.h"

class AuthorizationManager
{
public:
	virtual bool verifyName(const std::string& name) const = 0;
	
	std::variant<AuthError, std::vector<MailStorageInfo>> logon(const std::string& name, const std::string& password) {
		if (!verifyName(name)) {
			return AuthError::NoSuchConsumer;
		}
		if (!verifyCredentials(name, password)) {
			return AuthError::InvalidPassword;
		}
		return getMailStoragesAssociatedWithConsumer(name);
	}

protected:
	virtual std::vector<MailStorageInfo> getMailStoragesAssociatedWithConsumer(const std::string& name) const = 0;
	virtual bool verifyCredentials(const std::string& name, const std::string& password) const = 0;
};

/// <summary>
/// Dummy implementation of AuthorizationManager Interface which allows all connections. 
/// </summary>
class DummyAuthorizationManager : public AuthorizationManager
{
public:
	bool verifyName(const std::string& name) const override {
		return true; 
	}
protected:
	
	bool verifyCredentials(const std::string& name, const std::string& password) const override { 
		return true; 
	}

	std::vector<MailStorageInfo> getMailStoragesAssociatedWithConsumer(const std::string& name) const override {
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

	bool verifyName(const std::string& name) const override {
		assert(storage);
		return storage->hasMailbox(name);
	}

	/*inline void setStorage(std::shared_ptr<ConsumerInfoStorage> ptr) {
		storage = ptr; 
	}*/
private:
	bool verifyCredentials(const std::string& name, const std::string& password) const override {
		assert(storage);
		auto settings = storage->getConsumerInfo(name);
		return settings && settings->password == password;
	}

	std::vector<MailStorageInfo> getMailStoragesAssociatedWithConsumer(const std::string& name) const override {
		assert(storage);
		auto settings = storage->getConsumerInfo(name);
		if (!settings) {
			return std::vector<MailStorageInfo>();
		}
		return settings->storages;
	}

	std::shared_ptr<ConsumerInfoStorage> storage;
};