#include <fstream>

class Logger
{
private: 
	std::wofstream stream;
	bool IsTimeToRotate();
	void Rotate();
	
public:
	Logger(void);
	void open();
	void log(std::wstring);
	void close(void);
	~Logger();
};

struct loggerParams {

}