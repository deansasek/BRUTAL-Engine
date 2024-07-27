#pragma once

#ifndef model_h
#define model_h

#include "../../engine.h"
#include "../renderer/renderer.h"

#include <string>

namespace model {
	void loadModel(std::string modelPath);
}

namespace engine {
	class model {
	public:
		void loadModel();
		void renderModel();
		void destroyModel();
	private:
	};
}

#endif