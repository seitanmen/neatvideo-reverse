// FUN_009b62b4 size=132

ulong FUN_009b62b4(float param_1)

{
  uint uVar1;
  uint uVar2;
  ulong uVar3;
  
  uVar2 = *(uint *)(&UNK_00cc4618 + lRam0000000001489470);
  uVar1 = (uVar2 ^ uVar2 >> 1) & 0x50504411;
  uVar2 = (uVar1 | uVar1 << 1) ^ uVar2;
  if ((int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10) != 1) {
    *(uint *)(&UNK_00c63d68 + lRam0000000001489478) = uVar2;
  }
  uVar3 = (ulong)((param_1 + 16.0) * 0.03125);
  if (7 < (long)uVar3) {
    uVar3 = 8;
  }
  return uVar3 & ((long)uVar3 >> 0x3f ^ 0xffffffffffffffffU);
}


