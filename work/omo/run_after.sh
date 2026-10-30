#!/bin/zsh
# wait for the first queue, then resume the WPs whose harness requests were answered
cd /Users/seitan/Desktop/work/reverse/sandbox/omo
while pgrep -f "run_queue.sh 2 wp_unverified" >/dev/null; do sleep 60; done
export OMO_MSG="統括がハーネス要求に回答しました。自分のフォルダの harness_response.md を読んでください (追加されたサブコマンドは引数の向きが差分案と異なるものがあります。回答の記述と nvh の usage に従うこと)。sandbox/omo/COMMON.md と自分の INSTRUCTIONS.md の規則はそのまま有効です (書き込みは自分の wp フォルダだけ、repro/ のモジュールは base/ のコピーだけを PYTHONDONTWRITEBYTECODE=1 で import する (repro/ を sys.path に入れない)、本家は nvh selftest の終了コード 3 を確かめてから nvh だけで呼ぶ、保護判定・難読化の回避や調査をしない、make を実行しない)。追加されたサブコマンドで移植の部品を本家とビット照合し (入力の内訳を広く、誤りを注入して検出できることも確かめる)、却下・保留された部分は静的解読で進めてください。最後に REPORT.md と MERGE_PLAN.md を更新し、返答は要素ごとの結論・本家との比較の件数と不一致数・統括の判断が要る事項だけを簡潔に。"
echo "$(date '+%F %T') run_after: resume temporal, motion" >> queue.log
G=zai/glm-5.3
./run_queue.sh 2 wp_temporal=$G wp_motion=$G
