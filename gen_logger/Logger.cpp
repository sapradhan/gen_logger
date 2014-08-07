#include "StdAfx.h"
#include "logger.h"
#include <string>
#include <fstream>
#include "wa_ipc.h"
#include <Windows.h>
#include <locale>
#include <codecvt>
using namespace std;

const std::locale utf8_locale = std::locale(std::locale(),
	new std::codecvt_utf8<wchar_t>());
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

	CreateArchiveDir();

	if (!RotateIfNecessary()) {
		stream.open((basePath + currLogFilename).c_str(), std::ofstream::out | std::ofstream::app);
		stream.imbue(utf8_locale);
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

		MoveToArchive();

		currLogFilename = newLogFilename;
		stream.open((logfileBasePath + currLogFilename).c_str(), std::ofstream::out | std::ofstream::app);
		stream.imbue(utf8_locale);
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

void Logger::MoveToArchive() {
	int r = MoveFile((logfileBasePath + currLogFilename).c_str(), (logfileBasePath + ARCHIVE_FOLDER + currLogFilename).c_str()); 

	if (r == 0) {
		int code = GetLastError();
		wchar_t buffer[MAX_PATH + 64];
		wsprintf(buffer, L"Could not create archive file %s. Error code: %u", (logfileBasePath + ARCHIVE_FOLDER + currLogFilename).c_str(), code);
		MessageBox(NULL, buffer, L"ERROR", MB_OK);
	}
}

void Logger::ForceCloseAndRotate() {
	if (stream.is_open())
		stream.close();

	MoveToArchive();

}

void Logger::CreateArchiveDir() { 
	int r = CreateDirectory((logfileBasePath + ARCHIVE_FOLDER).c_str(), NULL);

	if (r == 0) {
		int code = GetLastError();
		if (code != ERROR_ALREADY_EXISTS ) {

			wchar_t buffer[MAX_PATH + 64];
			wsprintf(buffer, L"Could not create archive folder %s. Error code: %u", (logfileBasePath + ARCHIVE_FOLDER).c_str(), code);
			MessageBox(NULL, buffer, L"ERROR", MB_OK);
		}
	}
}

