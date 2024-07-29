#include "./filesystem.h"

extern std::vector<char> filesystem::readFile(const std::string& fileName) {
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file!");
	}
	else {
		logger::log("Successfully opened file: " + std::string(fileName), 1);
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	logger::log("File size: " + std::to_string(fileSize) + " bytes", 4);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}