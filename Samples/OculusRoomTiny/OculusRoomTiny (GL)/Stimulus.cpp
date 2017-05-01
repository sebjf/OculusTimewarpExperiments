
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
	charactersetdata = ReadAllBytes("./charactersets.bin");
	characterset = (Characterset*)charactersetdata.data();
	assert(characterset->textureWidth == 512);

	characterset->characters = (Character*)(charactersetdata.data() + (sizeof(unsigned int) * 2));

	for (size_t i = 0; i < characterset->numCharacters; i++)
	{
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
			characterset->textureWidth, 
			characterset->textureWidth, 
			0, 
			GL_RED, 
			GL_UNSIGNED_BYTE, 
			characterset->characters[i].data);
		
		// Poor filtering, or ...
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
		
		// ... nice trilinear filtering.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
		glGenerateMipmap(GL_TEXTURE_2D);

		textureids.push_back(textureID);
	}
}
