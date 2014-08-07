//Based on Winamp generic plugin template code.
#include "stdafx.h"
#include "gen_logger.h"
#include "wa_ipc.h"
#include <string>
#include <fstream>
#include <ShlObj.h>
#include <tchar.h>
#include "logger.h"
#include "resource.h"
#include "util.h"


// these are callback functions/events which will be called by Winamp
int  init(void);
void config(void);
void quit(void);

static LRESULT WINAPI SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ConfigDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool ExecuteConfigModalDialog(HWND hParent);

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
const wchar_t* CURRFILENAME_KEY = L"current_file";
const wchar_t* ROTATE_FREQ_KEY = L"rotate_freq";

//defaults
const wchar_t* DEFAULT_BASE_PATH = L"c:\\logs\\";
const RotateFreq DEFAULT_ROTATE_FREQ = DAILY;


//config
RotateFreq rotFreq;
wchar_t basePath[MAX_PATH];

wchar_t currentFilename[Logger::MAX_FILENAME_LEN];
wchar_t ini_path[MAX_PATH] = {0};
void GetIniFilePath(HWND hwnd);
void UpdateSettings(wchar_t* newBasePath, RotateFreq newFreq);
void ReadConfig();
void WriteConfig(wchar_t* basePath, RotateFreq freq);

RotateFreq RadioButtonToEnum( HWND hWnd );
int EnumToRadioButton( RotateFreq freq );

Logger logger;

// event functions follow

int init() {
	fUnicode = IsWindowUnicode(plugin.hwndParent);

	if(!fUnicode) {
		MessageBoxA(plugin.hwndParent, "Not Unicode !! May be problematic", "", MB_OK);
	}

	oldWndProc = (WNDPROC) ((fUnicode) ? SetWindowLongPtrW(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)SubclassProc) : 
		SetWindowLongPtrA(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)SubclassProc));

	ReadConfig();

	logger.open(basePath, currentFilename, rotFreq);
	return 0;

}

void config() {
	HWND hParent = (HWND) SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETPREFSWND);
	if(!IsWindow(hParent)) {
		hParent = (HWND) SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETDIALOGBOXPARENT);
		if(!IsWindow(hParent)) {
			hParent=plugin.hwndParent;
		}
	}
	ExecuteConfigModalDialog (hParent);

	return;
}

void quit() {

	if(fUnicode) SetWindowLongPtrW(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
	else SetWindowLongPtrA(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)oldWndProc);

	wstring currFilename = logger.close();

	WritePrivateProfileString( INI_KEY, CURRFILENAME_KEY, currFilename.c_str(), ini_path );
}

bool ExecuteConfigModalDialog(HWND hParent) {
	int res= DialogBox(
		plugin.hDllInstance,
		MAKEINTRESOURCE(IDD_CONFIG_DIALOG),
		hParent,
		ConfigDlgProc
		);
	return (res >= 1);
}

const wchar_t* META_DATA_NAMES_ARRAY[] = { L"artist", L"title", L"album", L"albumartist", L"composer", L"lyricist"}; 
const int META_ITEMS = sizeof(META_DATA_NAMES_ARRAY) / sizeof(wchar_t*);
const int BUFFER_SIZE = 1024;

wchar_t* GetMetaData(const wchar_t* filename, const wchar_t* metaname, wchar_t* buffer) {
	extendedFileInfoStructW efs;
	efs.filename=filename;
	efs.metadata=metaname;
	efs.ret=buffer;
	efs.retlen=BUFFER_SIZE;
	SendMessage(plugin.hwndParent,WM_WA_IPC,(WPARAM)&efs,IPC_GET_EXTENDED_FILE_INFOW);

	return buffer;
}

std::wstring StartJSONWithTimeAndFileName(std::wstring& json, wchar_t* filename) {
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t buffer[1024];
	wchar_t filenameBuffer[MAX_PATH * 2];
	swprintf(buffer, 1024, L"{\"time\":%04d%02d%02d%02d%02d%02d, \"filename\":\"%s\"", 
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, 
		escape(filename,filenameBuffer));

	json.append(buffer);
	return json;
}

std::wstring AppendToJSON(wchar_t* metadata, int i, std::wstring& buffer){
	buffer.append(L", \"");
	buffer.append(META_DATA_NAMES_ARRAY[i]);
	buffer.append(L"\":\"");

	wchar_t b[2 * BUFFER_SIZE];
	buffer.append(escape(metadata, b));
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
			json.append(L"},");
			//Sleep(10000);
			logger.log(json);

			//MessageBox(plugin.hwndParent,json.data(),L"meta",MB_OK);
		}
	}

	return (fUnicode) ? CallWindowProcW(oldWndProc, hwnd, msg, wParam, lParam) : CallWindowProcA(oldWndProc, hwnd, msg, wParam, lParam);
}

