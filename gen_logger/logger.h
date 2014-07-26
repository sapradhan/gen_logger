#include <fstream>
#include <Windows.h>
#include <string>
using namespace std;

enum RotateFreq { MONTHLY, DAILY, HOURLY, EVERY_MINUTE };

class Logger
{
private: 
	//static const wstring LOG_FILENAME;
	static const wstring ARCHIVE_FOLDER;
	static const wstring LOG_FILE_PATTERN;

	wofstream stream;
	wstring logfileBasePath;
	wstring logfileFullPath;
	wstring currLogFilename;

	bool IsTimeToRotate(wstring& calcFilename);
	bool RotateIfNecessary();
	wchar_t* CalcLogFilename(wchar_t* buffer);


public:
	static const int MAX_FILENAME_LEN = 24;
	Logger(void);
	void open(wstring basePath, wstring currFilename, RotateFreq r = DAILY);
	void log(wstring);
	wstring close(void);
	~Logger();

private: 
	RotateFreq rotateFreqency;

};