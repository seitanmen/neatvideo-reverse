cd /Users/seitan/Desktop/work/reverse/work/omo/wp_e9
E=../wp_motion6/eval_v6temporal
python3 tools/e9_eval.py park_joy/p_y3 --test base_pyr15=runs/mtn_pyr15/park_joy/p_y3/out.mp4 --test "genuine=genuine/progressive/Event_バージョン 1_0001_0003" --no-pairs --out $E/base_park_joy_p_y3.json > $E/base_park_joy.log 2>&1 &
python3 tools/e9_eval.py sintel/p_y3 --test base_sim3c=runs/mtn_sim3c/sintel/p_y3/out.mp4 --test "genuine=genuine/progressive/Event_バージョン 1_0001_0005" --no-pairs --out $E/base_sintel_p_y3.json > $E/base_sintel.log 2>&1 &
wait
python3 tools/e9_eval.py old_town_cross/p_y3 --test base_pyr15=runs/mtn_pyr15/old_town_cross/p_y3/out.mp4 --test "genuine=genuine/progressive/Event_バージョン 1_0001_0001" --no-pairs --out $E/base_old_town_cross_p_y3.json > $E/base_otc.log 2>&1
