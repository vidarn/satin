#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
void win32_set_parent_window(void *os_data, HWND parent_hWnd)
;
void win32_set_icon(void *os_data, HICON icon)
;