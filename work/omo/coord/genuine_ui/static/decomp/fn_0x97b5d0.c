// FUN_0097b5d0 size=328

void FUN_0097b5d0(long param_1,long param_2,long param_3,long param_4,long param_5,long param_6,
                 ulong param_7)

{
  long lVar1;
  long lVar2;
  long lVar3;
  ulong uVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  
  if (cRam00000000014a67a8 == '\0') {
    uVar4 = 0;
    if ((long)param_7 < 1) {
      return;
    }
  }
  else {
    uVar4 = param_7 & 0xfffffffffffffffc;
    func_0x009f1c7c(param_1,param_2,param_3,param_4,param_5,param_6,uVar4);
    if ((long)param_7 <= (long)uVar4) {
      return;
    }
  }
  lVar2 = 0;
  lVar3 = uVar4 * 4;
  do {
    lVar1 = lVar2 * 4;
    fVar5 = *(float *)(param_1 + lVar3 + lVar1);
    fVar7 = *(float *)(param_3 + lVar3 + lVar1);
    fVar6 = fVar5 * 0.2989 + *(float *)(param_2 + lVar3 + lVar1) * 0.5866 + fVar7 * 0.1145;
    *(float *)(param_4 + lVar3 + lVar1) = fVar6;
    *(float *)(param_5 + lVar3 + lVar1) = (fVar5 - fVar6) * 0.71316504;
    *(float *)(param_6 + lVar3 + lVar1) = (fVar7 - *(float *)(param_4 + lVar3 + lVar1)) * 0.56465274
    ;
    lVar2 = lVar2 + 1;
  } while (param_7 - uVar4 != lVar2);
  return;
}


