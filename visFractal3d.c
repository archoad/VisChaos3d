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

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#define WINDOW_TITLE_PREFIX "visFractal3d"
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
	mandelbulb = 0, mandelbrot = 0, julia = 0, menger = 0, sierpinski = 0,
	mengerDepth = 0, sierpinskiDepth = 0,
	flat = 0, dt = 20; // in milliseconds

static int textList = 0,
	cpt = 0,
	background = 0,
	maxIter = 0,
	mengerIter = 0, sierpinskiIter = 0,
	sideLength = 0,
	winPosx = 20,
	winPosy = 20;

static float fps = 0.0,
	rotx = -80.0,
	roty = 0.0,
	rotz = 10.0,
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
	cr=0, ci=0,
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
} point;

static float *colorsList = NULL;
static point *pointsList = NULL;
static object *cubeList = NULL;
static object *pyramidList = NULL;

static unsigned long iterations = 0;

static GLuint textureID;

static int mengerRemovals[7] = {4, 10, 12, 13, 14, 16, 22};


void usage(void) {
	couleur("31");
	printf("Michel Dubois -- visFractal3d -- (c) 2013\n\n");
	couleur("0");
	printf("Syntaxe: visFractal3d <fractal> <background color>\n");
	printf("\t<fractal> -> 'mandelbrot', 'julia', 'menger', 'sierpinski' or 'mandelbulb'\n");
	printf("\t<background color> -> 'white' or 'black'\n");
}


void help(void) {
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
	printf("\t'q' while displaying julia set, rotate variables\n");
	printf("Mouse usage:\n");
	printf("\t'LEFT CLICK' to zoom into the fractal\n");
	printf("\n");
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


void getWorldPos(int x, int y) {
	GLint viewport[4];
	GLdouble modelview[16];
	GLdouble projection[16];
	GLfloat winX, winY, winZ;
	GLdouble posX, posY, posZ;

	glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewport );

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


void displayFractal(void) {
	// https://rosettacode.org/wiki/Mandelbrot_set
	// https://rosettacode.org/wiki/Julia_set
	unsigned long cpt=0, cur=0;
	int i=0, j=0, iter=0, min=0, max=0;
	double x=0, y=0, zr=0, zi=0, zr2=0, zi2=0, tmp=0;
	float hue=0, val=0;
	float hueList[iterations];

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
	for (j=0; j<sideLength; j++) {
		for (i=0; i<sideLength; i++) {
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
			colorsList[cur+3] = 1.0;
			cpt++;
			cur+=4;
		}
	}
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
			if ((x>y) && (x>z)) {
				hue = (float)x / (float)xmax;
			} else if ((y>x) && (y>z)) {
				hue = (float)y / (float)xmax;
			} else if ((z>x) && (z>y)) {
				hue = (float)z / (float)xmax;
			} else {
				hue = 0.0;
			}
			hsv2rgb(hue, 0.75, 0.9, &(pointsList[i].r), &(pointsList[i].g), &(pointsList[i].b));
			pointsList[i].x = x * 10.0;
			pointsList[i].y = y * 10.0;
			pointsList[i].z = z * 10.0;
		}
		x += pas;
		if ((i!=0) && (i%sideLength==0)) {
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
	}else {
		sprintf(text1, "Nbr of points: %ld, Max iterations: %d", iterations, maxIter);
	}
	if (julia) {
		sprintf(text2, "dt: %1.3f, FPS: %4.2f, cr=%0.3f ci=%0.3f", (dt/1000.0), fps, cr, ci);
	} else {
		sprintf(text2, "dt: %1.3f, FPS: %4.2f", (dt/1000.0), fps);
	}
	textList = glGenLists(1);
	glNewList(textList, GL_COMPILE);
	drawString(-40.0, -40.0, -100.0, text1);
	drawString(-40.0, -38.0, -100.0, text2);
	glEndList();
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
	if (flat) {
		drawBox(pos, color, height, width, thickness);
	} else {
		glLineWidth(1.0);
		glTranslatef(0.0, 0.0, 0.0);
		glColor3f(0.8, 0.8, 0.8);
		glutWireCube(100.0 * 0.33);
	}
	glPopMatrix();
}


void drawTexture(void) {
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sideLength, sideLength,
		0, GL_RGBA, GL_FLOAT, colorsList);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
				getWorldPos(x, y);
				defineNewParameters(1);
				displayFractal();
			}
			break;
		case GLUT_RIGHT_BUTTON:
			if (state == GLUT_DOWN) { // decrease
				printf("INFO: right button, x %d, y %d\n", x, y);
				getWorldPos(x, y);
				defineNewParameters(0);
				displayFractal();
			}
			break;
	}
}


