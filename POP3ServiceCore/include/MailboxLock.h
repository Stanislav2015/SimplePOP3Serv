#pragma once

#include <string>

class MailboxLock {
public:
	MailboxLock(std::string name);

	//noncopyable
	MailboxLock(const MailboxLock&) = delete;
	MailboxLock operator=(const MailboxLock&) = delete;
	//move constructor
	MailboxLock(MailboxLock&& moved) noexcept {
		name = moved.name;
		acquired = moved.acquired;
		moved.acquired = false;
	}

	bool operator()() {
		return acquired;
	}

	~MailboxLock();
private:
	std::string name;
	bool acquired;
};