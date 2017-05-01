#pragma once

#include "../../OculusRoomTiny_Advanced/Common/Win32_GLAppUtil.h"
#include "OpenGL_3_2_Utils\TextureLoader.h"
#include <vector>

struct Character
{
	uint8_t code;
	float acuity;
	GLuint textureId;
};

struct Characterset
{
	unsigned int textureWidth;
};

class Stimulus
{
public:
	Stimulus()
	{
		radius = 9;
		phase = 0;
		clock1 = 0;
		height1 = 3;
		height2 = 0;
		speed1 = 0.015f;
		speed2 = 0.015f;
		Load();
	}

	void Update()
	{
		m_model->Pos = Vector3f(radius * (float)sin(clock1 + phase), height1 + height2 * (float)sin(clock2 + phase), radius * (float)cos(clock1 + phase));
		clock1 += speed1;
		clock2 += speed2;
	}

	int GetCharacterTexture(int i)
	{
		return characters[i % characters.size()].textureId;
	}

	float speed1;
	float speed2;
	float phase;
	float radius;
	float height1;
	float height2;
	Model* m_model;

private:
	float clock1;
	float clock2;
	Characterset characterset;
	std::vector<Character> characters;

	void Load();
};