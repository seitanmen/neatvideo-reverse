// FUN_00051f04 size=464

void FUN_00051f04(long *param_1,int param_2)

{
  uint uVar1;
  int iVar2;
  undefined8 uVar3;
  ulong uVar4;
  uint uVar5;
  undefined1 auStack_60 [8];
  undefined1 auStack_58 [32];
  undefined4 local_38;
  int local_34;
  int *local_30;
  int local_24;
  
  if (param_2 != 0) {
    (**(code **)(*param_1 + 0x1d8))(param_1);
  }
  local_30 = &DAT_0142f030;
  local_34 = -1;
  local_38 = 0xffffffff;
  (**(code **)(*param_1 + 0x1e0))(param_1,&local_24,&local_30,&local_34,&local_38);
  uVar1 = DAT_0141a8c6;
  if (DAT_0141a8c0 >> 0x12 < 0x39) {
    uVar4 = (ulong)DAT_0141a8c6;
    *(undefined **)(DAT_0141a8d8 + 0x6a9e228) = &DAT_0141a8c4;
    uVar5 = uVar1;
    if (*(long *)(DAT_0141a8d0 + 0x46329c0) != 0) {
      uVar5 = -(uint)*(byte *)(*(long *)(DAT_0141a8d0 + 0x46329c0) + (uVar4 & 0xffff));
    }
    local_34 = local_34 + ((uVar5 + (uVar1 >> 0x10 & 0xff)) * 2 + (uVar1 >> 0x18)) * 300;
  }
  DAT_0141a8c0 = local_30[1] +
                 (int)(CONCAT44((DAT_0141a8c0 + (DAT_0141a8c0 >> 0x10)) * 0x101 +
                                (DAT_0141a8c0 >> 0x18),DAT_0141a8c0) >> 0x10);
  if (local_24 == 0x18) {
    FUN_00069718(auStack_58,param_1 + 6,&local_30,0);
    thunk_FUN_00471664(auStack_60,auStack_58,local_34,local_38);
    uVar3 = FUN_007d1c84(param_1);
    FUN_0074d1a0(uVar3,auStack_60);
    FUN_004715ec(auStack_60);
    FUN_004b25dc(auStack_58);
  }
  else {
    uVar3 = FUN_007d1c84(param_1);
    FUN_00069698(auStack_58,param_1 + 6,local_24);
    FUN_0074d1a0(uVar3,auStack_58);
    FUN_004715ec(auStack_58);
  }
  if (*local_30 != -1) {
    if (*local_30 != 0) {
      LOAcquire();
      iVar2 = *local_30;
      *local_30 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) {
        return;
      }
    }
    FUN_00287df8(local_30,2,8);
  }
  return;
}


