// FUN_0014f4bc size=1228

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0014f4bc(long *param_1)

{
  bool bVar1;
  undefined8 uVar2;
  bool bVar3;
  byte bVar4;
  int iVar5;
  uint uVar6;
  long lVar7;
  long lVar8;
  long lVar9;
  undefined1 auStack_168 [264];
  undefined8 uStack_60;
  undefined8 uStack_58;
  undefined8 uStack_50;
  undefined8 uStack_48;
  
  *(undefined1 *)((long)param_1 + 0x152) = 0;
  lVar7 = func_0x009b8e98(param_1[0xe],param_1[0x10]);
  lVar8 = func_0x009b8e4c(param_1[0xe],param_1[0x10]);
  lVar9 = func_0x009b8d20(param_1[0xe],param_1[0x10]);
  *(undefined8 *)((long)param_1 + 0x184) = _UNK_0127a7b8;
  (**(code **)(*param_1 + 0x70))(param_1,lVar8 + lVar7 + lVar9,0);
  func_0x009b8c24(param_1[0xe]);
  func_0x0099df24(param_1[0xe]);
  *(undefined4 *)((long)param_1 + 0x34) = 1;
  param_1[7] = lVar7;
  uVar6 = *(uint *)(&UNK_00a9dd10 + lRam000000000141c288);
  if (*(int *)(&UNK_0126a348 + lRam000000000141c298) != 0) {
    if (iRam000000000141c280 != 0) {
      uVar6 = uVar6 >> 0x11 | uVar6 << 0xf;
    }
    *(uint *)(lRam000000000141c290 + 0x192d6c0) = uVar6;
  }
  iVar5 = func_0x009b1a5c(param_1[0xe],param_1[0x10],(char)param_1[0x27],&uStack_48,&uStack_50,
                          &uStack_58,&uStack_60);
  *(int *)((long)param_1 + 0x184) = iVar5;
  *(int *)((long)param_1 + 0x154) = (int)uStack_48;
  *(int *)(param_1 + 0x2b) = (int)uStack_50;
  *(int *)((long)param_1 + 0x15c) = (int)uStack_48 + (int)uStack_58 + -1;
  *(int *)(param_1 + 0x2c) = (int)uStack_50 + (int)uStack_60 + -1;
  if (*(char *)((long)param_1 + 0x6d) == '\0') {
    *(bool *)((long)param_1 + 0x152) = iVar5 != 5;
    bVar1 = true;
    if ((iVar5 != 1) && (iVar5 != 5)) {
      if (((char)param_1[0x2a] == '\0') || (lVar7 = func_0x009b87d0(param_1[0xe]), lVar7 == -1)) {
        bVar1 = false;
      }
      else {
        bVar1 = false;
        param_1[0x10] = param_1[lVar7 + 0x13];
        param_1[0x2f] = lVar7;
      }
    }
  }
  else {
    *(undefined1 *)((long)param_1 + 0x152) = 0;
    bVar1 = true;
  }
  lVar7 = param_1[7];
  param_1[8] = param_1[8] + lVar7;
  if (*(int *)((long)param_1 + 0x34) == 1) {
    param_1[9] = param_1[9] + lVar7;
    param_1[7] = 0;
  }
  else {
    param_1[10] = param_1[10] + lVar7;
    param_1[7] = 0;
    if ((*(int *)((long)param_1 + 0x34) == 2) && ((char)param_1[6] != '\0')) {
      *(undefined1 *)(param_1 + 6) = 0;
      if (iRam000000000155b550 != 1) {
        func_0x00af4a8c(0x155b548);
      }
      func_0x0097fb60(uRam000000000155b560);
    }
  }
  *(undefined4 *)((long)param_1 + 0x34) = 0;
  *(byte *)((long)param_1 + 0x152) =
       *(byte *)((long)param_1 + 0x152) & (*(byte *)((long)param_1 + 0x6d) ^ 1);
  func_0x0099df24(param_1[0xe]);
  *(undefined4 *)((long)param_1 + 0x34) = 1;
  param_1[7] = lVar8;
  if ((bool)(*(char *)((long)param_1 + 0x152) != '\0' & (bVar1 ^ 1U))) {
    func_0x001c1114(auStack_168);
    func_0x009c7318(param_1[0x10],uStack_48,uStack_50,uStack_58,uStack_60,auStack_168,
                    *(undefined4 *)(param_1[0x10] + 0xf0),1);
    iVar5 = func_0x009af718(param_1[0xe],auStack_168,1);
    if (iVar5 == 0) {
      if (*(char *)((long)param_1 + 0x6d) == '\0') {
        *(undefined4 *)(param_1 + 0x31) = 2;
        bVar4 = 1;
        bVar3 = false;
      }
      else {
        bVar4 = 0;
        bVar3 = true;
        *(undefined4 *)(param_1 + 0x31) = 1;
      }
    }
    else {
      uVar6 = func_0x0099da28(*(undefined4 *)((long)param_1 + 0x184));
      *(uint *)(param_1 + 0x31) = uVar6;
      bVar3 = (uVar6 & 0x39) != 0;
      bVar4 = *(byte *)((long)param_1 + 0x6d) ^ 1;
    }
    *(byte *)((long)param_1 + 0x152) = bVar4 & *(byte *)((long)param_1 + 0x152) & bVar3;
    func_0x009c3db4(auStack_168);
    lVar8 = param_1[7];
    param_1[8] = param_1[8] + lVar8;
    if (*(int *)((long)param_1 + 0x34) == 1) goto LAB_0014f7e4;
    param_1[10] = param_1[10] + lVar8;
    param_1[7] = 0;
    if ((*(int *)((long)param_1 + 0x34) == 2) && ((char)param_1[6] != '\0')) {
      *(undefined1 *)(param_1 + 6) = 0;
      if (iRam000000000155b550 != 1) {
        func_0x00af4a8c(0x155b548);
      }
      func_0x0097fb60(uRam000000000155b560);
    }
  }
  else {
    param_1[8] = param_1[8] + lVar8;
LAB_0014f7e4:
    param_1[9] = param_1[9] + lVar8;
    param_1[7] = 0;
  }
  *(undefined4 *)((long)param_1 + 0x34) = 0;
  func_0x0099df24(param_1[0xe]);
  *(undefined4 *)((long)param_1 + 0x34) = 1;
  param_1[7] = lVar9;
  bVar4 = *(byte *)((long)param_1 + 0x152);
  if (bVar1 || bVar4 == 0) {
LAB_0014f838:
    param_1[8] = param_1[8] + lVar9;
  }
  else {
    *(byte *)((long)param_1 + 0x152) = *(byte *)((long)param_1 + 0x6d) ^ 1;
    if (*(byte *)((long)param_1 + 0x6d) == 1) {
      bVar4 = 0;
      goto LAB_0014f838;
    }
    bVar4 = func_0x009a775c(param_1[0xe],param_1[0x10],0,0,0,0,0,0,0,0,1);
    *(byte *)((long)param_1 + 0x152) = bVar4;
    lVar9 = param_1[7];
    param_1[8] = param_1[8] + lVar9;
    if (*(int *)((long)param_1 + 0x34) != 1) {
      param_1[10] = param_1[10] + lVar9;
      param_1[7] = 0;
      if ((*(int *)((long)param_1 + 0x34) == 2) && ((char)param_1[6] != '\0')) {
        *(undefined1 *)(param_1 + 6) = 0;
        if (iRam000000000155b550 != 1) {
          func_0x00af4a8c(0x155b548);
        }
        func_0x0097fb60(uRam000000000155b560);
        bVar4 = *(byte *)((long)param_1 + 0x152);
      }
      goto LAB_0014f854;
    }
  }
  param_1[9] = param_1[9] + lVar9;
  param_1[7] = 0;
LAB_0014f854:
  *(undefined4 *)((long)param_1 + 0x34) = 0;
  bVar4 = bVar4 & (*(byte *)((long)param_1 + 0x6d) ^ 1);
  *(byte *)((long)param_1 + 0x152) = bVar4;
  if (bVar4 == 0) {
    func_0x009b8c84(param_1[0xe]);
    uVar2 = _UNK_010b4460;
    *(undefined8 *)((long)param_1 + 0x15c) = _UNK_010b4468;
    *(undefined8 *)((long)param_1 + 0x154) = uVar2;
  }
  return;
}


