#include <debug.h>

#include <strsafe.h>

static BOOL AcceptDEBUGImage(const unsigned char* buffer, unsigned int size) { return FALSE; }
const char* GetDEBUGDescription() { return "Debug Images"; }
const char* GetDEBUGExtension() { return "*.dbg"; }

static BOOL OpenDEBUGImage(ImagePtr img, const TCHAR* name) {
    img->numdirs = 10;
    img->supportmt = 1;
    img->currentdir = 0;
    return TRUE;
}

static void SetDEBUGDirectory(ImagePtr img, unsigned int which) {
    unsigned int size = 512;
    unsigned int idx;
    for (idx = 0; idx < which; ++idx)
	size *= 2;
    img->width = size;
    img->height  = size;
    img->theight = 256;
    img->twidth = 256;
    img->numtilesx = img->width / img->twidth;
    img->numtilesy = img->height / img->theight;
    img->istiled = TRUE;
    img->subfiletype = Virtual;
}

static HBITMAP
LoadDEBUGTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
    unsigned int* bits = 0;
    char text[MAX_PATH];
    StringCchPrintf(text, MAX_PATH, "Level %d : %dx%d", img->currentdir + 1, x, y); 
    return img->helper.CreateDefaultDIBSection(hdc, img->twidth, img->theight, text, &bits);
}

static void CloseDEBUGImage(ImagePtr img) {
}

void RegisterVlivPlugin(ImagePtr img) {
    img->helper.Register(AcceptDEBUGImage,
			 GetDEBUGDescription,
			 GetDEBUGExtension,
			 OpenDEBUGImage,
			 SetDEBUGDirectory,
			 LoadDEBUGTile,
			 CloseDEBUGImage);
}

