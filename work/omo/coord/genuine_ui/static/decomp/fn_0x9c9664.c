// FUN_009c9664 size=268

void FUN_009c9664(long param_1)

{
  uint uVar1;
  long lVar2;
  ulong uVar3;
  uint uVar4;
  
  lVar2 = 0;
  uVar3 = param_1 + 0x1fU & 0xffffffffffffffe0;
  *(ulong *)(&UNK_00010020 + param_1) = uVar3;
  uVar4 = 0x93f02c74;
  do {
    uVar1 = (uVar4 + (uVar4 >> 0x10)) * 0x101 + (uVar4 >> 0x18);
    uVar4 = ((int)(CONCAT44(uVar1,uVar4) >> 0x10) + (uVar1 & 0xffff)) * 0x101 + (uVar1 >> 8 & 0xff);
    uVar1 = ((uVar1 & 0xffff) + (uVar4 & 0xffff)) * 0x101 + (uVar4 >> 8 & 0xff);
    uVar4 = ((uVar4 & 0xffff) + (uVar1 & 0xffff)) * 0x101 + (uVar1 >> 8 & 0xff);
    uVar1 = ((uVar1 & 0xffff) + (uVar4 & 0xffff)) * 0x101 + (uVar4 >> 8 & 0xff);
    uVar4 = ((uVar4 & 0xffff) + (uVar1 & 0xffff)) * 0x101 + (uVar1 >> 8 & 0xff);
    uVar1 = ((uVar1 & 0xffff) + (uVar4 & 0xffff)) * 0x101 + (uVar4 >> 8 & 0xff);
    uVar4 = ((uVar4 & 0xffff) + (uVar1 & 0xffff)) * 0x101 + (uVar1 >> 8 & 0xff);
    uVar1 = ((uVar1 & 0xffff) + (uVar4 & 0xffff)) * 0x101 + (uVar4 >> 8 & 0xff);
    uVar4 = uVar1 & 0xffff | ((uVar1 + uVar4) * 0x101 + (uVar1 >> 8 & 0xff)) * 0x10000;
    *(float *)(uVar3 + lVar2) = (float)(long)((ulong)(uVar1 & 0x7f) - 0x3f) * 0.00390625;
    lVar2 = lVar2 + 4;
  } while (lVar2 != 0x10000);
  return;
}


