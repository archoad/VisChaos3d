/*visFractal3d
Copyright (C) 2013 Michel Dubois

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.*/


// tutorial openGL http://www-evasion.imag.fr/Membres/Antoine.Bouthors/teaching/opengl/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <png.h>

#include <GL/freeglut.h>

#define WINDOW_TITLE_PREFIX "visFractal3d"
#define MAX_SIDE_LENGTH 2000
#define MAX_RADIUS 15
#define ITERATIONS MAX_SIDE_LENGTH*MAX_SIDE_LENGTH
#define couleur(param) printf("\033[%sm",param)
#define N_ELEMENTS(array) (sizeof(array)/sizeof((array)[0]))

static short winSizeW = 920,
	winSizeH = 690,
	frame = 0,
	currentTime = 0,
	timebase = 0,
	fullScreen = 0,
	rotate = 0, rotColor = 0, rotJulia = 0,
	saturation = 1,
	mandelbulb = 0, mandelbrot = 0, julia = 0, menger = 0,
	sierpinski = 0, flame = 0, flame3d = 0, ifs = 0, planet = 0,
	mengerDepth = 0, sierpinskiDepth = 0,
	numOfFlameFunct = 0,
	blurRadius = 0, nbrIfs = 0,
	dt = 20; // in milliseconds

static int textList = 0,
	objectList = 0,
	cpt = 0,
	background = 0,
	maxIter = 0,
	mengerIter = 0, sierpinskiIter = 0,
	sideLength = 0,
	winPosx = 20,
	winPosy = 20;

static float fps = 0.0,
	sphereRadius = 0.0,
	rotx = -80.0,
	roty = 0.0,
	rotz = 15.0,
	xx = 0.0,
	yy = 0.0,
	zoom = 80.0,
	prevx = 0.0,
	prevy = 0.0;

static double pas=0.0,
	xmin=0.0, xmax=0.0,
	ymin=0.0, ymax=0.0,
	cx=0.0, cy=0.0,
	power=0.0,
	cr=0.0, ci=0.0,
	juliaA=0.0,
	juliaR=0.7885,
	pi=3.14159265359;

typedef struct _vector {
	float x, y, z;
} vector;

typedef struct _object {
	vector pos;
	vector color;
	float size;
} object;

typedef struct _point {
	GLfloat x, y, z;
	GLfloat r, g, b;
	double rx, ry, rz, rf;
	int counter;
} point;

typedef struct _flamepoint {
	double h, s, v;
	int counter;
} flamepoint;

typedef struct _flameFunction {
	int variation;
	double weight;
	double ac, bc, cc, dc, ec, fc;
	double h, s, v;
} flameFunction;


static float *verticesList = NULL;
static float *colorsList = NULL;
static point *pointsList = NULL;
static object *cubeList = NULL;
static object *pyramidList = NULL;
static flameFunction *flameFuncList = NULL;
static flamepoint flamePointList[MAX_SIDE_LENGTH][MAX_SIDE_LENGTH];
static double kernel[MAX_RADIUS][MAX_RADIUS];

static unsigned long iterations = 0;

static GLuint textureID;

static int mengerRemovals[7] = {4, 10, 12, 13, 14, 16, 22};

static char flameName[14];


void textMandel(void) {
	//source: http://paulbourke.net/fractals/
	int k=1;
	float i, j, r, x, y=-16;
	while(puts(""), y++<15)
	for(x=0; x++<84; putchar(" abcdefghijklmno"[k&15]))
	for(i=k=r=0; j=r*r-i*i-2+x/25, i=2*r*i+y/10, j*j+i*i<11&&k++<111; r=j);
}


void usage(void) {
	couleur("31");
	printf("Michel Dubois -- visFractal3d -- (c) 2013\n\n");
	couleur("0");
	printf("Syntaxe: visFractal3d <fractal> <background color>\n");
	printf("\t<fractal> -> 'mandelbrot', 'julia', 'menger', 'sierpinski', 'flame', 'flame3d', 'ifs', 'planet' or 'mandelbulb'\n");
	printf("\t<background color> -> 'white' or 'black'\n");
}


void help(void) {
	textMandel();
	couleur("31");
	printf("Michel Dubois -- gravity3d -- (c) 2013\n\n");
	couleur("0");
	printf("Key usage:\n");
	printf("\t'ESC' key to quit\n");
	printf("\t'BackSpace' to reinitialize\n");
	printf("\t'UP', 'DOWN', 'LEFT' and 'RIGHT' keys to rotate manually\n");
	printf("\t'r' to rotate continuously\n");
	printf("\t'x' and 'X' to move to right and left\n");
	printf("\t'y' and 'Y' to move to top and bottom\n");
	printf("\t'z' and 'Z' to zoom in or out\n");
	printf("\t'f' to switch to full screen\n");
	printf("\t'p' to take a screenshot\n");
	printf("\t'c' to rotate color\n");
	printf("\t's' to switch to black & white\n");
	printf("\t'a' while displaying menger fractal, add steps\n");
	printf("\t'b' while displaying flame fractal, increase blur radius\n");
	printf("\t'q' for julia set rotate variables and for flame fractal change variation\n");
	printf("Mouse usage:\n");
	printf("\t'LEFT CLICK' to zoom into the fractal\n");
	printf("\n");
}


double genDoubleRandom(double min, double max) {
	double t = 0.0;
	t = (max-min) * ((double)rand()/(double)(RAND_MAX)) + min;
	return(t);
}


vector rgb2hsv(float r, float g, float b) {
	float cmin, cmax, delta;
	vector out;
	cmax = fmaxf(r, fmaxf(g, b));
	cmin = fminf(r, fminf(g, b));
	out.z = cmax;
	delta = cmax - cmin;
	if (delta < 0.00001) { out.y = 0; out.x = 0; return(out); }
	if ( cmax>0.0 ) { out.y = delta / cmax; } else { out.y = 0; out.x = -1; return(out); }
	if ( r>=cmax ) { out.x=(g - b) / delta; }
	else if( g>=cmax ) { out.x = 2.0 + (b - r) / delta; }
	else { out.x = 4.0 + (r - g) / delta; }
	out.x = fmod( (out.x*60.0), 360.0);
	out.x = out.x / 360.0;
	return(out);
}


void hsv2rgb(float h, float s, float v, GLfloat *r, GLfloat *g, GLfloat *b) {
	float hp = h * 6;
	if ( hp == 6 ) hp = 0;
	int i = floor(hp);
	float v1 = v * (1 - s),
		v2 = v * (1 - s * (hp - i)),
		v3 = v * (1 - s * (1 - (hp - i)));
	if (i == 0) { *r=v; *g=v3; *b=v1; }
	else if (i == 1) { *r=v2; *g=v; *b=v1; }
	else if (i == 2) { *r=v1; *g=v; *b=v3; }
	else if (i == 3) { *r=v1; *g=v2; *b=v; }
	else if (i == 4) { *r=v3; *g=v1; *b=v; }
	else { *r=v; *g=v1; *b=v2; }
}


void displayKernel(int radius) {
	int x=0, y=0, r=(radius*2)+1;
	for (x=0; x<r; ++x) {
		for (y=0; y<r; ++y) {
			printf("%.8f ", kernel[x][y]);
		}
		printf("\n");
	}
}


void computeGaussianKernel(int radius) {
	double sigma=1.0, sum=0.0;
	int x=0, y=0, tx=0, ty=0, r=(radius*2)+1;

	// compute the kernel
	for (x=0; x<r; ++x) {
		for (y=0; y<r; ++y) {
			tx = x - radius; ty = y - radius;
			kernel[x][y] = exp(-0.5 * (tx*tx + ty*ty) / (sigma*sigma) ) / (2.0 * M_PI * sigma * sigma);
			sum += kernel[x][y];
		}
	}

	// Normalize the kernel
	for (x=0; x<r; ++x) {
		for (y=0; y<r; ++y) {
			kernel[x][y] /= sum;
		}
	}
}


void computeLowPassKernel(int radius) {
	double factor=1.0/9.0, sum=0.0;
	int x=0, y=0, tx=0, ty=0, r=(radius*2)+1;

	// compute the kernel
	for (x=0; x<r; ++x) {
		for (y=0; y<r; ++y) {
			tx = x - radius; ty = y - radius;
			kernel[x][y] = factor;
			sum += kernel[x][y];
		}
	}

	// Normalize the kernel
	for (x=0; x<r; ++x) {
		for (y=0; y<r; ++y) {
			kernel[x][y] /= sum;
		}
	}
}


void generateBlur(int radius) {
	static vector matrixPoint[MAX_SIDE_LENGTH][MAX_SIDE_LENGTH];
	unsigned long cur=0;
	int i=0, j=0, x=0, y=0, tx=0, ty=0, r=(radius*2)+1;
	double sumr=0.0, sumg=0.0, sumb=0.0;
	vector v;

	if (mandelbrot || julia) {
		computeGaussianKernel(radius);
	}
	if (flame || flame3d) {
		computeLowPassKernel(radius);
	}
	displayKernel(radius);

	cur=0;
	for (i=0; i<sideLength; i++) {
		for (j=0; j<sideLength; j++) {
			if (mandelbrot || julia) {
				matrixPoint[i][j].x = colorsList[cur];
				matrixPoint[i][j].y = colorsList[cur+1];
				matrixPoint[i][j].z = colorsList[cur+2];
				cur+=3;
			}
			if (flame || flame3d) {
				hsv2rgb(flamePointList[i][j].h,
					flamePointList[i][j].s, flamePointList[i][j].v,
					&matrixPoint[i][j].x, &matrixPoint[i][j].y, &matrixPoint[i][j].z);
			}
		}
	}

	for (i=0; i<sideLength; i++) {
		for (j=0; j<sideLength; j++) {
			if (i<radius || i>=sideLength-radius || j<radius || j>=sideLength-radius) {
				matrixPoint[i][j].x = 0;
				matrixPoint[i][j].y = 0;
				matrixPoint[i][j].z = 0;
			} else {
				sumr=0; sumg=0; sumb=0;
				for (x=0; x<r; ++x) {
					for (y=0; y<r; ++y) {
						tx = x - radius; ty = y - radius;
						sumr += matrixPoint[i+tx][j+ty].x * kernel[x][y];
						sumg += matrixPoint[i+tx][j+ty].y * kernel[x][y];
						sumb += matrixPoint[i+tx][j+ty].z * kernel[x][y];
					}
				}
				matrixPoint[i][j].x = sumr;
				matrixPoint[i][j].y = sumg;
				matrixPoint[i][j].z = sumb;
			}
		}
	}

	cur=0;
	for (i=0; i<sideLength; i++) {
		for (j=0; j<sideLength; j++) {
			if (mandelbrot || julia) {
				colorsList[cur] = matrixPoint[i][j].x;
				colorsList[cur+1] = matrixPoint[i][j].y;
				colorsList[cur+2] = matrixPoint[i][j].z;
				cur+=3;
			}
			if (flame || flame3d) {
				v = rgb2hsv(matrixPoint[i][j].x, matrixPoint[i][j].y, matrixPoint[i][j].z);
				flamePointList[i][j].h = v.x;
				flamePointList[i][j].s = v.y;
				flamePointList[i][j].v = v.z;
			}
		}
	}
}


