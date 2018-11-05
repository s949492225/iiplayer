package com.syiyi.player;

import android.annotation.SuppressLint;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import com.syiyi.player.listener.OnBufferTimeListener;
import com.syiyi.player.listener.OnPrepareListener;
import com.syiyi.player.listener.OnPlayTimeListener;
import com.syiyi.player.opengl.IIGlSurfaceView;
import com.xw.repo.BubbleSeekBar;

@SuppressLint("all")
public class MainActivity extends AppCompatActivity {
    private IIMediaPlayer mPlayer;
    private IIGlSurfaceView mVideoView;
    private TextView tv_time;
    private TextView tv_buffer_time;
    private BubbleSeekBar seekBar;
    private int position;
    private boolean seek = false;
    int lastime = -1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mVideoView = findViewById(R.id.surface);
        tv_time = findViewById(R.id.tv_time);
        tv_buffer_time = findViewById(R.id.tv_buffer_time);
        seekBar = findViewById(R.id.seekbar);
        mPlayer = new IIMediaPlayer();
        mPlayer.setDataSource("http://220.194.236.214/2/v/x/k/w/vxkwfozzamnhdwuiekdoukkvphikem/hd.yinyuetai.com/5AC80165F11A32EBBFD53F24DCDDA90D.mp4?sc=5ea95dc33763e01b");
        mPlayer.setOnPlayTimeListener(new OnPlayTimeListener() {
            @Override
            public void onPlayTime(TimeInfo info) {
                if (lastime != info.getCurrentTime()) {
                    tv_time.setText(
                            Util.secdsToDateFormat(info.getCurrentTime(), info.getTotalTime())
                                    + "/" +
                                    Util.secdsToDateFormat(info.getTotalTime(), info.getTotalTime()));

                    if (!seek) {
                        seekBar.setProgress(info.getCurrentTime() * 100f / info.getTotalTime());
                    }
                    lastime = info.getCurrentTime();
                }

            }

        });
        mPlayer.setOnBufferTimeListener(new OnBufferTimeListener() {
            @Override
            public void onBufferTime(TimeInfo info) {
                tv_buffer_time.setText(
                        Util.secdsToDateFormat(info.getCurrentTime(), info.getTotalTime())
                                + "/" +
                                Util.secdsToDateFormat(info.getTotalTime(), info.getTotalTime()));
            }
        });

        seekBar.setOnProgressChangedListener(new BubbleSeekBar.OnProgressChangedListener() {
            @Override
            public void onProgressChanged(BubbleSeekBar bubbleSeekBar, int progress, float progressFloat, boolean fromUser) {
                position = progress * mPlayer.getDuration() / 100;
            }

            @Override
            public void getProgressOnActionUp(BubbleSeekBar bubbleSeekBar, int progress, float progressFloat) {
                mPlayer.seek(position);
                seek = false;
            }

            @Override
            public void getProgressOnFinally(BubbleSeekBar bubbleSeekBar, int progress, float progressFloat, boolean fromUser) {

            }
        });
    }

    public void play(View view) {
        mPlayer.setSurfaceView(mVideoView);
        mPlayer.prepareAsync(new OnPrepareListener() {
            @Override
            public void onPrepared() {
                tv_time.setText(String.format("00:%d", mPlayer.getDuration()));
                mPlayer.play();
            }
        });

    }

    public void pause(View view) {
        mPlayer.pause();
    }

    public void resume(View view) {
        mPlayer.resume();
    }

    public void stop(View view) {
        mPlayer.stop();
    }

}
