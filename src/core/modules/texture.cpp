#include "texture.h"

#include "../../engine.h"
#include "../renderer/renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

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