//
// Created by 宋林涛 on 2018/11/13.
//

#include <cstdlib>
#include "SDLVideo.h"
#include "shaderUtils.h"
#include "../../android/android_log.h"
#include "../../android/iiplayer_jni.h"
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <unistd.h>

SDLVideo::SDLVideo(ANativeWindow *window, int renderType) {
    this->renderType = renderType;
    //get window
    nativeWindow = window;
    //egl-----------------------------------------------------------------------------------
    initEGL();
    //opengl
    if (this->renderType == RENDER_TYPE_OPEN_GL) {
        initYuvShader();
    } else {
        initMediaCodecShader();
    }

}

void SDLVideo::initEGL() {
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

    const EGLint surfaceAttrs[] =
            {
                    EGL_RENDER_BUFFER,
                    EGL_BACK_BUFFER,
                    EGL_NONE
            };

    //create surface
    eglSurface = eglCreateWindowSurface(eglDisp, eglConf, nativeWindow, surfaceAttrs);

    //create context
    const EGLint ctxAttr[] = {
            EGL_CONTEXT_CLIENT_VERSION,
            2,
            EGL_NONE
    };
    eglCtx = eglCreateContext(eglDisp, eglConf, EGL_NO_CONTEXT, ctxAttr);
    //绑定到当前线程
    eglMakeCurrent(eglDisp, eglSurface, eglSurface, eglCtx);
}

void SDLVideo::resetEGL(ANativeWindow *window) {
    nativeWindow = window;
    if (eglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(eglDisp, eglSurface);
    }

    const EGLint surfaceAttrs[] =
            {
                    EGL_RENDER_BUFFER,
                    EGL_BACK_BUFFER,
                    EGL_NONE
            };

    eglSurface = eglCreateWindowSurface(eglDisp, eglConf, window, surfaceAttrs);

    if (!eglMakeCurrent(eglDisp, eglSurface, eglSurface, eglCtx)) {
        LOGE("ResetSurface failed EGL unable to eglMakeCurrent");
    }
}


void SDLVideo::initYuvShader() {
    yuvProgramId = createProgram(vertex_base_glsl, fragment_yuv_glsl);

    GLuint aPositionHandle = (GLuint) glGetAttribLocation(yuvProgramId, "av_Position");
    GLuint aTextureCoordHandle = (GLuint) glGetAttribLocation(yuvProgramId, "af_Position");

    GLuint textureSamplerHandleY = (GLuint) glGetUniformLocation(yuvProgramId, "sampler_y");
    GLuint textureSamplerHandleU = (GLuint) glGetUniformLocation(yuvProgramId, "sampler_u");
    GLuint textureSamplerHandleV = (GLuint) glGetUniformLocation(yuvProgramId, "sampler_v");

    int width = ANativeWindow_getWidth(nativeWindow);
    int height = ANativeWindow_getHeight(nativeWindow);

    glViewport(0, 0, width, height);

    glUseProgram(yuvProgramId);
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

void SDLVideo::initMediaCodecShader() {
    mediaCodecProgramId = createProgram(vertex_base_glsl, fragmen_mediacodec_glsl);

    aPositionHandle_mediacodec = (GLuint) glGetAttribLocation(mediaCodecProgramId, "av_Position");
    aTextureCoordHandle_mediacodec = (GLuint) glGetAttribLocation(mediaCodecProgramId,
                                                                  "af_Position");
    uTextureSamplerHandle_mediacodec = (GLuint) glGetUniformLocation(mediaCodecProgramId,
                                                                     "sTexture");

    glGenTextures(1, &mediaCodecTextureId);
    gl_log_check("glGenTextures");

    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mediaCodecTextureId);
    gl_log_check("glBindTexture");

    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    createMediaSurface(mediaCodecTextureId);
}

void SDLVideo::gl_log_check(const char *msg) {
    GLenum error;
    if ((error = glGetError()) != GL_NO_ERROR) {
        LOGE("opengl %s code: %i", msg, error);
    }
}

