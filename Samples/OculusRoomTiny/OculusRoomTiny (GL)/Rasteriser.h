#pragma once

#include "OVR_CAPI_GL.h"
#include "GL\CAPI_GLE.h"
#include "../../OculusRoomTiny_Advanced/Common/Win32_GLAppUtil.h"

class Rasteriser
{
public:
	Rasteriser()
	{
		vertexShader = -1;
		geometryShader = -1;
		fragmentShader = -1;
		MakeTextures();
	}

protected:
	static ovrMatrix4f Multiply(const ovrMatrix4f& a, const ovrMatrix4f& b);
	static GLuint Rasteriser::LoadShader(GLenum type, const char* filename);

	std::vector<TextureBuffer*> textures;
	void MakeTextures();

	GLuint GetTexture(GLuint desired)
	{
		if (desired == 3452816845)
		{
			return textures[3]->texId;
		}
		return desired;
	}

	GLuint vertexShader;
	GLuint geometryShader;
	GLuint fragmentShader;
	GLuint program;

	void Link();
};