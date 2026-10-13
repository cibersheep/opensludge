/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * GLCompat.h - Part of the SLUDGE Dev Kit (GTK+ version)
 *
 * GTK 4's GtkGLArea always creates an OpenGL core profile context, which does
 * not provide the fixed-function pipeline the SLUDGE graphical tools were
 * written against. This compatibility layer emulates the handful of
 * immediate-mode calls they use on top of a small shader/VBO pipeline.
 *
 * Include it after the real OpenGL headers. It redefines the legacy entry
 * points to the compatibility implementations.
 */

#ifndef SLUDGE_GLCOMPAT_H
#define SLUDGE_GLCOMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

void compatInit(void);
void compatDestroy(void);
void compatBegin(GLenum mode);
void compatEnd(void);
void compatVertex3f(float x, float y, float z);
void compatVertex3i(int x, int y, int z);
void compatColor3f(float r, float g, float b);
void compatColor4f(float r, float g, float b, float a);
void compatTexCoord2f(float s, float t);
void compatMatrixMode(GLenum mode);
void compatLoadIdentity(void);
void compatOrtho(double left, double right, double bottom, double top,
                 double nearVal, double farVal);
void compatTexEnvf(GLenum target, GLenum pname, GLfloat param);
void compatEnable(GLenum cap);
void compatDisable(GLenum cap);

#ifdef __cplusplus
}
#endif

#ifndef GLCOMPAT_NO_MACROS
#define glBegin      compatBegin
#define glEnd        compatEnd
#define glVertex3f   compatVertex3f
#define glVertex3i   compatVertex3i
#define glColor3f    compatColor3f
#define glColor4f    compatColor4f
#define glTexCoord2f compatTexCoord2f
#define glMatrixMode compatMatrixMode
#define glLoadIdentity compatLoadIdentity
#define glOrtho      compatOrtho
#define glTexEnvf    compatTexEnvf
#define glEnable     compatEnable
#define glDisable    compatDisable
#endif

#endif
