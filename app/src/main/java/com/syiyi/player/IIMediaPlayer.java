package com.syiyi.player;

import android.graphics.Bitmap;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.Handler;
import android.os.Message;
import android.view.Surface;
import com.syiyi.player.listener.OnBufferTimeListener;
import com.syiyi.player.listener.OnErrorListener;
import com.syiyi.player.listener.OnPrepareListener;
import com.syiyi.player.listener.OnPlayTimeListener;
import com.syiyi.player.listener.OnSeekCompleteListener;
import com.syiyi.player.opengl.IIGlSurfaceView;
import com.syiyi.player.opengl.OnGlSurfaceViewCreateListener;
import java.nio.ByteBuffer;


@SuppressWarnings({"all"})
public class IIMediaPlayer {
    private String url = null;
    private long mNativePlayer;
    private int mDuration;
    private Handler mHandler = new EventHandler(this);
    private boolean isSoftOnly = false;
    private OnPrepareListener mPrepareListener;
    private OnPlayTimeListener mPlayTimeListener;
    private OnBufferTimeListener mBufferTimeListener;
    private OnErrorListener mErrorListener;
    private OnSeekCompleteListener mOnSeekCompleteListener;
    private IIGlSurfaceView mSurfaceView;

    private Surface mSurface;
    private MediaFormat mMediaFormat;
    private MediaCodec mMediaCodec;
    private MediaCodec.BufferInfo info;

    protected static class Code {
        static final int ERROR_OPEN_FILE = -1;
        static final int ERROR_FIND_STREAM = -2;
        static final int ERROR_AUDIO_DECODEC_EXCEPTION = -3;
        static final int ERROR_VIDEO_DECODEC_EXCEPTION = -4;
        static final int ERROR_REDAD_EXCEPTION = -5;
        static final int ERROR_OPEN_HARD_CODEC = -6;
        static final int ERROR_SURFACE_NULL = -7;
        static final int ERROR_JNI = -8;
        static final int DATA_DURATION = 100;
        static final int DATA_NOW_PLAYING_TIME = 101;
        static final int DATA_BUFFER_TIME = 102;
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

    protected static class EventHandler extends Handler {
        private IIMediaPlayer mPlayer;

        public EventHandler(IIMediaPlayer player) {
            this.mPlayer = player;
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case Code.ERROR_OPEN_FILE:
                case Code.ERROR_FIND_STREAM:
                case Code.ERROR_AUDIO_DECODEC_EXCEPTION:
                case Code.ERROR_VIDEO_DECODEC_EXCEPTION:
                case Code.ERROR_REDAD_EXCEPTION:
                case Code.ERROR_OPEN_HARD_CODEC:
                case Code.ERROR_SURFACE_NULL:
                case Code.ERROR_JNI:
                    mPlayer.onError(msg.arg1);
                    break;
                case Code.ACTION_PLAY_PREPARED:
                    mPlayer.onPrepared();
                    break;
                case Code.DATA_DURATION:
                    mPlayer.onGetDuration(msg.arg1);
                    break;
                case Code.DATA_NOW_PLAYING_TIME:
                    mPlayer.onPlayTimeUpdate(msg.arg1);
                    break;
                case Code.DATA_BUFFER_TIME:
                    mPlayer.onBufferTimeUpdate(msg.arg1);
                    break;
                case Code.ACTION_PLAY:
                    mPlayer.onPlaying(true);
                    break;
                case Code.ACTION_PLAY_SEEK_OVER:
                    mPlayer.onSeekComplete();
                    break;
                case Code.ACTION_PLAY_PAUSE:
                    mPlayer.onPlaying(false);
                    break;
                case Code.ACTION_PLAY_LOADING:
                    mPlayer.onLoading(true);
                    break;
                case Code.ACTION_PLAY_LOADING_OVER:
                    mPlayer.onLoading(false);
                    break;
                case Code.ACTION_PLAY_STOP:
                    mPlayer.onPlaying(false);
                    break;
                case Code.ACTION_PLAY_FINISH:
                    mPlayer.onFinish();
                    break;
                default:
                    break;
            }
        }
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

    public void setDataSource(String url) {
        this.url = url;
    }

