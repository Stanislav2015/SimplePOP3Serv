#pragma once

#include "Settings.h"

class UserManager
{
public:
	virtual bool isUserRegistered(const std::string& name) const = 0;
	
	std::variant<AuthError, std::vector<StorageSettings>> logon(const std::string& name, const std::string& password) {
		if (!isUserRegistered(name)) {
			return AuthError::UserIsNotRegistered;
		}
		if (!checkPassword(name, password)) {
			return AuthError::InvalidPassword;
		}
		return getUserStoragesList(name);
	}

protected:
	virtual std::vector<StorageSettings> getUserStoragesList(const std::string& name) const = 0;
	virtual bool checkPassword(const std::string& name, const std::string& password) const = 0;
};

/// <summary>
/// Dummy implementation of User Manager Interface which always returns true. 
/// </summary>
class DummyUserManager : public UserManager
{
public:
	bool isUserRegistered(const std::string& name) const override { 
		return true; 
	}
protected:
	
	bool checkPassword(const std::string& name, const std::string& password) const override { 
		return true; 
	}

	std::vector<StorageSettings> getUserStoragesList(const std::string& name) const override {
		std::vector<StorageSettings> vec;

		{
			std::map<std::string, std::string> fileSystemStorageOptions;
			vec.emplace_back(StorageType::FileSystemMailStorage, std::move(fileSystemStorageOptions));
		}

		return vec;
	}
};

class SingleStorageUserManager : public UserManager
{
public:
	SingleStorageUserManager(std::shared_ptr<SettingsStorage> ptr) : storage(ptr) {}

	bool isUserRegistered(const std::string& name) const override {
		assert(storage);
		return storage->hasMailbox(name);
	}

	/*inline void setStorage(std::shared_ptr<SettingsStorage> ptr) {
		storage = ptr; 
	}*/
private:
	bool checkPassword(const std::string& name, const std::string& password) const override {
		assert(storage);
		auto settings = storage->getUserSettings(name);
		return settings && settings->password == password;
	}

	std::vector<StorageSettings> getUserStoragesList(const std::string& name) const override {
		assert(storage);
		auto settings = storage->getUserSettings(name);
		if (!settings) {
			return std::vector<StorageSettings>();
		}
		return settings->storages;
	}

	std::shared_ptr<SettingsStorage> storage;
};