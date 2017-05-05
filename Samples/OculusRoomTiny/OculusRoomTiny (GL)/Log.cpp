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

	string filename = "Logs/Log_" + GetHeader() + "_" + str + ".log";

	logfile = std::ofstream(filename, ofstream::out | ofstream::ate);

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

void Logger2::Step(float time, int answer, int renderingcondition, Stimulus* stimuli)
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

	logfile << entry << endl;

	numEntries++;
}

void Logger3::Step(float time, Vector3f head, Vector3f box, int condition, float speed)
{

	logfile << time << ", ";
	logfile << condition << ", ";
	logfile << head.x << ", ";
	logfile << head.y << ", ";
	logfile << head.z << ", ";
	logfile << box.x << ", ";
	logfile << box.y << ", ";
	logfile << box.z << ", ";
	logfile << speed << ", ";
	logfile << endl;

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