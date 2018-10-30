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
    private OnPrepareListener mPrepareListener;

    protected static class Code {
        static final int ERROR_OPEN_FILE = -1;
        static final int ERROR_FIND_STREAM = -2;
        static final int DATA_DURATION = 100;
        static final int DATA_NOW_PLAYING_TIME = 101;
        static final int ACTION_PLAY_PREPARE = 1;
        static final int ACTION_PLAY_PREPARED = 2;
        static final int ACTION_PLAY = 3;
        static final int ACTION_PLAY_PAUSE = 4;
        static final int ACTION_PLAY_SEEK = 5;
        static final int ACTION_PLAY_LOADING = 6;
        static final int ACTION_PLAY_LOADING_OVER = 7;
        static final int ACTION_PLAY_STOP = 8;
        static final int ACTION_PLAY_FINISH = 9;
    }

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

    public static interface OnPrepareListener {
        void onPrepared();
    }

    public IIMediaPlayer() {
        initMsg();
    }


    public void setDataSource(String url) {
        this.url = url;
    }

    public void prepareAsync(OnPrepareListener listener) {
        this.mPrepareListener = listener;
        if (url == null) {
            throw new RuntimeException("url is null");
        }
        nativeOpen(url);
    }

    public void prepareAsync() {
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

    protected void onOpenFail() {
        Log.d("iiplayer", "onOpenFail");
    }

    protected void onPrepared() {
        Log.d("iiplayer", "onPrepared");
        if (mPrepareListener != null) {
            mPrepareListener.onPrepared();
        }
    }

    protected void onGetDuration(int sec) {
        Log.d("iiplayer", "onGetDuration");
    }

    protected void onGetPlayingTime(int sec) {
        Log.d("iiplayer", "onGetPlayingTime");
    }

    protected void onPlaying(boolean isPlay) {
        Log.d("iiplayer", "onPlaying:" + isPlay);
    }

    protected void onLoading(boolean isLoad) {
        Log.d("iiplayer", "onLoading:" + isLoad);

    }

    protected void onFinish() {
        Log.d("iiplayer", "onFinish");
    }

    protected native void nativeOpen(String path);

    protected native void nativePlay();

    protected native void nativePause();

    protected native void nativeSeek(int sec);

    protected native void nativeResume();

    protected native void nativeStop();

    protected void initMsg() {
        mHandler = new Handler(Looper.myLooper(), new Handler.Callback() {
            @Override
            public boolean handleMessage(Message msg) {
                switch (msg.what) {
                    case Code.ERROR_OPEN_FILE:
                    case Code.ERROR_FIND_STREAM:
                        onOpenFail();
                        break;
                    case Code.ACTION_PLAY_PREPARED:
                        onPrepared();
                    case Code.DATA_DURATION:
                        onGetDuration(msg.arg1);
                    case Code.DATA_NOW_PLAYING_TIME:
                        onGetPlayingTime(msg.arg1);
                    case Code.ACTION_PLAY:
                        onPlaying(true);
                        break;
                    case Code.ACTION_PLAY_PAUSE:
                        onPlaying(false);
                    case Code.ACTION_PLAY_LOADING:
                        onLoading(true);
                    case Code.ACTION_PLAY_LOADING_OVER:
                        onLoading(false);
                    case Code.ACTION_PLAY_STOP:
                        onPlaying(false);
                        break;
                    case Code.ACTION_PLAY_FINISH:
                        onFinish();
                        break;
                    default:
                        break;
                }
                return false;
            }
        });
    }

}
