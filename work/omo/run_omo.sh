#!/bin/zsh
# usage: run_omo.sh <wp_name>   (run from anywhere; cwd is set to the project root)
wp=$1
root=/Users/seitan/Desktop/work/reverse
cd $root || exit 1
mkdir -p sandbox/omo/$wp/logs
prompt="あなたは Neat Video v6 リバース・再現プロジェクトの作業者です (プロジェクトの根 $root)。
まず sandbox/omo/COMMON.md と sandbox/omo/$wp/INSTRUCTIONS.md を全部読み、それに厳密に従ってください。
書いてよいのは sandbox/omo/$wp/ 以下だけです (シェルからの書き込みも同じ。/tmp・repro/・repro/out/・harness/・Agents.md・exp_results.txt・他の wp_* には書かない)。
安全上の禁止事項 (本家バンドルを読み込む新しいプログラムを書かない・ビルドしない・実行しない、本家は neatvideo_ofx_614/analysis/harness/nvh だけから selftest の終了コード 3 を確かめてから呼ぶ、保護判定・難読化・トークン表・認証の回避や調査をしない、本家のメモリに書かない、nvh の入力制限を守る、make を実行しない) は例外なしです。
sandbox/omo/$wp/ には前任の作業者が途中まで作ったファイル (notes/・logs/・base/ など) が残っています。使えるものは中身を確かめてから引き継いでかまいませんが、正しさは自分で確かめてください。
ハーネスの新サブコマンドが要るときは harness_request.md と nvh_patch.diff を書くだけにし、それ以外の作業を最後まで進めてください。
成果: sandbox/omo/$wp/REPORT.md と MERGE_PLAN.md (日本語、観測と推定を区別、数値は実測のみ)。
ユーザーへの質問はせず (非対話で動いています)、迷ったら REPORT.md の「要相談」に書いて範囲内で進めてください。
最後の返答は、要素ごとの結論 (1〜3 行)、本家との比較の件数と不一致数、統括の判断が要る事項 (ハーネス要求・要相談) だけを簡潔に。"
exec omo -p --name "nv-$wp" --session-id "nv3-$wp" --model opencode-free/big-pickle \
  --permission-preset workspace --permission "eval=allow" \
  "$prompt" < /dev/null > sandbox/omo/$wp/logs/omo_run.log 2>&1
