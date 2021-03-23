#ifndef _VLIV_WINDOW_H
#define _VLIV_WINDOW_H

#include <windows.h>
#include <vliv.h>

VLIV_EXPORT(void) SwitchFullscreen(HWND window, HWND subwindow);
VLIV_EXPORT(void) SaveWindowPlacement(HWND hwnd);

typedef struct {
  int x;
  int y;
  int cx;
  int cy;
  int max;
} WININFO;

VLIV_EXPORT(void) LoadWindowPlacement(WININFO* winfo);

#endif
