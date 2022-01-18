#include <handlers.h>

#include <shlwapi.h>

static FormatHandlerPtr* formats = 0;
static unsigned int numformats = 0;
static TCHAR customfilter[1024];

const TCHAR* GetCustomFilter() {
    return customfilter;
}

FormatHandlerPtr GetFormatHandler(const unsigned char* name,
				  const unsigned char* buffer, 
				  unsigned int size) {
    unsigned int idx;
    for (idx = 0; idx < numformats; ++idx) {
	FormatHandlerPtr handler = formats[idx];
	BOOL okaccept = handler->accept(buffer, size);
	// should look at all possible extensions
	BOOL okexten = PathMatchSpec(name, handler->getextension());
	if ((okaccept == TRUE) || (okexten == TRUE))
	    return handler;
    }
    return 0;
}

FormatHandlerPtr GetHandlerFromFile(const TCHAR* name) {
    FormatHandlerPtr handler = 0;
    unsigned char buffer[12];
    unsigned int size;
    HANDLE handle = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
			       FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle) {
	ReadFile(handle, (char*)buffer, 12, &size, 0);
	CloseHandle(handle);
	handler = GetFormatHandler(name, buffer, size);
    }
    return handler;
}

void RegisterFormatHandler(ACCEPTF accept,
			   GETDESCRIPTIONF getdescription,
			   GETEXTENSIONF getextension,
			   OPENF open,
			   SETDIRECTORYF setdirectory,
			   LOADTILEF loadtile,
			   CLOSEF close) {
    FormatHandlerPtr handler = (FormatHandlerPtr)MYALLOC(sizeof(FormatHandler));
    handler->accept = accept;
    handler->getdescription = getdescription;
    handler->getextension = getextension;
    handler->open = open;
    handler->setdirectory = setdirectory;
    handler->loadtile = loadtile;
    handler->close = close;
    // realloc 
    if (formats == 0) 
	formats = (FormatHandlerPtr*)MYALLOC(sizeof(FormatHandlerPtr));
    else 
	formats = MYREALLOC(formats, (numformats + 1) * sizeof(FormatHandlerPtr));
    formats[numformats] = handler;
    ++numformats;
}

void RegisterPluginHandler(ImagePtr img, const char* dllname) {
    char realname[MAX_PATH];
    HMODULE hmod;
    GetModuleFileName(0, realname, MAX_PATH);
    PathRemoveFileSpec(realname);
    PathAppend(realname, "plugins");
    PathAppend(realname, dllname);
    if (hmod = LoadLibrary(realname)) {
	REGISTERPLUGINF registerPlugin = (REGISTERPLUGINF)GetProcAddress(hmod, "RegisterVlivPlugin");
	if (registerPlugin)
	    registerPlugin(img);
    }
}

void RegisterPlugins(ImagePtr img) {
    char directory[MAX_PATH];
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    GetModuleFileName(0, directory, MAX_PATH);
    PathRemoveFileSpec(directory);
    PathAppend(directory, "plugins\\*.dll");
    hFind = FindFirstFile(directory, &FindFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
	RegisterPluginHandler(img, FindFileData.cFileName);
	while (FindNextFile(hFind, &FindFileData) != 0) 
	    RegisterPluginHandler(img, FindFileData.cFileName);
	FindClose(hFind);
    } 
}

void RegisterFormatHandlers(ImagePtr img) {
    unsigned int idx;
    unsigned int len;
    char tempfilter[1024];
    //    RegisterPluginHandler(img, "vliv.dll");
    //    RegisterPluginHandler(img, "newhandler.dll");
    RegisterPlugins(img);
    // build the filter
    strcpy(customfilter, "All Files|*.*|");
    strcat(customfilter, "Recognized image types|");
    strcpy(tempfilter, "|");
    for (idx = 0; idx < numformats; ++idx) {
	FormatHandlerPtr handler = formats[idx];
	if (idx != 0) {
	    strcat(customfilter, ";");
	    strcat(tempfilter, "|");
	}
	strcat(customfilter, handler->getextension());
	strcat(tempfilter, handler->getdescription());
	strcat(tempfilter, "|");
	strcat(tempfilter, handler->getextension());
    }
    strcat(customfilter, tempfilter);
    len = (unsigned int)strlen(customfilter);
    for (idx = 0; idx < len; ++idx) {
	if (customfilter[idx] == '|')
	    customfilter[idx] = 0;
    }
}

void UnRegisterFormatHandlers() {
    unsigned int idx;
    for (idx = 0; idx < numformats; ++idx) {
	FormatHandlerPtr handler = formats[idx];
	MYFREE(handler);
    }
    MYFREE(formats);
}

