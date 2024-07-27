#pragma once

#ifndef engine_h
#define engine_h

#include <sdl2/include/SDL.h>
#include <sdl2/include/SDL_vulkan.h>

#include "./core/logger/logger.h"
#include "./core/modules/input.h"

namespace engine {
	extern const char* name;
	extern bool running;

	extern uint32_t width;
	extern uint32_t height;

	extern SDL_Window* window;
	extern SDL_Event event;

	void init();
	void initWindow();
	void initRenderer();
	void mainLoop();
	void cleanUp();

	void framebufferResizeCallback(SDL_Window* window, int width, int height);

	void log(std::string message, uint32_t type);
}

#endif