// FUN_000cec18 size=1052

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_000cec18(long param_1)

{
  uint uVar1;
  uint uVar2;
  code *pcVar3;
  int iVar4;
  int extraout_var;
  undefined8 uVar5;
  long lVar6;
  ulong uVar7;
  int extraout_w1;
  int extraout_var_00;
  bool bVar8;
  float fVar9;
  undefined1 auVar10 [16];
  int *piStack_a0;
  undefined1 auStack_98 [4];
  undefined1 auStack_94 [4];
  undefined1 auStack_90 [4];
  undefined1 auStack_8c [4];
  undefined1 auStack_88 [7];
  undefined1 uStack_81;
  undefined1 auStack_80 [8];
  long lStack_78;
  undefined8 uStack_70;
  undefined8 uStack_68;
  int *piStack_58;
  int *apiStack_50 [3];
  long lStack_38;
  
  lStack_38 = *(long *)PTR____stack_chk_guard_013b4320;
  iVar4 = (**(code **)(**(long **)(param_1 + 0x1f0) + 0x220))();
  if ((iVar4 == 0) || (*(char *)(*(long *)(param_1 + 0x1f8) + 0x40) == '\0')) {
LAB_000cecb4:
    func_0x0004b83c(*(undefined8 *)(param_1 + 0x1f0));
    bVar8 = false;
  }
  else {
    iVar4 = (**(code **)(**(long **)(param_1 + 0x1f0) + 0x280))();
    (**(code **)(**(long **)(param_1 + 0x1f0) + 0x280))();
    fVar9 = (float)func_0x009b0850((long)((extraout_w1 - iVar4) + 1),
                                   (long)((extraout_var_00 - extraout_var) + 1));
    if (fVar9 < 1.0) goto LAB_000cecb4;
    func_0x002f351c(apiStack_50,(double)*(float *)(*(long *)(param_1 + 0x1f8) + 0x1c),0x66,1);
    func_0x002efbd0(&piStack_58,"Noise Level: ",0xd);
    func_0x002e97cc(&piStack_58,apiStack_50);
    if (*apiStack_50[0] != -1) {
      if (*apiStack_50[0] != 0) {
        LOAcquire();
        iVar4 = *apiStack_50[0];
        *apiStack_50[0] = iVar4 + -1;
        LORelease();
        if (iVar4 != 1) goto LAB_000cedfc;
      }
      func_0x00287df8(apiStack_50[0],2,8);
    }
LAB_000cedfc:
    lVar6 = *(long *)(*(long *)(param_1 + 0x1d0) + 0x108);
    if (*(int *)(&UNK_0000c2f0 + *(long *)(*(long *)(lVar6 + 0x350) + 0x28)) != 1) {
      uStack_68 = _UNK_010b4468;
      uStack_70 = _UNK_010b4460;
      func_0x001566c0(lVar6,&uStack_70,&lStack_78,auStack_80,&uStack_81);
      lVar6 = *(long *)(*(long *)(param_1 + 0x1d0) + 0x108);
      if ((((&UNK_0000bf34)[*(long *)(*(long *)(lVar6 + 0x350) + 0x28)] != '\0') &&
          ((*(long *)(lVar6 + 0x518) == 0 || (uVar7 = func_0x0015756c(), (uVar7 & 1) == 0)))) &&
         ((auVar10 = (**(code **)(**(long **)(param_1 + 0x1f0) + 0x280))(),
          *(int *)(param_1 + 0x208) != auVar10._0_4_ ||
          ((((*(int *)(param_1 + 0x210) != auVar10._8_4_ ||
             (*(int *)(param_1 + 0x20c) != auVar10._4_4_)) ||
            (*(int *)(param_1 + 0x214) != auVar10._12_4_)) ||
           (*(long *)(*(long *)(*(long *)(param_1 + 0x1d0) + 0x108) + 0xd0) != lStack_78)))))) {
        func_0x009bab0c(*(undefined8 *)
                         (*(long *)(*(long *)(*(long *)(param_1 + 0x1d0) + 0x108) + 0x350) + 0x28),
                        auStack_88,auStack_8c,apiStack_50,auStack_90,auStack_94,auStack_98);
        func_0x002f304c(&piStack_a0," (%+d%%)");
        func_0x002e97cc(&piStack_58,&piStack_a0);
        if (*piStack_a0 != -1) {
          if (*piStack_a0 != 0) {
            LOAcquire();
            iVar4 = *piStack_a0;
            *piStack_a0 = iVar4 + -1;
            LORelease();
            if (iVar4 != 1) goto LAB_000cef68;
          }
          func_0x00287df8(piStack_a0,2,8);
        }
      }
    }
LAB_000cef68:
    func_0x0004b810(*(undefined8 *)(param_1 + 0x1f0),&piStack_58);
    if (*piStack_58 != -1) {
      if (*piStack_58 != 0) {
        LOAcquire();
        iVar4 = *piStack_58;
        *piStack_58 = iVar4 + -1;
        LORelease();
        if (iVar4 != 1) goto LAB_000cefa8;
      }
      func_0x00287df8(piStack_58,2,8);
    }
LAB_000cefa8:
    bVar8 = true;
  }
  uVar1 = *(uint *)(&UNK_00357e30 + lRam000000000141bbf0);
  uVar2 = *(uint *)(&UNK_00932568 + lRam000000000141bc00);
  if ((int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10) != 0) {
    uVar2 = (uVar1 ^ uVar1 >> 1) & 0x41011414;
    *(uint *)(&UNK_00f682a0 + lRam000000000141bbf8) = (uVar2 | uVar2 << 1) ^ uVar1;
  }
  if (bVar8) {
    uVar5 = *(undefined8 *)(param_1 + 0x1f0);
    if ((*(byte *)(lRam000000000155b458 + 0x180) & 1) == 0) goto LAB_000ced5c;
    FUN_0004b0c0(uVar5,*(long *)(param_1 + 0x1f8) + 0x30,*(long *)(param_1 + 0x1f8) + 0x38);
  }
  else {
    uVar5 = *(undefined8 *)(param_1 + 0x1f0);
LAB_000ced5c:
    func_0x0004b7cc(uVar5);
  }
  if (*(long *)PTR____stack_chk_guard_013b4320 == lStack_38) {
    return;
  }
  uVar5 = ___stack_chk_fail();
  if (*piStack_a0 != -1) {
    if (*piStack_a0 != 0) {
      LOAcquire();
      iVar4 = *piStack_a0;
      *piStack_a0 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000cf004;
    }
    func_0x00287df8(piStack_a0,2,8);
  }
LAB_000cf004:
  if (*piStack_58 != -1) {
    if (*piStack_58 != 0) {
      LOAcquire();
      iVar4 = *piStack_58;
      *piStack_58 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000cf0a4;
    }
    func_0x00287df8(piStack_58,2,8);
  }
LAB_000cf0a4:
  __Unwind_Resume(uVar5);
                    /* WARNING: Does not return */
  pcVar3 = (code *)SoftwareBreakpoint(1,0xcf0b0);
  (*pcVar3)();
}


