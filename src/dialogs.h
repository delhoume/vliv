#ifndef _VLIV_DIALOGS_H
#define _VLIV_DIALOGS_H

#include <vliv.h>

BOOL CALLBACK DialogProcAbout (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DialogProcCredits (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK RegisterProc (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AbortDialogProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AbortProc(HDC hdc, int nCode);
HWND GetProgress();

void CleanupURLControl();
BOOL checkLicense();
BOOL CenterDialog (HWND hwndChild, HWND hwndParent);

HWND abortDialog;
BOOL bPrint;

#endif
