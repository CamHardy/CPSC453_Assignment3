// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>
#include <iterator>
#include "glm/glm.hpp"
#include "GlyphExtractor.h"

// Specify that we want the OpenGL core profile before including GLFW headers
#ifndef LAB_LINUX
	#include <glad/glad.h>
#else
	#define GLFW_INCLUDE_GLCOREARB
	#define GL_GLEXT_PROTOTYPES
#endif
#include <GLFW/glfw3.h>

//STB
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
using namespace std;
using namespace glm;

// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes
void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint TCSshader, GLuint TESshader, GLuint fragmentShader);

struct MyShader
{
	// OpenGL names for vertex and fragment shaders, shader program
	GLuint  vertex;
	GLuint  TCS;
	GLuint  TES;
	GLuint  fragment;
	GLuint  program;

	// initialize shader and program names to zero (OpenGL reserved value)
	MyShader() : vertex(0), fragment(0), program(0)
	{}
};

// load, compile, and link shaders, returning true if successful
bool InitializeShaders(MyShader *shader)
{
	// load shader source from files
	string vertexSource = LoadSource("vertex.glsl");
	string fragmentSource = LoadSource("fragment.glsl");
	string TCSSource = LoadSource("tessControl.glsl");
	string TESSource = LoadSource("tessEval.glsl");
	if (vertexSource.empty() || fragmentSource.empty()) return false;

	// compile shader source into shader objects
	shader->vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
	shader->fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
	shader->TCS = CompileShader(GL_TESS_CONTROL_SHADER, TCSSource);
	shader->TES = CompileShader(GL_TESS_EVALUATION_SHADER, TESSource);

	// link shader program
	shader->program = LinkProgram(shader->vertex, shader->TCS, shader->TES, shader->fragment);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate shader-related objects
void DestroyShaders(MyShader *shader)
{
	// unbind any shader programs and destroy shader objects
	glUseProgram(0);
	glDeleteProgram(shader->program);
	glDeleteShader(shader->vertex);
	glDeleteShader(shader->fragment);
	glDeleteShader(shader->TCS);
	glDeleteShader(shader->TES);
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct MyGeometry
{
	// OpenGL names for array buffer objects, vertex array object
	GLuint  vertexBuffer;
	GLuint  textureBuffer;
	GLuint  colourBuffer;
	GLuint  vertexArray;
	GLsizei elementCount;

	// initialize object names to zero (OpenGL reserved value)
	MyGeometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
	{}
};

// Hold your breath, here come a literal ton of global variables lol
bool yeah = false;
MyGeometry lineGeometry;
MyGeometry quadGeometry;
MyGeometry cubicGeometry;
MyShader shader;
bool extras = false;
vector<vec2> lines;
vector<vec2> quads;
vector<vec2> cubics;
vector<vec3> lineColours;
vector<vec3> quadColours;
vector<vec3> cubicColours;
string font = "fonts/AlexBrush-Regular.ttf";
float charSize = 0;
int currentFont = 0;
int currentScale = 0;
string texts[4] = {"Cameron Hardy", "The quick brown fox jumps over the lazy dog.", "A phrase!", "there is no need to be upset"};
int currentText = 0;
string fonts[12] = {
	"fonts/AlexBrush-Regular.ttf",
	"fonts/Comic_Sans.ttf",
	"fonts/Inconsolata.otf",
	"fonts/Lora-Bold.ttf",
	"fonts/Lora-BoldItalic.ttf",
	"fonts/Lora-Italic.ttf",
	"fonts/Lora-Regular.ttf",
	"fonts/OptimusPrinceps.ttf",
	"fonts/OptimusPrincepsSemiBold.ttf",
	"fonts/SourceSansPro-Black.otf",
	"fonts/SourceSansPro-Semibold.otf",
	"fonts/SourceSansPro-ExtraLight.otf"
};
float scales[12] = {
	0.3,
	0.25,
	0.28,
	0.24,
	0.25,
	0.25,
	0.25,
	0.24,
	0.215,
	0.26,
	0.27,
	0.29
};
float textLen = 0.0;
float scrollSpeed = 0.0;
float xPan = 0.0;
float scale = 0.3;
float tx, ty;
int scene = 0;
float mx;
float my;
vec2 info;
int extraSelect = 0;
int dSelect = 1;

void RenderGeometry(MyGeometry *geometry)
{
	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;

	glGenBuffers(1, &geometry->vertexBuffer);
	glGenBuffers(1, &geometry->colourBuffer);
	glGenVertexArrays(1, &geometry->vertexArray);

	// create a vertex array object encapsulating all our vertex attributes
	glBindVertexArray(geometry->vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_INDEX);

	// assocaite the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void InitializeGeometry(MyGeometry *geometry, vector<vec2> points, vector<vec3> colours)
{
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec2)*points.size(), &points[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*colours.size(), &colours[0], GL_STATIC_DRAW);

	geometry->elementCount = points.size();
}

// deallocate geometry-related objects
void DestroyGeometry(MyGeometry *geometry)
{
	// unbind and destroy our vertex array object and associated buffers
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &geometry->vertexArray);
	glDeleteBuffers(1, &geometry->vertexBuffer);
	glDeleteBuffers(1, &geometry->colourBuffer);
}

void RenderScene(MyGeometry *geometry, MyShader *shader)
{
	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(shader->program);
	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(GL_PATCHES, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}

vector<vec2> drawLines(vector<vec2> points, int degree)
{
	vector<vec2> lines;
	for(uint i = 0; i<points.size()-degree; i+=degree+1)
	{
		for(int j = 0; j < degree; j++)
		{
			lines.push_back(points[i+j]);
			lines.push_back(points[i+j+1]);
		}
	}
	return lines;
}

void addPoints(vector<vec2> *quads, vector<vec2> points, vector<vec3> *colours, vector<vec3> controlColour, int degree)
{
	uint size = points.size();
	int c = 0;
	int spline = 0;
	for (uint i = 0; i < size; i++)
	{
		quads->push_back(vec2(-0.1,  0.0) + points[i]);
		quads->push_back(vec2(-0.1,  0.1) + points[i]);
		quads->push_back(vec2( 0.0,  0.1) + points[i]);

		quads->push_back(vec2( 0.0,  0.1) + points[i]);
		quads->push_back(vec2( 0.1,  0.1) + points[i]);
		quads->push_back(vec2( 0.1,  0.0) + points[i]);

		quads->push_back(vec2( 0.1,  0.0) + points[i]);
		quads->push_back(vec2( 0.1, -0.1) + points[i]);
		quads->push_back(vec2( 0.0, -0.1) + points[i]);

		quads->push_back(vec2( 0.0, -0.1) + points[i]);
		quads->push_back(vec2(-0.1, -0.1) + points[i]);
		quads->push_back(vec2(-0.1,  0.0) + points[i]);

		for(int j = 0; j < 12; j++)
		{
			colours->push_back(controlColour[c]);
		}

		spline = (spline + 1)%(degree + 1);
		c = 1;
		if(spline == 0 || spline == degree)
		{
			c = 0;
		}
	}
}

void createColours(vector<vec3> *colours, vec3 colour, int size)
{
	for(int i = 0; i < size; i++)
	{
		colours->push_back(colour);
	}
}

void drawKettle() {
	glUseProgram(shader.program);
	GLint loc = glGetUniformLocation(shader.program, "offset");
	glUniform2f(loc, 0,0);
	loc = glGetUniformLocation(shader.program, "scrollOffset");
	glUniform2f(loc, 0,0);
	loc = glGetUniformLocation(shader.program, "scale");
	glUniform1f(loc, 0.35);

	vector<vec2> quads = {
		vec2( 1.0,  1.0), vec2( 2.0, -1.0), vec2( 0.0, -1.0),
		vec2( 0.0, -1.0), vec2(-2.0, -1.0), vec2(-1.0,  1.0),
		vec2(-1.0,  1.0), vec2( 0.0,  1.0), vec2( 1.0,  1.0),
		vec2( 1.2,  0.5), vec2( 2.5,  1.0), vec2( 1.3, -0.4)
	};

	vector<vec2> lines = {vec2(0.0, 0.0)};
	vector<vec3> colours;

	vector<vec2> cubics = {vec2(0.0, 0.0)};
	vector<vec3> opaques;

	createColours(&colours, vec3(0.0, 0.6, 0.9), quads.size());

	if(extras) {
		lines = drawLines(quads, 2);
		vector<vec3> twoColours = {vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 1.0)};
		addPoints(&quads, quads, &colours, twoColours, 2);
	}

	createColours(&opaques, vec3(0.6, 0.6, 0.6), lines.size());

	InitializeGeometry(&lineGeometry, lines, opaques);
	InitializeGeometry(&quadGeometry, quads, colours);
	InitializeGeometry(&cubicGeometry, cubics, colours);
}

void drawFish() {
	glUseProgram(shader.program);
	GLint loc = glGetUniformLocation(shader.program, "offset");
	glUniform2f(loc, -0.7, -0.5);
	loc = glGetUniformLocation(shader.program, "scrollOffset");
	glUniform2f(loc, 0.0, 0.0);
	loc = glGetUniformLocation(shader.program, "scale");
	glUniform1f(loc, 0.18);

	vector<vec2> cubics = {
		vec2( 1.0,  1.0), vec2( 4.0,  0.0), vec2( 6.0,  2.0), vec2( 9.0,  1.0),
		vec2( 8.0,  2.0), vec2( 0.0,  8.0), vec2( 0.0, -2.0), vec2( 8.0,  4.0),
		vec2( 5.0,  3.0), vec2( 3.0,  2.0), vec2( 3.0,  3.0), vec2( 5.0,  2.0),
		vec2( 3.0,  2.2), vec2( 3.5,  2.7), vec2( 3.5,  3.3), vec2( 3.0,  3.8),
		vec2( 2.8,  3.5), vec2( 2.4,  3.8), vec2( 2.4,  3.2), vec2( 2.8,  3.5)
	};

	vector<vec2> lines = {vec2(0.0, 0.0)};
	vector<vec3> colours;

	vector<vec2> quads;
	vector<vec3> opaques;
	vector<vec3> controlColours = {};

	if(extras) {
		lines = drawLines(cubics, 3);
		vector<vec3> twoColours = {vec3(1.0, 1.0, 0.0), vec3(0.0, 1.0, 1.0)};
		addPoints(&quads, cubics, &controlColours, twoColours, 3);
	}

	createColours(&colours, vec3(1.0, 0.4, 0.1), cubics.size());
	createColours(&opaques, vec3(0.6, 0.6, 0.6), lines.size());

	InitializeGeometry(&lineGeometry, lines, opaques);
	InitializeGeometry(&quadGeometry, quads, controlColours);
	InitializeGeometry(&cubicGeometry, cubics, colours);
}

void drawCall()
{
	glUseProgram(shader.program);
	GLint loc = glGetUniformLocation(shader.program, "mode");
	glUniform1i(loc, 0);

	glPatchParameteri(GL_PATCH_VERTICES, 2);
	RenderScene(&lineGeometry, &shader);

	glUseProgram(shader.program);
	loc = glGetUniformLocation(shader.program, "mode");
	glUniform1i(loc, 1);

	glPatchParameteri(GL_PATCH_VERTICES, 3);
	RenderScene(&quadGeometry, &shader);

	glUseProgram(shader.program);
	loc = glGetUniformLocation(shader.program, "mode");
	glUniform1i(loc, 2);

	glPatchParameteri(GL_PATCH_VERTICES, 4);
	RenderScene (&cubicGeometry, &shader);
}

float setGlyph(char c, vec2 offset)
{
	GlyphExtractor extractor;
	MyGlyph glyph;
	//extractor.LoadFontFile("fonts/GreatVibes-Regular.otf");
	extractor.LoadFontFile(font);
	glyph = extractor.ExtractGlyph(c);

	MyContour contour;
	MySegment segment;

	for(uint i = 0; i < glyph.contours.size(); i++)
	{
		contour = glyph.contours[i];
		for(uint j = 0; j < contour.size(); j++)
		{
			segment = contour[j];
			vec3 lineColour;
			vec3 quadColour;
			vec3 cubicColour;

			if (yeah) {
				lineColour = vec3(1.0, 0.0, 0.0);
				quadColour = vec3(0.0, 1.0, 0.0);
				cubicColour = vec3(0.0, 0.0, 1.0);
			}
			else {
				lineColour = vec3(0.33, 0.7, 0.33);
				quadColour = vec3(0.33, 0.7, 0.33);
				cubicColour = vec3(0.33, 0.7, 0.33);
			}

			switch (segment.degree)
			{
				case (1):
					lines.push_back(vec2(segment.x[0], segment.y[0])+offset);
					lines.push_back(vec2(segment.x[1], segment.y[1])+offset);

					lineColours.push_back(lineColour);
					lineColours.push_back(lineColour);
					break;
				case (2):
					quads.push_back(vec2(segment.x[0], segment.y[0])+offset);
					quads.push_back(vec2(segment.x[1], segment.y[1])+offset);
					quads.push_back(vec2(segment.x[2], segment.y[2])+offset);

					quadColours.push_back(quadColour);
					quadColours.push_back(vec3(1.0, 1.0, 1.0));
					quadColours.push_back(quadColour);
					break;
				case (3):
					cubics.push_back(vec2(segment.x[0], segment.y[0])+offset);
					cubics.push_back(vec2(segment.x[1], segment.y[1])+offset);
					cubics.push_back(vec2(segment.x[2], segment.y[2])+offset);
					cubics.push_back(vec2(segment.x[3], segment.y[3])+offset);

					cubicColours.push_back(cubicColour);
					cubicColours.push_back(vec3(1.0, 1.0, 1.0));
					cubicColours.push_back(vec3(1.0, 1.0, 1.0));
					cubicColours.push_back(cubicColour);
					break;
			}
		}
	}

	charSize = glyph.advance;
	return glyph.advance;
}

float setText(string s) {
	lines.clear();
	quads.clear();
	cubics.clear();
	lineColours.clear();
	quadColours.clear();
	cubicColours.clear();

	float textLength = 0;
	vec2 offset = vec2(0,0);
	float x;
	for(uint i = 0; i < s.size(); i++)
	{
		x = setGlyph(s[i], offset);
		offset += vec2(x,0);
		textLength += x;
	}

	InitializeGeometry(&lineGeometry, lines, lineColours);
	InitializeGeometry(&quadGeometry, quads, quadColours);
	InitializeGeometry(&cubicGeometry, cubics, cubicColours);

	return textLength;
}

// --------------------------------------------------------------------------
// GLFW callback functions

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
	cout << "GLFW ERROR " << error << ":" << endl;
	cout << description << endl;
}

