#pragma once

#include "StorageDescription.h"

class UserManager
{
public:
	virtual bool isUserRegistered(const std::string& name) const = 0;
	
	std::variant<AuthError, std::vector<StorageDescription>> logon(const std::string& name, const std::string& password) {
		if (!isUserRegistered(name)) {
			return AuthError::UserIsNotRegistered;
		}
		if (!checkPassword(name, password)) {
			return AuthError::InvalidPassword;
		}
		return getUserStoragesList(name);
	}

protected:
	virtual std::vector<StorageDescription> getUserStoragesList(const std::string& name) const = 0;
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

	std::vector<StorageDescription> getUserStoragesList(const std::string& name) const override {
		std::vector<StorageDescription> vec;
		{
			std::map<std::string, std::string> fileSystemStorageOptions;
			vec.emplace_back(StorageType::FileSystemMailStorage, std::move(fileSystemStorageOptions));
		}
		return vec;
	}
};