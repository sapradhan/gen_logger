#include "StdAfx.h"
#include "logger.h"
#include <string>
#include <fstream>
#include "wa_ipc.h"
#include <Windows.h>
using namespace std;

//char* LOG_DIR = "C:\\logs\\";
//const wstring Logger::LOG_FILENAME = L"log.txt";
const wstring Logger::ARCHIVE_FOLDER = L"archive\\";

Logger::Logger() {

}

Logger::~Logger(void) { 
	close();
}

void Logger::open(wstring basePath, wstring currentFilename, RotateFreq freq) {
	logfileBasePath = basePath;
	currLogFilename = currentFilename;
	rotateFreqency = freq;

	if (!RotateIfNecessary()) {
		stream.open((basePath + currLogFilename).c_str(), std::ofstream::out | std::ofstream::app);
	}
}

void Logger::log(std::wstring entry) {
	RotateIfNecessary();
	stream << entry << std::endl;
}

wstring Logger::close() {
	RotateIfNecessary();
	if (stream.is_open())
		stream.close();
	return currLogFilename;
}

bool Logger::IsTimeToRotate(wstring& calcFilename) {
	wchar_t buffer[24];
	calcFilename = CalcLogFilename(buffer);

	return calcFilename.compare(currLogFilename) != 0; 
}

bool Logger::RotateIfNecessary() {
	wstring newLogFilename;
	wstring& newLogFileref = newLogFilename;
	if (IsTimeToRotate(newLogFileref)) {
		if (stream.is_open())
			stream.close();

		int r = MoveFile((logfileBasePath + currLogFilename).c_str(), (logfileBasePath + ARCHIVE_FOLDER + currLogFilename).c_str()); 

		//TODO exception handling
		if (r == 0) {
			int code = GetLastError();
			r++;
		}

		currLogFilename = newLogFilename;
		stream.open((logfileBasePath + currLogFilename).c_str(), std::ofstream::out | std::ofstream::app);
		return true;
	}
	return false;
}

wchar_t* Logger::CalcLogFilename(wchar_t* buffer) {
	SYSTEMTIME localtime; 
	GetLocalTime(&localtime);
	switch (rotateFreqency) {
	case EVERY_MINUTE:
		swprintf(buffer, MAX_FILENAME_LEN, L"%04d%02d%02d%02d%02d.txt", localtime.wYear, localtime.wMonth, localtime.wDay, localtime.wHour, localtime.wMinute);
		break;
	case HOURLY:
		swprintf(buffer, MAX_FILENAME_LEN, L"%04d%02d%02d%02d.txt", localtime.wYear, localtime.wMonth, localtime.wDay, localtime.wHour);
		break;
	case DAILY:
		swprintf(buffer, MAX_FILENAME_LEN, L"%04d%02d%02d.txt", localtime.wYear, localtime.wMonth, localtime.wDay);
		break;
	case MONTHLY:
		swprintf(buffer, MAX_FILENAME_LEN, L"%04d%02d.txt", localtime.wYear, localtime.wMonth);
		break;
	}
	return buffer;
}
