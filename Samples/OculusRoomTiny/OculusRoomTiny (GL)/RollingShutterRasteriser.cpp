#include "RollingShutterRasteriser.h"

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <exception>
#include "OpenGL_3_2_Utils\TextureLoader.h"

RollingShutterRasteriser::RollingShutterRasteriser(ovrSizei fb)
{
	vertexShader   = LoadShader(GL_VERTEX_SHADER,   "./GLSL/RollingShutterRasterizationTexture2DVertex.glsl");
	geometryShader = LoadShader(GL_GEOMETRY_SHADER, "./GLSL/RollingShutterRasterizationTexture2DGeometry.glsl");
	fragmentShader = LoadShader(GL_FRAGMENT_SHADER, "./GLSL/RollingShutterRasterizationTexture2DFragment.glsl");
	
	Link();

	useConvexHull = false;
	primitiveFilterIndex = -1;
	useAdaptiveBounds = false;
	m_resolution.x = fb.w;
	m_resolution.y = fb.h;
	useRaytracing = true;

	rollOffset = 0.0f;
}

void RollingShutterRasteriser::Render(Scene* scene, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f projection)
{
	this->view0Matrix4 = view0;
	this->view1Matrix4 = view1;
	this->projectionMatrix4 = projection;

	glUseProgram(getProgram());

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

	glUniform1f(glGetUniformLocation(getProgram(), "rollOffset"), rollOffset);

	for (size_t i = 0; i < scene->models.size(); ++i) {
		Render(scene->models[i], view0, view1, projection);
	}

	glUseProgram(0);
}

void RollingShutterRasteriser::Render(Model* model, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f projection)
{
	Matrix4f matW = model->GetMatrix();
	glUniformMatrix4fv(glGetUniformLocation(getProgram(), "matW"), 1, GL_TRUE, (FLOAT*)&matW);

	glBindBuffer(GL_ARRAY_BUFFER, model->vertexBuffer->buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->indexBuffer->buffer);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, GetTexture(model->getTextureId()));

	GLuint posLoc = glGetAttribLocation(getProgram(), "Position");
	GLuint colorLoc = glGetAttribLocation(getProgram(), "Color");
	GLuint uvLoc = glGetAttribLocation(getProgram(), "TexCoord");

	GLuint lPosition = glGetAttribLocation(program, "Position");
	GLuint lNormal = glGetAttribLocation(program, "Normal");
	GLuint lColour = glGetAttribLocation(program, "Color");
	GLuint lUVs = glGetAttribLocation(program, "TexCoord");

	glEnableVertexAttribArray(lPosition);
	glEnableVertexAttribArray(lColour);
	glEnableVertexAttribArray(lUVs);
	glEnableVertexAttribArray(lNormal);

	glVertexAttribPointer(lPosition, 3, GL_FLOAT, GL_FALSE, sizeof(Model::Vertex), (void*)OVR_OFFSETOF(Model::Vertex, Position));
	glVertexAttribPointer(lNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Model::Vertex), (void*)OVR_OFFSETOF(Model::Vertex, Normal));
	glVertexAttribPointer(lColour, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Model::Vertex), (void*)OVR_OFFSETOF(Model::Vertex, C));
	glVertexAttribPointer(lUVs, 2, GL_FLOAT, GL_FALSE, sizeof(Model::Vertex), (void*)OVR_OFFSETOF(Model::Vertex, U));

	glDisable(GL_CULL_FACE);
	glDrawElements(GL_TRIANGLES, model->Indices.size(), GL_UNSIGNED_SHORT, NULL);
	glEnable(GL_CULL_FACE);

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