#pragma once

#include <windows.h>
#include <iostream>
#include <string>

#ifndef logger_h
#define logger_h

namespace logger {
	void log(std::string message, uint32_t type);
}

#endif