#include <recent.h>
#include <vliv.h>
#include <resources/resource.h>

static unsigned int numrecent = 10;
static TCHAR** recentFiles;
const char* VlivRegKeyMru = "Software\\Delhoume\\Vliv\\MRU";
static TCHAR sz[MAX_PATH];

unsigned int GetNumRecent() {
    return numrecent;
}

const TCHAR* GetRecentFile(unsigned int num) {
    return recentFiles[num];
}

void FreeRecentFiles() {
    unsigned int idx;
    for (idx = 0; idx < numrecent; ++idx) 
	if (recentFiles[idx]) {
	    MYFREE(recentFiles[idx]);
	    recentFiles[idx] = 0;
	}
}

void LoadRecentList() {
    unsigned int idx;
    HKEY hKey;
    TCHAR szFileName[MAX_PATH];
    recentFiles = MYALLOC(numrecent * sizeof(TCHAR*));
    for (idx = 0; idx < numrecent; ++idx) {
	recentFiles[idx] = 0;
    }
    if (RegOpenKeyEx(HKEY_CURRENT_USER, VlivRegKeyMru, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
	for (idx = 0; idx < numrecent; ++idx) {
	    DWORD buflen = MAX_PATH;
	    char buffer[10];
	    wsprintf(buffer, "File%2d", idx);
	    if (RegQueryValueEx(hKey, TEXT(buffer), 0, 0, (LPBYTE)szFileName, &buflen) == ERROR_SUCCESS) {
		recentFiles[idx] = MYALLOC(buflen);
		lstrcpy(recentFiles[idx], szFileName);
	    } else 
		break;
	}
	RegCloseKey(hKey);
    }
}

void SaveRecentList() {
    HKEY hKey;
    DWORD disposition;
    if (RegCreateKeyEx(HKEY_CURRENT_USER,
		       VlivRegKeyMru,
		       0,
		       0,
		       REG_OPTION_NON_VOLATILE,
		       KEY_ALL_ACCESS,
		       0,
		       &hKey,
		       &disposition) == ERROR_SUCCESS) {
	unsigned int idx;
	for (idx = 0; idx < numrecent; ++idx) {
	    char buffer[10];
	    wsprintf(buffer, "File%2d", idx);
	    if (recentFiles[idx] != 0) {
		RegSetValueEx(hKey,
			      TEXT(buffer),
			      0,
			      REG_SZ,
			      recentFiles[idx],
			      lstrlen(recentFiles[idx]) + 1);
		MYFREE(recentFiles[idx]);
	    } else {
		RegDeleteValue(hKey, TEXT(buffer));
	    }
	}
	RegCloseKey(hKey);
    } 
    MYFREE(recentFiles);
}

void UpdateRecentList(const TCHAR* filename) {
    unsigned int idx;
    TCHAR fullname[MAX_PATH];
    // TODO: absolute filename
    DWORD numchars = GetFullPathName(filename, MAX_PATH, fullname, 0);
    TCHAR* copyfilename = MYALLOC(numchars + 1);
    lstrcpy(copyfilename, fullname);
    for (idx = 0; idx < numrecent - 1; ++idx) {
	if ((recentFiles[idx] == 0) || !lstrcmp(recentFiles[idx], copyfilename))
           break;
    }
    for (; idx > 0; --idx) {
	if (recentFiles[idx])
	    MYFREE(recentFiles[idx]);
	recentFiles[idx] = MYALLOC(lstrlen(recentFiles[idx - 1]) + 1);
	lstrcpy(recentFiles[idx], recentFiles[idx - 1]);
    }
    recentFiles[0] = copyfilename;
}

void UpdateRecentMenu(HWND window) {
    HMENU mainMenu = GetMenu(window);
    HMENU hMenuFile = GetSubMenu(mainMenu, 0);
    HMENU hMenuRecent = GetSubMenu(hMenuFile, 4);
    HMENU hMenuTools = GetSubMenu(mainMenu, 2);
    unsigned int idx;
    LoadString(languageInst, IDS_RECENT_MENU, sz, sizeof(sz));
    if (recentFiles[0] == 0) {
	ModifyMenu(hMenuFile, 
		   4, 
		   MF_BYPOSITION | MF_GRAYED | MF_STRING | MF_POPUP, 
		   (UINT_PTR)hMenuRecent, 
		   sz);
	for (idx = 0; idx < numrecent; ++idx) {
	    if (recentFiles[idx] != 0) 
		DeleteMenu(hMenuRecent, idx, MF_BYPOSITION);
	    else 
		break;
	}
	EnableMenuItem(hMenuTools, ID_TOOLS_CLEAR, MF_BYCOMMAND | MF_GRAYED);
    } else {
	ModifyMenu(hMenuFile, 
		   4, 
		   MF_BYPOSITION | MF_STRING | MF_POPUP, 
		   (UINT_PTR)hMenuRecent, 
		   sz);
	for (idx = 0; idx < numrecent; ++idx) 
	    DeleteMenu(hMenuRecent, 0, MF_BYPOSITION);
	for (idx = 0; idx < numrecent; ++idx) {
	    if (recentFiles[idx] != 0) 
		AppendMenu(hMenuRecent, MF_STRING, ID_FILE_RECENT + idx, recentFiles[idx]);
	    else 
		break;
	}   
	EnableMenuItem(hMenuTools, ID_TOOLS_CLEAR, MF_BYCOMMAND | MF_ENABLED); 
    }
    DrawMenuBar(window);
}


