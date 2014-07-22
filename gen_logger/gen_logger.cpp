//Based on Winamp generic plugin template code.
#include "stdafx.h"
#include <ctime>
#include <windows.h>
#include "gen_logger.h"
#include "wa_ipc.h"
#include <string>
#include <stdio.h>
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
char* LOG_DIR = "C:\\logs\\";
char* LOG_FILENAME = "C:\\logs\\log.txt";
Logger logger;

// event functions follow

int init() {
	fUnicode = IsWindowUnicode(plugin.hwndParent);
	oldWndProc = (WNDPROC) ((fUnicode) ? SetWindowLongPtrW(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)SubclassProc) : 
		SetWindowLongPtrA(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)SubclassProc));

	logger.open();
	return 0;

}

void config() {

	//A basic messagebox that tells you the 'config' event has been triggered.
	//You can change this later to do whatever you want (including nothing)
	MessageBox(plugin.hwndParent, L"Config event triggered for gen_myplugin.", L"", MB_OK);

	int version = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETVERSION); 
	int majVersion = WINAMP_VERSION_MAJOR(version); 
	int minVersion = WINAMP_VERSION_MINOR(version);
	wchar_t msg[1024]; 
	wsprintf(msg,L"The version of Winamp is: %x\n Major version: %x\nMinor version: %x\n",
		version,
		majVersion,
		minVersion
		);

	MessageBox(plugin.hwndParent,msg,L"Winamp Version",MB_OK); 

}

void quit() {

	if(fUnicode) SetWindowLongPtrW(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
	else SetWindowLongPtrA(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)oldWndProc);

	logger.close();
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

			MessageBox(plugin.hwndParent,json.data(),L"meta",MB_OK);
		}
	}

	return (fUnicode) ? CallWindowProcW(oldWndProc, hwnd, msg, wParam, lParam) : CallWindowProcA(oldWndProc, hwnd, msg, wParam, lParam);
}


// This is an export function called by winamp which returns this plugin info. 
// We wrap the code in 'extern "C"' to ensure the export isn't mangled if used in a CPP file. 
extern "C" __declspec(dllexport) winampGeneralPurposePlugin* winampGetGeneralPurposePlugin() {

	return &plugin;

} 

