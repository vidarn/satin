#include "engine.h"
#include "opengl/opengl.h"
#include "os/os.h"
#include "window/window.h"
#include <Windows.h>
#include <Xinput.h>
#include <stdio.h>
#include <math.h>
float controller_left_thumb_deadzone = (float)XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32768.f;
float controller_right_thumb_deadzone = (float)XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32768.f;

struct WindowData {
	struct OSData* os_data;
	struct GraphicsData* graphics_data;
	struct WindowProcParams* window_proc_params;
	struct InputState *input_state;
	struct OpenGLFuncPointers *opengl_func_ptrs;
	HWND hWnd;
	HWND parent_hWnd;
	HICON icon;
	HANDLE timer;
	HGLRC hRC;
	HDC hDC;
    LARGE_INTEGER counter_frequency;
	LARGE_INTEGER timer_interval;
	LARGE_INTEGER last_tick;
	long int drag_start_x;
	long int drag_start_y;
	float min_x, max_x, min_y, max_y;
	int framebuffer_w;
	int framebuffer_h;
	int wait_for_event;
	int cursor_visible;
};

struct WindowProcParams {
	volatile struct InputState *input_state;
	int framebuffer_w;
	int framebuffer_h;
	int should_quit;
	int window_active;
};


static LONG WINAPI
WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	struct WindowProcParams *params = (struct WindowProcParams*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch (uMsg) {
	case WM_SIZE:
		if (params) {
			params->framebuffer_w = LOWORD(lParam);
			params->framebuffer_h = HIWORD(lParam);
			PostMessage(hWnd, WM_PAINT, 0, 0);
			return 0;
		}
		break;

	case WM_CHAR:
		switch (wParam) {
		case 27:			/* ESC key */
			break;
		}
		return 0;
		case WM_SETFOCUS:
		{
			params->window_active = 1;
			break ;
		}
		case WM_KILLFOCUS:
		{
			params->window_active = 0;
			break ;
		}
		case WM_ACTIVATE:
		{
			if (LOWORD(wParam) == WA_INACTIVE)
				params->window_active = 0;
			else // WA_ACTIVE or WA_CLICKACTIVE
				params->window_active = 1;
			break;
		}
		case WM_CAPTURECHANGED:
		{
			params->window_active = 0;
			break;
		}

	case WM_CLOSE:
		if (params) {
			params->should_quit = 1;
			return 0;
		}break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static HWND
CreateOpenGLWindow(const char* title, int x, int y, struct WindowProcParams *window_proc_params,
	BYTE type, DWORD flags, HWND parent_hWnd, HICON icon)
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
		wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = (WNDPROC)WindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		if (icon) {
			wc.hIcon = icon;
		}
		else {
			wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
		}
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = "Satin";

		if (!RegisterClassA(&wc)) {
			//NOTE(Vidar):The class is probably already registered, carry on!
			/*
			MessageBoxA(NULL, "RegisterClass() failed:  "
				"Cannot register window class.", "Error", MB_OK);
				*/
			//return NULL;
		}
	}

	DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	hWnd = CreateWindowA("Satin", title, style,
		x, y, window_proc_params->framebuffer_w, window_proc_params->framebuffer_h, parent_hWnd, NULL, hInstance, NULL);

	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)window_proc_params);

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

    int dpi = GetDpiForWindow(hWnd);
    int w = (window_proc_params->framebuffer_w * dpi)/96;
    int h = (window_proc_params->framebuffer_h * dpi)/96;
    SetWindowPos(hWnd, 0, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);

	return hWnd;
}

uint64_t window_get_current_tick(struct WindowData *window_data)
{
    LARGE_INTEGER current_tick;
    QueryPerformanceCounter(&current_tick);
    current_tick.QuadPart*=TICKS_PER_SECOND;
    current_tick.QuadPart/=window_data->counter_frequency.QuadPart;
    return (int)current_tick.QuadPart;
}

