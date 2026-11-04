#!/bin/bash
# usage: run_c90c.sh TAG GATE clip1 [clip2 ...]   (env extra via EXTRA_ENV)
cd /Users/seitan/Desktop/work/reverse
TAG=$1; GATE=$2; shift 2
E=work/omo/wp_motion6/eval_v6temporal; L=$E/logs
for c in "$@"; do
  if [ $c = sintel ]; then BASE=sim3c; SETS=(); else BASE=builtin; SETS=(--set similarity=pyramid); fi
  mkdir -p $E/${TAG}_log/$c
  rm -f $E/${TAG}_log/$c/*.jsonl
  env NV_NATIVE=0 NV6_C90C_BASE=$BASE NV6_C90C_GATE=$GATE NV6_C90C_LOG=$E/${TAG}_log/$c $EXTRA_ENV \
    python3 work/omo/wp_motion6/tools/e9_nvm.py $c/p_y3 --frames 60 --jobs 4 --tag $TAG --variant c90c "${SETS[@]}" \
    --profile-file work/omo/wp_e9/runs/mtn_spatial_nl08/$c/p_y3/ui_profile_spatial.json \
    --noise-level 0.8 --chroma-noise 0.3 --spatial-variant v6set0 > $L/${TAG}_$c.log 2>&1 &
done
wait
for c in "$@"; do tail -1 $L/${TAG}_$c.log; done
