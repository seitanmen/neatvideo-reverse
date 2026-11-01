import json, sys
S="/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/verify_n4d"
base = json.load(open(f"{S}/my1/plugin_X-thin.f32.cp_0.json"))
txt = json.dumps(base)
def nest(d, mixed):
    if not mixed: return "["*d + "]"*d
    o, c = "", ""
    for k in range(d):
        if k % 2: o += '{"k": '; c = "}" + c
        else: o += "["; c = "]" + c
    return o + "1" + c
# total depth of the whole document: the top-level object counts 1 -> inner nest d-1 gives depth d
for d in (255, 256, 257, 3000, 100000):
    for mixed in (False, True):
        inner = nest(d - 1, mixed)
        doc = txt[:-1] + ', "zz": ' + inner + "}"
        open(f"{S}/r2/m1/d{d}_{'mix' if mixed else 'arr'}.json", "w").write(doc)
open(f"{S}/r2/m1/valid.json","w").write(txt)
