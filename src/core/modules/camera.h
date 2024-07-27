#pragma once
#ifndef camera_h
#define camera_h

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include "glm/gtx/rotate_vector.hpp"

#include "../logger/logger.h"

namespace camera {
	struct cameraStruct {
		glm::vec3 eye;
		glm::vec3 viewDirection;
		glm::vec3 upVector;

		float fov;
		float xSensitivity;
		float ySensitivity;
	};

	extern cameraStruct camera;

	extern glm::vec2 oldMousePosition;

	void createCamera();
	glm::mat4 getView();
	float getFOV();

	void moveUp(float speed);
	void moveDown(float speed);
	void moveForward(float speed);
	void moveBackward(float speed);
	void moveLeft(float speed);
	void moveRight(float speed);
	void mouseLook(float mouseX, float mouseY);
}

#endif