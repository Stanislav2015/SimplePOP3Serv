#pragma once

#include <iostream>

enum class POP3CommandType {
	UNKNOWN,
	USER,
	PASS,
	STAT,
	LIST,
	RETR,
	DELE,
	NOOP,
	RSET,
	QUIT
};

inline std::ostream& operator<<(std::ostream& out, const POP3CommandType& val) {
	switch (val)
	{
	case POP3CommandType::USER: 
		out << "USER";
		break;
	case POP3CommandType::PASS:
		out << "PASS";
		break;
	case POP3CommandType::STAT:
		out << "STAT";
		break;
	case POP3CommandType::LIST:
		out << "LIST";
		break;
	case POP3CommandType::RETR:
		out << "RETR";
		break;
	case POP3CommandType::DELE:
		out << "DELE";
		break;
	case POP3CommandType::RSET:
		out << "RSET";
		break;
	case POP3CommandType::NOOP:
		out << "NOOP";
		break;
	case POP3CommandType::QUIT:
		out << "QUIT";
		break;
	default:
		break;
	}
	return out;
}

inline std::istream& operator>>(std::istream& in, POP3CommandType& val) {
	std::string stringWCommand;
	in >> stringWCommand;
	if (stringWCommand == "USER")
		val = POP3CommandType::USER;
	else if (stringWCommand == "PASS")
		val = POP3CommandType::PASS;
	else if (stringWCommand == "STAT")
		val = POP3CommandType::STAT;
	else if (stringWCommand == "LIST")
		val = POP3CommandType::LIST;
	else if (stringWCommand == "RETR")
		val = POP3CommandType::RETR;
	else if (stringWCommand == "DELE")
		val = POP3CommandType::DELE;
	else if (stringWCommand == "RSET")
		val = POP3CommandType::RSET;
	else if (stringWCommand == "NOOP")
		val = POP3CommandType::NOOP;
	else if (stringWCommand == "QUIT")
		val = POP3CommandType::QUIT;
	else {
		//Perhaps wrong command
		val = POP3CommandType::UNKNOWN;
	}
	return in;
}
