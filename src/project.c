#include <math.h>

#include "project.h"
#include "basic.h"
#include "draw.h"

// return the vector between camera.O and the intersection of
// (AB) and the NEARPLAN
void projectPoint(const Point *A, const Point *B, Point *S)
{
    Point AB, d;
    diffPoint(B, A, &AB);
    diffPoint(A, &camera.O, &d);
    float k =
	(NEARPLAN - scalarProduct(&camera.j, &d))
	/ scalarProduct(&camera.j, &AB);

    setPoint(S,
	     A->x + k * AB.x - camera.O.x,
	     A->y + k * AB.y - camera.O.y,
	     A->z + k * AB.z - camera.O.z);
}

// return the Coord projection of a vector between camera.O
// and A, knowing the depth of A
void projectCoord(const Point *OA, float depth, Coord *S)
{
    static float wCoef = WIDTH / (2. * tan(WFOV * M_PI / 360.));
    static float hCoef = -HEIGHT / (2. * tan(HFOV * M_PI / 360.));
    S->w = wCoef * scalarProduct(&camera.i, OA) / depth + WIDTH / 2;
    S->h = hCoef * scalarProduct(&camera.k, OA) / depth + HEIGHT / 2;
}

void projectVertex(const Point *A, const Color *color)
{
    Point OA;
    diffPoint(A, &camera.O, &OA);
    float depthA = scalarProduct(&camera.j, &OA);

    Coord t;
    projectCoord(&OA, depthA, &t);
    if (depthA > NEARPLAN)
	drawPixel(&t, color);
}

void projectSegment(const Point *A, const Point *B, const Color *color)
{
    Point OA, OB, AB;
    diffPoint(A, &camera.O, &OA);
    diffPoint(B, &camera.O, &OB);
    float depthA = scalarProduct(&camera.j, &OA);
    float depthB = scalarProduct(&camera.j, &OB);

    Coord t, u;
    if (depthA > NEARPLAN && depthB > NEARPLAN) {
	projectCoord(&OA, depthA, &t);
	projectCoord(&OB, depthB, &u);
	drawSegment(&t, &u, depthA, depthB, color);
    } else if (depthA < NEARPLAN && depthB > NEARPLAN) {
	projectPoint(A, B, &AB);
	projectCoord(&AB, NEARPLAN, &t);
	projectCoord(&OB, depthB, &u);
    } else if (depthA > NEARPLAN && depthB < NEARPLAN) {
	projectPoint(A, B, &AB);
	projectCoord(&OA, depthA, &t);
	projectCoord(&AB, NEARPLAN, &u);
    }
    drawSegment(&t, &u, depthA, depthB, color);
}

