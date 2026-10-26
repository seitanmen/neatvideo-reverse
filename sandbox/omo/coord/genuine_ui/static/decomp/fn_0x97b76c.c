// FUN_0097b76c size=440

void FUN_0097b76c(long param_1,long param_2,long param_3,long param_4,long param_5,long param_6,
                 ulong param_7)

{
  uint uVar1;
  long lVar2;
  long lVar3;
  ulong uVar4;
  long lVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  
  uVar1 = uRam000000000147e0e6;
  if (uRam000000000147e0e0 >> 0x18 == 0) {
    uVar4 = (ulong)uRam000000000147e0e6;
    *(undefined8 *)(lRam000000000147e0f8 + 0x73b42f8) = 0x147e0e4;
    if (*(long *)(lRam000000000147e0f0 + 0x78233e0) != 0) {
      uVar4 = -(ulong)*(byte *)(*(long *)(lRam000000000147e0f0 + 0x78233e0) + (uVar4 & 0xffff));
    }
    param_6 = param_6 + ((ulong)(uVar1 >> 0x18) + (uVar4 + (uVar1 >> 0x10 & 0xff)) * 2) * 0x400;
  }
  uRam000000000147e0e0 =
       (int)(CONCAT44((uRam000000000147e0e0 + (uRam000000000147e0e0 >> 0x10)) * 0x101 +
                      (uRam000000000147e0e0 >> 0x18),uRam000000000147e0e0) >> 0x10) + (int)param_1;
  if (cRam00000000014a67a8 == '\0') {
    uVar4 = 0;
    if ((long)param_7 < 1) {
      return;
    }
  }
  else {
    uVar4 = param_7 & 0xfffffffffffffffc;
    func_0x009f1d0c(param_1,param_2,param_3,param_4,param_5,param_6,uVar4);
    if ((long)param_7 <= (long)uVar4) {
      return;
    }
  }
  lVar3 = 0;
  lVar5 = uVar4 * 4;
  do {
    lVar2 = lVar3 * 4;
    fVar6 = *(float *)(param_1 + lVar5 + lVar2);
    fVar7 = *(float *)(param_2 + lVar5 + lVar2);
    fVar8 = *(float *)(param_3 + lVar5 + lVar2);
    *(float *)(param_4 + lVar5 + lVar2) = fVar6 + fVar7 * 1.4022;
    *(float *)(param_5 + lVar5 + lVar2) = fVar6 + fVar8 * -0.3456 + fVar7 * -0.7145;
    *(float *)(param_6 + lVar5 + lVar2) = fVar6 + fVar8 * 1.771;
    lVar3 = lVar3 + 1;
  } while (param_7 - uVar4 != lVar3);
  return;
}


