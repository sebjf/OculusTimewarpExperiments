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
};

class Logger3 : public Logger
{
public:
	void Step(float time, float error, int condition);
	int previousCharacterId;
};