    public void setSurfaceView(IIGlSurfaceView view) {
        mSurfaceView = view;
        mSurfaceView.setOnGlSurfaceViewOncreateListener(new OnGlSurfaceViewCreateListener() {
            @Override
            public void onGlSurfaceCreated(Surface surface) {
                mSurface = surface;
            }

            @Override
            public void onCutVideoImg(Bitmap bitmap) {

            }
        });
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

    private void sendMessage(Message msg) {
        mHandler.sendMessage(msg);
    }

    private void setFrameData(int width, int height, byte[] y, byte[] u, byte[] v) {
        if (mSurfaceView != null) {
            mSurfaceView.setFrameData(width, height, y, u, v);
        }
    }

    private void setCodecType(int type) {
        if (mSurfaceView != null) {
            mSurfaceView.setCodecType(type);
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

    protected void onError(int code) {
        if (mErrorListener != null) {
            mErrorListener.onError(code);
        }
    }

    protected void onPrepared() {
        if (mPrepareListener != null) {
            mPrepareListener.onPrepared();
        }
    }

    protected void onGetDuration(int sec) {
        mDuration = sec;
    }

    protected void onPlaying(boolean isPlay) {
    }

    protected void onLoading(boolean isLoad) {
    }

    protected void onFinish() {
    }

    private native void nativeOpen(String path);

    private native void nativePlay();

    private native void nativePause();

    private native void nativeSeek(int sec);

    private native void nativeResume();

    private native void nativeStop();

    private native String nativeGetInfo(String name);


    private void onSeekComplete() {
        if (mOnSeekCompleteListener != null) {
            mOnSeekCompleteListener.onFinish();
        }
    }

    private void onBufferTimeUpdate(int current) {
        if (mBufferTimeListener!=null) {
            mBufferTimeListener.onBufferTime(new TimeInfo(current, mDuration));
        }
    }

    private void onPlayTimeUpdate(int current) {
        if(mPlayTimeListener!=null) {
            mPlayTimeListener.onPlayTime(new TimeInfo(current, mDuration));
        }

    }

    public void setOnSeekCompleteListener(OnSeekCompleteListener listener) {
        mOnSeekCompleteListener = listener;
    }

    public void setOnPlayTimeListener(OnPlayTimeListener listener) {
        mPlayTimeListener = listener;
    }

    public void setOnBufferTimeListener(OnBufferTimeListener listener) {
        mBufferTimeListener = listener;
    }

    public void setOnErrorListener(OnErrorListener listener) {
        mErrorListener = listener;
    }

    public void setSoftOnly(boolean softOnly) {
        isSoftOnly = softOnly;
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

    private boolean isSupportHard(String codecName) {
        return VideoSupportUtil.isSupportCodec(codecName);
    }

    private int initMediaCodec(String codeName, int width, int height, byte[] csd0, byte[] csd1) {
        if (mSurface != null) {
            try {
                mSurfaceView.setCodecType(IIGlSurfaceView.CODEC_HARD);
                String mime = VideoSupportUtil.findVideoCodecName(codeName);
                mMediaFormat = MediaFormat.createVideoFormat(mime, width, height);
                mMediaFormat.setInteger(MediaFormat.KEY_WIDTH, width);
                mMediaFormat.setInteger(MediaFormat.KEY_HEIGHT, height);
                mMediaFormat.setLong(MediaFormat.KEY_MAX_INPUT_SIZE, width * height);
                mMediaFormat.setByteBuffer("csd-0", ByteBuffer.wrap(csd0));
                mMediaFormat.setByteBuffer("csd-1", ByteBuffer.wrap(csd1));

                mMediaCodec = MediaCodec.createDecoderByType(mime);

                info = new MediaCodec.BufferInfo();
                mMediaCodec.configure(mMediaFormat, mSurface, null, 0);
                mMediaCodec.start();
                return 0;
            } catch (Exception e) {
                return -1;
            }
        } else {
            return -2;
        }

    }

    public void decodeAVPacket(int datasize, byte[] data) {
        if (mSurface != null && datasize > 0 && data != null && mMediaCodec != null) {
            try {
                int intputBufferIndex = mMediaCodec.dequeueInputBuffer(10);
                if (intputBufferIndex >= 0) {
                    ByteBuffer byteBuffer = mMediaCodec.getInputBuffers()[intputBufferIndex];
                    byteBuffer.clear();
                    byteBuffer.put(data);
                    mMediaCodec.queueInputBuffer(intputBufferIndex, 0, datasize, 0, 0);
                }
                int outputBufferIndex = mMediaCodec.dequeueOutputBuffer(info, 10);
                while (outputBufferIndex >= 0) {
                    mMediaCodec.releaseOutputBuffer(outputBufferIndex, true);
                    outputBufferIndex = mMediaCodec.dequeueOutputBuffer(info, 10);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }

        }
    }

    private void releaseMediaCodec() {
        if (mMediaCodec != null) {
            try {
                mMediaCodec.flush();
                mMediaCodec.stop();
                mMediaCodec.release();
            } catch (Exception e) {
                //ignore
            }
            mMediaCodec = null;
            mMediaFormat = null;
            info = null;
        }

    }

}
