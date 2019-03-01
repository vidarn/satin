#include "os/os.h"
#include "engine.h"
#include <Windows.h>
#include <stdlib.h>

#include <Pathcch.h>
#pragma comment(lib, "Pathcch.lib") 

#define _SATIN_OPEN_FILE_MAX_PATH_LEN PATHCCH_MAX_CCH

struct Win32Data {
    WCHAR data_base_path[_SATIN_OPEN_FILE_MAX_PATH_LEN];
};

static void set_data_base_path(struct Win32Data *os_data, WCHAR *data_folder_name)
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

void *os_data_create(void)
{
	struct Win32Data *os_data = calloc(1, sizeof(struct Win32Data));
	set_data_base_path(os_data, L"data/");
	return os_data;
}
void os_data_set_data_folder_name(void *os_data, char *path)
{
	size_t len = 0;
	mbstowcs_s(&len, NULL, 0, path, 0);
	WCHAR *buffer = calloc(len, sizeof(WCHAR));
	mbstowcs_s(&len, buffer, len*sizeof(WCHAR), path, _TRUNCATE);
	set_data_base_path(os_data, buffer);
	free(buffer);
}

FILE *open_file(const char *filename,const char *extension,const char *mode, struct GameData *data)
{
    WCHAR path_w[_SATIN_OPEN_FILE_MAX_PATH_LEN];

	struct Win32Data *os_data = (struct Win32Data *)get_os_data(data);
	memcpy(path_w, os_data->data_base_path, _SATIN_OPEN_FILE_MAX_PATH_LEN * sizeof(WCHAR));

	size_t path_len = wcslen(path_w);
	size_t filename_len = 0;
	mbstowcs_s(&filename_len, path_w + path_len,
		(_SATIN_OPEN_FILE_MAX_PATH_LEN - path_len),
		filename, _TRUNCATE);
	size_t ext_len = 0;
	mbstowcs_s(&ext_len, path_w + path_len + filename_len-1,
		(_SATIN_OPEN_FILE_MAX_PATH_LEN - path_len - filename_len+1),
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

LARGE_INTEGER counter_frequency;

#include "engine.h"
uint64_t get_current_tick(void)
{
    LARGE_INTEGER current_tick;
    QueryPerformanceCounter(&current_tick);
    current_tick.QuadPart*=TICKS_PER_SECOND;
    current_tick.QuadPart/=counter_frequency.QuadPart;
    return (int)current_tick.QuadPart;
}

const char *get_computer_name(void)
{
	static char buffer[MAX_COMPUTERNAME_LENGTH + 2] = {0};
	int len = sizeof(buffer);
	if (buffer[0] == 0) {
		GetComputerNameA(buffer, &len);
	}
	return buffer;
}

int get_num_cores(void)
{
	SYSTEM_INFO system_info = { 0 };
	GetSystemInfo(&system_info);
	return system_info.dwNumberOfProcessors;
}

int os_is_key_down(int key) {
	return GetKeyState(key) < 0;
}

//#include "GL/glew.h"
//#include "GL/wglew.h"
#include "opengl.h"
#include <stdio.h>
#include <math.h>

int atomic_increment_int32(int *a) {
	return InterlockedIncrement(a);
}


struct InputState input_state = { 0 };
int framebuffer_w = 0;
int framebuffer_h = 0;


static LONG WINAPI
WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static PAINTSTRUCT ps;

	switch (uMsg) {
	case WM_SIZE:
		framebuffer_w = LOWORD(lParam);
		framebuffer_h = HIWORD(lParam);
		PostMessage(hWnd, WM_PAINT, 0, 0);
		return 0;

	case WM_CHAR:
		switch (wParam) {
		case 27:			/* ESC key */
			break;
		}
		return 0;

	case WM_LBUTTONDOWN:
	{
		POINT p = {
			LOWORD(lParam),
			HIWORD(lParam)
		};
		if (DragDetect(hWnd, p)) {
			printf("Drag\n");
			input_state.mouse_state = MOUSE_DRAG;
			POINT p;
			GetCursorPos(&p);
			ScreenToClient(hWnd, &p);
			RECT r;
			GetClientRect(hWnd, &r);
			input_state.drag_start_x = (float)p.x / (float)(r.right - r.left);
			input_state.drag_start_y = (float)p.y / (float)(r.top - r.bottom) + 1.f;
		}
		else {
			printf("Click!\n");
			input_state.mouse_state = MOUSE_CLICKED;
		}
	}
	return 0;

	case WM_LBUTTONUP:
		printf("Drag stop\n");
		input_state.mouse_state = MOUSE_NOTHING;
		return 0;

	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static HWND
CreateOpenGLWindow(const char* title, int x, int y, int width, int height,
	BYTE type, DWORD flags)
{
	int         pf;
	HDC         hDC;
	HWND        hWnd;
	WNDCLASSA    wc;
	PIXELFORMATDESCRIPTOR pfd;
	static HINSTANCE hInstance = 0;

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	/* only register the window class once - use hInstance as a flag. */
	if (!hInstance) {
		hInstance = GetModuleHandle(NULL);
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = (WNDPROC)WindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = "OpenGL";

		if (!RegisterClassA(&wc)) {
			MessageBoxA(NULL, "RegisterClass() failed:  "
				"Cannot register window class.", "Error", MB_OK);
			return NULL;
		}
	}

	hWnd = CreateWindowA("OpenGL", title, WS_OVERLAPPEDWINDOW |
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		x, y, width, height, NULL, NULL, hInstance, NULL);

	if (hWnd == NULL) {
		MessageBoxA(NULL, "CreateWindow() failed:  Cannot create a window.",
			"Error", MB_OK);
		return NULL;
	}

	hDC = GetDC(hWnd);

	/* there is no guarantee that the contents of the stack that become
	the pfd are zeroed, therefore _make sure_ to clear these bits. */
	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | flags;
	pfd.iPixelType = type;
	pfd.cColorBits = 32;

	pf = ChoosePixelFormat(hDC, &pfd);
	if (pf == 0) {
		MessageBoxA(NULL, "ChoosePixelFormat() failed:  "
			"Cannot find a suitable pixel format.", "Error", MB_OK);
		return 0;
	}

	if (SetPixelFormat(hDC, pf, &pfd) == FALSE) {
		MessageBoxA(NULL, "SetPixelFormat() failed:  "
			"Cannot set format specified.", "Error", MB_OK);
		return 0;
	}

	DescribePixelFormat(hDC, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	ReleaseDC(hWnd, hDC);

	return hWnd;
}

 void 
error_message_callback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
}


#include <Xinput.h>
float controller_left_thumb_deadzone = (float)XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/32768.f;
float controller_right_thumb_deadzone = (float)XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE/32768.f;

void launch_game(const char *window_title, int _framebuffer_w, int _framebuffer_h, int show_console, 
	int num_game_states, void *param, struct GameState *game_states, int debug_mode)
{
	framebuffer_w = _framebuffer_w;
	framebuffer_h = _framebuffer_h;
	OutputDebugStringA("Launching game!\n");

	if (show_console) {
		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
	}

	HDC hDC;				/* device context */
	HGLRC hRC_tmp;				/* opengl context */
	HGLRC hRC;				/* opengl context */
	HWND  hWnd;				/* window */
	MSG   msg;				/* message */

	hWnd = CreateOpenGLWindow(window_title, 0, 0, framebuffer_w, framebuffer_h, PFD_TYPE_RGBA, 0);
	if (hWnd == NULL)
		exit(1);

	hDC = GetDC(hWnd);
	hRC_tmp = wglCreateContext(hDC);
	wglMakeCurrent(hDC, hRC_tmp);

	opengl_load();

	/*
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		OutputDebugStringA("Error: \n");
		MessageBoxA(NULL, "GLEW error"
			"Could not initialize", "Error", MB_OK);
		return;
	}
	*/

	int attribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONTEXT_FLAGS_ARB, 0,
		0
	};
	if (debug_mode) {
		attribs[2] |= WGL_CONTEXT_DEBUG_BIT_ARB;
	}
	hRC = wglCreateContextAttribsARB(hDC, 0, attribs);
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hRC_tmp);
	wglMakeCurrent(hDC, hRC);

	if (debug_mode) {
		glEnable(GL_DEBUG_OUTPUT);
		//glDebugMessageCallback(error_message_callback, 0);
		//TODO(Vidar):Implement this
		PROC proc = wglGetProcAddress("glDebugMessageCallback");
		if (proc) {
			printf("hej\n");
		}
		else {
			printf("Blaj\n");
		}
	}

	const GLubyte *GLVersionString = glGetString(GL_VERSION);
	OutputDebugStringA("GL version: ");
	OutputDebugStringA((const char*)GLVersionString);
	OutputDebugStringA("\n");

	ShowWindow(hWnd, SW_SHOW);

	struct Win32Data* os_data = os_data_create();
	struct GameData* game_data = init(num_game_states, game_states, param, os_data, debug_mode);

	HANDLE timer = CreateWaitableTimerA(NULL, TRUE, "Engine FPS timer");
	if (!timer) {
		MessageBoxA(hWnd, "Could not create multimedia timer", "Error!", MB_OK | MB_ICONERROR);
	}
	LARGE_INTEGER timer_interval;
	timer_interval.QuadPart = -1e7 / 60;
	SetWaitableTimer(timer, &timer_interval, 0, 0, 0, FALSE);

	int done = 0;
	LARGE_INTEGER last_tick;
	QueryPerformanceCounter(&last_tick);
	QueryPerformanceFrequency(&counter_frequency);
	long int drag_start_x = 0;
	long int drag_start_y = 0;
	int cursor_visible = 1;
	int wait_for_event = 0;
	while (!done) {
		int event_recieved;
		if ((event_recieved = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) || wait_for_event) {
			wait_for_event = 0;
			if (!event_recieved) {
				GetMessageA(&msg, NULL, 0, 0);
			}
			switch (msg.message) {
			case WM_QUIT:
				done = 1;
				break;
			case WM_CHAR:
			{
				/*
				OutputDebugStringA("Key input: ");
				char c[2] = { 0 };
				c[0] = (char)msg.wParam;
				OutputDebugStringA(c);
				OutputDebugStringA("\n");
				*/
				if (input_state.num_keys_typed<max_num_keys_typed) {
					input_state.keys_typed[input_state.num_keys_typed] = msg.wParam;
					input_state.num_keys_typed++;
				}
				break;
			}
			case WM_KEYDOWN:
			{
				int handled = 0;
				if (input_state.num_keys_typed < max_num_keys_typed) {
					int ok = 0;
					switch (msg.wParam)
					{
					case VK_LEFT:
						ok = 1; break;
					case VK_RIGHT:
						ok = 1; break;
					}
					if (ok) {
						input_state.keys_typed[input_state.num_keys_typed] = msg.wParam;
						input_state.num_keys_typed++;
						handled = 1;
					}
				}
				if (!handled) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				break;
			}
			case WM_LBUTTONDOWN:
			{
				input_state.mouse_state = MOUSE_CLICKED;
				input_state.mouse_down = 1;
				//POINTS tmp = MAKEPOINTS(msg.lParam);
				POINT p;
				GetCursorPos(&p);
				ScreenToClient(hWnd, &p);
				drag_start_x = p.x;
				drag_start_y = p.y;
				break;
			}
			case WM_LBUTTONUP:
			{
				input_state.mouse_down = 0;
				input_state.mouse_state = MOUSE_NOTHING;
				break;
			}
			case WM_MOUSEWHEEL:
			{
				input_state.scroll_delta_y = (float)GET_WHEEL_DELTA_WPARAM(msg.wParam)/120.f;
			}
			default:
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				break;
			}
		}
		else {
			WaitForSingleObject(timer, INFINITE);
			SetWaitableTimer(timer, &timer_interval, 0, 0, 0, FALSE);
			POINT p;
			GetCursorPos(&p);
			ScreenToClient(hWnd, &p);
			RECT r;
			GetClientRect(hWnd, &r);
			float p_x = p.x - (float)r.left;
			float p_y = p.y - (float)r.top;
			float drag_x = drag_start_x - r.left;
			float drag_y = drag_start_y - r.top;
			float w = r.right - r.left;
			float h = r.bottom - r.top;
			float m_x;
			float m_y;
			float d_x;
			float d_y;
			if (w<h) {
				m_x = p_x / w;
				m_y = 1.f - (p_y / w - 0.5*(h - w) / w);
				d_x = drag_x / w;
				d_y = 1.f - (drag_y / w - 0.5*(h - w) / w);
			}
			else {
				m_x = p_x / h - 0.5f*(w - h) / h;
				m_y = 1.f - p_y / h;
				d_x = drag_x / h - 0.5f*(w - h) / h;
				d_y = 1.f - drag_y / h;
			}
			input_state.delta_x = m_x - input_state.mouse_x;
			input_state.delta_y = m_y - input_state.mouse_y;
			if (cursor_locked(game_data)) {
				m_x -= input_state.delta_x;
				m_y -= input_state.delta_y;
				d_x -= input_state.delta_x;
				d_y -= input_state.delta_y;

				POINT tmp_p = { drag_start_x, drag_start_y };
				ClientToScreen(hWnd, &tmp_p);
				if (cursor_visible) {
					while (ShowCursor(FALSE) >= 0) {}
				}
				cursor_visible = 0;
				SetCursorPos(tmp_p.x, tmp_p.y);
			}
			else {
				if (!cursor_visible) {
					while (ShowCursor(TRUE) < 0) {}
				}
				cursor_visible = 1;
			}
			input_state.mouse_x = m_x;
			input_state.mouse_y = m_y;
			input_state.drag_start_x = d_x;
			input_state.drag_start_y = d_y;

			for (int i_controller = 0; i_controller < max_num_controllers; i_controller++) {
				XINPUT_STATE xinput_state = { 0 };
				DWORD result = XInputGetState(i_controller, &xinput_state);
				if (result == ERROR_SUCCESS) {
					input_state.controllers[i_controller].valid = 1;
					input_state.controllers[i_controller].buttons_pressed = xinput_state.Gamepad.wButtons;
					input_state.controllers[i_controller].left_trigger = (float)xinput_state.Gamepad.bLeftTrigger/255.f;
					input_state.controllers[i_controller].right_trigger = (float)xinput_state.Gamepad.bRightTrigger/255.f;
					input_state.controllers[i_controller].left_thumb[0] = (float)xinput_state.Gamepad.sThumbLX/32768.f;
					input_state.controllers[i_controller].left_thumb[1] = (float)xinput_state.Gamepad.sThumbLY/32768.f;
					input_state.controllers[i_controller].right_thumb[0] = (float)xinput_state.Gamepad.sThumbRX/32768.f;
					input_state.controllers[i_controller].right_thumb[1] = (float)xinput_state.Gamepad.sThumbRY/32768.f;
				}
				else {
					memset(input_state.controllers + i_controller, 0, sizeof(struct ControllerState));
				}
			}

			if (input_state.mouse_down) {
				float dx = fabsf(input_state.drag_start_x - input_state.mouse_x);
				float dy = fabsf(input_state.drag_start_y - input_state.mouse_y);
				//printf("%f %f, %f %f\n",input_state.mouse_x,input_state.mouse_y,input_state.drag_start_x,input_state.drag_start_y);
				float drag_dx = fabsf((float)GetSystemMetrics(SM_CXDRAG) / (float)(r.right - r.left));
				float drag_dy = fabsf((float)GetSystemMetrics(SM_CYDRAG) / (float)(r.top - r.bottom));
				if (dx>drag_dx || dy>drag_dy) {
					input_state.mouse_state = MOUSE_DRAG;
				}
			}


			LARGE_INTEGER current_tick, delta_ticks;
			QueryPerformanceCounter(&current_tick);
			delta_ticks.QuadPart = current_tick.QuadPart - last_tick.QuadPart;
			last_tick = current_tick;
			delta_ticks.QuadPart *= TICKS_PER_SECOND;
			delta_ticks.QuadPart /= counter_frequency.QuadPart;
			wait_for_event = update((int)delta_ticks.QuadPart, input_state, game_data);
			printf("Update!\n");
			render(framebuffer_w, framebuffer_h, game_data);
			glFlush();
			SwapBuffers(hDC);
			input_state.prev_mouse_state = input_state.mouse_state;
			if (input_state.mouse_state == MOUSE_CLICKED) {
				input_state.mouse_state = MOUSE_NOTHING;
			}
			input_state.num_keys_typed = 0;
			input_state.scroll_delta_y = 0.f;
		}
	}

	end_game(game_data);

	CloseHandle(timer);

	wglMakeCurrent(NULL, NULL);
	ReleaseDC(hWnd, hDC);
	wglDeleteContext(hRC);
	DestroyWindow(hWnd);
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
