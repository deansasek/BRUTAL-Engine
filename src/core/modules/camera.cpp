#include "camera.h"

glm::vec3 camera::cameraPos = glm::vec3(20.0f, 3.0f, -30.0f);

glm::vec3 camera::getView() {
	return cameraPos;
}