#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif 
#include <Windows.h>

enum class WindowFixState {
	Waiting,
	Targeting,
};

const TCHAR* SINGLE_INSTANCE_EVENT = TEXT("WindowFix.0");
const TCHAR* WINDOW_CLASS = TEXT("WindowFix");
const TCHAR* WINDOW_TITLE = TEXT("WindowFix");

const DWORD WINDOW_FIX_KEY = VK_SPACE;

const COLORREF COLOR_WINDOW_INACTIVE = 0x0000FF00;
const COLORREF COLOR_WINDOW_ACTIVE = 0x000000FF;

const int TARGET_WINDOW_X = 10;
const int TARGET_WINDOW_Y = 10;
const int TARGET_WINDOW_WIDTH = 250;
const int TARGET_WINDOW_HEIGHT = 200;

HWND hwnd = NULL;
HBRUSH hbrWindowInactive = NULL;
HBRUSH hbrWindowActive = NULL;
HHOOK hook = NULL;
WindowFixState state = WindowFixState::Waiting;

BOOL CheckOneInstance() {
	HANDLE m_hStartEvent = CreateEvent(NULL, TRUE, FALSE, SINGLE_INSTANCE_EVENT);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		if (m_hStartEvent != NULL) {
			CloseHandle(m_hStartEvent);
		}
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT ps;
	HDC hdc;
	switch (msg) {
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		if (hook) {
			UnhookWindowsHookEx(hook);
		}
		DeleteObject(hbrWindowInactive);
		DeleteObject(hbrWindowActive);
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		FillRect(hdc, &ps.rcPaint, state == WindowFixState::Targeting ? hbrWindowActive : hbrWindowInactive);
		EndPaint(hwnd, &ps);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK LowLevelKeyboardProc(_In_ int code, _In_ WPARAM wParam, _In_ LPARAM lParam) {
	KBDLLHOOKSTRUCT* eventData = (KBDLLHOOKSTRUCT*) lParam;
	BOOL stateChanged = FALSE;
	HWND activeWindow = NULL;
	switch (wParam) {
	case WM_KEYDOWN:
   		if (eventData->vkCode == WINDOW_FIX_KEY) {
			activeWindow = GetForegroundWindow();
			if (state == WindowFixState::Waiting) {
    			if (activeWindow != NULL && activeWindow == hwnd) {
					state = WindowFixState::Targeting;
					stateChanged = TRUE;
				}
			} else if (state == WindowFixState::Targeting) {
				if (activeWindow != NULL && activeWindow != hwnd) {
					MoveWindow(activeWindow, TARGET_WINDOW_X, TARGET_WINDOW_Y, TARGET_WINDOW_WIDTH, TARGET_WINDOW_HEIGHT, TRUE);
					state = WindowFixState::Waiting;
					stateChanged = TRUE;
				}
			}
		}
		break;
	case WM_KEYUP:
		break;
	case WM_SYSKEYDOWN:
		break;
	case WM_SYSKEYUP:
		break;
	default:
		break;
	}
	if (stateChanged) {
		InvalidateRect(hwnd, NULL, FALSE);
		UpdateWindow(hwnd);
		return 1;
	}
	return CallNextHookEx(NULL, code, wParam, lParam);
}


int _stdcall WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {

	// Single instance
	if (!CheckOneInstance()) {
		return 0;
	}

	// Brushes
	hbrWindowInactive = CreateSolidBrush(COLOR_WINDOW_INACTIVE);
	hbrWindowActive = CreateSolidBrush(COLOR_WINDOW_ACTIVE);

	// Register the window class.
	WNDCLASS wc = { };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = WINDOW_CLASS;
	RegisterClass(&wc);

	// Create the window.
	hwnd = CreateWindowEx(
		0,                              // Optional window styles.
		WINDOW_CLASS,                   // Window class
		WINDOW_TITLE,                   // Window text
		WS_OVERLAPPEDWINDOW,            // Window style
		100, 100, 250, 200,             // Size and position
		NULL,                           // Parent window    
		NULL,                           // Menu
		hInstance,                      // Instance handle
		NULL                            // Additional application data
	);
	if (hwnd == NULL) {
		return -111;
	}
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	// Set keyboard hook
	hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
	if (hook == NULL) {
		return -222;
	}

	// Handle message queue
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;

}