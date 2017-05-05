#pragma once

#include <vector>
#include <fstream>

#include "Stimulus.h"

struct LogEntry
{
	float time;
	int answer;

	int currentCharacter;
	int currentCharacterCode;
	float currentCharacterAcuity;

	int previousCharacter;
	int previousCharacterCode;
	float previousCharacterAcuity;

	int renderingCondition;
	float targetSpeed;
	float targetX;
	float targetY;
	float targetZ;

	friend std::ostream& operator<<(std::ostream& os, const LogEntry& entry);
};

class Logger
{
public:
	Logger();
	~Logger();

	int GetNumEntries()
	{
		return numEntries;
	}

protected:
	std::ofstream logfile;

	void Step(LogEntry entry);

	int numEntries;

	virtual std::string GetHeader() 
	{
		return "v0";
	}
};

class Logger2 : public Logger
{
public:
	Logger2()
	{
		previousCharacterId = -1;
	}
	void Step(float time, int answer, int renderingcondition, Stimulus* stimuli);
	int previousCharacterId;

	virtual std::string GetHeader()
	{
		return "v2";
	}
};

class Logger3 : public Logger
{
public:
	void Step(float time, Vector3f head, Vector3f box, int condition, float speed);
	int previousCharacterId;

	virtual std::string GetHeader()
	{
		return "v5";
	}
};