#include "./logger.h";

void logger::log(std::string message, uint32_t type) {

	// 1 = green, 2 = yellow, 3 = error, 4 = standard
	// success, warning, error, normal

	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

	if (type == 1) {
		SetConsoleTextAttribute(h, 10);
	}
	else if (type == 2) {
		SetConsoleTextAttribute(h, 14);
	}
	else if (type == 3) {
		SetConsoleTextAttribute(h, 12);
	}
	else if (type == 4) {
		SetConsoleTextAttribute(h, 7);
	}
	else {
		SetConsoleTextAttribute(h, 12);

		std::cout << "Invalid message type parsed for logger!" << std::endl;

		SetConsoleTextAttribute(h, 7);
	}

	std::cout << message << std::endl;

	SetConsoleTextAttribute(h, 7);
}