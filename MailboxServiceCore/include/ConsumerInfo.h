#pragma once

#include <map>
#include <vector>
#include <filesystem>
#include <string_view>
#include <shared_mutex>
#include <optional>
#include <array>
#include <set>
#include <variant>
#include <fstream>
#include <sstream>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>


#include "Enums.h"
#include "Global.h"
#include "compact_string.h"

 
inline int constexpr length(const char* str) {
	return *str ? 1 + length(str + 1) : 0;
}

inline constexpr static std::size_t getMaximumOptionName() {
	constexpr const char* options[] = {
		"path" //for FileSystemMailStorage
	};
	std::size_t maximum = 0;
	for (int i = 0; i < sizeof(options) / sizeof(char*); i++) {
		auto len = length(options[i]) + 1;
		if (length(options[i]) > maximum) {
			maximum = len;
		}
	}
	return maximum;
}

struct Option {
	using name_type = compact_string::string<getMaximumOptionName()>;
	compact_string::string<getMaximumOptionName()> name;
	std::variant<std::monostate, unsigned int, std::string> value;

	Option() {}

	template<typename T>
	Option(std::string_view name, T value) : name(name), value(value) {}
};

#define MAX_OPTIONS_COUNT 2

struct MailStorageInfo 
{
	StorageType storageType;
	unsigned int count;
	std::array<Option, MAX_OPTIONS_COUNT> options;
	
	MailStorageInfo() : storageType(StorageType::Undefined), count(0) {}
	MailStorageInfo(StorageType t) : storageType(t), count(0) {}

	template<typename ValueType>
	inline void addOption(std::string_view name, ValueType val) {
		assert(count < MAX_OPTIONS_COUNT);
		if (count == MAX_OPTIONS_COUNT) {
			return;
		}
		options[count++] = Option(name, val);
	}

	inline void addOption(const Option& op) {
		assert(count < MAX_OPTIONS_COUNT);
		if (count == MAX_OPTIONS_COUNT) {
			return;
		}
		options[count++] = op;
	}

	const Option& operator[](const unsigned int i) const {
		return options[i];
	}

	inline bool Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const {
		writer.StartObject();

		writer.String("type");
		writer.Uint(static_cast<unsigned int>(storageType));

		std::for_each_n(options.cbegin(), count, [&](const auto& option) {
			assert(!std::holds_alternative<std::monostate>(option.value));
			writer.String(option.name.c_str());
			if (std::holds_alternative<unsigned int>(option.value)) {
				writer.Uint(std::get<unsigned int>(option.value));
			}
			else {
				writer.String(std::get<std::string>(option.value).c_str());
			}
		});

		writer.EndObject();
		return true;
	}

	static std::optional<MailStorageInfo> Deserialize(const rapidjson::Value& obj) {
		using namespace rapidjson;
		MailStorageInfo info;
		std::for_each(obj.MemberBegin(), obj.MemberEnd(), [&info](const auto& member) {
			auto _type = member.value.GetType();
			std::string membName(member.name.GetString());
			if (membName == "type") {
				info.storageType = static_cast<StorageType>(member.value.GetUint());
			}
			else if (_type == Type::kStringType) {
				info.addOption(member.name.GetString(), member.value.GetString());
			}
			else if (_type == Type::kNumberType) {
				info.addOption(member.name.GetString(), member.value.GetUint());
			}
			else {
				assert(false);
			}
		});

		return info;
	}
};

template <typename NameType, typename PasswordType>
struct ConsumerInfo
{
	using _NameType = typename NameType;
	using _PasswordType = typename PasswordType;
	NameType name;
	PasswordType password;
	std::vector<MailStorageInfo> storages;

	ConsumerInfo() {}
	ConsumerInfo(NameType name, PasswordType password) : name(name), password(password) {}
	
	inline void addStorage(StorageType type, const std::map<std::string, std::string>& string_options,
		const std::map<std::string, unsigned int>& numeric_options) {
		MailStorageInfo obj{ type };
		std::for_each(string_options.cbegin(), string_options.cend(), [&obj](const auto& p) { obj.addOption(p.first, p.second); });
		std::for_each(numeric_options.cbegin(), numeric_options.cend(), [&obj](const auto& p) { obj.addOption(p.first, p.second); });
		storages.push_back(obj);
	}

	template<typename AnyPasswordType>
	bool operator<(const ConsumerInfo<NameType, AnyPasswordType>& right) const {
		return name < right.name;
	}

	bool operator<(const NameType& right) const {
		return name < right;
	}

	template<typename AnyPasswordType>
	bool operator>(const ConsumerInfo<NameType, AnyPasswordType>& right) const {
		return name > right.name;
	}