void getWorldPos(int x, int y) {
	GLint viewport[4];
	GLdouble modelview[16];
	GLdouble projection[16];
	GLfloat winX, winY, winZ;
	GLdouble posX, posY, posZ;

	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetIntegerv(GL_VIEWPORT, viewport);

	winX = (float)x;
	winY = (float)viewport[3] - (float)y;
	glReadPixels( x, (int)winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );
	gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
	winPosx = (int)posX;
	winPosy = (int)posY;
	if (winPosx>20) { winPosx=20; }
	if (winPosy>20) { winPosy=20; }
	if (winPosx<-20) { winPosx=-20; }
	if (winPosy<-20) { winPosy=-20; }
	printf("INFO: winPosx=%d, winPosy=%d\n", winPosx, winPosy);
}


void displayIFSfern(void) {
	// source http://www.paulbourke.net/fractals/ifs_fern_a/
	unsigned long iter=0;
	int k=0, r=0, i=0, val=0;
	double x=0, y=0, xn=0, yn=0, maxy=0, dx=0, dy=0;
	float value=0, hue=0;

	double m[4][7] = { // fern
		{ 0.00, 0.00, 0.00, 0.16, 0.00, 0.00, 0.10},
		{ 0.20,-0.26, 0.23, 0.22, 0.00, 1.60, 0.08},
		{-0.15, 0.28, 0.26, 0.24, 0.00, 1.44, 0.08},
		{ 0.75, 0.04,-0.04, 0.85, 0.00, 1.60, 0.74}
	};

	int w[4] = {m[0][6]*100,
		(m[0][6]+m[1][6])*100,
		(m[0][6]+m[1][6]+m[2][6])*100, (m[0][6]+m[1][6]+m[2][6]+m[3][6])*100};

	for (iter=0, x=0, y=0; iter<iterations; iter++) {
		r = rand() % 100;
		if (r<w[0]) { k=0; }
		else if (r<w[1]) { k=1; }
		else if (r<w[2]) { k=2; }
		else { k=3; }
		xn = m[k][0]*x + m[k][1]*y + m[k][4];
		yn = m[k][2]*x + m[k][3]*y + m[k][5];
		maxy = fmax(maxy, y);
		x = xn;
		y = yn;
		value = 1.0 - ((double)iter / (double)iterations);
		pointsList[iter].x = xn;
		pointsList[iter].y = 0.0;
		pointsList[iter].z = yn;
		pointsList[iter].r = value;
	}
	for (iter=0; iter<iterations; iter++) {
		pointsList[iter].x *= 30.0 / maxy;
		pointsList[iter].z *= 30.0 / maxy;
		pointsList[iter].z -= 15;
	}
	for (i=0; i<nbrIfs; i++) {
		dx = genDoubleRandom(-15.0, 15.0);
		dy = genDoubleRandom(-15.0, 15.0);
		hue = genDoubleRandom(0.0, 1.0);
		cpt = 0;
		for (iter=0; iter<iterations; iter++) {
			val = iter + (i*iterations);
			pointsList[val].x = pointsList[cpt].x + dx;
			pointsList[val].y = pointsList[cpt].y + dy;
			pointsList[val].z = pointsList[cpt].z;
			hsv2rgb(hue, 0.90, pointsList[cpt].r, &pointsList[val].r, &pointsList[val].g, &pointsList[val].b);
			cpt++;
		}
	}
}


void defineNewParameters(int direction) {
	float currentx, currenty, scale;

	currentx = ((float)(winPosx+20)*(xmax-xmin)/40.0)+cx;
	currenty = ((float)(winPosy+20)*(xmax-xmin)/40.0)+cy;
	if (direction) { // increase
		scale = (xmax-xmin)*0.5;
		xmin = currentx - (scale * 0.5);
		xmax = currentx + (scale * 0.5);
		ymin = currenty - (scale * 0.5);
		ymax = currenty + (scale * 0.5);
		cx = xmin; cy = ymin;
		maxIter += 128;
	} else { // decrease
		scale = xmax-xmin;
		xmin = currentx - scale;
		xmax = currentx + scale;
		ymin = currenty - scale;
		ymax = currenty + scale;
		cx = xmin; cy = ymin;
		maxIter -= 128;
	}
}


void contractiveMapping(double *ac, double *bc, double *cc, double *dc, double *ec, double *fc) {
	// from Paul Bourke
	double a, b, c, d, e, f;
	int t;
	do {
		do {
			a = genDoubleRandom(0.0, 1.0);
			d = genDoubleRandom(a*a, 1);
			t = rand();
			if (t&1) d = -d;
		} while ((a*a + d*d) > 1);
		do {
			b = genDoubleRandom(0.0, 1.0);
			e = genDoubleRandom(b*b, 1);
			t = rand();
			if (t&1) e = -e;
		} while ((b*b + e*e) > 1);
	} while ((a*a + b*b + d*d + e*e) > (1 + (a*e - d*b) * (a*e - d*b)));
	c = genDoubleRandom(-2.0, 2.0);
	f = genDoubleRandom(-2.0, 2.0);
	*ac=a; *bc=b; *cc=c; *dc=d; *ec=e; *fc=f;
}


double genFlameRand01() {
	double tmp=0.0;
	tmp = (rand() & 0xfffffff) / (double) 0xfffffff;
	return(tmp);
}


double genFlameRand11() {
	double tmp=0.0;
	tmp = ((rand() & 0xfffffff) - 0x7ffffff) / (double) 0x7ffffff;
	return(tmp);
}


void initFlameFunction(void) {
	int i=0, var=0;
	var = rand() % 51;;
	for (i=0; i<numOfFlameFunct; i++) {
		//flameFuncList[i].variation = rand() % 51;
		flameFuncList[i].variation = var;
		flameFuncList[i].weight = genFlameRand01();
		flameFuncList[i].h = genFlameRand01();
		flameFuncList[i].s = 0.75;
		flameFuncList[i].v = 0.9;
		contractiveMapping(&flameFuncList[i].ac, &flameFuncList[i].bc, &flameFuncList[i].cc, &flameFuncList[i].dc, &flameFuncList[i].ec, &flameFuncList[i].fc);
	}
}


