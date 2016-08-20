/*visChaos3d
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

#define WINDOW_TITLE_PREFIX "visChaos3d"
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
	objectList = 0,
	cpt = 0,
	animation = 0,
	background = 0;

static float fps = 0.0,
	rotx = -80.0,
	roty = 0.0,
	rotz = 20.0,
	xx = 0.0,
	yy = 5.0,
	zoom = 100.0,
	prevx = 0.0,
	prevy = 0.0,
	sphereRadius = 0.4,
	squareWidth = 0.055;

static double zMax = 0,
	zMin = 0,
	decal = 0;

typedef struct _point {
	GLfloat x, y, z;
	GLfloat r, g, b;
} point;

static point *pointsList = NULL;

static unsigned long iterations = 0,
	current = 1,
	seuil = 50000;




void usage(void) {
	couleur("31");
	printf("Michel Dubois -- visChaos3d -- (c) 2013\n\n");
	couleur("0");
	printf("Syntaxe: visualize3d <iterations> <background color> <attractor> <animation>\n");
	printf("\t<iterations> -> Size of the sample to play with\n");
	printf("\t<background color> -> 'white' or 'black'\n");
	printf("\t<attractor> -> 'lorenz', 'halvorsen', 'chua', 'ikeda', 'thomas' or 'rossler'\n");
	printf("\t<animation> -> 'static' 'dynamic'\n");
}


void help(void) {
	couleur("31");
	printf("Michel Dubois -- visChaos3d -- (c) 2013\n\n");
	couleur("0");
	printf("Key usage:\n");
	printf("\t'ESC' key to quit\n");
	printf("\t'UP', 'DOWN', 'LEFT' and 'RIGHT' keys to rotate manually\n");
	printf("\t'r' to rotate continuously\n");
	printf("\t'x' and 'X' to move to right and left\n");
	printf("\t'y' and 'Y' to move to top and bottom\n");
	printf("\t'z' and 'Z' to zoom in or out\n");
	printf("\t'f' to switch to full screen\n");
	printf("\t'p' to take a screenshot\n");
	printf("\n");
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


void drawPoint(point p) {
	glPointSize(1.0);
	glColor3f(p.r, p.g, p.b);
	glBegin(GL_POINTS);
	glNormal3f(p.x, p.y, p.z);
	glVertex3f(p.x, p.y, p.z);
	glEnd();
}


void drawSphere(point p) {
	glColor3f(p.r, p.g, p.b);
	glTranslatef(p.x, p.y, p.z);
	glutSolidSphere(sphereRadius, 6, 6);
}


void drawSquare(point p) {
	glColor3f(p.r, p.g, p.b);
	glTranslatef(p.x, p.y, p.z);
	glBegin(GL_QUADS);
	glVertex3f(-squareWidth, -squareWidth, 0.0); // Bottom left corner
	glVertex3f(-squareWidth, squareWidth, 0.0); // Top left corner
	glVertex3f(squareWidth, squareWidth, 0.0); // Top right corner
	glVertex3f(squareWidth, -squareWidth, 0.0); // Bottom right corner
	glEnd();
}


void drawLine(point p1, point p2){
	glLineWidth(1.0);
	glBegin(GL_LINES);
	glColor3f(p1.r, p1.g, p1.b);
	glNormal3f(p1.x, p1.y, p1.z);
	glVertex3f(p1.x, p1.y, p1.z);
	glVertex3f(p2.x, p2.y, p2.z);
	glEnd();
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
	if (animation) {
		sprintf(text1, "dt: %1.3f, FPS: %4.2f, current iteration: %lu", (dt/1000.0), fps, current);
	} else {
		sprintf(text1, "dt: %1.3f, FPS: %4.2f", (dt/1000.0), fps);
	}
	sprintf(text2, "Nbr itérations: %ld, zMin: %f, zMax: %f", iterations, floor(zMin), floor(zMax));
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


void drawObject(void) {
	unsigned long i;
	for (i=0; i<iterations; i++) {
		pointsList[i].z = pointsList[i].z - decal;
	}
	if (iterations <= seuil) {
		objectList = glGenLists(1);
		glNewList(objectList, GL_COMPILE_AND_EXECUTE);
		for (i=1; i<iterations; i++) {
			glPushMatrix();
			//drawLine(pointsList[i-1], pointsList[i]);
			//drawPoint(pointsList[i]);
			drawSphere(pointsList[i]);
			glPopMatrix();
		}
		glEndList();
	}
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


void update(int value) {
	switch (value) {
		case 0:
			break;
		default:
			break;
	}
	if (current<iterations) {
		current += 20;
	}
	glutPostRedisplay();
	glutTimerFunc(dt, update, 0);
}


void display(void) {
	unsigned long i;
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

	if (animation) {
		for (i=1; i<current; i++) {
			glPushMatrix();
			drawLine(pointsList[i-1], pointsList[i]);
			glPopMatrix();
		}
		glPushMatrix();
		drawSphere(pointsList[current]);
		glPopMatrix();
	} else {
		if (iterations >= seuil) {
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_COLOR_ARRAY);
			glVertexPointer(3, GL_FLOAT, sizeof(point), pointsList);
			glColorPointer(3, GL_FLOAT, sizeof(point), &pointsList[0].r);
			glDrawArrays(GL_POINTS, 0, iterations);
			glDisableClientState(GL_COLOR_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);
		} else {
			glCallList(objectList);
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

	drawObject();
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
	fprintf(stdout, "INFO: Min: %.02lf, Max: %.02lf\n", zMin, zMax);
	fprintf(stdout, "INFO: Nbr elts: %ld\n", iterations);
	glutMainLoop();
	glDeleteLists(textList, 1);
	glDeleteLists(objectList, 1);
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


void lorenzAttractor(void) {
	// https://en.wikipedia.org/wiki/Lorenz_system
	unsigned long i;
	double hue=0, dtime=0.001, x=0, y=0, z=0;
	double r=28.0, s=10.0, b=8.0/3.0;
	pointsList = (point*)calloc(iterations, sizeof(point));
	decal = zoom/4.0;
	for (i=0; i<iterations; i++) {
		hue = (double)i / (double)iterations;
		hsv2rgb(hue, 0.8, 0.8, &(pointsList[i].r), &(pointsList[i].g), &(pointsList[i].b));
		if (i==0) {
			pointsList[i].x = 0.1;
			pointsList[i].y = 0.0;
			pointsList[i].z = 0.0;
		} else {
			x = pointsList[i-1].x;
			y = pointsList[i-1].y;
			z = pointsList[i-1].z;
			pointsList[i].x = x + dtime * (s * (y-x));
			pointsList[i].y = y + dtime * ((r*x) - y - (x*z));
			pointsList[i].z = z + dtime * ((x*y) - (b*z));
			if (zMax < pointsList[i].z) { zMax = pointsList[i].z; }
			if (zMin > pointsList[i].z) { zMin = pointsList[i].z; }
		}
	}
}


void thomasAttractor(void) {
	//https://en.wikipedia.org/wiki/Thomas%27_cyclically_symmetric_attractor
	unsigned long i;
	double hue=0, dtime=0.001, x=0, y=0, z=0;
	double b=0.1998;
	pointsList = (point*)calloc(iterations, sizeof(point));
	decal = 0;
	for (i=0; i<iterations; i++) {
		hue = (double)i / (double)iterations;
		hsv2rgb(hue, 0.8, 0.8, &(pointsList[i].r), &(pointsList[i].g), &(pointsList[i].b));
		if (i==0) {
			pointsList[i].x = 0.1;
			pointsList[i].y = 0.0;
			pointsList[i].z = 0.0;
		} else {
			x = pointsList[i-1].x;
			y = pointsList[i-1].y;
			z = pointsList[i-1].z;
			pointsList[i].x = x + dtime * (sin(y) - (b*x));
			pointsList[i].y = y + dtime * (sin(z) - (b*y));
			pointsList[i].z = z + dtime * (sin(x) - (b*z));
			if (zMax < pointsList[i].z) { zMax = pointsList[i].z; }
			if (zMin > pointsList[i].z) { zMin = pointsList[i].z; }
		}
	}
}


void rosslerAttractor(void) {
	// https://en.wikipedia.org/wiki/R%C3%B6ssler_attractor
	unsigned long i;
	double hue=0, dtime=0.01, x=0, y=0, z=0;
	double a=0.10, b=0.10, c=14.0;
	pointsList = (point*)calloc(iterations, sizeof(point));
	decal = zoom/4.0;
	for (i=0; i<iterations; i++) {
		hue = (double)i / (double)iterations;
		hsv2rgb(hue, 0.8, 0.8, &(pointsList[i].r), &(pointsList[i].g), &(pointsList[i].b));
		if (i==0) {
			pointsList[i].x = 0.1;
			pointsList[i].y = 0.0;
			pointsList[i].z = 0.0;
		} else {
			x = pointsList[i-1].x;
			y = pointsList[i-1].y;
			z = pointsList[i-1].z;
			pointsList[i].x = x + dtime * (-y -z);
			pointsList[i].y = y + dtime * (x + (a * y));
			pointsList[i].z = z + dtime * (b + (z * (x - c)));
			if (zMax < pointsList[i].z) { zMax = pointsList[i].z; }
			if (zMin > pointsList[i].z) { zMin = pointsList[i].z; }
		}
	}
}


void halvorsenAttractor(void) {
	unsigned long i;
	double hue=0, dtime=0.001, x=0, y=0, z=0;
	double a=1.4, b=4.0;
	pointsList = (point*)calloc(iterations, sizeof(point));
	decal = 0;
	for (i=0; i<iterations; i++) {
		hue = (double)i / (double)iterations;
		hsv2rgb(hue, 0.8, 0.8, &(pointsList[i].r), &(pointsList[i].g), &(pointsList[i].b));
		if (i==0) {
			pointsList[i].x = -5.0;
			pointsList[i].y = 0.0;
			pointsList[i].z = 0.0;
		} else {
			x = pointsList[i-1].x;
			y = pointsList[i-1].y;
			z = pointsList[i-1].z;
			pointsList[i].x = x + dtime * (-(a*x) - (b*y) - (b*z) - pow(y,2));
			pointsList[i].y = y + dtime * (-(a*y) - (b*z) - (b*x) - pow(z,2));
			pointsList[i].z = z + dtime * (-(a*z) - (b*x) - (b*y) - pow(x,2));
			if (zMax < pointsList[i].z) { zMax = pointsList[i].z; }
			if (zMin > pointsList[i].z) { zMin = pointsList[i].z; }
		}
	}
}


void chuaAttractor(void) {
	unsigned long i;
	double hue=0, dtime=0.01, x=0, y=0, z=0;
	double c1=15.6, c2=1.0, c3=28.0, m0=-1.143, m1=-0.714, h=0.0;
	pointsList = (point*)calloc(iterations, sizeof(point));
	decal = 0;
	for (i=0; i<iterations; i++) {
		hue = (double)i / (double)iterations;
		hsv2rgb(hue, 0.8, 0.8, &(pointsList[i].r), &(pointsList[i].g), &(pointsList[i].b));
		if (i==0) {
			pointsList[i].x = 0.7;
			pointsList[i].y = 0.0;
			pointsList[i].z = 0.0;
		} else {
			h = m1*x+(m0-m1)/2*(fabs(x+1)-fabs(x-1));
			x = pointsList[i-1].x;
			y = pointsList[i-1].y;
			z = pointsList[i-1].z;
			pointsList[i].x = x + dtime * (c1 * (y - x - h));
			pointsList[i].y = y + dtime * (c2 * (x - y + z));
			pointsList[i].z = z + dtime * (-c3 * y);
			if (zMax < pointsList[i].z) { zMax = pointsList[i].z; }
			if (zMin > pointsList[i].z) { zMin = pointsList[i].z; }
		}
	}
}


void ikedaAttractor(void) {
	unsigned long i;
	double hue=0, dtime=0.1, x=0, y=0, z=0;
	double a=1.0, b=0.9, c=0.4, d=3.0;
	pointsList = (point*)calloc(iterations, sizeof(point));
	decal = 0;
	for (i=0; i<iterations; i++) {
		hue = (double)i / (double)iterations;
		hsv2rgb(hue, 0.8, 0.8, &(pointsList[i].r), &(pointsList[i].g), &(pointsList[i].b));
		if (i==0) {
			pointsList[i].x = 0.0;
			pointsList[i].y = 0.0;
			pointsList[i].z = 0.0;
		} else {
			x = pointsList[i-1].x;
			y = pointsList[i-1].y;
			z = pointsList[i-1].z;
			pointsList[i].x = x + dtime * (a + b*(x*cos(z) - y*sin(z)));
			pointsList[i].y = y + dtime * (b*(x*sin(z) + y*cos(z)));
			pointsList[i].z = z + dtime * (c - d/(1.0+pow(x,2)+pow(y,2)));
			if (zMax < pointsList[i].z) { zMax = pointsList[i].z; }
			if (zMin > pointsList[i].z) { zMin = pointsList[i].z; }
		}
	}
}


void launchDisplay(int argc, char *argv[]) {
	iterations = atoi(argv[1]);
	if (!strncmp(argv[2], "white", 5)) {
		background = 1;
	}
	if (strcmp(argv[3], "lorenz") == 0) {
		lorenzAttractor();
	} else if (strcmp(argv[3], "thomas") == 0) {
		thomasAttractor();
	} else if (strcmp(argv[3], "rossler") == 0) {
		rosslerAttractor();
	} else if (strcmp(argv[3], "halvorsen") == 0) {
		halvorsenAttractor();
	} else if (strcmp(argv[3], "chua") == 0) {
		chuaAttractor();
	} else if (strcmp(argv[3], "ikeda") == 0) {
		ikedaAttractor();
	} else {
		usage();
		exit(EXIT_FAILURE);
	}
	if (!strncmp(argv[4], "dynamic", 7)) {
		animation = 1;
	}
	glmain(argc, argv);
}


int main(int argc, char *argv[]) {
	switch (argc) {
		case 5:
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
