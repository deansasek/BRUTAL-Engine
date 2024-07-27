#pragma once

#ifndef input_h
#define input_h

#include "../modules/camera.h"
#include "../renderer/renderer.h"

namespace input {
	static float mouseX;
	static float mouseY;

	void initializeInput();
	void inputLoop();

	void mouseMotion(float mouseX, float mouseY);
}

#endif