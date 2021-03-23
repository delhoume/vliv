#ifndef _TILEMGR_H
#define _TILEMGR_H

#include <windows.h>
#include <vliv.h>


VLIV_EXPORT(void) InitTileManager(unsigned int x, unsigned int y);
VLIV_EXPORT(void) FreeTileManager();
VLIV_EXPORT(void) FreeRow(unsigned int y);
VLIV_EXPORT(void) CreateRow(unsigned int y);
VLIV_EXPORT(void) SetTileBitmap(unsigned int x, unsigned int y, HBITMAP bitmap);
VLIV_EXPORT(BOOL) IsTileLoaded(unsigned int x, unsigned int y);
VLIV_EXPORT(Tile*) GetTileAtPos(unsigned int x, unsigned int y);
VLIV_EXPORT(void) FreeTile(unsigned int x, unsigned int y);

#endif
