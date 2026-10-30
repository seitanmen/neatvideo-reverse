## x86_exe_pair.py --report (verify_spatial.py inputs)
| kind | calls identical/total | words differ | both NaN (payload / sign) | NaN on one side | finite (float32 view) | size differs |
|---|---|---|---|---|---|---|
| spatial_cases.bin | 0/1 | 668088 | 667858 | 0 | 230 | 0 |
{'calls': 1, 'same': 0, 'bytes': 570552448, 'words': 142638112, 'diff': 668088, 'both_nan': 667858, 'one_nan': 0, 'finite': 230}

## per case (v5pair.py; "input" = the planes of the case only, the profile and parameters of a "finite input" case can hold inf, e.g. verify_spatial.py curves with 1e30 / inf)
| kind | input | cases identical/total | words | both NaN, bits differ | NaN on one side | finite differ |
|---|---|---|---|---|---|---|
| gauss | finite input | 42/42 | 52,548 | 0 | 0 | 0 |
| uniform | finite input | 320/320 | 346,272 | 0 | 0 | 0 |
| median | finite input | 36/36 | 37,428 | 0 | 0 | 0 |
| atrous | finite input | 108/108 | 449,136 | 0 | 0 | 0 |
| exp | finite input | 31/36 | 37,428 | 0 | 0 | 133 |
| mapc | finite input | 36/36 | 37,428 | 0 | 0 | 0 |
| curve | finite input | 42/42 | 37,650 | 0 | 0 | 0 |
| down2 | finite input | 27/27 | 9,178 | 0 | 0 | 0 |
| to_grid | finite input | 81/81 | 38,635 | 0 | 0 | 0 |
| gauss | NaN/inf in input | 10/24 | 37,300 | 1,963 | 0 | 0 |
| uniform | NaN/inf in input | 42/192 | 298,400 | 243,682 | 0 | 0 |
| median | NaN/inf in input | 24/24 | 37,300 | 0 | 0 | 0 |
| atrous | NaN/inf in input | 12/72 | 447,600 | 256,474 | 0 | 0 |
| exp | NaN/inf in input | 16/24 | 37,300 | 0 | 0 | 97 |
| mapc | NaN/inf in input | 14/25 | 37,307 | 701 | 0 | 0 |
| curve | NaN/inf in input | 24/24 | 37,300 | 0 | 0 | 0 |
| down2 | NaN/inf in input | 17/21 | 9,158 | 34 | 0 | 0 |
| to_grid | NaN/inf in input | 51/63 | 38,501 | 196 | 0 | 0 |
| spatial | finite input | 40/42 | 84,147,235 | 19,107 | 0 | 0 |
| spatial_mix | finite input | 32/36 | 56,318,667 | 29,515 | 0 | 0 |
| spatial | NaN/inf in input | 2/6 | 48,917 | 39,254 | 0 | 0 |
| spatial_mix | NaN/inf in input | 0/6 | 92,123 | 76,932 | 0 | 0 |

## NaN bit-pattern pairs (arm64, x86_64), most common
('0x7fc00000', '0xffc00000') 77726
('0xffc12345', '0xffc00000') 29282
('0xffc00000', '0x7fc00001') 27626
('0xffc12345', '0x7fc00001') 25217
('0xffc00000', '0x7fc00000') 24700
('0xffc00000', '0xffc12345') 24376
('0xffc12345', '0x7fc00000') 24149
('0xffc00000', '0x7fe00000') 23140
