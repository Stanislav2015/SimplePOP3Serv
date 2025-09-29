#pragma once

#include <map>
#include "Enums.h"

struct StorageDescription {
	StorageType storageType;
	std::map<std::string, std::string> options;

	StorageDescription(StorageType t, std::map<std::string, std::string> options) : storageType{ t }, options{ options }
	{
	}
};