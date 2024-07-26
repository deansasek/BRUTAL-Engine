#pragma once
#ifndef camera_h
#define camera_h

#include <glm/glm.hpp>

namespace camera {
	extern glm::vec3 cameraPos;

	glm::vec3 getView();
}

#endif