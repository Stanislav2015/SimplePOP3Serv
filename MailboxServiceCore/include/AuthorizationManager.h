#pragma once

#include "ConsumerInfo.h"
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

	std::vector<MailStorageInfo> getMailStoragesAssociatedWithConsumer(std::string_view name) const override 
	{	
		std::vector<MailStorageInfo> vec;

		{
			MailStorageInfo info(StorageType::FileSystemMailStorage);
			vec.push_back(info);
		}

		return vec;
	}
};

template<typename ConsumerInfoType>
class SingleConsumerInfoStorageAuthorizationManager : public AuthorizationManager
{
public:

	SingleConsumerInfoStorageAuthorizationManager(std::unique_ptr<ConsumerInfoStorage<ConsumerInfoType>> ptr) : storage(std::move(ptr)) {}

	bool verifyName(std::string_view name) const override {
		assert(storage);
		if constexpr (std::is_integral_v<ConsumerInfoType::_NameType>) {
			constexpr static auto nameConv = compact_string::string_view_hash_converter();
			return storage->hasMailbox(nameConv(name));
		}
		else {
			constexpr static auto nameConv = compact_string::compact_string_converter<ConsumerInfoType::_NameType::const_size>>();
			return storage->hasMailbox(nameConv(name));
		}
	}

private:
	bool verifyCredentials(std::string_view name, std::string_view password) const override {
		assert(storage);
		if constexpr (std::is_integral_v<ConsumerInfoType::_NameType>) {
			constexpr static auto nameConv = compact_string::string_view_hash_converter();
			auto settings = storage->getConsumerInfo(nameConv(name));
			if constexpr (std::is_integral_v<ConsumerInfoType::_PasswordType>) {
				constexpr static auto passConv = compact_string::string_view_hash_converter();
				return settings && settings->password == passConv(password);
			}
			else {
				constexpr static auto passConv = compact_string::compact_string_converter<ConsumerInfoType::_PasswordType::const_size>>();
				return settings && settings->password == passConv(password);
			}
		}
		else {
			constexpr static auto nameConv = compact_string::compact_string_converter<ConsumerInfoType::_NameType::const_size>();
			auto settings = storage->getConsumerInfo(nameConv(name));
			if constexpr (std::is_integral_v<ConsumerInfoType::_PasswordType>) {
				constexpr static auto passConv = compact_string::string_view_hash_converter();
				return settings && settings->password == passConv(password);
			}
			else {
				constexpr static auto passConv = compact_string::compact_string_converter<ConsumerInfoType::_PasswordType::const_size >> ();
				return settings && settings->password == passConv(password);
			}
		}
	}

	std::vector<MailStorageInfo> getMailStoragesAssociatedWithConsumer(std::string_view name) const override {
		assert(storage);
		if constexpr (std::is_integral_v<ConsumerInfoType::_NameType>) {
			constexpr static auto nameConv = compact_string::string_view_hash_converter();
			auto settings = storage->getConsumerInfo(nameConv(name));
			if (!settings) {
				return std::vector<MailStorageInfo>();
			}
			return settings->storages;
		}
		else {
			constexpr static auto nameConv = compact_string::compact_string_converter<ConsumerInfoType::_NameType::const_size>();
			auto settings = storage->getConsumerInfo(nameConv(name));
			if (!settings) {
				return std::vector<MailStorageInfo>();
			}
			return settings->storages;
		}
	}

	std::unique_ptr<ConsumerInfoStorage<ConsumerInfoType>> storage;
};