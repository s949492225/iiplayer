//
// Created by 宋林涛 on 2018/11/13.
//

#ifndef IIPLAYER_SHADERUTILS_H
#define IIPLAYER_SHADERUTILS_H

#include <GLES2/gl2.h>

GLuint createProgram(const char *vertexSource, const char *fragmentSource);
GLuint loadShader(GLenum shaderType, const char *source);
#endif //IIPLAYER_SHADERUTILS_H
