#ifndef _VLIV_RECENT_H
#define _VLIV_RECENT_H

#include <windows.h>
#include <vliv.h>

VLIV_EXPORT(void) UpdateRecentList(const TCHAR* filename);
VLIV_EXPORT(void) UpdateRecentMenu(HWND window);
VLIV_EXPORT(void) FreeRecentFiles();
VLIV_EXPORT(void) LoadRecentList();
VLIV_EXPORT(void) SaveRecentList();
VLIV_EXPORT(unsigned int) GetNumRecent();
VLIV_EXPORT(const TCHAR*) GetRecentFile(unsigned int num);

#endif
