#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include "../GL/glew.h"
#include "../GL/freeglut.h"
#include "../shader_lib/shader.h"

#define WINDOW_WIDTH 200		// x, up(-)/down(+)
#define WINDOW_HEIGHT 200		// y, left(-)/right(+)
#define D(ax, ay, bx, by) ((ax-bx)*(ax-bx) + (ay-by)*(ay-by))

void init(void);
void display(void);
void reshape(int width, int height);
void keyboard(unsigned char key, int x, int y);
void mouseButton(int button, int state, int x, int y);
void mouseMove(int x, int y);
void idle(void);

typedef struct _Rgb {
	float r, g, b;
}Rgb;

GLuint program;
GLuint vao, vbo;
GLfloat canvasArray[WINDOW_WIDTH][WINDOW_HEIGHT * 6];
GLfloat wetCanvas[WINDOW_WIDTH][WINDOW_HEIGHT * 3] = {0.0}, dryCanvas[WINDOW_WIDTH][WINDOW_HEIGHT * 3] = { 0.0 };
Rgb color[4] = { {1.0, 1.0, 1.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0} };
int colori = 0, innerR = 5, outerR = 15;
int lastX, lastY;
float inkMove = 1.0, inkMove_descent = 0.9, inkTime_descent = 0.995, wetSpread = 0.02, waterCoef = 1.2;
int inkTime_limitBase = 100, wetSpread_num = 0;
int canvasTime[WINDOW_WIDTH][WINDOW_HEIGHT] = { 0 }, inkTime_limit[WINDOW_WIDTH][WINDOW_HEIGHT];

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow("NPR hw2 - 0556619");
	glutReshapeWindow(WINDOW_WIDTH, WINDOW_HEIGHT);

	glewInit();

	init();

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouseButton);
	glutMotionFunc(mouseMove);

	glutMainLoop();

	return 0;
}

