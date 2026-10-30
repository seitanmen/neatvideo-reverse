// FUN_0004c29c size=1236

void FUN_0004c29c(long param_1,undefined8 param_2)

{
  uint uVar1;
  bool bVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  ulong uVar6;
  uint uVar7;
  long lVar8;
  uint uVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  int iVar15;
  long lVar16;
  int iVar17;
  
  uVar1 = DAT_0141a686;
  uVar9 = *(uint *)(param_1 + 0x130);
  iVar3 = (int)param_2;
  iVar4 = (int)((ulong)param_2 >> 0x20);
  if (uVar9 == 2) {
    iVar14 = *(int *)(param_1 + 0x1a4);
    if ((*(long *)(param_1 + 0x148) == 0) ||
       (lVar8 = *(long *)(*(long *)(param_1 + 0x148) + 8), lVar8 == 0)) {
      uVar5 = 0;
    }
    else {
      uVar5 = *(undefined4 *)(lVar8 + 0xd0);
    }
    iVar3 = FUN_00052338(param_1,0,uVar5,iVar3 - *(int *)(param_1 + 0x1a0),
                         (*(int *)(param_1 + 0x1d8) - *(int *)(param_1 + 0x1d0)) + 1);
    if ((*(long *)(param_1 + 0x148) == 0) ||
       (lVar8 = *(long *)(*(long *)(param_1 + 0x148) + 8), lVar8 == 0)) {
      uVar5 = 0;
    }
    else {
      uVar5 = *(undefined4 *)(lVar8 + 0xd8);
    }
    iVar4 = FUN_00052338(param_1,0,uVar5,iVar4 - iVar14,
                         (*(int *)(param_1 + 0x1dc) - *(int *)(param_1 + 0x1d4)) + 1);
    *(ulong *)(param_1 + 0x1d8) =
         CONCAT44((iVar4 - (int)((ulong)*(undefined8 *)(param_1 + 0x1d0) >> 0x20)) +
                  (int)((ulong)*(undefined8 *)(param_1 + 0x1d8) >> 0x20),
                  (iVar3 - (int)*(undefined8 *)(param_1 + 0x1d0)) +
                  (int)*(undefined8 *)(param_1 + 0x1d8));
    *(int *)(param_1 + 0x1d0) = iVar3;
    *(int *)(param_1 + 0x1d4) = iVar4;
    return;
  }
  lVar8 = *(long *)(param_1 + 0x1c8);
  if (lVar8 == 0) {
    iVar13 = 0;
    iVar14 = 0;
  }
  else {
    iVar14 = *(int *)(*(long *)(param_1 + 0x1b8) + 0x10);
    iVar13 = *(int *)(*(long *)(param_1 + 0x1b8) + 0x14);
  }
  iVar17 = *(int *)(param_1 + 0x1d0);
  iVar12 = *(int *)(param_1 + 0x1d4);
  iVar10 = *(int *)(param_1 + 0x1d8);
  iVar11 = *(int *)(param_1 + 0x1dc);
  if (DAT_0141a680 >> 0x12 < 0x3d) {
    uVar6 = (ulong)DAT_0141a686;
    *(undefined **)(DAT_0141a698 + 0x710b0a8) = &DAT_0141a684;
    uVar7 = uVar1;
    if (*(long *)(DAT_0141a690 + 0x7eb1860) != 0) {
      uVar7 = -(uint)*(byte *)(*(long *)(DAT_0141a690 + 0x7eb1860) + (uVar6 & 0xffff));
    }
    iVar14 = iVar14 + ((uVar7 + (uVar1 >> 0x10 & 0xff)) * 2 + (uVar1 >> 0x18)) * -1000;
  }
  DAT_0141a680 = (int)(CONCAT44((DAT_0141a680 + (DAT_0141a680 >> 0x10)) * 0x101 +
                                (DAT_0141a680 >> 0x18),DAT_0141a680) >> 0x10) + iVar17;
  if (10 < uVar9) {
LAB_0004c4a0:
    if (iVar10 < iVar17) goto LAB_0004c4a8;
LAB_0004c52c:
    bVar2 = false;
    iVar14 = iVar17;
    iVar17 = iVar10;
switchD_0004c4d0_caseD_5:
    iVar10 = iVar14;
joined_r0x0004c768:
    if (10 < uVar9) goto LAB_0004c654;
    uVar1 = 1 << (ulong)(uVar9 & 0x1f);
    if ((uVar1 & 0x1a0) != 0) goto LAB_0004c590;
    if ((uVar1 & 0x640) == 0) goto LAB_0004c654;
    goto LAB_0004c600;
  }
  uVar1 = 1 << (ulong)(uVar9 & 0x1f);
  if ((uVar1 & 0x288) == 0) {
    if ((uVar1 & 0x510) != 0) {
      iVar10 = 0;
      if (-1 < iVar17 - iVar14 && lVar8 != 0) {
        iVar10 = (iVar17 - iVar14) + 1;
      }
      iVar15 = 0;
      if (*(long *)(param_1 + 0x148) != 0) {
        lVar16 = *(long *)(*(long *)(param_1 + 0x148) + 8);
        iVar15 = 0;
        if (lVar16 != 0) {
          iVar15 = *(int *)(lVar16 + 0xd0);
        }
      }
      if (iVar14 + iVar17 <= iVar15 && lVar8 != 0) {
        iVar15 = iVar14 + iVar17;
      }
      iVar14 = iVar3;
      if (iVar15 <= iVar3) {
        iVar14 = iVar15 + -1;
      }
      if (iVar10 <= iVar3) {
        iVar10 = iVar14;
      }
    }
    goto LAB_0004c4a0;
  }
  iVar17 = 0;
  if (-1 < iVar10 - iVar14 && lVar8 != 0) {
    iVar17 = (iVar10 - iVar14) + 1;
  }
  iVar15 = 0;
  if (*(long *)(param_1 + 0x148) != 0) {
    lVar16 = *(long *)(*(long *)(param_1 + 0x148) + 8);
    iVar15 = 0;
    if (lVar16 != 0) {
      iVar15 = *(int *)(lVar16 + 0xd0);
    }
  }
  if (iVar14 + iVar10 <= iVar15 && lVar8 != 0) {
    iVar15 = iVar14 + iVar10;
  }
  iVar14 = iVar3;
  if (iVar15 <= iVar3) {
    iVar14 = iVar15 + -1;
  }
  if (iVar17 <= iVar3) {
    iVar17 = iVar14;
  }
  if (iVar17 <= iVar10) goto LAB_0004c52c;
LAB_0004c4a8:
  bVar2 = true;
  iVar14 = iVar10;
  switch(uVar9) {
  case 3:
    uVar9 = 4;
    break;
  case 4:
    uVar9 = 3;
    break;
  case 5:
  case 6:
    goto switchD_0004c4d0_caseD_5;
  case 7:
    uVar9 = 8;
    goto LAB_0004c584;
  case 8:
    uVar9 = 7;
LAB_0004c584:
    *(uint *)(param_1 + 0x130) = uVar9;
    bVar2 = true;
LAB_0004c590:
    iVar12 = 0;
    if (-1 < iVar11 - iVar13 && lVar8 != 0) {
      iVar12 = (iVar11 - iVar13) + 1;
    }
    if ((*(long *)(param_1 + 0x148) == 0) ||
       (lVar16 = *(long *)(*(long *)(param_1 + 0x148) + 8), lVar16 == 0)) {
      iVar14 = -1;
    }
    else {
      iVar14 = *(int *)(lVar16 + 0xd8) + -1;
    }
    iVar13 = iVar13 + iVar11 + -1;
    if (iVar13 <= iVar14 && lVar8 != 0) {
      iVar14 = iVar13;
    }
    if (iVar4 <= iVar14) {
      iVar14 = iVar4;
    }
    if (iVar12 <= iVar4) {
      iVar12 = iVar14;
    }
    goto LAB_0004c654;
  case 9:
    uVar9 = 10;
    goto LAB_0004c5f4;
  case 10:
    uVar9 = 9;
LAB_0004c5f4:
    *(uint *)(param_1 + 0x130) = uVar9;
    bVar2 = true;
LAB_0004c600:
    iVar11 = 0;
    if (-1 < iVar12 - iVar13 && lVar8 != 0) {
      iVar11 = (iVar12 - iVar13) + 1;
    }
    if ((*(long *)(param_1 + 0x148) == 0) ||
       (lVar16 = *(long *)(*(long *)(param_1 + 0x148) + 8), lVar16 == 0)) {
      iVar14 = -1;
    }
    else {
      iVar14 = *(int *)(lVar16 + 0xd8) + -1;
    }
    iVar13 = iVar13 + iVar12 + -1;
    if (iVar13 <= iVar14 && lVar8 != 0) {
      iVar14 = iVar13;
    }
    if (iVar4 <= iVar14) {
      iVar14 = iVar4;
    }
    if (iVar11 <= iVar4) {
      iVar11 = iVar14;
    }
    goto LAB_0004c654;
  default:
    goto joined_r0x0004c768;
  }
  *(uint *)(param_1 + 0x130) = uVar9;
  bVar2 = true;
LAB_0004c654:
  uVar1 = DAT_0141a6a6;
  if (DAT_0141a6a0 >> 0x10 < 0xef) {
    uVar6 = (ulong)DAT_0141a6a6;
    *(undefined **)(DAT_0141a6b8 + 0x7c164a8) = &DAT_0141a6a4;
    uVar7 = uVar1;
    if (*(long *)(DAT_0141a6b0 + 0x7b9bb60) != 0) {
      uVar7 = -(uint)*(byte *)(*(long *)(DAT_0141a6b0 + 0x7b9bb60) + (uVar6 & 0xffff));
    }
    iVar12 = iVar12 - ((uVar7 + (uVar1 >> 0x10 & 0xff)) * 2 + (uVar1 >> 0x18)) * iVar11;
  }
  DAT_0141a6a0 = (int)(CONCAT44(((DAT_0141a6a0 >> 0x10) + DAT_0141a6a0) * 0x101 +
                                (DAT_0141a6a0 >> 0x18),DAT_0141a6a0) >> 0x10) + iVar11;
  if (iVar11 < iVar12) {
    if (uVar9 - 5 < 6) {
      *(undefined4 *)(param_1 + 0x130) = *(undefined4 *)(&UNK_00c93fe4 + (long)(int)(uVar9 - 5) * 4)
      ;
    }
    *(int *)(param_1 + 0x1d0) = iVar10;
    *(int *)(param_1 + 0x1d4) = iVar11;
    *(int *)(param_1 + 0x1d8) = iVar17;
    *(int *)(param_1 + 0x1dc) = iVar12;
  }
  else {
    *(int *)(param_1 + 0x1d0) = iVar10;
    *(int *)(param_1 + 0x1d4) = iVar12;
    *(int *)(param_1 + 0x1d8) = iVar17;
    *(int *)(param_1 + 0x1dc) = iVar11;
    if (!bVar2) {
      return;
    }
  }
  FUN_00051f04((double)iVar3,(double)iVar4,param_1,0);
  return;
}


