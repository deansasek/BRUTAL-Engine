#pragma once

#ifndef texture_h
#define texture_h

#include <string>

#include <stb/stb_image.h>

namespace texture {
	extern int textureWidth, textureHeight, textureChannels;
	extern stbi_uc* pixels;

	void loadTexture(std::string texturePath);
	void freeTexture(stbi_uc* pixels);
}

#endif