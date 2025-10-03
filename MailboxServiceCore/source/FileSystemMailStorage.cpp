
#include "FileSystemMailStorage.h"
#include <numeric>
#include <fstream>
#include <sstream>

std::filesystem::path FileSystemStorageFactory::defaultPath = "";

#ifdef WIN32
#include <ShlObj_core.h>
#include <Windows.h>
#else
#include <unistd.h>
#endif

std::map<std::size_t, std::size_t> FileSystemMailStorage::getEmailsLengths(std::size_t mailNumberOffset) const {
	std::map<std::size_t, std::size_t> lengths;
	for (std::size_t i = 0; i < emails.size(); i++) {
		if (!isMailMarkedAsDeleted(i)) {
			lengths.emplace(i + mailNumberOffset, getEmailLength(i));
		}
	}
	return lengths;
}

std::size_t FileSystemMailStorage::getEmailLength(std::size_t emailNumber) const {
	return std::filesystem::file_size(emails[emailNumber]);
}

std::variant<std::string, MailboxOperationError> FileSystemMailStorage::getEmail(std::size_t emailNumber) const {
	if (isMailMarkedAsDeleted(emailNumber)) {
		return MailboxOperationError::MailIsMarkedAsDeleted;
	}
	auto len = std::filesystem::file_size(emails[emailNumber]);
	std::ifstream file{ emails[emailNumber] };
	if (!file.is_open()) {
		return MailboxOperationError::InternalError;
	}
	std::ostringstream os;
	os << file.rdbuf();
	return os.str();
	//return read_file(fis, len);
}

auto FileSystemMailStorage::read_file(std::ifstream& stream, std::size_t len) -> std::string {
	constexpr auto read_size = std::size_t{ 4096 };
	stream.exceptions(std::ios_base::badbit);

	auto out = std::string{};
	out.reserve(len);
	auto buf = std::string(read_size, '\0');
	while (stream.read(&buf[0], read_size)) {
		out.append(buf, 0, stream.gcount());
	}
	out.append(buf, 0, stream.gcount());
	return out;
}

FileSystemMailStorage::~FileSystemMailStorage() {
	if (updateAtClose) {
		std::for_each(emailsToBeDeleted.cbegin(), emailsToBeDeleted.cend(), [this](const std::size_t& number) {
			std::filesystem::remove(emails[number]);
			});
	}
}

std::shared_ptr<FileSystemMailStorage> FileSystemStorageFactory::create(const MailStorageInfo& info, 
	[[maybe_unused]] std::string_view name)  
{
	std::filesystem::path path;
	for (unsigned int i = 0; i < info.count; i++) {
		if (info[i].name == Option::name_type("path")) {
			path = std::get<std::string>(info[i].value);
		}
	}

	if (path.empty()) {
		path = defaultPath / name;
	}

	if (!std::filesystem::exists(path)) {
		if (!std::filesystem::create_directory(path)) {
			throw std::exception{ "Failed to create mailbox directory" };
		}
	}

	std::vector<std::filesystem::path> emails;
	std::copy_if(std::filesystem::directory_iterator(path), std::filesystem::directory_iterator{},
		std::back_inserter(emails), [](const auto& path) { return std::filesystem::is_regular_file(path); });
	return std::make_shared<FileSystemMailStorage>(emails);
}

/*
void FileSystemStorageFactory::setDefaultPath() {
#ifdef WIN32
	PWSTR homeDir = nullptr;
	SHGetKnownFolderPath(FOLDERID_Profile, KF_FLAG_DEFAULT, nullptr, &homeDir);
	mailStoragePath = homeDir;
	CoTaskMemFree(homeDir);
#else
	char* homeDir = getenv("HOME");
	mailStoragePath = homeDir;
#endif
}
*/