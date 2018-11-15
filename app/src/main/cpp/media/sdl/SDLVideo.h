//
// Created by 宋林涛 on 2018/11/13.
//

#ifndef IIPLAYER_SDLVIDEO_H
#define IIPLAYER_SDLVIDEO_H

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <jni.h>

#define GET_STR(x) #x
#define RENDER_TYPE_OPEN_GL 0
#define RENDER_TYPE_MEDIA_CODEC 1
typedef void (*onTextureReady)();

class SDLVideo {
private:
    const float *vertexData = new float[12]{
            1.0f, -1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f
    };

    const float *textureVertexData = new float[8]{
            1.0f, 0.0f,//右下
            0.0f, 0.0f,//左下
            1.0f, 1.0f,//右上
            0.0f, 1.0f//左上
    };

    const char *vertexShaderString = GET_STR(
            attribute
            vec4 aPosition;
            attribute
            vec2 aTexCoord;
            varying
            vec2 vTexCoord;
            void main() {
                vTexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
                gl_Position = aPosition;
            }
    );
    const char *fragmentShaderString = GET_STR(
            precision
            mediump float;
            varying
            vec2 vTexCoord;
            uniform
            sampler2D yTexture;
            uniform
            sampler2D uTexture;
            uniform
            sampler2D vTexture;
            void main() {
                vec3 yuv;
                vec3 rgb;
                yuv.r = texture2D(yTexture, vTexCoord).r;
                yuv.g = texture2D(uTexture, vTexCoord).r - 0.5;
                yuv.b = texture2D(vTexture, vTexCoord).r - 0.5;
                rgb = mat3(1.0, 1.0, 1.0,
                           0.0, -0.39465, 2.03211,
                           1.13983, -0.58060, 0.0) * yuv;
                gl_FragColor = vec4(rgb, 1.0);
            }
    );

    const char *fragmentMediaCodecShaderString = GET_STR(
            ##extension GL_OES_EGL_image_external : require
            precision mediump float;
            varying vec2 v_texPo;
            uniform samplerExternalOES sTexture;

            void main() {
            //黑白滤镜 关闭
            gl_FragColor=texture2D(sTexture, v_texPo);
            //    lowp vec4 textureColor = texture2D(sTexture, v_texPo);
            //    float gray = textureColor.r * 0.299 + textureColor.g * 0.587 + textureColor.b * 0.114;
            //    gl_FragColor = vec4(gray, gray, gray, textureColor.w);
    }
    );

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

    onTextureReady mReadyCallBack;

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
    SDLVideo(JavaVM *vm, ANativeWindow *window, int renderType,onTextureReady callBack);

    ~SDLVideo();

    void drawYUV(int w, int h, void *y, void *u, void *v);

    void onFrameAvailable();

    void drawMediaCodec();

    jobject getMediaCodecSurface();

private:
    void initEGL(ANativeWindow *nativeWindow);

    void initYuvShader();

    void initMediaCodecShader();

    void createMediaSurface(GLuint textureId);


};


#endif //IIPLAYER_SDLVIDEO_H
