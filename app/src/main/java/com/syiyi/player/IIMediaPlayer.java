package com.syiyi.player;

import android.util.Log;

@SuppressWarnings("WeakerAccess")
public class IIMediaPlayer {
    private String url = null;
    @SuppressWarnings("unused")
    private long mNativePlayer;

    static {
        System.loadLibrary("player");
        System.loadLibrary("avcodec");
        System.loadLibrary("avdevice");
        System.loadLibrary("avfilter");
        System.loadLibrary("avformat");
        System.loadLibrary("avutil");
        System.loadLibrary("postproc");
        System.loadLibrary("swresample");
        System.loadLibrary("swscale");
    }

    public IIMediaPlayer() {
    }

    public void setDataSource(String url) {
        this.url = url;
    }

    public void prepareAsyn() {
        if (url == null) {
            throw new RuntimeException("url is null");
        }
        nativeInit();
        nativeOpen(url);
    }

    public void play() {
        nativePlay();
    }

    public void pause() {
        nativePause();
    }

    public void resume() {
        nativeResume();
    }

    public void stop() {
        nativeStop();
    }

    private native void nativeInit();

    private native void nativeOpen(String path);

    private native void nativePlay();

    private native void nativePause();

    private native void nativeResume();

    private native void nativeStop();

    @Override
    protected void finalize() throws Throwable {
        Log.d("ffplayer", "mNativePlayer:" + mNativePlayer);
        super.finalize();
    }
}
