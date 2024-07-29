#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

engine::texture engine::texture::createTexture(std::string texturePath) {
	engine::texture texture;

	texture.textureStruct.texturePath = texturePath;

	texture.loadTexture(texture);

	return texture;
}

void engine::texture::loadTexture(engine::texture texture) {
	texture.textureStruct.data = stbi_load(texture.textureStruct.texturePath.c_str(), &texture.textureStruct.textureDimensionsX, &texture.textureStruct.textureDimensionsY, &texture.textureStruct.textureChannels, STBI_rgb_alpha);

	if (!texture.textureStruct.data) {
		throw std::runtime_error("Failed to load texture image!");
	}
	else {
		logger::log("Successfully loaded texture image!", 1);
	}
}

void engine::texture::destroyTexture(stbi_uc* data) {
	std::cout << "in progress" << std::endl;

	stbi_image_free(data);
}

/*
int texture::textureWidth, texture::textureHeight, texture::textureChannels;
stbi_uc* texture::pixels;

void texture::loadTexture(std::string texturePath) {
	texture::pixels = stbi_load(texturePath.c_str(), &texture::textureWidth, &texture::textureHeight, &texture::textureChannels, STBI_rgb_alpha);

	if (!texture::pixels) {
		throw std::runtime_error("Failed to load texture image!");
	}
	else {
		logger::log("Successfully loaded texture image!", 1);
	}
}

void texture::freeTexture(stbi_uc* pixels) {
	stbi_image_free(pixels);
}
*/