#pragma once

#include <map>
#include <vector>
#include <filesystem>
#include <string_view>
#include <shared_mutex>
#include <optional>


#include "Enums.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

struct StorageSettings 
{
	StorageType storageType;
	std::map<std::string, std::string> options;

	StorageSettings() : storageType(StorageType::Undefined)
	{}

	StorageSettings(StorageType type, std::map<std::string, std::string> options) : storageType(type), options(options)
	{}

	bool Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;
	static std::optional<StorageSettings> Deserialize(const rapidjson::Value& obj);
};

struct UserSettings
{
	std::string name;
	std::string password;
	std::vector<StorageSettings> storages;

	UserSettings(std::string name, std::string password) : name(name), password(password) {}
	UserSettings() {}

	void addStorage(StorageType type, std::map<std::string, std::string> options = std::map<std::string, std::string>()) {
		storages.emplace_back(type, options);
	}

	bool Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;
	std::string Serialize() const;
	bool SerializeToFile(std::filesystem::path _Where) const;

	static std::optional<UserSettings> Deserialize(const rapidjson::Value& obj);
	static std::optional<UserSettings> Deserialize(std::string_view jsonString);
	static std::optional<UserSettings> DeserializeFromFile(std::filesystem::path _From);
};

class SettingsStorage
{
public:
	virtual std::optional<UserSettings> getUserSettings(std::string_view userName) const = 0;
	virtual bool addUserSettings(const UserSettings&) = 0;
	virtual void refresh() = 0;
	virtual bool hasMailbox(std::string_view) const = 0;
};

class FileSystemSettingsStorage : public SettingsStorage
{
public:
	FileSystemSettingsStorage(std::filesystem::path _path) : SettingsStorage(), path_to_storage(_path) {
		refresh();
	}

	std::optional<UserSettings> getUserSettings(std::string_view userName) const override;
	bool addUserSettings(const UserSettings&) override;
	void refresh() override;
	bool hasMailbox(std::string_view) const override;

	static std::shared_ptr<SettingsStorage> createDefault();

private:
	
	std::filesystem::path path_to_storage;
	std::map<std::string, std::string> username_vs_filename;
	mutable std::shared_mutex m_mutex;
};