void onKeyboard(unsigned char key, int x, int y) {
	char *name = malloc(20 * sizeof(char));
	switch (key) {
		case 27: // Escape
			printf("INFO: exit\n");
			printf("x %d, y %d\n", x, y);
			exit(0);
			break;
		case 8: // BackSpace
			if (flat) {
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
		case 'q':
			if (julia) {
				rotJulia = !rotJulia;
				printf("INFO: rotate Julia %d\n", rotJulia);
			}
			break;
		case 's':
			saturation = 1 - saturation;
			printf("INFO: saturation color %d\n", saturation);
			displayFractal();
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
	switch (event) {
		case 0:
			break;
		default:
			break;
	}
	prevx = 0.0;
	prevy = 0.0;
	if (rotate) {
		rotz -= 0.2;
	} else {
		rotz += 0.0;
	}
	if (rotz > 360) rotz = 360;
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
		if (flat) {
			for (i=0; i<iterations*4; i+=4) {
				temp = rgb2hsv(colorsList[i], colorsList[i+1], colorsList[i+2]);
				hsv2rgb(temp.x+0.01, 0.75, 0.9, &colorsList[i], &colorsList[i+1], &colorsList[i+2]);
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	drawText();
	glCallList(textList);

	glPushMatrix();
	glTranslatef(xx, yy, -zoom);
	glRotatef(rotx, 1.0, 0.0, 0.0);
	glRotatef(roty, 0.0, 1.0, 0.0);
	glRotatef(rotz, 0.0, 0.0, 1.0);

	GLfloat ambient1[] = {0.15f, 0.15f, 0.15f, 1.0f};
	GLfloat diffuse1[] = {0.8f, 0.8f, 0.8f, 1.0f};
	GLfloat specular1[] = {1.0f, 1.0f, 1.0f, 1.0f};
	GLfloat position1[] = {0.0f, 0.0f, 20.0f, 1.0f};
	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, specular1);
	glLightfv(GL_LIGHT1, GL_POSITION, position1);
	glEnable(GL_LIGHT1);

	drawAxes();

	if (flat) {
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &textureID);
		drawTexture();
		glBegin(GL_QUADS);
			glTexCoord2i(0, 0); glVertex3f(-25.0, 0.0, -25.0);
			glTexCoord2i(1, 0); glVertex3f(25.0, 0.0, -25.0);
			glTexCoord2i(1, 1); glVertex3f(25.0, 0.0, 25.0);
			glTexCoord2i(0, 1); glVertex3f(-25.0, 0.0, 25.0);
		glEnd();
		glDisable(GL_TEXTURE_2D);
		drawWindow(winPosx, winPosy);
	} else if (mandelbulb) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(point), pointsList);
		glColorPointer(3, GL_FLOAT, sizeof(point), &pointsList[0].r);
		glDrawArrays(GL_POINTS, 0, iterations);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
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

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &textureID);
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
	if (menger) {
		fprintf(stdout, "INFO: Nbr of cubes: %d (%d)\n", cpt, mengerIter);
	} else {
		fprintf(stdout, "INFO: Nbr points: %ld\n", iterations);
	}
	glutMainLoop();
	free(colorsList);
	free(pointsList);
	free(cubeList);
	free(pyramidList);
	glDeleteLists(textList, 1);
	glDeleteTextures(1, &textureID);
}


void launchDisplay(int argc, char *argv[]) {
	if (!strncmp(argv[2], "white", 5)) {
		background = 1;
	}
	if (strcmp(argv[1], "mandelbrot") == 0) { mandelbrot=1; }
	else if (strcmp(argv[1], "julia") == 0) { julia=1; }
	else if (strcmp(argv[1], "mandelbulb") == 0) { mandelbulb=1; }
	else if (strcmp(argv[1], "menger") == 0) { menger=1; }
	else if (strcmp(argv[1], "sierpinski") == 0) { sierpinski=1; }
	else {
		usage();
		exit(EXIT_FAILURE);
	}
	if ((mandelbrot) || (julia)) {
		flat = 1;
		xmin=-2.0; xmax=2.0;
		ymin=-2.0; ymax=2.0;
		cx=xmin, cy=ymin,
		sideLength = 600;
		iterations = sideLength * sideLength;
		maxIter = 128;
		colorsList = (float*)calloc(iterations*4, sizeof(colorsList));
		displayFractal();
	}
	if (mandelbulb) {
		power=5;
		xmin=-1.20; xmax=1.20;
		sideLength = 200;
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
