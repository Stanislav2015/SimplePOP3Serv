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

struct MailStorageInfo 
{
	StorageType storageType;
	std::map<std::string, std::string> options;

	MailStorageInfo() : storageType(StorageType::Undefined)
	{}

	MailStorageInfo(StorageType type, std::map<std::string, std::string> options) : storageType(type), options(options)
	{}

	bool Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;
	static std::optional<MailStorageInfo> Deserialize(const rapidjson::Value& obj);
};

struct ConsumerInfo
{
	std::string name;
	std::string password;
	std::vector<MailStorageInfo> storages;

	ConsumerInfo(std::string name, std::string password) : name(name), password(password) {}
	ConsumerInfo() {}

	void addStorage(StorageType type, std::map<std::string, std::string> options = std::map<std::string, std::string>()) {
		storages.emplace_back(type, options);
	}

	bool Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;
	std::string Serialize() const;
	bool SerializeToFile(std::string_view _Where) const;

	static std::optional<ConsumerInfo> Deserialize(const rapidjson::Value& obj);
	static std::optional<ConsumerInfo> Deserialize(std::string_view jsonString);
	static std::optional<ConsumerInfo> DeserializeFromFile(std::string_view _From);
};

class ConsumerInfoStorage
{
public:
	virtual std::optional<ConsumerInfo> getConsumerInfo(std::string_view userName) const = 0;
	virtual bool addConsumerInfo(const ConsumerInfo&) = 0;
	virtual void refresh() = 0;
	virtual bool hasMailbox(std::string_view) const = 0;
};

class FileSystemConsumerInfoStorage : public ConsumerInfoStorage
{
public:
	FileSystemConsumerInfoStorage(std::filesystem::path _path) : ConsumerInfoStorage(), path_to_storage(_path) {
		refresh();
	}

	std::optional<ConsumerInfo> getConsumerInfo(std::string_view userName) const override;
	bool addConsumerInfo(const ConsumerInfo&) override;
	void refresh() override;
	bool hasMailbox(std::string_view) const override;

	static std::unique_ptr<ConsumerInfoStorage> Default();

private:
	
	std::filesystem::path path_to_storage;
	std::map<std::string, std::string> username_vs_filename;
	mutable std::shared_mutex m_mutex;
};
