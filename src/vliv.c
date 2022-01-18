#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <vliv.h>

#include <windowsx.h>
#include <joystickapi.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <strsafe.h>

#include <dialogs.h>
#include <handlers.h>
#include <recent.h>
#include <window.h>
#include <tilemgr.h>
#include <bitmap.h>
#include <rawinput.h>

#if !defined(WM_MOUSEWHEEL)
#  define WM_MOUSEWHEEL 0x020A
#endif /* !WM_MOUSEWHEEL */
#if !defined(WHEEL_DELTA)
#  define WHEEL_DELTA   120     /* Value for rolling one detent */
#endif /* !WHEEL_DELTA */                          
#if !defined(GET_WHEEL_DELTA_WPARAM)
#define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))
#endif

#include <resources/resource.h>
#include <buffer.h>

static UINT uOriginWindowUpdateMessage;
static UINT uOriginWindowEndMessage;

static void UpdateStatusTile(BOOL busy);
static void UpdateScrollbars(BOOL center);
static void UpdateToolbarAndMenus();
static void SetDirectory(unsigned int which);

HDC hdcbitmap;
HCURSOR oldcursor;
HCURSOR cursor;
HICON warningicon;
HICON idleicon;
HICON busyicon;
HWND mainwindow;
HWND vlivview;
HWND status;
HWND toolbar;
HINSTANCE hInst;
HMODULE owndInst;

static TCHAR sz[MAX_PATH];
static TCHAR szFile[MAX_PATH];
static TCHAR statustext[MAX_PATH];

static OPENFILENAME ofnOpen;
static OPENFILENAME ofnSaveBMP;

typedef struct {
    int x;
    int y;
    int sx;
    int sy;
    int sw;
    int sh;
    int dx;
    int dy;
} TileInfo;

static TileInfo* tileinfo = 0;
static unsigned int ntileinfo;
static unsigned int ntileinfomax;

static struct Image image;

static void InitImage() {
    image.helper.Register = RegisterFormatHandler;
    image.helper.CreateTrueColorDIBSection = VlivCreateTrueColorDIBSection;
    image.helper.CreateIndexedDIBSection = VlivCreateIndexedDIBSection;
    image.helper.CreateDefaultDIBSection = VlivCreateDefaultDIBSection;
}

static void
FreeTileInfo() {
   if (tileinfo) 
       MYFREE(tileinfo); 
}

static void 
InitTileInfo() {
    FreeTileInfo();
    ntileinfomax = 64;
    tileinfo = MYALLOC(sizeof(TileInfo) * ntileinfomax);
    ntileinfo = 0;
}

static void 
AddTileInfo(int x, int y, int sx, int sy, int sw, int sh, int dx, int dy) {
    if (ntileinfo >= ntileinfomax) {
	// realloc
	ntileinfomax *= 2;
	tileinfo = MYREALLOC(tileinfo, sizeof(TileInfo) * ntileinfomax);
    }
    tileinfo[ntileinfo].x = x;
    tileinfo[ntileinfo].y = y;
    tileinfo[ntileinfo].sx = sx;
    tileinfo[ntileinfo].sy = sy;
    tileinfo[ntileinfo].sw = sw;
    tileinfo[ntileinfo].sh = sh;
    tileinfo[ntileinfo].dx = dx;
    tileinfo[ntileinfo].dy = dy;
    ++ntileinfo;
}

void LoadTile(HDC hdc, unsigned int x, unsigned int y);

static BOOL InitJoystick(HWND hWnd) {
	if (joyGetNumDevs()) {
		DWORD result = joySetCapture(hWnd, JOYSTICKID1, 0, FALSE);
		switch (result) {
		case JOYERR_UNPLUGGED:								// The joystick is unplugged
			return FALSE;									
		case MMSYSERR_NODRIVER:								// There is no driver for a joystick
			return FALSE;								
		case JOYERR_NOCANDO:								// Unknown error
			return FALSE;									
		}
	} else {
		//MessageBox(NULL, "There are no joystick devices installed.", "Error", MB_OK | MB_ICONEXCLAMATION);
	}
	return TRUE;
}


static DWORD procnum = 1;
typedef WINUSERAPI BOOL (WINAPI * QUEUEUSERWORKITEMF)(LPTHREAD_START_ROUTINE, LPVOID, ULONG);
QUEUEUSERWORKITEMF QueueUserWorkItemF = 0;
static const TCHAR* VlivRegKeyCPU = "Software\\Delhoume\\Vliv";

