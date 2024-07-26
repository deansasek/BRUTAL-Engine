#include "./engine.h"
#include "../src/core/renderer/renderer.h"
#include "../src/core/modules/input.h"
#include "../src/core/modules/camera.h"

SDL_Window* engine::window;
SDL_Event engine::event;

const char* engine::name = "BRUTAL Engine";
bool engine::running;

uint32_t engine::width = 1280;
uint32_t engine::height = 960;

void engine::init() {
	engine::running = true;

	engine::initWindow();

	input::initializeInput();

	engine::initRenderer();
	engine::mainLoop();
}

void engine::initWindow() {
	if (SDL_Init(SDL_INIT_VIDEO) == 0) {
		logger::log("Successfully initialized video!", 1);
	}
	else {
		logger::log("Error initializing video!", 3);
	}

	engine::window = SDL_CreateWindow(engine::name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, engine::width, engine::height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

	if (engine::window != nullptr) {
		logger::log("Successfully created window!", 1);
	}
	else {
		logger::log("Error creating window!", 3);
	}
}

void engine::initRenderer() {
	renderer::init();
}

void engine::mainLoop() {
	while (engine::running) {
		SDL_PollEvent(&engine::event);

		renderer::drawFrame();

		if (engine::event.type == SDL_QUIT) {
			engine::running = false;
		}
		else if (engine::event.type == SDL_WINDOWEVENT) {
			if (engine::event.window.event == SDL_WINDOWEVENT_MOVED) {
			}
			else if (engine::event.window.event == SDL_WINDOWEVENT_RESIZED) {
				renderer::recreateSwapChain();
			}
		}
		else if (engine::event.type == SDL_KEYDOWN) {
			if (engine::event.key.keysym.sym == SDLK_w) {
				camera::cameraPos.x = camera::cameraPos.x - 1.0f;
			}
			else if (engine::event.key.keysym.sym == SDLK_s) {
				camera::cameraPos.x = camera::cameraPos.x + 1.0f;
			}
			else if (engine::event.key.keysym.sym == SDLK_a) {
				camera::cameraPos.z = camera::cameraPos.z + 1.0f;
			}
			else if (engine::event.key.keysym.sym == SDLK_d) {
				camera::cameraPos.z = camera::cameraPos.z - 1.0f;
			}
		}
	}

	engine::cleanUp();
}

void engine::framebufferResizeCallback(SDL_Window* window, int width, int height) {
	engine::window = SDL_CreateWindow(engine::name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, engine::width, engine::height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
}

void engine::cleanUp() {
	logger::log("Quitting...", 4);

	renderer::cleanup();

	SDL_DestroyWindow(engine::window);

	SDL_Quit();
}