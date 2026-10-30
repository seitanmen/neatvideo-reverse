## x86_exe_pair.py --report (verify_aft.py inputs)
| kind | calls identical/total | words differ | both NaN (payload / sign) | NaN on one side | finite (float32 view) | size differs |
|---|---|---|---|---|---|---|
| aft | 2/2 | 0 | 0 | 0 | 0 | 0 |
| combine | 1/1 | 0 | 0 | 0 | 0 | 0 |
| finish | 0/1 | 835 | 835 | 0 | 0 | 0 |
| shuffle | 1/1 | 0 | 0 | 0 | 0 | 0 |
| sim | 0/1 | 66 | 66 | 0 | 0 | 0 |
{'calls': 6, 'same': 4, 'bytes': 190226621, 'words': 37452000, 'diff': 901, 'both_nan': 901, 'one_nan': 0, 'finite': 0}

## NaN bit-pattern pairs (arm64, x86_64), most common
('0x7fc00000', '0xffc00000') 837
('0x7fe00000', '0x7fc00000') 7
('0x7fe00000', '0xffc12345') 6
('0x7fc00000', '0x7fc12345') 5
('0xffc00000', '0xffc12345') 4
('0xffc00000', '0x7fc12345') 4
('0x7fc12345', '0xffc12345') 4
('0xffc00000', '0x7fe00000') 4
