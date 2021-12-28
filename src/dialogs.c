#include <dialogs.h>
#include <vliv.h>

#include <commctrl.h>
#include <commdlg.h>

#include <resources/resource.h>

static char name[100];
static char key[100];
static BOOL isRegistered = FALSE;
static TCHAR sz[MAX_PATH];
static HWND progress;

BOOL StaticToURLControl(HWND hDlg, UINT staticid, TCHAR *szURL, COLORREF crLink);

BOOL CALLBACK DialogProcAbout (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
     switch (message) {
     case WM_INITDIALOG :
	 ShowWindow (hDlg, SW_HIDE);
	 if (GetDlgItem(hDlg, IDC_HYPERLINK)) 
	     StaticToURLControl(hDlg, IDC_HYPERLINK, "http://vlivviewer.free.fr/vliv.htm", -1);
	 if (isRegistered == TRUE && GetDlgItem(hDlg, 3)) {
	     char buffer[100];
	     LoadString(languageInst, IDS_REGISTERED_TO, sz, sizeof(sz));
	     wsprintf(buffer, sz, name);
	     SendDlgItemMessage(hDlg, 3, WM_SETTEXT, 0, (LPARAM)buffer);
	 }
	 // does not center in window, but in screen
	 CenterDialog (hDlg, GetWindow(hDlg, GW_OWNER));
	 ShowWindow (hDlg, SW_SHOW);
	 return TRUE ;
     case WM_COMMAND :
	 switch (LOWORD (wParam)) {
	 case IDOK :
	 case IDCANCEL :
	     EndDialog (hDlg, 0) ;
	     return TRUE ;
	 }
	 break ;
     }
     return FALSE ;
}

const char* credits = 
"libtiff 4.3.0\n"
"   Copyright (c) 1988-1997 Sam Leffler\n"
"   Copyright (c) 1991-1997 Silicon Graphics, Inc.\n\n"
"libpng 1.6.37\n"
"   Copyright (c) 1995-2019 The PNG Reference Library Authors.\n"
"   Copyright (c) 2018-2019 Cosmin Truta.\n"
"   Copyright (c) 2000-2002, 2004, 2006-2018 Glenn Randers-Pehrson.\n"
"   Copyright (c) 1996-1997 Andreas Dilger.\n"
"   Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.\n\n"
"zlib 1.2.11\n"
"   Copyright (c)  1995-2017 Jean-loup Gailly and Mark Adler\n\n"
"libjpeg 6b\n"
"   Copyright (c) 1994-1998, Thomas G. Lane.\n\n"
"libjpeg-turbo 2.1.2\n"
"   Copyright (c)2009-2021 D. R. Commander.\n"
"   Copyright (c)2015 Viktor Szathmáry.\n\n"
"Ownd\n"
"   Copyright (c) 1998 Russell Freeman."
;


BOOL CALLBACK DialogProcCredits (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
     switch (message) {
     case WM_INITDIALOG :
	 ShowWindow (hDlg, SW_HIDE);
	 SendDlgItemMessage(hDlg, IDC_STATIC, WM_SETTEXT, 0, (LPARAM)credits);
	 CenterDialog (hDlg, GetWindow(hDlg, GW_OWNER));
	 ShowWindow (hDlg, SW_SHOW);
	 return TRUE ;
     case WM_COMMAND :
	 switch (LOWORD (wParam)) {
	 case IDOK :
	 case IDCANCEL :
	     EndDialog (hDlg, 0) ;
	     return TRUE ;
	 }
	 break ;
     }
     return FALSE ;
}

const char* VlivRegLicenseInfo = "Software\\Delhoume\\Vliv\\License";

const char* hextable = "0123456789ABCDEF";

_inline BOOL isKeyValid(const char* tryname, const char* key) {
        char* startkey = "pitchoune";
        unsigned int namelength = lstrlen(tryname);
        unsigned int keylength = lstrlen(key);
        unsigned int ii = 0;
        unsigned int idx;
        if (keylength != 18) return FALSE;
        for (idx = 0; idx < 9; ++idx, ++ii) {
            unsigned int value = startkey[idx] ^ tryname[ii];
            unsigned int hex1 = value / 16;
            unsigned int hex2 = value % 16;
             if (hextable[hex1] != key[idx * 2     ])
                 return FALSE;
             if (hextable[hex2] != key[idx * 2 + 1])
                 return FALSE;
             if (ii > namelength)
               ii = 0;
        }
        return TRUE;
}

BOOL checkLicense() {
     HKEY hKey;
     if (RegOpenKeyEx(HKEY_CURRENT_USER,
		      VlivRegLicenseInfo,
		      0,
		      KEY_QUERY_VALUE,
		      &hKey) == ERROR_SUCCESS) {
	 DWORD buflen = MAX_PATH;
	 DWORD buflen2 = MAX_PATH;
	 if ((RegQueryValueEx(hKey, TEXT("Name"), 0, 0, (LPBYTE)name, &buflen) == ERROR_SUCCESS) &&
	     (RegQueryValueEx(hKey, TEXT("Key"), 0, 0, (LPBYTE)key, &buflen2) == ERROR_SUCCESS)) {
	     RegCloseKey(hKey);
	     return isKeyValid(name, key);
	 }
	 RegCloseKey(hKey);
     }
     return FALSE;
}

