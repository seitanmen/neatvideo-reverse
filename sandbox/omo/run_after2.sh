#!/bin/zsh
cd /Users/seitan/Desktop/work/reverse/sandbox/omo
while pgrep -f "run_queue.sh 2 wp_temporal" >/dev/null; do sleep 120; done
echo "$(date '+%F %T') run_after2: aft fix" >> queue.log
export OMO_MSG="統括からの修正依頼です。自分のフォルダの FIX_REQUEST.md と ../coord/verify_aft/VERDICT.md を読み、その項目を直してください。sandbox/omo/COMMON.md と自分の INSTRUCTIONS.md の規則はそのまま有効です (書き込みは自分の wp フォルダだけ、repro/ は base/ のコピーだけを PYTHONDONTWRITEBYTECODE=1 で import、本家は nvh selftest の終了コード 3 を確かめてから nvh だけで呼ぶ、保護判定・難読化・ハッシュ書き込み様コードの回避や調査をしない、make を実行しない)。最後に REPORT.md と MERGE_PLAN.md を更新し、返答は直した点・試験の件数・再測定の数値・統括の判断が要る事項だけを簡潔に。"
./run_queue.sh 1 wp_aft=zai/glm-5.3