void SDLVideo::createMediaSurface(GLuint textureId) {
    if (get_jni_jvm()->AttachCurrentThread(&env, 0) != JNI_OK) {
        LOGE("SDLVideo AttachCurrentThread ERROR ")
        return;
    }
    jclass jcls = get_mediacodec_surface();
    updateTextureJmid = env->GetMethodID(jcls, "update", "()V");
    releaseJmid = env->GetMethodID(jcls, "release", "()V");

    jmethodID jmid = env->GetMethodID(jcls, "<init>", "(I)V");
    jobject obj = env->NewObject(jcls, jmid, textureId, this);
    mediaCodecSurface = env->NewGlobalRef(obj);
    env->DeleteLocalRef(obj);
    get_jni_jvm()->DetachCurrentThread();
    usleep(20 * 1000);
}

jobject SDLVideo::getMediaCodecSurface(JNIEnv *jniEnv) {
    jclass jcls = get_mediacodec_surface();
    jmethodID jmid = jniEnv->GetMethodID(jcls, "getSurface", "()Landroid/view/Surface;");
    return jniEnv->CallObjectMethod(mediaCodecSurface, jmid);
}

void SDLVideo::drawMediaCodec() {
    eglMakeCurrent(eglDisp, eglSurface, eglSurface, eglCtx);

    int width = ANativeWindow_getWidth(nativeWindow);
    int height = ANativeWindow_getHeight(nativeWindow);

    glViewport(0, 0, width, height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0, 0, 0, 1);

    glUseProgram(mediaCodecProgramId);

    JNIEnv *env;

    if (get_jni_jvm()->AttachCurrentThread(&env, 0) != JNI_OK) {
        LOGE("SDLVideo AttachCurrentThread ERROR ")
        return;
    }
    env->CallVoidMethod(mediaCodecSurface, updateTextureJmid);
    get_jni_jvm()->DetachCurrentThread();

    glEnableVertexAttribArray(aPositionHandle_mediacodec);
    glVertexAttribPointer(aPositionHandle_mediacodec, 3, GL_FLOAT, GL_FALSE,
                          12, vertexData);
    glEnableVertexAttribArray(aTextureCoordHandle_mediacodec);
    glVertexAttribPointer(aTextureCoordHandle_mediacodec, 2, GL_FLOAT, GL_FALSE, 8,
                          textureVertexData);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mediaCodecTextureId);
    glUniform1i(uTextureSamplerHandle_mediacodec, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    eglSwapBuffers(eglDisp, eglSurface);
}

void SDLVideo::drawYUV(int w, int h, void *y, void *u, void *v) {
    int width = ANativeWindow_getWidth(nativeWindow);
    int height = ANativeWindow_getHeight(nativeWindow);
    glViewport(0, 0, width, height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0, 0, 0, 1);

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

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    eglSwapBuffers(eglDisp, eglSurface);

}

SDLVideo::~SDLVideo() {
    //release opengl
    glDeleteTextures(1, &yTextureId);
    glDeleteTextures(1, &uTextureId);
    glDeleteTextures(1, &vTextureId);
    glDeleteProgram(yuvProgramId);
    //release egl
    eglMakeCurrent(eglDisp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(eglDisp, eglCtx);
    eglDestroySurface(eglDisp, eglSurface);
    eglTerminate(eglDisp);

    eglDisp = EGL_NO_DISPLAY;
    eglSurface = EGL_NO_SURFACE;
    eglCtx = EGL_NO_CONTEXT;

    //release jni
    if (renderType == RENDER_TYPE_MEDIA_CODEC) {


        if (get_jni_jvm()->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("SDLVideo AttachCurrentThread ERROR ")
            return;
        }

        env->CallVoidMethod(mediaCodecSurface, releaseJmid);
        env->DeleteGlobalRef(mediaCodecSurface);
        get_jni_jvm()->DetachCurrentThread();
    }
}









