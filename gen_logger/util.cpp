#include "stdafx.h"
#include "util.h"

wchar_t* escape(wchar_t* in, wchar_t* buffer) {
	int j = 0;
	for ( int i = 0; in[i] != 0; i++ ){
		switch ( in[i] ) { 
		case L'\b':
		case L'\f':
		case L'\n':
		case L'\r':
		case L'\t': buffer[j++] = L' '; break;

		case L'\"': 
		case L'\\': buffer[j++] = L'\\';
			buffer[j++] = in[i];
			break;
		default : buffer[j++] = in[i];
		}
	}
	buffer[j] = 0;
	return buffer;
}

RotateFreq RadioButtonToEnum( HWND hWnd ) {
	RotateFreq freq = DAILY;
	if (IsDlgButtonChecked(hWnd, R_MONTHLY) == BST_CHECKED) { 
		freq = MONTHLY;
	} else if (IsDlgButtonChecked(hWnd, R_DAILY) == BST_CHECKED) {
		freq = DAILY;
	} else if (IsDlgButtonChecked(hWnd, R_HOURLY) == BST_CHECKED) {
		freq = HOURLY;
	}
	//freq = (IsDlgButtonChecked(hWnd, R_MONTHLY) == BST_CHECKED) ? MONTHLY :
	//((IsDlgButtonChecked(hWnd, R_DAILY) == BST_CHECKED) ? DAILY : 
	//	((IsDlgButtonChecked(hWnd, R_HOURLY) == BST_CHECKED) ? HOURLY : DAILY)
	//	);

	return freq;
}

int EnumToRadioButton( RotateFreq freq ) {
	switch (freq) {
	case MONTHLY: 
		return R_MONTHLY;
	case DAILY:
		return R_DAILY;
	case HOURLY:
		return R_HOURLY;
	default:
		return R_DAILY;
	}
}

wchar_t* AddTrailingBackSlash(wchar_t* path, size_t maxlen) {
	size_t len =  wcslen(path);
	if (len < maxlen && path[len - 1] != L'\\') {
		path[len] = L'\\';
		path[len + 1] = 0;
	}

	return path;
}