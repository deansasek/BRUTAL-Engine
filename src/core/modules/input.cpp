#include "../src/engine.h"
#include "input.h"

SDL_Event input::inputEvent;

void input::initializeInput() {
	logger::log("Input successfully initialized!", 1);
}