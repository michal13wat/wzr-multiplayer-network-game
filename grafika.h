#include <gl\gl.h>
#include <gl\glu.h>
#ifndef _WEKTOR__H
  #include "wektor.h"
#endif

enum GLDisplayListNames
{
	Wall1 = 1,
	Wall2 = 2,
	Floor = 3,
	Cube = 4,
  Auto = 5,
	PowierzchniaTerenu = 10
};


int InicjujGrafike(HDC g_context);
void RysujScene();
void ZmianaRozmiaruOkna(int cx,int cy);
Wektor3 WspolrzedneKursora3D(int x, int y);
void WspolrzedneEkranu(float *xx, float *yy, float *zz, Wektor3 Punkt3D);
void ZakonczenieGrafiki();

BOOL SetWindowPixelFormat(HDC hDC);
BOOL CreateViewGLContext(HDC hDC);
GLvoid BuildFont(HDC hDC);
GLvoid glPrint(const char *fmt, ...);