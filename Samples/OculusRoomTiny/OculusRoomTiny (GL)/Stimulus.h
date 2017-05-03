#pragma once

#include "../../OculusRoomTiny_Advanced/Common/Win32_GLAppUtil.h"
#include "OpenGL_3_2_Utils\TextureLoader.h"
#include <vector>

struct Character
{
	size_t id;
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
		radius = 8;
		phase = 0;
		clock1 = 0;
		clock2 = 0;
		height1 = 3;
		height2 = 0;
		speed1 = 0.01f;
		speed2 = 0.01f;
		currentCharacterId = 0;
		Load();
	}

	void Update()
	{
		m_model->Pos = Vector3f(radius * (float)sin(clock1 + phase), height1 + height2 * (float)sin(clock2 + phase), radius * (float)cos(clock1 + phase));
		m_model->Rot = Quatf(Vector3f(0, 0, -1), clock2);
		m_model->Scale = 0.5f;
		clock1 += speed1;
		clock2 += speed2;
	}

	int GetCharacterTexture(int i)
	{
		return characters[i].textureId;
	}

	int GetCharacterCode(int i)
	{
		if (i < 0)
		{
			return -1;
		}
		return characters[i].code;
	}

	float GetCharacterAcuity(int i)
	{
		if (i < 0)
		{
			return -1;
		}
		return characters[i].acuity;
	}

	int GetCurrentCharacterId()
	{
		return currentCharacterId;
	}

	int GetCharacterCount()
	{
		return characters.size();
	}

	void SetCurrentCharacterId(int i)
	{
		currentCharacterId = i % characters.size();
	}

	void GetPosition(float& x, float& y, float& z)
	{
		x = m_model->Pos.x;
		y = m_model->Pos.y;
		z = m_model->Pos.z;
	}

	float speed1;
	float speed2;
	float phase;
	float radius;
	float height1;
	float height2;
	Model* m_model;

	std::vector<Character> characters;

private:
	float clock1;
	float clock2;

	Characterset characterset;

	int currentCharacterId;

	void Load();
};