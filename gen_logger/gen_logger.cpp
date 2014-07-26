//Based on Winamp generic plugin template code.
#include "stdafx.h"
#include "gen_logger.h"
#include "wa_ipc.h"
#include <string>
#include <fstream>
#include "logger.h"

// these are callback functions/events which will be called by Winamp
int  init(void);
void config(void);
void quit(void);

static LRESULT WINAPI SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// this structure contains plugin information, version, name... 
// GPPHDR_VER is the version of the winampGeneralPurposePlugin (GPP) structure 
winampGeneralPurposePlugin plugin = {

	GPPHDR_VER,  // version of the plugin, defined in "gen_myplugin.h"
	PLUGIN_NAME, // name/title of the plugin, defined in "gen_myplugin.h"
	&init,        // function name which will be executed on init event
	&config,      // function name which will be executed on config event
	&quit,        // function name which will be executed on quit event
	0,           // handle to Winamp main window, loaded by winamp when this dll is loaded
	0            // hinstance to this dll, loaded by winamp when this dll is loaded

};
WNDPROC oldWndProc;
BOOL fUnicode;

const wchar_t* INI_KEY = L"SONG_LOGGER";
const wchar_t* LOGFILE_BASE_PATH_KEY = L"logfile_basepath";
const wchar_t* DEFAULT_BASE_PATH = L"c:\\logs\\";
const wchar_t* CURRFILENAME_KEY = L"current_file";
const wchar_t* ROTATE_FREQ_KEY = L"rotate_freq";
wchar_t ini_path[MAX_PATH] = {0};
void GetIniFilePath(HWND hwnd);

Logger logger;

// event functions follow

int init() {
	fUnicode = IsWindowUnicode(plugin.hwndParent);

	if(!fUnicode) {
		MessageBoxA(plugin.hwndParent, "Not Unicode !! May be problematic", "", MB_OK);
	}

	oldWndProc = (WNDPROC) ((fUnicode) ? SetWindowLongPtrW(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)SubclassProc) : 
		SetWindowLongPtrA(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)SubclassProc));

	GetIniFilePath(plugin.hwndParent);

	wchar_t basePath[MAX_PATH];
	GetPrivateProfileString( INI_KEY, LOGFILE_BASE_PATH_KEY, DEFAULT_BASE_PATH, basePath, MAX_PATH, ini_path );
	
	wchar_t currentFilename[Logger::MAX_FILENAME_LEN];
	GetPrivateProfileString( INI_KEY, CURRFILENAME_KEY, L"NULL", currentFilename, Logger::MAX_FILENAME_LEN, ini_path );

	wchar_t rotFreqStr[2];
	wchar_t defBuffer[2];
	swprintf(defBuffer, 2, L"%d", static_cast<int>(DAILY));
	GetPrivateProfileString( INI_KEY, ROTATE_FREQ_KEY, defBuffer, rotFreqStr, 2, ini_path );
	RotateFreq rotFreq = static_cast<RotateFreq> (stoi(rotFreqStr));

	logger.open(basePath, currentFilename, rotFreq);
	return 0;

}

void config() {

	wchar_t basePath[MAX_PATH];
	GetPrivateProfileString( INI_KEY, LOGFILE_BASE_PATH_KEY, DEFAULT_BASE_PATH, basePath, MAX_PATH, ini_path );

	// TODO config window / input dialog
	MessageBox(plugin.hwndParent, basePath, L"Saving to ini", MB_OK);
	
	int r = WritePrivateProfileString( INI_KEY, LOGFILE_BASE_PATH_KEY, basePath, ini_path );

	if (r == 0) {
		int code = GetLastError();
	}

	wchar_t defBuffer[2];
	swprintf(defBuffer, 2, L"%d", static_cast<int>(DAILY));
	r = WritePrivateProfileString( INI_KEY, ROTATE_FREQ_KEY, defBuffer, ini_path );

	if (r == 0) {
		int code = GetLastError();
	}
}

void quit() {

	if(fUnicode) SetWindowLongPtrW(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
	else SetWindowLongPtrA(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)oldWndProc);

	wstring currFilename = logger.close();

	WritePrivateProfileString( INI_KEY, CURRFILENAME_KEY, currFilename.c_str(), ini_path );
}

const int META_ITEMS = 4;
const wchar_t* META_DATA_NAMES_ARRAY[META_ITEMS] = { L"artist", L"title", L"album", L"albumartist"}; 
const int BUFFER_SIZE = 512;

wchar_t* GetMetaData(const wchar_t* filename, const wchar_t* metaname, wchar_t* buffer) {
	extendedFileInfoStructW efs;
	efs.filename=filename;
	efs.metadata=metaname;
	efs.ret=buffer;
	efs.retlen=BUFFER_SIZE;
	SendMessage(plugin.hwndParent,WM_WA_IPC,(WPARAM)&efs,IPC_GET_EXTENDED_FILE_INFOW);

	return buffer;
}

void WriteToLog(wchar_t* log);

std::wstring StartJSONWithTimeAndFileName(std::wstring& json, wchar_t* filename) {
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t buffer[1024];
	swprintf(buffer, 1024, L"{\"time\":%04d%02d%02d%02d%02d%02d, \"filename\":\"%s\"", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, filename);

	json.append(buffer);
	return json;
}

std::wstring AppendToJSON(wchar_t* metadata, int i, std::wstring& buffer){
	buffer.append(L", \"");
	buffer.append(META_DATA_NAMES_ARRAY[i]);
	buffer.append(L"\":\"");
	//TODO escape ' " backslash
	buffer.append(metadata);
	buffer.append(L"\"");

	return buffer;
}

static LRESULT WINAPI SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_USER:
		if (lParam  == IPC_PLAYING_FILEW ) {
			wchar_t* filename = (wchar_t*)wParam;

			wchar_t buffer[BUFFER_SIZE];
			//wchar_t json[4096];
			std::wstring json;
			StartJSONWithTimeAndFileName(json, filename);
			for (int i=0; i<META_ITEMS; i++) {
				GetMetaData(filename, META_DATA_NAMES_ARRAY[i], buffer);
				AppendToJSON(buffer, i, json);
			}
			json.append(L"}");
			//Sleep(10000);
			logger.log(json);

			//MessageBox(plugin.hwndParent,json.data(),L"meta",MB_OK);
		}
	}

	return (fUnicode) ? CallWindowProcW(oldWndProc, hwnd, msg, wParam, lParam) : CallWindowProcA(oldWndProc, hwnd, msg, wParam, lParam);
}


// This is an export function called by winamp which returns this plugin info. 
// We wrap the code in 'extern "C"' to ensure the export isn't mangled if used in a CPP file. 
extern "C" __declspec(dllexport) winampGeneralPurposePlugin* winampGetGeneralPurposePlugin() {

	return &plugin;

} 

void GetIniFilePath(HWND hwnd){
	if(SendMessage(hwnd,WM_WA_IPC,0,IPC_GETVERSION) >= 0x2900){
		// this gets the string of the full ini file path
		char *ini_pathA = (char*) SendMessage(hwnd, WM_WA_IPC, 0, IPC_GETINIFILE);
		const size_t cSize = strlen(ini_pathA)+1;
		mbstowcs(ini_path, ini_pathA, cSize);
	}
	else{
		// TODO throw error not supporting lower versions
		//wchar_t* p = ini_path;
		//p += GetModuleFileName(0,ini_path,sizeof(ini_path)) - 1;
		//while(p && *p != '.'){p--;}
		//lstrcpyn(p+1,L"ini",sizeof(ini_path));
	}
}
