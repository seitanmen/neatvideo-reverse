#!/bin/zsh
# usage: run.sh SCEN W H DEPTH [NFRAMES]
SCEN=$1; W=$2; H=$3; D=$4; N=${5:-12}
V=/Users/seitan/Desktop/work/reverse/sandbox/omo/coord/verify_qtui_paths/verify2
O=$V/out_${SCEN}_${W}x${H}_d${D}
rm -rf $O; mkdir -p $O
cd /Users/seitan/Desktop/work/reverse
case $D in 8) PF=yuv420p;; 16) PF=yuv420p16le;; 32) PF=yuv420p;; esac
if [ $D = 32 ]; then
  # float32 planes: code value/255 (limited range) built from 8-bit via python
  ffmpeg -y -nostdin -v error -i test.mp4 -vf "scale=$W:$H" -pix_fmt yuv420p -frames:v $N -f rawvideo - | python3 -c "
import sys,numpy as np
W,H=$W,$H; fs=W*H+2*(W//2)*(H//2)
d=sys.stdin.buffer.read()
a=np.frombuffer(d,np.uint8).astype('<f4')/np.float32(255)
sys.stdout.buffer.write(a.tobytes())" | python3 $V/drv2.py $SCEN $O $W $H 24 --depth 32 > $O/stdout.bin 2> $O/stderr.log
elif [ $D = 16 ]; then
  ffmpeg -y -nostdin -v error -i test.mp4 -vf "scale=$W:$H" -pix_fmt yuv420p -frames:v $N -f rawvideo - | python3 -c "
import sys,numpy as np
d=sys.stdin.buffer.read()
a=np.frombuffer(d,np.uint8).astype('<u2')*256
sys.stdout.buffer.write(a.tobytes())" | python3 $V/drv2.py $SCEN $O $W $H 24 --depth 16 > $O/stdout.bin 2> $O/stderr.log
else
  ffmpeg -y -nostdin -v error -i test.mp4 -vf "scale=$W:$H" -pix_fmt yuv420p -frames:v $N -f rawvideo - | python3 $V/drv2.py $SCEN $O $W $H 24 > $O/stdout.bin 2> $O/stderr.log
fi
echo "pipeline exit ${pipestatus[*]}"
