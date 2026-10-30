#!/bin/zsh
# usage: run_omo_retry.sh <wp_name> [max_tries]
# Runs run_omo.sh; on a 429 rate limit, waits and continues the same session.
wp=$1; max=${2:-12}
root=/Users/seitan/Desktop/work/reverse
log=$root/sandbox/omo/$wp/logs/omo_run.log
cd $root || exit 1
$root/sandbox/omo/run_omo.sh $wp; rc=$?
n=1
while grep -q '^429' $log && [ $n -lt $max ]; do
  cp $log $log.try$n
  sleep 300
  omo -p --name "nv-$wp" --session-id "nv3-$wp" --model opencode-free/big-pickle \
    --permission-preset workspace --permission "eval=allow" \
    "レート制限で中断しました。COMMON.md と INSTRUCTIONS.md の規則のまま、中断したところから作業を続けてください。" \
    < /dev/null > $log 2>&1; rc=$?
  n=$((n+1))
done
exit $rc
