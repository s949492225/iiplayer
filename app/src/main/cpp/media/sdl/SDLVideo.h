//
// Created by 宋林涛 on 2018/11/13.
//

#ifndef IIPLAYER_SDLVIDEO_H
#define IIPLAYER_SDLVIDEO_H

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <jni.h>
#include <functional>

#define RENDER_TYPE_OPEN_GL 0
#define RENDER_TYPE_MEDIA_CODEC 1

class SDLVideo {
private:
    const float *vertexData = new float[12]{
            1.0f,1.0f,0.0f,
            -1.0f,1.0f,0.0f,
            1.0f,-1.0f,0.0f,
            -1.0f,-1.0f,0.0f
    };

    const float *textureVertexData = new float[8]{
            1.0f, 0.0f,//右下
            0.0f, 0.0f,//左下
            1.0f, 1.0f,//右上
            0.0f, 1.0f//左上
    };

    const char *vertex_base_glsl = "attribute vec4 av_Position;\n"
                                   "attribute vec2 af_Position;\n"
                                   "varying vec2 v_texPo;\n"
                                   "void main() {\n"
                                   "    v_texPo = af_Position;\n"
                                   "    gl_Position = av_Position;\n"
                                   "}";

    const char *fragment_yuv_glsl = "precision mediump float;\n"
                                    "varying vec2 v_texPo;\n"
                                    "uniform sampler2D sampler_y;\n"
                                    "uniform sampler2D sampler_u;\n"
                                    "uniform sampler2D sampler_v;\n"
                                    "void main() {\n"
                                    "    float y,u,v;\n"
                                    "    y = texture2D(sampler_y,v_texPo).x;\n"
                                    "    u = texture2D(sampler_u,v_texPo).x- 128./255.;\n"
                                    "    v = texture2D(sampler_v,v_texPo).x- 128./255.;\n"
                                    "\n"
                                    "    vec3 rgb;\n"
                                    "    rgb.r = y + 1.403 * v;\n"
                                    "    rgb.g = y - 0.344 * u - 0.714 * v;\n"
                                    "    rgb.b = y + 1.770 * u;\n"
                                    "    gl_FragColor = vec4(rgb,1);\n"
                                    "}";
    const char *fragmen_mediacodec_glsl = "#extension GL_OES_EGL_image_external : require\n"
                                          "precision mediump float;\n"
                                          "varying vec2 v_texPo;\n"
                                          "uniform samplerExternalOES sTexture;\n"
                                          "\n"
                                          "void main() {\n"
                                          "    gl_FragColor=texture2D(sTexture, v_texPo);\n"
                                          "}";
    GLuint yTextureId;
    GLuint uTextureId;
    GLuint vTextureId;

    GLuint yuvProgramId;
    //mdeiacodec
    GLuint mediaCodecProgramId;
    GLuint mediaCodecTextureId;
    GLuint aPositionHandle_mediacodec;
    GLuint aTextureCoordHandle_mediacodec;
    GLuint uTextureSamplerHandle_mediacodec;

    std::function<void()> mReadyCallBack;

    //egl
    EGLConfig eglConf;
    EGLSurface eglSurface;
    EGLContext eglCtx;
    EGLDisplay eglDisp;
    //jni
    JavaVM *vm;
    JNIEnv *env;
    ANativeWindow *nativeWindow;
    jobject mediaCodecSurface;
    jmethodID updateTextureJmid;
    jmethodID releaseJmid;

    int renderType;

public:
    SDLVideo(JavaVM *vm, ANativeWindow *window, int renderType, std::function<void()> callBack);

    ~SDLVideo();

    void drawYUV(int w, int h, void *y, void *u, void *v);

    void onFrameAvailable();

    void drawMediaCodec();

    jobject getMediaCodecSurface(JNIEnv *jniEnv);

private:
    void initEGL(ANativeWindow *nativeWindow);

    void initYuvShader();

    void initMediaCodecShader();

    void createMediaSurface(GLuint textureId);


    void gl_log_check() const;
};


#endif //IIPLAYER_SDLVIDEO_H
