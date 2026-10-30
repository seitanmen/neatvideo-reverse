// FUN_000d1120 size=1504

void FUN_000d1120(long param_1,int param_2)

{
  uint uVar1;
  uint uVar2;
  int *piVar3;
  long *plVar4;
  ulong uVar5;
  undefined8 uVar6;
  int iVar7;
  int iVar8;
  undefined4 local_48;
  undefined4 local_44;
  int *local_40;
  undefined1 local_38 [16];
  long local_28;
  
  local_28 = *(long *)PTR____stack_chk_guard_013b4320;
  plVar4 = *(long **)(param_1 + 0x1f0);
  if (param_2 == 0) {
    uVar5 = (**(code **)(*plVar4 + 0x220))();
    if ((uVar5 & 1) == 0) {
      FUN_00157638(*(undefined8 *)(param_1 + 0x1f8));
      *(undefined8 *)(param_1 + 0x230) = 0xffffffffffffffff;
      *(undefined8 *)(param_1 + 0x228) = 0xffffffffffffffff;
      *(undefined8 *)(param_1 + 0x220) = 0xffffffffffffffff;
      FUN_001d86f4(param_1);
      if (*(long *)PTR____stack_chk_guard_013b4320 == local_28) {
                    /* WARNING: Could not recover jumptable at 0x000d1340. Too many branches */
                    /* WARNING: Treating indirect jump as call */
        (**(code **)(**(long **)(param_1 + 0x1d0) + 0x228))();
        return;
      }
      goto LAB_000d16fc;
    }
    uVar1 = *(uint *)(DAT_0141bc78 + 0x5fde98);
    if (DAT_0141bc70 != 0) {
      uVar1 = uVar1 >> 3 | uVar1 << 0x1d;
    }
    if ((int)(CONCAT44((uVar1 + (uVar1 >> 0x10)) * 0x101 + (uVar1 >> 0x18),uVar1) >> 0x10) != 1) {
      *(uint *)(DAT_0141bc80 + 0x46ce28) = uVar1;
    }
    local_38 = (**(code **)(**(long **)(param_1 + 0x1f0) + 0x280))();
    FUN_001576c0(*(undefined8 *)(param_1 + 0x1f8),local_38,
                 *(undefined8 *)(*(long *)(param_1 + 0x1d0) + 0x108));
    iVar8 = *(int *)(BYTE_ARRAY_ARRAY_015b2719[0xbc7] + DAT_0141bc90 + 0xf);
    *(undefined8 *)(param_1 + 0x230) = 0xffffffffffffffff;
    *(undefined8 *)(param_1 + 0x228) = 0xffffffffffffffff;
    *(undefined8 *)(param_1 + 0x220) = 0xffffffffffffffff;
    FUN_009c4a4c(*(undefined8 *)(*(long *)(param_1 + 0x1f8) + 0x10),&local_40,&local_44,&local_48,1)
    ;
    uVar6 = FUN_009b62b4((ulong)local_40 & 0xffffffff);
    *(undefined8 *)(param_1 + 0x220) = uVar6;
    uVar6 = FUN_009b62b4(local_44);
    *(undefined8 *)(param_1 + 0x228) = uVar6;
    uVar6 = FUN_009b62b4(local_48);
    *(undefined8 *)(param_1 + 0x230) = uVar6;
    FUN_001d86f4(param_1);
    if (iVar8 == 0) {
      FUN_000cec18(param_1);
    }
    else {
      uVar2 = *(uint *)(DAT_0141bc98 + 0x18a5448);
      uVar1 = (uVar2 ^ uVar2 >> 1) & 0x41544401;
      uVar2 = (uVar1 | uVar1 << 1) ^ uVar2;
      if (uVar2 == 1) {
        FUN_000cec18(param_1);
      }
      else {
        *(uint *)(&UNK_000f6e58 + DAT_0141bca0) = uVar2;
        FUN_000cec18(param_1);
        if (uVar2 == 0) goto LAB_000d16c4;
      }
      uVar1 = *(uint *)(DAT_0141bcb0 + 0xbc1288);
      if (DAT_0141bca8 != 0) {
        uVar1 = uVar1 >> 0xc | uVar1 << 0x14;
      }
      if ((int)(CONCAT44((uVar1 + (uVar1 >> 0x10)) * 0x101 + (uVar1 >> 0x18),uVar1) >> 0x10) != 1) {
        *(uint *)(DAT_0141bcb8 + 0xb0de58) = uVar1;
      }
    }
  }
  else {
    if (*(char *)((long)plVar4 + 0x1e1) == '\0') {
      FUN_0004b83c();
    }
    else {
      iVar7 = ((int)plVar4[0x3b] - (int)plVar4[0x3a]) + 1;
      iVar8 = ((int)((ulong)plVar4[0x3b] >> 0x20) - (int)((ulong)plVar4[0x3a] >> 0x20)) + 1;
      if (iVar7 <= iVar8) {
        iVar8 = iVar7;
      }
      local_40 = &DAT_0142f030;
      if (*(int *)(*(long *)(param_1 + 0x1d0) + 0xe0) == 0) {
        if (iVar8 < 0x10) {
          FUN_002efbd0(local_38,"",0);
        }
        else if (iVar8 < 0x20) {
          FUN_003bbff0(local_38,&DAT_013c4d88,"Size: Too small to profile",0,0xffffffff);
        }
        else if (iVar8 < 0x40) {
          FUN_003bbff0(local_38,&DAT_013c4d88,"Size: Small",0,0xffffffff);
        }
        else if (iVar8 < 0x80) {
          FUN_003bbff0(local_38,&DAT_013c4d88,"Size: Small",0,0xffffffff);
        }
        else if (iVar8 < 0xc0) {
          FUN_003bbff0(local_38,&DAT_013c4d88,"Size: Good",0,0xffffffff);
        }
        else if (iVar8 < 0x140) {
          FUN_003bbff0(local_38,&DAT_013c4d88,"Size: Good",0,0xffffffff);
        }
        else if (iVar8 < 0x200) {
          FUN_003bbff0(local_38,&DAT_013c4d88,"Size: Good",0,0xffffffff);
        }
        else {
          FUN_003bbff0(local_38,&DAT_013c4d88,"Size: Best",0,0xffffffff);
        }
      }
      else if (iVar8 < 0x10) {
        FUN_002efbd0(local_38,"",0);
      }
      else if (iVar8 < 0x20) {
        FUN_003bbff0(local_38,&DAT_013c4d88,"Size: Fine-tuning only",0,0xffffffff);
      }
      else if (iVar8 < 0x40) {
        FUN_003bbff0(local_38,&DAT_013c4d88,"Size: Small (High Freq)",0,0xffffffff);
      }
      else if (iVar8 < 0x80) {
        FUN_003bbff0(local_38,&DAT_013c4d88,"Size: Small (Mid Freq)",0,0xffffffff);
      }
      else if (iVar8 < 0xc0) {
        FUN_003bbff0(local_38,&DAT_013c4d88,"Size: Good (Low Freq)",0,0xffffffff);
      }
      else if (iVar8 < 0x140) {
        FUN_003bbff0(local_38,&DAT_013c4d88,"Size: Good (Very Low Freq)",0,0xffffffff);
      }
      else if (iVar8 < 0x200) {
        FUN_003bbff0(local_38,&DAT_013c4d88,"Size: Good (Ultra Low Freq)",0,0xffffffff);
      }
      else {
        FUN_003bbff0(local_38,&DAT_013c4d88,"Size: Best (Ultra Low Freq)",0,0xffffffff);
      }
      piVar3 = local_40;
      local_40 = (int *)local_38._0_8_;
      local_38._0_8_ = piVar3;
      if (*piVar3 != -1) {
        if (*piVar3 != 0) {
          LOAcquire();
          iVar8 = *piVar3;
          *piVar3 = iVar8 + -1;
          LORelease();
          if (iVar8 != 1) goto LAB_000d167c;
        }
        FUN_00287df8(piVar3,2,8);
      }
LAB_000d167c:
      FUN_0004b810(*(undefined8 *)(param_1 + 0x1f0),&local_40);
      if (*local_40 != -1) {
        if (*local_40 != 0) {
          LOAcquire();
          iVar8 = *local_40;
          *local_40 = iVar8 + -1;
          LORelease();
          if (iVar8 != 1) goto LAB_000d16bc;
        }
        FUN_00287df8(local_40,2,8);
      }
    }
LAB_000d16bc:
    FUN_0004b7cc(*(undefined8 *)(param_1 + 0x1f0));
  }
LAB_000d16c4:
  (**(code **)(**(long **)(param_1 + 0x1d0) + 0x228))();
  if (*(long *)PTR____stack_chk_guard_013b4320 == local_28) {
    return;
  }
LAB_000d16fc:
                    /* WARNING: Subroutine does not return */
  ___stack_chk_fail();
}