void resetUniforms()
{
	tx = -(textLen)/2.0;
	ty = -0.2;

		glUseProgram(shader.program);
	GLint loc = glGetUniformLocation(shader.program, "offset");
	glUniform2f(loc, tx, ty);

	loc = glGetUniformLocation(shader.program, "scale");
	glUniform1f(loc, scale);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	// draw that qt kettle
	if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		scene = 0;
		drawKettle();
	}

	// draw that funky fish
	if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
		scene = 1;
		drawFish();
	}

	// draw some letters or whatever
	if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
		scene = 2;
		textLen = setText(texts[currentText]);
		textLen = textLen * scale;
		resetUniforms();

	}

	// move up to next font
	if (key == GLFW_KEY_UP && action == GLFW_PRESS && scene == 2) {
		currentFont++;
		currentScale++;
		if (currentFont > 11)
			currentFont -= 12;
		if (currentScale > 11)
			currentScale -= 12;
		font = fonts[currentFont];
		scale = scales[currentScale];
		textLen=setText(texts[currentText]);
		textLen = textLen * scale;
		resetUniforms();
	}

	// move up to next message
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS && scene == 2)
	{
		currentText++;
		if (currentText > 3)
		 	currentText -= 4;
		font = fonts[currentFont];
		textLen=setText(texts[currentText]);
		textLen = textLen * scale;
		resetUniforms();
	}

	// move down to previous font
	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS && scene == 2)
	{
		currentFont--;
		currentScale--;
		if (currentFont < 0)
			currentFont += 12;
		if (currentScale < 0)
			currentScale += 12;
		font = fonts[currentFont];
		scale = scales[currentScale];
		textLen=setText(texts[currentText]);
		textLen = textLen*scale;
		resetUniforms();
	}

	// move down to previous message
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS && scene == 2)
	{
		currentText--;
		if (currentText < 0)
			currentText += 4;
		font = fonts[currentFont];
		textLen=setText(texts[currentText]);
		textLen = textLen*scale;
		resetUniforms();
	}

	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		extras = !extras;
		yeah = !yeah;

		if (scene == 0)
			drawKettle();
		if (scene == 1)
			drawFish();
		if (scene == 2) {
			font = fonts[currentFont];
			textLen=setText(texts[currentText]);
			textLen = textLen*scale;
			resetUniforms();
		}
	}

	if (key == GLFW_KEY_0 && action == GLFW_PRESS) {
		scrollSpeed = 0.0;
		xPan = 0.0;
		resetUniforms();
	}
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	scrollSpeed += yoffset / 100;
}

// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
	// initialize the GLFW windowing system
	if (!glfwInit()) {
		cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
		return -1;
	}
	glfwSetErrorCallback(ErrorCallback);

	// attempt to create a window with an OpenGL 4.1 core profile context
	GLFWwindow *window = 0;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(1024, 1024, "Holy cow, you are experiencing the graphics!", 0, 0);
	if (!window) {
		cout << "Program failed to create GLFW window, TERMINATING" << endl;
		glfwTerminate();
		return -1;
	}

	// set keyboard callback function and make our context current (active)
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwMakeContextCurrent(window);

	//Intialize GLAD if not lab linux
	#ifndef LAB_LINUX
	if (!gladLoadGL())
	{
		cout << "GLAD init failed" << endl;
		return -1;
	}
	#endif

	// query and print out information about our OpenGL environment
	QueryGLVersion();

	// call function to load and compile shader programs
	if (!InitializeShaders(&shader)) {
		cout << "Program could not initialize shaders, TERMINATING" << endl;
		return -1;
	}

	RenderGeometry(&lineGeometry);
	RenderGeometry(&quadGeometry);
	RenderGeometry(&cubicGeometry);

	drawKettle();

	// run an event-triggered main loop
	float r = 1;
	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.2, 0.2, 0.2, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		drawCall();

		glUseProgram(shader.program);
		GLint loc = glGetUniformLocation(shader.program, "scrollOffset");
		if (loc != -1)
		{
		   glUniform2f(loc, xPan, 0.0);
		}
		xPan += scrollSpeed;
		if(xPan> textLen + 1 && scrollSpeed > 0)
		{
			xPan = -textLen - 1;
		}
		else if(xPan < -textLen - 1 && scrollSpeed <= 0)
		{
			xPan = textLen + 1;
		}
		r = r - 0.05;
		loc = glGetUniformLocation(shader.program, "p");
		glUniform1f(loc, r);

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	// clean up allocated resources before exit
	DestroyGeometry(&lineGeometry);
	DestroyShaders(&shader);
	glfwDestroyWindow(window);
	glfwTerminate();

	cout << "Goodbye!" << endl;
	return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
	// query opengl version and renderer information
	string version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	string glslver = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

	cout << "OpenGL [ " << version << " ] "
		<< "with GLSL [ " << glslver << " ] "
		<< "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors()
{
	bool error = false;
	for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
	{
		cout << "OpenGL ERROR:  ";
		switch (flag) {
		case GL_INVALID_ENUM:
			cout << "GL_INVALID_ENUM" << endl; break;
		case GL_INVALID_VALUE:
			cout << "GL_INVALID_VALUE" << endl; break;
		case GL_INVALID_OPERATION:
			cout << "GL_INVALID_OPERATION" << endl; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
		case GL_OUT_OF_MEMORY:
			cout << "GL_OUT_OF_MEMORY" << endl; break;
		default:
			cout << "[unknown error code]" << endl;
		}
		error = true;
	}
	return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
	string source;

	ifstream input(filename.c_str());
	if (input) {
		copy(istreambuf_iterator<char>(input),
			istreambuf_iterator<char>(),
			back_inserter(source));
		input.close();
	}
	else {
		cout << "ERROR: Could not load shader source from file "
			<< filename << endl;
	}

	return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
	// allocate shader object name
	GLuint shaderObject = glCreateShader(shaderType);

	// try compiling the source as a shader of the given type
	const GLchar *source_ptr = source.c_str();
	glShaderSource(shaderObject, 1, &source_ptr, 0);
	glCompileShader(shaderObject);

	// retrieve compile status
	GLint status;
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
		cout << "ERROR compiling shader:" << endl << endl;
		cout << source << endl;
		cout << info << endl;
	}

	return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint TCSshader, GLuint TESshader, GLuint fragmentShader)
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
	if (TCSshader) glAttachShader(programObject, TCSshader);
	if (TESshader) glAttachShader(programObject, TESshader);
	if (fragmentShader) glAttachShader(programObject, fragmentShader);

	// try linking the program with given attachments
	glLinkProgram(programObject);

	// retrieve link status
	GLint status;
	glGetProgramiv(programObject, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
		cout << "ERROR linking shader program:" << endl;
		cout << info << endl;
	}

	return programObject;
}
