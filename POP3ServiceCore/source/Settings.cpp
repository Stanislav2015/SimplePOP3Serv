
#include "Settings.h"

#include <algorithm>
#include <fstream>

#include <boost/lexical_cast.hpp>
#include <rapidjson/prettywriter.h>

bool StorageSettings::Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const {
	writer.StartObject();

	writer.String("type");
	writer.String(boost::lexical_cast<std::string>(static_cast<unsigned int>(storageType)).c_str());

	std::for_each(options.cbegin(), options.cend(), [&writer](const auto& pair) {
		writer.String(pair.first.c_str());
		writer.String(pair.second.c_str());
		});

	writer.EndObject();
	return true;
}


std::optional<StorageSettings> StorageSettings::Deserialize(const rapidjson::Value& obj) {
	StorageSettings settings;
	std::transform(obj.MemberBegin(), obj.MemberEnd(), std::inserter(settings.options, settings.options.begin()), [](const auto& member) {
		return std::make_pair(member.name.GetString(), member.value.GetString());
		});
	auto typeIterator = settings.options.find("type");
	if (typeIterator == settings.options.cend()) {
		return std::optional<StorageSettings>();
	}
	settings.storageType = static_cast<StorageType>(boost::lexical_cast<unsigned int>(typeIterator->second));
	if (!checkIfStorageTypeValid(settings.storageType)) {
		return std::optional<StorageSettings>();
	}
	settings.options.erase(typeIterator);
	return settings;
}

bool UserSettings::Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const {
	writer.StartObject();
	writer.String("name");
	writer.String(name.c_str());
	writer.String("password");
	writer.String(password.c_str());

	writer.String(("storages"));
	writer.StartArray();
	std::for_each(storages.cbegin(), storages.cend(), [&writer](const auto& description) {
			description.Serialize(writer);
		});
	writer.EndArray();
	writer.EndObject();
	return true;
}

std::optional<UserSettings> UserSettings::Deserialize(const rapidjson::Value& obj) {
	UserSettings settings;
	if (!obj.HasMember("name") || !obj.HasMember("password")) {
		return std::optional<UserSettings>();
	}
	settings.name = (obj["name"].GetString());
	settings.password = (obj["password"].GetString());
	if (!obj.HasMember("storages") || !obj["storages"].IsArray()) {
		return settings;
	}
	auto stores = obj["storages"].GetArray();
	settings.storages.reserve(stores.Size());
	auto it = std::back_inserter(settings.storages);
	std::for_each(stores.Begin(), stores.End(), [&it](const auto& storageJsonMember) {
		auto result = StorageSettings::Deserialize(storageJsonMember);
		if (result) {
			it++ = *result;
		}
	});
	return settings;
}

std::string UserSettings::Serialize() const {
	using namespace rapidjson;
	StringBuffer sb;
	PrettyWriter<StringBuffer> writer(sb);
	if (Serialize(writer)) {
		return sb.GetString();
	}
	else {
		return "";
	}
}

bool UserSettings::SerializeToFile(std::filesystem::path _Where) const {
	std::ofstream file(_Where);
	if (!file.is_open()) {
		return false;
	}
	auto jsonString = Serialize();
	if (jsonString.empty()) {
		return false;
	}
	file << jsonString;
	return true;
}

std::optional<UserSettings> UserSettings::Deserialize(std::string_view jsonString) {
	using namespace rapidjson;
	Document document;  // Default template parameter uses UTF8 and MemoryPoolAllocator.
	// In-situ parsing, decode strings directly in the source string. Source must be string.
	char* buffer = new char[jsonString.length() + 1];
	memcpy(buffer, jsonString.data(), jsonString.length());
	buffer[jsonString.length()] = '\0';
	if (document.ParseInsitu(buffer).HasParseError())
		return std::optional<UserSettings>();
	return Deserialize(document);
}

std::optional<UserSettings> UserSettings::DeserializeFromFile(std::filesystem::path _From) {
	using namespace rapidjson;
	std::ifstream file(_From);
	if (!file.is_open()) {
		return std::optional<UserSettings>();
	}
	std::ostringstream os;
	os << file.rdbuf();
	return Deserialize(os.str());
}

std::optional<UserSettings> FileSystemSettingsStorage::getUserSettings(std::string_view name) const {
	std::shared_lock lock(m_mutex);
	auto it = std::find_if(username_vs_filename.cbegin(), username_vs_filename.cend(), [&name](const auto& pair) {
			return pair.first == name;
		});
	if (it == username_vs_filename.cend()) {
		return std::optional<UserSettings>();
	}
	return UserSettings::DeserializeFromFile(it->second);
}


bool FileSystemSettingsStorage::addUserSettings(const UserSettings& settings) {
	std::unique_lock lock(m_mutex);
	if (username_vs_filename.find(settings.name) != username_vs_filename.cend()) {
		return false;
	}
	auto filepath = path_to_storage / (settings.name + ".json");
	auto result = settings.SerializeToFile(filepath);
	if (result) {
		username_vs_filename.emplace(settings.name, filepath.string());
	}
	return result;
}

std::string quickExtractNameFromJson(std::filesystem::path _path) {
	std::ifstream file(_path);
	if (!file.is_open()) {
		return "";
	}
	std::ostringstream os;
	os << file.rdbuf();
	std::string s = os.str();
	auto offset = s.find("name");
	offset = s.find('\"', offset + sizeof("name") + 1);
	auto next_offset = s.find('\"', offset + 1);
	return s.substr(offset + 1, next_offset - offset - 1);
}

void FileSystemSettingsStorage::refresh() {
	std::vector<std::filesystem::path> files;
	std::copy_if(std::filesystem::directory_iterator(path_to_storage), std::filesystem::directory_iterator{},
		std::back_inserter(files), [](const auto& path) {
			return std::filesystem::is_regular_file(path);
		});

	std::unique_lock lock{ m_mutex };
	username_vs_filename.clear();
	std::for_each(files.cbegin(), files.cend(), [this](const auto& filename) {
			auto name = quickExtractNameFromJson(filename);
			username_vs_filename.emplace(name, filename.string());
		});
}

bool FileSystemSettingsStorage::hasMailbox(std::string_view name) const {
	std::shared_lock lock(m_mutex);
	auto it = std::find_if(username_vs_filename.cbegin(), username_vs_filename.cend(), [&name](const auto& pair) {
		return pair.first == name;
		});
	return it != username_vs_filename.cend();
}

#ifdef WIN32
#include <ShlObj_core.h>
#include <Windows.h>
#else
#include <unistd.h>
#endif

std::shared_ptr<SettingsStorage> FileSystemSettingsStorage::createDefault() {
	std::filesystem::path path;
#ifdef WIN32
	PWSTR homeDir = nullptr;
	SHGetKnownFolderPath(FOLDERID_Profile, KF_FLAG_DEFAULT, nullptr, &homeDir);
	path = homeDir;
	CoTaskMemFree(homeDir);
#else
	char* homeDir = getenv("HOME");
	path = homeDir;
#endif
	path /= "mailclients";
	std::filesystem::create_directory(path);
	return std::make_shared<FileSystemSettingsStorage>(path);
}