#include<windows.h>
#include<tchar.h>

#define global static

#define up    8
#define down  5
#define left  4
#define right 6

#define right_click 9
#define left_click  7

#define APP_EXIT 101
#define APP_STARTUP 102
#define WM_TRAYSHOW WM_USER + 1


global bool startup_state;
global int speed = 10;

void showtrayicon(HWND window)
{
	NOTIFYICONDATA nid = {};
	nid.cbSize = sizeof(nid);
	nid.hWnd = window;
	nid.uID = 101;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_INFO | NIF_TIP;
	nid.hIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(1));
	nid.uCallbackMessage = WM_TRAYSHOW;

	_tcscpy(nid.szTip, _T("mkeys | nlife"));
	_tcscpy(nid.szInfo, _T("mkeys - mouse keys alt"));

	nid.dwInfoFlags = NIIF_INFO;
	Shell_NotifyIcon(NIM_ADD, &nid);
}

void hidetrayicon(HWND window)
{
	NOTIFYICONDATA nid = {};
	nid.cbSize = sizeof(nid);
	nid.hWnd = window;
	nid.uID = 101;
	Shell_NotifyIcon(NIM_DELETE, &nid);
}

void showpopupontray(HWND window)
{
	HMENU menu;
	POINT cursor;

	GetCursorPos(&cursor);
	menu = CreatePopupMenu();

	DWORD is_set = MF_CHECKED;
	if(startup_state) is_set = MF_CHECKED;
	else is_set = MF_UNCHECKED;

	AppendMenu(menu, MF_STRING | is_set, APP_STARTUP, "&Auto Start");
	AppendMenu(menu, MF_SEPARATOR, 0, 0);
	AppendMenu(menu, MF_STRING, APP_EXIT, "&Exit");

	SetForegroundWindow(window);
	TrackPopupMenu(menu, TPM_LEFTALIGN, cursor.x, cursor.y, 0, window, 0);
	DestroyMenu(menu);
}

void rightclick(int x, int y)
{
	INPUT input = {};
	input.type = INPUT_MOUSE;

	input.mi.dx = x;
	input.mi.dy = y;

	input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP;
	SendInput(1, &input, sizeof(input));
}

void leftclick(int x, int y)
{
	INPUT input = {};
	input.type = INPUT_MOUSE;

	input.mi.dx = x;
	input.mi.dy = y;

	input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
	SendInput(1, &input, sizeof(input));
}

void movemouse(int dir)
{

	POINT pt;
	GetCursorPos(&pt);
	int x , y;
	x = y = 0;

	switch(dir)
	{
		case right_click: rightclick(x,y); break;
		case left_click: leftclick(x,y); break;
		default:
		{
			if(dir == up)
			{
				y = -speed;
			}
			else if(dir == down)
			{
				y  = speed;
			}
			else if(dir == left)
			{
				x = -speed;
			}
			else if(dir == right)
			{
				x = speed;
			}

			INPUT input ={};
			input.type = INPUT_MOUSE;
			input.mi.dx = x;
			input.mi.dy = y;
			input.mi.dwFlags = MOUSEEVENTF_MOVE;
			SendInput(1, &input, sizeof(input));
		}
	}
}

void get_exe_directory(char *buffer, int size, int flag=0)
{
	HMODULE app = GetModuleHandle(0);
	GetModuleFileName(app, buffer, size);

	if(flag==0)
	{
		char *rm_slash = strrchr(buffer, '\\');
		*rm_slash = 0;
	}
}

void enablestartup()
{
	char exe[512];
	get_exe_directory(exe, sizeof(exe), 1);
	HKEY key = 0;

	RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &key);
	RegSetValueEx(key, "mkeys_nlife", 0, REG_SZ, (BYTE*)exe, 2*(strlen(exe)+1));
	RegCloseKey(key);
}

void disablestartup()
{
	HKEY key = 0;
	RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &key);
	RegDeleteValue(key, "mkeys_nlife");
	RegCloseKey(key);
}

bool startupstatus()
{
	HKEY key = 0;
	RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &key);

	DWORD size = 513*2;

	char exe[512];
	char actual_path[512];
	LONG err = RegGetValue(key, 0, "mkeys_nlife", RRF_RT_ANY, 0, exe, &size);

	get_exe_directory(actual_path, sizeof(actual_path), 1);

	bool same_path = (strcmp(actual_path, exe) == 0);

	if(err != ERROR_SUCCESS || (!same_path))
	{
		return false;
	}
	return true;
}

LRESULT CALLBACK
windowprocedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;
	switch(message)
	{
		case WM_CREATE:
		{
			showtrayicon(window);
		} break;

		case WM_TRAYSHOW:
		{
			switch(lparam)
			{
				case WM_RBUTTONUP:
			    {
					showpopupontray(window);
				}
			}
		} break;

		case WM_COMMAND:
		{
			switch(LOWORD(wparam))
			{
				case APP_EXIT:
				{
					hidetrayicon(window);
					PostQuitMessage(0);
				} break;

				case APP_STARTUP:
				{
					if(!startup_state)
					{
						enablestartup();
					}
					else
					{
						disablestartup();
					}

					startup_state = !startup_state;
				} break;
			}

		} break;

		case WM_HOTKEY:
		{
			movemouse(wparam);
		} break;

		default:
		{
			result = DefWindowProc(window, message, wparam, lparam);
		} break;
	}
	return result;
}

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE previnstance,
		LPSTR cmdline, int cmdshow)
{
	startup_state = startupstatus();
	WNDCLASSEX window_class = {};

	window_class.cbSize = sizeof(window_class);
	window_class.lpfnWndProc = windowprocedure;
	window_class.hInstance = instance;
	window_class.hCursor = LoadCursor(0, IDC_ARROW);
	window_class.lpszClassName = "nlife_mkey";

	RegisterClassEx(&window_class);

	HWND window = CreateWindowEx(0, window_class.lpszClassName,
								 "nlife_mkey", WS_POPUP,
								 CW_USEDEFAULT, CW_USEDEFAULT,
								 CW_USEDEFAULT, CW_USEDEFAULT,
								 0, 0, instance, 0);

	RegisterHotKey(window, 4, 0, VK_NUMPAD4);
	RegisterHotKey(window, 5, 0, VK_NUMPAD5);
	RegisterHotKey(window, 6, 0, VK_NUMPAD6);

	RegisterHotKey(window, 8, 0, VK_NUMPAD8);

	RegisterHotKey(window, 9, 0, VK_NUMPAD9);
	RegisterHotKey(window, 7, 0, VK_NUMPAD7);

	MSG message;
	while(GetMessage(&message, 0, 0, 0) > 0)
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	return 0;
}
