// FUN_000d1120 size=1576

void FUN_000d1120(long param_1,int param_2)

{
  uint uVar1;
  uint uVar2;
  int *piVar3;
  code *pcVar4;
  long *plVar5;
  ulong uVar6;
  undefined8 uVar7;
  int iVar8;
  int iVar9;
  undefined4 uStack_48;
  undefined4 uStack_44;
  int *piStack_40;
  undefined1 auStack_38 [16];
  long lStack_28;
  
  lStack_28 = *(long *)PTR____stack_chk_guard_013b4320;
  plVar5 = *(long **)(param_1 + 0x1f0);
  if (param_2 == 0) {
    uVar6 = (**(code **)(*plVar5 + 0x220))();
    if ((uVar6 & 1) != 0) {
      uVar1 = *(uint *)(&UNK_005fde98 + lRam000000000141bc78);
      if (iRam000000000141bc70 != 0) {
        uVar1 = uVar1 >> 3 | uVar1 << 0x1d;
      }
      if ((int)(CONCAT44((uVar1 + (uVar1 >> 0x10)) * 0x101 + (uVar1 >> 0x18),uVar1) >> 0x10) != 1) {
        *(uint *)(lRam000000000141bc80 + 0x46ce28) = uVar1;
      }
      auStack_38 = (**(code **)(**(long **)(param_1 + 0x1f0) + 0x280))();
      func_0x001576c0(*(undefined8 *)(param_1 + 0x1f8),auStack_38,
                      *(undefined8 *)(*(long *)(param_1 + 0x1d0) + 0x108));
      iVar9 = *(int *)(BYTE_ARRAY_ARRAY_015b2719[0xbc7] + lRam000000000141bc90 + 0xf);
      *(undefined8 *)(param_1 + 0x230) = 0xffffffffffffffff;
      *(undefined8 *)(param_1 + 0x228) = 0xffffffffffffffff;
      *(undefined8 *)(param_1 + 0x220) = 0xffffffffffffffff;
      func_0x009c4a4c(*(undefined8 *)(*(long *)(param_1 + 0x1f8) + 0x10),&piStack_40,&uStack_44,
                      &uStack_48,1);
      uVar7 = func_0x009b62b4((ulong)piStack_40 & 0xffffffff);
      *(undefined8 *)(param_1 + 0x220) = uVar7;
      uVar7 = func_0x009b62b4(uStack_44);
      *(undefined8 *)(param_1 + 0x228) = uVar7;
      uVar7 = func_0x009b62b4(uStack_48);
      *(undefined8 *)(param_1 + 0x230) = uVar7;
      func_0x001d86f4(param_1);
      if (iVar9 == 0) {
        FUN_000cec18(param_1);
      }
      else {
        uVar2 = *(uint *)(lRam000000000141bc98 + 0x18a5448);
        uVar1 = (uVar2 ^ uVar2 >> 1) & 0x41544401;
        uVar2 = (uVar1 | uVar1 << 1) ^ uVar2;
        if (uVar2 == 1) {
          FUN_000cec18(param_1);
        }
        else {
          *(uint *)(&UNK_000f6e58 + lRam000000000141bca0) = uVar2;
          FUN_000cec18(param_1);
          if (uVar2 == 0) goto LAB_000d16c4;
        }
        uVar1 = *(uint *)(&UNK_00bc1288 + lRam000000000141bcb0);
        if (iRam000000000141bca8 != 0) {
          uVar1 = uVar1 >> 0xc | uVar1 << 0x14;
        }
        if ((int)(CONCAT44((uVar1 + (uVar1 >> 0x10)) * 0x101 + (uVar1 >> 0x18),uVar1) >> 0x10) != 1)
        {
          *(uint *)(&UNK_00b0de58 + lRam000000000141bcb8) = uVar1;
        }
      }
      goto LAB_000d16c4;
    }
    func_0x00157638(*(undefined8 *)(param_1 + 0x1f8));
    *(undefined8 *)(param_1 + 0x230) = 0xffffffffffffffff;
    *(undefined8 *)(param_1 + 0x228) = 0xffffffffffffffff;
    *(undefined8 *)(param_1 + 0x220) = 0xffffffffffffffff;
    func_0x001d86f4(param_1);
    if (*(long *)PTR____stack_chk_guard_013b4320 == lStack_28) {
                    /* WARNING: Could not recover jumptable at 0x000d1340. Too many branches */
                    /* WARNING: Treating indirect jump as call */
      (**(code **)(**(long **)(param_1 + 0x1d0) + 0x228))();
      return;
    }
  }
  else {
    if (*(char *)((long)plVar5 + 0x1e1) == '\0') {
      func_0x0004b83c();
    }
    else {
      iVar8 = ((int)plVar5[0x3b] - (int)plVar5[0x3a]) + 1;
      iVar9 = ((int)((ulong)plVar5[0x3b] >> 0x20) - (int)((ulong)plVar5[0x3a] >> 0x20)) + 1;
      if (iVar8 <= iVar9) {
        iVar9 = iVar8;
      }
      piStack_40 = (int *)0x142f030;
      if (*(int *)(*(long *)(param_1 + 0x1d0) + 0xe0) == 0) {
        if (iVar9 < 0x10) {
          func_0x002efbd0(auStack_38,"",0);
        }
        else if (iVar9 < 0x20) {
          func_0x003bbff0(auStack_38,&UNK_013c4d88,"Size: Too small to profile",0,0xffffffff);
        }
        else if (iVar9 < 0x40) {
          func_0x003bbff0(auStack_38,&UNK_013c4d88,"Size: Small",0,0xffffffff);
        }
        else if (iVar9 < 0x80) {
          func_0x003bbff0(auStack_38,&UNK_013c4d88,"Size: Small",0,0xffffffff);
        }
        else if (iVar9 < 0xc0) {
          func_0x003bbff0(auStack_38,&UNK_013c4d88,"Size: Good",0,0xffffffff);
        }
        else if (iVar9 < 0x140) {
          func_0x003bbff0(auStack_38,&UNK_013c4d88,"Size: Good",0,0xffffffff);
        }
        else if (iVar9 < 0x200) {
          func_0x003bbff0(auStack_38,&UNK_013c4d88,"Size: Good",0,0xffffffff);
        }
        else {
          func_0x003bbff0(auStack_38,&UNK_013c4d88,"Size: Best",0,0xffffffff);
        }
      }
      else if (iVar9 < 0x10) {
        func_0x002efbd0(auStack_38,"",0);
      }
      else if (iVar9 < 0x20) {
        func_0x003bbff0(auStack_38,&UNK_013c4d88,"Size: Fine-tuning only",0,0xffffffff);
      }
      else if (iVar9 < 0x40) {
        func_0x003bbff0(auStack_38,&UNK_013c4d88,"Size: Small (High Freq)",0,0xffffffff);
      }
      else if (iVar9 < 0x80) {
        func_0x003bbff0(auStack_38,&UNK_013c4d88,"Size: Small (Mid Freq)",0,0xffffffff);
      }
      else if (iVar9 < 0xc0) {
        func_0x003bbff0(auStack_38,&UNK_013c4d88,"Size: Good (Low Freq)",0,0xffffffff);
      }
      else if (iVar9 < 0x140) {
        func_0x003bbff0(auStack_38,&UNK_013c4d88,"Size: Good (Very Low Freq)",0,0xffffffff);
      }
      else if (iVar9 < 0x200) {
        func_0x003bbff0(auStack_38,&UNK_013c4d88,"Size: Good (Ultra Low Freq)",0,0xffffffff);
      }
      else {
        func_0x003bbff0(auStack_38,&UNK_013c4d88,"Size: Best (Ultra Low Freq)",0,0xffffffff);
      }
      piVar3 = piStack_40;
      piStack_40 = (int *)auStack_38._0_8_;
      auStack_38._0_8_ = piVar3;
      if (*piVar3 != -1) {
        if (*piVar3 != 0) {
          LOAcquire();
          iVar9 = *piVar3;
          *piVar3 = iVar9 + -1;
          LORelease();
          if (iVar9 != 1) goto LAB_000d167c;
        }
        func_0x00287df8(piVar3,2,8);
      }
LAB_000d167c:
      func_0x0004b810(*(undefined8 *)(param_1 + 0x1f0),&piStack_40);
      if (*piStack_40 != -1) {
        if (*piStack_40 != 0) {
          LOAcquire();
          iVar9 = *piStack_40;
          *piStack_40 = iVar9 + -1;
          LORelease();
          if (iVar9 != 1) goto LAB_000d16bc;
        }
        func_0x00287df8(piStack_40,2,8);
      }
    }
LAB_000d16bc:
    func_0x0004b7cc(*(undefined8 *)(param_1 + 0x1f0));
LAB_000d16c4:
    (**(code **)(**(long **)(param_1 + 0x1d0) + 0x228))();
    if (*(long *)PTR____stack_chk_guard_013b4320 == lStack_28) {
      return;
    }
  }
  uVar7 = ___stack_chk_fail();
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar9 = *piStack_40;
      *piStack_40 = iVar9 + -1;
      LORelease();
      if (iVar9 != 1) goto LAB_000d1770;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000d1770:
  __Unwind_Resume(uVar7);
                    /* WARNING: Does not return */
  pcVar4 = (code *)SoftwareBreakpoint(1,0xd177c);
  (*pcVar4)();
}


