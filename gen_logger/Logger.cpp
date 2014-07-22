#include "StdAfx.h"
#include "logger.h"
#include <string>
#include <fstream>

const char* LOG_DIR = "C:\\logs\\";
const char* LOG_FILENAME = "C:\\logs\\log.txt";

Logger::Logger() {
	
}

Logger::~Logger(void) { 
	close();
}

void Logger::open() {
	stream.open(LOG_FILENAME, std::ofstream::out | std::ofstream::app);
}

void Logger::log(std::wstring entry) {
	
	stream << entry << std::endl;
}

void Logger::close() {
	if (stream.is_open())
		stream.close();
}

bool Logger::IsTimeToRotate(){
}
