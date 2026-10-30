set -x
cd /Users/seitan/Desktop/work/reverse/repro
V=../sandbox/omo/coord/genuine_ui/verify/dv
B=../sandbox/omo/coord/backup/denoise_video_v5.py.20261026_pre_genuineui
python3 denoise_video_v5.py ../test.mp4 $V/new_default.mkv --start 300 --frames 8 --crf 0 --jobs 4 --audio none > $V/new_default.log 2>&1; echo rc=$?
PYTHONPATH=. python3 $B ../test.mp4 $V/old_default.mkv --start 300 --frames 8 --crf 0 --jobs 4 --audio none > $V/old_default.log 2>&1; echo rc=$?
python3 denoise_video_v5.py ../test.mp4 $V/new_sq.mkv --start 300 --frames 8 --crf 0 --jobs 4 --audio none --area 40,60,96 --profile spatial > $V/new_sq.log 2>&1; echo rc=$?
PYTHONPATH=. python3 $B ../test.mp4 $V/old_sq.mkv --start 300 --frames 8 --crf 0 --jobs 4 --audio none --area 40,60,96 --profile spatial > $V/old_sq.log 2>&1; echo rc=$?
python3 denoise_video_v5.py ../test.mp4 $V/new_rect.mkv --start 300 --frames 4 --crf 0 --jobs 4 --audio none --area 40,60,64,160 --profile spatial > $V/new_rect.log 2>&1; echo rc=$?
python3 denoise_video_v5.py ../test.mp4 $V/new_rect_clamp.mkv --start 300 --frames 2 --crf 0 --jobs 4 --audio none --area 340,600,216,512 > $V/new_rect_clamp.log 2>&1; echo rc=$?
python3 denoise_video_v5.py ../test.mp4 $V/bad1.mkv --start 300 --frames 2 --area 1,2 > $V/bad1.log 2>&1; echo rc=$?
python3 denoise_video_v5.py ../test.mp4 $V/bad2.mkv --start 300 --frames 2 --area 1,2,3,4,5 > $V/bad2.log 2>&1; echo rc=$?
python3 denoise_video_v5.py ../test.mp4 $V/bad3.mkv --start 300 --frames 2 --area a,b,c > $V/bad3.log 2>&1; echo rc=$?
for f in new_default old_default new_sq old_sq; do ffmpeg -v error -i $V/$f.mkv -f rawvideo -pix_fmt yuv420p - | shasum -a 256; done