void projectTriangle(const Point *A, const Point *B, const Point *C,
		     SDL_Surface *triangle,
		     const Texture *U, const Texture *V, const Texture *W,
		     const Point *normalA, const Point *normalB,
		     const Point *normalC)
{
    Point OA, OB, OC;
    diffPoint(A, &camera.O, &OA);
    diffPoint(B, &camera.O, &OB);
    diffPoint(C, &camera.O, &OC);
    float depthA = scalarProduct(&camera.j, &OA);
    float depthB = scalarProduct(&camera.j, &OB);
    float depthC = scalarProduct(&camera.j, &OC);
    int test =
	(depthA > NEARPLAN) + (depthB > NEARPLAN) + (depthC > NEARPLAN);

    if (test == 3) {
	Coord a, b, c;
	projectCoord(&OA, depthA, &a);
	projectCoord(&OB, depthB, &b);
	projectCoord(&OC, depthC, &c);
	drawTriangle(&a, &b, &c,
		     depthA, depthB, depthC,
		     triangle,
		     U, V, W,
		     normalA, normalB, normalC);
    } else if (test == 0) {
	return;
    } else if (test == 1) {
	if (depthA > NEARPLAN) {
	    Point b, c;
	    diffPoint(B, A, &b);
	    diffPoint(C, A, &c);

	    float kB = (NEARPLAN - depthA) / scalarProduct(&camera.j, B);
	    float kC = (NEARPLAN - depthA) / scalarProduct(&camera.j, C);

	    setPoint(&OB, 
		     A->x + kB * b.x - camera.O.x,
		     A->y + kB * b.y - camera.O.y,
		     A->z + kB * b.z - camera.O.z);
	    setPoint(&OC,
		     A->x + kC * c.x - camera.O.x,
		     A->y + kC * c.y - camera.O.y,
		     A->z + kC * c.z - camera.O.z);

	    Coord a, bn, cn;
	    projectCoord(&OA, depthA, &a);
	    projectCoord(&OB, NEARPLAN, &bn);
	    projectCoord(&OC, NEARPLAN, &cn);

	    Texture UV, UW;
	    setTexture(&UV,
		       (V->x - U->x) * kB + U->x,
		       (V->y - U->y) * kB + U->y);
	    setTexture(&UW,
		       (W->x - U->x) * kC + U->x,
		       (W->y - U->y) * kC + U->y);

	    Point nAB, nAC;
	    setPoint(&nAB,
		     (normalB->x - normalA->x) * kB + normalA->x,
		     (normalB->y - normalA->y) * kB + normalA->y,
		     (normalB->z - normalA->z) * kB + normalA->z);
	    setPoint(&nAC,
		     (normalC->x - normalA->x) * kC + normalA->x,
		     (normalC->y - normalA->y) * kC + normalA->y,
		     (normalC->z - normalA->z) * kC + normalA->z);
	    
	    drawTriangle(&a, &bn, &cn,
			 depthA,
			 (depthB - depthA) * kB + depthA,
			 (depthC - depthA) * kC + depthA,
			 triangle,
			 U, &UV, &UW,
			 normalA, &nAB, &nAC);
	} else if (depthB > NEARPLAN) {
	    Point c, a;
	    diffPoint(C, B, &c);
	    diffPoint(A, B, &a);

	    float kC = (NEARPLAN - depthB) / scalarProduct(&camera.j, &c);
	    float kA = (NEARPLAN - depthB) / scalarProduct(&camera.j, &a);

	    setPoint(&OC,
		     B->x + kC * c.x - camera.O.x,
		     B->y + kC * c.y - camera.O.y,
		     B->z + kC * c.z - camera.O.z);
	    setPoint(&OA,
		     B->x + kA * a.x - camera.O.x,
		     B->y + kA * a.y - camera.O.y,
		     B->z + kA * a.z - camera.O.z);

	    Coord b, cn, an;
	    projectCoord(&OB, depthB, &b);
	    projectCoord(&OC, NEARPLAN, &cn);
	    projectCoord(&OA, NEARPLAN, &an);

	    Texture VW, VU;
	    setTexture(&VW,
		       (W->x - V->x) * kC + V->x,
		       (W->y - V->y) * kC + V->y);
	    setTexture(&VU,
		       (U->x - V->x) * kA + V->x,
		       (U->y - V->y) * kA + V->y);

	    Point nBC, nBA;
	    setPoint(&nBC,
		     (normalC->x - normalB->x) * kC + normalB->x,
		     (normalC->y - normalB->y) * kC + normalB->y,
		     (normalC->z - normalB->z) * kC + normalB->z);
	    setPoint(&nBA,
		     (normalA->x - normalB->x) * kA + normalB->x,
		     (normalA->y - normalB->y) * kA + normalB->y,
		     (normalA->z - normalB->z) * kA + normalB->z);
	    
	    drawTriangle(&b, &cn, &an,
			 depthB,
			 (depthC - depthB) * kC + depthB,
			 (depthA - depthB) * kA + depthB,
			 triangle,
			 V, &VW, &VU,
			 normalB, &nBC, &nBA);

	} else if (depthC > NEARPLAN) {
	    Point a,b;
	    diffPoint(A, C, &a);
	    diffPoint(B, C, &b);

	    float kA = (NEARPLAN - depthC) / scalarProduct(&camera.j, &a);
	    float kB = (NEARPLAN - depthC) / scalarProduct(&camera.j, &b);

	    setPoint(&OA,
		     C->x + kA * a.x - camera.O.x,
		     C->y + kA * a.y - camera.O.y,
		     C->z + kA * a.z - camera.O.z);
	    setPoint(&OB,
		     C->x + kB * b.x - camera.O.x,
		     C->y + kB * b.y - camera.O.y,
		     C->z + kB * b.z - camera.O.z);

	    Coord c, an, bn;
	    projectCoord(&OC, depthC, &c);
	    projectCoord(&OA, NEARPLAN, &an);
	    projectCoord(&OB, NEARPLAN, &bn);

	    Texture WU, WV;
	    setTexture(&WU,
		       (U->x - W->x) * kA + W->x,
		       (U->y - W->y) * kA + W->y);
	    setTexture(&WV,
		       (V->x - W->x) * kB + W->x,
		       (V->y - W->y) * kB + W->y);

	    Point nCA, nCB;
	    setPoint(&nCA,
		     (normalA->x - normalC->x) * kA + normalC->x,
		     (normalA->y - normalC->y) * kA + normalC->y,
		     (normalA->z - normalC->z) * kA + normalC->z);
	    setPoint(&nCB,
		     (normalB->x - normalC->x) * kB + normalC->x,
		     (normalB->y - normalC->y) * kB + normalC->y,
		     (normalB->z - normalC->z) * kB + normalC->z);
	    
	    drawTriangle(&c, &an, &bn,
			 depthC,
			 (depthA - depthC) * kA + depthC,
			 (depthB - depthC) * kB + depthC,
			 triangle,
			 W, &WU, &WV,
			 normalC, &nCA, &nCB);
	}
    } else {
	// Coord XY, XZ, Y, Z;
	if (depthA < NEARPLAN) {
	    Point AB, AC;
	    diffPoint(B, A, &AB);
	    diffPoint(C, A, &AC);

	    float kB = (NEARPLAN - depthA) / scalarProduct(&camera.j, &AB);
	    float kC = (NEARPLAN - depthA) / scalarProduct(&camera.j, &AC);

	    Point OpB, OpC;
	    setPoint(&OpB,
		     A->x + kB * AB.x - camera.O.x,
		     A->y + kB * AB.y - camera.O.y,
		     A->z + kB * AB.z - camera.O.z);
	    setPoint(&OpC,
		     A->x + kC * AC.x - camera.O.x,
		     A->y + kC * AC.y - camera.O.y,
		     A->z + kC * AC.z - camera.O.z);

	    Coord opbn, c, opcn;
	    projectCoord(&OpB, NEARPLAN, &opbn);
	    projectCoord(&OC, depthC, &c);
	    projectCoord(&OpC, NEARPLAN, &opcn);

	    Texture VU, WU;
	    setTexture(&VU,
		       (V->x - U->x) * kB + U->x,
		       (V->y - U->y) * kB + U->y);
	    setTexture(&WU,
		       (W->x - U->x) * kC + U->x,
		       (W->y - U->y) * kC + U->y);

	    Point nAB, nAC;
	    setPoint(&nAB,
		     (normalB->x - normalA->x) * kB + normalA->x,
		     (normalB->y - normalA->y) * kB + normalA->y,
		     (normalB->z - normalA->z) * kB + normalA->z);
	    setPoint(&nAC,
		     (normalC->x - normalA->x) * kC + normalA->x,
		     (normalC->y - normalA->y) * kC + normalA->y,
		     (normalC->z - normalA->z) * kC + normalA->z);
	    
	    drawTriangle(&opbn, &c, &opcn,
			 (depthB - depthA) * kB + depthA,
			 depthC,
			 (depthC - depthA) * kC + depthA,
			 triangle,
			 &VU, W, &WU,
			 &nAB, normalC, &nAC);

	    Coord b;
	    projectCoord(&OB, depthB, &b);
	    drawTriangle(&opbn, &b, &c,
			 (depthB - depthA) * kB + depthA,
			 depthB, depthC,
			 triangle,
			 &VU, V, W,
			 &nAB, normalB, normalC);
	} else if (depthB < NEARPLAN) {
	    Point BC, BA;
	    diffPoint(C, B, &BC);
	    diffPoint(A, B, &BA);

	    float kC = (NEARPLAN - depthB) / scalarProduct(&camera.j, &BC);
	    float kA = (NEARPLAN - depthB) / scalarProduct(&camera.j, &BA);

	    Point OpC, OpA;
	    setPoint(&OpC,
		     B->x + kC * BC.x - camera.O.x,
		     B->y + kC * BC.y - camera.O.y,
		     B->z + kC * BC.z - camera.O.z);
	    setPoint(&OpA,
		     B->x + kA * BA.x - camera.O.x,
		     B->y + kA * BA.y - camera.O.y,
		     B->z + kA * BA.z - camera.O.z);

	    Coord opcn, a, opan;
	    projectCoord(&OpC, NEARPLAN, &opcn);
	    projectCoord(&OA, depthA, &a);
	    projectCoord(&OpA, NEARPLAN, &opan);
	    Texture VW, VU;
	    setTexture(&VW,
		       (W->x - V->x) * kC + V->x,
		       (W->y - V->y) * kC + V->y);
	    setTexture(&VU,
		       (U->x - V->x) * kA + V->x,
		       (U->y - V->y) * kA + V->y);

	    Point nBC, nBA;
	    setPoint(&nBC,
		     (normalC->x - normalB->x) * kC + normalB->x,
		     (normalC->y - normalB->y) * kC + normalB->y,
		     (normalC->z - normalB->z) * kC + normalB->z);
	    setPoint(&nBA,
		     (normalA->x - normalB->x) * kA + normalB->x,
		     (normalA->y - normalB->y) * kA + normalB->y,
		     (normalA->z - normalB->z) * kA + normalB->z);
	    drawTriangle(&opcn, &a, &opan,
			 (depthC - depthB) * kC + depthB,
			 depthA,
			 (depthA - depthB) * kA + depthB,
			 triangle,
			 &VW, U, &VU,
			 &nBC, normalA, &nBA);

	    Coord c;
	    projectCoord(&OC, depthC, &c);
	    drawTriangle(&opcn, &c, &a,
			 (depthC - depthB) * kC + depthB,
			 depthC, depthA,
			 triangle,
			 &VW, W, U,
			 &nBC, normalC, normalA);
	} else if (depthC < NEARPLAN) {
	    Point CA, CB;
	    diffPoint(A, C, &CA);
	    diffPoint(B, C, &CB);

	    float kA = (NEARPLAN - depthC) / scalarProduct(&camera.j, &CA);
	    float kB = (NEARPLAN - depthC) / scalarProduct(&camera.j, &CB);

	    Point OpA, OpB;
	    setPoint(&OpA,
		     C->x + kA * CA.x - camera.O.x,
		     C->y + kA * CA.y - camera.O.y,
		     C->z + kA * CA.z - camera.O.z);
	    setPoint(&OpB,
		     C->x + kB * CB.x - camera.O.x,
		     C->y + kB * CB.y - camera.O.y,
		     C->z + kB * CB.z - camera.O.z);

	    Coord opan, b, opbn;
	    projectCoord(&OpA, NEARPLAN, &opan);
	    projectCoord(&OB, depthB, &b);
	    projectCoord(&OpB, NEARPLAN, &opbn);

	    Texture WU, WV;
	    setTexture(&WU,
		       (U->x - W->x) * kA + W->x,
		       (U->y - W->y) * kA + W->y);
	    setTexture(&WV,
		       (V->x - W->x) * kB + W->x,
		       (V->y - W->y) * kB + W->y);

	    Point nCA, nCB;
	    setPoint(&nCA,
		     (normalA->x - normalC->x) * kA + normalC->x,
		     (normalA->y - normalC->y) * kA + normalC->y,
		     (normalA->z - normalC->z) * kA + normalC->z);
	    setPoint(&nCB,
		     (normalB->x - normalC->x) * kB + normalC->x,
		     (normalB->y - normalC->y) * kB + normalC->y,
		     (normalB->z - normalC->z) * kB + normalC->z);
	    drawTriangle(&opan, &b, &opan,
			 (depthA - depthC) * kA + depthC,
			 depthB,
			 (depthB - depthC) * kB + depthC,
			 triangle,
			 &WU, V, &WV,
			 &nCA, normalB, &nCB);

	    Coord a;
	    projectCoord(&OA, depthA, &a);
	    drawTriangle(&opan, &a, &b,
			 (depthA - depthC) * kA + depthC,
			 depthA, depthB,
			 triangle,
			 &WU, U, V,
			 &nCA, normalA, normalB);
	}
    }
}
