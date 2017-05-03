#include "TraditionalRasteriser.h"

TraditionalRasteriser::TraditionalRasteriser()
{
	vertexShader = LoadShader(GL_VERTEX_SHADER, "./GLSL/TraditionalRasterizationVertex.glsl");
	fragmentShader = LoadShader(GL_FRAGMENT_SHADER, "./GLSL/TraditionalRasterizationFragment.glsl");
	Link();
}

void TraditionalRasteriser::Render(Model* model, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f proj)
{
	Matrix4f wv = Multiply(view0, model->GetMatrix());
	Matrix4f wvp = Multiply(proj, wv);

	glUseProgram(program);
	glUniform1i(glGetUniformLocation(program, "Texture0"), 0);
	glUniformMatrix4fv(glGetUniformLocation(program, "matWV"), 1, GL_TRUE, (FLOAT*)&wv);
	glUniformMatrix4fv(glGetUniformLocation(program, "matWVP"), 1, GL_TRUE, (FLOAT*)&wvp);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, GetTexture(model->getTextureId()));

	glBindBuffer(GL_ARRAY_BUFFER, model->vertexBuffer->buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->indexBuffer->buffer);

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

	glDrawElements(GL_TRIANGLES, model->Indices.size(), GL_UNSIGNED_SHORT, NULL);

	glDisableVertexAttribArray(lPosition);
	glDisableVertexAttribArray(lColour);
	glDisableVertexAttribArray(lUVs);
	glDisableVertexAttribArray(lNormal);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glUseProgram(0);
}

void TraditionalRasteriser::Render(Scene* scene, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f projection)
{
	for (size_t i = 0; i < scene->models.size(); ++i) {
		Render(scene->models[i], view0, view1, projection);
	}
}