package com.syiyi.player.opengl;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

/**
 * Created by hlwky001 on 2017/12/15.
 */

public class IIGlSurfaceView extends GLSurfaceView {

    private GlRender glRender;

    public IIGlSurfaceView(Context context) {
        this(context, null);
    }

    public IIGlSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        glRender = new GlRender(context);
        //设置egl版本为2.0
        setEGLContextClientVersion(2);
        //设置render
        setRenderer(glRender);
        //设置为手动刷新模式
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        glRender.setOnRenderRefreshListener(new OnRenderRefreshListener() {
            @Override
            public void onRefresh() {
                requestRender();
            }
        });
    }

    public void setOnGlSurfaceViewOncreateListener(OnGlSurfaceViewCreateListener listener) {
        if (glRender != null) {
            glRender.setOnGlSurfaceViewCreateListener(listener);
        }
    }

    public void setCodecType(int type) {
        if (glRender != null) {
            glRender.setCodecType(type);
        }
    }


    public void setFrameData(int w, int h, byte[] y, byte[] u, byte[] v) {
        if (glRender != null) {
            glRender.setFrameData(w, h, y, u, v);
            requestRender();
        }
    }

    public void cutVideoImg() {
        if (glRender != null) {
            glRender.cutVideoImg();
            requestRender();
        }
    }
}
