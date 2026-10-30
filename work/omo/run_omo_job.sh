#!/bin/zsh
# usage: run_omo_job.sh <wp_name>  — continue session nv3-<wp> until it ends without a 429.
wp=$1; root=/Users/seitan/Desktop/work/reverse
log=$root/sandbox/omo/$wp/logs/omo_run.log
cd $root || exit 1
cont_default="作業を中断したところから続けてください。sandbox/omo/COMMON.md と sandbox/omo/$wp/INSTRUCTIONS.md の規則はそのまま有効です (書き込みは sandbox/omo/$wp/ だけ、本家は nvh selftest の終了コード 3 を確かめてから nvh だけで呼ぶ、保護判定・難読化の回避や調査をしない、make を実行しない)。必要なら両方を読み直し、自分のフォルダの既存ファイルで進捗を確かめてから進めてください。ユーザーへの質問はせず、迷ったら REPORT.md の「要相談」に書いて進めてください。最後に REPORT.md と MERGE_PLAN.md を必ず書き、返答は要素ごとの結論・本家との比較の件数と不一致数・統括の判断が要る事項だけを簡潔に。"
cont="${OMO_MSG:-$cont_default}"
for n in {1..40}; do
  [ -f $log ] && cp $log $log.prev
  omo -p --name "nv-$wp" --session-id "nv3-$wp" --model "${OMO_MODEL:-opencode-free/big-pickle}" \
    --permission-preset workspace --permission "eval=allow" "$cont" < /dev/null > $log 2>&1
  rc=$?
  if grep -q '"code":"1308"' $log; then
    reset=$(grep -o 'reset at [0-9-]* [0-9:]*' $log | tail -1 | cut -c10-)
    now=$(date +%s); until=$(date -j -f "%Y-%m-%d %H:%M:%S" "$reset" +%s 2>/dev/null || echo $((now+1800)))
    w=$((until - now + 90)); [ $w -lt 60 ] && w=60
    echo "$(date '+%F %T') $wp quota; sleep $w" >> $root/sandbox/omo/queue.log
    sleep $w
  elif grep -q 'hit its usage limit\|No credential slots available' $log; then
    echo "$(date '+%F %T') $wp usage limit; sleep 1800" >> $root/sandbox/omo/queue.log
    sleep 1800
  elif grep -q '^429' $log; then
    echo "$(date '+%F %T') $wp rate; sleep 180" >> $root/sandbox/omo/queue.log
    sleep 180
  elif [ $rc -ne 0 ] && [ ! -f $root/sandbox/omo/$wp/REPORT.md ]; then
    echo "$(date '+%F %T') $wp error rc=$rc; retry in 60" >> $root/sandbox/omo/queue.log
    sleep 60
  else
    echo "$(date '+%F %T') $wp ended rc=$rc report=$([ -f $root/sandbox/omo/$wp/REPORT.md ] && echo y || echo n)" >> $root/sandbox/omo/queue.log
    exit $rc
  fi
done
