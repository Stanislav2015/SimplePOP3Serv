#pragma once

#include <iostream>

enum class POP3Status {
	OK,
	ERR,
	UNKNOWN
};

inline std::ostream& operator<<(std::ostream& out, const POP3Status& val) {
	switch (val)
	{
	case POP3Status::OK:
		out << "+OK";
		break;
	case POP3Status::ERR:
		out << "-ERR";
		break;
	default:
		break;
	}
	return out;
}

inline std::istream& operator>>(std::istream& in, POP3Status& val) {
	std::string stringWCommand;
	in >> stringWCommand;
	if (stringWCommand == "+OK")
		val = POP3Status::OK;
	else if (stringWCommand == "-ERR")
		val = POP3Status::ERR;
	else {
		//Wrong command
		val = POP3Status::UNKNOWN;
	}
	return in;
}

