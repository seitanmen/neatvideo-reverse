// func entry: 00112c60 name: FUN_00112c60 size: 932

long * FUN_00112c60(long *param_1,undefined8 param_2,undefined8 *param_3,undefined4 param_4,
                   int param_5)

{
  char *pcVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  int iVar6;
  undefined8 *puVar7;
  long lVar8;
  ulong uVar9;
  undefined8 uVar10;
  size_t sVar11;
  long *plVar12;
  int *piVar13;
  int *local_78;
  int *local_70;
  int local_68;
  int local_64;
  undefined1 auStack_60 [8];
  undefined8 local_58;
  
  puVar7 = (undefined8 *)FUN_0089c0a4(param_1,param_2,0x8001000);
  *puVar7 = &DAT_013c72d8;
  puVar7[2] = &DAT_013c74b0;
  puVar7[10] = &DAT_013c7500;
  FUN_000673dc(puVar7 + 10,param_1);
  *param_1 = (long)&DAT_013c72d8;
  param_1[2] = (long)&DAT_013c74b0;
  param_1[10] = (long)&DAT_013c7500;
  piVar13 = (int *)*param_3;
  param_1[0x12] = (long)piVar13;
  if (1 < *piVar13 + 1U) {
    LOAcquire();
    iVar2 = *piVar13;
    *piVar13 = iVar2 + 1;
    piVar13 = (int *)CONCAT44((int)((ulong)piVar13 >> 0x20),iVar2);
    LORelease();
  }
  *(undefined4 *)(param_1 + 0x13) = param_4;
  FUN_0011314c(piVar13,param_1 + 6,param_1);
  FUN_0086f794(param_1[7],param_3);
  FUN_003d02f4(param_1[7],param_1);
  FUN_003d02f4(param_1[9],param_1);
  FUN_003d0e8c(auStack_60,param_1[7],"2textChanged()",param_1,"1onTextChanged()",0);
  FUN_003d24d8(auStack_60);
  param_1[0x11] = 0;
  if (param_5 == 0) {
    FUN_00753798(param_1[8]);
    FUN_0084e420(param_1[9],0x400400);
    lVar8 = FUN_0084e758(param_1[9],0x400);
    param_1[0x11] = lVar8;
    FUN_0074abd0(lVar8,0);
  }
  else {
    FUN_0087189c(param_1[7],1);
    FUN_0084e420(param_1[9],0x400);
  }
  FUN_0073abe0(param_1[6],0x10,0xe,0x10,1);
  FUN_0084ea88(param_1[9],0);
  uVar9 = (**(code **)(*param_1 + 0x70))(param_1);
  uVar10 = DAT_0155b5a8;
  local_58 = (int *)FUN_002efa1c("PresetInfoDlgHeight",0x13);
  uVar4 = FUN_001806a0(uVar10,&local_58,uVar9 >> 0x20,0);
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar2 = *local_58;
      *local_58 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_00112e50;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_00112e50:
  uVar5 = (**(code **)(*param_1 + 0x70))(param_1);
  local_58 = (int *)CONCAT44(uVar4,uVar5);
  FUN_00751eb8(param_1,&local_58);
  uVar10 = FUN_00752dd0(param_1);
  FUN_0073aa28(uVar10,&local_64,0,&local_68,0);
  FUN_004e4f6c(&local_58,*(long *)(param_1[7] + 0x28) + 0x38);
  pcVar1 = DAT_0141c0a0;
  if (DAT_0141c0a0 == (char *)0x0) {
    sVar11 = 0xffffffff;
  }
  else {
    sVar11 = _strlen(DAT_0141c0a0);
  }
  local_70 = (int *)FUN_002efa1c(pcVar1,sVar11);
  iVar2 = FUN_004e52a0(&local_58,&local_70,0xffffffff);
  plVar12 = (long *)FUN_007496f0(param_1[7]);
  iVar6 = (**(code **)(*plVar12 + 0xe0))(plVar12,9,0,0);
  if (*local_70 != -1) {
    if (*local_70 != 0) {
      LOAcquire();
      iVar3 = *local_70;
      *local_70 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_00112f48;
    }
    FUN_00287df8(local_70,2,8);
  }
LAB_00112f48:
  FUN_0074c210(param_1,iVar6 + iVar2 + local_64 + local_68);
  local_78 = (int *)FUN_002efa1c("InfoPreset.png",0xe);
  FUN_00068450(&local_70,&local_78,0,1);
  FUN_007503ec(param_1,&local_70);
  FUN_004bbe60(&local_70);
  if (*local_78 != -1) {
    if (*local_78 != 0) {
      LOAcquire();
      iVar2 = *local_78;
      *local_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_00112fc4;
    }
    FUN_00287df8(local_78,2,8);
  }
LAB_00112fc4:
  FUN_007d1e0c(param_1[7],1);
  FUN_007d1d58(param_1[7],1);
  FUN_004e4f9c(&local_58);
  return param_1;
}