void computeFlame(void) {
	double tx=0.0, ty=0.0, nx=0.0, ny=0.0,
	ac=0.0, bc=0.0, cc=0.0, dc=0.0, ec=0.0, fc=0.0;
	int i=0, endx=0, endy=0;
	unsigned long iter=0;

	for (endx=0; endx<sideLength; endx++) {
		for (endy=0; endy<sideLength; endy++) {
			flamePointList[endx][endy].h = 0.0;
			flamePointList[endx][endy].s = 0.0;
			flamePointList[endx][endy].v = 0.0;
			flamePointList[endx][endy].counter = 0;
		}
	}

	cpt = 0;
	nx = genDoubleRandom(xmin, xmax);
	ny = genDoubleRandom(ymin, ymax);
	for (iter=0; iter<iterations; iter++) {
		i = rand() % numOfFlameFunct;
		ac = flameFuncList[i].ac;
		bc = flameFuncList[i].bc;
		cc = flameFuncList[i].cc;
		dc = flameFuncList[i].dc;
		ec = flameFuncList[i].ec;
		fc = flameFuncList[i].fc;

		tx = ac*nx + bc*ny + cc;
		ty = dc*nx + ec*ny + fc;

		switch (flameFuncList[i].variation) {
			case 0:
				snprintf(flameName, 14, "Linear");
				nx = tx;
				ny = ty;
				break;
			case 1:
				snprintf(flameName, 14, "Sinusoidal");
				nx = sin(tx);
				ny = sin(ty);
				break;
			case 2:
				snprintf(flameName, 14, "Spherical");
				double r2 = tx*tx + ty*ty + 1e-6;
				nx = tx / r2;
				ny = ty / r2;
				break;
			case 3:
				snprintf(flameName, 14, "Swirl");
				r2 = tx*tx + ty*ty;
				double c1 = sin(r2);
				double c2 = cos(r2);
				nx = c1*tx - c2*ty;
				ny = c2*tx + c1*ty;
				break;
			case 4:
				snprintf(flameName, 14, "Horseshoe");
				double a = atan2(tx, ty);
				c1 = sin(a);
				c2 = cos(a);
				nx = c1*tx - c2*ty;
				ny = c2*tx + c1*ty;
				break;
			case 5:
				snprintf(flameName, 14, "Polar");
				nx = atan2(tx, ty) / M_PI;
				ny = sqrt(tx*tx + ty*ty) - 1.0;
				break;
			case 6:
				snprintf(flameName, 14, "Handkerchief");
				a = atan2(tx, ty);
				double r = sqrt(tx*tx + ty*ty);
				nx = r * sin(a + r);
				ny = r * cos(a - r);
				break;
			case 7:
				snprintf(flameName, 14, "Heart");
				r = sqrt(tx*tx + ty*ty);
				a = atan2(tx, ty);
				a *= r;
				nx = r * sin(a);
				ny = -r * cos(a);
				break;
			case 8:
				snprintf(flameName, 14, "Disc");
				nx = tx * M_PI;
				ny = ty * M_PI;
				a = atan2(nx, ny);
				r = sqrt(nx*nx + ny*ny);
				nx = sin(r) * a / M_PI;
				ny = cos(r) * a / M_PI;
				break;
			case 9:
				snprintf(flameName, 14, "Spiral");
				a = atan2(tx, ty);
				r = sqrt(tx*tx + ty*ty) + 1e-6;
				nx = (cos(a) + sin(r)) / r;
				ny = (sin(a) - cos(r)) / r;
				break;
			case 10:
				snprintf(flameName, 14, "Hyperbolic");
				a = atan2(tx, ty);
				r = sqrt(tx*tx + ty*ty) + 1e-6;
				nx = sin(a) / r;
				ny = cos(a) * r;
				break;
			case 11:
				snprintf(flameName, 14, "Diamond");
				a = atan2(tx, ty);
				r = sqrt(tx*tx + ty*ty);
				nx = sin(a) * cos(r);
				ny = cos(a) * sin(r);
				break;
			case 12:
				snprintf(flameName, 14, "Ex");
				a = atan2(tx, ty);
				r = sqrt(tx*tx + ty*ty);
				double n0 = sin(a+r);
				double n1 = cos(a-r);
				double m0 = n0 * n0 * n0 * r;
				double m1 = n1 * n1 * n1 * r;
				nx = m0 + m1;
				ny = m0 - m1;
				break;
			case 13:
				snprintf(flameName, 14, "Julia");
				a = atan2(tx, ty) / 2.0;
				if (rand()%2) a += M_PI;
				r = pow(tx*tx + ty*ty, 0.25);
				nx = r * cos(a);
				ny = r * sin(a);
				break;
			case 14:
				snprintf(flameName, 14, "Bent");
				nx = tx;
				ny = ty;
				if (nx<0.0) { nx = nx * 2.0; }
				if (ny<0.0) { ny = ny / 2.0; }
				break;
			case 15:
				snprintf(flameName, 14, "Waves");
				nx = tx + bc * sin(ty/((cc*cc) + 1e-10));
				ny = ty + ec * sin(tx/((fc*fc) + 1e-10));
				break;
			case 16:
				snprintf(flameName, 14, "Fisheye");
				a = atan2(tx, ty);
				r = sqrt(tx*tx + ty*ty);
				r = 2.0 * r / (r + 1.0);
				nx = r * cos(a);
				ny = r * sin(a);
				break;
			case 17:
				snprintf(flameName, 14, "Popcorn");
				double dx = tan(3*ty);
				double dy = tan(3*tx);
				nx = tx + cc * sin(dx);
				ny = ty + fc * sin(dy);
				break;
			case 18:
				snprintf(flameName, 14, "Exponential");
				dx = exp(tx - 1.0);
				dy = M_PI * ty;
				nx = cos(dy) * dx;
				ny = sin(dy) * dx;
				break;
			case 19:
				snprintf(flameName, 14, "Power");
				a = atan2(tx, ty);
				double sa = sin(a);
				double ca = cos(a);
				r = sqrt(tx*tx + ty*ty);
				r = pow(r, sa);
				nx = r * ca;
				ny = r * sa;
				break;
			case 20:
				snprintf(flameName, 14, "Cosine");
				nx = cos(tx * M_PI) * cosh(ty);
				ny = -sin(tx * M_PI) * sinh(ty);
				break;
			case 21:
				snprintf(flameName, 14, "Rings");
				dx = cc * cc + 1e-10;
				r = sqrt(tx*tx + ty*ty);
				r = fmod(r + dx, 2*dx) - dx + r*(1-dx);
				a = atan2(tx, ty);
				nx = cos(a) * r;
				ny = sin(a) * r;
				break;
			case 22:
				snprintf(flameName, 14, "Fan");
				dx = M_PI * (cc * cc + 1e-10);
				double dx2 = dx/2;
				a = atan2(tx,ty);
				r = sqrt(tx*tx + ty*ty);
				a += (fmod(a+fc, dx) > dx2) ? -dx2 : dx2;
				nx = cos(a) * r;
				ny = sin(a) * r;
				break;
			case 23:
				snprintf(flameName, 14, "Blob");
				a = atan2(tx, ty);
				r = sqrt(tx*tx + ty*ty);
				double bloblow = genDoubleRandom(0.0, 0.5);
				double blobhigh = genDoubleRandom(0.5, 1.0);
				double blobwaves = M_PI / 8.0;
				r = r * (bloblow + (blobhigh-bloblow) * (0.5 + 0.5 * sin(blobwaves * a)));
				nx = sin(a) * r;
				ny = cos(a) * r;
				break;
			case 24:
				snprintf(flameName, 14, "PDJ");
				double nx1 = cos(genFlameRand11() * tx);
				double nx2 = sin(genFlameRand11() * tx);
				double ny1 = sin(genFlameRand11() * ty);
				double ny2 = cos(genFlameRand11() * ty);
				nx = ny1 - nx1;
				ny = nx2 - ny2;
				break;
			case 25:
				snprintf(flameName, 14, "Fan2");
				a = atan2(tx, ty);
				r = sqrt(tx*tx + ty*ty);
				double fan2 = genFlameRand11();
				dy = genFlameRand11();
				dx = M_PI * (fan2 * fan2 + 1e-10);
				dx2 = dx / 2.0;
				double t = a + dy - dx * (int)((a + dy)/dx);
				if (t > dx2) { a = a - dx2; }
				else { a = a + dx2; }
				nx = sin(a) * r;
				ny = cos(a) * r;
				break;
			case 26:
				snprintf(flameName, 14, "Rings2");
				double fx = genFlameRand11();
				r = sqrt(tx*tx + ty*ty);
				dx = fx * fx + 1e-10;
				r += dx - 2.0*dx*(int)((r + dx)/(2.0 * dx)) - dx + r * (1.0-dx);
				nx = sin(a) * r;
				ny = cos(a) * r;
				break;
			case 27:
				snprintf(flameName, 14, "Eyefish");
				r = 2.0 / (sqrt(tx*tx + ty*ty) + 1.0);
				nx = r * tx;
				ny = r * ty;
				break;
			case 28:
				snprintf(flameName, 14, "Bubble");
				r = 4.0 / ((tx*tx + ty*ty) + 4.0);
				nx = r * tx;
				ny = r * ty;
				break;
			case 29:
				snprintf(flameName, 14, "Cylinder");
				nx = sin(tx);
				ny = ty;
				break;
			case 30:
				snprintf(flameName, 14, "Perspective");
				double p1 = M_PI_4;
				double p2 = 2*genFlameRand01() + 1.0;
				t = p2 / (p2 - ty*sin(p1));
				nx = t * tx;
				ny = ty * cos(p1);
				break;
			case 31:
				snprintf(flameName, 14, "Noise");
				p1 = genFlameRand01();
				p2 = genFlameRand01();
				nx = p1 * tx * cos(2*M_PI*p2);
				ny = p1 * ty * sin(2*M_PI*p2);
				break;
			case 32:
				snprintf(flameName, 14, "JuliaN");
				a = atan2(ty,tx);
				r = tx*tx + ty*ty;
				p1 = 1 + rand() % 7;
				p2 = genFlameRand01();
				double p3 = trunc(p1 * genFlameRand01());
				t = 2*M_PI*p3 + a / p1;
				r = pow(r, p2/p1);
				nx = r * cos(t);
				ny = r * sin(t);
				break;
			case 33:
				snprintf(flameName, 14, "JuliaScope");
				a = atan2(ty,tx);
				r = tx*tx + ty*ty;
				p1 = 1 + rand() % 7;
				p2 = genFlameRand01();
				p3 = trunc(p1 * genFlameRand01());
				if (rand()%2) { t = 2*M_PI*p3 + a / p1; } else { t = 2*M_PI*p3 - a / p1; }
				r = pow(r, p2/p1);
				nx = r * cos(t);
				ny = r * sin(t);
				break;
			case 34:
				snprintf(flameName, 14, "Blur");
				p1 = genFlameRand01();
				p2 = genFlameRand01() * 2 * M_PI;
				nx = p1 * cos(p2);
				ny = p1 * sin(p2);
				break;
			case 35:
				snprintf(flameName, 14, "Gaussian");
				r = genFlameRand01() + genFlameRand01() + genFlameRand01() + genFlameRand01() - 2.0;
				p1 = genFlameRand01() * 2 * M_PI;
				nx = r * cos(p1);
				ny = r * sin(p1);
				break;
			case 36:
				snprintf(flameName, 14, "RadialBlur");
				t = genFlameRand01() + genFlameRand01() + genFlameRand01() + genFlameRand01() - 2.0;
				r = sqrt(tx*tx + ty*ty);
				a = atan2(ty,tx);
				p1 = M_PI/8.0 * M_PI/2.0;
				p2 = a + t * p1;
				p3 = t * p1 -1.0;
				nx = r*cos(p2) + p3*tx;
				ny = r*sin(p2) + p3*ty;
				break;
			case 37:
				snprintf(flameName, 14, "Pie");
				p1 = 10.0*genFlameRand01(); // slices
				p2 = 2.0 * M_PI * genFlameRand11(); // rotation
				p3 = genFlameRand01(); // thickness
				t = (int)(genFlameRand01() * p1 + 0.5);
				a = p2 + 2.0 * M_PI * (t + genFlameRand01() * p3) / p1;
				r = genFlameRand01();
				nx = r * cos(a);
				ny = r * sin(a);
				break;
			case 38:
				snprintf(flameName, 14, "Ngon");
				p1 = 3*genFlameRand01() + 1; // power
				p2 = 2*M_PI / (10*genFlameRand01() + 3); // sides
				p3 = 2*genFlameRand01(); // corners
				double p4 = 3*genFlameRand01(); // circle
				double theta = atan2(ty,tx);
				t = theta - (p2 * floor(theta / p2));
				if (t > p2 / 2) { r = t; } else { r = t - p2; }
				r = (1.0 / cos(r)) - 1;
				double k = (p3*r + p4) / pow(tx*tx + ty*ty, p1);
				nx = k * tx;
				ny = k * ty;
				break;
			case 39:
				snprintf(flameName, 14, "Tangent");
				nx = sin(tx) / cos(ty);
				ny = tan(ty);
				break;
			case 40:
				snprintf(flameName, 14, "Square");
				p1 = genFlameRand01();
				p2 = genFlameRand01();
				nx = p1 - 0.5;
				ny = p2 - 0.5;
				break;
			case 41:
				snprintf(flameName, 14, "Rays");
				t = genFlameRand01() * M_PI;
				r = 1.0 / (tx*tx + ty*ty + 1e-10);
				p1 = tan(t) * r;
				nx = p1 * cos(tx);
				ny = p1 * sin(ty);
				break;
			case 42:
				snprintf(flameName, 14, "Blade");
				r = genFlameRand01() * sqrt(tx*tx + ty*ty);
				nx = tx * (cos(r) + sin(r));
				ny = tx * (cos(r) - sin(r));
				break;
			case 43:
				snprintf(flameName, 14, "Twintrian");
				r = genFlameRand01() * sqrt(tx*tx + ty*ty);
				p1 = cos(r);
				p2 = sin(r);
				t = log10(p2*p2)+p1;
				nx = tx * t;
				ny = tx * (t - p2*M_PI);
				break;
			case 44:
				snprintf(flameName, 14, "Cross");
				t = tx*tx - ty*ty;
				r = sqrt(1.0 / (t*t+1e-10));
				nx = tx * r;
				ny = ty * r;
				break;
			case 45:
				snprintf(flameName, 14, "Flower");
				a = atan2(ty,tx);
				r = genFlameRand01() - genFlameRand01() * cos(4*a*genFlameRand01()) / sqrt(tx*tx + ty*ty);
				nx = r * tx;
				ny = r * ty;
				break;
			case 46:
				snprintf(flameName, 14, "Butterfly");
				t = 1.3029400317411197908970256609023;
				p1 = ty * 2.0;
				r = t * sqrt(fabs(ty*tx)/(1e-10 + tx*tx + p1*p1));
				nx = r * tx;
				ny = r * p1;
				break;
			case 47:
				snprintf(flameName, 14, "Escher");
				a = atan2(ty,tx);
				r = 0.5 * log(tx*tx + ty*ty);
				t = M_PI * genFlameRand11();
				double vc = 0.5 * (1.0 + cos(t));
				double vd = 0.5 * sin(t);
				double m = exp(vc*r - vd*a);
				double n = vc*a + vd*r;
				nx = m * cos(n);
				ny = m * sin(n);
				break;
			case 48:
				snprintf(flameName, 14, "Popcorn2");
				t = 5 * genFlameRand01();
				nx = tx * genFlameRand01() * sin(tan(ty*t));
				ny = ty * genFlameRand01() * sin(tan(tx*t));
				break;
			case 49:
				snprintf(flameName, 14, "Log");
				nx = 0.5 * log(tx*tx + ty*ty);
				ny = atan2(ty,tx);
				break;
			case 50:
				snprintf(flameName, 14, "Mobius");
				double rea = genFlameRand11();
				double reb = genFlameRand11();
				double rec = genFlameRand11();
				double red = genFlameRand11();
				double ima = genFlameRand11();
				double imb = genFlameRand11();
				double imc = genFlameRand11();
				double imd = genFlameRand11();
				double reu = rea * tx - ima * ty + reb;
				double imu = rea * ty + ima * tx + imb;
				double rev = rec * tx - imc * ty + red;
				double imv = rec * ty + imc * tx + imd;
				double radv = 1.0 / (rev*rev + imv*imv);
				nx = radv * (reu*rev + imu*imv);
				ny = radv * (imu*rev - reu*imv);
				break;
			default:
				break;
		}
		if (iter>20) {
			if (nx>=xmin && nx<=xmax && ny>=ymin && ny<=ymax) {
				endx = (sideLength/2) - (int)((nx * sideLength) / (xmax - xmin));
				endy = (sideLength/2) - (int)((ny * sideLength) / (ymax - ymin));

				if (!flamePointList[endx][endy].counter) {
					flamePointList[endx][endy].h = flameFuncList[i].h;
					flamePointList[endx][endy].s = flameFuncList[i].s;
					flamePointList[endx][endy].v = flameFuncList[i].v;
				} else {
					flamePointList[endx][endy].h = flameFuncList[i].h;
					flamePointList[endx][endy].s = flamePointList[endx][endy].s - 0.002;
					flamePointList[endx][endy].v = flamePointList[endx][endy].v - 0.001;
				}
				flamePointList[endx][endy].counter++;
				cpt++;
			}
		}
	}
}


