## x86_exe_pair.py --report (verify_profile.py inputs)
| kind | calls identical/total | words differ | both NaN (payload / sign) | NaN on one side | finite (float32 view) | size differs |
|---|---|---|---|---|---|---|
| -j | 2/2 | 0 | 0 | 0 | 0 | 0 |
| floor | 1/1 | 0 | 0 | 0 | 0 | 0 |
| noise 0x0 | 1/1 | 0 | 0 | 0 | 0 | 0 |
| noise 0x12345678 | 1/1 | 0 | 0 | 0 | 0 | 0 |
| noise 0xdeadbeef | 1/1 | 0 | 0 | 0 | 0 | 0 |
{'calls': 6, 'same': 6, 'bytes': 2413228, 'words': 0, 'diff': 0, 'both_nan': 0, 'one_nan': 0, 'finite': 0}

## NaN bit-pattern pairs (arm64, x86_64), most common
