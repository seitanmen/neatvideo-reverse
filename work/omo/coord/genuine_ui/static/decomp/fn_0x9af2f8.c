// FUN_009af2f8 size=384

void FUN_009af2f8(long param_1,long param_2,long param_3,ulong param_4,long param_5,long param_6,
                 long param_7,long param_8,long param_9,long param_10,undefined8 param_11,
                 undefined8 param_12)

{
  long lVar1;
  uint uVar2;
  short sVar3;
  short sVar4;
  long lVar5;
  
  sVar4 = sRam0000000001488174;
  iRam000000000148834c = *(int *)(lRam0000000001488e18 + 0x1a594f8);
  if (lRam00000000015683f0 != 0) {
    iRam000000000148834c = iRam000000000148834c + (uint)(byte)(&UNK_000037c9)[lRam00000000015683f0];
  }
  sVar3 = *(short *)(&UNK_00025bc8 + param_1);
  param_6 = *(long *)(&UNK_01247820 + (param_4 & 0xffffffff) * 8) * param_6;
  lVar1 = param_2 + param_8 * param_5 * 4 + param_7 * 4;
  lVar5 = param_3 + param_6 * param_10 +
          *(long *)(&UNK_01247820 + (param_4 & 0xffffffff) * 8) * param_9;
  if ((int)param_4 == 2) {
    FUN_009c06cc(lVar1,2,param_5 << 2,lVar5,2,param_6,param_11,param_12,1);
  }
  else {
    func_0x009c0e5c(lVar1,param_5 << 2,lVar5,param_4,param_6,param_11,param_12,1);
  }
  uRam0000000001488350 =
       (int)(CONCAT44((uRam0000000001488350 + (uRam0000000001488350 >> 0x10)) * 0x101 +
                      (uRam0000000001488350 >> 0x18),uRam0000000001488350) >> 0x10) + (int)param_7;
  if (sVar3 == -sVar4) {
    uVar2 = *(uint *)(&UNK_00a2e5a8 + lRam0000000001488e20);
    if (uRam0000000001488354 != 0) {
      uVar2 = uVar2 >> 0x15 | uVar2 << 0xb;
    }
    uRam0000000001488354 = *(uint *)(lRam0000000001488e30 + 0x7650f0);
    if (uRam0000000001488354 != 0) {
      uVar2 = uRam0000000001488354;
    }
    *(uint *)(&UNK_0119ea58 + lRam0000000001488e28) = uVar2;
  }
  return;
}


