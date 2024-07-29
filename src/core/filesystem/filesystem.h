#pragma once
#ifndef filesystem_h
#define filesystem_h

#include "../src/engine.h"

#include <fstream>
#include <vector>

namespace filesystem {
	extern std::vector<char> readFile(const std::string& fileName);
}

#endif