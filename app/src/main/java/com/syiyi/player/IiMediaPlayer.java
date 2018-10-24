package com.syiyi.player;

public class IiMediaPlayer {
    private String url = null;
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

    public IiMediaPlayer() {
        nativeInit();
    }

    public void setDataSource(String url) {
        this.url = url;
    }

    public void prepareAsyn() {
        if (url == null) {
            throw new RuntimeException("url is null");
        }
        nativeOpen(url);
    }

    public void play() {
        nativePlay();
    }

    private native void nativeInit();

    private native void nativeOpen(String path);

    private native void nativePlay();


}
