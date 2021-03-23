#ifndef _VLIV_H
#define _VLIV_H

#define _WIN32_WINNT 0x0501
#include <windows.h>

#define VLIV_EXPORT(type) type
#define VLIV_IMPORT(type) type

typedef BOOL (*ACCEPTF)(const unsigned char* buffer, unsigned int size);
typedef const char* (*GETDESCRIPTIONF)();
typedef const char* (*GETEXTENSIONF)();
typedef BOOL (*OPENF)(struct Image* image, const TCHAR* name);
typedef void (*SETDIRECTORYF)(struct Image* image, unsigned int which);
typedef HBITMAP (*LOADTILEF)(struct Image* image, HDC hdc, unsigned int x, unsigned int y);
typedef void (*CLOSEF)(struct Image* image);

typedef void (*REGISTERFORMATF)(ACCEPTF,
				GETDESCRIPTIONF,
				GETEXTENSIONF,
				OPENF,
				SETDIRECTORYF,
				LOADTILEF,
				CLOSEF);

typedef struct {
    ACCEPTF accept;
    GETDESCRIPTIONF getdescription;
    GETEXTENSIONF getextension;
    OPENF open;
    SETDIRECTORYF setdirectory;
    LOADTILEF loadtile;
    CLOSEF close;
    void* internal;
} FormatHandler;

typedef FormatHandler* FormatHandlerPtr;

typedef enum {
    Normal = 0,
    ReducedImage = 1,
    Page = 2,
    Virtual = 3
} SubFileType;

// plugin entry point
typedef void (*REGISTERPLUGINF)(struct Image* img);

// helper for internal Vliv function access
typedef HBITMAP (*VLIVCREATETRUECOLORDIBSECTIONF)(HDC hdc, 
						  int width, 
						  int height, 
						  unsigned int** bits, 
						  unsigned int depth);
typedef HBITMAP (*VLIVCREATEINDEXEDDIBSECTIONF)(HDC hdc, 
						int width, 
						int height, 
						unsigned int** bits, 
						void (*fillcmap)(BITMAPINFO* bmi, unsigned int idx, void* arg),
						void* arg);

typedef HBITMAP (*VLIVCREATEDEFAULTDIBSECTIONF)(HDC hdc,
						int width, 
						int height,
						const char* text,
						unsigned int** bits);

struct FunctionHelper {
    REGISTERFORMATF Register;
    VLIVCREATETRUECOLORDIBSECTIONF CreateTrueColorDIBSection;
    VLIVCREATEINDEXEDDIBSECTIONF CreateIndexedDIBSection;
    VLIVCREATEDEFAULTDIBSECTIONF CreateDefaultDIBSection;
};

struct Image {
    FormatHandlerPtr handler;
    unsigned int width;
    unsigned int height;
    unsigned int twidth;
    unsigned int theight;
    unsigned int numtilesx;
    unsigned int numtilesy;
    unsigned int istiled;
    unsigned int numdirs;
    unsigned int currentdir;
    unsigned int supportmt;
    char name[MAX_PATH];
    SubFileType  subfiletype;
    struct FunctionHelper helper;
};

typedef struct Image* ImagePtr;

typedef struct {
    HBITMAP bitmap;
} Tile;

#define MYALLOC(size) HeapAlloc(GetProcessHeap(), 0, size)
#define MYFREE(data) HeapFree(GetProcessHeap(), 0, data);
#define MYREALLOC(data, size) HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, data, size);

HMODULE languageInst;

VLIV_EXPORT(void) ShowLastError(LPTSTR lpszFunction);



#endif