void init(void) 
{
	// OpenGL, GLSL version
	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("\n");

	// shader
	GLuint vert = createShader("../Shaders/hw2.vert", "vertex");
	GLuint frag = createShader("../Shaders/hw2.frag", "fragment");
	program = createProgram(vert, frag);

	// canvas
	for (int i = 0; i < WINDOW_WIDTH; i++) {
		for (int j = 0; j < WINDOW_HEIGHT; j++) {
			canvasArray[i][6 * j] = 2.0 * i / WINDOW_WIDTH - 1;			// x
			canvasArray[i][6 * j + 1] = -2.0 * j / WINDOW_HEIGHT + 1;	// y
			canvasArray[i][6 * j + 2] = 0.0f;	// z
			canvasArray[i][6 * j + 3] = 0.0f;	// r
			canvasArray[i][6 * j + 4] = 0.0f;	// g
			canvasArray[i][6 * j + 5] = 0.0f;	// b
			
			inkTime_limit[i][j] = inkTime_limitBase + 50 * (1.2 - waterCoef);
		}
	}

	// vao, vbo
	glGenBuffers(1, &vbo);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * WINDOW_WIDTH * WINDOW_HEIGHT * 6, canvasArray, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(NULL);
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (int i = 0; i < WINDOW_WIDTH; i++) {
		for (int j = 0; j < WINDOW_HEIGHT; j++) {
			// ink time descent
			if (canvasTime[i][j] < inkTime_limit[i][j]) {
				wetCanvas[i][3 * j] *= inkTime_descent;
				wetCanvas[i][3 * j + 1] *= inkTime_descent;
				wetCanvas[i][3 * j + 2] *= inkTime_descent;

				// wet spread
				if (i + 1 < WINDOW_WIDTH) {
					wetCanvas[i + 1][3 * j] += wetSpread*wetCanvas[i][3 * j];
					wetCanvas[i + 1][3 * j + 1] += wetSpread*wetCanvas[i][3 * j + 1];
					wetCanvas[i + 1][3 * j + 2] += wetSpread*wetCanvas[i][3 * j + 2];
					wetSpread_num++;
				}
				else if (i - 1 >= 0) {
					wetCanvas[i - 1][3 * j] += wetSpread*wetCanvas[i][3 * j];
					wetCanvas[i - 1][3 * j + 1] += wetSpread*wetCanvas[i][3 * j + 1];
					wetCanvas[i - 1][3 * j + 2] += wetSpread*wetCanvas[i][3 * j + 2];
					wetSpread_num++;
				}
				else if (j + 1 < WINDOW_HEIGHT) {
					wetCanvas[i][3 * (j + 1)] += wetSpread*wetCanvas[i][3 * j];
					wetCanvas[i][3 * (j + 1) + 1] += wetSpread*wetCanvas[i][3 * j + 1];
					wetCanvas[i][3 * (j + 1) + 2] += wetSpread*wetCanvas[i][3 * j + 2];
					wetSpread_num++;
				}
				else if (j - 1 >= 0) {
					wetCanvas[i][3 * (j - 1)] += wetSpread*wetCanvas[i][3 * j];
					wetCanvas[i][3 * (j - 1) + 1] += wetSpread*wetCanvas[i][3 * j + 1];
					wetCanvas[i][3 * (j - 1) + 2] += wetSpread*wetCanvas[i][3 * j + 2];
					wetSpread_num++;
				}
				wetCanvas[i][3 * j] *= 1 - wetSpread_num * wetSpread * waterCoef;
				wetCanvas[i][3 * j + 1] *= 1 - wetSpread_num * wetSpread * waterCoef;
				wetCanvas[i][3 * j + 2] *= 1 - wetSpread_num * wetSpread * waterCoef;
				wetSpread_num = 0;

				canvasTime[i][j]++;
			}
			else {
				dryCanvas[i][3 * j] += wetCanvas[i][3 * j];
				dryCanvas[i][3 * j + 1] += wetCanvas[i][3 * j + 1];
				dryCanvas[i][3 * j + 2] += wetCanvas[i][3 * j + 2];

				wetCanvas[i][3 * j] = 0.0f;
				wetCanvas[i][3 * j + 1] = 0.0f;
				wetCanvas[i][3 * j + 2] = 0.0f;
			}

			// mix wet and dry
			canvasArray[i][6 * j + 3] = dryCanvas[i][3 * j] + wetCanvas[i][3 * j];
			canvasArray[i][6 * j + 4] = dryCanvas[i][3 * j + 1] + wetCanvas[i][3 * j + 1];
			canvasArray[i][6 * j + 5] = dryCanvas[i][3 * j + 2] + wetCanvas[i][3 * j + 2];
		}
	}

	// draw
	glUseProgram(program);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * WINDOW_WIDTH * WINDOW_HEIGHT * 6, canvasArray, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	glDrawArrays(GL_POINTS, 0, sizeof(GLfloat) * WINDOW_WIDTH * WINDOW_HEIGHT);
	glBindVertexArray(NULL);
	glUseProgram(NULL);

	glutSwapBuffers();
}

void reshape(int width, int height)
{
	glViewport(0, 0, width, height);
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 27:	//ESC
		exit(0);
		break;
	case 'F':
	case 'f':
		colori++;
		if (colori >= 4)
			colori = 0;
		break;
	case 'C':
	case 'c':
		for (int i = 0; i < WINDOW_WIDTH; i++) {
			for (int j = 0; j < WINDOW_HEIGHT; j++) {
				wetCanvas[i][3 * j] = 0.0f;
				wetCanvas[i][3 * j + 1] = 0.0f;
				wetCanvas[i][3 * j + 2] = 0.0f;
				dryCanvas[i][3 * j] = 0.0f;
				dryCanvas[i][3 * j + 1] = 0.0f;
				dryCanvas[i][3 * j + 2] = 0.0f;
			}
		}
		break;
	case 'A':
	case 'a':
		if (waterCoef < 0)
			waterCoef = 0;
		else
			waterCoef -= 0.1;
		break;
	case 'Z':
	case 'z':
		if (waterCoef > 1.2)
			waterCoef = 1.2;
		else
			waterCoef += 0.1;
		break;
	default:
		break;
	}
}

