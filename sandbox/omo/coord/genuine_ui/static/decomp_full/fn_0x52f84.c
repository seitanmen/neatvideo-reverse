// FUN_00052f84 size=960

undefined8 FUN_00052f84(long param_1,long *param_2,int param_3)

{
  ulong uVar1;
  int iVar2;
  uint uVar3;
  code *pcVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  ulong uVar8;
  undefined8 uVar9;
  int iVar10;
  long lVar11;
  long *plVar12;
  long lVar13;
  uint uVar14;
  long lVar15;
  int *piVar16;
  int iVar17;
  undefined1 auVar18 [16];
  int *local_60;
  undefined8 uStack_58;
  
  if (param_3 == 1) {
    iVar5 = (**(code **)(*param_2 + 0x220))(param_2);
    if (iVar5 != 0) {
      auVar18 = (**(code **)(*param_2 + 0x280))(param_2);
      uVar8 = (long)auVar18._8_4_ + (long)auVar18._0_4_;
      if ((long)uVar8 < 0) {
        uVar8 = uVar8 + 1;
      }
      uVar8 = uVar8 >> 1;
      uVar1 = (auVar18._8_8_ >> 0x20) + (auVar18._0_8_ >> 0x20);
      if ((long)uVar1 < 0) {
        uVar1 = uVar1 + 1;
      }
      iVar5 = (int)(uVar1 >> 1);
      goto LAB_000532c8;
    }
  }
  else if (param_3 == 2) {
    uVar8 = FUN_0095e7cc(param_2,param_2 + 0xe);
    iVar5 = (int)(uVar8 >> 0x20);
    goto LAB_000532c8;
  }
  lVar11 = param_2[5];
  iVar5 = *(int *)(lVar11 + 0x1c);
  iVar6 = *(int *)(lVar11 + 0x20);
  iVar17 = *(int *)(lVar11 + 0x14);
  iVar7 = *(int *)(lVar11 + 0x18);
  piVar16 = *(int **)(param_1 + 0x160);
  if (*piVar16 == 0) {
    if (piVar16[2] < 0) {
      piVar16 = (int *)FUN_00287b7c(0x10,8,piVar16[2] & 0x7fffffff,0);
      local_60 = piVar16;
      if (piVar16 == (int *)0x0) {
        FUN_00c78400();
      }
      piVar16[2] = piVar16[2] | 0x80000000;
    }
    else {
      piVar16 = (int *)FUN_00287b7c(0x10,8,(long)piVar16[1],0);
      local_60 = piVar16;
      if (piVar16 == (int *)0x0) {
        FUN_00c78400();
                    /* WARNING: Does not return */
        pcVar4 = (code *)SoftwareBreakpoint(1,0x53344);
        (*pcVar4)();
      }
    }
    if ((piVar16[2] & 0x7fffffffU) != 0) {
      lVar11 = *(long *)(param_1 + 0x160);
      _memcpy((void *)((long)piVar16 + *(long *)(piVar16 + 4)),
              (void *)(lVar11 + *(long *)(lVar11 + 0x10)),(long)*(int *)(lVar11 + 4) << 4);
      piVar16[1] = *(int *)(*(long *)(param_1 + 0x160) + 4);
    }
  }
  else {
    local_60 = piVar16;
    if (*piVar16 != -1) {
      LOAcquire();
      *piVar16 = *piVar16 + 1;
      LORelease();
      piVar16 = *(int **)(param_1 + 0x160);
      local_60 = piVar16;
    }
  }
  iVar5 = (iVar5 - iVar17) + 1;
  iVar17 = (iVar6 - iVar7) + 1;
  lVar11 = (long)piVar16[1] << 4;
  if (lVar11 != 0) {
    plVar12 = (long *)((long)piVar16 + *(long *)(piVar16 + 4));
    lVar13 = param_2[5];
    do {
      lVar15 = *(long *)(*plVar12 + 0x28);
      iVar6 = (*(int *)(lVar15 + 0x1c) - *(int *)(lVar15 + 0x14)) + 1;
      iVar7 = iVar6 + ~*(uint *)(lVar13 + 0x1c) + *(int *)(lVar13 + 0x14);
      iVar2 = -iVar7;
      if (-1 < iVar7) {
        iVar2 = iVar7;
      }
      if (iVar2 < 2) {
        iVar7 = (*(int *)(lVar15 + 0x20) - *(int *)(lVar15 + 0x18)) + 1;
        iVar2 = iVar7 + ~*(uint *)(lVar13 + 0x20) + *(int *)(lVar13 + 0x18);
        iVar10 = -iVar2;
        if (-1 < iVar2) {
          iVar10 = iVar2;
        }
        if (iVar10 < 2) {
          if (iVar6 <= iVar5) {
            iVar5 = iVar6;
          }
          if (iVar7 <= iVar17) {
            iVar17 = iVar7;
          }
        }
      }
      plVar12 = plVar12 + 2;
      lVar11 = lVar11 + -0x10;
    } while (lVar11 != 0);
  }
  if (*piVar16 != -1) {
    if (*piVar16 != 0) {
      LOAcquire();
      iVar6 = *piVar16;
      *piVar16 = iVar6 + -1;
      LORelease();
      piVar16 = local_60;
      if (iVar6 != 1) goto LAB_000531d4;
    }
    FUN_00287df8(piVar16,0x10,8);
  }
LAB_000531d4:
  uVar3 = DAT_0141a9b6;
  if (DAT_0141a9b0 >> 0x12 < 0x75) {
    uVar8 = (ulong)DAT_0141a9b6;
    *(undefined **)(DAT_0141a9c8 + 0x40995a8) = &DAT_0141a9b4;
    uVar14 = uVar3;
    if (*(long *)(DAT_0141a9c0 + 0x23aeca0) != 0) {
      uVar14 = -(uint)*(byte *)(*(long *)(DAT_0141a9c0 + 0x23aeca0) + (uVar8 & 0xffff));
    }
    iVar5 = iVar5 + ((uVar14 + (uVar3 >> 0x10 & 0xff)) * 2 + (uVar3 >> 0x18)) * -0x408;
  }
  DAT_0141a9b0 = (int)(CONCAT44((DAT_0141a9b0 + (DAT_0141a9b0 >> 0x10)) * 0x101 +
                                (DAT_0141a9b0 >> 0x18),DAT_0141a9b0) >> 0x10) + iVar17;
  lVar11 = FUN_007d1c84(param_2);
  lVar11 = *(long *)(lVar11 + 0x28);
  lVar13 = param_2[5];
  iVar6 = ((*(int *)(lVar11 + 0x1c) + iVar5) - *(int *)(lVar11 + 0x14)) + ~*(uint *)(lVar13 + 0x1c)
          + *(int *)(lVar13 + 0x14);
  iVar5 = ((*(int *)(lVar11 + 0x20) + iVar17) - *(int *)(lVar11 + 0x18)) + ~*(uint *)(lVar13 + 0x20)
          + *(int *)(lVar13 + 0x18);
  if (iVar6 < 0) {
    iVar6 = iVar6 + 1;
  }
  uVar8 = (ulong)(uint)(iVar6 >> 1);
  if (iVar5 < 0) {
    iVar5 = iVar5 + 1;
  }
  iVar5 = iVar5 >> 1;
LAB_000532c8:
  FUN_007d1ea8(param_2);
  iVar6 = FUN_007d81e8();
  FUN_007d1df4(param_2);
  iVar7 = FUN_007d81e8();
  local_60 = (int *)0x0;
  uStack_58 = 0;
  uVar9 = FUN_0095e7cc(param_2,&local_60);
  iVar17 = 0;
  if (0 < (int)uVar9) {
    iVar17 = -(int)uVar9;
  }
  iVar10 = (int)((ulong)uVar9 >> 0x20);
  iVar2 = 0;
  if (0 < iVar10) {
    iVar2 = -iVar10;
  }
  return CONCAT44(iVar7 + iVar5 + iVar2,iVar6 + (int)uVar8 + iVar17);
}


