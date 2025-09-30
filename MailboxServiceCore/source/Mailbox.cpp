
#include "Mailbox.h"
#include "common_headers.h"

std::size_t Mailbox::getEmailsCount() const {
	return std::accumulate(storages.cbegin(), storages.cend(), static_cast<std::size_t>(0), [](std::size_t sum, const auto& storage)
		{
			return sum + storage->getEmailsCount();
		}
	);
}

std::map<std::size_t, std::size_t> Mailbox::getEmailsLengths() const {
	std::map<std::size_t, std::size_t> lengths;
	auto it = std::inserter(lengths, lengths.begin());
	std::size_t offset = 0;
	std::for_each(storages.cbegin(), storages.cend(), [&lengths, &offset, &it](const auto& storage)
		{
			auto mp = storage->getEmailsLengths(offset);
			std::copy(mp.cbegin(), mp.cend(), it);
			offset += storage->getEmailsCount();
		}
	);
	return lengths;
}

std::size_t Mailbox::getWholeMailboxSize() const {
	auto mp = getEmailsLengths();
	return std::accumulate(mp.cbegin(), mp.cend(), static_cast<std::size_t>(0), [](auto sum, const auto& pair) { return sum + pair.second; });
}

std::variant<std::size_t, MailboxOperationError> Mailbox::getEmailLength(std::size_t emailNumber) const {
	auto it = findStorage(emailNumber);

	if (it == storages.cend()) {
		return MailboxOperationError::EmailNotFound;
	}

	if ((*it)->isMailMarkedAsDeleted(emailNumber)) {
		return MailboxOperationError::MailIsMarkedAsDeleted;
	}

	return (*it)->getEmailLength(emailNumber);
}

std::variant<std::string, MailboxOperationError> Mailbox::getEmail(std::size_t emailNumber) const {
	auto it = findStorage(emailNumber);

	if (it == storages.cend()) {
		return MailboxOperationError::EmailNotFound;
	}

	return (*it)->getEmail(emailNumber);
}

MailboxOperationError Mailbox::deleteEmail(std::size_t emailNumber) {
	auto it = findStorage(emailNumber);

	if (it == storages.cend()) {
		return MailboxOperationError::EmailNotFound;
	}
	
	return (*it)->deleteEmail(emailNumber);
}

void Mailbox::deleteEmails() {
	std::for_each(storages.cbegin(), storages.cend(), [](const auto& storage) { storage->deleteEmails(); });
}

void Mailbox::reset() {
	std::for_each(storages.cbegin(), storages.cend(), [](const auto& storage) { storage->reset(); });
}

std::vector<std::shared_ptr<MailStorage>>::const_iterator Mailbox::findStorage(std::size_t& emailNumber) const {
	return std::find_if(storages.cbegin(), storages.cend(), [&emailNumber](const auto& storage) {
		auto count = storage->getEmailsCount();
		if (emailNumber < storage->getEmailsCount()) {
			return true;
		}
		else {
			emailNumber -= count;
			return false;
		}
		});
}