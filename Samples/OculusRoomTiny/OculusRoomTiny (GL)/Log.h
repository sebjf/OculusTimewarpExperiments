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

	void Step(float time, int answer, int renderingcondition, Stimulus* stimuli);

	int GetNumEntries()
	{
		return numEntries;
	}

private:
	std::ofstream logfile;

	void Step(LogEntry entry);

	int numEntries;
	int previousCharacterId;
};