#include "stdafx.h"
#include "logger.h"
#include "resource.h"

wchar_t* escape(wchar_t* in, wchar_t* buffer);
RotateFreq RadioButtonToEnum( HWND hWnd );
int EnumToRadioButton( RotateFreq freq );
wchar_t* AddTrailingBackSlash(wchar_t* path, size_t maxlen = MAX_PATH);