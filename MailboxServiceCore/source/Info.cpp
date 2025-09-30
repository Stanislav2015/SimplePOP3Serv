
#include "Info.h"

#include <algorithm>
#include <fstream>

#include <boost/lexical_cast.hpp>
#include <rapidjson/prettywriter.h>

bool MailStorageInfo::Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const {
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


std::optional<MailStorageInfo> MailStorageInfo::Deserialize(const rapidjson::Value& obj) {
	MailStorageInfo settings;
	std::transform(obj.MemberBegin(), obj.MemberEnd(), std::inserter(settings.options, settings.options.begin()), [](const auto& member) {
		return std::make_pair(member.name.GetString(), member.value.GetString());
		});
	auto typeIterator = settings.options.find("type");
	if (typeIterator == settings.options.cend()) {
		return std::optional<MailStorageInfo>();
	}
	settings.storageType = static_cast<StorageType>(boost::lexical_cast<unsigned int>(typeIterator->second));
	if (!checkIfStorageTypeValid(settings.storageType)) {
		return std::optional<MailStorageInfo>();
	}
	settings.options.erase(typeIterator);
	return settings;
}

bool ConsumerInfo::Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const {
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

std::optional<ConsumerInfo> ConsumerInfo::Deserialize(const rapidjson::Value& obj) {
	ConsumerInfo settings;
	if (!obj.HasMember("name") || !obj.HasMember("password")) {
		return std::optional<ConsumerInfo>();
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
		auto result = MailStorageInfo::Deserialize(storageJsonMember);
		if (result) {
			it++ = *result;
		}
	});
	return settings;
}

std::string ConsumerInfo::Serialize() const {
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

bool ConsumerInfo::SerializeToFile(std::string_view _Where) const {
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

std::optional<ConsumerInfo> ConsumerInfo::Deserialize(std::string_view jsonString) {
	using namespace rapidjson;
	Document document;  // Default template parameter uses UTF8 and MemoryPoolAllocator.
	// In-situ parsing, decode strings directly in the source string. Source must be string.
	std::unique_ptr<char[]> buffer{ new char[jsonString.length() + 1] };
	if (!buffer) {
		return std::optional<ConsumerInfo>();
	}
	memcpy(buffer.get(), jsonString.data(), jsonString.length());
	buffer[jsonString.length()] = '\0';
	if (document.ParseInsitu(buffer.get()).HasParseError())
		return std::optional<ConsumerInfo>();
	return Deserialize(document);
}

std::optional<ConsumerInfo> ConsumerInfo::DeserializeFromFile(std::string_view _From) {
	using namespace rapidjson;
	std::ifstream file(_From);
	if (!file.is_open()) {
		return std::optional<ConsumerInfo>();
	}
	std::ostringstream os;
	os << file.rdbuf();
	return Deserialize(os.str());
}

std::optional<ConsumerInfo> FileSystemConsumerInfoStorage::getConsumerInfo(std::string_view name) const {
	std::shared_lock lock(m_mutex);
	auto it = username_vs_filename.find(name.data());
	if (it == username_vs_filename.cend()) {
		return std::optional<ConsumerInfo>();
	}
	return ConsumerInfo::DeserializeFromFile(it->second);
}


bool FileSystemConsumerInfoStorage::addConsumerInfo(const ConsumerInfo& settings) {
	std::unique_lock lock(m_mutex);
	if (username_vs_filename.find(settings.name) != username_vs_filename.cend()) {
		return false;
	}
	auto filepath = path_to_storage / (settings.name + ".json");
	auto result = settings.SerializeToFile(filepath.string());
	if (result) {
		username_vs_filename.emplace(settings.name, filepath.string());
	}
	return result;
}

std::string quickExtractNameFromJson(std::string_view _path) {
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

void FileSystemConsumerInfoStorage::refresh() {
	std::vector<std::filesystem::path> files;
	std::copy_if(std::filesystem::directory_iterator(path_to_storage), std::filesystem::directory_iterator{},
		std::back_inserter(files), [](const auto& path) {
			return std::filesystem::is_regular_file(path);
		});

	std::unique_lock lock{ m_mutex };
	username_vs_filename.clear();
	std::for_each(files.cbegin(), files.cend(), [this](const auto& filename) {
			auto name = quickExtractNameFromJson(filename.string());
			username_vs_filename.emplace(name, filename.string());
		});
}

bool FileSystemConsumerInfoStorage::hasMailbox(std::string_view name) const {
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

std::shared_ptr<ConsumerInfoStorage> FileSystemConsumerInfoStorage::createDefault() {
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
	return std::make_shared<FileSystemConsumerInfoStorage>(path);
}