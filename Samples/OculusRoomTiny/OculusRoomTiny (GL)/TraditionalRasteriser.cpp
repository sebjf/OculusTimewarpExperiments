#include "TraditionalRasteriser.h"

static ovrMatrix4f Multiply(const ovrMatrix4f& a, const ovrMatrix4f& b)
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

static const GLchar* VertexShaderSrc =
"#version 150\n"
"uniform mat4 matWVP;\n"
"in      vec4 Position;\n"
"in      vec4 Color;\n"
"in      vec2 TexCoord;\n"
"out     vec2 oTexCoord;\n"
"out     vec4 oColor;\n"
"void main()\n"
"{\n"
"   gl_Position = (matWVP * Position);\n"
"   oTexCoord   = TexCoord;\n"
"   oColor.rgb  = pow(Color.rgb, vec3(2.2));\n"   // convert from sRGB to linear
"   oColor.a    = Color.a;\n"
"}\n";

static const char* FragmentShaderSrc =
"#version 150\n"
"uniform sampler2D Texture0;\n"
"in      vec4      oColor;\n"
"in      vec2      oTexCoord;\n"
"out     vec4      FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = oColor * texture2D(Texture0, oTexCoord).r;\n"
"}\n";

GLuint CreateShader(GLenum type, const GLchar* src)
{
	GLuint shader = glCreateShader(type);

	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint r;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &r);
	if (!r)
	{
		GLchar msg[1024];
		glGetShaderInfoLog(shader, sizeof(msg), 0, msg);
		if (msg[0]) {
			OVR_DEBUG_LOG(("Compiling shader failed: %s\n", msg));
		}
		return 0;
	}

	return shader;
}

TraditionalRasteriser::TraditionalRasteriser()
{
	vertexShader = CreateShader(GL_VERTEX_SHADER, VertexShaderSrc);
	fragmentShader = CreateShader(GL_FRAGMENT_SHADER, FragmentShaderSrc);

	program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);

	GLint r;
	glGetProgramiv(program, GL_LINK_STATUS, &r);
	if (!r)
	{
		GLchar msg[1024];
		glGetProgramInfoLog(program, sizeof(msg), 0, msg);
		OVR_DEBUG_LOG(("Linking shaders failed: %s\n", msg));
	}
}

void TraditionalRasteriser::Render(Model* model, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f proj)
{
	Matrix4f combined = Multiply(proj, Multiply(view0, model->GetMatrix()));

	glUseProgram(program);
	glUniform1i(glGetUniformLocation(program, "Texture0"), 0);
	glUniformMatrix4fv(glGetUniformLocation(program, "matWVP"), 1, GL_TRUE, (FLOAT*)&combined);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, model->getTextureId());

	glBindBuffer(GL_ARRAY_BUFFER, model->vertexBuffer->buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->indexBuffer->buffer);

	GLuint posLoc = glGetAttribLocation(program, "Position");
	GLuint colorLoc = glGetAttribLocation(program, "Color");
	GLuint uvLoc = glGetAttribLocation(program, "TexCoord");

	glEnableVertexAttribArray(posLoc);
	glEnableVertexAttribArray(colorLoc);
	glEnableVertexAttribArray(uvLoc);

	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Model::Vertex), (void*)OVR_OFFSETOF(Model::Vertex, Pos));
	glVertexAttribPointer(colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Model::Vertex), (void*)OVR_OFFSETOF(Model::Vertex, C));
	glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Model::Vertex), (void*)OVR_OFFSETOF(Model::Vertex, U));

	glDrawElements(GL_TRIANGLES, model->numIndices, GL_UNSIGNED_SHORT, NULL);

	glDisableVertexAttribArray(posLoc);
	glDisableVertexAttribArray(colorLoc);
	glDisableVertexAttribArray(uvLoc);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glUseProgram(0);
}

void TraditionalRasteriser::Render(Scene* scene, ovrMatrix4f view0, ovrMatrix4f view1, ovrMatrix4f projection)
{
	for (int i = 0; i < scene->numModels; ++i) {
		Render(scene->Models[i], view0, view1, projection);
	}
}