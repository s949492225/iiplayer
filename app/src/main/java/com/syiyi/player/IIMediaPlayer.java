package com.syiyi.player;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;


@SuppressWarnings("WeakerAccess")
public class IIMediaPlayer {
    private String url = null;
    @SuppressWarnings("unused")
    private long mNativePlayer;
    private Handler mHandler;

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
        mHandler = new Handler(Looper.myLooper(), new Handler.Callback() {
            @Override
            public boolean handleMessage(Message msg) {
                Log.d("IIMediaPlayer", "handleMessage: " + msg.what);
                return false;
            }
        });
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

    public void pause() {
        nativePause();
    }

    public void resume() {
        nativeResume();
    }

    public void stop() {
        nativeStop();
    }

    public void seek(int sec) {
        nativeSeek(sec);
    }

    private native void nativeOpen(String path);

    private native void nativePlay();

    private native void nativePause();

    private native void nativeSeek(int sec);

    private native void nativeResume();

    private native void nativeStop();

    @Override
    protected void finalize() throws Throwable {
        Log.d("ffplayer", "mNativePlayer:" + mNativePlayer);
        super.finalize();
    }

}
