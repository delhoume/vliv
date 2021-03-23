#include <lyapunov.h>

#include <stdio.h>

static BOOL AcceptLYAPUNOVImage(const unsigned char* buffer, unsigned int size) { 
  return FALSE; 
}

const char* GetLYAPUNOVDescription() { return "Lyapunov Images"; }
const char* GetLYAPUNOVExtension() { return "*.lya"; }

struct lyapunov_internal {
    double xmin;
    double ymin;
    double xmax;
    double ymax;
    int maxiter;
};

// here we should read the .new file and fill the specific structure
// to be used in tile computing
static BOOL OpenLYAPUNOVImage(ImagePtr img, const TCHAR* name) {
    struct lyapunov_internal* new_internal = 
      (struct lyapunov_internal*)MYALLOC(sizeof(struct lyapunov_internal));
    img->handler->internal = (void*)new_internal;
    new_internal->xmin = 2.0;
    new_internal->ymin = 4.0;
    new_internal->xmax = 4.0;
    new_internal->ymax = 2.0;
    new_internal->maxiter = 75;
    img->numdirs = 10;
    img->supportmt = 1;
    img->currentdir = 0;
    return TRUE;
}

static void SetLYAPUNOVDirectory(ImagePtr img, unsigned int which) {
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

#define CLAMP(x) (((x) > 255) ? 255 : ((x) < 0) ? 0 : (x))
//int seq_length = 16;
//int seq[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1 };
//int seq_length = 5;
//int seq[] = {1, 1, 0, 1, 0};
//int seq_length = 12;
//int seq[] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1};
int seq_length = 2;
int seq[] = {1, 0};
double lambda_min = -2.55;
double lambda_max = 0.3959;

static void ComputeColor(struct lyapunov_internal* internal, unsigned char* pixel, double a, double b) {
  double x = 0.5;
  int m, n;
  double r, sum_log_deriv, prod_deriv, lambda;
  double rgb_f[3];
  for (m = 0; m < seq_length; ++m) {
    r = seq[m] ? b : b;
    x = r * x * (1 - x);
  }
  sum_log_deriv = 0.0;
  for (n = 0; n < internal->maxiter; ++n) {
    prod_deriv = 1.0;
    for (m = 0; m < seq_length; ++m) {
      r = seq[m] ? b : a;
      prod_deriv *= r * (1 - 2 * x); 
      x = r * x * (1 - x);
    }
    sum_log_deriv += (double)log(fabs(prod_deriv));
  }
  lambda = sum_log_deriv / (internal->maxiter * seq_length);
  if (lambda > 0) {
    rgb_f[2] = lambda / lambda_max;
    pixel[0] = (unsigned char)CLAMP(rgb_f[2] * 255);
    pixel[2] = 0;
    pixel[1] = 0;
  } else {
    rgb_f[0] = 1 - (double)pow(lambda/lambda_min, 2/3.0f);
    rgb_f[1] = 1 - (double)pow(lambda/lambda_min, 1/3.0f);
    pixel[0] = 0;
    pixel[2] = (unsigned char)CLAMP(rgb_f[0] * 255);
    pixel[1] = (unsigned char)CLAMP(rgb_f[1] * 255);
  }
}

static HBITMAP
LoadLYAPUNOVTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
    unsigned int* bits = 0;
    HBITMAP hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, -(int)img->theight, &bits, 32);
     if (bits) {
	struct lyapunov_internal* internal = (struct lyapunov_internal*)img->handler->internal;
	double tilew = (internal->xmax - internal->xmin) / (double)img->numtilesx;
	double tilesx = internal->xmin + tilew * x;
	double sx = tilew / (double)img->twidth;
	double tileh = (internal->ymax - internal->ymin) / (double)img->numtilesy;
	double tilesy = internal->ymin + tileh * y;
	double sy = tileh / (double)img->theight;
	unsigned int xpos;
	unsigned int ypos;
	for (ypos = 0; ypos < img->theight; ++ypos) {
	    double yy = tilesy + ypos * sy;
	    unsigned char* row = (unsigned char*)(bits + ypos * img->theight);
	    double xx = tilesx;
	    for (xpos = 0; xpos < img->twidth; ++xpos, xx += sx) {
		ComputeColor(internal, row + 4 * xpos, xx, yy);
	    }
	}
    }
     return hbitmap;
}

static void CloseLYAPUNOVImage(ImagePtr img) {
    if (img->handler && img->handler->internal) {
	struct lyapunov_internal* internal = (struct lyapunov_internal*)img->handler->internal;
	MYFREE(internal);
    }
}

void RegisterVlivPlugin(ImagePtr img) {
    img->helper.Register(AcceptLYAPUNOVImage,
			 GetLYAPUNOVDescription,
			 GetLYAPUNOVExtension,
			 OpenLYAPUNOVImage,
			 SetLYAPUNOVDirectory,
			 LoadLYAPUNOVTile,
			 CloseLYAPUNOVImage);
}

