/*visMandel3d
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

// http://www.mpipks-dresden.mpg.de/~tisean/TISEAN_2.1/docs/chaospaper/node6.html

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <png.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#define WINDOW_TITLE_PREFIX "visMandel3d"
#define couleur(param) printf("\033[%sm",param)

static short winSizeW = 920,
	winSizeH = 690,
	frame = 0,
	currentTime = 0,
	timebase = 0,
	fullScreen = 0,
	rotate = 0,
	dt = 20; // in milliseconds

static int textList = 0,
	cpt = 0,
	background = 0,
	maxIter = 16;

static float fps = 0.0,
	rotx = -80.0,
	roty = 0.0,
	rotz = 20.0,
	xx = 0.0,
	yy = 5.0,
	zoom = 100.0,
	prevx = 0.0,
	prevy = 0.0;

typedef struct _point {
	GLfloat x, y, z;
	GLfloat r, g, b;
} point;

static point *pointsList = NULL;

static unsigned long iterations = 0;




void usage(void) {
	couleur("31");
	printf("Michel Dubois -- visMandel3d -- (c) 2013\n\n");
	couleur("0");
	printf("Syntaxe: visualize3d <background color>\n");
	printf("\t<background color> -> 'white' or 'black'\n");
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
	glScalef(0.01, 0.01, 0.01);
	for(i=0; i < strlen(text); i++) {
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, (int)text[i]);
	}
	glPopMatrix();
}


void drawText(void) {
	char text1[70], text2[70];
	sprintf(text1, "Nbr of points: %ld, Max iterations: %d", iterations, maxIter);
	sprintf(text2, "dt: %1.3f, FPS: %4.2f", (dt/1000.0), fps);
	textList = glGenLists(1);
	glNewList(textList, GL_COMPILE);
	drawString(-40.0, -40.0, -100.0, text1);
	drawString(-40.0, -38.0, -100.0, text2);
	glEndList();
}


void drawAxes(void) {
	glPushMatrix();
	glLineWidth(1.0);
	glColor3f(0.8, 0.8, 0.8);
	glTranslatef(0.0, 0.0, 0.0);
	glutWireCube(100.0/2.0);
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
			if (state == GLUT_DOWN) {
				printf("INFO: left button, x %d, y %d\n", x, y);
			}
			break;
		case GLUT_RIGHT_BUTTON:
			if (state == GLUT_DOWN) {
				printf("INFO: right button, x %d, y %d\n", x, y);
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


void display(void) {
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
	drawAxes();

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(point), pointsList);
	glColorPointer(3, GL_FLOAT, sizeof(point), &pointsList[0].r);
	glDrawArrays(GL_POINTS, 0, iterations);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

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

	GLfloat position[] = {0.0, 0.0, 0.0, 1.0};
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	GLfloat modelAmbient[] = {1.0, 1.0, 1.0, 1.0};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, modelAmbient);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);

	GLfloat no_mat[] = {0.0, 0.0, 0.0, 1.0};
	GLfloat mat_diffuse[] = {0.1, 0.5, 0.8, 1.0};
	GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
	GLfloat shininess[] = {128.0};
	glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);

	glEnable(GL_NORMALIZE);
	glEnable(GL_AUTO_NORMAL);
	glDepthFunc(GL_LESS);
}


void glmain(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitWindowSize(winSizeW, winSizeH);
	glutInitWindowPosition(120, 10);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
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
	fprintf(stdout, "INFO: OpenGL Version: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "INFO: Nbr points: %ld\n", iterations);
	glutMainLoop();
	glDeleteLists(textList, 1);
}


void hsv2rgb(double h, double s, double v, GLfloat *r, GLfloat *g, GLfloat *b) {
	double hp = h * 6;
	if ( hp == 6 ) hp = 0;
	int i = floor(hp);
	double v1 = v * (1 - s),
		v2 = v * (1 - s * (hp - i)),
		v3 = v * (1 - s * (1 - (hp - i)));
	if (i == 0) { *r=v; *g=v3; *b=v1; }
	else if (i == 1) { *r=v2; *g=v; *b=v1; }
	else if (i == 2) { *r=v1; *g=v; *b=v3; }
	else if (i == 3) { *r=v1; *g=v2; *b=v; }
	else if (i == 4) { *r=v3; *g=v1; *b=v; }
	else { *r=v; *g=v1; *b=v2; }
}


int mandelbrot(float x0, float y0, int iMax) {
	// how many iterations we need to get infinity
	float x=0, y=0, xn=0, yn=0;
	int i=0;
	x = x0;
	y = y0;
	for (i=0; i<iMax; i++) {
		xn = x*x - y*y + x0;
		yn = 2 * x * y + y0;
		if (xn*xn + yn*yn > 8) {
			return(i);
		}
		x = xn;
		y = yn;
	}
	return(iMax);
}


int mandelbulb(float x0, float y0, float z0, int iMax, float power) {
	// how many iterations we need to get infinity
	float x=0, y=0, z=0, xn=0, yn=0, zn=0,
		r=0, theta=0, phi=0;
	int i=0;
	x = x0;
	y = y0;
	z = z0;
	for (i=0; i<iMax; i++) {
		r = sqrt(x*x + y*y + z*z);
		theta = atan2(sqrt(x*x + y*y), z);
		phi = atan2(y, x);

		xn = pow(r, power) * sin(theta*power) * cos(phi*power) + x0;
		yn = pow(r, power) * sin(theta*power) * sin(phi*power) + y0;
		zn = pow(r, power) * cos(theta*power) + z0;

		if (xn*xn + yn*yn + zn*zn > 8) {
			return(i);
		}
		x = xn;
		y = yn;
		z = zn;
	}
	return(iMax);
}


void displayMandelbrot(void) {
	unsigned long i=0;
	double hue=0;
	float x=0.0, z=0.0, pas=0.002,
		minCoordonate=-2.0, maxCoordonate=2.0;
	int m=0;
	iterations = 1 + ((maxCoordonate-minCoordonate)/pas) * ((maxCoordonate-minCoordonate)/pas);
	pointsList = (point*)calloc(iterations, sizeof(point));
	x = minCoordonate;
	z = minCoordonate;
	for (i=0; i<iterations; i++) {
		m = mandelbrot(x, z, maxIter);
		if (m < maxIter) {
			hue = (double)m / (double)maxIter;
			hsv2rgb(hue, 0.8, 0.8, &(pointsList[i].r), &(pointsList[i].g), &(pointsList[i].b));
			pointsList[i].x = x * 10;
			pointsList[i].y = 0.0;
			pointsList[i].z = z * 10;
		}
		x += pas;
		if (x>=maxCoordonate) {
			x = minCoordonate;
			z += pas;
		}
	}
}


void displayMandelbulb(void) {
	unsigned long i=0;
	double hue=0;
	float x=0.0, y=0.0, z=0.0, pas=0.016, power=3,
		minCoordonate=-1.2, maxCoordonate=1.2;
	int m=0;
	iterations = ((maxCoordonate-minCoordonate)/pas) * ((maxCoordonate-minCoordonate)/pas) * ((maxCoordonate-minCoordonate)/pas);
	pointsList = (point*)calloc(iterations, sizeof(point));
	x = minCoordonate;
	y = minCoordonate;
	z = minCoordonate;
	for (i=0; i<iterations; i++) {
		m = mandelbulb(x, y, z, maxIter, power);
		if (m >= maxIter) {
			if ((x>y) & (x>z)) {
				hue = (double)x / (double)maxCoordonate;
			} else if ((y>x) & (y>z)) {
				hue = (double)y / (double)maxCoordonate;
			} else if ((z>x) & (z>y)) {
				hue = (double)z / (double)maxCoordonate;
			} else {
				hue = 0.8;
			}
			hsv2rgb(hue, 0.6, 0.8, &(pointsList[i].r), &(pointsList[i].g), &(pointsList[i].b));
			pointsList[i].x = x * 20;
			pointsList[i].y = y * 20;
			pointsList[i].z = z * 20;
		}
		x += pas;
		if (x>=maxCoordonate) {
			x = minCoordonate;
			z += pas;
			if (z>=maxCoordonate) {
				x = minCoordonate;
				z = minCoordonate;
				y += pas;
			}
		}
	}
}


void launchDisplay(int argc, char *argv[]) {
	if (!strncmp(argv[1], "white", 5)) {
		background = 1;
	}
	//displayMandelbrot();
	displayMandelbulb();
	glmain(argc, argv);
}


int main(int argc, char *argv[]) {
	switch (argc) {
		case 2:
			launchDisplay(argc, argv);
			exit(EXIT_SUCCESS);
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			break;
	}
}
