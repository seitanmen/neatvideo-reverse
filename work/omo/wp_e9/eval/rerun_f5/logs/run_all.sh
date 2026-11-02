#!/bin/sh
# E9 frame-5 rerun (e9-f5-worker, 2026-11-02). B = main sources, C = aft2 worktree sources. One clip at a time, --jobs 4.
set -e
W=/Users/seitan/Desktop/work/reverse/work/omo/wp_e9
T=/Users/seitan/Desktop/work/reverse/.claude/worktrees/aft2/work/omo/wp_e9/tools
P="old_town_cross/p_y3 old_town_cross/p_y6 park_joy/p_y3 park_joy/p_y6 sintel/p_y3 sintel/p_y6"
I="old_town_cross/tff_y6 park_joy/tff_y6 sintel/tff_y3 sintel/tff_y6"
# progressive check: frame 0 with the main build reproduces A (runs/ours_frame)
python3 $T/run_ours.py $P --root $W --build $W/build --nvcore /Users/seitan/Desktop/work/reverse/native/libnvcore.dylib --profile-frame 0 --tag chk_f0 --jobs 4
# B progressive / interlaced (main sources)
python3 $T/run_ours.py $P --root $W --build $W/build --nvcore /Users/seitan/Desktop/work/reverse/native/libnvcore.dylib --profile-frame 5 --tag f5 --jobs 4
python3 $T/run_ours_fields.py $I --root $W --build $W/build_main_int3 --variants same_frames:same:4 --profile-frame 5 --tag f5 --jobs 4
# C progressive / interlaced (aft2 sources)
python3 $T/run_ours.py $P --root $W --build $W/build_aft2 --nvcore /Users/seitan/Desktop/work/reverse/.claude/worktrees/aft2/native/libnvcore.dylib --profile-frame 5 --tag f5q16 --jobs 4
python3 $T/run_ours_fields.py $I --root $W --build $W/build_aft2_int3 --variants same_frames:same:4 --profile-frame 5 --tag f5q16 --jobs 4
