package com.syiyi.player.opengl;

import android.content.Context;
import android.util.AttributeSet;
import android.view.TextureView;

public class IIGLSurfaceView extends TextureView {

    private Render render;

    public IIGLSurfaceView(Context context) {
        this(context, null);
    }

    public IIGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        render = new Render(context);
        setSurfaceTextureListener(render);
    }

    public Render getRender() {
        return render;
    }

    public void setVideoRotation(int degree) {
        setRotation(degree);
    }

}
