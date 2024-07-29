#pragma once

#ifndef model_h
#define model_h

#include "../src/engine.h"

#include <string>

namespace engine {
	class model {
		public:
			struct vertexStruct {
				glm::vec3 position;
				glm::vec3 color;
				glm::vec2 textureCoordinates;
			} vertex;

			struct modelStruct {
				std::string modelPath;
				std::vector<vertexStruct> vertices;
				std::vector<uint32_t> indices;
			} data;

			engine::model createModel(std::string modelPath);

			void loadModel(engine::model model);
			void renderModel();
			void destroyModel();
		private:
			void createVertexBuffer();
			void createIndexBuffer();
	};
}

#endif