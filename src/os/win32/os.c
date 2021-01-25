#include "os/os.h"
#include "engine.h"
#include <Windows.h>
#include <stdlib.h>
#include <string.h>

#include <Pathcch.h>
#pragma comment(lib, "Pathcch.lib") 

#define _SATIN_OPEN_FILE_MAX_PATH_LEN PATHCCH_MAX_CCH

struct OSData {
    WCHAR data_base_path[_SATIN_OPEN_FILE_MAX_PATH_LEN];
};

char os_folder_separator = '\\';

static void set_data_base_path(struct OSData *os_data, WCHAR *data_folder_name)
{

    HMODULE hModule = NULL;
    //HMODULE hModule=GetModuleHandleA(NULL);
	GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCWSTR)&reference_resolution,
		&hModule);
    GetModuleFileNameW(hModule,os_data->data_base_path,MAX_PATH);
    PathCchRemoveFileSpec(os_data->data_base_path,MAX_PATH);
	PathCchAppendEx(os_data->data_base_path, _SATIN_OPEN_FILE_MAX_PATH_LEN, data_folder_name, PATHCCH_ALLOW_LONG_PATHS);
}

struct OSData* os_data_create(void)
{
	struct OSData *os_data = calloc(1, sizeof(struct OSData));
	set_data_base_path(os_data, L"data/");
	return os_data;
}
void os_data_set_data_folder_name(struct OSData* os_data, char* path)
{
	size_t len = 0;
	mbstowcs_s(&len, NULL, 0, path, 0);
	WCHAR *buffer = calloc(len, sizeof(WCHAR));
	mbstowcs_s(&len, buffer, len*sizeof(WCHAR), path, _TRUNCATE);
	set_data_base_path(os_data, buffer);
	free(buffer);
}

FILE *open_file(const char *filename,const char *extension,const char *mode, struct OSData *os_data)
{
    WCHAR path_w[_SATIN_OPEN_FILE_MAX_PATH_LEN];

	memcpy(path_w, os_data->data_base_path, _SATIN_OPEN_FILE_MAX_PATH_LEN * sizeof(WCHAR));

	size_t path_len = wcslen(path_w);
	size_t filename_len = 0;
	mbstowcs_s(&filename_len, path_w + path_len,
		(_SATIN_OPEN_FILE_MAX_PATH_LEN - path_len),
		filename, _TRUNCATE);
	path_w[path_len + filename_len - 1] = L'.';
	size_t ext_len = 0;
	mbstowcs_s(&ext_len, path_w + path_len + filename_len,
		(_SATIN_OPEN_FILE_MAX_PATH_LEN - path_len - filename_len),
		extension, _TRUNCATE);

	wchar_t mode_w[32];
	size_t mode_len = 0;
	mbstowcs_s(&mode_len, mode_w, sizeof(mode_w)/sizeof(wchar_t), mode, _TRUNCATE);

	FILE *fp = 0;
	_wfopen_s(&fp, path_w, mode_w);

    return fp;
}

#include <commdlg.h>
char *get_save_file_name(const char *title)
{
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	char szFileName[MAX_PATH] = { 0 };
	ofn.lpstrFile = (LPSTR)szFileName;
	ofn.nMaxFile = MAX_PATH;
	if (GetSaveFileNameA(&ofn)) {
		return strdup(ofn.lpstrFile);
	}
    return 0;
}

char *get_open_file_name(const char *title)
{
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	char szFileName[MAX_PATH] = { 0 };
	ofn.lpstrFile = (LPSTR)szFileName;
	ofn.nMaxFile = MAX_PATH;
	if (GetOpenFileNameA(&ofn)) {
		return strdup(ofn.lpstrFile);
	}
    return 0;
}

#include "engine.h"

const char *get_computer_name(void)
{
	static char buffer[MAX_COMPUTERNAME_LENGTH + 2] = {0};
	int len = sizeof(buffer);
	if (buffer[0] == 0) {
		GetComputerNameA(buffer, &len);
	}
	return buffer;
}

