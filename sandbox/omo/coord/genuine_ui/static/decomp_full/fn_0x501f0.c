// FUN_000501f0 size=276

void FUN_000501f0(undefined8 *param_1)

{
  int iVar1;
  long lVar2;
  int *piVar3;
  
  *param_1 = &DAT_013c16a8;
  param_1[2] = &DAT_013c1958;
  param_1[6] = &DAT_013c19a8;
  if ((long *)param_1[0x29] != (long *)0x0) {
    (**(code **)(*(long *)param_1[0x29] + 8))();
  }
  param_1[0x29] = 0;
  *(undefined1 *)(param_1 + 0x2b) = 1;
  lVar2 = param_1[0x33];
  if (*(int *)(lVar2 + 0x10) != -1) {
    if (*(int *)(lVar2 + 0x10) != 0) {
      LOAcquire();
      iVar1 = *(int *)(lVar2 + 0x10);
      *(int *)(lVar2 + 0x10) = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0005027c;
      lVar2 = param_1[0x33];
    }
    FUN_002a9a28(lVar2,FUN_000557bc);
  }
LAB_0005027c:
  piVar3 = (int *)param_1[0x2c];
  if (*piVar3 != -1) {
    if (*piVar3 != 0) {
      LOAcquire();
      iVar1 = *piVar3;
      *piVar3 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000502b0;
      piVar3 = (int *)param_1[0x2c];
    }
    FUN_00287df8(piVar3,0x10,8);
  }
LAB_000502b0:
  thunk_FUN_009409f0(param_1 + 0x27);
  piVar3 = (int *)param_1[0x22];
  if (*piVar3 != -1) {
    if (*piVar3 != 0) {
      LOAcquire();
      iVar1 = *piVar3;
      *piVar3 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000502ec;
      piVar3 = (int *)param_1[0x22];
    }
    FUN_00287df8(piVar3,2,8);
  }
LAB_000502ec:
  FUN_000677ac(param_1 + 6);
  FUN_00960bd0(param_1);
  return;
}


