/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * GLCompat.cpp - Part of the SLUDGE Dev Kit (GTK+ version)
 *
 * Emulates the legacy immediate-mode OpenGL calls used by the graphical tools
 * with a modern core-profile pipeline. See GLCompat.h.
 */

#include <epoxy/gl.h>

#define GLCOMPAT_NO_MACROS
#include "GLCompat.h"

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <vector>

struct CompatVertex {
	float x, y, z;
	float r, g, b, a;
	float u, v;
};

static GLuint program = 0;
static GLuint vao = 0;
static GLuint vbo = 0;
static GLint uProjection = -1;
static GLint uTexture = -1;
static GLint uUseTexture = -1;
static GLint aPosition = -1;
static GLint aColor = -1;
static GLint aTexCoord = -1;

static CompatVertex current = { 0, 0, 0, 1, 1, 1, 1, 0, 0 };
static GLenum currentMode = 0;
static bool inBegin = false;
static bool textureEnabled = false;
static std::vector<CompatVertex> vertices;

static float projection[16] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

static const char *vertexSource =
	"#version 150\n"
	"in vec3 aPos;\n"
	"in vec4 aCol;\n"
	"in vec2 aTex;\n"
	"uniform mat4 uProjection;\n"
	"out vec4 vCol;\n"
	"out vec2 vTex;\n"
	"void main() {\n"
	"    vCol = aCol;\n"
	"    vTex = aTex;\n"
	"    gl_Position = uProjection * vec4(aPos, 1.0);\n"
	"}\n";

static const char *fragmentSource =
	"#version 150\n"
	"in vec4 vCol;\n"
	"in vec2 vTex;\n"
	"uniform sampler2D uTexture;\n"
	"uniform int uUseTexture;\n"
	"out vec4 fragColor;\n"
	"void main() {\n"
	"    vec4 color = vCol;\n"
	"    if (uUseTexture == 1) color *= texture(uTexture, vTex);\n"
	"    fragColor = color;\n"
	"}\n";

static GLuint compileShader(GLenum type, const char *source)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint ok = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[1024];
		GLsizei len = 0;
		glGetShaderInfoLog(shader, sizeof(log) - 1, &len, log);
		log[len] = 0;
		fprintf(stderr, "GLCompat: shader compilation failed: %s\n", log);
	}
	return shader;
}

void compatInit(void)
{
	GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glBindAttribLocation(program, 0, "aPos");
	glBindAttribLocation(program, 1, "aCol");
	glBindAttribLocation(program, 2, "aTex");
	glLinkProgram(program);

	GLint ok = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[1024];
		GLsizei len = 0;
		glGetProgramInfoLog(program, sizeof(log) - 1, &len, log);
		log[len] = 0;
		fprintf(stderr, "GLCompat: program link failed: %s\n", log);
	}

	glDeleteShader(vs);
	glDeleteShader(fs);

	uProjection = glGetUniformLocation(program, "uProjection");
	uTexture = glGetUniformLocation(program, "uTexture");
	uUseTexture = glGetUniformLocation(program, "uUseTexture");
	aPosition = 0;
	aColor = 1;
	aTexCoord = 2;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
}

void compatDestroy(void)
{
	if (program) glDeleteProgram(program);
	if (vbo) glDeleteBuffers(1, &vbo);
	if (vao) glDeleteVertexArrays(1, &vao);
	program = vao = vbo = 0;
}

void compatBegin(GLenum mode)
{
	currentMode = mode;
	inBegin = true;
	vertices.clear();
}

void compatVertex3f(float x, float y, float z)
{
	if (!inBegin) return;
	CompatVertex v = current;
	v.x = x;
	v.y = y;
	v.z = z;
	vertices.push_back(v);
}

void compatVertex3i(int x, int y, int z)
{
	compatVertex3f((float) x, (float) y, (float) z);
}

void compatColor3f(float r, float g, float b)
{
	compatColor4f(r, g, b, 1.0f);
}

void compatColor4f(float r, float g, float b, float a)
{
	current.r = r;
	current.g = g;
	current.b = b;
	current.a = a;
}