int os_get_num_cores(void)
{
	SYSTEM_INFO system_info = { 0 };
	GetSystemInfo(&system_info);
	return system_info.dwNumberOfProcessors;
}

int os_is_key_down(int key) {
	return GetKeyState(key) < 0;
}

void os_set_clipboard_contents(void* os_data, char* string, size_t len)
{
	//TODO: Move to window.c
	/*
	struct OSData* data = os_data;
	if (!OpenClipboard(data->hWnd)) {
		return;
	}
	EmptyClipboard(); 

	HGLOBAL hglb = GlobalAlloc(GMEM_MOVEABLE, len); 
	if (hglb == NULL) 
	{ 
		CloseClipboard(); 
		return; 
	} 

	char *dest = GlobalLock(hglb); 
	memcpy(dest, string, len); 
	GlobalUnlock(hglb); 

	SetClipboardData(CF_TEXT, hglb); 

    CloseClipboard(); 
	*/
}

void os_path_strip_leaf(char *path) {
	if (path) {
		char *last_separator = 0;
		char *c = path;
		while (*c != 0)
		{
			if (*c == os_folder_separator) {
				last_separator = c;
			}
			c++;
		}
		if (last_separator) {
			last_separator[1] = 0;
		}
	}
}

#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib") 
int os_is_path_valid(char *path)
{
	return PathFileExistsA(path);
}

//#include "GL/glew.h"
//#include "GL/wglew.h"
#include <stdio.h>
#include <math.h>

int atomic_increment_int32(int *a) {
	return InterlockedIncrement(a);
}

char **get_args(int *argc)
{
	*argc = __argc;
	return __argv;
}

char *get_cwd()
{
	size_t len = GetCurrentDirectoryA(0, 0);
	char *ret = calloc(len + 1, 1);
	GetCurrentDirectoryA(len, ret);
	return ret;
}

int os_list_entries_in_folder(const char *path, const char **entries, int max_num_entries, enum OS_LIST_ENTRIES_TYPE type)
{
	int num_entries = 0;
	char *buffer = calloc(strlen(path) + 8, 1);
	sprintf(buffer, "%s/*.*", path);
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(buffer, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (
				(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || type & OS_LIST_ENTRIES_TYPE_FOLDER) &&
				( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || type & OS_LIST_ENTRIES_TYPE_FILE))
			{
				entries[num_entries] = strdup(fd.cFileName);
				num_entries++;
				if (num_entries == max_num_entries) {
					break;
				}
			}
		} while (FindNextFile(hFind, &fd));
		FindClose(hFind);
	}
	free(buffer);
	return num_entries;
}

int os_list_resource_entries(const char *data_path, const char** entries, int max_num_entries, enum OS_LIST_ENTRIES_TYPE type, struct OSData *os_data)
{
	size_t os_data_path_len = wcstombs(0,os_data->data_base_path,0);
	size_t len = os_data_path_len + strlen(data_path) + 1;
	char* buffer = calloc(len, 1);
	wcstombs(buffer, os_data->data_base_path, os_data_path_len);
	strcpy(buffer + os_data_path_len, data_path);
	int ret = os_list_entries_in_folder(buffer, entries, max_num_entries, type);
	free(buffer);
	return ret;
}

int os_does_file_exist(const char *filename)
{
	DWORD dwAttrib = GetFileAttributesA(filename);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

int os_does_folder_exist(const char *path)
{
	DWORD dwAttrib = GetFileAttributesA(path);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

int os_make_folder(const char* path)
{
	if (!os_does_folder_exist(path)) {
		return CreateDirectoryA(path, 0);
	}
	return 1;
}

void save_screenshot(const char* filename)
{
	const char* arg = "savescreenshotfull ";
	size_t arg_len = strlen(arg) + 1;
	size_t filename_len = strlen(filename) + 1;
	char* buffer = calloc(arg_len + filename_len + 32, 1);
	sprintf(buffer, "%s %s", arg, filename);
	ShellExecuteA(0, "open", "M:/_Common/Software/nircmd-x64/nircmdc.exe",
		buffer, 0, SW_HIDE);
	free(buffer);
}
