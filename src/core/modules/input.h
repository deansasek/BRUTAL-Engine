#pragma once

#ifndef input_h
#define input_h

#include "../src/engine.h"

namespace input {
	static float mouseX;
	static float mouseY;

	static float scrollX;
	static float scrollY;

	void initializeInput();
	void inputLoop();

	void mouseMotion(float mouseX, float mouseY);
	void mouseScroll(float scrollX, float scrollY);
}

#endif