	bool operator>(const NameType& right) const {
		return name > right;
	}

	inline bool Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const {
		writer.StartObject();
		writer.String("name");
		if constexpr (std::is_integral_v<NameType>) {
			writer.Uint64(name);
		}
		else {
			writer.String(name.data());
		}

		writer.String("password");
		if constexpr (std::is_integral_v<PasswordType>) {
			writer.Uint64(password);
		}
		else {
			writer.String(password.c_str());
		}

		writer.String(("storages"));
		writer.StartArray();
		std::for_each(storages.cbegin(), storages.cend(), [&writer](const auto& description) {
			description.Serialize(writer);
			});
		writer.EndArray();
		writer.EndObject();
		return true;
	}

	inline std::string Serialize() const {
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

	inline bool SerializeToFile(std::string_view _Where) const {
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

	inline static std::optional<ConsumerInfo<NameType, PasswordType>> Deserialize(const rapidjson::Value& obj) {
		ConsumerInfo settings;
		if (!obj.HasMember("name") || !obj.HasMember("password")) {
			return std::optional<ConsumerInfo>();
		}

		if constexpr (std::is_integral_v<NameType>) {
			assert(obj["name"].IsUint64());
			if (!obj["name"].IsUint64()) {
				return std::optional<ConsumerInfo>();
			}
			settings.name = obj["name"].GetUint64();
		}
		else {
			settings.name = obj["name"].GetString();
		}

		if constexpr (std::is_integral_v<PasswordType>) {
			assert(obj["password"].IsUint64());
			if (!obj["password"].IsUint64()) {
				return std::optional<ConsumerInfo>();
			}
			settings.password = obj["password"].GetUint64();
		}
		else {
			settings.password = obj["password"].GetString();
		}

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

	inline static std::optional<ConsumerInfo<NameType, PasswordType>> Deserialize(std::string_view jsonString) {
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

	inline static std::optional<ConsumerInfo<NameType, PasswordType>> DeserializeFromFile(std::string_view _From) {
		using namespace rapidjson;
		std::ifstream file(_From);
		if (!file.is_open()) {
			return std::optional<ConsumerInfo>();
		}
		std::ostringstream os;
		os << file.rdbuf();
		return Deserialize(os.str());
	}
};

template<typename NameType>
struct ConsumerInfoComparator
{
	using is_transparent = void;

	template<typename PasswordType>
	bool operator()(const ConsumerInfo<NameType, PasswordType>& left, const ConsumerInfo<NameType, PasswordType>& right) const {
		return left < right;
	}

	template<typename PasswordType>
	bool operator()(const ConsumerInfo<NameType, PasswordType>& left, const NameType& right) const {
		return left < right;
	}

	template<typename PasswordType>
	bool operator()(const NameType& left, const ConsumerInfo<NameType, PasswordType>& right) const {
		return right > left;
	}
};

//Both mailbox name and password are presented as strings
typedef ConsumerInfo<compact_string::string<MAILBOX_NAME_MAX_LEN>, compact_string::string<PASSWORD_MAX_LEN>> BothStringConsumerInfo;
typedef compact_string::compact_string_converter<MAILBOX_NAME_MAX_LEN> BothStringConsumerInfoNameConverter;
typedef compact_string::compact_string_converter<PASSWORD_MAX_LEN> BothStringConsumerInfoPasswordConverter;

//A mailbox's name is presented as a number (hash value), whereas password is presented by a string 
typedef ConsumerInfo<std::size_t, compact_string::string<PASSWORD_MAX_LEN>> HashedNameConsumerInfo;
typedef compact_string::string_view_hash_converter HashedNameConsumerInfoNameConverter;
typedef compact_string::compact_string_converter<PASSWORD_MAX_LEN> HashedNameConsumerInfoPasswordConverter;

//A password is presented as a number (hash value), whereas mailbox's name is presented by a string 
typedef ConsumerInfo<compact_string::string<MAILBOX_NAME_MAX_LEN>, std::size_t> HashedPasswordConsumerInfo;
typedef compact_string::compact_string_converter<MAILBOX_NAME_MAX_LEN> HashedPasswordConsumerInfoNameConverter;
typedef compact_string::string_view_hash_converter HashedPasswordConsumerInfoPasswordConverter;

//Both mailbox name and password are hashes
typedef ConsumerInfo<std::size_t, std::size_t> BothNumericConsumerInfo;
typedef compact_string::string_view_hash_converter BothNumericConsumerInfoNameConverter;
typedef compact_string::string_view_hash_converter BothNumericConsumerInfoPasswordConverter;

template<typename ConsumerInfoType>
class ConsumerInfoStorage
{
public:
	using comparator = ConsumerInfoComparator<typename ConsumerInfoType::_NameType>;
	virtual std::optional<ConsumerInfoType> getConsumerInfo(typename ConsumerInfoType::_NameType) const = 0;
	virtual bool addConsumerInfo(const ConsumerInfoType&) = 0;
	virtual void refresh() = 0;
	virtual bool hasMailbox(typename ConsumerInfoType::_NameType) const = 0;
protected:
	std::set<ConsumerInfoType, comparator> cachedConsumerInfos;
	mutable std::shared_mutex m_mutex;
};

template<typename ConsumerInfoType>
class FileSystemConsumerInfoStorage : public ConsumerInfoStorage<typename ConsumerInfoType>
{
public:
	FileSystemConsumerInfoStorage(std::filesystem::path _path) : ConsumerInfoStorage<ConsumerInfoType>(), path_to_storage(_path) 
	{
		refresh();
	}

	inline std::optional<ConsumerInfoType> getConsumerInfo(typename ConsumerInfoType::_NameType userName) const override {
		std::shared_lock lock(m_mutex);
		auto it = cachedConsumerInfos.find(userName);
		if (it == cachedConsumerInfos.cend()) {
			return std::optional<ConsumerInfoType>();
		}
		lock.unlock();
		return *it;
	}

	inline bool addConsumerInfo(const ConsumerInfoType& info) override {
		std::unique_lock lock(m_mutex);
		if (cachedConsumerInfos.find(info.name) != cachedConsumerInfos.cend()) {
			return false;
		}
		std::string filename;
		if constexpr (std::is_integral_v<typename ConsumerInfoType::_NameType>) {
			filename = std::to_string(info.name);
		}
		else {
			filename = info.name.data();
		}
		auto filepath = path_to_storage / filename;
		auto result = info.SerializeToFile(filepath.string());
		if (result) {
			cachedConsumerInfos.insert(info);
		}
		return result;
	}

	inline void refresh() override {
		std::vector<std::filesystem::path> files;
		std::copy_if(std::filesystem::directory_iterator(path_to_storage), std::filesystem::directory_iterator{},
			std::back_inserter(files), [this](const auto& path) {
				return std::filesystem::is_regular_file(path) && shouldIncludeFile(path);
			});

		std::unique_lock lock{ m_mutex };
		cachedConsumerInfos.clear();
		std::for_each(files.cbegin(), files.cend(), [this](const auto& filepath) {
			auto info = ConsumerInfoType::DeserializeFromFile(filepath.string());
			if (info) {
				cachedConsumerInfos.insert(*info);
			}
		});
	}

	inline bool hasMailbox(typename ConsumerInfoType::_NameType userName) const override {
		std::shared_lock lock(m_mutex);
		auto it = cachedConsumerInfos.find(userName);
		return it != cachedConsumerInfos.cend();
	}

private:
	inline bool shouldIncludeFile(const std::filesystem::path&) { return true; }
	std::filesystem::path path_to_storage;
};

typedef FileSystemConsumerInfoStorage<BothStringConsumerInfo> StandardFileSystemConsumerInfoStorage;
typedef FileSystemConsumerInfoStorage<HashedNameConsumerInfo> HashedNameFileSystemConsumerInfoStorage;
typedef FileSystemConsumerInfoStorage<HashedPasswordConsumerInfo> HashedPasswordFileSystemConsumerInfoStorage;
typedef FileSystemConsumerInfoStorage<BothNumericConsumerInfo> HashedFileSystemConsumerInfoStorage;


std::filesystem::path CreateDefaultImpl();

inline std::unique_ptr<StandardFileSystemConsumerInfoStorage> CreateDefaultFileSystemConsumerInfoStorage() {
	std::filesystem::path path = CreateDefaultImpl();
	return std::make_unique<StandardFileSystemConsumerInfoStorage>(path);
}

inline std::unique_ptr<HashedNameFileSystemConsumerInfoStorage> CreateHashedNameFileSystemConsumerInfoStorage() {
	std::filesystem::path path = CreateDefaultImpl();
	return std::make_unique<HashedNameFileSystemConsumerInfoStorage>(path);
}

inline std::unique_ptr<HashedPasswordFileSystemConsumerInfoStorage> CreateHashedPasswordFileSystemConsumerInfoStorage() {
	std::filesystem::path path = CreateDefaultImpl();
	return std::make_unique<HashedPasswordFileSystemConsumerInfoStorage>(path);
}

inline std::unique_ptr<HashedFileSystemConsumerInfoStorage> CreateHashedFileSystemConsumerInfoStorage() {
	std::filesystem::path path = CreateDefaultImpl();
	return std::make_unique<HashedFileSystemConsumerInfoStorage>(path);
}