void mouseButton(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON) {
		if (state != GLUT_UP) {
			//printf("(%d, %d) / (%lf, %lf)\n", x, y, 2.0 * x / glutGet(GLUT_WINDOW_WIDTH) - 1, -2.0 * y / glutGet(GLUT_WINDOW_HEIGHT) + 1);
			lastX = x;
			lastY = y;
			inkMove = 1.0;

			for (int i = (x - outerR) < 0 ? 0 : (x - outerR); i < WINDOW_WIDTH && i < x + outerR; i++) {
				for (int j = (y - outerR) < 0 ? 0 : (y - outerR); j < WINDOW_HEIGHT && j < y + outerR; j++) {
					int d = D(x, y, i, j);

					if (d <= innerR * innerR) {
						wetCanvas[i][3 * j] += inkMove*color[colori].r;
						wetCanvas[i][3 * j + 1] += inkMove*color[colori].g;
						wetCanvas[i][3 * j + 2] += inkMove*color[colori].b;
					
						canvasTime[i][j] = 0;
						inkTime_limit[i][j] = inkTime_limitBase + 50 * (1.2 - waterCoef);
					}
					else if (d <= outerR * outerR) {	// diffuse
						float a = 1.0 - (float)d / (outerR * outerR);
						if (a < 0)
							a = 0.0;

						wetCanvas[i][3 * j] += a*inkMove*color[colori].r;
						wetCanvas[i][3 * j + 1] += a*inkMove*color[colori].g;
						wetCanvas[i][3 * j + 2] += a*inkMove*color[colori].b;
					
						canvasTime[i][j] = 0;
						inkTime_limit[i][j] = inkTime_limitBase + 50 * (1.2 - waterCoef);
					}
				}
			}
		}
	}
}

void mouseMove(int x, int y)
{
	// interpolation
	int tmp[100][2], count = 0;

	if (x < lastX) {
		if (y < lastY) {
			for (int xi = x, yi = y; xi <= lastX && yi <= lastY; count++, xi++, yi++) {
				tmp[count][0] = xi;
				tmp[count][1] = yi;
			}
		}
		else if (y > lastY) {
			for (int xi = x, yi = y; xi <= lastX && yi >= lastY; count++, xi++, yi--) {
				tmp[count][0] = xi;
				tmp[count][1] = yi;
			}
		}
	}
	else if (x > lastX) {
		if (y < lastY) {
			for (int xi = x, yi = y; xi >= lastX && yi <= lastY; count++, xi--, yi++) {
				tmp[count][0] = xi;
				tmp[count][1] = yi;
			}
		}
		else if (y > lastY) {
			for (int xi = x, yi = y; xi >= lastX && yi >= lastY; count++, xi--, yi--) {
				tmp[count][0] = xi;
				tmp[count][1] = yi;
			}
		}
	}
	
	for (int k = 0; k < count; k++) {
		int xi = tmp[k][0], yi = tmp[k][1];
		for (int i = (xi - outerR) < 0 ? 0 : (xi - outerR); i < WINDOW_WIDTH && i < xi + outerR; i++) {
			for (int j = (yi - outerR) < 0 ? 0 : (yi - outerR); j < WINDOW_HEIGHT && j < yi + outerR; j++) {
				int d = D(xi, yi, i, j);

				if (d <= innerR * innerR) {
					wetCanvas[i][3 * j] += inkMove*color[colori].r;
					wetCanvas[i][3 * j + 1] += inkMove*color[colori].g;
					wetCanvas[i][3 * j + 2] += inkMove*color[colori].b;
				
					canvasTime[i][j] = 0;
					inkTime_limit[i][j] = inkTime_limitBase + 50 * (1.2 - waterCoef);
				}
				else if (d <= outerR * outerR) {	// diffuse
					float a = 1.0 - (float)d / (outerR * outerR);
					if (a < 0)
						a = 0.0;

					wetCanvas[i][3 * j] += a*inkMove*color[colori].r;
					wetCanvas[i][3 * j + 1] += a*inkMove*color[colori].g;
					wetCanvas[i][3 * j + 2] += a*inkMove*color[colori].b;
				
					canvasTime[i][j] = 0;
					inkTime_limit[i][j] = inkTime_limitBase + 50 * (1.2 - waterCoef);
				}
			}
		}
	}

	lastX = x;
	lastY = y;
	inkMove *= inkMove_descent;
}

void idle(void)
{
	glutPostRedisplay();
}