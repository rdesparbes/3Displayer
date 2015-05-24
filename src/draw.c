#include <math.h>

#include "draw.h"
#include "coord.h"
#include "color.h"
#include "scene.h"
#include "SDL/SDL.h"

static inline int min(int a, int b)
{
    return (a < b) ? a : b;
}

static inline int max(int a, int b)
{
    return (a > b) ? a : b;
}

void setPixel(const Coord *A, const Color *color)
{
    Uint32 pixel = SDL_MapRGB(getScreen()->format,
			      color->r, color->g, color->b);
    *((Uint32 *) (getScreen()->pixels) + A->w + A->h * getScreen()->w) = pixel;
}

void drawPixel(const Coord *A, float depthA, const Color *color)
{
    int i = A->w + A->h * getScreenWidth();
    if (A->h >= 0 && A->h < getScreenHeight() && 
	A->w >= 0 && A->w < getScreenWidth() && 
	(getZBuffer()[i] < 0 || 
	 getZBuffer()[i] > depthA))
	setPixel(A, color);
}

void drawSegment(const Coord *A, const Coord *B,
		 float depthA, float depthB,
		 const Color *color)
{
    Coord M = *A;
    int dx = B->w - A->w;
    int dy = B->h - A->h;
    int xIncr = dx > 0 ? 1 : -1;
    int yIncr = dy > 0 ? 1 : -1;
    float alpha, depthM;
    int error;

    if ((abs(dx) > abs(dy))) {
	error = dx;
	dx = 2 * dx;
	dy = 2 * dy;
	while (M.w != B->w) {
	    M.w += xIncr;
	    alpha = (float) (M.w - A->w) / (B->w - A->w);
	    depthM =
		depthA * depthB / ((1 - alpha) * depthB + alpha * depthA);
	    if ((error -= yIncr * dy) <= 0) {
		M.h += yIncr;
		error += xIncr * dx;
	    }
	    if (M.h >= 0 && M.h < getScreenHeight() && 
		M.w >= 0 && M.w < getScreenWidth() && 
		(getZBuffer()[M.w + M.h * getScreenWidth()] < 0 || 
		 getZBuffer()[M.w + M.h * getScreenWidth()] > depthM)) {
		setPixel(&M, color);
		//setPixel(M, {255 /depthM , 255 /depthM, 255 /depthM});
		//setPixel(M, {255 * alpha, 0, 0});
		getZBuffer()[M.w + M.h * getScreenWidth()] = depthM;
	    }
	}
    } else {
	error = dy;
	dy = 2 * dy;
	dx = 2 * dx;
	while (M.h != B->h) {
	    M.h += yIncr;
	    alpha = (float) (M.h - A->h) / (B->h - A->h);
	    depthM =
		depthA * depthB / ((1 - alpha) * depthB + alpha * depthA);
	    if ((error -= xIncr * dx) <= 0) {
		M.w += xIncr;
		error += yIncr * dy;
	    }
	    if (M.h >= 0 && M.h < getScreenHeight() && 
		M.w >= 0 && M.w < getScreenWidth()
		&& (getZBuffer()[M.w + M.h * getScreenWidth()] < 0
		    || getZBuffer()[M.w + M.h * getScreenWidth()] > depthM)) {
		setPixel(&M, color);
		//setPixel(M, {255 /depthM , 255 /depthM, 255 /depthM});
		//setPixel(M, {255 * alpha, 0, 0});
		getZBuffer()[M.w + M.h * getScreenWidth()] = depthM;
	    }
	}
    }
}

