#include <bitmap.h>

HBITMAP VlivCreateDefaultDIBSection(HDC hdc, int width, int height, const char* text, unsigned int** bits) {
    BITMAPINFO bmi;
    HBITMAP hbitmap;
    HDC hdctemp;
    RECT rect;
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = width * height * 4;
    hbitmap = CreateDIBSection(hdc, 
			       (const BITMAPINFO*)&bmi, 
			       DIB_RGB_COLORS, 
			       (void**)bits, 
			       0, 
			       0);

    memset((char*)*bits, 0, 4 * width * height);
    hdctemp = CreateCompatibleDC(hdc);
    SelectObject(hdctemp, hbitmap);
    SetRect(&rect, 0, 0, width, height);
    DrawText(hdctemp, text, lstrlen(text), &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DeleteDC(hdctemp);
    return hbitmap;
}

HBITMAP VlivCreateTrueColorDIBSection(HDC hdc, int width, int height, 
					     unsigned int** bits, unsigned int format) {
    BITMAPINFO bmi;
    HBITMAP hbitmap;
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = format;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = width * height * 3;
    hbitmap = CreateDIBSection(hdc, 
			       (const BITMAPINFO*)&bmi, 
			       DIB_RGB_COLORS, 
			       (void**)bits, 
			       0, 
			       0);
    if (hbitmap == 0) {
	ShowLastError(TEXT("VlivCreateTrueColorDIBSection"));
    }
    return hbitmap;
}

static void FillGrey(BITMAPINFO* bmi, unsigned int idx, void* arg) {
    bmi->bmiColors[idx].rgbBlue = idx;
    bmi->bmiColors[idx].rgbGreen = idx;
    bmi->bmiColors[idx].rgbRed = idx;
}

HBITMAP VlivCreateIndexedDIBSection(HDC hdc, int width, int height, unsigned int** bits, 
					   void (*fillcmap)(BITMAPINFO* bmi, unsigned int idx, void* arg), 
					   void* arg) {
    unsigned int bmisize = sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
    unsigned int i;
    HBITMAP hbitmap;
    BITMAPINFO* bmi = (BITMAPINFO*)MYALLOC(bmisize);
    ZeroMemory(bmi, bmisize);
    bmi->bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth       = width;
    bmi->bmiHeader.biHeight      = height;
    bmi->bmiHeader.biPlanes      = 1;
    bmi->bmiHeader.biBitCount    = 8;
    bmi->bmiHeader.biCompression = BI_RGB;
    bmi->bmiHeader.biClrUsed = 256;
    if (fillcmap) {
	for (i = 0; i < 256; ++i) 
	    fillcmap(bmi, i, arg);
    } else {
	for (i = 0; i < 256; ++i) 
	    FillGrey(bmi, i, 0);
    }
    hbitmap =  CreateDIBSection(hdc, 
				(const BITMAPINFO*)bmi, 
				DIB_RGB_COLORS, 
				(void**)bits, 
				0, 
				0);
    MYFREE(bmi);
    return hbitmap;
}

HBITMAP VlivCreateMapIndexedDIBSection(HDC hdc, HANDLE hsection, int width, int height, unsigned int** bits, 
				       void (*fillcmap)(BITMAPINFO* bmi, unsigned int idx, void* arg), 
				       void* arg) {
    unsigned int bmisize = sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
    unsigned int i;
    HBITMAP hbitmap;
    BITMAPINFO* bmi = (BITMAPINFO*)MYALLOC(bmisize);
    ZeroMemory(bmi, bmisize);
    bmi->bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth       = width;
    bmi->bmiHeader.biHeight      = height;
    bmi->bmiHeader.biPlanes      = 1;
    bmi->bmiHeader.biBitCount    = 8;
    bmi->bmiHeader.biCompression = BI_RGB;
    bmi->bmiHeader.biClrUsed = 256;
    for (i = 0; i < 256; ++i) {
	if (fillcmap) 
	    fillcmap(bmi, i, arg);
	else 
	    FillGrey(bmi, i, 0);
    }
    hbitmap =  CreateDIBSection(hdc, 
				(const BITMAPINFO*)bmi, 
				DIB_RGB_COLORS, 
				(void**)bits, 
				hsection, 
				0);
    if (hbitmap == 0) {
	
    } 
    MYFREE(bmi);
    return hbitmap;
}