const char* paypalurl = "https://www.paypal.com/cgi-bin/webscr?cmd=_xclick&business=delhoume%40yahoo%2ecom&item_name=Vliv%20Individual%20License&amount=10%2e00&no_shipping=2&no_note=1&currency_code=USD&lc=FR&bn=PP%2dBuyNowBF&charset=UTF%2d8";

BOOL CALLBACK RegisterProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
     switch (message) {
     case WM_INITDIALOG :
          ShowWindow (hDlg, SW_HIDE);
          CenterDialog (hDlg, GetWindow (hDlg, GW_OWNER));
          ShowWindow (hDlg, SW_SHOW);
          return TRUE ;
     case WM_COMMAND :
          switch (LOWORD (wParam)) {
          case IDOK : {
	    int n;
	    char* dname = 0;
	    char* dkey = 0;
	    n = SendDlgItemMessage(hDlg, IDC_NAME, WM_GETTEXTLENGTH, 0, 0);
	    if (n > 0) {
	      dname = (char*)MYALLOC(n + 1);
	      SendDlgItemMessage(hDlg, IDC_NAME, WM_GETTEXT, n+1, (LPARAM)dname);
	    }
	    n = SendDlgItemMessage(hDlg, IDC_KEY, WM_GETTEXTLENGTH, 0, 0);
	    if (n > 0) {
	      dkey = (char*)MYALLOC(n + 1);
	      SendDlgItemMessage(hDlg, IDC_KEY, WM_GETTEXT, n+1, (LPARAM)dkey);
	    }
	    if (isKeyValid(dname, dkey)) {
		HKEY hKey;
		DWORD disposition;
		if (RegCreateKeyEx(HKEY_CURRENT_USER,
				   VlivRegLicenseInfo,
				   0,
				   0,
				   REG_OPTION_NON_VOLATILE,
				   KEY_ALL_ACCESS,
				   0,
				   &hKey,
				   &disposition) == ERROR_SUCCESS) {
		    RegSetValueEx(hKey, "Name", 0, REG_SZ, dname, lstrlen(dname) + 1);
		    RegSetValueEx(hKey, "Key", 0, REG_SZ, dkey, lstrlen(dkey) + 1);
		    RegCloseKey(hKey);
		    isRegistered = TRUE;
		    lstrcpy(name, dname);
		    lstrcpy(key, dkey);
		    //		    UpdateToolbarAndMenus();
		}
	    }
	    if (dname != 0)
	      MYFREE(dname);
	    if (dkey != 0)
	      MYFREE(dkey);
	    EndDialog (hDlg, 0) ;
	    break;
	  }
          case IDCANCEL :
	    EndDialog (hDlg, 0) ;
	    break;
	  case IDC_PAYPAL:
	      ShellExecute(hDlg, "open", paypalurl, NULL, NULL, SW_SHOWNORMAL);
	      break;
          }
          break ;
     }
     return FALSE ;
}

BOOL CenterDialog (HWND hwndChild, HWND hwndParent) {
   RECT    rChild, rParent, rWorkArea;
   int     wChild, hChild, wParent, hParent;
   int     xNew, yNew;

   if (hwndParent == 0) 
       MessageBox(0, "", "", MB_OK);
   // Get the Height and Width of the child window
   GetWindowRect (hwndChild, &rChild);
   wChild = rChild.right - rChild.left;
   hChild = rChild.bottom - rChild.top;
   // Get the Height and Width of the parent window
   GetWindowRect (hwndParent, &rParent);
   wParent = rParent.right - rParent.left;
   hParent = rParent.bottom - rParent.top;
   // Get the limits of the 'workarea'
   if (!SystemParametersInfo(SPI_GETWORKAREA, sizeof(RECT), &rWorkArea, 0)) {
      rWorkArea.left = rWorkArea.top = 0;
      rWorkArea.right = GetSystemMetrics(SM_CXSCREEN);
      rWorkArea.bottom = GetSystemMetrics(SM_CYSCREEN);
   }
   // Calculate new X position
   xNew = rParent.left + ((wParent - wChild) /2);
   // Calculate new Y position
   yNew = rParent.top  + ((hParent - hChild) /2);
   return SetWindowPos (hwndChild, 0, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

BOOL CALLBACK AbortDialogProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam) { 
    switch (message) { 
    case WM_INITDIALOG: 

	progress = CreateWindowEx(0, 
				  PROGRESS_CLASS, 
				  0, 
				  WS_CHILD | WS_VISIBLE, 
				  10, 60, 260, 20, 
				  hdlg, 
				  0, 0, 0);    
	SendMessage(progress, PBM_SETSTEP, (WPARAM) 1, 0); 	    
	CenterDialog (hdlg, GetWindow (hdlg, GW_OWNER));
	ShowWindow (hdlg, SW_SHOW);
	return TRUE; 
    case WM_COMMAND:    
	bPrint = FALSE; 
	return TRUE; 
    default: 
	return FALSE; 
    } 
} 

BOOL CALLBACK AbortProc(HDC hdc, int nCode) { 
    MSG msg; 
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) { 
        if (!IsDialogMessage(abortDialog, &msg)) { 
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
        } 
    } 
    return bPrint; 
} 

HWND GetProgress() {
    return progress;
}

