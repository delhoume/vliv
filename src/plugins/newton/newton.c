#include <newton.h>

#include <stdio.h>

static BOOL AcceptNEWTONImage(const unsigned char* buffer, unsigned int size) { return FALSE; }
const char* GetNEWTONDescription() { return "Newton Images"; }
const char* GetNEWTONExtension() { return "*.new"; }

struct newton_internal {
    float xmin;
    float ymin;
    float xmax;
    float ymax;
    float tolerance;
    float tolerancelog;
    unsigned int maxiter;
    BOOL shaded;
};

// here we should read the .new file and fill the specific structure
// to be used in tile computing
static BOOL OpenNEWTONImage(ImagePtr img, const TCHAR* name) {
    struct newton_internal* new_internal = (struct newton_internal*)MYALLOC(sizeof(struct newton_internal));
    img->handler->internal = (void*)new_internal;
    new_internal->xmin = -2.0;
    new_internal->ymin = -2.0;
    new_internal->xmax = 2.0;
    new_internal->ymax = 2.0;
    new_internal->tolerance = .001f;
    new_internal->tolerancelog = (float)log(new_internal->tolerance);
    new_internal->maxiter = 15;
    new_internal->shaded = FALSE;
    img->numdirs = 14;
    img->supportmt = 1;
    img->currentdir = 0;
    return TRUE;
}

static void SetNEWTONDirectory(ImagePtr img, unsigned int which) {
    unsigned int size = 512;
    unsigned int idx;
    for (idx = 0; idx < which; ++idx)
	size *= 2;
    img->width = size;
    img->height  = size;
    img->theight = 128;
    img->twidth = 128;
    img->numtilesx = img->width / img->twidth;
    img->numtilesy = img->height / img->theight;
    img->istiled = TRUE;
    img->subfiletype = Virtual;
}

typedef struct {
    unsigned char B;
    unsigned char G;
    unsigned char R;
    unsigned char A;
} BGRA;

typedef struct {
    float r;
    float i;
} Complex;

static Complex  cadd(Complex a, Complex b) {
    Complex ret;
    ret.r = a.r + b.r;
    ret.i = a.i + b.i;
    return ret;
}

static Complex csub(Complex a, Complex b) {
    Complex ret;
    ret.r = a.r - b.r;
    ret.i = a.i - b.i;
    return ret;
}

static Complex csubone(Complex a) {
    Complex ret;
    ret.r = a.r - 1.0f;
    ret.i = a.i;
    return ret;
}

static Complex cmul(Complex a, Complex b) {
    Complex ret;
    ret.r = a.r * b.r - a.i * b.i;
    ret.i = a.i * b.r + a.r * b.i;
    return ret;
}

static Complex cmultwo(Complex a) {
    Complex ret;
    ret.r = a.r * 2.0f;
    ret.i = a.i * 2.0f;
    return ret;
}

static Complex cmulthree(Complex a) {
    Complex ret;
    ret.r = a.r * 3.0f;
    ret.i = a.i * 3.0f;
    return ret;
}

static Complex cdiv(Complex a, Complex b) {
    Complex ret;
    float divisor = b.r * b.r + b.i * b.i;
    ret.r = (a.r * b.r + a.i * b.i) / divisor;
    ret.i = (a.i * b.r - a.r * b.i) / divisor;
    return ret;
}

static float cdistsquared(Complex a, Complex b) {
    float dr, di;
    dr = a.r - b.r;
    di = a.i - b.i;
    return dr * dr + di * di;
}

//#define STDFORMULA

#if defined(STDFORMULA)
static BGRA colors[] = {
    { 255, 0, 0, 0 },
    { 0, 255, 0, 0 },
    { 0, 0, 255, 0 },
    { 0, 0, 0, 0 }
};
#else
static BGRA colors[] = {
    { 73, 255, 0, 0 },
    { 36, 255, 255, 0 },
    { 120, 120, 73, 0 },
    { 0, 0, 0, 0 }
};
#endif

