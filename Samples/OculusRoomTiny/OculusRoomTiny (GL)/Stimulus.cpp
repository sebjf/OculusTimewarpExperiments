
#include <vector>
#include <fstream>
#include <iostream>
#include "Stimulus.h"

using namespace std;

static std::vector<char> ReadAllBytes(char const* filename)
{
	ifstream ifs(filename, ios::binary | ios::ate);
	ifstream::pos_type pos = ifs.tellg();

	vector<char>  result((size_t)pos);

	ifs.seekg(0, ios::beg);
	ifs.read(&result[0], pos);

	return result;
}

void Stimulus::Load()
{
	ifstream ifs("./charactersets.bin", ios::binary | ios::in);

	assert(ifs.is_open());

	uint32_t numCharacters;
	ifs.read(reinterpret_cast<char *>(&numCharacters), sizeof(uint32_t));
	ifs.read(reinterpret_cast<char *>(&characterset.textureWidth), sizeof(uint32_t));

	assert(characterset.textureWidth == 512);

	int datalength = characterset.textureWidth * characterset.textureWidth;
	char* data = new char[datalength];

	for (size_t i = 0; i < numCharacters; i++)
	{
		Character character;

		ifs.read(reinterpret_cast<char *>(&character.code), sizeof(uint8_t));
		ifs.read(reinterpret_cast<char *>(&character.acuity), sizeof(float));
		ifs.read(data, datalength);

		// Create one OpenGL texture
		GLuint textureID;
		glGenTextures(1, &textureID);
		
		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, textureID);
		
		// Give the image to OpenGL
		glTexImage2D(
			GL_TEXTURE_2D, 
			0, 
			GL_R8_SNORM, 
			characterset.textureWidth, 
			characterset.textureWidth, 
			0, 
			GL_RED, 
			GL_UNSIGNED_BYTE, 
			data);
		
		// Poor filtering, or ...
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
		
		// ... nice trilinear filtering.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
		glGenerateMipmap(GL_TEXTURE_2D);

		character.textureId = textureID;
		characters.push_back(character);
	}

	delete data;
	ifs.close();
}
