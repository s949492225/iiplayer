# iiplayer
ffmpeg media_codec 面向对象的方式编写的播放器
面向对象的方式编写的播放器

支持ffmpeg软解码和media_codec硬解码

输出画面采用opengles 

软解码用openglesl来把yuv420格式转换为rgb

硬解码采用media_codec自带的渲染进行输出

目前支持 视频部分信息读取，快进，快退，暂停，恢复，停止等功能。