#if defined(STDFORMULA)
static Complex roots[3] = {
    { 1.0f, 0.0f },
    { -0.5f, 0.86603f },
    { -0.5f, -0.86603f}
};
#else
static Complex roots[3] = {
    { -1.7693f, 0.0f },
    { 0.884646f, 0.589743f },
    { 0.884646f, -0.589743f}
};
#endif

static Complex one = { 1.0f, 0.0f };
static Complex two = { 2.0f, 0.0f };
static Complex three = { 3.0f, 0.0f };

static Complex Iterate(Complex c) {
    Complex sq = cmul(c, c);
    Complex cu = cmul(sq, c);
#if defined(STDFORMULA)
#if 0
    Complex top = csub(cu, one);
    Complex bot = cmul(sq, three);
#else
    // is it really optimized like this ??
    Complex top = csubone(cu);
    Complex bot = cmulthree(sq);
#endif
#else
    // new formula z^3 -2z +2
    Complex top = cadd(csub(cu, cmultwo(c)), two);
    Complex bot = csub(cmulthree(sq), two);
#endif
    Complex frac = cdiv(top, bot);
    Complex res = csub(c, frac);
    return res;
}

static void ComputeColor(struct newton_internal* newinternal, unsigned char* pixel, float x, float y) {
    unsigned int i, j;
    Complex c;
    Complex prev;
    float dist1;
    BGRA* color = (BGRA*)pixel;
    float tolerance = newinternal->tolerance;
    c.r = x;
    c.i = y;
    for (i = 0; i < newinternal->maxiter; ++i) {
	prev = c;
	c = Iterate(c);
	for (j = 0; j < 3; ++j) {
	    if ((dist1 = cdistsquared(c, roots[j])) <= tolerance) {
		float dist0 = cdistsquared(prev, roots[j]);
		float proportion;
		if (newinternal->shaded && dist1 < tolerance && dist0 > tolerance) {
		    float log0 = (float)log(dist0);
		    float log1 = (float)log(dist1);
		    proportion = (newinternal->tolerancelog - log0) / (log1 - log0);
		} else 
		    proportion = 0.0;
		proportion = 1.0f - ((i + proportion) / (float)newinternal->maxiter);
		// precompute color range would go faster ?
		color->B = (unsigned char)(colors[j].B * proportion);
		color->G = (unsigned char)(colors[j].G * proportion);
		color->R = (unsigned char)(colors[j].R * proportion);
		return;
	    }
	}
    }
    // no root found -> uniform color
    memcpy(pixel, &colors[3], 4);
}

static HBITMAP
LoadNEWTONTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
    unsigned int* bits = 0;
    HBITMAP hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, -(int)img->theight, &bits, 32);
     if (bits) {
	struct newton_internal* newinternal = (struct newton_internal*)img->handler->internal;
	float tilew = (newinternal->xmax - newinternal->xmin) / (float)img->numtilesx;
	float tilesx = newinternal->xmin + tilew * x;
	float sx = tilew / (float)img->twidth;
	float tileh = (newinternal->ymax - newinternal->ymin) / (float)img->numtilesy;
	float tilesy = newinternal->ymin + tileh * y;
	float sy = tileh / (float)img->theight;
	unsigned int xpos;
	unsigned int ypos;
	for (ypos = 0; ypos < img->theight; ++ypos) {
	    float yy = tilesy + ypos * sy;
	    unsigned char* row = (unsigned char*)(bits + ypos * img->theight);
	    float xx = tilesx;
	    for (xpos = 0; xpos < img->twidth; ++xpos, xx += sx) {
		ComputeColor(newinternal, row + 4 * xpos, xx, yy);
	    }
	}
    }
     return hbitmap;
}

static void CloseNEWTONImage(ImagePtr img) {
    if (img->handler && img->handler->internal) {
	struct newton_internal* new_internal = (struct newton_internal*)img->handler->internal;
	MYFREE(new_internal);
    }
}

void RegisterVlivPlugin(ImagePtr img) {
    img->helper.Register(AcceptNEWTONImage,
			 GetNEWTONDescription,
			 GetNEWTONExtension,
			 OpenNEWTONImage,
			 SetNEWTONDirectory,
			 LoadNEWTONTile,
			 CloseNEWTONImage);
}

