#include <window.h>

static BOOL fullscreen = FALSE;
static const TCHAR* VlivRegKeyWindow = "Software\\Delhoume\\Vliv\\Window";

void SwitchFullscreen(HWND window, HWND subwindow) {
    WINDOWPLACEMENT wplacement;
    wplacement.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(subwindow, &wplacement );
    if (fullscreen == FALSE) {
	ShowWindow(window, SW_HIDE);
	SetWindowLong(subwindow, GWL_STYLE,  WS_CLIPCHILDREN | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL);
	SetParent(subwindow, 0);
	wplacement.showCmd = SW_SHOWMAXIMIZED;
	SetWindowPlacement(subwindow, &wplacement);
	SetWindowPos(subwindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED); 
	fullscreen = TRUE;
    } else {
	SetParent(subwindow, window);
	SetWindowLong(subwindow, GWL_STYLE, WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL );
	wplacement.showCmd = SW_SHOWNORMAL; 
 	SetWindowPlacement(subwindow, &wplacement ); 
 	SetWindowPos(subwindow, 0, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
	ShowWindow(window, SW_SHOW);
	fullscreen = FALSE;
    }
}

static void RegSetInt(HKEY hKey, LPCSTR lpName, int nValue) {
    RegSetValueEx(hKey, lpName, 0, REG_DWORD, (LPBYTE)&nValue, sizeof(int));
}

static int RegGetInt(HKEY hKey, LPCSTR lpName, int nDefault) {
  LONG  nRet;
  DWORD dwValue;
  DWORD cbData = sizeof(DWORD);
  DWORD dwType = REG_DWORD;

  nRet = RegQueryValueEx(hKey, lpName, NULL, &dwType, (LPBYTE)&dwValue, &cbData);
  if (nRet == ERROR_SUCCESS)
    return(int)dwValue;
  else
    return(nDefault);
}

void SaveWindowPlacement(HWND hwnd) { 
    DWORD dwDisp;
    HKEY hKey;
    WINDOWPLACEMENT wndpl;
    WININFO wi;
    wndpl.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &wndpl);
    wi.x = wndpl.rcNormalPosition.left;
    wi.y = wndpl.rcNormalPosition.top;
    wi.cx = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
    wi.cy = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;
    wi.max = (IsZoomed(hwnd) || (wndpl.flags & WPF_RESTORETOMAXIMIZED));
    RegCreateKeyEx(HKEY_CURRENT_USER, VlivRegKeyWindow, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp);
    RegSetInt(hKey, "PosX", wi.x);
    RegSetInt(hKey, "PosY", wi.y);
    RegSetInt(hKey, "SizeX", wi.cx);
    RegSetInt(hKey, "SizeY", wi.cy);
    RegSetInt(hKey, "Maximized",wi.max);
    RegCloseKey(hKey);
}

void LoadWindowPlacement(WININFO* wi) {
    DWORD dwDisp;
    HKEY hKey;
    RegCreateKeyEx(HKEY_CURRENT_USER, VlivRegKeyWindow, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp);
    wi->x = RegGetInt(hKey, "PosX", CW_USEDEFAULT);
    wi->y = RegGetInt(hKey, "PosY", CW_USEDEFAULT);
    wi->cx = RegGetInt(hKey, "SizeX", CW_USEDEFAULT);
    wi->cy = RegGetInt(hKey, "SizeY", CW_USEDEFAULT);
    wi->max = RegGetInt(hKey, "Maximized", 0);
    if (wi->max) 
	wi->max = 1;
  RegCloseKey(hKey);
}

