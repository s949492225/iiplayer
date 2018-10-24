package com.syiyi.player;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        final IiMediaPlayer mPlayer = new IiMediaPlayer();
//        mPlayer.setDataSource("/sdcard/DCIM/Camera/VID_20180420_143223.mp4");
        mPlayer.setDataSource("http://220.194.236.214/2/v/x/k/w/vxkwfozzamnhdwuiekdoukkvphikem/hd.yinyuetai.com/5AC80165F11A32EBBFD53F24DCDDA90D.mp4?sc=5ea95dc33763e01b");
        mPlayer.prepareAsyn();
        findViewById(R.id.begin).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mPlayer.play();
            }
        });
    }

}
