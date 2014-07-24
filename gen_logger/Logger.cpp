#include "StdAfx.h"
#include "logger.h"
#include <string>
#include <fstream>
#include "wa_ipc.h"
#include <Windows.h>
using namespace std;

//char* LOG_DIR = "C:\\logs\\";
const wstring Logger::LOG_FILENAME = L"log.txt";
const wstring Logger::ARCHIVE_FOLDER = L"archive\\";
const wstring Logger::ARCHIVE_FILE_PATTERN = ARCHIVE_FOLDER + L"%04d%02d%02d.txt";

Logger::Logger() {

}

Logger::~Logger(void) { 
	close();
}

void Logger::open(wstring basePath) {
	logfileBasePath = basePath;
	logfilename = basePath + LOG_FILENAME;
	if (!RotateIfNecessary())
		stream.open(logfilename.c_str(), std::ofstream::out | std::ofstream::app);
}

void Logger::log(std::wstring entry) {
	RotateIfNecessary();
	stream << entry << std::endl;
}

void Logger::close() {
	RotateIfNecessary();
	if (stream.is_open())
		stream.close();
}

bool Logger::IsTimeToRotate(SYSTEMTIME* logfileCreated) {
	SYSTEMTIME systemtime; 
	WIN32_FILE_ATTRIBUTE_DATA attributes;
	int result = GetFileAttributesEx(logfilename.c_str(), GetFileExInfoStandard, &attributes);
	//TODO exception handling
	if (result == 0 ) {
		int code = GetLastError();
	}
	FileTimeToSystemTime(&attributes.ftCreationTime, logfileCreated);

	GetSystemTime(&systemtime);
	//return false;
	return !(systemtime.wYear == logfileCreated->wYear && systemtime.wMonth == logfileCreated->wMonth && systemtime.wDay == logfileCreated->wDay);
}

bool Logger::RotateIfNecessary() {
	SYSTEMTIME logfileCreated;
	if (IsTimeToRotate(&logfileCreated)) {
		if (stream.is_open())
			stream.close();
		wchar_t buffer[1024];
		swprintf(buffer, 1024, (logfileBasePath + ARCHIVE_FILE_PATTERN).c_str(), logfileCreated.wYear, logfileCreated.wMonth, logfileCreated.wDay);

		int r = MoveFile(logfilename.c_str(), buffer); 

		//TODO exception handling
		if (r == 0) {
			int code = GetLastError();
		}

		stream.open(logfilename.c_str(), std::ofstream::out | std::ofstream::app);
		return true;
	}
	return false;
}
