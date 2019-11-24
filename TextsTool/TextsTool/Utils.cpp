#include <iostream>
#include <experimental/filesystem>
#include <QTimer>
#include <QDateTime>
#include "Utils.h"

//---------------------------------------------------------------

void Log(const std::string& str)
{
    FILE* fp = nullptr;
    errno_t err = fopen_s(&fp, "d:\\ClientLog.txt", "at");
    if (err != 0) {
        return;
    }
    fprintf(fp, "%d: %s\n", Utils::GetCurrentTimestamp(), str.c_str());
    fclose(fp);
}

//---------------------------------------------------------------

void ExitMsg(const std::string& message)
{
    Log("Fatal: " + message);
    throw std::exception("Exiting app exception");
}


namespace Utils
{

//---------------------------------------------------------------

    uint GetCurrentTimestamp()
    {
        QDateTime current = QDateTime::currentDateTime();
        return  current.toTime_t();
    }

//---------------------------------------------------------------

	void LogBuf(std::vector<uint8_t>& buffer)
	{
		std::string sstr;
		for (auto val : buffer) {
			sstr = sstr + std::to_string(val);
			sstr = sstr + " ";
		}
		Log(sstr);
	}

//---------------------------------------------------------------
// FNV-1a algorithm https://softwareengineering.stackexchange.com/questions/49550/which-hashing-algorithm-is-best-for-uniqueness-and-speed
//---------------------------------------------------------------

	uint64_t AddHash(uint64_t curHash, std::vector<uint8_t>& key, bool isFirstPart)
	{
		if (isFirstPart) {
			curHash = 14695981039346656037;
		}

		for (uint8_t el : key) {
			curHash = curHash ^ el;
			curHash *= 1099511628211;
		}
		return curHash;
	}

}; // namespace Utils
