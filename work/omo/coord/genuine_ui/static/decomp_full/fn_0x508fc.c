// FUN_000508fc size=1768

void FUN_000508fc(long *param_1,long param_2,undefined8 param_3)

{
  int iVar1;
  uint uVar2;
  code *pcVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  undefined8 uVar11;
  undefined8 uVar12;
  undefined8 uVar13;
  undefined8 uVar14;
  long lVar15;
  long lVar16;
  long *plVar17;
  long lVar18;
  int iVar19;
  long lVar20;
  int *local_68;
  
  if (param_2 == 0) {
    return;
  }
  lVar15 = *(long *)(param_2 + 0x148);
  if ((lVar15 == 0) || (*(long *)(lVar15 + 8) == 0)) {
    iVar8 = 0;
    lVar16 = param_1[0x29];
    if (lVar16 == 0) goto LAB_00050948;
LAB_00050964:
    iVar19 = 0;
    if (*(long *)(lVar16 + 8) != 0) {
      iVar19 = *(int *)(*(long *)(lVar16 + 8) + 0xd0);
    }
    if (iVar8 != iVar19) {
      return;
    }
  }
  else {
    iVar8 = *(int *)(*(long *)(lVar15 + 8) + 0xd0);
    lVar16 = param_1[0x29];
    if (lVar16 != 0) goto LAB_00050964;
LAB_00050948:
    if (iVar8 != 0) {
      return;
    }
  }
  iVar8 = 0;
  if (lVar15 != 0) {
    iVar8 = 0;
    if (*(long *)(lVar15 + 8) != 0) {
      iVar8 = *(int *)(*(long *)(lVar15 + 8) + 0xd8);
    }
  }
  iVar19 = 0;
  if (lVar16 != 0) {
    iVar19 = 0;
    if (*(long *)(lVar16 + 8) != 0) {
      iVar19 = *(int *)(*(long *)(lVar16 + 8) + 0xd8);
    }
  }
  if (iVar8 != iVar19) {
    return;
  }
  if (*(double *)(&DAT_00c94070 + (long)*(int *)(param_2 + 0x134) * 8) !=
      *(double *)(&DAT_00c94070 + (long)*(int *)((long)param_1 + 0x134) * 8)) {
    return;
  }
  uVar11 = FUN_007d1ea8(param_1);
  uVar12 = FUN_007d1df4(param_1);
  uVar13 = FUN_007d1ea8(param_2);
  uVar14 = FUN_007d1df4(param_2);
  uVar7 = *(uint *)(DAT_0141a9e0 + 0xb94178);
  if (DAT_0141a9d8 != 0) {
    uVar7 = uVar7 >> 5 | uVar7 << 0x1b;
  }
  if (uVar7 != 1) {
    *(uint *)(DAT_0141a9e8 + 0x370dc8) = uVar7;
  }
  if ((int)param_3 == 0) {
    uVar9 = FUN_007d81e8(uVar13);
    uVar7 = FUN_007d7cf8(uVar11);
    if ((int)uVar9 <= (int)uVar7) {
      uVar7 = uVar9;
    }
    uVar7 = uVar7 & ((int)uVar7 >> 0x1f ^ 0xffffffffU);
    uVar10 = FUN_007d81e8(uVar14);
    uVar9 = FUN_007d7cf8(uVar12);
    if ((int)uVar10 <= (int)uVar9) {
      uVar9 = uVar10;
    }
    uVar9 = uVar9 & ((int)uVar9 >> 0x1f ^ 0xffffffffU);
    goto LAB_00050f58;
  }
  uVar13 = (**(code **)(*param_1 + 0x248))(param_1,param_2,param_3);
  lVar15 = param_1[5];
  iVar8 = *(int *)(lVar15 + 0x1c);
  iVar4 = *(int *)(lVar15 + 0x20);
  iVar19 = *(int *)(lVar15 + 0x14);
  iVar5 = *(int *)(lVar15 + 0x18);
  local_68 = (int *)param_1[0x2c];
  if (*local_68 == 0) {
    if (local_68[2] < 0) {
      local_68 = (int *)FUN_00287b7c(0x10,8,local_68[2] & 0x7fffffff,0);
      if (local_68 == (int *)0x0) {
        FUN_00c78400();
      }
      local_68[2] = local_68[2] | 0x80000000;
    }
    else {
      local_68 = (int *)FUN_00287b7c(0x10,8,(long)local_68[1],0);
      if (local_68 == (int *)0x0) goto LAB_00050fdc;
    }
    if ((local_68[2] & 0x7fffffffU) != 0) {
      lVar15 = param_1[0x2c];
      _memcpy((void *)((long)local_68 + *(long *)(local_68 + 4)),
              (void *)(lVar15 + *(long *)(lVar15 + 0x10)),(long)*(int *)(lVar15 + 4) << 4);
      local_68[1] = *(int *)(param_1[0x2c] + 4);
    }
  }
  else if (*local_68 != -1) {
    LOAcquire();
    *local_68 = *local_68 + 1;
    LORelease();
    local_68 = (int *)param_1[0x2c];
  }
  iVar8 = (iVar8 - iVar19) + 1;
  iVar19 = (iVar4 - iVar5) + 1;
  lVar15 = (long)local_68[1] << 4;
  if (lVar15 != 0) {
    plVar17 = (long *)((long)local_68 + *(long *)(local_68 + 4));
    lVar16 = param_1[5];
    do {
      lVar18 = *(long *)(*plVar17 + 0x28);
      iVar4 = (*(int *)(lVar18 + 0x1c) - *(int *)(lVar18 + 0x14)) + 1;
      iVar5 = iVar4 + ~*(uint *)(lVar16 + 0x1c) + *(int *)(lVar16 + 0x14);
      iVar6 = -iVar5;
      if (-1 < iVar5) {
        iVar6 = iVar5;
      }
      if (iVar6 < 2) {
        iVar5 = (*(int *)(lVar18 + 0x20) - *(int *)(lVar18 + 0x18)) + 1;
        iVar6 = iVar5 + ~*(uint *)(lVar16 + 0x20) + *(int *)(lVar16 + 0x18);
        iVar1 = -iVar6;
        if (-1 < iVar6) {
          iVar1 = iVar6;
        }
        if (iVar1 < 2) {
          if (iVar4 <= iVar8) {
            iVar8 = iVar4;
          }
          if (iVar5 <= iVar19) {
            iVar19 = iVar5;
          }
        }
      }
      plVar17 = plVar17 + 2;
      lVar15 = lVar15 + -0x10;
    } while (lVar15 != 0);
  }
  if (*local_68 != -1) {
    if (*local_68 != 0) {
      LOAcquire();
      iVar4 = *local_68;
      *local_68 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_00050c5c;
    }
    FUN_00287df8(local_68,0x10,8);
  }
LAB_00050c5c:
  lVar15 = FUN_007d1c84(param_1);
  lVar15 = *(long *)(lVar15 + 0x28);
  lVar16 = param_1[5];
  iVar8 = ((*(int *)(lVar15 + 0x1c) + iVar8) - *(int *)(lVar15 + 0x14)) + ~*(uint *)(lVar16 + 0x1c)
          + *(int *)(lVar16 + 0x14);
  iVar19 = ((*(int *)(lVar15 + 0x20) + iVar19) - *(int *)(lVar15 + 0x18)) +
           ~*(uint *)(lVar16 + 0x20) + *(int *)(lVar16 + 0x18);
  iVar4 = FUN_007d81e8(uVar11);
  iVar5 = FUN_007d81e8(uVar12);
  iVar6 = FUN_007d81e8(uVar11);
  if (iVar8 < 0) {
    iVar8 = iVar8 + 1;
  }
  uVar9 = (((int)uVar13 - (iVar8 >> 1)) - iVar4) + iVar6;
  uVar7 = FUN_007d7cf8(uVar11);
  if ((int)uVar9 <= (int)uVar7) {
    uVar7 = uVar9;
  }
  iVar8 = FUN_007d81e8(uVar12);
  if (iVar19 < 0) {
    iVar19 = iVar19 + 1;
  }
  uVar10 = (((int)((ulong)uVar13 >> 0x20) - iVar5) - (iVar19 >> 1)) + iVar8;
  uVar9 = FUN_007d7cf8(uVar12);
  if ((int)uVar10 <= (int)uVar9) {
    uVar9 = uVar10;
  }
  local_68 = (int *)param_1[0x2c];
  if (*local_68 == 0) {
    if (local_68[2] < 0) {
      local_68 = (int *)FUN_00287b7c(0x10,8,local_68[2] & 0x7fffffff,0);
      if (local_68 == (int *)0x0) {
        FUN_00c78400();
      }
      local_68[2] = local_68[2] | 0x80000000;
    }
    else {
      local_68 = (int *)FUN_00287b7c(0x10,8,(long)local_68[1],0);
      if (local_68 == (int *)0x0) {
LAB_00050fdc:
        FUN_00c78400();
                    /* WARNING: Does not return */
        pcVar3 = (code *)SoftwareBreakpoint(1,0x50fe4);
        (*pcVar3)();
      }
    }
    if ((local_68[2] & 0x7fffffffU) != 0) {
      lVar15 = param_1[0x2c];
      _memcpy((void *)((long)local_68 + *(long *)(local_68 + 4)),
              (void *)(lVar15 + *(long *)(lVar15 + 0x10)),(long)*(int *)(lVar15 + 4) << 4);
      local_68[1] = *(int *)(param_1[0x2c] + 4);
    }
  }
  else if (*local_68 != -1) {
    LOAcquire();
    *local_68 = *local_68 + 1;
    LORelease();
    local_68 = (int *)param_1[0x2c];
  }
  uVar7 = uVar7 & ((int)uVar7 >> 0x1f ^ 0xffffffffU);
  uVar9 = uVar9 & ((int)uVar9 >> 0x1f ^ 0xffffffffU);
  lVar15 = (long)local_68[1] << 4;
  if (lVar15 != 0) {
    plVar17 = (long *)((long)local_68 + *(long *)(local_68 + 4));
    do {
      lVar20 = *plVar17;
      lVar16 = *(long *)(lVar20 + 0x28);
      lVar18 = param_1[5];
      uVar10 = (*(int *)(lVar16 + 0x1c) - *(int *)(lVar16 + 0x14)) + ~*(uint *)(lVar18 + 0x1c) +
               *(int *)(lVar18 + 0x14);
      uVar2 = ~uVar10;
      if (-1 < (int)(uVar10 + 1)) {
        uVar2 = uVar10 + 1;
      }
      if ((int)uVar2 < 2) {
        uVar10 = (*(int *)(lVar16 + 0x20) - *(int *)(lVar16 + 0x18)) + ~*(uint *)(lVar18 + 0x20) +
                 *(int *)(lVar18 + 0x18);
        uVar2 = ~uVar10;
        if (-1 < (int)(uVar10 + 1)) {
          uVar2 = uVar10 + 1;
        }
        if ((int)uVar2 < 2) {
          lVar16 = FUN_007d1c84(param_1);
          iVar8 = *(int *)(*(long *)(lVar16 + 0x28) + 0x1c);
          iVar19 = *(int *)(*(long *)(lVar16 + 0x28) + 0x14);
          iVar4 = FUN_007d7cf8(uVar11);
          lVar16 = FUN_007d1c84(lVar20);
          iVar8 = (iVar8 - iVar19) + iVar4 + ~*(uint *)(*(long *)(lVar16 + 0x28) + 0x1c) +
                  *(int *)(*(long *)(lVar16 + 0x28) + 0x14);
          if (iVar8 + 1 <= (int)uVar7) {
            uVar7 = iVar8 + 1;
          }
          lVar16 = FUN_007d1c84(param_1);
          iVar8 = *(int *)(*(long *)(lVar16 + 0x28) + 0x20);
          iVar19 = *(int *)(*(long *)(lVar16 + 0x28) + 0x18);
          iVar4 = FUN_007d7cf8(uVar12);
          lVar16 = FUN_007d1c84(lVar20);
          iVar8 = (iVar8 - iVar19) + iVar4 + ~*(uint *)(*(long *)(lVar16 + 0x28) + 0x20) +
                  *(int *)(*(long *)(lVar16 + 0x28) + 0x18);
          if (iVar8 + 1 <= (int)uVar9) {
            uVar9 = iVar8 + 1;
          }
        }
      }
      plVar17 = plVar17 + 2;
      lVar15 = lVar15 + -0x10;
    } while (lVar15 != 0);
  }
  if (*local_68 != -1) {
    if (*local_68 != 0) {
      LOAcquire();
      iVar8 = *local_68;
      *local_68 = iVar8 + -1;
      LORelease();
      if (iVar8 != 1) goto LAB_00050f58;
    }
    FUN_00287df8(local_68,0x10,8);
  }
LAB_00050f58:
  uVar10 = FUN_007d81e8(uVar11);
  if (uVar7 != uVar10) {
    FUN_007d7958(uVar11,uVar7);
  }
  uVar7 = FUN_007d81e8(uVar12);
  if (uVar9 == uVar7) {
    return;
  }
  FUN_007d7958(uVar12,uVar9);
  return;
}


