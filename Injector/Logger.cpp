#include "pch.h"
#include "Logger.h"
#include <string>
#include <fstream>

void writeLog(const char* text)
{
    string filePath = "C:\\log.txt";
    ofstream ofs(filePath.c_str(), std::ios_base::out | std::ios_base::app);
    ofs << text;
    ofs.close();
}

void writeLog(std::string text)
{
    string filePath = "C:\\log.txt";
    ofstream ofs(filePath.c_str(), std::ios_base::out | std::ios_base::app);
    ofs << text;
    ofs.close();
}
