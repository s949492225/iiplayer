//
// Created by 宋林涛 on 2018/11/13.
//

#include <cstdlib>
#include "shaderUtils.h"
#include "../../android/android_log.h"

GLuint createProgram(const char *vertexSource, const char *fragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
    if (!vertexShader) {
        return 0;
    }
    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program != 0) {
        glAttachShader(program, vertexShader);
        glAttachShader(program, pixelShader);
        glLinkProgram(program);
        GLint linkStatus = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (!linkStatus) {
            LOGE("opengl Could not link program");
            GLint info_length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_length);
            if (info_length) {
                char *buf = (char *) malloc(info_length * sizeof(char));
                glGetProgramInfoLog(program, info_length, NULL, buf);
                free(buf);
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint loadShader(GLenum shaderType, const char *source) {
    GLuint shader = glCreateShader(shaderType);
    if (shader != 0) {
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            LOGE("opengl could not compile shader :%s", source);
            GLint info_length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_length);
            if (info_length) {
                char *buf = (char *) malloc(info_length * sizeof(char));
                if (buf) {
                    glGetShaderInfoLog(shader, info_length, NULL, buf);
                }
                free(buf);
            }
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}