void displayFlame(void) {
	unsigned long iter=0;
	int x=0, y=0;

	if (flame) { generateBlur(blurRadius); }
	if (flame3d) { generateBlur(blurRadius); }

	if (flame3d) {
		free(verticesList);
		free(colorsList);
		verticesList = (float *)calloc(iterations*18, sizeof(float));
		colorsList = (float *)calloc(iterations*18, sizeof(float));
	}
	iter=0;
	for (x=0; x<sideLength; x++) {
		for (y=0; y<sideLength; y++) {
			if (flame3d) {
				hsv2rgb(flamePointList[x][y].h,
					flamePointList[x][y].s,
					flamePointList[x][y].v,
					&colorsList[iter], &colorsList[iter+1], &colorsList[iter+2]);
				hsv2rgb(flamePointList[x+1][y].h,
					flamePointList[x+1][y].s,
					flamePointList[x+1][y].v,
					&colorsList[iter+3], &colorsList[iter+4], &colorsList[iter+5]);
				hsv2rgb(flamePointList[x][y+1].h,
					flamePointList[x][y+1].s,
					flamePointList[x][y+1].v,
					&colorsList[iter+6], &colorsList[iter+7], &colorsList[iter+8]);

				hsv2rgb(flamePointList[x+1][y].h,
					flamePointList[x+1][y].s,
					flamePointList[x+1][y].v,
					&colorsList[iter+9], &colorsList[iter+10], &colorsList[iter+11]);
				hsv2rgb(
					flamePointList[x+1][y+1].h,
					flamePointList[x+1][y+1].s,
					flamePointList[x+1][y+1].v,
					&colorsList[iter+12], &colorsList[iter+13], &colorsList[iter+14]);
				hsv2rgb(
					flamePointList[x][y+1].h,
					flamePointList[x][y+1].s,
					flamePointList[x][y+1].v,
					&colorsList[iter+15], &colorsList[iter+16], &colorsList[iter+17]);

				verticesList[iter] = (x - (sideLength/2)) / (sideLength/50.0);
				verticesList[iter+1] = 4 * flamePointList[x][y].h;
				verticesList[iter+2] = (y - (sideLength/2)) / (sideLength/50.0);

				verticesList[iter+3] = (x+1 - (sideLength/2)) / (sideLength/50.0);
				verticesList[iter+4] = 4 * flamePointList[x+1][y].h;
				verticesList[iter+5] = (y - (sideLength/2)) / (sideLength/50.0);

				verticesList[iter+6] = (x - (sideLength/2)) / (sideLength/50.0);
				verticesList[iter+7] = 4 * flamePointList[x][y+1].h;
				verticesList[iter+8] = (y+1 - (sideLength/2)) / (sideLength/50.0);

				verticesList[iter+9] = (x+1 - (sideLength/2)) / (sideLength/50.0);
				verticesList[iter+10] = 4 * flamePointList[x+1][y].h;
				verticesList[iter+11] = (y - (sideLength/2)) / (sideLength/50.0);

				verticesList[iter+12] = (x+1 - (sideLength/2)) / (sideLength/50.0);
				verticesList[iter+13] = 4 * flamePointList[x+1][y+1].h;
				verticesList[iter+14] = (y+1 - (sideLength/2)) / (sideLength/50.0);

				verticesList[iter+15] = (x - (sideLength/2)) / (sideLength/50.0);
				verticesList[iter+16] = 4 * flamePointList[x][y+1].h;
				verticesList[iter+17] = (y+1 - (sideLength/2)) / (sideLength/50.0);
				iter+=18;
			}

			if (flame) {
				hsv2rgb(
					(float)flamePointList[x][y].h,
					(float)flamePointList[x][y].s,
					(float)flamePointList[x][y].v,
					&pointsList[iter].r, &pointsList[iter].g, &pointsList[iter].b);
				pointsList[iter].x = (x - (sideLength/2)) / (sideLength/50.0);
				pointsList[iter].y = 0.0;
				pointsList[iter].z = (y - (sideLength/2)) / (sideLength/50.0);
				iter++;
			}
		}
	}
}