void drawTriangle(const Coord *A, const Coord *B, const Coord *C,
		  float depthA, float depthB, float depthC,
		  SDL_Surface *triangle,
		  const Texture *U, const Texture *V, const Texture *W,
		  const Point *normalA,
		  const Point *normalB,
		  const Point *normalC)
{
    Coord AB, BC;
    diffRect(B, A, &AB);
    diffRect(C, B, &BC);

    if (relativeProduct(&AB, &BC) <= 0)
	return;

    int minW = min(min(A->w, B->w), C->w);
    int maxW = max(max(A->w, B->w), C->w);

    int minH = min(min(A->h, B->h), C->h);
    int maxH = max(max(A->h, B->h), C->h);

    if (minW < 0)
	minW = 0;
    if (maxW >= getScreenWidth())
	maxW = getScreenWidth() - 1;
    if (minH < 0)
	minH = 0;
    if (maxH >= getScreenHeight())
	maxH = getScreenHeight() - 1;

    if (triangle == NULL)
	triangle = SDL_LoadBMP("error.bmp");

    Coord AC;
    diffRect(C, A, &AC);

    Point p;
    sumPoint(normalA, getLight(), &p);
    float scaleA = 1 - normPoint(&p) / 2;
    sumPoint(normalB, getLight(), &p);
    float scaleB = 1 - normPoint(&p) / 2;
    sumPoint(normalC, getLight(), &p);
    float scaleC = 1 - normPoint(&p) / 2;

    float scaleABC = scaleA * scaleB * scaleC;
    
    float scaleAB = scaleA * scaleB;
    float scaleBC = scaleB * scaleC;
    float scaleCA = scaleC * scaleA;

    float depthABC = depthA * depthB * depthC;

    float depthAB = depthA * depthB;
    float depthBC = depthB * depthC;
    float depthCA = depthC * depthA;

    float scale;
    float depthM;

    Texture N, u, v, w;
    setTexture(&u,
	       U->x / depthA,
	       U->y / depthA);
    setTexture(&v,
	       V->x / depthB,
	       V->y / depthB);
    setTexture(&w,
	       W->x / depthC,
	       W->y / depthC);

    int det = relativeProduct(&AB, &AC);
    Uint8 r, g, b;
    Uint32 *pixel;

    Coord M;
    Coord AM, BM, CM;
    for (M.h = minH; M.h <= maxH; ++M.h) {
	M.w = minW;
	diffRect(&M, A, &AM);
	diffRect(&M, B, &BM);
	diffRect(&M, C, &CM);
	while (M.w <= maxW
	       && (relativeProduct(&AB, &AM) < 0
		   || relativeProduct(&BC, &BM) < 0
		   || relativeProduct(&CM, &AC) < 0)) {
	    M.w++;
	    diffRect(&M, A, &AM);
	    diffRect(&M, B, &BM);
	    diffRect(&M, C, &CM);
	}
	while (M.w <= maxW && relativeProduct(&AB, &AM) >= 0
	       && relativeProduct(&BC, &BM) >= 0
	       && relativeProduct(&CM, &AC) >= 0) {
	    float alpha = (float) relativeProduct(&AB, &AM) / det;
	    float beta = (float) relativeProduct(&AM, &AC) / det;
	    float gamma = 1. - alpha - beta;

	    depthM = depthABC / 
		(gamma * depthBC + beta * depthCA + alpha * depthAB);

	    if (getZBuffer()[M.w + M.h * getScreenWidth()] < 0
		|| getZBuffer()[M.w + M.h * getScreenWidth()] > depthM) {
		scale = scaleABC / 
		    (gamma * scaleBC + beta * scaleCA + alpha * scaleAB);
		N.x = (gamma * u.x + beta * v.x +
		       alpha * w.x) / (gamma / depthA +
				       beta / depthB + alpha / depthC);
		N.y = (gamma * u.y + beta * v.y +
		       alpha * w.y) /
		    (gamma / depthA + beta / depthB + alpha / depthC);
		pixel =
		    (Uint32 *) (triangle->pixels +
				(int) (loopTexture(N.x) * triangle->w) *
				triangle->format->BitsPerPixel / 8 +
				(int) (loopTexture(N.y) * triangle->h) *
				(triangle->format->BitsPerPixel *
				 triangle->w / 8 + triangle->w % 4));
		SDL_GetRGB(*pixel, triangle->format, &r, &g, &b);
		Color scaled = {
		    .r = scale * r,
		    .g = scale * g,
		    .b = scale * b
		};
		setPixel(&M, &scaled);
		//setPixel(M, {255 /depthM , 255 /depthM, 255 /depthM});
		//setPixel(M, {255 * N.x, 255 * N.y, 0});
		//setPixel(M, {255 * alpha, 255 * beta, 0});
		getZBuffer()[M.w + M.h * getScreenWidth()] = depthM;
	    }
	    M.w++;
	    diffRect(&M, A, &AM);
	    diffRect(&M, B, &BM);
	    diffRect(&M, C, &CM);
	}
    }
}
