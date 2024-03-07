#include <vliv.h>
#include <tilemgr.h>

typedef struct {
    Tile** tiles;
    unsigned int ntilesx;
    unsigned int ntilesy;
} TileManager;

static TileManager tilemanager = { 0, 0, 0 };
static unsigned int debug = 0;

void 
FreeRow(unsigned int y) {
    if ((tilemanager.tiles != 0) && (tilemanager.tiles[y] != 0)) {
	unsigned int x;
	for (x = 0; x < tilemanager.ntilesx; ++x)
	    FreeTile(x, y);
	MYFREE(tilemanager.tiles[y]);
	tilemanager.tiles[y] = 0;
    }
}

void
FreeTileManager() {
    if (tilemanager.tiles != 0) {
	unsigned int y;
	for (y = 0; y < tilemanager.ntilesy; ++y) 
	    FreeRow(y);
	MYFREE(tilemanager.tiles);
	tilemanager.tiles = 0;
    }
}

void 
InitTileManager(unsigned int x, unsigned int y) {
    unsigned int idx;
    FreeTileManager();
    tilemanager.ntilesx = x;
    tilemanager.ntilesy = y;
    tilemanager.tiles = (Tile**)MYALLOC(y * sizeof(Tile*));
    for (idx = 0; idx < tilemanager.ntilesy; ++idx)
	tilemanager.tiles[idx] = 0;
}

void
CreateRow(unsigned int y) {
    if (tilemanager.tiles[y] == 0) {
	unsigned int idx = 0;
	Tile* tiles = (Tile*)MYALLOC(tilemanager.ntilesx * sizeof(Tile));
	tilemanager.tiles[y] = tiles;
	for (idx = 0; idx < tilemanager.ntilesx; ++idx) 
	    tiles[idx].bitmap = 0;
    }
}

Tile*
GetTileAtPos(unsigned int x, unsigned int y) {
    if (tilemanager.tiles != 0) {
	Tile* rowtiles = tilemanager.tiles[y];
	if (rowtiles) 
	    return rowtiles + x;
    }
    return 0;
}

BOOL 
IsTileLoaded(unsigned int x, unsigned int y) {
    Tile* tile = GetTileAtPos(x, y);
    if (tile)
	    return tile->bitmap != 0;
    else
	    return FALSE;
}

extern DWORD _debug;

void
SetTileBitmap(unsigned int x, unsigned int y, HBITMAP bitmap) {
    Tile* tile = GetTileAtPos(x, y);
    if (tile) {
	tile->bitmap = bitmap;  
    if (_debug) {
			char message[128];
            HDC dc = GetDC(NULL);
            BITMAP bm;
            RECT rect;
            HDC hdcmem = CreateCompatibleDC(dc);
            SelectObject(hdcmem, bitmap);   
            GetObject(bitmap, sizeof(BITMAP), &bm);          
            SelectObject(hdcmem, GetStockObject(WHITE_PEN));
            SelectObject(hdcmem, GetStockObject(HOLLOW_BRUSH));

            SetRect(&rect, 0, 0, bm.bmWidth, bm.bmHeight < 0 ? -bm.bmHeight : bm.bmHeight);
			Rectangle(hdcmem, 0, 0, rect.right - rect.left, rect.bottom - rect.top);
            wsprintf(message, "%dx%d", x * bm.bmWidth, y * (rect.bottom - rect.top));
			DrawText(hdcmem, message, (int)strlen(message), &rect, DT_LEFT | DT_TOP | DT_SINGLELINE);
            wsprintf(message, "%dx%d", x, y);
			DrawText(hdcmem, message, (int)strlen(message), &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            DeleteDC(hdcmem);
	   	 }
    }
}

void 
FreeTile(unsigned int x, unsigned int y) {
    Tile* tile = GetTileAtPos(x, y);
    if (tile) {
	if (tile->bitmap) {
	    DeleteObject(tile->bitmap);
	    tile->bitmap = 0;
	}
    }
}
