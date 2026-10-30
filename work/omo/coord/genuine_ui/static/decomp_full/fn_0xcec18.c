// FUN_000cec18 size=924

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_000cec18(long param_1)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  int extraout_var;
  undefined8 uVar4;
  long lVar5;
  ulong uVar6;
  int extraout_w1;
  int extraout_var_00;
  bool bVar7;
  float fVar8;
  undefined1 auVar9 [16];
  int *local_a0;
  undefined1 auStack_98 [4];
  undefined1 auStack_94 [4];
  undefined1 auStack_90 [4];
  undefined1 local_8c [4];
  undefined1 auStack_88 [7];
  undefined1 uStack_81;
  undefined1 auStack_80 [8];
  long local_78;
  undefined8 local_70;
  undefined8 uStack_68;
  int *local_58;
  int *local_50 [3];
  long local_38;
  
  local_38 = *(long *)PTR____stack_chk_guard_013b4320;
  iVar3 = (**(code **)(**(long **)(param_1 + 0x1f0) + 0x220))();
  if ((iVar3 == 0) || (*(char *)(*(long *)(param_1 + 0x1f8) + 0x40) == '\0')) {
LAB_000cecb4:
    FUN_0004b83c(*(undefined8 *)(param_1 + 0x1f0));
    bVar7 = false;
  }
  else {
    iVar3 = (**(code **)(**(long **)(param_1 + 0x1f0) + 0x280))();
    (**(code **)(**(long **)(param_1 + 0x1f0) + 0x280))();
    fVar8 = (float)FUN_009b0850((long)((extraout_w1 - iVar3) + 1),
                                (long)((extraout_var_00 - extraout_var) + 1));
    if (fVar8 < 1.0) goto LAB_000cecb4;
    FUN_002f351c(local_50,(double)*(float *)(*(long *)(param_1 + 0x1f8) + 0x1c),0x66,1);
    FUN_002efbd0(&local_58,"Noise Level: ",0xd);
    FUN_002e97cc(&local_58,local_50);
    if (*local_50[0] != -1) {
      if (*local_50[0] != 0) {
        LOAcquire();
        iVar3 = *local_50[0];
        *local_50[0] = iVar3 + -1;
        LORelease();
        if (iVar3 != 1) goto LAB_000cedfc;
      }
      FUN_00287df8(local_50[0],2,8);
    }
LAB_000cedfc:
    lVar5 = *(long *)(*(long *)(param_1 + 0x1d0) + 0x108);
    if (*(int *)(*(long *)(*(long *)(lVar5 + 0x350) + 0x28) + 0xc2f0) != 1) {
      uStack_68 = _UNK_010b4468;
      local_70 = _DAT_010b4460;
      FUN_001566c0(lVar5,&local_70,&local_78,auStack_80,&uStack_81);
      lVar5 = *(long *)(*(long *)(param_1 + 0x1d0) + 0x108);
      if (((*(char *)(*(long *)(*(long *)(lVar5 + 0x350) + 0x28) + 0xbf34) != '\0') &&
          ((*(long *)(lVar5 + 0x518) == 0 || (uVar6 = FUN_0015756c(), (uVar6 & 1) == 0)))) &&
         ((auVar9 = (**(code **)(**(long **)(param_1 + 0x1f0) + 0x280))(),
          *(int *)(param_1 + 0x208) != auVar9._0_4_ ||
          ((((*(int *)(param_1 + 0x210) != auVar9._8_4_ ||
             (*(int *)(param_1 + 0x20c) != auVar9._4_4_)) ||
            (*(int *)(param_1 + 0x214) != auVar9._12_4_)) ||
           (*(long *)(*(long *)(*(long *)(param_1 + 0x1d0) + 0x108) + 0xd0) != local_78)))))) {
        FUN_009bab0c(*(undefined8 *)
                      (*(long *)(*(long *)(*(long *)(param_1 + 0x1d0) + 0x108) + 0x350) + 0x28),
                     auStack_88,local_8c,local_50,auStack_90,auStack_94,auStack_98);
        FUN_002f304c(&local_a0," (%+d%%)");
        FUN_002e97cc(&local_58,&local_a0);
        if (*local_a0 != -1) {
          if (*local_a0 != 0) {
            LOAcquire();
            iVar3 = *local_a0;
            *local_a0 = iVar3 + -1;
            LORelease();
            if (iVar3 != 1) goto LAB_000cef68;
          }
          FUN_00287df8(local_a0,2,8);
        }
      }
    }
LAB_000cef68:
    FUN_0004b810(*(undefined8 *)(param_1 + 0x1f0),&local_58);
    if (*local_58 != -1) {
      if (*local_58 != 0) {
        LOAcquire();
        iVar3 = *local_58;
        *local_58 = iVar3 + -1;
        LORelease();
        if (iVar3 != 1) goto LAB_000cefa8;
      }
      FUN_00287df8(local_58,2,8);
    }
LAB_000cefa8:
    bVar7 = true;
  }
  uVar1 = *(uint *)(DAT_0141bbf0 + 0x357e30);
  uVar2 = *(uint *)(DAT_0141bc00 + 0x932568);
  if ((int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10) != 0) {
    uVar2 = (uVar1 ^ uVar1 >> 1) & 0x41011414;
    *(uint *)(&PNG_00f0ab00.field_0x5d7a0 + DAT_0141bbf8) = (uVar2 | uVar2 << 1) ^ uVar1;
  }
  if (bVar7) {
    uVar4 = *(undefined8 *)(param_1 + 0x1f0);
    if ((*(byte *)(DAT_0155b458 + 0x180) & 1) != 0) {
      FUN_0004b0c0(uVar4,*(long *)(param_1 + 0x1f8) + 0x30,*(long *)(param_1 + 0x1f8) + 0x38);
      goto LAB_000ced60;
    }
  }
  else {
    uVar4 = *(undefined8 *)(param_1 + 0x1f0);
  }
  FUN_0004b7cc(uVar4);
LAB_000ced60:
  if (*(long *)PTR____stack_chk_guard_013b4320 == local_38) {
    return;
  }
                    /* WARNING: Subroutine does not return */
  ___stack_chk_fail();
}


