#include <vliv.h>
#include <tilemgr.h>

typedef struct {
    Tile** tiles;
    unsigned int ntilesx;
    unsigned int ntilesy;
} TileManager;

static TileManager tilemanager = { 0, 0, 0 };

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

void
SetTileBitmap(unsigned int x, unsigned int y, HBITMAP bitmap) {
    Tile* tile = GetTileAtPos(x, y);
    if (tile) 
	tile->bitmap = bitmap;
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
