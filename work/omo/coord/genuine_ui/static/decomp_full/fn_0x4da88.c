// FUN_0004da88 size=296

ulong FUN_0004da88(long param_1,undefined8 param_2,undefined8 param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  long lVar4;
  ulong uVar5;
  undefined8 uVar6;
  undefined8 uVar7;
  ulong uVar8;
  double local_40;
  double dStack_38;
  
  lVar4 = FUN_003bbf58(&DAT_013c12b0);
  if (((((int)param_3 == 1) && (lVar4 != 0)) && (*(char *)(lVar4 + 0x1e0) != '\0')) &&
     (*(char *)(param_1 + 0x168) == '\0')) {
    local_40 = (double)((*(int *)(lVar4 + 0x1d8) - *(int *)(lVar4 + 0x1d0)) + 1) * 0.5 +
               (double)*(int *)(lVar4 + 0x1d0);
    dStack_38 = (double)((*(int *)(lVar4 + 0x1dc) - *(int *)(lVar4 + 0x1d4)) + 1) * 0.5 +
                (double)*(int *)(lVar4 + 0x1d4);
    uVar6 = FUN_0095e7cc(lVar4,&local_40);
    FUN_007d1ea8(lVar4);
    iVar2 = FUN_007d81e8();
    FUN_007d1df4(lVar4);
    iVar3 = FUN_007d81e8();
    local_40 = 0.0;
    dStack_38 = 0.0;
    uVar7 = FUN_0095e7cc(lVar4,&local_40);
    iVar1 = 0;
    if (0 < (int)uVar7) {
      iVar1 = -(int)uVar7;
    }
    uVar5 = (ulong)(uint)(iVar2 + (int)uVar6 + iVar1);
    iVar2 = (int)((ulong)uVar7 >> 0x20);
    iVar1 = 0;
    if (0 < iVar2) {
      iVar1 = -iVar2;
    }
    uVar8 = (ulong)(uint)(iVar3 + (int)((ulong)uVar6 >> 0x20) + iVar1);
  }
  else {
    uVar5 = FUN_00052f84(param_1,param_2,param_3);
    uVar8 = uVar5 >> 0x20;
  }
  return uVar5 & 0xffffffff | uVar8 << 0x20;
}


