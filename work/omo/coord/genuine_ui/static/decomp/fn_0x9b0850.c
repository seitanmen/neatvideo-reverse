// FUN_009b0850 size=284

float FUN_009b0850(long param_1,long param_2)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  long lVar4;
  long lVar5;
  float fVar6;
  
  uVar1 = *(uint *)(&UNK_001039e0 + lRam0000000001489458);
  uVar2 = *(uint *)(&UNK_00031988 + lRam0000000001489468);
  iVar3 = (int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10);
  if (iVar3 != 0) {
    if (iRam0000000001488474 != 0) {
      uVar1 = uVar1 >> 0x1c | uVar1 << 4;
    }
    *(uint *)(&UNK_0085b510 + lRam0000000001489460) = uVar1;
  }
  iRam0000000001488474 = iVar3;
  if (param_1 <= param_2) {
    param_2 = param_1;
  }
  fVar6 = 0.0;
  if (lRam0000000001488128 <= param_2) {
    if (param_2 < lRam0000000001488130) {
      lVar4 = 1;
    }
    else if (param_2 < lRam0000000001488138) {
      lVar4 = 2;
    }
    else if (param_2 < lRam0000000001488140) {
      lVar4 = 3;
    }
    else {
      if (lRam0000000001488148 <= param_2) {
        return 5.0;
      }
      lVar4 = 4;
    }
    lVar5 = *(long *)(lVar4 * 8 + 0x1488120);
    fVar6 = (float)(param_2 - lVar5) / (float)(*(long *)(lVar4 * 8 + 0x1488128) - lVar5) +
            (float)lVar4;
  }
  return fVar6;
}