BOOL CALLBACK ConfigDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {

	case WM_INITDIALOG:
		SetDlgItemText(hWnd, EDIT_LOG_FOLDER, basePath);
		CheckRadioButton(hWnd, R_MONTHLY, R_HOURLY, EnumToRadioButton(rotFreq));
		break;

	case WM_COMMAND:
		switch(HIWORD(wParam)) {
		case BN_CLICKED:
			switch(LOWORD(wParam)) {
			case IDOK:
				wchar_t basePathBuffer[MAX_PATH];
				RotateFreq freq;
				GetDlgItemText(hWnd, EDIT_LOG_FOLDER, basePathBuffer, MAX_PATH);
				freq = RadioButtonToEnum(hWnd);

				wchar_t buffer[1024];
				swprintf(buffer, 1024, L"%s %d", basePathBuffer, freq);

				MessageBox(plugin.hwndParent, buffer, L"meta", MB_OK);
				EndDialog(hWnd, 1);

				UpdateSettings( basePathBuffer, freq );

				WriteConfig( basePathBuffer, freq );
				return TRUE;

			case IDCANCEL: 
				EndDialog(hWnd, 0);
				break;

			case LOG_FOLDER_BROWSE:
				CoInitialize(NULL);
				BROWSEINFO bInfo;
				ZeroMemory(&bInfo, sizeof(BROWSEINFO));
				bInfo.hwndOwner = hWnd;
				bInfo.pidlRoot = NULL;
				bInfo.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS;
				bInfo.lpfn = NULL;
				bInfo.lParam = NULL;
				bInfo.lpszTitle = _T("Pick a Directory");
				LPITEMIDLIST pidl;
				pidl = SHBrowseForFolder ( &bInfo );
				if ( pidl != 0 ) {
					// get the name of the folder
					TCHAR path[MAX_PATH];
					if ( SHGetPathFromIDList ( pidl, path ) ) {
						SetDlgItemText(hWnd, EDIT_LOG_FOLDER, path);
					}

					// free memory used
					IMalloc * imalloc = 0;
					if ( SUCCEEDED( SHGetMalloc ( &imalloc )) ) {
						imalloc->Free ( pidl );
						imalloc->Release ( );
					}
				}
				CoUninitialize();
				break;
			}}
		break;

	case WM_CLOSE:
		EndDialog(hWnd, 0);
		break;

		/*default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);*/
	}
	return FALSE;
}



// This is an export function called by winamp which returns this plugin info. 
// We wrap the code in 'extern "C"' to ensure the export isn't mangled if used in a CPP file. 
extern "C" __declspec(dllexport) winampGeneralPurposePlugin* winampGetGeneralPurposePlugin() {

	return &plugin;

} 

void GetIniFilePath(HWND hwnd) {
	if(SendMessage(hwnd,WM_WA_IPC,0,IPC_GETVERSION) >= 0x2900) {
		// this gets the string of the full ini file path
		char *ini_pathA = (char*) SendMessage(hwnd, WM_WA_IPC, 0, IPC_GETINIFILE);
		size_t x;
		size_t cSize = strlen(ini_pathA)+1;
		mbstowcs_s(&x, ini_path, ini_pathA, cSize);
	} else {
		// TODO throw error not supporting lower versions
		//wchar_t* p = ini_path;
		//p += GetModuleFileName(0,ini_path,sizeof(ini_path)) - 1;
		//while(p && *p != '.'){p--;}
		//lstrcpyn(p+1,L"ini",sizeof(ini_path));
	}
}

void UpdateSettings(wchar_t* newBasePath, RotateFreq newFreq) {
	bool settingsChanged = false;
	AddTrailingBackSlash(newBasePath);
	if ( wcscmp (newBasePath, basePath) != 0 ){
		settingsChanged = true;
		wcscpy_s(basePath, MAX_PATH, newBasePath);
	}
	if ( newFreq != rotFreq ) {
		settingsChanged = true;
		rotFreq = newFreq;
	}

	if(settingsChanged){
		logger.close();

		logger.open(basePath, currentFilename, rotFreq);
	}

}

void ReadConfig() {
	GetIniFilePath(plugin.hwndParent);

	GetPrivateProfileString( INI_KEY, LOGFILE_BASE_PATH_KEY, DEFAULT_BASE_PATH, basePath, MAX_PATH, ini_path );

	GetPrivateProfileString( INI_KEY, CURRFILENAME_KEY, L"NULL", currentFilename, Logger::MAX_FILENAME_LEN, ini_path );

	wchar_t rotFreqStr[2];
	wchar_t defBuffer[2];
	swprintf(defBuffer, 2, L"%d", static_cast<int>(DEFAULT_ROTATE_FREQ));
	GetPrivateProfileString( INI_KEY, ROTATE_FREQ_KEY, defBuffer, rotFreqStr, 2, ini_path );
	rotFreq = static_cast<RotateFreq> (stoi(rotFreqStr));

}

void WriteConfig(wchar_t* basePath, RotateFreq freq) {
	int r = WritePrivateProfileString( INI_KEY, LOGFILE_BASE_PATH_KEY, basePath, ini_path );

	if (r == 0) {
		int code = GetLastError();
	}

	wchar_t defBuffer[2];
	swprintf(defBuffer, 2, L"%d", static_cast<int>(freq));
	r = WritePrivateProfileString( INI_KEY, ROTATE_FREQ_KEY, defBuffer, ini_path );

	if (r == 0) {
		int code = GetLastError();
	}
}


