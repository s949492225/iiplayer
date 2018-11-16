package com.syiyi.player.sdl;

import android.graphics.SurfaceTexture;
import android.view.Surface;
@SuppressWarnings("all")
public class MediaCodecSurface {
    private Surface mSurface;
    private SurfaceTexture surfaceTexture;

    public MediaCodecSurface(int textureId) {
        surfaceTexture = new SurfaceTexture(textureId);
        mSurface = new Surface(surfaceTexture);
    }


    public void update() {
        surfaceTexture.updateTexImage();
    }

    public void release() {
        surfaceTexture.release();
        mSurface.release();
    }

    public Surface getSurface() {
        return mSurface;
    }
}
