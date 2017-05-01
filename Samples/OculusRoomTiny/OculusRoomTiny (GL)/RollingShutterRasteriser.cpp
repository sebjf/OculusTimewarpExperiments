#include "RollingShutterRasteriser.h"

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <exception>

#include "OpenGL_3_2_Utils\TextureLoader.h"

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

GLuint LoadShader(GLenum type, const char* filename)
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

RollingShutterRasteriser::RollingShutterRasteriser(ovrSizei fb)
{
	vertexShader   = LoadShader(GL_VERTEX_SHADER,   "./GLSL/RollingShutterRasterizationTexture2DVertex.glsl");
	geometryShader = LoadShader(GL_GEOMETRY_SHADER, "./GLSL/RollingShutterRasterizationTexture2DGeometry.glsl");
	fragmentShader = LoadShader(GL_FRAGMENT_SHADER, "./GLSL/RollingShutterRasterizationTexture2DFragment.glsl");
	
	program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, geometryShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);

	if (!linked)
	{
		throw new std::exception("failed to link rolling rasterisation program");
	}

	useConvexHull = false;
	primitiveFilterIndex = -1;
	useAdaptiveBounds = false;
	m_resolution.x = fb.w;
	m_resolution.y = fb.h;
	useRaytracing = true;
}

void RollingShutterRasteriser::Begin()
{
	glUseProgram(program);

	/*
	ovrMatrix4f view0Matrix4;
	ovrMatrix4f view1Matrix4;
	ovrMatrix4f projectionMatrix4;
	bool useConvexHull;
	int primitiveFilterIndex;
	bool useAdaptiveBounds;
	ovrVector2i resolution;
	bool useRaytracing;
	*/
	glUniformMatrix4fv(glGetUniformLocation(getProgram(), "view0Matrix4"), 1, GL_TRUE, (FLOAT*)&view0Matrix4);
	glUniformMatrix4fv(glGetUniformLocation(getProgram(), "view1Matrix4"), 1, GL_TRUE, (FLOAT*)&view1Matrix4);
	glUniformMatrix4fv(glGetUniformLocation(getProgram(), "projectionMatrix4"), 1, GL_TRUE, (FLOAT*)&projectionMatrix4);
	glUniform1i(glGetUniformLocation(getProgram(), "useConvexHull"), useConvexHull ? 1 : 0);
	glUniform1i(glGetUniformLocation(getProgram(), "primitiveFilterIndex"), primitiveFilterIndex);
	glUniform1i(glGetUniformLocation(getProgram(), "useAdaptiveBounds"), useAdaptiveBounds ? 1 : 0);
	glUniform2i(glGetUniformLocation(getProgram(), "resolution"), m_resolution.x, m_resolution.y);
	glUniform1i(glGetUniformLocation(getProgram(), "useRaytracing"), useRaytracing ? 1 : 0);

}

void RollingShutterRasteriser::End()
{
	glUseProgram(0);
}

void RollingShutterRasteriser::Render(Scene* scene, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f projection)
{
	this->view0Matrix4 = view0;
	this->view1Matrix4 = view1;
	this->projectionMatrix4 = projection;

	Begin();

	for (int i = 0; i < scene->numModels; ++i) {
		Render(scene->Models[i], view0, view1, projection);
	}

	End();
}

void RollingShutterRasteriser::Render(Model* model, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f projection)
{
	Matrix4f matM = model->GetMatrix();
	glUniformMatrix4fv(glGetUniformLocation(getProgram(), "matWVP"), 1, GL_TRUE, (FLOAT*)&matM);

	glBindBuffer(GL_ARRAY_BUFFER, model->vertexBuffer->buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->indexBuffer->buffer);

	GLuint posLoc = glGetAttribLocation(getProgram(), "Position");
	GLuint colorLoc = glGetAttribLocation(getProgram(), "Color");
	GLuint uvLoc = glGetAttribLocation(getProgram(), "TexCoord");

	glUniform1i(glGetUniformLocation(getProgram(), "Texture0"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, model->getTextureId());

	glEnableVertexAttribArray(posLoc);
	glEnableVertexAttribArray(colorLoc);
	glEnableVertexAttribArray(uvLoc);

	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Model::Vertex), (void*)OVR_OFFSETOF(Model::Vertex, Pos));
	glVertexAttribPointer(colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Model::Vertex), (void*)OVR_OFFSETOF(Model::Vertex, C));
	glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Model::Vertex), (void*)OVR_OFFSETOF(Model::Vertex, U));

	glDisable(GL_CULL_FACE);
	glDrawElements(GL_TRIANGLES, model->numIndices, GL_UNSIGNED_SHORT, NULL);

	glDisableVertexAttribArray(posLoc);
	glDisableVertexAttribArray(colorLoc);
	glDisableVertexAttribArray(uvLoc);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

struct Texture
{
	GLuint textureID;

	Texture(const char* imagepath)
	{
		unsigned int width, height;
		unsigned char * data = loadBMPRaw(imagepath, width, height);

		// Create one OpenGL texture
		glGenTextures(1, &textureID);

		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, textureID);

		// Give the image to OpenGL
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

		// ... nice trilinear filtering.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
		delete[] data;

		glBindTexture(GL_TEXTURE_2D, 0);
	}

};