void launch_game(const char* window_title, int _framebuffer_w, int _framebuffer_h, int show_console,
	int num_game_states, void* param, struct GameState* game_states, int debug_mode, struct OSData* os_data)
{
	struct GameData* game_data = window_start_game((struct WindowStartGameParams){ window_title, _framebuffer_w, _framebuffer_h, show_console, num_game_states, param, game_states, debug_mode, os_data });
	int should_quit = 0;
	while (!should_quit) {
		should_quit = window_step_game(game_data);
	}
	window_end_game(game_data);
}

struct GameData *window_start_game(struct WindowStartGameParams params)
{
	if (params.game_data) {
        struct WindowData* window_data = get_window_data(params.game_data);
        opengl_activate(window_data->opengl_func_ptrs);
		reinit_game_states(params.num_game_states, params.game_states, params.game_data);
		return params.game_data;
	}
	struct InputState* input_state = calloc(1, sizeof(struct InputState));
	struct WindowProcParams* window_proc_params = calloc(1, sizeof(struct WindowProcParams));
	window_proc_params->input_state = input_state;
	window_proc_params->framebuffer_w = params._framebuffer_w;
	window_proc_params->framebuffer_h = params._framebuffer_h;
	window_proc_params->window_active = 1;
	OutputDebugStringA("Launching game!\n");

	if (params.show_console) {
		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
	}

	HDC hDC;				/* device context */
	HGLRC hRC_tmp;				/* opengl context */
	HGLRC hRC;				/* opengl context */
	HWND  hWnd;				/* window */

	hWnd = CreateOpenGLWindow(params.window_title, 0, 0, window_proc_params, PFD_TYPE_RGBA, 0,
		0, 0
	);
	if (hWnd == NULL)
		exit(1);

	hDC = GetDC(hWnd);
	hRC_tmp = wglCreateContext(hDC);
	wglMakeCurrent(hDC, hRC_tmp);

	struct OpenGLFuncPointers *opengl_func_ptrs = opengl_load();

	int attribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONTEXT_FLAGS_ARB, 0,
		0
	};
	if (params.debug_mode) {
		attribs[2] |= WGL_CONTEXT_DEBUG_BIT_ARB;
	}
	hRC = wglCreateContextAttribsARB(hDC, 0, attribs);
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hRC_tmp);
	wglMakeCurrent(hDC, hRC);

	if (params.debug_mode) {
		glEnable(GL_DEBUG_OUTPUT);
	}

	const GLubyte* GLVersionString = glGetString(GL_VERSION);
	OutputDebugStringA("GL version: ");
	OutputDebugStringA((const char*)GLVersionString);
	OutputDebugStringA("\n");

	ShowWindow(hWnd, SW_SHOW);

	struct WindowData* window_data = calloc(1, sizeof(struct WindowData));
	window_data->hDC = hDC;
	window_data->hRC = hRC;
	window_data->hWnd = hWnd;
	window_data->opengl_func_ptrs = opengl_func_ptrs;
	window_data->input_state = input_state;
	window_data->window_proc_params = window_proc_params;
	struct OSData* os_data = params.os_data;
	if (!os_data) {
		os_data = os_data_create();
	}
	window_data->os_data = os_data;
	struct GraphicsData* graphics = graphics_create(0);
	window_data->graphics_data = graphics;

	struct GameData* game_data = init(params.num_game_states, params.game_states, params.param, window_data, params.debug_mode);

	window_data->timer = CreateWaitableTimerA(NULL, TRUE, "Satin FPS timer");
	if (!window_data->timer) {
		MessageBoxA(hWnd, "Could not create multimedia timer", "Error!", MB_OK | MB_ICONERROR);
	}
	window_data->timer_interval.QuadPart = -1e7 / 60;
	SetWaitableTimer(window_data->timer, &window_data->timer_interval, 0, 0, 0, FALSE);

	QueryPerformanceCounter(&window_data->last_tick);
	QueryPerformanceFrequency(&window_data->counter_frequency);
	return game_data;
}