static void CheckProcNumber() {
    SYSTEM_INFO sinfo;
    HKEY hKey;
    DWORD dwDisp;
    HMODULE hmod = GetModuleHandle("kernel32.dll");
    GetSystemInfo(&sinfo);
    procnum = sinfo.dwNumberOfProcessors;
    RegCreateKeyEx(HKEY_CURRENT_USER, VlivRegKeyCPU, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp);
    if (hKey) {
	DWORD dwValue;
	DWORD cbData = sizeof(DWORD);
	DWORD dwType = REG_DWORD;
	if (RegQueryValueEx(hKey, "NoMT", NULL, &dwType, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) 
	    procnum = 1;
	RegCloseKey(hKey);
    }
    if (hmod) {
	// available from Windows 2000 only
	QueueUserWorkItemF = (QUEUEUSERWORKITEMF)GetProcAddress(hmod, "QueueUserWorkItem");
    }
}

long activeThreads = 1L;
HANDLE hEvent = 0;

DWORD CALLBACK LoadTileThread(void* pv) {
    TileInfo* tinfo = (TileInfo*)pv;
#if 0
    char buffer[100];
    wsprintf(buffer, "tile %dx%d ", tinfo->x, tinfo->y);
    OutputDebugString(buffer);
#endif
    LoadTile(hdcbitmap, tinfo->x, tinfo->y);
#if 0
    wsprintf(buffer, "tile %dx%d done", tinfo->x, tinfo->y);
    OutputDebugString(buffer);
#endif
    InterlockedDecrement(&activeThreads);
    if (activeThreads == 0) {
	SetEvent(hEvent);
    }
    return 0;
}

// TODO: get time measure comparison to se if it is faster
// test on more than 1 core...
static void 
LoadTiles(HDC hdcbitmap) {
    unsigned int idx;
    if ((procnum > 1) && 
	(image.supportmt == 1) && 
	QueueUserWorkItemF) {
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	activeThreads = 1L;
	UpdateStatusTile(TRUE);
	for (idx = 0; idx < ntileinfo; ++idx) {
	    TileInfo* tinfo = &tileinfo[idx];
	    if (!IsTileLoaded(tinfo->x, tinfo->y)) {
		InterlockedIncrement(&activeThreads);
		QueueUserWorkItemF(LoadTileThread,
				   (LPVOID)tinfo, 
				   WT_EXECUTEDEFAULT); // WT_EXECUTELONGFUNCTION); 
	    }
	}
	InterlockedDecrement(&activeThreads);
	if (activeThreads != 0)
	    WaitForSingleObject(hEvent, INFINITE);
	CloseHandle(hEvent);
	UpdateStatusTile(FALSE);
    } else {
#if 0
	wsprintf(buffer, "------\ntiles %d", ntileinfo);
	OutputDebugString(buffer);
#endif
	for (idx = 0; idx < ntileinfo; ++idx) {
	    TileInfo* tinfo = &tileinfo[idx];
	    if (!IsTileLoaded(tinfo->x, tinfo->y)) {
#if 0
		char buffer[100];
		wsprintf(buffer, "tile %dx%d", tinfo->x, tinfo->y);
		OutputDebugString(buffer);
#endif
		UpdateStatusTile(TRUE);
		LoadTile(hdcbitmap, tinfo->x, tinfo->y);
		UpdateStatusTile(FALSE);
	    }
	}
    }
}

static void FreeImage() {
    FreeTileManager();
    if (image.handler && image.handler->internal && image.handler->close)
		image.handler->close(&image);
		image.handler = 0;
}

static void ClearImage() {
    FreeImage();
    UpdateToolbarAndMenus();
}

static TCHAR* SubFileTypeNames[4];

static void InitFileTypeNames() {
    int readbytes;
    readbytes = LoadString(languageInst, IDS_FULL_RESOLUTION, sz, sizeof(sz));
    sz[readbytes] = 0;
    SubFileTypeNames[0] = MYALLOC(readbytes + 1);
    CopyMemory(SubFileTypeNames[0], sz, readbytes + 1);
    readbytes = LoadString(languageInst, IDS_REDUCED_RESOLUTION, sz, sizeof(sz));
    sz[readbytes] = 0;
    SubFileTypeNames[1] = MYALLOC(readbytes + 1);
    CopyMemory(SubFileTypeNames[1], sz, readbytes + 1);
    readbytes = LoadString(languageInst, IDS_SINGLE_PAGE, sz, sizeof(sz));
    sz[readbytes] = 0;
    SubFileTypeNames[2] = MYALLOC(readbytes + 1);
    CopyMemory(SubFileTypeNames[2], sz, readbytes + 1);
    readbytes = LoadString(languageInst, IDS_VIRTUAL_RESOLUTION, sz, sizeof(sz));
    sz[readbytes] = 0;
    SubFileTypeNames[3] = MYALLOC(readbytes + 1);
    CopyMemory(SubFileTypeNames[3], sz, readbytes + 1);
}

static void FreeFileTypeNames() {
    MYFREE(SubFileTypeNames[0]);
    MYFREE(SubFileTypeNames[1]);
    MYFREE(SubFileTypeNames[2]);
    MYFREE(SubFileTypeNames[3]);
}

static void UpdateStatusBar() {
    if (image.handler) {
	TCHAR buffer[MAX_PATH];
	unsigned __int64 gigapixels = UInt32x32To64(image.width, image.height);
	double gp = gigapixels / (double)1000000000;
	if (gigapixels >= 1000000000) {
		LoadString(languageInst, IDS_IMAGE_WIDTH_GIGA, sz, sizeof(sz));
		StringCchPrintf(buffer, MAX_PATH, sz, image.width, image.height, gp);
	} else {
		LoadString(languageInst, IDS_IMAGE_WIDTH, sz, sizeof(sz));
		StringCchPrintf(buffer, MAX_PATH, sz, image.width, image.height);
	}
	SendMessage(status, SB_SETTEXT, (WPARAM)0, (LPARAM)buffer);
	if (image.istiled) {
	    LoadString(languageInst, IDS_TILES_INFO, sz, sizeof(sz));
	    StringCchPrintf(buffer, MAX_PATH, sz, image.numtilesx, image.numtilesy, image.twidth, image.theight);
	    SendMessage(status, SB_SETICON, (WPARAM)(int)4, (LPARAM)0);
	    SendMessage(status, SB_SETTIPTEXT, (WPARAM)(int)4, (LPARAM)0);
	} else {
	    LoadString(languageInst, IDS_STRIPS_INFO, sz, sizeof(sz));
#if 0
	    wsprintf(buffer, sz, 
		     image.numtilesy, (image.numtilesy == 1) ? "" : "s", 
		     image.theight, (image.theight == 1) ? "" : "s");
#else
	    StringCchPrintf(buffer, MAX_PATH, sz, 
			    image.numtilesy, (image.numtilesy == 1) ? "" : "s", 
			    image.theight, (image.theight == 1) ? "" : "s");
#endif
	    SendMessage(status, SB_SETICON, (WPARAM)(int)4, (LPARAM)warningicon);
	    LoadString(languageInst, IDS_IMAGE_NOT_TILED, sz, sizeof(sz));
	    SendMessage(status, SB_SETTIPTEXT, (WPARAM)(int)4, (LPARAM)sz);
	}
	SendMessage(status, SB_SETTEXT, (WPARAM)1, (LPARAM)buffer);
	
	if (image.numdirs > 1) {
	    int which = image.subfiletype;
	    LoadString(languageInst, IDS_PAGE, sz, sizeof(sz));
	    //	    wsprintf(buffer, sz, image.currentdir + 1, image.numdirs, SubFileTypeNames[which]);
	    StringCchPrintf(buffer, MAX_PATH, sz, image.currentdir + 1, image.numdirs, SubFileTypeNames[which]);
	} else {
	    LoadString(languageInst, IDS_SINGLE_IMAGE, sz, sizeof(sz));
            //wsprintf(buffer, sz);
	    StringCchPrintf(buffer, MAX_PATH, sz);
	}
	SendMessage(status, SB_SETTEXT, (WPARAM)2, (LPARAM)buffer);
    }
    SendMessage(status, SB_SETTEXT, (WPARAM)5, (LPARAM)statustext);
}

static void UpdateStatusTile(BOOL busy) {
    SendMessage(status, SB_SETICON, (WPARAM)3, (LPARAM)(busy ? busyicon : idleicon)); 
}

unsigned int usecenter = 1;

static void LoadTile(HDC hdc, unsigned int x, unsigned int y) {
    HBITMAP hbitmap = image.handler->loadtile(&image, hdc, x, y);
    if (hbitmap) 
	SetTileBitmap(x, y, hbitmap);
    else {
	LoadString(languageInst, IDS_CANNOT_LOADTILE, sz, sizeof(sz));
	//	wsprintf(statustext, sz, x, y);
	StringCchPrintf(statustext, MAX_PATH, sz, x, y);
	UpdateStatusBar();
    }
}

static void
OpenImage(const TCHAR* name) {
	if (!PathFileExists(name))
		return;
	else {
		FormatHandlerPtr handler = GetHandlerFromFile(name);
		if (handler) {
			ClearImage();
			image.handler = handler;
			image.supportmt = 0;
			//	strcpy(image.name, name);
			StringCchCopy(image.name, MAX_PATH, name);
			if (image.handler->open(&image, name)) {
				TCHAR buffer[MAX_PATH];
				TCHAR fname[MAX_PATH];
				LoadString(languageInst, IDS_SUCCESSFULLY_LOADED, sz, sizeof(sz));
				//	    wsprintf(statustext, sz, name);
				StringCchPrintf(statustext, MAX_PATH, sz, name);
				GetFileTitle(name, fname, MAX_PATH - 1);
				LoadString(languageInst, IDS_TITLED, sz, sizeof(sz));
				//	    wsprintf(buffer, sz, fname);
				StringCchPrintf(buffer, MAX_PATH, sz, fname);
				SetWindowText(mainwindow, buffer);
				SetDirectory(image.currentdir);
				UpdateScrollbars(FALSE);
				UpdateRecentList(name);
			} else {
				LoadString(languageInst, IDS_CANNOT_LOAD, sz, sizeof(sz));
				//	    wsprintf(statustext, sz, name);
				StringCchPrintf(statustext, MAX_PATH, sz, name);
			}
			UpdateRecentMenu(mainwindow);
			UpdateStatusBar();
		}
	}
}

static BOOL HandleMouseWheel(short zDelta) {
    if (image.handler != 0) {
		if (image.numdirs > 1) {
			if (zDelta >= 0) {
				if (image.currentdir != 0) {
					SetDirectory(image.currentdir - 1);
				}
			} else {
				if (image.currentdir != (image.numdirs - 1)) {
					SetDirectory(image.currentdir + 1);
				}		
			}
			UpdateScrollbars(TRUE);
		}
    }
    return TRUE;
}

static BOOL fullscreen = FALSE;
void
SetDirectory(unsigned int which) {
    FreeTileManager();
    if (image.handler && image.handler->setdirectory) {
	image.handler->setdirectory(&image, which);  
	image.currentdir = which;
	InitTileManager(image.numtilesx, image.numtilesy);
	UpdateToolbarAndMenus();
	UpdateStatusBar();
    }
}
static void UpdateScrollbars(BOOL center) {
    SCROLLINFO si;
    RECT rect;
    GetClientRect(vlivview, &rect);
    si.cbSize = sizeof (si);
    si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE | SIF_DISABLENOSCROLL;
    if (center == TRUE) {
	unsigned int centerx, centery;
	float posx, posy;
	GetScrollInfo(vlivview, SB_HORZ, &si);
	centerx = si.nPos + (si.nPage / 2);
	posx = centerx / (float)si.nMax;
	GetScrollInfo(vlivview, SB_VERT, &si);
	centery = si.nPos + (si.nPage / 2);
	posy = centery / (float)si.nMax;
	si.nMin = 0;
	si.nPos = (int)((image.width - 1) * posx) - (rect.right / 2);
	si.nPage = rect.right;
	si.nMax = image.width - 1;
	SetScrollInfo(vlivview, SB_HORZ, &si, TRUE);
	si.nPos = (int)((image.height - 1) * posy) - (rect.bottom / 2);
	si.nMax = image.height - 1;
	si.nPage = rect.bottom;
	SetScrollInfo(vlivview, SB_VERT, &si, TRUE);
    } else {
	si.nMin = 0;
	si.nPos = 0;
	si.nPage = rect.right;
	si.nMax = image.width - 1;
	SetScrollInfo(vlivview, SB_HORZ, &si, TRUE);
	si.nMax = image.height - 1;
	si.nPage = rect.bottom;
	SetScrollInfo(vlivview, SB_VERT, &si, TRUE);
    }
    InvalidateRect(vlivview, 0, TRUE);
}

static int parts[] = { 250, 440, 640, 665, 690, -1 };
static HIMAGELIST imagelist;
static HIMAGELIST imagelistd;
static HIMAGELIST imagelisth;

LRESULT CALLBACK TopWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    RECT rect;
    switch(msg) {
    case WM_CREATE: {
	TBADDBITMAP tbbitmap = { hInst, IDB_TOOLBAR };
	TBBUTTON tbButtons[] = {
	    {0, ID_FILE_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON | BTNS_DROPDOWN | TBSTYLE_AUTOSIZE, 0, 0},
	    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0},
	    {3, ID_FILE_PRINT, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON, 0, 0},
	    {5, ID_TOOLS_CLIPBOARD, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON, 0, 0},
	    {6, ID_TOOLS_EXPORTBMP, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON, 0, 0},
	    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0},
	    {7, ID_IMAGE_FIRST, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON, 0, 0},
	    {1, ID_IMAGE_PREV, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON, 0, 0},
	    {2, ID_IMAGE_NEXT, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON, 0, 0},
	    {8, ID_IMAGE_LAST, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON, 0, 0},
	    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0},
	    {4, ID_HELP_ABOUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
	};
	HMODULE hMod;
	status = CreateWindowEx(0, STATUSCLASSNAME, 0, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | SBT_TOOLTIPS, 
				0, 0, 0, 0, hwnd, 0, 0, 0);
	SendMessage(status, SB_SETPARTS, (WPARAM)6, (LPARAM)parts); 
	toolbar = CreateWindowEx(0, TOOLBARCLASSNAME, 0, 
				 WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT | TBSTYLE_FLAT,
				 0, 0, 0, 0, hwnd, 0, 0, 0); 
	SendMessage(toolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
	SendMessage(toolbar, TB_ADDBUTTONS, (WPARAM)12, (LPARAM)tbButtons);
	hMod = GetModuleHandle("comctl32.dll");
	if (GetProcAddress(hMod, "InitCommonControlsEx")) { // Version 4.70
	    imagelist  = ImageList_LoadImage(hInst, MAKEINTRESOURCE(IDB_TOOLBAR24),  
					     16, 0, RGB(255, 0, 255), IMAGE_BITMAP, LR_CREATEDIBSECTION);
	    imagelistd = ImageList_LoadImage(hInst, MAKEINTRESOURCE(IDB_TOOLBAR24D), 
					     16, 0, RGB(255, 0, 255), IMAGE_BITMAP, LR_CREATEDIBSECTION);
	    imagelisth = ImageList_LoadImage(hInst, MAKEINTRESOURCE(IDB_TOOLBAR24H), 
					     16, 0, RGB(255, 0, 255), IMAGE_BITMAP, LR_CREATEDIBSECTION);
	    SendMessage(toolbar, TB_SETIMAGELIST, 0, (LPARAM)imagelist);
	    SendMessage(toolbar, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)imagelistd);
	    SendMessage(toolbar, TB_SETHOTIMAGELIST, 0, (LPARAM)imagelisth);
	    if (GetProcAddress(hMod, "DllGetVersion")) 
		SendMessage(toolbar, TB_SETEXTENDEDSTYLE, 0, (LPARAM)(DWORD)TBSTYLE_EX_DRAWDDARROWS);
	} else {
	    SendMessage(toolbar, TB_ADDBITMAP, (WPARAM)9, (LPARAM)&tbbitmap);
	}
	FreeLibrary(hMod);
	SendMessage(toolbar, TB_AUTOSIZE, 0, 0);
	GetClientRect(hwnd, &rect);
	vlivview = CreateWindowEx(WS_EX_STATICEDGE,
				  "VlivViewClass",
				  0,
				  WS_CHILD  | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
				  rect.left,
				  rect.top,
				  rect.right - rect.left,
				  rect.bottom - rect.top,
				  hwnd,
				  0, 0, 0);
    }
      break;
    case WM_SIZE: {
	int toolbarheight;
	int statusbarheight;
	SendMessage(status, WM_SIZE, 0, 0);
	SendMessage(toolbar, WM_SIZE, 0, 0);
	GetWindowRect(toolbar, &rect);
	toolbarheight = rect.bottom - rect.top;
	GetWindowRect(status, &rect);
	statusbarheight = rect.bottom - rect.top;
	GetClientRect(hwnd, &rect);
	MoveWindow(vlivview, 
		   rect.left, 
		   rect.top + toolbarheight,
		   rect.right - rect.left,
		   rect.bottom - rect.top - statusbarheight - toolbarheight,
		   TRUE);
    }
	break;
    case WM_DESTROY:	
        SaveRecentList();
	SaveWindowPlacement(hwnd);
	FreeFileTypeNames();
	FreeTileInfo();
	CleanupURLControl();
	PostQuitMessage(0);
	break;
    case WM_DROPFILES: {
	TCHAR filename[MAX_PATH];
	if (DragQueryFile((HDROP)wParam, 0, filename, MAX_PATH - 1)) 
	    OpenImage(filename);
	return 0;
    }
	break;
    case WM_COMMAND:
	switch(LOWORD(wParam)) {
	case ID_FILE_EXIT:
	    PostMessage(hwnd, WM_CLOSE, 0, 0);
	    break;
	case ID_FILE_PRINT: {
	  PRINTDLG pd;
	  ZeroMemory(&pd, sizeof(PRINTDLG));
	  pd.lStructSize = sizeof(PRINTDLG);
	  pd.hwndOwner   = hwnd;
	  pd.Flags       = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION; 
	  pd.nCopies     = 1;
	  pd.nFromPage   = 0xFFFF; 
	  pd.nToPage     = 0xFFFF; 
	  pd.nMinPage    = 1; 
	  pd.nMaxPage    = 0xFFFF; 

	  EnableWindow(mainwindow, FALSE);
	  if (PrintDlg(&pd) == TRUE) {
	    HDC hdcPrinter = pd.hDC;
	    HDC hdctemp = CreateCompatibleDC(hdcPrinter);
	    DOCINFO docinfo;
	    unsigned int startx, starty;
	    int widthPage, heightPage;
	    int leftMargin, topMargin, rightMargin, bottomMargin;
	    float scalex, scaley, scale;
	    int nLeftRight, nBottomTop;
	    int w, h;
	    unsigned int currentCopy = 0;
	    unsigned int numtiles = image.numtilesx * image.numtilesy;
	  
	    widthPage = GetDeviceCaps(hdcPrinter, HORZRES);
	    heightPage = GetDeviceCaps(hdcPrinter, VERTRES);
	    leftMargin = GetDeviceCaps(hdcPrinter, PHYSICALOFFSETX);
	    topMargin = GetDeviceCaps(hdcPrinter, PHYSICALOFFSETY);
	    rightMargin = GetDeviceCaps(hdcPrinter, PHYSICALWIDTH) - leftMargin - widthPage;
	    bottomMargin = GetDeviceCaps(hdcPrinter, PHYSICALHEIGHT) - topMargin - heightPage;
	    nLeftRight = max(leftMargin, rightMargin);
	    nBottomTop = max(topMargin, bottomMargin);

	    startx = nLeftRight;
	    starty = nBottomTop;
	    
	    widthPage -= nLeftRight * 2;
	    heightPage -= nBottomTop * 2;

	    scalex = widthPage / (float)image.width;
	    scaley = heightPage / (float)image.height;

	    scale = min(scalex, scaley);

	    w = (int)(image.width * scale);
	    h = (int)(image.height * scale);

	    startx += (widthPage  - w) / 2;
	    starty += (heightPage - h) / 2;

	    ZeroMemory(&docinfo, sizeof(docinfo));
	    docinfo.cbSize = sizeof(docinfo);
	    docinfo.lpszDocName = "Vliv";
	    bPrint = TRUE;
	    abortDialog = CreateDialog(languageInst, (LPTSTR)"AbortDlg", mainwindow, (DLGPROC)AbortDialogProc);
	    SendMessage(GetProgress(), PBM_SETRANGE, 0, MAKELPARAM(0, pd.nCopies * numtiles)); 
	    SetAbortProc(hdcPrinter, AbortProc);
	    StartDoc(hdcPrinter, &docinfo);
	    for (currentCopy = 0; currentCopy < pd.nCopies; ++currentCopy) {
		int tileWidthOnPaper = (int)(image.twidth * scale);
		int tileHeightOnPaper = (int)(image.theight * scale);
		int currenty = starty;
		unsigned int x, y;
		StartPage(hdcPrinter);
		BeginPath(hdcPrinter);
		Rectangle(hdcPrinter, startx, starty, startx + w, starty + h);
		EndPath(hdcPrinter);
		SelectClipPath(hdcPrinter, RGN_COPY);
		for (y = 0; y < image.numtilesy; ++y, currenty += tileHeightOnPaper) {
		    int currentx = startx;
		    for (x = 0; x < image.numtilesx; ++x, currentx += tileWidthOnPaper) {
			TCHAR buffer[100];
			unsigned int pos = x + y * image.numtilesx;
			HBITMAP hbitmap = image.handler->loadtile(&image, hdctemp, x, y);
			if (hbitmap) {
			    SelectObject(hdctemp, hbitmap);
			    // halftone ?
			    StretchBlt(hdcPrinter, // dst
				       currentx, currenty,
				       tileWidthOnPaper, tileHeightOnPaper,
				       hdctemp, // src
				       0, 0,
				       image.twidth, image.theight,
				       SRCCOPY);
			    DeleteObject(hbitmap);
			    
			} 
			SendMessage(GetProgress(), PBM_STEPIT, 0, 0); 
			LoadString(languageInst, IDS_PRINTING, sz, sizeof(sz));
			//			wsprintf(buffer, sz, pos + 1, numtiles, currentCopy + 1, pd.nCopies);
			StringCchPrintf(buffer, MAX_PATH, sz, pos + 1, numtiles, currentCopy + 1, pd.nCopies);
			SetDlgItemText(abortDialog, IDD_PROGTEXT, buffer);
			if (bPrint == FALSE)
			    goto end;
		    }
		}
		EndPage(hdcPrinter);
	    }
	  end:
	    EndDoc(hdcPrinter);
	    DestroyWindow(abortDialog);
	    DeleteDC(hdctemp);
	    DeleteDC(hdcPrinter);
	  } 
	  EnableWindow(mainwindow, TRUE);
	}
	    break;
	case ID_TOOLS_CLEAR:
	    FreeRecentFiles();
	    UpdateRecentMenu(mainwindow);
	    break;
	case ID_TOOLS_FULLSCREEN:
	    SwitchFullscreen(mainwindow, vlivview);
	    break;
	case ID_TOOLS_REGISTER:
	  DialogBox(languageInst, "RegisterDlg", mainwindow, RegisterProc);
	  UpdateToolbarAndMenus();
	  break;
	case ID_TOOLS_EXPORTBMP: {
	    ofnSaveBMP.nMaxFile = MAX_PATH - 1;
	    if (GetSaveFileName(&ofnSaveBMP) == TRUE) {
		HBITMAP bitmap;
		BITMAPFILEHEADER header;
		BITMAPINFO bitmapinfo;
		HANDLE file;
		unsigned int* bits;
		HDC memoryDC;
		HDC winDC;
		RECT rect;
		int width, height;
		HGDIOBJ oldObject;
		GetWindowRect(vlivview, &rect);
		width = rect.right - rect.left - GetSystemMetrics(SM_CXVSCROLL) - GetSystemMetrics(SM_CXEDGE); 
		height = rect.bottom - rect.top - GetSystemMetrics(SM_CYHSCROLL) - GetSystemMetrics(SM_CYEDGE);
		winDC = GetDC(vlivview);
		memoryDC = CreateCompatibleDC(winDC);
		bitmapinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmapinfo.bmiHeader.biWidth = width;
		bitmapinfo.bmiHeader.biHeight = height;
		bitmapinfo.bmiHeader.biPlanes = 1;
		bitmapinfo.bmiHeader.biBitCount = 32;
		bitmapinfo.bmiHeader.biCompression = BI_RGB;
		bitmapinfo.bmiHeader.biSizeImage = width * height * 4;
		bitmap = (HBITMAP)CreateDIBSection(memoryDC, (const BITMAPINFO*)&bitmapinfo, DIB_RGB_COLORS, 
						   (void**)&bits, 0, 0);
		oldObject = SelectObject(memoryDC, bitmap);
		BitBlt(memoryDC, 0, 0, width, height, winDC, 0, 0, SRCCOPY);
		// create the file
		file = CreateFile(szFile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (file != INVALID_HANDLE_VALUE) {
		    DWORD bytesWritten = 0;
		    // now save
		    ZeroMemory(&header, sizeof(BITMAPFILEHEADER));
		    header.bfType = 0x4d42;
		    header.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) + 4 * width * height;
		    header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO);
		    WriteFile(file, &header, sizeof(BITMAPFILEHEADER), &bytesWritten, 0);
		    WriteFile(file, &bitmapinfo, sizeof(BITMAPINFO), &bytesWritten, 0);
		    WriteFile(file, bits, width * height * 4, &bytesWritten, 0);
		    CloseHandle(file);
		}
		SelectObject(memoryDC, oldObject);
		DeleteObject(bitmap);
		DeleteDC(memoryDC);
	    }
	    break;
	}
	case ID_TOOLS_CLIPBOARD: {
	    HBITMAP bitmap;
	    HDC memoryDC;
	    HDC winDC;
	    RECT rect;
	    int width, height;
	    HGDIOBJ oldObject;
	    OpenClipboard(vlivview);
	    EmptyClipboard();
	    GetWindowRect(vlivview, &rect);
	    width = rect.right - rect.left - GetSystemMetrics(SM_CXVSCROLL) - GetSystemMetrics(SM_CXEDGE); 
	    height = rect.bottom - rect.top - GetSystemMetrics(SM_CYHSCROLL) - GetSystemMetrics(SM_CYEDGE);
	    winDC = GetDC(vlivview);
	    memoryDC = CreateCompatibleDC(winDC);
	    bitmap = CreateCompatibleBitmap(winDC, width, height);
	    oldObject = SelectObject(memoryDC, bitmap);
	    BitBlt(memoryDC, 0, 0, width, height, winDC, 0, 0, SRCCOPY);
	    SetClipboardData(CF_BITMAP, bitmap);
	    SelectObject(memoryDC, oldObject);
	    CloseClipboard();
	    DeleteDC(memoryDC);
	    break;
	}
	case ID_FILE_OPEN:
	    ofnOpen.nMaxFile = MAX_PATH - 1;
	    if (GetOpenFileName(&ofnOpen) == TRUE) 
		OpenImage(ofnOpen.lpstrFile);
	    break;
	case ID_IMAGE_FIRST:
	    SetDirectory(0);
	    UpdateScrollbars(TRUE);
	    break;
	case ID_IMAGE_PREV:
	    SetDirectory(image.currentdir - 1);
	    UpdateScrollbars(TRUE);
	    break;
	case ID_IMAGE_NEXT:
	    SetDirectory(image.currentdir + 1);
	    UpdateScrollbars(TRUE);
	    break;
	case ID_IMAGE_LAST:
	    SetDirectory(image.numdirs - 1);
	    UpdateScrollbars(TRUE);
	    break;
	case ID_HELP_ABOUT:
	    DialogBox(languageInst, "AboutDlg", mainwindow, DialogProcAbout);
	    break;
	case ID_HELP_CREDITS:
	    DialogBox(hInst, "CreditsDlg", mainwindow, DialogProcCredits);
	    break;
	}
	if ((LOWORD(wParam) >= ID_FILE_RECENT) && (LOWORD(wParam) < (ID_FILE_RECENT + GetNumRecent()))) 
	    OpenImage(GetRecentFile(LOWORD(wParam) - ID_FILE_RECENT));
	return 0;
	break; 
     case WM_CHAR:
	switch(wParam) { 
	case 0x1B: 
	    PostQuitMessage(0);
	    break; 
	default:
	    break;
	}
	return 0;
	break;
    case WM_KEYDOWN:
	switch(wParam) {
	case VK_HOME:  SendMessage(vlivview, WM_VSCROLL, SB_TOP, 0); break;
	case VK_END:   SendMessage(vlivview, WM_VSCROLL, SB_BOTTOM, 0); break;
	case VK_PRIOR: SendMessage(vlivview, WM_VSCROLL, SB_PAGEUP, 0); break;
	case VK_NEXT:  SendMessage(vlivview, WM_VSCROLL, SB_PAGEDOWN, 0); break;
	case VK_UP:    SendMessage(vlivview, WM_VSCROLL, SB_LINEUP, 0); break;
	case VK_DOWN:  SendMessage(vlivview, WM_VSCROLL, SB_LINEDOWN, 0); break;
	case VK_LEFT:  SendMessage(vlivview, WM_HSCROLL, SB_PAGEUP, 0); break;
	case VK_RIGHT: SendMessage(vlivview, WM_HSCROLL, SB_PAGEDOWN, 0); break;
	case VK_RETURN: SwitchFullscreen(mainwindow, vlivview); break;
	default: break;
	}
	return 0;
	break;
    case WM_MOUSEWHEEL: 
	if (HandleMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam)))
	    return 0;
	break;
    case WM_SIZING: {
	const int minwidth = 300;
	const int minheight = 300;
	UINT dir = (UINT)wParam;
	LPRECT rect = (LPRECT)lParam;
	int width = rect->right - rect->left;
	int height = rect->bottom - rect->top;
	if (width < minwidth) {
	    if ((dir == WMSZ_RIGHT) || (dir == WMSZ_TOPRIGHT) || (dir == WMSZ_BOTTOMRIGHT))
		rect->right = rect->left + minwidth;
	    else
		rect->left = rect->right - minwidth;
	}
	if (height <= minheight) {
	    if ((dir == WMSZ_BOTTOM) || (dir == WMSZ_BOTTOMRIGHT) || (dir == WMSZ_BOTTOMLEFT))
		rect->bottom = rect->top + minheight;
	    else
		rect->top = rect->bottom - minheight;
	}
	return TRUE;
    }
    case WM_NOTIFY: {
      LPNMHDR lpnmhdr = (LPNMHDR)lParam;
      switch (lpnmhdr->code) {
      case TTN_NEEDTEXT: {
	  TCHAR buffer[100];
	  LPTOOLTIPTEXT lpToolTipText = (LPTOOLTIPTEXT)lParam;
	  LoadString(languageInst, lpToolTipText->hdr.idFrom, buffer, 30);
	  lpToolTipText->lpszText = buffer;
	break;
      }
      case TBN_DROPDOWN: {
	  if (GetRecentFile(0) != 0) {
	      HMENU mainMenu = GetMenu(mainwindow);
	      HMENU hMenuFile = GetSubMenu(mainMenu, 0);
	      HMENU hMenuRecent = GetSubMenu(hMenuFile, 4);
	      RECT rc;
	      LPNMTOOLBAR toolbar = (LPNMTOOLBAR)lParam;
	      SendMessage(toolbar->hdr.hwndFrom, TB_GETRECT, (WPARAM)toolbar->iItem, (LPARAM)&rc);
	      MapWindowPoints(toolbar->hdr.hwndFrom, HWND_DESKTOP, (LPPOINT)&rc, 2);	
	      SetForegroundWindow(mainwindow);
	      TrackPopupMenu(hMenuRecent, 
			     TPM_LEFTALIGN|TPM_VERTICAL|TPM_LEFTBUTTON,
			     rc.left, rc.bottom,
			     0,
			     mainwindow,
			     0);
	      PostMessage(mainwindow, WM_NULL, 0, 0); 
	      return TBDDRET_DEFAULT;
	  }
	  break;
      }
    break;
      }
    }
    default:
	return DefWindowProc( hwnd, msg, wParam, lParam );
    }
    return 0;	
}

