#pragma once

#include "../../OculusRoomTiny_Advanced/Common/Win32_GLAppUtil.h"
#include "OpenGL_3_2_Utils\TextureLoader.h"
#include <vector>

struct Character
{
	char code;
	float acuity;
	char data[512 * 512 * 1];
};

struct Characterset
{
	unsigned int numCharacters;
	unsigned int textureWidth;
	Character* characters;
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
		return textureids[i % textureids.size()];
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
	std::vector<char> charactersetdata;
	Characterset* characterset;
	std::vector<GLuint> textureids;

	void Load();
};