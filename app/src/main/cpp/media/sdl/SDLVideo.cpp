//
// Created by 宋林涛 on 2018/11/13.
//

#include <cstdlib>
#include "SDLVideo.h"
#include "shaderUtils.h"
#include "../../android/android_log.h"
#include <android/native_window_jni.h>
#include <android/native_window.h>

SDLVideo::SDLVideo(JavaVM *vm, ANativeWindow *window) {
    //jni-----------------------------------------------------------------------------------
    this->vm = vm;
    //get window
    nativeWindow = window;
    //egl-----------------------------------------------------------------------------------
    initEGL(nativeWindow);
    //opengl
    initOpenGL();

}

void SDLVideo::initOpenGL() {
    programId = createProgram(vertexShaderString, fragmentShaderString);

    GLuint aPositionHandle = (GLuint) glGetAttribLocation(programId, "aPosition");
    GLuint aTextureCoordHandle = (GLuint) glGetAttribLocation(programId, "aTexCoord");

    GLuint textureSamplerHandleY = (GLuint) glGetUniformLocation(programId, "yTexture");
    GLuint textureSamplerHandleU = (GLuint) glGetUniformLocation(programId, "uTexture");
    GLuint textureSamplerHandleV = (GLuint) glGetUniformLocation(programId, "vTexture");

    int width = ANativeWindow_getWidth(nativeWindow);
    int height = ANativeWindow_getHeight(nativeWindow);

    glViewport(0, 0, width, height);

    glUseProgram(programId);
    glEnableVertexAttribArray(aPositionHandle);

    glVertexAttribPointer(aPositionHandle, 3, GL_FLOAT, GL_FALSE,
                          12, vertexData);
    glEnableVertexAttribArray(aTextureCoordHandle);
    glVertexAttribPointer(aTextureCoordHandle, 2, GL_FLOAT, GL_FALSE, 8, textureVertexData);

    /***
     * 初始化空的yuv纹理
     * **/
    glGenTextures(1, &yTextureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, yTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUniform1i(textureSamplerHandleY, 0);

    glGenTextures(1, &uTextureId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, uTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUniform1i(textureSamplerHandleU, 1);

    glGenTextures(1, &vTextureId);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, vTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUniform1i(textureSamplerHandleV, 2);
}

void SDLVideo::initEGL(ANativeWindow *nativeWindow) {
    //get display
    eglDisp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    //init
    EGLint eglMajvers, eglMinVers;
    eglInitialize(eglDisp, &eglMajvers, &eglMinVers);
    //choose config

    EGLint numConfigs;
    EGLint configSpec[] = {EGL_RED_SIZE, 8,
                           EGL_GREEN_SIZE, 8,
                           EGL_BLUE_SIZE, 8,
                           EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE};

    eglChooseConfig(eglDisp, configSpec, &eglConf, 1, &numConfigs);

    //create surface
    eglSurface = eglCreateWindowSurface(eglDisp, eglConf, nativeWindow, NULL);

    //create contex
    const EGLint ctxAttr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };
    eglCtx = eglCreateContext(eglDisp, eglConf, EGL_NO_CONTEXT, ctxAttr);
    //绑定到当前线程
    eglMakeCurrent(eglDisp, eglSurface, eglSurface, eglCtx);
}

void SDLVideo::drawYUV(int w, int h, void *y, void *u, void *v) {
    int width = ANativeWindow_getWidth(nativeWindow);
    int height = ANativeWindow_getHeight(nativeWindow);
    glViewport(0, 0, width, height);

    //使 GL_TEXTURE0 单元 活跃 opengl最多支持16个纹理
    //纹理单元是显卡中所有的可用于在shader中进行纹理采样的显存 数量与显卡类型相关，至少16个
    glActiveTexture(GL_TEXTURE0);
    //绑定纹理空间 下面的操作就会作用在这个空间中
    glBindTexture(GL_TEXTURE_2D, yTextureId);
    //创建一个2d纹理 使用亮度颜色模型并且纹理数据也是亮度颜色模型
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, y);


    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, uTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w / 2, h / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                 u);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, vTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w / 2, h / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                 v);


    /***
     * 纹理更新完成后开始绘制
     ***/
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    eglSwapBuffers(eglDisp, eglSurface);

}

SDLVideo::~SDLVideo() {
    delete vm;
    //release opengl
    glDeleteTextures(1, &yTextureId);
    glDeleteTextures(1, &uTextureId);
    glDeleteTextures(1, &vTextureId);
    glDeleteProgram(programId);
    //release egl
    eglMakeCurrent(eglDisp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(eglDisp, eglCtx);
    eglDestroySurface(eglDisp, eglSurface);
    eglTerminate(eglDisp);

    eglDisp = EGL_NO_DISPLAY;
    eglSurface = EGL_NO_SURFACE;
    eglCtx = EGL_NO_CONTEXT;
}