void displayFractal(void) {
	// https://rosettacode.org/wiki/Mandelbrot_set
	// https://rosettacode.org/wiki/Julia_set
	unsigned long cpt=0, cur=0;
	int i=0, j=0, iter=0, min=0, max=0;
	double x=0, y=0, zr=0, zi=0, zr2=0, zi2=0, tmp=0;
	float hue=0.0, val=0.0;
	static float hueList[ITERATIONS];

	cpt = 0;
	min=maxIter; max=0;
	for (j=0; j<sideLength; j++) {
		y = ((ymax-ymin) * j / sideLength) + cy;

		for (i=0; i<sideLength; i++) {
			x = ((xmax-xmin) * i / sideLength) + cx;
			iter = 0;
			if (mandelbrot) {
				// Mandelbrot set: formula z = z2 + c with c = x + iy
				cr = x; ci = y;
				zr = 0; zi = 0;
			}
			if (julia) {
				// Julia set: formula z = z2 + c with z = x + iy
				zr = x; zi = y;
				cr = juliaR * cos(juliaA);
				ci = juliaR * sin(juliaA);
			}
			do {
				tmp = zr;
				zr2 = zr*zr;
				zi2 = zi*zi;
				zr = zr2 - zi2 + cr;
				zi = 2*zi*tmp + ci;
				iter++;
			} while ( (zr2+zi2<4) && (iter<maxIter) );

			if (iter < min) min = iter;
			if (iter > max) max = iter;
			hueList[cpt] = iter;
			cpt++;
		}
	}
	cpt = 0;
	cur = 0;
	for (i=0; i<sideLength; i++) {
		for (j=0; j<sideLength; j++) {
			hue = hueList[cpt];
			if (!saturation) {
				val = (float)(max-hue) / (float)(max-min);
				colorsList[cur] = val;
				colorsList[cur+1] = val;
				colorsList[cur+2] = val;
			} else {
				hue = (hue+min) / (float)(max - min);
				hsv2rgb(hue, saturation, 1.0, &colorsList[cur], &colorsList[cur+1], &colorsList[cur+2]);
			}
			cpt++;
			cur+=3;
		}
	}
	if (mandelbrot) { generateBlur(blurRadius); }
	if (julia && !rotJulia) { generateBlur(blurRadius); }
}


int computeMandelbulb(float x0, float y0, float z0, int iMax, float power) {
	// http://images.math.cnrs.fr/Mandelbulb.html
	// how many iterations we need to get infinity
	double x=0.0, y=0.0, z=0.0, xn=0.0, yn=0.0, zn=0.0,
		r=0.0, theta=0.0, phi=0.0;
	int i=0;
	x = (double)x0;
	y = (double)y0;
	z = (double)z0;
	for (i=0; i<iMax; i++) {
		r = sqrt(x*x + y*y + z*z);
		theta = atan2(sqrt(x*x + y*y), z);
		phi = atan2(y, x);

		xn = pow(r, power) * sin(theta*power) * cos(phi*power) + x0;
		yn = pow(r, power) * sin(theta*power) * sin(phi*power) + y0;
		zn = pow(r, power) * cos(theta*power) + z0;

		if (xn*xn + yn*yn + zn*zn > 8.0f) {
			return(i);
		}
		x = xn;
		y = yn;
		z = zn;
	}
	return(iMax);
}


void displayMandelbulb(void) {
	unsigned long i=0;
	float hue=0.0;
	double x=0.0, y=0.0, z=0.0;
	int m=0;

	x = xmin; y = xmin; z = xmin;
	for (i=0; i<iterations; i++) {
		m = computeMandelbulb(x, y, z, maxIter, power);
		if (m >= maxIter) {
			if (x>y && x>z) {
				hue = (float)x / (float)xmax;
			} else if (y>x && y>z) {
				hue = (float)y / (float)xmax;
			} else if (z>x && z>y) {
				hue = (float)z / (float)xmax;
			} else {
				hue = 0.0;
			}
			hsv2rgb(hue, 0.75, 0.9, &(pointsList[i].r), &(pointsList[i].g), &(pointsList[i].b));

			pointsList[i].x = x * 8.0;
			pointsList[i].y = y * 8.0;
			pointsList[i].z = z * 8.0;
		}
		x += pas;
		if (i!=0 && i%sideLength==0) {
			x = xmin;
			z += pas;
			if (z>xmax) {
				x = xmin;
				z = xmin;
				y += pas;
			}
		}
	}
}


void drawCube(vector pos, vector color, double size) {
	float dim = size * 0.5;
	glLineWidth(1.0);
	glTranslatef(pos.x, pos.y, pos.z);
	glColor3f(color.x, color.y, color.z);

	glBegin(GL_QUADS);
		// front
		glVertex3f(-dim, -dim,  dim);
		glVertex3f( dim, -dim,  dim);
		glVertex3f( dim, -dim, -dim);
		glVertex3f(-dim, -dim, -dim);
		// back
		glVertex3f(-dim,  dim,  dim);
		glVertex3f( dim,  dim,  dim);
		glVertex3f( dim,  dim, -dim);
		glVertex3f(-dim,  dim, -dim);
		// right
		glVertex3f( dim, -dim,  dim);
		glVertex3f( dim,  dim,  dim);
		glVertex3f( dim,  dim, -dim);
		glVertex3f( dim, -dim, -dim);
		// left
		glVertex3f(-dim, -dim,  dim);
		glVertex3f(-dim,  dim,  dim);
		glVertex3f(-dim,  dim, -dim);
		glVertex3f(-dim, -dim, -dim);
		// top
		glVertex3f(-dim, -dim,  dim);
		glVertex3f( dim, -dim,  dim);
		glVertex3f( dim,  dim,  dim);
		glVertex3f(-dim,  dim,  dim);
		// bottom
		glVertex3f(-dim, -dim, -dim);
		glVertex3f( dim, -dim, -dim);
		glVertex3f( dim,  dim, -dim);
		glVertex3f(-dim,  dim, -dim);
	glEnd();
}


void drawPyramid(vector pos, vector color, double size) {
	float dim = size * 0.5;
	glLineWidth(1.0);
	glTranslatef(pos.x, pos.y, pos.z);
	glColor3f(color.x, color.y, color.z);

	glBegin(GL_TRIANGLE_FAN);
		glVertex3f( 0.0, 0.0, dim);
		glVertex3f( dim, -dim, -dim);
		glVertex3f( dim,  dim, -dim);
		glVertex3f(-dim,  dim, -dim);
		glVertex3f(-dim, -dim, -dim);
		glVertex3f( dim, -dim, -dim);
	glEnd();
	glBegin(GL_QUADS);
		glVertex3f(-dim, -dim, -dim);
		glVertex3f( dim, -dim, -dim);
		glVertex3f( dim,  dim, -dim);
		glVertex3f(-dim,  dim, -dim);
	glEnd();
}


short isUnRemovable(int val) {
	unsigned long i;
	short result=1;
	for (i=0; i<N_ELEMENTS(mengerRemovals); i++) {
		if (val == mengerRemovals[i]) {
			result = 0;
			break;
		}
	}
	return(result);
}


void computeMenger(float xc, float yc, float zc, float size, int depth) {
	int x=0, y=0, z=0, val=0;
	float xorig, yorig, zorig;

	if (depth==1) {
		cubeList[cpt].size = size;
		cubeList[cpt].pos.x = 0.0;
		cubeList[cpt].pos.y = 0.0;
		cubeList[cpt].pos.z = 0.0;
	} else {
		xorig = xc;
		yorig = yc;
		zorig = zc;
		size = size / 3.0;
		val = 0;
		for (x=0; x<3; x++) {
			for (z=0; z<3; z++) {
				for (y=0; y<3; y++) {
					if (isUnRemovable(val)) {
						xc = xorig + x * size;
						yc = yorig + y * size;
						zc = zorig + z * size;
						if (depth > 2) {
							computeMenger(xc, yc, zc, size, depth-1);
						} else {
							if (xmax < xc) { xmax = xc; }
							cubeList[cpt].size = size;
							cubeList[cpt].pos.x = xc;
							cubeList[cpt].pos.y = yc;
							cubeList[cpt].pos.z = zc;
							cpt++;
						}
					}
					val++;
				}
			}
		}
	}
}


void colorizeMenger(void) {
	int i=0;
	float decal;
	double hue=0.0, saturation=0.0, value=0.0;
	saturation = 0.50;
	value = 0.50;
	decal = xmax / 2.0;
	for (i=0; i<mengerIter; i++) {
		hue = (double)i / (double)mengerIter;
		hsv2rgb(hue, saturation, value, &cubeList[i].color.x, &cubeList[i].color.y, &cubeList[i].color.z);
		cubeList[i].pos.x -= decal;
		cubeList[i].pos.y -= decal;
		cubeList[i].pos.z -= decal;
	}
}


void displayMenger(void) {
	computeMenger(0.0f, 0.0f, 0.0f, 30, mengerDepth);
	colorizeMenger();
}


void colorizeSierpinski(void) {
	int i=0;
	float decal;
	double hue=0.0, saturation=0.0, value=0.0;
	saturation = 0.50;
	value = 0.50;
	decal = xmax / 2.0;
	for (i=0; i<sierpinskiIter; i++) {
		hue = (double)i / (double)sierpinskiIter;
		hsv2rgb(hue, saturation, value, &pyramidList[i].color.x, &pyramidList[i].color.y, &pyramidList[i].color.z);
		pyramidList[i].pos.z -= decal;
	}
}


void computeSierpinski(float xc, float yc, float zc, float size, int depth) {
	float decal=0.0;
	vector pos;

	size = size / 2.0;
	decal = size / 2.0;
	if (depth==1) {
		pos.x=0.0; pos.y=0.0; pos.z=0.0;
		pyramidList[cpt].size = size*2;
		pyramidList[cpt].pos = pos;
	} else {
		if (depth>2) {
			computeSierpinski(xc+decal, yc-decal, zc, size, depth-1);
			computeSierpinski(xc+decal, yc+decal, zc, size, depth-1);
			computeSierpinski(xc-decal, yc-decal, zc, size, depth-1);
			computeSierpinski(xc-decal, yc+decal, zc, size, depth-1);
			computeSierpinski(xc, yc, zc+size, size, depth-1);
		} else {
			if (xmax < zc+size) { xmax = zc+size; }
			pos.x=xc+decal; pos.y=yc-decal; pos.z=zc;
			pyramidList[cpt].size = size;
			pyramidList[cpt].pos = pos;

			pos.x=xc+decal; pos.y=yc+decal; pos.z=zc;
			pyramidList[cpt+1].size = size;
			pyramidList[cpt+1].pos = pos;

			pos.x=xc-decal; pos.y=yc-decal; pos.z=zc;
			pyramidList[cpt+2].size = size;
			pyramidList[cpt+2].pos = pos;

			pos.x=xc-decal; pos.y=yc+decal; pos.z=zc;
			pyramidList[cpt+3].size = size;
			pyramidList[cpt+3].pos = pos;

			pos.x=xc; pos.y=yc; pos.z=zc+size;
			pyramidList[cpt+4].size = size;
			pyramidList[cpt+4].pos = pos;
			cpt += 5;
		}
	}
}


