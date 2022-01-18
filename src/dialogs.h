#ifndef _VLIV_DIALOGS_H
#define _VLIV_DIALOGS_H

#include <vliv.h>

INT_PTR DialogProcAbout(HWND, UINT, WPARAM, LPARAM);
INT_PTR DialogProcCredits(HWND, UINT, WPARAM, LPARAM);
INT_PTR RegisterProc (HWND, UINT, WPARAM, LPARAM);
INT_PTR AbortDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AbortProc(HDC hdc, int nCode);
HWND GetProgress();

void CleanupURLControl();
BOOL checkLicense();
BOOL CenterDialog (HWND hwndChild, HWND hwndParent);

HWND abortDialog;
BOOL bPrint;

#endif
