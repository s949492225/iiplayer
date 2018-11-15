package com.syiyi.player.sdl;

import android.graphics.SurfaceTexture;
import android.view.Surface;

public class MediaCodecSurface {
    private Surface mSurface;
    private SurfaceTexture surfaceTexture;
    private long mNativeOjbect;

    public MediaCodecSurface(int textureId, long nativeOjbect) {
        mNativeOjbect = nativeOjbect;
        surfaceTexture = new SurfaceTexture(textureId);
        surfaceTexture.setOnFrameAvailableListener(new SurfaceTexture.OnFrameAvailableListener() {
            @Override
            public void onFrameAvailable(SurfaceTexture surfaceTexture) {
                if (mNativeOjbect != 0) {
                    onDraw(mNativeOjbect);
                }
            }
        });
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

    public native void onDraw(long object);
}
