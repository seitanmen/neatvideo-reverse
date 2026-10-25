#!/bin/zsh
# usage: run_queue.sh <concurrency> wp[=model] ...   — at most <concurrency> jobs at once.
root=/Users/seitan/Desktop/work/reverse/sandbox/omo
k=$1; shift
typeset -a pids
for item in "$@"; do
  while true; do
    alive=(); for p in $pids; do kill -0 $p 2>/dev/null && alive+=$p; done; pids=($alive)
    [ ${#pids} -lt $k ] && break
    sleep 30
  done
  wp=${item%%=*}; model=${item#*=}; [ "$model" = "$item" ] && model=""
  echo "$(date '+%F %T') start $wp ${model}" >> $root/queue.log
  OMO_MODEL=$model $root/run_omo_job.sh $wp &
  pids+=$!
  sleep 20
done
wait
echo "$(date '+%F %T') queue done" >> $root/queue.log