void compatTexCoord2f(float s, float t)
{
	current.u = s;
	current.v = t;
}

void compatEnd(void)
{
	inBegin = false;

	if (vertices.empty() || program == 0) return;

	// GL_QUADS and GL_POLYGON do not exist in a core profile; turn them into
	// triangles. GL_POLYGON is only well defined for convex polygons, which
	// matches the behaviour of the fixed-function pipeline it replaces.
	std::vector<CompatVertex> draw;
	GLenum primitive = currentMode;

	if (currentMode == GL_QUADS) {
		primitive = GL_TRIANGLES;
		size_t n = vertices.size() - (vertices.size() % 4);
		for (size_t i = 0; i < n; i += 4) {
			draw.push_back(vertices[i + 0]);
			draw.push_back(vertices[i + 1]);
			draw.push_back(vertices[i + 2]);
			draw.push_back(vertices[i + 0]);
			draw.push_back(vertices[i + 2]);
			draw.push_back(vertices[i + 3]);
		}
	} else if (currentMode == GL_POLYGON) {
		primitive = GL_TRIANGLES;
		for (size_t i = 1; i + 1 < vertices.size(); i++) {
			draw.push_back(vertices[0]);
			draw.push_back(vertices[i]);
			draw.push_back(vertices[i + 1]);
		}
	} else {
		draw = vertices;
	}

	if (draw.empty()) return;

	glUseProgram(program);
	glUniformMatrix4fv(uProjection, 1, GL_FALSE, projection);
	glUniform1i(uUseTexture, textureEnabled ? 1 : 0);
	if (textureEnabled) {
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(uTexture, 0);
	}

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, draw.size() * sizeof(CompatVertex),
	             draw.data(), GL_STREAM_DRAW);

	glEnableVertexAttribArray(aPosition);
	glVertexAttribPointer(aPosition, 3, GL_FLOAT, GL_FALSE, sizeof(CompatVertex),
	                      (void *) offsetof(CompatVertex, x));
	glEnableVertexAttribArray(aColor);
	glVertexAttribPointer(aColor, 4, GL_FLOAT, GL_FALSE, sizeof(CompatVertex),
	                      (void *) offsetof(CompatVertex, r));
	glEnableVertexAttribArray(aTexCoord);
	glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(CompatVertex),
	                      (void *) offsetof(CompatVertex, u));

	glDrawArrays(primitive, 0, (GLsizei) draw.size());

	glBindVertexArray(0);
}

void compatMatrixMode(GLenum mode)
{
	// Only the projection matrix matters for this renderer; the model view
	// matrix is always the identity.
	(void) mode;
}

void compatLoadIdentity(void)
{
	memset(projection, 0, sizeof(projection));
	projection[0] = projection[5] = projection[10] = projection[15] = 1.0f;
}

void compatOrtho(double left, double right, double bottom, double top,
                 double nearVal, double farVal)
{
	float rl = (float) (right - left);
	float tb = (float) (top - bottom);
	float fn = (float) (farVal - nearVal);

	projection[0] = 2.0f / rl;
	projection[1] = 0.0f;
	projection[2] = 0.0f;
	projection[3] = 0.0f;

	projection[4] = 0.0f;
	projection[5] = 2.0f / tb;
	projection[6] = 0.0f;
	projection[7] = 0.0f;

	projection[8] = 0.0f;
	projection[9] = 0.0f;
	projection[10] = -2.0f / fn;
	projection[11] = 0.0f;

	projection[12] = (float) (-(right + left) / rl);
	projection[13] = (float) (-(top + bottom) / tb);
	projection[14] = (float) (-(farVal + nearVal) / fn);
	projection[15] = 1.0f;
}

void compatTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
	// Modulate is the only mode used; the shader always multiplies the
	// texture by the current colour.
	(void) target;
	(void) pname;
	(void) param;
}

void compatEnable(GLenum cap)
{
	if (cap == GL_TEXTURE_2D)
		textureEnabled = true;
	else
		glEnable(cap);
}

void compatDisable(GLenum cap)
{
	if (cap == GL_TEXTURE_2D)
		textureEnabled = false;
	else
		glDisable(cap);
}
