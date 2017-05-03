#pragma once

/* assimp include files. These three are usually needed. */
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include "Stimulus.h"


Model* ImportAssimpModel(const aiScene* scene, int meshid = 0)
{
	Model* model = new Model(Vector3f(0, 0, 0), NULL);

	auto mesh = scene->mMeshes[meshid];

	for (size_t i = 0; i < mesh->mNumFaces; i++)
	{
		auto face = mesh->mFaces[i];
		assert(face.mNumIndices == 3);
		model->AddIndex(face.mIndices[0]);
		model->AddIndex(face.mIndices[1]);
		model->AddIndex(face.mIndices[2]);
	}

	for (size_t i = 0; i < mesh->mNumVertices; i++)
	{
		Model::Vertex v;
		v.Position = Vector3f(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
		v.Normal = Vector3f(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
		v.C = 0xFFFFFFFF;
		v.U = mesh->mTextureCoords[0][i].x;
		v.V = mesh->mTextureCoords[0][i].y;
		model->AddVertex(v);
	}

	model->AllocateBuffers();

	return model;
}

std::vector<Model*> ImportAssimpScene(const aiScene* scene)
{
	std::vector<Model*> models;
	for (size_t i = 0; i < scene->mNumMeshes; i++)
	{
		auto mesh = ImportAssimpModel(scene, i);
		models.push_back(mesh);
	}
	return models;
}

class TriggerInput
{
public:
	TriggerInput()
	{
		isCurrentlyDown = false;
		isTriggered = false;
	}

	TriggerInput& Update(bool keyIsDown)
	{
		isTriggered = false;

		if (keyIsDown)
		{
			if (!isCurrentlyDown)
			{
				isTriggered = true;
				isCurrentlyDown = true;
			}
		}
		else
		{
			isCurrentlyDown = false;
		}

		return *this;
	}

	bool IsTriggered()
	{
		return isTriggered;
	}

private:

	bool isCurrentlyDown;
	bool isTriggered;

};

class TriggerInputs
{
public:
	TriggerInputs()
	{
	}

	void Update(bool one, bool two, bool three, bool four)
	{
		inputs[0].Update(one);
		inputs[1].Update(two);
		inputs[2].Update(three);
		inputs[3].Update(four);
	}

	bool IsTriggered()
	{
		return Trigger() > -1;
	}

	int Trigger()
	{
		for (size_t i = 0; i < 4; i++)
		{
			if (inputs[i].IsTriggered())
			{
				return i;
			}
		}
		return -1;
	}

private:

	TriggerInput inputs[4];

};

class TimeTrigger
{
public:
	int last[10];

	TimeTrigger()
	{
		for (size_t i = 0; i < 10; i++)
		{
			last[i] = 0;
		}
	}

	bool IsTrigger(float time, int i)
	{
		int seconds = (int)time / i;
		
		if (last[i] != seconds)
		{
			last[i] = seconds;
			return true;
		}

		return false;
	}
};

class BlockedConditions
{
public:
	struct Condition
	{
		int rasterisation;
		int character;
	};

	BlockedConditions(size_t total, size_t blocksize, Stimulus* stimulus)
	{
		size_t range = 2;
		size_t numblocks = total / blocksize;
		size_t numblockseach = numblocks / range;

		std::vector<int> conditions_compressed;

		for (size_t i = 0; i < range; i++)
		{
			for (size_t j = 0; j < numblockseach; j++)
			{
				conditions_compressed.push_back(i);
			}
		}

		std::random_shuffle(conditions_compressed.begin(), conditions_compressed.end());


		std::vector<int> matching;
		for (size_t i = 0; i < stimulus->characters.size(); i++)
		{
			if (stimulus->GetCharacterCode(i) > 7)
			{
				matching.push_back(i);
			}
		}

		for (size_t i = 0; i < conditions_compressed.size(); i++)
		{
			for (size_t j = 0; j < blocksize; j++)
			{
				Condition condition;
				condition.rasterisation = conditions_compressed[i];
				condition.character = matching[rand() % matching.size()];
				conditions.push_back(condition);
			}
		}

		counter = 0;
	}

	bool IsDone()
	{
		return counter >= conditions.size();
	}

	Condition GetNext()
	{
		return conditions[counter++];
	}

private:
	std::vector<Condition> conditions;
	size_t counter;
};
