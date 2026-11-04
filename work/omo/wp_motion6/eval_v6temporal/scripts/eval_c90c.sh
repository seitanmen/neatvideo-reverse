#!/bin/bash
# usage: eval_c90c.sh NAME TAG1[,TAG2..] clip...   -> eval_v6temporal/NAME_<clip>_p_y3.json (baseline + tags + genuine)
cd /Users/seitan/Desktop/work/reverse/work/omo/wp_e9
NAME=$1; TAGS=$2; shift 2
E=../wp_motion6/eval_v6temporal
for c in "$@"; do
  case $c in old_town_cross) G=0001;; park_joy) G=0003;; sintel) G=0005;; esac
  if [ $c = sintel ]; then B="base_sim3c=runs/mtn_sim3c/$c/p_y3/out.mp4"; else B="base_pyr15=runs/mtn_pyr15/$c/p_y3/out.mp4"; fi
  T=(--test "$B")
  IFS=, read -ra TT <<< "$TAGS"
  for t in "${TT[@]}"; do T+=(--test "$t=runs/mtn_$t/$c/p_y3/out.mp4"); done
  T+=(--test "genuine=genuine/progressive/Event_バージョン 1_0001_$G")
  python3 tools/e9_eval.py $c/p_y3 "${T[@]}" --no-pairs --out $E/${NAME}_${c}_p_y3.json > $E/logs/eval_${NAME}_$c.log 2>&1 &
done
wait
for c in "$@"; do grep -v "^wrote" $E/logs/eval_${NAME}_$c.log | tail -4; done
