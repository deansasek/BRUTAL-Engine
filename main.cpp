#include "./src/engine.h"
#include <iostream>

#undef main

int main() {
	try {
		logger::log(std::string("Starting ") + engine::name + std::string("..."), 4);

		engine::init();
	}
	catch (const std::exception& exception) {
		std::cerr << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}