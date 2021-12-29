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
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    hbitmap = CreateDIBSection(hdc, 
			       (const BITMAPINFO*)&bmi, 
			       DIB_RGB_COLORS, 
			       (void**)bits, 
			       0, 
			       0);

    hdctemp = CreateCompatibleDC(hdc);
    SelectObject(hdctemp, hbitmap);
    SetRect(&rect, 0, 0, width, height);
    TRIVERTEX vertex[2] ;
    vertex[0].x     = 0;
    vertex[0].y     = 0;
    vertex[0].Red   = 0x0000;
    vertex[0].Green = 0x8000;
    vertex[0].Blue  = 0x8000;
    vertex[0].Alpha = 0x0000;

    vertex[1].x     = width;
    vertex[1].y     = height; 
    vertex[1].Red   = 0x0000;
    vertex[1].Green = 0xd000;
    vertex[1].Blue  = 0xd000;
    vertex[1].Alpha = 0x0000;

     GRADIENT_RECT gRect;
    gRect.UpperLeft  = 0;
    gRect.LowerRight = 1;

    GradientFill(hdctemp, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_H);
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
    bmi.bmiHeader.biSizeImage = 0;
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



