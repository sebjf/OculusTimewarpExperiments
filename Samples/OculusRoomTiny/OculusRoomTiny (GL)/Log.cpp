#include "Log.h"

#include <fstream>
#include <ostream>
#include <vector>
#include <ctime>

using namespace std;

Logger::Logger()
{
	time_t rawtime;
	struct tm timeinfo;
	char buffer[80];

	time(&rawtime);
	errno_t err = localtime_s(&timeinfo, &rawtime);

	strftime(buffer, sizeof(buffer), "%d-%m-%Y_%I-%M-%S", &timeinfo);
	string str(buffer);

	string filename = "Logs/Log_v2_" + str + ".log";

	logfile = std::ofstream(filename, ofstream::out | ofstream::ate);

	previousCharacterId = -1;
	numEntries = 0;
}

std::ostream& operator<<(std::ostream& os, const LogEntry& entry)
{
	os << entry.time << ", ";
	os << entry.answer << ", ";
	os << entry.currentCharacter << ", ";
	os << entry.currentCharacterCode << ", ";
	os << entry.currentCharacterAcuity << ", ";
	os << entry.previousCharacter << ", ";
	os << entry.previousCharacterCode << ", ";
	os << entry.previousCharacterAcuity << ", ";
	os << entry.renderingCondition << ", ";
	os << entry.targetSpeed << ", ";
	os << entry.targetX << ", ";
	os << entry.targetY << ", ";
	os << entry.targetZ << ", ";
	return os;
}

void Logger::Step(float time, int answer, int renderingcondition, Stimulus* stimuli)
{
	int currentCharacterId = stimuli->GetCurrentCharacterId();
	
	LogEntry entry;
	entry.time = time;
	entry.answer = answer;
	entry.currentCharacter = currentCharacterId;
	entry.currentCharacterCode = stimuli->GetCharacterCode(currentCharacterId);
	entry.currentCharacterAcuity = stimuli->GetCharacterAcuity(currentCharacterId);

	entry.previousCharacter = previousCharacterId;
	entry.previousCharacterCode = stimuli->GetCharacterCode(previousCharacterId);
	entry.previousCharacterAcuity = stimuli->GetCharacterAcuity(previousCharacterId);

	entry.renderingCondition = renderingcondition;
	entry.targetSpeed = stimuli->speed1;
	stimuli->GetPosition(entry.targetX, entry.targetY, entry.targetZ);

	previousCharacterId = currentCharacterId;

	Step(entry);
}

void Logger::Step(LogEntry entry)
{
	logfile << entry << endl;
	numEntries++;
}

Logger::~Logger()
{
	if (logfile.is_open())
	{
		logfile.flush();
		logfile.close();
	}
}