import json, sys
keys = ["yuv.Y","rgb.RGB","yuv.Cb","yuv.Cr","yuv.Y.static","yuv.Y.small","yuv.Y.large","rgb.RGB.static","rgb.RGB.small","rgb.RGB.large"]
print("| file | test | " + " | ".join(keys) + " |")
for f in sys.argv[1:]:
    d = json.load(open(f))
    for t, v in d["tests"].items():
        it = v["interior"]
        print("| %s | %s | " % (f.split("/")[-1], t) + " | ".join("%.2f" % it[k]["psnr"] if k in it else "-" for k in keys) + " |")
