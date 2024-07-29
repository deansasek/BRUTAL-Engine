#include "./gameObject.h"

engine::gameObject::data engine::gameObject::createGameObject(std::string modelPath) {
	engine::model model;

	model.createModel(modelPath);

	engine::gameObject::data gameObject{};
	gameObject.model = model;

	engine::gameObject::gameObjects.push_back(gameObject);

	logger::log("Successfully created game object!", 1);

	return gameObject;
}