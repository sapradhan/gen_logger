#include <fstream>
#include <Windows.h>
#include <string>
using namespace std;
class Logger
{
private: 
	static const wstring LOG_FILENAME;
	static const wstring ARCHIVE_FOLDER;
	static const wstring ARCHIVE_FILE_PATTERN;

	wofstream stream;
	wstring logfileBasePath;
	wstring logfilename;

	bool IsTimeToRotate(SYSTEMTIME*);
	bool RotateIfNecessary();

public:
	Logger(void);
	void open(wstring basePath);
	void log(wstring);
	void close(void);
	~Logger();
};