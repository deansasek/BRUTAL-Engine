#include "camera.h"

camera::cameraStruct camera::camera;

glm::vec2 camera::oldMousePosition;

void camera::createCamera() {
	camera::cameraStruct camera{};
	camera.eye = glm::vec3(0.0f, 0.0f, 0.0f);
	camera.viewDirection = glm::vec3(0.0f, 0.0f, -1.0f);
	camera.upVector = glm::vec3(0.0f, 1.0f, 0.0f);

	camera.fov = 90.0f;
	camera.xSensitivity = 0.25f;
	camera.ySensitivity = 0.25f;

	camera::camera = camera;

	logger::log("Successfully created camera!", 1);
}

glm::mat4 camera::getView() {


	return glm::lookAt(camera::camera.eye, camera::camera.eye + camera::camera.viewDirection, camera::camera.upVector);
}

float camera::getFOV() {
	return glm::radians(camera::camera.fov);
}

void camera::moveForward(float speed) {
	camera::camera.eye += (camera::camera.viewDirection * speed);
}

void camera::moveBackward(float speed) {
	camera::camera.eye -= (camera::camera.viewDirection * speed);
}

void camera::moveLeft(float speed) {
	glm::vec3 rightVector = glm::cross(camera::camera.viewDirection, camera::camera.upVector);
	camera::camera.eye -= (rightVector * speed);
}

void camera::moveRight(float speed) {
	glm::vec3 rightVector = glm::cross(camera::camera.viewDirection, camera::camera.upVector);
	camera::camera.eye += (rightVector * speed);
}

void camera::mouseLook(float mouseX, float mouseY) {
	glm::vec2 currentMousePosition = glm::vec2(mouseX, mouseY);

	static bool firstLook = true;
	if (firstLook) {
		camera::oldMousePosition = currentMousePosition;

		firstLook = false;
	}

	glm::vec2 mouseDelta = camera::oldMousePosition - currentMousePosition;

	mouseDelta.x = mouseDelta.x * camera.xSensitivity;
	mouseDelta.y = mouseDelta.y * camera.ySensitivity;

	camera::camera.viewDirection = glm::rotate(camera::camera.viewDirection, glm::radians(mouseDelta.x), camera::camera.upVector);
	camera::camera.viewDirection = glm::rotate(camera::camera.viewDirection, glm::radians(mouseDelta.y), glm::cross(camera::camera.viewDirection, camera::camera.upVector));

	camera::oldMousePosition = currentMousePosition;
}