void displaySierpinski(void) {
	computeSierpinski(0.0f, 0.0f, 0.0f, 30, sierpinskiDepth);
	colorizeSierpinski();
}


void takeScreenshot(char *filename) {
	FILE *fp = fopen(filename, "wb");
	int width = glutGet(GLUT_WINDOW_WIDTH);
	int height = glutGet(GLUT_WINDOW_HEIGHT);
	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info = png_create_info_struct(png);
	unsigned char *buffer = calloc((width * height * 3), sizeof(unsigned char));
	int i;

	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)buffer);
	png_init_io(png, fp);
	png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png, info);
	for (i=0; i<height; i++) {
		png_write_row(png, &(buffer[3*width*((height-1) - i)]));
	}
	png_write_end(png, NULL);
	png_destroy_write_struct(&png, &info);
	free(buffer);
	fclose(fp);
	printf("INFO: Save screenshot on %s (%d x %d)\n", filename, width, height);
}


void drawString(float x, float y, float z, char *text) {
	unsigned i = 0;
	glPushMatrix();
	glLineWidth(1.0);
	if (background){ // White background
		glColor3f(0.0, 0.0, 0.0);
	} else { // Black background
		glColor3f(1.0, 1.0, 1.0);
	}
	glTranslatef(x, y, z);
	glScalef(0.008, 0.008, 0.008);
	for(i=0; i < strlen(text); i++) {
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, (int)text[i]);
	}
	glPopMatrix();
}


void drawText(void) {
	char text1[70], text2[70];
	if (menger) {
		sprintf(text1, "Nbr of steps: %d, Nbr of cubes: %d", mengerDepth, mengerIter);
	} else if (sierpinski) {
		sprintf(text1, "Nbr of steps: %d, Nbr of pyramids: %d", sierpinskiDepth, sierpinskiIter);
	}else if (flame || flame3d) {
		sprintf(text1, "Iterations: %ld, Displayed points: %d, Blur radius %d", iterations, cpt, blurRadius);
	} else {
		sprintf(text1, "Nbr of points: %ld, Max iterations: %d", iterations, maxIter);
	}
	if (julia) {
		sprintf(text2, "dt: %1.3f, FPS: %4.2f, cr=%0.3f ci=%0.3f", (dt/1000.0), fps, cr, ci);
	} else if (flame || flame3d) {
		sprintf(text2, "dt: %1.3f, FPS: %4.2f, Flame variation: %s", (dt/1000.0), fps, flameName);
	} else {
		sprintf(text2, "dt: %1.3f, FPS: %4.2f", (dt/1000.0), fps);
	}
	textList = glGenLists(1);
	glNewList(textList, GL_COMPILE);
	drawString(-40.0, -40.0, -100.0, text1);
	drawString(-40.0, -38.0, -100.0, text2);
	glEndList();
}


GLuint loadPNGtexture(const char *filename) {
	GLuint texID;
	png_byte ct, bd;
	int w=0, h=0, x=0, y=0, cur=0;
	unsigned char *data=NULL;
	png_bytep *rowPointers=NULL, row, px;

	FILE *fp = fopen(filename, "rb");
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info = png_create_info_struct(png);
	png_init_io(png, fp);
	png_read_info(png, info);

	w = png_get_image_width(png, info);
	h = png_get_image_height(png, info);
	ct = png_get_color_type(png, info);
	bd = png_get_bit_depth(png, info);

	if(bd == 16) png_set_strip_16(png);
	if(ct == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
	if(ct == PNG_COLOR_TYPE_GRAY && bd < 8) png_set_expand_gray_1_2_4_to_8(png);
	if(png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
	if(ct == PNG_COLOR_TYPE_RGB || ct == PNG_COLOR_TYPE_GRAY || ct == PNG_COLOR_TYPE_PALETTE) png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
	if(ct == PNG_COLOR_TYPE_GRAY || ct == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);

	png_read_update_info(png, info);
	rowPointers = (png_bytep*)malloc(sizeof(png_bytep)*h);
	for (y=0; y<h; y++) rowPointers[y] = (png_byte*)malloc(png_get_rowbytes(png, info));
	png_read_image(png, rowPointers);
	fclose(fp);
	data = (unsigned char*)calloc(h*w*4, sizeof(data));
	cur=0;
	for(y=h-1; y>=0; y--) {
		row = rowPointers[y];
		for(x=0; x<w; x++) {
			px = &(row[x * 4]);
			data[cur] = px[0];
			data[cur+1] = px[1];
			data[cur+2] = px[2];
			data[cur+3] = px[3];
			cur+=4;
		}
	}

	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	gluBuild2DMipmaps(GL_TEXTURE_2D, 4, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
	free(data);
	free(rowPointers);
	return(texID);
}


GLuint createPlanetTexture(void) {
	// source http://paulbourke.net/fractals/noise/
	int i=0, j=0, k=0, x1=0, y1=0, x2=0, y2=0,
		iter=0, test=0;
	float hue=0.0, h=0.0, s=0.0, v=0.0,
		a=0.0, b=0.0, factor=0.0;
	GLuint texID;

	hue = genFlameRand01();
	iter = 1000;
	factor = 0.01;
	cpt=0;
	for (i=0; i<sideLength; i++) {
		for (j=0; j<sideLength; j++) {
			colorsList[cpt] = hue;
			colorsList[cpt+1] = 1.0;
			colorsList[cpt+2] = 0.5;
			cpt+=3;
		}
	}

	for (k=0; k<iter; k++) {
		test = rand() % 2;
		x1 = rand() % sideLength;
		y1 = rand() % sideLength;
		x2 = rand() % sideLength;
		y2 = rand() % sideLength;
		a = (float)(y2 - y1) / (float)(x2 - x1);
		b = (float)(y1 - a*x1);
		cpt=0;
		for (i=0; i<sideLength; i++) {
			for (j=0; j<sideLength; j++) {
				if (test) {
					if (j > a*(float)i + b) { colorsList[cpt+2] += factor; }
					else { colorsList[cpt+2] -= factor; }
				} else {
					if (j > a*(float)i + b) { colorsList[cpt+2] -= factor; }
					else { colorsList[cpt+2] += factor; }
				}
				cpt+=3;
			}
		}
	}

	cpt=0;
	for (i=0; i<sideLength; i++) {
		for (j=0; j<sideLength; j++) {
			h = colorsList[cpt];
			s = colorsList[cpt+1];
			v = colorsList[cpt+2];
			hsv2rgb(h, s, v, &colorsList[cpt], &colorsList[cpt+1], &colorsList[cpt+2]);
			cpt+=3;
		}
	}

	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sideLength, sideLength,
		0, GL_RGB, GL_FLOAT, colorsList);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	return(texID);
}


void assignTexture(void) {
	unsigned long i=0;
	for (i=0; i<iterations; i++) {
		switch (pointsList[i].counter) {
			case 0:
				pointsList[i].counter = loadPNGtexture("texture/earthmap.png");
				break;
			case 1:
				pointsList[i].counter = loadPNGtexture("texture/marsmap.png");
				break;
			case 2:
				pointsList[i].counter = loadPNGtexture("texture/moonmap.png");
				break;
			case 3:
				pointsList[i].counter = loadPNGtexture("texture/venusmap.png");
				break;
			case 4:
				pointsList[i].counter = loadPNGtexture("texture/jupitermap.png");
				break;
			default:
				pointsList[i].counter = createPlanetTexture();
				break;
		}
	}
}


void drawObjects(void) {
	unsigned long i=0;
	GLUquadricObj *object=NULL;
	object = gluNewQuadric();
	objectList = glGenLists(iterations);
	for (i=0; i<iterations; i++) {
		glNewList(objectList+i, GL_COMPILE_AND_EXECUTE);
			glPushMatrix();
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, pointsList[i].counter);
			glTranslatef(pointsList[i].x, pointsList[i].y, pointsList[i].z);
			glRotatef(pointsList[i].rx, 1.0f, 0.0f, 0.0f);
			glRotatef(pointsList[i].ry, 0.0f, 1.0f, 0.0f);
			glRotatef(pointsList[i].rz, 0.0f, 0.0f, 1.0f);
			gluQuadricDrawStyle(object, GLU_FILL); //GLU_FILL, GLU_LINE
			gluQuadricNormals(object, GLU_SMOOTH); // GLU_FLAT
			gluQuadricTexture(object, 1);
			gluSphere(object, sphereRadius, 64, 64);
			glDisable(GL_TEXTURE_2D);
			glPopMatrix();
		glEndList();
	}
	gluDeleteQuadric(object);
}


void displayPlanet(void) {
	unsigned long i=0;
	for (i=0; i<iterations; i++) {
		pointsList[i].x = genDoubleRandom(-12.0, 12.0);
		pointsList[i].y = genDoubleRandom(-12.0, 12.0);
		pointsList[i].z = genDoubleRandom(-12.0, 12.0);
		pointsList[i].rx = genFlameRand01();
		pointsList[i].ry = genFlameRand01();
		pointsList[i].rz = genFlameRand01();
		pointsList[i].rf = genFlameRand01();
		pointsList[i].counter = rand() % 12;
	}
}


void drawBox(vector pos, vector color, float height, float width, float thickness) {
	glLineWidth(1.0);
	glTranslatef(pos.x, pos.y, pos.z);
	glColor3f(color.x, color.y, color.z);

	glBegin(GL_LINES);
	glVertex3f(-width/2.0, -thickness,  height/2.0);
	glVertex3f( width/2.0, -thickness,  height/2.0);
	glVertex3f(-width/2.0,  thickness,  height/2.0);
	glVertex3f( width/2.0,  thickness,  height/2.0);
	glVertex3f(-width/2.0, -thickness,  height/2.0);
	glVertex3f(-width/2.0,  thickness,  height/2.0);
	glVertex3f( width/2.0, -thickness,  height/2.0);
	glVertex3f( width/2.0,  thickness,  height/2.0);

	glVertex3f(-width/2.0, -thickness, -height/2.0);
	glVertex3f( width/2.0, -thickness, -height/2.0);
	glVertex3f(-width/2.0,  thickness, -height/2.0);
	glVertex3f( width/2.0,  thickness, -height/2.0);
	glVertex3f(-width/2.0, -thickness, -height/2.0);
	glVertex3f(-width/2.0,  thickness, -height/2.0);
	glVertex3f( width/2.0, -thickness, -height/2.0);
	glVertex3f( width/2.0,  thickness, -height/2.0);

	glVertex3f(-width/2.0, -thickness,  height/2.0);
	glVertex3f(-width/2.0, -thickness, -height/2.0);
	glVertex3f( width/2.0, -thickness,  height/2.0);
	glVertex3f( width/2.0, -thickness, -height/2.0);
	glVertex3f(-width/2.0,  thickness,  height/2.0);
	glVertex3f(-width/2.0,  thickness, -height/2.0);
	glVertex3f( width/2.0,  thickness,  height/2.0);
	glVertex3f( width/2.0,  thickness, -height/2.0);
	glEnd();
}


void drawWindow(int x, int y) {
	vector pos = {(float)x, 0.0, (float)y};
	vector color = {0.6, 0.6, 0.6};
	float height = 100 * 0.1,
		width = 100 * 0.1,
		thickness = 0.4;
	glPushMatrix();
	drawBox(pos, color, height, width, thickness);
	glPopMatrix();
}


void drawAxes(void) {
	vector pos = {0.0, 0.0, 0.0};
	vector color = {0.6, 0.6, 0.6};
	float height = 100 * 0.5,
		width = 100 * 0.5,
		thickness = 0.8;
	glPushMatrix();
	if (mandelbrot || julia || flame) {
		drawBox(pos, color, height, width, thickness);
	} else if (flame3d) {
		drawBox(pos, color, height, width, thickness*4);
	} else {
		glLineWidth(1.0);
		glTranslatef(0.0, 0.0, 0.0);
		glColor3f(0.8, 0.8, 0.8);
		glutWireCube(100.0 * 0.33);
	}
	glPopMatrix();
}


void onReshape(int width, int height) {
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, width/height, 1.0, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


void onSpecial(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_UP:
			rotx += 5.0;
			printf("INFO: x = %f\n", rotx);
			break;
		case GLUT_KEY_DOWN:
			rotx -= 5.0;
			printf("INFO: x = %f\n", rotx);
			break;
		case GLUT_KEY_LEFT:
			rotz += 5.0;
			printf("INFO: z = %f\n", rotz);
			break;
		case GLUT_KEY_RIGHT:
			rotz -= 5.0;
			printf("INFO: z = %f\n", rotz);
			break;
		default:
			printf("x %d, y %d\n", x, y);
			break;
	}
	glutPostRedisplay();
}


void onMotion(int x, int y) {
	if (prevx) {
		xx += ((x - prevx)/10.0);
		printf("INFO: x = %f\n", xx);
	}
	if (prevy) {
		yy -= ((y - prevy)/10.0);
		printf("INFO: y = %f\n", yy);
	}
	prevx = x;
	prevy = y;
	glutPostRedisplay();
}


void onIdle(void) {
	frame += 1;
	currentTime = glutGet(GLUT_ELAPSED_TIME);
	if (currentTime - timebase >= 1000.0){
		fps = frame*1000.0 / (currentTime-timebase);
		timebase = currentTime;
		frame = 0;
	}
	glutPostRedisplay();
}


void onMouse(int button, int state, int x, int y) {
	switch (button) {
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_DOWN) { // increase
				printf("INFO: left button, x %d, y %d\n", x, y);
				if (mandelbrot || julia) {
					getWorldPos(x, y);
					defineNewParameters(1);
					displayFractal();
				}
			}
			break;
		case GLUT_RIGHT_BUTTON:
			if (state == GLUT_DOWN) { // decrease
				printf("INFO: right button, x %d, y %d\n", x, y);
				if (mandelbrot || julia) {
					getWorldPos(x, y);
					defineNewParameters(0);
					displayFractal();
				}
			}
			break;
	}
}


