#!/bin/zsh
# 8-bit regression: backup script vs new script, same args
cd /Users/seitan/Desktop/work/reverse/repro
V=../sandbox/omo/coord/highbits/verify/r1
OLD=../sandbox/omo/coord/backup/denoise_video_v5.py.20261026_pre_highbits
run() { # name src args...
  n=$1; s=$2; shift 2
  PYTHONPATH=$PWD python3 $OLD $s $V/${n}_old.mp4 "$@" > $V/${n}_old.log 2>&1 &
  python3 denoise_video_v5.py $s $V/${n}_new.mp4 "$@" > $V/${n}_new.log 2>&1 &
  wait
  for k in old new; do ffmpeg -v error -i $V/${n}_$k.mp4 -map 0:v -f framemd5 $V/${n}_$k.framemd5; md5 -q $V/${n}_$k.mp4 > $V/${n}_$k.md5; done
}
run a ../test.mp4 --start 300 --frames 8 --crf 0 --jobs 4
run b ../test_v02.mp4 --start 568 --frames 5 --crf 0 --jobs 4
run c ../test.mp4 --start 100 --frames 6 --crf 16 --jobs 4 --flicker 5 --profile spatial
echo done
