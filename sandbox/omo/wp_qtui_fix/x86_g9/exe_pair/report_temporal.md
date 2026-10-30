## x86_exe_pair.py --report (verify_temporal.py inputs)
| kind | calls identical/total | words differ | both NaN (payload / sign) | NaN on one side | finite (float32 view) | size differs |
|---|---|---|---|---|---|---|
| temporal_cases.bin | 0/1 | 184270 | 24187 | 0 | 160083 | 0 |
{'calls': 1, 'same': 0, 'bytes': 445632632, 'words': 111408158, 'diff': 184270, 'both_nan': 24187, 'one_nan': 0, 'finite': 160083}

## per case (v5pair.py; "input" = the planes of the case only, the profile and parameters of a "finite input" case can hold inf, e.g. verify_spatial.py curves with 1e30 / inf)
| kind | input | cases identical/total | words | both NaN, bits differ | NaN on one side | finite differ |
|---|---|---|---|---|---|---|
| flow | finite input | 44/44 | 6,384 | 0 | 0 | 0 |
| dense | finite input | 44/44 | 421,138 | 0 | 0 | 0 |
| warp | finite input | 8/8 | 35,851 | 0 | 0 | 0 |
| warp | NaN/inf in input | 2/8 | 35,851 | 30 | 0 | 0 |
| pyramid | finite input | 24/24 | 107,553 | 0 | 0 | 0 |
| blocksum | finite input | 60/60 | 520 | 0 | 0 | 0 |
| refine | finite input | 60/60 | 5,720 | 0 | 0 | 0 |
| blocksum | NaN/inf in input | 11/60 | 520 | 274 | 0 | 0 |
| refine | NaN/inf in input | 12/60 | 5,720 | 2,513 | 0 | 0 |
| temporal | finite input | 37/69 | 102,082,712 | 0 | 0 | 127,602 |
| frame | finite input | 12/17 | 8,645,063 | 0 | 0 | 32,474 |
| temporal | NaN/inf in input | 2/15 | 42,614 | 11,108 | 0 | 7 |
| frame | NaN/inf in input | 0/5 | 12,154 | 10,262 | 0 | 0 |

## NaN bit-pattern pairs (arm64, x86_64), most common
('0x7fc12345', '0x7fc00000') 2972
('0xffc12345', '0x7fc00000') 2941
('0x7fc00000', '0xffc00000') 2832
('0xffc00000', '0x7fc12345') 2253
('0xffc00000', '0x7fc00000') 1580
('0xffc12345', '0xffc00000') 1470
('0x7fe00000', '0x7fc00001') 1205
('0x7fc00000', '0x7fc12345') 800
