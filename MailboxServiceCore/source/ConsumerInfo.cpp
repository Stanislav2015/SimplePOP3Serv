
#include "ConsumerInfo.h"

#ifdef WIN32
#include <ShlObj_core.h>
#include <Windows.h>
#else
#include <unistd.h>
#endif


std::filesystem::path CreateDefaultImpl() {
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
	return path;
}