void onKeyboard(unsigned char key, int x, int y) {
	char *name = malloc(20 * sizeof(char));
	switch (key) {
		case 27: // Escape
			printf("x %d, y %d\n", x, y);
			printf("INFO: exit loop\n");
			glutLeaveMainLoop();
			break;
		case 8: // BackSpace
			if (mandelbrot || julia) {
				blurRadius=1;
				xmin=-2.0; xmax=2.0;
				ymin=-2.0; ymax=2.0;
				cx=xmin; cy=ymin;
				maxIter = 128;
				winPosx = 20; winPosy = 20;
				displayFractal();
			}
			break;
		case 'x':
			xx += 1.0;
			printf("INFO: x = %f\n", xx);
			break;
		case 'X':
			xx -= 1.0;
			printf("INFO: x = %f\n", xx);
			break;
		case 'y':
			yy += 1.0;
			printf("INFO: y = %f\n", yy);
			break;
		case 'Y':
			yy -= 1.0;
			printf("INFO: y = %f\n", yy);
			break;
		case 'f':
			fullScreen = !fullScreen;
			if (fullScreen) {
				glutFullScreen();
			} else {
				glutReshapeWindow(winSizeW, winSizeH);
				glutPositionWindow(100,100);
				printf("INFO: fullscreen %d\n", fullScreen);
			}
			break;
		case 'r':
			rotate = !rotate;
			printf("INFO: rotate %d\n", rotate);
			break;
		case 'c':
			if (saturation) {
				rotColor = !rotColor;
				printf("INFO: rotate color %d\n", rotColor);
			}
			break;
		case 'b':
			if (flame || flame3d) {
				blurRadius++;
				free(flameFuncList);
				flameFuncList = (flameFunction*)calloc(numOfFlameFunct, sizeof(flameFunction));
				displayFlame();
				printf("INFO: blur radius %d\n", blurRadius);
			}
			break;
		case 'q':
			if (julia) {
				rotJulia = !rotJulia;
				printf("INFO: rotate Julia %d\n", rotJulia);
			}
			if (flame || flame3d) {
				blurRadius = 1;
				free(flameFuncList);
				flameFuncList = (flameFunction*)calloc(numOfFlameFunct, sizeof(flameFunction));
				initFlameFunction();
				computeFlame();
				displayFlame();
			}
			break;
		case 's':
			if (mandelbrot || julia) {
				saturation = 1 - saturation;
				printf("INFO: saturation color %d\n", saturation);
				displayFractal();
			}
			break;
		case 'a':
			if (menger) {
				mengerDepth += 1;
				if (mengerDepth == 6) { mengerDepth = 1; }
				xmax = 0;
				cpt = 0;
				mengerIter = pow(20, mengerDepth-1);
				free(cubeList);
				cubeList = (object*)calloc(mengerIter, sizeof(object));
				displayMenger();
			}
			if (sierpinski) {
				sierpinskiDepth += 1;
				if (sierpinskiDepth == 9) { sierpinskiDepth = 1; }
				xmax = 0;
				cpt = 0;
				sierpinskiIter = pow(5, sierpinskiDepth-1);
				free(pyramidList);
				pyramidList = (object*)calloc(sierpinskiIter, sizeof(object));
				displaySierpinski();
			}
			break;
		case 'z':
			zoom -= 5.0;
			if (zoom < 5.0) {
				zoom = 5.0;
			}
			printf("INFO: zoom = %f\n", zoom);
			break;
		case 'Z':
			zoom += 5.0;
			printf("INFO: zoom = %f\n", zoom);
			break;
		case 'p':
			printf("INFO: take a screenshot\n");
			sprintf(name, "capture_%.3d.png", cpt);
			takeScreenshot(name);
			cpt += 1;
			break;
		default:
			break;
	}
	free(name);
	glutPostRedisplay();
}


void onTimer(int event) {
	unsigned long i=0;
	switch (event) {
		case 0:
			break;
		default:
			break;
	}
	prevx = 0.0;
	prevy = 0.0;
	if (rotate) {
		if (flame || flame3d) { roty += 0.2; }
		else { rotz -= 0.2; }
		if (planet) {
			for (i=0; i<iterations; i++) {
				pointsList[i].rx += pointsList[i].rf;
				pointsList[i].ry -= pointsList[i].rf;
				pointsList[i].rz += pointsList[i].rf;
			}
			drawObjects();
		}
	}
	glutPostRedisplay();
	glutTimerFunc(dt, onTimer, 0);
}