typedef HWND (WINAPI *StartPanProc) (HWND hwnd, BOOL, BOOL, POINT);
typedef WINUSERAPI BOOL (WINAPI * SetLayeredWindowAttributesF)(HWND, COLORREF,	BYTE, DWORD);
#if !defined(LWA_ALPHA)
#  define LWA_ALPHA     0x00000002
#endif /* LWA_ALPHA */
void ScrollWithOffset(HWND hwnd, int dx, int dy);

LRESULT CALLBACK VlivWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static BOOL started;
    static int mousex;
    static int mousey;
    RECT rect;
    SCROLLINFO si;
    si.cbSize = sizeof (si);

    switch(msg) {
    case WM_CREATE: {
	uOriginWindowUpdateMessage = RegisterWindowMessage("gs_OriginWindowUpdate");
	uOriginWindowEndMessage = RegisterWindowMessage("gs_OriginWindowEnd");
	InitRawInputDevice(hwnd);
	InitJoystick(hwnd);
	hdcbitmap = CreateCompatibleDC(0);
	return 0;
	break;
    }
    case WM_SIZE:
	GetClientRect(hwnd, &rect);
	InvalidateRect(hwnd, 0, TRUE);
	si.fMask = SIF_PAGE | SIF_DISABLENOSCROLL;
	si.nPage = rect.right;
	SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
	si.nPage = rect.bottom;
	SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
	return 0;
	break;
    case WM_MBUTTONDOWN: {
	owndInst = LoadLibrary("ownd.dll");
	if(owndInst) {
	    StartPanProc pStartPanning = (StartPanProc)GetProcAddress(owndInst, "StartPanning");
	    if(pStartPanning) {
		POINT pt = { LOWORD(lParam), HIWORD(lParam) };
		HWND wpan;
		pStartPanning(hwnd, TRUE, TRUE, pt);
		wpan = FindWindow("gs_Ownd10", NULL);
		if (wpan) {
		    HINSTANCE user32Inst = LoadLibrary("user32.dll");
		    if (user32Inst) {
			SetLayeredWindowAttributesF pSetLayeredAttributes = 
			    (SetLayeredWindowAttributesF)GetProcAddress(user32Inst, "SetLayeredWindowAttributes");
			if (pSetLayeredAttributes) {
			    SetWindowLong(wpan, GWL_EXSTYLE, GetWindowLong(wpan, GWL_EXSTYLE) | 0x00080000);
			    pSetLayeredAttributes(wpan, 0, (BYTE)128, LWA_ALPHA);
			} 
		    }
		} 
	    }
	}
	return 0;
    }
    case WM_PAINT: {
	HDC hDC;
	PAINTSTRUCT ps;
	RECT invalidrect, backrect;
	unsigned int x, y;
	int startx, starty;
	unsigned int miny, maxy;
	unsigned int currenty;
      
	si.fMask  = SIF_ALL;
	hDC = BeginPaint(hwnd, &ps);
	GetClientRect(hwnd, &rect);
	if (image.handler == 0) {
	    FillRect(hDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
	    EndPaint(hwnd, &ps);
	    return 0;
	}
	GetScrollInfo (hwnd, SB_HORZ, &si);
	startx = -si.nPos;
	invalidrect.left = si.nPos + ps.rcPaint.left;
	invalidrect.right = invalidrect.left + ps.rcPaint.right - ps.rcPaint.left;
      
	GetScrollInfo (hwnd, SB_VERT, &si);
	starty = -si.nPos;

	invalidrect.top = si.nPos + ps.rcPaint.top;
	invalidrect.bottom = invalidrect.top + ps.rcPaint.bottom - ps.rcPaint.top;

	currenty = 0;
	// create a list of tiles to load
	if (image.theight != 0) {
		miny = invalidrect.top / image.theight;
		maxy = invalidrect.bottom / image.theight + 1;
	} else {
		miny = invalidrect.top; 
		maxy = image.numtilesy;
	}
	if (maxy >= image.numtilesy)
	    maxy = image.numtilesy;

	// image is fully visible, but size not multiple of tiles
	if ((invalidrect.left == 0) && (invalidrect.right > (int)image.width))
	  invalidrect.right = image.width;
	if ((invalidrect.top == 0) && (invalidrect.bottom > (int)image.height))
	  invalidrect.bottom = image.height;

	InitTileInfo();
	for (y = 0; y < miny; ++y) FreeRow(y);
	//	for (y = 0; y < image.numtilesy; ++y) {
	for (y = miny; y < maxy; ++y) {
	    unsigned int currentx = 0;
	    currenty = y * image.theight;
	    CreateRow(y);
	    for (x = 0; x < image.numtilesx; ++x) {
		RECT tilerect;
		RECT inter;
		currentx = x * image.twidth;
		SetRect(&tilerect, 
			currentx, 
			currenty, 
			currentx + image.twidth, 
			currenty + image.theight);
		if (IntersectRect(&inter, &invalidrect, &tilerect)) {
		    AddTileInfo(x, y,
				startx + inter.left,
				starty + inter.top,
				inter.right - inter.left,
				inter.bottom - inter.top,
				inter.left - tilerect.left,
				inter.top - tilerect.top);
		} else {
		    FreeTile(x, y);
		}
	    }
	}
	for (y = maxy; y < image.numtilesy; ++y) FreeRow(y);
	LoadTiles(hdcbitmap);
	// draw
	for (x = 0; x < ntileinfo; ++x) {
	    TileInfo* tinfo = &tileinfo[x];
	    if (IsTileLoaded(tinfo->x, tinfo->y)) {
		Tile* tile = GetTileAtPos(tinfo->x, tinfo->y);
		SelectObject(hdcbitmap, tile->bitmap);
		BitBlt(hDC, 
		       tinfo->sx,
		       tinfo->sy,
		       tinfo->sw,
		       tinfo->sh,
		       hdcbitmap,
		       tinfo->dx,
		       tinfo->dy,
		       SRCCOPY);
	    } else {
		// fill with black
		RECT tilerect;
		SetRect(&tilerect, tinfo->dx, tinfo->dy,  tinfo->sh, tinfo->sw);
		FillRect(hDC, &tilerect, (HBRUSH)GetStockObject(BLACK_BRUSH));
	    }
	}
	if (rect.right >= (int)(image.width)) {
	    SetRect(&backrect, image.width, 0, rect.right, rect.bottom); 
	    FillRect(hDC, &backrect, (HBRUSH)GetStockObject(BLACK_BRUSH));
	}
	if (rect.bottom >= (int)(image.height)) {
	    SetRect(&backrect, 0, image.height, rect.right, rect.bottom);
	    FillRect(hDC, &backrect, (HBRUSH)GetStockObject(BLACK_BRUSH));
	}
	EndPaint(hwnd, &ps);
    }
	return 0;
	break;
    case WM_INPUT: {
      // rawinput
      RAWINPUTHEADER header;
      UINT size = sizeof(header);
      if (GetRawInputData( (HRAWINPUT)lParam, RID_HEADER, &header,  &size, sizeof(RAWINPUTHEADER)) != -1) {
	// "rawEvent.header: hDevice = 0x%x\n", header.hDevice
	LPRAWINPUT event;
	size = header.dwSize;
	event = (LPRAWINPUT)malloc(size);
	if (GetRawInputData( (HRAWINPUT)lParam, RID_INPUT, event, &size, sizeof(RAWINPUTHEADER)) != -1) {
	  if (event->header.dwType == RIM_TYPEHID) { // space nagivator ?
	    static BOOL bGotTranslation = FALSE;
	    static BOOL bGotRotation = FALSE;
	    static int all6DOFs[6] = {0};
	    LPRAWHID pRawHid = &event->data.hid;
	    if (pRawHid->bRawData[0] == 1) { // translation
	      all6DOFs[0] = (pRawHid->bRawData[1] & 0x000000ff) | ((signed short)(pRawHid->bRawData[2]<<8) & 0xffffff00); 
	      all6DOFs[1] = (pRawHid->bRawData[3] & 0x000000ff) | ((signed short)(pRawHid->bRawData[4]<<8) & 0xffffff00); 
	      all6DOFs[2] = (pRawHid->bRawData[5] & 0x000000ff) | ((signed short)(pRawHid->bRawData[6]<<8) & 0xffffff00);
#if 0
	      {
		char buffer[100];
		wsprintf(buffer, "translation %d %d %d", all6DOFs[0], all6DOFs[1], all6DOFs[2]);
		OutputDebugString(buffer);
	      }
#endif
	      ScrollWithOffset(hwnd, -all6DOFs[0] / 10, -all6DOFs[1] / 10);
	      bGotTranslation = TRUE;
	    } else if (pRawHid->bRawData[0] == 2) { // rotation
	      all6DOFs[3] = (pRawHid->bRawData[1] & 0x000000ff) | ((signed short)(pRawHid->bRawData[2]<<8) & 0xffffff00); 
	      all6DOFs[4] = (pRawHid->bRawData[3] & 0x000000ff) | ((signed short)(pRawHid->bRawData[4]<<8) & 0xffffff00); 
	      all6DOFs[5] = (pRawHid->bRawData[5] & 0x000000ff) | ((signed short)(pRawHid->bRawData[6]<<8) & 0xffffff00);
	      bGotRotation = TRUE;
	    } else if (pRawHid->bRawData[0] == 3) { // Buttons
#if 0
	      char buffer[100];
	      wsprintf(buffer, "%.2x %.2x %.2x\n",
		       (unsigned char)pRawHid->bRawData[3],
		       (unsigned char)pRawHid->bRawData[2],
		       (unsigned char)pRawHid->bRawData[1]);
	      OutputDebugString(buffer);
#endif
	    }
	    if (bGotTranslation && bGotRotation) {
	      bGotTranslation = bGotRotation = FALSE;
	    }
	  }
	}
      }
    }
      break;
    case WM_DESTROY:
	DeleteDC(hdcbitmap);
	DestroyIcon(warningicon);
	DestroyIcon(idleicon);
	DestroyIcon(busyicon);
	FreeImage();
	UnRegisterFormatHandlers();
	FreeRawInputDevices();
	ImageList_Destroy(imagelist);
	ImageList_Destroy(imagelistd);
	ImageList_Destroy(imagelisth);
	return 0;
    case WM_ERASEBKGND:
	return 0;
	break;
    case WM_HSCROLL: {
	int oldxpos;
	si.fMask  = SIF_ALL;
	GetScrollInfo (hwnd, SB_HORZ, &si);
	oldxpos = si.nPos;
	switch (LOWORD (wParam)) {
	case SB_LINELEFT:  si.nPos -= 1; break;
	case SB_LINERIGHT: si.nPos += 1; break;
	case SB_PAGELEFT:  si.nPos -= si.nPage; break;
	case SB_PAGERIGHT: si.nPos += si.nPage; break;
	case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
	default: break;
	}
	si.fMask = SIF_POS;
	SetScrollInfo (hwnd, SB_HORZ, &si, TRUE);
	GetScrollInfo (hwnd, SB_HORZ, &si);
	if (oldxpos != si.nPos) {
	    ScrollWindowEx(hwnd, (oldxpos - si.nPos), 0, 0, 0, 0, 0, SW_INVALIDATE);
	    UpdateWindow(hwnd);
	}
	return 0;
	break;
    }
    case WM_VSCROLL: {
	int oldypos;
	si.fMask  = SIF_ALL;
	GetScrollInfo (hwnd, SB_VERT, &si);
	oldypos = si.nPos;
	switch (LOWORD (wParam)) {
	case SB_TOP:      si.nPos = si.nMin; break;
	case SB_BOTTOM:   si.nPos = si.nMax; break;
	case SB_LINEUP:   si.nPos -= 1; break;
	case SB_LINEDOWN: si.nPos += 1; break;
	case SB_PAGEUP:   si.nPos -= si.nPage; break;
	case SB_PAGEDOWN: si.nPos += si.nPage; break;
	case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
	default: break;
	}
	si.fMask = SIF_POS;
	SetScrollInfo (hwnd, SB_VERT, &si, TRUE);
	GetScrollInfo (hwnd, SB_VERT, &si);
	if (oldypos != si.nPos) {
	    ScrollWindowEx(hwnd, 0, (oldypos - si.nPos), 0, 0, 0, 0, SW_INVALIDATE);
	    UpdateWindow(hwnd);
	}
	return 0;
	break;
    }
    case WM_LBUTTONDOWN:
	if (image.handler) {
	    started = TRUE;
	    mousex = GET_X_LPARAM(lParam); 
	    mousey = GET_Y_LPARAM(lParam);
	    SetCursor(cursor);
	    SetCapture(hwnd);
	}
	return 0;
	break;
    case WM_LBUTTONUP:
	if (image.handler) {
	    started = FALSE;
	    mousex = GET_X_LPARAM(lParam); 
	    mousey = GET_Y_LPARAM(lParam);
	    SetCursor(oldcursor);
	    ReleaseCapture();
	}
	return 0;
	break;    
    case WM_MOUSEWHEEL: 
	if (HandleMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam)))
	    return 0;
	break;
    case WM_MOUSEMOVE:
		if (image.handler) {
			if ((started == TRUE) && (wParam && MK_LBUTTON)) {
			int newmousex = GET_X_LPARAM(lParam); 
			int newmousey = GET_Y_LPARAM(lParam);
			int dx = newmousex - mousex;
			int dy = newmousey - mousey;
			ScrollWithOffset(hwnd, dx, dy);
			mousex = newmousex;
			mousey = newmousey;
			}
		}
		return 0;
	break;
	case MM_JOY1BUTTONDOWN:
		if (wParam & JOY_BUTTON1)
			SetDirectory(image.currentdir - 1);
		if (wParam & JOY_BUTTON2)
			SetDirectory(image.currentdir + 1);
	    UpdateScrollbars(TRUE);
	break;
	case MM_JOY1MOVE: {
		int x = 0, y = 0;
		x = LOWORD(lParam) >> 12;
		y = HIWORD(lParam) >> 12;
		// add some factor here ?
		ScrollWithOffset(hwnd, x, y);
		return 0;
	}
	break;
    case WM_KEYDOWN:
	switch(wParam) {
	case VK_RETURN: SwitchFullscreen(mainwindow, vlivview); break;
	default: break;
	}
	return 0;
	break;
    default:
	if(msg == uOriginWindowEndMessage) {
	    FreeLibrary(owndInst);
	    SetCursor(cursor);
	} else if (msg == uOriginWindowUpdateMessage) {
	    if (image.handler) {
		SIZE *psizeDistance = (SIZE*)lParam;
		int dx = -psizeDistance->cx;
		int dy = -psizeDistance->cy;
		ScrollWithOffset(hwnd, dx, dy);
	    }
	    return 1;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

static void ScrollWithOffset(HWND hwnd, int dx, int dy) {
  int oldposx;
  int newposx;
  int oldposy;
  int newposy;
  SCROLLINFO si;
  si.cbSize = sizeof (si);
  si.fMask = SIF_ALL;
  GetScrollInfo(hwnd, SB_HORZ, &si);
  oldposx = si.nPos;
  if ((si.nPos - dx) <= si.nMin) {
    si.nPos = si.nMin;
  } else if ((si.nPos - dx) >= (si.nMax - (int)si.nPage + 1)) {
    si.nPos = (si.nMax - (int)si.nPage + 1);
  } else 
    si.nPos -= dx;
  newposx = si.nPos;
  SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
  
  GetScrollInfo(hwnd, SB_VERT, &si);
  oldposy = si.nPos;
  if ((si.nPos - dy) <= si.nMin) {
    si.nPos = si.nMin;
  } else if ((si.nPos - dy) >= (si.nMax - (int)si.nPage + 1)) {
    si.nPos = (si.nMax - (int)si.nPage + 1);
  } else 
    si.nPos -= dy;
  newposy = si.nPos;
  SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
  ScrollWindowEx(hwnd, oldposx - newposx, oldposy - newposy, 0, 0, 0, 0, SW_INVALIDATE);
  UpdateWindow(hwnd);
}

static void 
InitLanguage(HINSTANCE hInstance) {
    LCID lcid = GetUserDefaultLCID();
//	LCID lcid = MAKELCID(MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH), SORT_DEFAULT);
    TCHAR lpszLang[4];
    TCHAR buffer[20];
    GetLocaleInfo(lcid, LOCALE_SABBREVLANGNAME, lpszLang, 4);
    StringCchPrintf(buffer, 20, TEXT("%s.dll"), lpszLang);
     languageInst = LoadLibrary(buffer);
     if (languageInst) {
		SetThreadLocale(lcid);
	 } else {
		languageInst = hInstance;
    }
    InitFileTypeNames();
}

static void InitOpenFileName(HWND window) {
    ZeroMemory(&ofnOpen, sizeof(OPENFILENAME));
    ofnOpen.lStructSize = sizeof(OPENFILENAME);
    ofnOpen.hwndOwner = window;
    ofnOpen.lpstrFilter = GetCustomFilter();
    ofnOpen.nFilterIndex = 2;
    ofnOpen.lpstrTitle = (LPTSTR)"Open image file...";
    ofnOpen.lpstrFile = szFile;
    ofnOpen.nMaxFile = sizeof(szFile);
    ofnOpen.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
}

static void InitSaveFileName(HWND window) {
    ZeroMemory(&ofnSaveBMP, sizeof(OPENFILENAME));
    ofnSaveBMP.lStructSize = sizeof(OPENFILENAME);
    ofnSaveBMP.hwndOwner = window;
    ofnSaveBMP.lpstrFilter = "All Files (*.*)\0*.*\0Windows BMP Files\0*.bmp\0";
    ofnSaveBMP.nFilterIndex = 2;
    ofnSaveBMP.lpstrTitle = (LPTSTR)"Save as BMP...";
    ofnSaveBMP.lpstrFile = szFile;
    ofnSaveBMP.nMaxFile = sizeof(szFile);
    ofnSaveBMP.Flags = OFN_OVERWRITEPROMPT;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow) {
    WNDCLASS wc;
    MSG msg;
    HANDLE hAccel;
    WININFO wi;
    TCHAR filename[MAX_PATH];
    hInst = hInstance;
    InitCommonControls();
    InitLanguage(hInstance);
    InitImage();
    RegisterFormatHandlers(&image);
    CheckProcNumber();
    oldcursor = LoadCursor(hInst, "HAND1");
    warningicon = (HICON)LoadImage(hInst, "MYINFO", IMAGE_ICON, 0, 0, 0);
    idleicon = (HICON)LoadImage(hInst, "IDLE", IMAGE_ICON, 0, 0, 0);
    busyicon = (HICON)LoadImage(hInst, "BUSY", IMAGE_ICON, 0, 0, 0);
    ZeroMemory(&wc, sizeof(wc));
    if(!hPrevInstance) {
	wc.lpszClassName = "VlivViewClass";
	wc.lpfnWndProc = VlivWndProc;
	wc.hInstance = hInstance;
	wc.hCursor = oldcursor;
	wc.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	RegisterClass(&wc);

	wc.lpszClassName = "VlivTopClass";
	wc.lpfnWndProc = TopWndProc;
	wc.hIcon = LoadIcon(hInst, "APPLICATION");
	RegisterClass(&wc);
    }
    ZeroMemory(filename, sizeof(filename));
    if ((lpszCmdLine != 0) && (lpszCmdLine[0] != '\0'))
        if (lpszCmdLine[0] == '"') 
	    CopyMemory(filename, lpszCmdLine + 1, lstrlen(lpszCmdLine) - 2);
	else {
	    //  lstrcpy(filename, lpszCmdLine);
	    StringCchCopy(filename, MAX_PATH, lpszCmdLine);
	}
    else
        filename[0] = 0;
    LoadRecentList();
    LoadWindowPlacement(&wi);
    LoadString(languageInst, IDS_UNTITLED, sz, sizeof(sz));
    mainwindow = CreateWindowEx(WS_EX_ACCEPTFILES,
				"VlivTopClass",
				sz,
				WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
				wi.x, wi.y,
				wi.cx, wi.cy,
				0, 0,
				hInstance,
				0);
    SetCursor(oldcursor);
    ShowWindow(mainwindow, wi.max ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
    SetMenu(mainwindow, LoadMenu(languageInst, "VLIVMENU"));
    checkLicense();
    EnableMenuItem(GetSubMenu(GetMenu(mainwindow), 2), ID_TOOLS_REGISTER, MF_BYCOMMAND | MF_GRAYED);

    UpdateRecentMenu(mainwindow);
    UpdateStatusTile(FALSE);
    cursor = LoadCursor(hInst, "Hand2");
    hAccel = LoadAccelerators(hInst, "VLIVACCEL");

    InitOpenFileName(mainwindow);
    InitSaveFileName(mainwindow);
    
    if (filename[0] != 0) {
		OpenImage(filename);
	}
    while(GetMessage(&msg, 0, 0, 0)) {
      if (!TranslateAccelerator(mainwindow, hAccel, &msg)) {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
      }
    }
    return msg.wParam;
}

void UpdateToolbarAndMenus() {
    HMENU mainMenu = GetMenu(mainwindow);
    HMENU hMenuFile = GetSubMenu(mainMenu, 0);
    HMENU hMenuImage = GetSubMenu(mainMenu, 1);
    HMENU hMenuTools = GetSubMenu(mainMenu, 2);
    if (image.handler) {
	if (image.currentdir == 0) {
	    SendMessage(toolbar, TB_SETSTATE, (WPARAM)ID_IMAGE_PREV, (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
	    SendMessage(toolbar, TB_SETSTATE, (WPARAM)ID_IMAGE_FIRST, (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
	    EnableMenuItem(hMenuImage, ID_IMAGE_PREV, MF_BYCOMMAND | MF_GRAYED);
	    EnableMenuItem(hMenuImage, ID_IMAGE_FIRST, MF_BYCOMMAND | MF_GRAYED);
	} else {
	    SendMessage(toolbar, TB_SETSTATE, (WPARAM)ID_IMAGE_PREV, (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
	    SendMessage(toolbar, TB_SETSTATE, (WPARAM)ID_IMAGE_FIRST, (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
	    EnableMenuItem(hMenuImage, ID_IMAGE_PREV, MF_BYCOMMAND | MF_ENABLED);
	    EnableMenuItem(hMenuImage, ID_IMAGE_FIRST, MF_BYCOMMAND | MF_ENABLED);
	}
	if (image.currentdir == (image.numdirs - 1)) {
	    SendMessage(toolbar, TB_SETSTATE, (WPARAM)ID_IMAGE_NEXT, (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
	    SendMessage(toolbar, TB_SETSTATE, (WPARAM)ID_IMAGE_LAST, (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
	    EnableMenuItem(hMenuImage, ID_IMAGE_NEXT, MF_BYCOMMAND | MF_GRAYED);
	    EnableMenuItem(hMenuImage, ID_IMAGE_LAST, MF_BYCOMMAND | MF_GRAYED);
	} else {
	    SendMessage(toolbar, TB_SETSTATE, (WPARAM)ID_IMAGE_NEXT, (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
	    SendMessage(toolbar, TB_SETSTATE, (WPARAM)ID_IMAGE_LAST, (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
	    EnableMenuItem(hMenuImage, ID_IMAGE_NEXT, MF_BYCOMMAND | MF_ENABLED);
	    EnableMenuItem(hMenuImage, ID_IMAGE_LAST, MF_BYCOMMAND | MF_ENABLED);
	}
    }
    EnableMenuItem(hMenuTools, ID_TOOLS_REGISTER, MF_BYCOMMAND | MF_GRAYED);
    if (image.handler == 0) {
	SendMessage(toolbar, TB_SETSTATE, (WPARAM)ID_FILE_PRINT, (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
	SendMessage(toolbar, TB_SETSTATE, (WPARAM)ID_TOOLS_EXPORTBMP, (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
	SendMessage(toolbar, TB_SETSTATE, (WPARAM)ID_TOOLS_CLIPBOARD, (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
    } else {
	SendMessage(toolbar, TB_SETSTATE, (WPARAM)ID_FILE_PRINT, (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
	SendMessage(toolbar, TB_SETSTATE, (WPARAM)ID_TOOLS_EXPORTBMP, (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
	SendMessage(toolbar, TB_SETSTATE, (WPARAM)ID_TOOLS_CLIPBOARD, (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
	EnableMenuItem(hMenuFile, ID_FILE_PRINT, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(hMenuTools, ID_TOOLS_EXPORTBMP, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(hMenuTools, ID_TOOLS_CLIPBOARD, MF_BYCOMMAND | MF_ENABLED);
    }
    DrawMenuBar(mainwindow);
}

void
ShowLastError(LPTSTR lpszFunction) {
    DWORD dw = GetLastError();
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		  FORMAT_MESSAGE_FROM_SYSTEM |
		  FORMAT_MESSAGE_IGNORE_INSERTS,
		  NULL,
		  dw,
		  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		  (LPTSTR) &lpMsgBuf,
		  0, NULL );
    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
				      (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
		    LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		    TEXT("%s failed with error %d: %s"), 
		    lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 
    
    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}
