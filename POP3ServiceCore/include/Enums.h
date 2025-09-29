#pragma once

#include <iostream>

enum class StorageType
{
	FileSystemMailStorage
};

enum class AuthError
{
	UserIsNotRegistered,
	InvalidPassword,
	UserHasNoStorages
};

inline std::ostream& operator<<(std::ostream& out, const AuthError& err) {
	std::string message;
	switch (err)
	{
	case AuthError::UserIsNotRegistered:
		message = "sorry, no mailbox for such user here";
		break;
	case AuthError::InvalidPassword:
		message = "invalid password";
		break;
	case AuthError::UserHasNoStorages:
		message = "login and password are corrent, however default mail storage is not set for this user";
		break;
	default:
		message = "";
		break;
	}
	out << message;
	return out;
}

enum class MailboxOperationError {
	NoError,
	MailboxAlreadyExists,
	MailboxIsBusy,
	MailboxNotExist,
	MailboxCreateError,
	EmailNotFound,
	UserStoragesInstantinationFailure,
	EmailAlreadyDeleted,
	MailIsMarkedAsDeleted,
	InternalError
};