void update(int value) {
	unsigned int i=0;
	vector temp;
	switch (value) {
		case 0:
			break;
		default:
			break;
	}
	if (rotColor) {
		if (mandelbrot || julia) {
			for (i=0; i<iterations*3; i+=3) {
				temp = rgb2hsv(colorsList[i], colorsList[i+1], colorsList[i+2]);
				hsv2rgb(temp.x+0.01, 0.75, 0.9, &colorsList[i], &colorsList[i+1], &colorsList[i+2]);
			}
		} else if (flame) {
			for (i=0; i<iterations; i++) {
				if (pointsList[i].r!=0 && pointsList[i].g!=0 && pointsList[i].b!=0) {
					temp = rgb2hsv(pointsList[i].r, pointsList[i].g, pointsList[i].b);
					hsv2rgb(temp.x+0.01, temp.y, temp.z, &(pointsList[i].r), &(pointsList[i].g), &(pointsList[i].b));
				}
			}
		} else if (flame3d) {
			for (i=0; i<iterations*18; i+=3) {
				temp = rgb2hsv(colorsList[i], colorsList[i+1], colorsList[i+2]);
				if (temp.x!=0.0 && temp.y!=0.0 && temp.z!=0.0) {
					hsv2rgb(temp.x+0.01, temp.y, temp.z, &colorsList[i], &colorsList[i+1], &colorsList[i+2]);
				}
			}
		} else {
			for (i=0; i<iterations; i++) {
				temp = rgb2hsv(pointsList[i].r, pointsList[i].g, pointsList[i].b);
				hsv2rgb(temp.x+0.01, 0.75, 0.9, &(pointsList[i].r), &(pointsList[i].g), &(pointsList[i].b));
			}
		}
	}
	if (rotJulia) {
		juliaA = fmod(juliaA+0.005, 2*pi);
		displayFractal();
	}
	glutPostRedisplay();
	glutTimerFunc(dt, update, 0);
}


void display(void) {
	int i = 0;
	unsigned long j = 0;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	drawText();
	glCallList(textList);

	glPushMatrix();
	glTranslatef(xx, yy, -zoom);
	glRotatef(rotx, 1.0f, 0.0f, 0.0f);
	glRotatef(roty, 0.0f, 1.0f, 0.0f);
	glRotatef(rotz, 0.0f, 0.0f, 1.0f);

	GLfloat ambient1[] = {0.15f, 0.15f, 0.15f, 1.0f};
	GLfloat diffuse1[] = {0.8f, 0.8f, 0.8f, 1.0f};
	GLfloat specular1[] = {1.0f, 1.0f, 1.0f, 1.0f};
	GLfloat position1[] = {0.0f, 0.0f, 24.0f, 1.0f};
	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, specular1);
	glLightfv(GL_LIGHT1, GL_POSITION, position1);
	glEnable(GL_LIGHT1);

	drawAxes();

	if (mandelbrot || julia) {
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sideLength, sideLength,
			0, GL_RGB, GL_FLOAT, colorsList);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBegin(GL_QUADS);
			glTexCoord2i(0, 0); glVertex3f(-25.0, 0.0, -25.0);
			glTexCoord2i(1, 0); glVertex3f(25.0, 0.0, -25.0);
			glTexCoord2i(1, 1); glVertex3f(25.0, 0.0, 25.0);
			glTexCoord2i(0, 1); glVertex3f(-25.0, 0.0, 25.0);
		glEnd();
		glDisable(GL_TEXTURE_2D);
		drawWindow(winPosx, winPosy);
	} else if (flame || mandelbulb || ifs) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(point), pointsList);
		glColorPointer(3, GL_FLOAT, sizeof(point), &pointsList[0].r);
		if (flame || mandelbulb) { glDrawArrays(GL_POINTS, 0, iterations); }
		if (ifs) { glDrawArrays(GL_POINTS, 0, iterations*nbrIfs); }
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	} else if (flame3d) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(3, GL_FLOAT, 0, colorsList);
		glVertexPointer(3, GL_FLOAT, 0, verticesList);
		glDrawArrays(GL_TRIANGLES, 0, iterations*6);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	} else if (planet) {
		glPushMatrix();
		for (j=0; j<iterations; j++) { glCallList(objectList+j); }
		glPopMatrix();
	} else if (menger) {
		for (i=0; i<mengerIter; i++) {
			glPushMatrix();
			drawCube(cubeList[i].pos, cubeList[i].color, cubeList[i].size);
			glPopMatrix();
		}
	} else {
		for (i=0; i<sierpinskiIter; i++) {
			glPushMatrix();
			drawPyramid(pyramidList[i].pos, pyramidList[i].color, pyramidList[i].size);
			glPopMatrix();
		}
	}
	glPopMatrix();

	glutSwapBuffers();
	glutPostRedisplay();
}


void init(void) {
	if (background){ // White background
		glClearColor(1.0, 1.0, 1.0, 1.0);
	} else { // Black background
		glClearColor(0.1, 0.1, 0.1, 1.0);
	}

	glEnable(GL_LIGHTING);

	GLfloat ambient[] = {0.05f, 0.05f, 0.05f, 1.0f};
	GLfloat diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
	GLfloat specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
	GLfloat position[] = {0.0f, 0.0f, 0.0f, 1.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, specular);
	glLightfv(GL_LIGHT0, GL_POSITION, position);
	glEnable(GL_LIGHT0);

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	GLfloat matAmbient[] = {0.3f, 0.3f, 0.3f, 1.0f};
	GLfloat matDiffuse[] = {0.6f, 0.6f, 0.6f, 1.0f};
	GLfloat matSpecular[] = {0.8f, 0.8f, 0.8f, 1.0f};
	GLfloat matShininess[] = {128.0f};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShininess);

	GLfloat baseAmbient[] = {0.5f, 0.5f, 0.5f, 0.5f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, baseAmbient);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_NORMALIZE);
	glEnable(GL_AUTO_NORMAL);
	glDepthFunc(GL_LESS);

	glDisable(GL_CULL_FACE);
	if (planet) {
		assignTexture();
		drawObjects();
	}
}


void glmain(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(winSizeW, winSizeH);
	glutInitWindowPosition(120, 10);
	glutCreateWindow(WINDOW_TITLE_PREFIX);
	init();
	glutDisplayFunc(display);
	glutReshapeFunc(onReshape);
	glutSpecialFunc(onSpecial);
	glutMotionFunc(onMotion);
	glutIdleFunc(onIdle);
	glutMouseFunc(onMouse);
	glutKeyboardFunc(onKeyboard);
	glutTimerFunc(dt, onTimer, 0);
	glutTimerFunc(dt, update, 0);
	fprintf(stdout, "INFO: OpenGL Version: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "INFO: FreeGLUT Version: %d\n", glutGet(GLUT_VERSION));
	if (menger) {
		fprintf(stdout, "INFO: Nbr of cubes: %d (%d)\n", cpt, mengerIter);
	} else {
		fprintf(stdout, "INFO: Nbr points: %ld\n", iterations);
	}
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
	fprintf(stdout, "INFO: Freeing memory\n");
	free(colorsList);
	free(verticesList);
	free(pointsList);
	free(cubeList);
	free(pyramidList);
	free(flameFuncList);
	glDeleteLists(textList, 1);
	glDeleteLists(objectList, iterations);
	glDeleteTextures(1, &textureID);
}


void launchDisplay(int argc, char *argv[]) {
	srand(time(NULL));
	if (!strncmp(argv[2], "white", 5)) {
		background = 1;
	}
	if (strcmp(argv[1], "mandelbrot") == 0) { mandelbrot=1; }
	else if (strcmp(argv[1], "julia") == 0) { julia=1; }
	else if (strcmp(argv[1], "mandelbulb") == 0) { mandelbulb=1; }
	else if (strcmp(argv[1], "menger") == 0) { menger=1; }
	else if (strcmp(argv[1], "sierpinski") == 0) { sierpinski=1; }
	else if (strcmp(argv[1], "flame") == 0) { flame=1; }
	else if (strcmp(argv[1], "flame3d") == 0) { flame3d=1; }
	else if (strcmp(argv[1], "ifs") == 0) { ifs=1; }
	else if (strcmp(argv[1], "planet") == 0) { planet=1; }
	else {
		usage();
		exit(EXIT_FAILURE);
	}
	if (mandelbrot || julia) {
		blurRadius = 2;
		xmin=-2.0; xmax=2.0;
		ymin=-2.0; ymax=2.0;
		cx=xmin, cy=ymin,
		sideLength = 1000;
		iterations = sideLength * sideLength;
		maxIter = 128;
		colorsList = (float*)calloc(iterations*3, sizeof(colorsList));
		displayFractal();
	}
	if (mandelbulb) {
		power=5;
		xmin=-1.20; xmax=1.20;
		sideLength = 180;
		iterations = sideLength * sideLength * sideLength;
		maxIter = 20;
		pas = (xmax-xmin)/(float)sideLength;
		pointsList = (point*)calloc(iterations, sizeof(point));
		displayMandelbulb();
	}
	if (menger) {
		mengerDepth = 1;
		xmax = 0;
		cpt = 0;
		mengerIter = pow(20, mengerDepth-1);
		cubeList = (object*)calloc(mengerIter, sizeof(object));
		displayMenger();
	}
	if (sierpinski) {
		sierpinskiDepth = 1;
		xmax = 0;
		cpt = 0;
		sierpinskiIter = pow(5, sierpinskiDepth-1);
		pyramidList = (object*)calloc(sierpinskiIter, sizeof(object));
		displaySierpinski();
	}
	if (flame || flame3d) {
		blurRadius = 1;
		numOfFlameFunct = 16;
		if (flame) { sideLength = 1200; }
		else if (flame3d) { sideLength = 800; }
		xmin=-2.0; xmax=2.0;
		ymin=-2.0; ymax=2.0;
		iterations = sideLength * sideLength;
		flameFuncList = (flameFunction*)calloc(numOfFlameFunct, sizeof(flameFunction));
		pointsList = (point*)calloc(iterations, sizeof(point));
		initFlameFunction();
		computeFlame();
		displayFlame();
	}
	if (ifs) {
		nbrIfs = 10;
		iterations = 100000;
		pointsList = (point*)calloc(iterations*nbrIfs, sizeof(point));
		displayIFSfern();
	}
	if (planet) {
		iterations = 20;
		sphereRadius = 2.0;
		sideLength = 256;
		pointsList = (point*)calloc(iterations, sizeof(point));
		colorsList = (float*)calloc(sideLength*sideLength*3, sizeof(colorsList));
		displayPlanet();
	}
	glmain(argc, argv);
}


int main(int argc, char *argv[]) {
	switch (argc) {
		case 3:
			help();
			launchDisplay(argc, argv);
			exit(EXIT_SUCCESS);
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			break;
	}
}
