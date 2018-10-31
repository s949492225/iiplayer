package com.syiyi.player;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;

import com.syiyi.player.opengl.IIGLSurfaceView;
import com.syiyi.player.opengl.Render;

public class MainActivity extends AppCompatActivity {
    private IIMediaPlayer mPlayer;
    private IIGLSurfaceView mVideoView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mVideoView = findViewById(R.id.surface);
        mPlayer = new IIMediaPlayer();
//        mPlayer.setDataSource("/sdcard/DCIM/Camera/VID_20180914_123709.mp4");
        mPlayer.setDataSource("http://220.194.236.214/2/v/x/k/w/vxkwfozzamnhdwuiekdoukkvphikem/hd.yinyuetai.com/5AC80165F11A32EBBFD53F24DCDDA90D.mp4?sc=5ea95dc33763e01b");
    }

    public void play(View view) {
        Render render = mVideoView.getRender();
        mPlayer.setRender(render);
        mPlayer.prepareAsync(new IIMediaPlayer.OnPrepareListener() {
            @Override
            public void onPrepared() {
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

    public void seek(View view) {
        mPlayer.seek(30);
    }

    public void stop(View view) {
        mPlayer.stop();
    }

}
