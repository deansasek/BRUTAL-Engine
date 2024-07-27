#include "../src/engine.h"
#include "input.h"

SDL_Event inputEvent;
bool leftMouseButtonDown, rightMouseButtonDown = false;

bool leftShiftPressed = false;
bool wPressed = false;
bool aPressed = false;
bool sPressed = false;
bool dPressed = false;
bool spacePressed = false;
bool leftCtrlPressed = false;

void input::initializeInput() {
	logger::log("Input successfully initialized!", 1);
}

void input::inputLoop() {
	SDL_PollEvent(&inputEvent);

	if (inputEvent.type == SDL_QUIT) {
		engine::running = false;
	}

	if (inputEvent.type == SDL_WINDOWEVENT) {
		if (inputEvent.window.event == SDL_WINDOWEVENT_MOVED) {
		}
		if (inputEvent.window.event == SDL_WINDOWEVENT_RESIZED) {
			renderer::recreateSwapChain();
		}
	}

	if (inputEvent.type == SDL_KEYDOWN) {
		if (inputEvent.key.keysym.sym == SDLK_w) {
			wPressed = true;
		}
		if (inputEvent.key.keysym.sym == SDLK_s) {
			sPressed = true;
		}
		if (inputEvent.key.keysym.sym == SDLK_a) {
			aPressed = true;
		}
		if (inputEvent.key.keysym.sym == SDLK_d) {
			dPressed = true;
		}
		if (inputEvent.key.keysym.sym == SDLK_SPACE) {
			spacePressed = true;
		}
		if (inputEvent.key.keysym.sym == SDLK_LCTRL) {
			leftCtrlPressed = true;
		}
		if (inputEvent.key.keysym.sym == SDLK_LSHIFT) {
			leftShiftPressed = true;
		}
	}

	if (inputEvent.type == SDL_KEYUP) {
		if (inputEvent.key.keysym.sym == SDLK_w) {
			wPressed = false;
		}
		if (inputEvent.key.keysym.sym == SDLK_s) {
			sPressed = false;
		}
		if (inputEvent.key.keysym.sym == SDLK_a) {
			aPressed = false;
		}
		if (inputEvent.key.keysym.sym == SDLK_d) {
			dPressed = false;
		}
		if (inputEvent.key.keysym.sym == SDLK_SPACE) {
			spacePressed = false;
		}
		if (inputEvent.key.keysym.sym == SDLK_LCTRL) {
			leftCtrlPressed = false;
		}
		if (inputEvent.key.keysym.sym == SDLK_LSHIFT) {
			leftShiftPressed = false;
		}
	}

	float cameraSpeed = 0.00125f;

	if (leftShiftPressed) {
		cameraSpeed = cameraSpeed * 2.5;
	}

	if (wPressed) {
		camera::moveForward(cameraSpeed);
	}
	if (sPressed) {
		camera::moveBackward(cameraSpeed);
	}
	if (aPressed) {
		camera::moveLeft(cameraSpeed);
	}
	if (dPressed) {
		camera::moveRight(cameraSpeed);
	}
	if (spacePressed) {
		camera::moveUp(cameraSpeed);
	}
	if (leftCtrlPressed) {
		camera::moveDown(cameraSpeed);
	}

	if (inputEvent.type == SDL_MOUSEWHEEL) {
		input::mouseScroll((float)inputEvent.wheel.x, (float)inputEvent.wheel.y);
	}

	if (inputEvent.type == SDL_MOUSEBUTTONDOWN) {
		if (inputEvent.button.button == SDL_BUTTON_LEFT) {
			leftMouseButtonDown = true;
		}

		if (inputEvent.button.button == SDL_BUTTON_RIGHT) {
			rightMouseButtonDown = true;
			SDL_SetRelativeMouseMode(SDL_TRUE);
		}
	}

	if (inputEvent.type == SDL_MOUSEBUTTONUP) {
		if (inputEvent.button.button == SDL_BUTTON_LEFT) {
			leftMouseButtonDown = false;
		}

		if (inputEvent.button.button == SDL_BUTTON_RIGHT) {
			rightMouseButtonDown = false;
			SDL_SetRelativeMouseMode(SDL_FALSE);
		}
	}

	if (inputEvent.type == SDL_MOUSEMOTION) {
		if (rightMouseButtonDown) {
			input::mouseMotion(inputEvent.motion.xrel, inputEvent.motion.yrel);
		}
	}
}

void input::mouseMotion(float mouseX, float mouseY) {
	input::mouseX += mouseX;
	input::mouseY += mouseY;

	camera::mouseLook(input::mouseX, input::mouseY);
}

void input::mouseScroll(float scrollX, float scrollY) {
	glm::vec2 currentScrollPosition = glm::vec2(scrollX, scrollY);

	if (currentScrollPosition.y >= 0) {
		camera::moveForward(0.25f);
	}
	else if (currentScrollPosition.y < 0) {
		camera::moveBackward(0.25f);
	}
}