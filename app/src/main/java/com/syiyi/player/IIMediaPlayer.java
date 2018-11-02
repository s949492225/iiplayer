package com.syiyi.player;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

import com.syiyi.player.listener.OnPrepareListener;
import com.syiyi.player.listener.OnPlayTimeListener;
import com.syiyi.player.opengl.Render;


@SuppressWarnings("WeakerAccess")
public class IIMediaPlayer {
    private String url = null;
    @SuppressWarnings("unused")
    private long mNativePlayer;
    private Handler mHandler;
    private OnPrepareListener mPrepareListener;
    private OnPlayTimeListener mPlayTimeListener;
    private Render mRender;

    @SuppressWarnings("unused")
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
        static final int ACTION_PLAY_SEEK_OVER = 6;
        static final int ACTION_PLAY_LOADING = 7;
        static final int ACTION_PLAY_LOADING_OVER = 8;
        static final int ACTION_PLAY_STOP = 9;
        static final int ACTION_PLAY_FINISH = 10;
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


    public IIMediaPlayer() {
        initHandler();
    }

    public void setDataSource(String url) {
        this.url = url;
    }

    public void setRender(Render mRender) {
        this.mRender = mRender;
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

    @SuppressWarnings("unused")
    public void sendMessage(Message msg) {
        mHandler.sendMessage(msg);
    }

    @SuppressWarnings("unused")
    public void setFrameData(int width, int height, byte[] y, byte[] u, byte[] v) {
        if (mRender != null) {
            mRender.setYUVRenderData(width, height, y, u, v);
        }
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

    protected void onPlaying(boolean isPlay) {
//        Log.d("iiplayer", "onPlaying:" + isPlay);
    }

    protected void onLoading(boolean isLoad) {
        Log.d("iiplayer", "onLoading:" + isLoad);
    }

    protected void onFinish() {
        Log.d("iiplayer", "onFinish");
    }

    private native void nativeOpen(String path);

    private native void nativePlay();

    private native void nativePause();

    private native void nativeSeek(int sec);

    private native void nativeResume();

    private native void nativeStop();

    private native String nativeGetInfo(String name);

    protected void initHandler() {
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
                        mPlayTimeListener.onPlayTime(new TimeInfo(msg.arg1, getDuration()));
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

    public void setOnPlayTimeListener(OnPlayTimeListener listener) {
        mPlayTimeListener = listener;
    }


    public int getWidth() {
        return Integer.parseInt(nativeGetInfo("width"));
    }

    public int getHeight() {
        return Integer.parseInt(nativeGetInfo("height"));
    }

    public int getDuration() {
        return Integer.parseInt(nativeGetInfo("duration"));
    }

    public int getRotation() {
        return Integer.parseInt(nativeGetInfo("rotation"));
    }

    public int getPlayedTime() {
        return Integer.parseInt(nativeGetInfo("played_time"));
    }

}
