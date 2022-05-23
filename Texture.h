#pragma once
#include <vector>
#include <string>

class Texture
{
public:
	void setupTexture(const char* texturePath);
	void loadBMPtoTexture(const char* imagepath);
	void bind(unsigned int slot) const;
	void bindCubeMap() const;
	void unbind() const;
	void unbindCubeMap() const;
private:
	unsigned int ID = 0;
	int Width = 0, Height = 0, BPP = 0;
};