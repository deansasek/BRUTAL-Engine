#pragma once

#ifndef input_h
#define input_h

namespace input {
	extern SDL_Event inputEvent;

	void initializeInput();
	void inputLoop();
}

#endif