#pragma once

#ifndef texture_h
#define texture_h

#include <string>

#include <stb/stb_image.h>
#include <glm/glm.hpp>

#include "../src/engine.h"

namespace engine {
	class texture {
		public:
			struct textureStruct {
				int textureDimensionsX;
				int textureDimensionsY;
				int textureChannels;
				std::string texturePath;
				stbi_uc* data;
			} textureStruct;
			
			engine::texture createTexture(std::string texturePath);
			void loadTexture(engine::texture texture);
			void destroyTexture(stbi_uc* data);
		private:
	};
}

#endif