#include "Rasteriser.h"

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <exception>
#include "OpenGL_3_2_Utils\TextureLoader.h"

ovrMatrix4f Rasteriser::Multiply(const ovrMatrix4f& a, const ovrMatrix4f& b)
{
	ovrMatrix4f result;
	ovrMatrix4f* d = &result;
	OVR_MATH_ASSERT((d != &a) && (d != &b));
	int i = 0;
	do {
		d->M[i][0] = a.M[i][0] * b.M[0][0] + a.M[i][1] * b.M[1][0] + a.M[i][2] * b.M[2][0] + a.M[i][3] * b.M[3][0];
		d->M[i][1] = a.M[i][0] * b.M[0][1] + a.M[i][1] * b.M[1][1] + a.M[i][2] * b.M[2][1] + a.M[i][3] * b.M[3][1];
		d->M[i][2] = a.M[i][0] * b.M[0][2] + a.M[i][1] * b.M[1][2] + a.M[i][2] * b.M[2][2] + a.M[i][3] * b.M[3][2];
		d->M[i][3] = a.M[i][0] * b.M[0][3] + a.M[i][1] * b.M[1][3] + a.M[i][2] * b.M[2][3] + a.M[i][3] * b.M[3][3];
	} while ((++i) < 4);

	return *d;
}

#define ERRORMSG(msg) MessageBoxA(NULL, (msg), "OculusRoomTiny", MB_ICONERROR | MB_OK);

std::string readFile(const char *filePath) {
	std::string content;
	std::ifstream fileStream(filePath, std::ios::in);

	if (!fileStream.is_open()) {
		int currentdirlength = GetCurrentDirectory(0, NULL);
		TCHAR* currentdir = new TCHAR[currentdirlength];
		GetCurrentDirectory(currentdirlength, currentdir);
		auto errorstring = std::string("Could not read file ") + std::string(currentdir) + std::string(filePath) + std::string(". File does not exist.");
		delete currentdir;
		ERRORMSG(errorstring.c_str());
		return "";
	}

	std::string line = "";
	while (!fileStream.eof()) {
		std::getline(fileStream, line);
		content.append(line + "\n");
	}

	fileStream.close();
	return content;
}

GLuint Rasteriser::LoadShader(GLenum type, const char* filename)
{
	GLuint shader;

	GLint result = GL_FALSE;
	int logLength = 0;

	// load the shaders
	shader = glCreateShader(type);
	std::string shaderSrcStr = readFile(filename);
	const char* shaderSrcStrPtr = shaderSrcStr.c_str();
	glShaderSource(shader, 1, &shaderSrcStrPtr, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	char* shaderErrorMessagePtr = new char[logLength];
	glGetShaderInfoLog(shader, logLength, NULL, shaderErrorMessagePtr);

	if (result != GL_TRUE)
	{
		throw new std::exception(shaderErrorMessagePtr);
	}

	return shader;
}

void Rasteriser::Link()
{
	program = glCreateProgram();
	if (vertexShader != -1) 
	{
		glAttachShader(program, vertexShader);
	}
	if (geometryShader != -1)
	{
		glAttachShader(program, geometryShader);
	}
	if (fragmentShader != -1)
	{
		glAttachShader(program, fragmentShader);
	}
	glLinkProgram(program);

	GLint r;
	glGetProgramiv(program, GL_LINK_STATUS, &r);
	if (!r)
	{
		GLchar msg[1024];
		glGetProgramInfoLog(program, sizeof(msg), 0, msg);
		OVR_DEBUG_LOG(("Linking shaders failed: %s\n", msg));
	}
}

void Rasteriser::MakeTextures()
{
	// Make textures
	for (int k = 0; k < 5; ++k)
	{
		static DWORD tex_pixels[256 * 256];
		for (int j = 0; j < 256; ++j)
		{
			for (int i = 0; i < 256; ++i)
			{
				if (k == 0) tex_pixels[j * 256 + i] = (((i >> 7) ^ (j >> 7)) & 1) ? 0xffb4b4b4 : 0xff505050;// floor
				if (k == 1) tex_pixels[j * 256 + i] = (((j / 4 & 15) == 0) || (((i / 4 & 15) == 0) && ((((i / 4 & 31) == 0) ^ ((j / 4 >> 4) & 1)) == 0)))
					? 0xff3c3c3c : 0xffb4b4b4;// wall
				if (k == 2) tex_pixels[j * 256 + i] = (i / 4 == 0 || j / 4 == 0) ? 0xff505050 : 0xffb4b4b4;// ceiling
				if (k == 3) tex_pixels[j * 256 + i] = 0xffffffff;// blank
				if (k == 4) tex_pixels[j * 256 + i] = 0xffff00ff;// blank 2
			}
		}
		textures.push_back(new TextureBuffer(nullptr, false, false, Sizei(256, 256), 4, (unsigned char *)tex_pixels, 1));
	}
}