int window_step_game(struct GameData *game_data)
{
	MSG   msg;
    int event_recieved;
	struct WindowData* window_data = get_window_data(game_data);
	struct InputState* input_state = window_data->input_state;
    if ((event_recieved = PeekMessage(&msg, window_data->hWnd, 0, 0, PM_REMOVE)) || window_data->wait_for_event) {
        window_data->wait_for_event = 0;
        if (!event_recieved) {
            GetMessageA(&msg, window_data->hWnd, 0, 0);
        }
        switch (msg.message) {
        case WM_CHAR:
        {
            if (input_state->num_keys_typed < max_num_keys_typed) {
                input_state->keys_typed[input_state->num_keys_typed] = msg.wParam;
                input_state->num_keys_typed++;
            }
            break;
        }
        case WM_KEYDOWN:
        {
            int handled = 0;
            if (input_state->num_keys_typed < max_num_keys_typed) {
                int ok = 0;
                switch (msg.wParam)
                {
                case VK_LEFT:
                    ok = 1; break;
                case VK_RIGHT:
                    ok = 1; break;
                }
                if (ok) {
                    input_state->keys_typed[input_state->num_keys_typed] = msg.wParam;
                    input_state->num_keys_typed++;
                    handled = 1;
                }
            }
            if (!handled) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            break;
        }
        case WM_RBUTTONDOWN:
        {
            input_state->mouse_state_right = MOUSE_CLICKED;
            input_state->mouse_down_right = 1;
            POINT p;
            GetCursorPos(&p);
            ScreenToClient(window_data->hWnd, &p);
            window_data->drag_start_x = p.x;
            window_data->drag_start_y = p.y;
            break;
        }
        case WM_RBUTTONUP:
        {
            input_state->mouse_down_right = 0;
            input_state->mouse_state_right = MOUSE_NOTHING;
            break;
        }
        case WM_LBUTTONDOWN:
        {
            input_state->mouse_state = MOUSE_CLICKED;
            input_state->mouse_down = 1;
            POINT p;
            GetCursorPos(&p);
            ScreenToClient(window_data->hWnd, &p);
            window_data->drag_start_x = p.x;
            window_data->drag_start_y = p.y;
            window_data->window_proc_params->window_active = 1;
            break;
        }
        case WM_LBUTTONUP:
        {
            input_state->mouse_down = 0;
            input_state->mouse_state = MOUSE_NOTHING;
            if (input_state->prev_mouse_state == MOUSE_DRAG) {
                //ClipCursor(NULL); 
                ReleaseCapture(); 
            }
            break;
        }
        case WM_MBUTTONDOWN:
        {
            input_state->mouse_state_middle = MOUSE_CLICKED;
            input_state->mouse_down_middle = 1;
            POINT p;
            GetCursorPos(&p);
            ScreenToClient(window_data->hWnd, &p);
            window_data->drag_start_x = p.x;
            window_data->drag_start_y = p.y;
            break;
        }
        case WM_MBUTTONUP:
        {
            input_state->mouse_down_middle = 0;
            input_state->mouse_state_middle = MOUSE_NOTHING;
            break;
        }
        case WM_MOUSEWHEEL:
        {
            input_state->scroll_delta.y = (float)GET_WHEEL_DELTA_WPARAM(msg.wParam) / 120.f;
            break;
        }
        default:
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            break;
        }
    }
    else {
        WaitForSingleObject(window_data->timer, INFINITE);
        SetWaitableTimer(window_data->timer, &window_data->timer_interval, 0, 0, 0, FALSE);
        POINT p;
        GetCursorPos(&p);
        ScreenToClient(window_data->hWnd, &p);
        RECT r;
        GetClientRect(window_data->hWnd, &r);
        float p_x = p.x - (float)r.left;
        float p_y = p.y - (float)r.top;
        float drag_x = window_data->drag_start_x - r.left;
        float drag_y = window_data->drag_start_y - r.top;
        float w = r.right - r.left;
        float h = r.bottom - r.top;
        float m_x;
        float m_y;
        float d_x;
        float d_y;
        m_x = p_x;
        m_y = h - p_y;
        d_x = drag_x;
        d_y = h - drag_y;
        input_state->delta.x = m_x - input_state->mouse_p.x;
        input_state->delta.y = m_y - input_state->mouse_p.y;
        if (cursor_locked(game_data)) {
            float offset_x = d_x - m_x;
            float offset_y = d_y - m_y;
            m_x += offset_x;
            m_y += offset_y;
            d_x += offset_x;
            d_y += offset_y;

            POINT tmp_p = { window_data->drag_start_x, window_data->drag_start_y };
            if (window_data->cursor_visible) {
                while (ShowCursor(FALSE) >= 0) {}
            }
            window_data->cursor_visible = 0;
            ClientToScreen(window_data->hWnd, &tmp_p);
            SetCursorPos(tmp_p.x, tmp_p.y);
        }
        else {
            if (!window_data->cursor_visible) {
                while (ShowCursor(TRUE) < 0) {}
            }
            window_data->cursor_visible = 1;
        }
        input_state->mouse_p.x = m_x;
        input_state->mouse_p.y = m_y;
        input_state->drag_start.x = d_x;
        input_state->drag_start.y = d_y;

        for (int i_controller = 0; i_controller < max_num_controllers; i_controller++) {
            XINPUT_STATE xinput_state = { 0 };
            DWORD result = XInputGetState(i_controller, &xinput_state);
            if (result == ERROR_SUCCESS) {
                input_state->controllers[i_controller].valid = 1;
                input_state->controllers[i_controller].buttons_pressed = xinput_state.Gamepad.wButtons;
                input_state->controllers[i_controller].left_trigger = (float)xinput_state.Gamepad.bLeftTrigger / 255.f;
                input_state->controllers[i_controller].right_trigger = (float)xinput_state.Gamepad.bRightTrigger / 255.f;
                input_state->controllers[i_controller].left_thumb[0] = (float)xinput_state.Gamepad.sThumbLX / 32768.f;
                input_state->controllers[i_controller].left_thumb[1] = (float)xinput_state.Gamepad.sThumbLY / 32768.f;
                input_state->controllers[i_controller].right_thumb[0] = (float)xinput_state.Gamepad.sThumbRX / 32768.f;
                input_state->controllers[i_controller].right_thumb[1] = (float)xinput_state.Gamepad.sThumbRY / 32768.f;
            }
            else {
                memset(input_state->controllers + i_controller, 0, sizeof(struct ControllerState));
            }
        }

        if (!window_data->window_proc_params->window_active && input_state->mouse_down) {
            input_state->mouse_down = 0;
            input_state->mouse_state = MOUSE_NOTHING;
            if (input_state->prev_mouse_state == MOUSE_DRAG) {
                //ClipCursor(NULL);
                ReleaseCapture();
            }
        }


        if (input_state->mouse_down || input_state->mouse_down_right || input_state->mouse_down_middle)
        {
            float dx = fabsf(input_state->drag_start.x - input_state->mouse_p.x);
            float dy = fabsf(input_state->drag_start.y - input_state->mouse_p.y);
            float drag_dx = fabsf((float)GetSystemMetrics(SM_CXDRAG) / (float)(r.right - r.left));
            float drag_dy = fabsf((float)GetSystemMetrics(SM_CYDRAG) / (float)(r.top - r.bottom));
            if (dx > drag_dx || dy > drag_dy) {
                if (input_state->mouse_down) {
                    input_state->mouse_state = MOUSE_DRAG;
                }
                if (input_state->mouse_down_right) {
                    input_state->mouse_state_right = MOUSE_DRAG;
                }
                if (input_state->mouse_down_middle) {
                    input_state->mouse_state_middle = MOUSE_DRAG;
                }
            }
        }

        if (input_state->mouse_state == MOUSE_DRAG && input_state->prev_mouse_state != MOUSE_DRAG)
        {
            RECT rcClient;                 // client area rectangle 
            POINT ptClientUL;              // client upper left corner 
            POINT ptClientLR;              // client lower right corner 
            SetCapture(window_data->hWnd); 
            GetClientRect(window_data->hWnd, &rcClient); 
            ptClientUL.x = rcClient.left; 
            ptClientUL.y = rcClient.top; 
 
            // Add one to the right and bottom sides, because the 
            // coordinates retrieved by GetClientRect do not 
            // include the far left and lowermost pixels. 
 
            ptClientLR.x = rcClient.right + 1; 
            ptClientLR.y = rcClient.bottom + 1; 
            ClientToScreen(window_data->hWnd, &ptClientUL); 
            ClientToScreen(window_data->hWnd, &ptClientLR); 
 
            SetRect(&rcClient, ptClientUL.x, ptClientUL.y, 
                ptClientLR.x, ptClientLR.y); 
            //ClipCursor(&rcClient); 
        }

        float framebuffer_w = window_data->window_proc_params->framebuffer_w;
        float framebuffer_h = window_data->window_proc_params->framebuffer_h;
        float min_size =
            (float)(framebuffer_w > framebuffer_h ? framebuffer_h : framebuffer_w);
        float pad = fabsf((float)(framebuffer_w - framebuffer_h))*0.5f/min_size;
        window_data->min_x = 0.f; window_data->max_x = 1.f;
        window_data->min_y = 0.f; window_data->max_y = 1.f;
        if(framebuffer_w > framebuffer_h){
            window_data->min_x -= pad;
            window_data->max_x += pad;
        }else{
            window_data->min_y -= pad;
            window_data->max_y += pad;
        }
        window_data->framebuffer_w = framebuffer_w;
        window_data->framebuffer_h = framebuffer_h;

        LARGE_INTEGER current_tick, delta_ticks;
        QueryPerformanceCounter(&current_tick);
        delta_ticks.QuadPart = current_tick.QuadPart - window_data->last_tick.QuadPart;
        window_data->last_tick = current_tick;
        delta_ticks.QuadPart *= TICKS_PER_SECOND;
        delta_ticks.QuadPart /= window_data->counter_frequency.QuadPart;
        window_data->wait_for_event = update((int)delta_ticks.QuadPart, *input_state, game_data);
        render(framebuffer_w, framebuffer_h, game_data);
        glFlush();
        SwapBuffers(window_data->hDC);
        input_state->prev_mouse_state = input_state->mouse_state;
        if (input_state->mouse_state == MOUSE_CLICKED) {
            input_state->mouse_state = MOUSE_NOTHING;
        }
        input_state->prev_mouse_state_right = input_state->mouse_state_right;
        if (input_state->mouse_state_right == MOUSE_CLICKED) {
            input_state->mouse_state_right = MOUSE_NOTHING;
        }
        input_state->prev_mouse_state_middle = input_state->mouse_state_middle;
        if (input_state->mouse_state_middle == MOUSE_CLICKED) {
            input_state->mouse_state_middle = MOUSE_NOTHING;
        }
        input_state->num_keys_typed = 0;
        input_state->scroll_delta.y = 0.f;
    }
	return window_data->window_proc_params->should_quit;
}

void window_end_game(struct GameData *game_data)
{
	end_game(game_data);
	struct WindowData* window_data = get_window_data(game_data);
	CloseHandle(window_data->timer);
	wglMakeCurrent(NULL, NULL);
	ReleaseDC(window_data->hWnd, window_data->hDC);
	wglDeleteContext(window_data->hRC);
	DestroyWindow(window_data->hWnd);
}

struct OSData *window_get_os_data(struct WindowData *window)
{
	return window->os_data;
}

struct GraphicsData *window_get_graphics_data(struct WindowData *window)
{
    return window->graphics_data;
}

void window_get_extents(float *x_min, float *x_max, float *y_min, float *y_max,
    struct WindowData *window)
{
    *x_min = window->min_x;
    *x_max = window->max_x;
    *y_min = window->min_y;
    *y_max = window->max_y;
}

void window_get_res(float* w, float* h, struct WindowData* window)
{
	*w = (float)window->framebuffer_w;
	*h = (float)window->framebuffer_h;
}

int window_is_key_down(int key)
{
	return 0;
}

int window_get_dpi(struct WindowData *window)
{
    return GetDpiForWindow(window->hWnd);
}
