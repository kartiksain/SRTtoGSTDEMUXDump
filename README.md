# SRTtoGSTDEMUXDump
This Project is to get data via SRT protocol and listen through gstreamer  and do demuxing then dump

A streaming transport protocol that is being used more and more is SRT ( Github ).

You will build a small program that takes an SRT stream and:

Parse the TS-Packets and prints some information about its containing streams to stdout.
Demux the TS-packets into bitstreams in two separate folders audio and video
You can use ffmpeg to generate an SRT stream from an mp4 eg. big buck bunny

ffmpeg -stream_loop -1 -re -i bbb_sunflower_1080p_60fps_normal.mp4 -f mpegts "srt://127.0.0.1:1234?mode=listener&pkt_size=1316"
Expected result
The created files should be put in two separate folders audio and video where the files have the suffix .raw. You can verify that the output files are correct by using ffplay, e.g. the video: ffplay video.raw
The stdout should show any information about the ts-packet streams


Machine Setup to Build
1) Ubuntu 20.04 LTS
2) SRT with ffmpeg install


Build Instruction

sh# g++ SRTtoGSTDEMUXDump.cpp -o test `pkg-config --cflags --libs gstreamer-1.0'


Usage:

./test  IP port videopath audiopath


