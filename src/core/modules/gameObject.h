#pragma once
#ifndef gameObject_h
#define gameObject_h

#include "../src/engine.h"

namespace engine {
	class gameObject {
		public:
			struct data {
				engine::model model;
			} dataObject;

			static std::vector<data> gameObjects;

			engine::gameObject::data createGameObject(std::string modelPath);
		private:
	};
}

#endif