#pragma once

#include "POP3CommandType.h"
#include <optional>
#include <variant>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

struct POP3Command {
	POP3CommandType cmdType;
	std::variant<std::monostate, unsigned int, std::string> parameter;

	POP3Command() : cmdType{ POP3CommandType::NOOP } {

	}

	static bool supportsUintParameter(POP3CommandType type){
		return type == POP3CommandType::LIST || type == POP3CommandType::RETR || type == POP3CommandType::DELE;
	}

	static bool supportStringParameter(POP3CommandType type) {
		return type == POP3CommandType::USER || type == POP3CommandType::PASS;
	}
};

enum class ParsingError {
	EmptyString,
	InvalidFormat,
	UnknownCommand,
	InvalidUintParameter,
	UserNameRequired,
	PasswordRequired,
	MailNumberRequired
};

inline std::ostream& operator<<(std::ostream& out, const ParsingError& err) {
	std::string message;
	switch (err)
	{
	case ParsingError::EmptyString:
		message = "empty line has been sent";
		break;
	case ParsingError::InvalidFormat:
		message = "command line has invalid format";
		break;
	case ParsingError::UnknownCommand:
		message = "unknown command";
		break;
	case ParsingError::InvalidUintParameter:
		message = "expected integer parameter, but received something else";
		break;
	case ParsingError::UserNameRequired:
		message = "user name required";
		break;
	case ParsingError::PasswordRequired:
		message = "password required";
		break;
	case ParsingError::MailNumberRequired:
		message = "in RETR command mail number required";
		break;
	default:
		break;
	}
	out << message;
	return out;
}

inline std::variant<POP3Command, ParsingError> parsePOP3Command(const std::string& cmdLine) {
	using namespace boost;
	using namespace boost::algorithm;
	if (cmdLine.empty()) {
		return ParsingError::EmptyString;
	}
	constexpr std::size_t reserve_enough = 4;
	std::vector<std::string> vv;
	vv.reserve(reserve_enough);
	split(vv, cmdLine, boost::is_space());
	std::vector<std::string> v;
	v.reserve(vv.size());
	std::copy_if(vv.cbegin(), vv.cend(), std::back_inserter(v), [](const auto& s) { return !s.empty(); });
	POP3Command cmd;
	try {
		cmd.cmdType = lexical_cast<POP3CommandType>(v[0]);
	}
	catch (...) {
		return ParsingError::UnknownCommand;
	}

	if (cmd.cmdType == POP3CommandType::UNKNOWN) {
		return ParsingError::UnknownCommand;
	}

	if (v.size() > 1) {
		if (POP3Command::supportsUintParameter(cmd.cmdType)) {
			try {
				unsigned int param = lexical_cast<unsigned int>(v[1]);
				cmd.parameter = param;
			}
			catch (bad_lexical_cast) {
				return ParsingError::InvalidUintParameter;
			}
		}
		else if (POP3Command::supportStringParameter(cmd.cmdType)) {
			cmd.parameter = v[1];
		}
		/*else {
			return ParsingError::InvalidFormat;
		}*/
	}

	if (cmd.cmdType == POP3CommandType::USER && !std::holds_alternative<std::string>(cmd.parameter)) {
		return ParsingError::UserNameRequired;
	}

	if (cmd.cmdType == POP3CommandType::PASS && !std::holds_alternative<std::string>(cmd.parameter)) {
		return ParsingError::PasswordRequired;
	}

	if (cmd.cmdType == POP3CommandType::RETR && !std::holds_alternative<unsigned int>(cmd.parameter)) {
		return ParsingError::MailNumberRequired;
	}

	return cmd;
}
