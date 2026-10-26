// FUN_0004fe44 size=936

void FUN_0004fe44(long param_1)

{
  bool bVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  long *plVar9;
  ulong uVar10;
  long lVar11;
  int iVar12;
  uint uVar13;
  int iVar14;
  long lVar15;
  double dVar16;
  double dVar17;
  
  if (0 < *(int *)(param_1 + 0x9c)) {
    return;
  }
  *(int *)(param_1 + 0x9c) = *(int *)(param_1 + 0x9c) + 1;
  iVar5 = FUN_007d1e00();
  iVar6 = FUN_007d1d4c(param_1);
  if (*(long *)(param_1 + 0x150) == 0) {
    lVar15 = 0;
    uVar10 = 0;
  }
  else if (*(char *)(param_1 + 0xd4) == '\0') {
    plVar9 = (long *)FUN_007496f0(param_1);
    iVar7 = (**(code **)(*plVar9 + 0xe0))(plVar9,9,0,param_1);
    uVar2 = *(uint *)(&PNG_00f0ab00.field_0x33088 + DAT_0141a860);
    if (DAT_0141a858 != 0) {
      uVar2 = uVar2 >> 0x1f | uVar2 << 1;
    }
    DAT_0141a858 = (int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10
                        );
    if (DAT_0141a858 != 1) {
      *(uint *)(&PNG_00ce5d0a.field_0x10824e + DAT_0141a868) = uVar2;
    }
    lVar15 = FUN_007d1c84(param_1);
    iVar3 = *(int *)(*(long *)(lVar15 + 0x28) + 0x1c);
    iVar4 = *(int *)(*(long *)(lVar15 + 0x28) + 0x14);
    lVar15 = FUN_007d1c84(param_1);
    uVar2 = DAT_0141a876;
    iVar8 = 1;
    iVar14 = iVar8;
    if (iVar5 == 2) {
      iVar14 = iVar7 + 1;
    }
    iVar14 = (iVar14 + *(int *)(*(long *)(lVar15 + 0x28) + 0x20)) -
             *(int *)(*(long *)(lVar15 + 0x28) + 0x18);
    if (iVar6 == 2) {
      iVar8 = iVar7 + 1;
    }
    if (DAT_0141a870 >> 0x14 < 0x1d) {
      uVar10 = (ulong)DAT_0141a876;
      *(undefined **)(DAT_0141a888 + 0x6dfb0f8) = &DAT_0141a874;
      uVar13 = uVar2;
      if (*(long *)(DAT_0141a880 + 0x5ea9260) != 0) {
        uVar13 = -(uint)*(byte *)(*(long *)(DAT_0141a880 + 0x5ea9260) + (uVar10 & 0xffff));
      }
      iVar14 = iVar14 + ((uVar13 + (uVar2 >> 0x10 & 0xff)) * 2 + (uVar2 >> 0x18)) * 10000;
    }
    iVar12 = 0;
    if (*(long *)(param_1 + 0x148) != 0) {
      lVar15 = *(long *)(*(long *)(param_1 + 0x148) + 8);
      iVar12 = 0;
      if (lVar15 != 0) {
        iVar12 = *(int *)(lVar15 + 0xd0);
      }
    }
    iVar4 = (iVar8 + iVar3) - iVar4;
    DAT_0141a870 = iVar12 + (int)(CONCAT44((DAT_0141a870 + (DAT_0141a870 >> 0x10)) * 0x101 +
                                           (DAT_0141a870 >> 0x18),DAT_0141a870) >> 0x10);
    dVar17 = *(double *)(&DAT_00c94070 + (long)*(int *)(param_1 + 0x134) * 8);
    plVar9 = (long *)(*(long *)(param_1 + 0x60) + 0x10);
    iVar8 = (**(code **)(*plVar9 + 0x20))(plVar9,0xc);
    dVar17 = dVar17 / ((double)iVar8 * 1.52587890625e-05);
    lVar15 = *(long *)(param_1 + 0x148);
    if ((lVar15 == 0) || (lVar11 = *(long *)(lVar15 + 8), lVar11 == 0)) {
      dVar16 = 0.0;
      bVar1 = (double)iVar14 < dVar17 * 0.0;
      if ((double)iVar4 < dVar17 * 0.0) goto LAB_000500c8;
LAB_000500a0:
      if (!bVar1) goto LAB_0004fe9c;
    }
    else {
      dVar16 = (double)(long)*(int *)(lVar11 + 0xd8);
      bVar1 = (double)iVar14 < dVar17 * dVar16;
      if (dVar17 * (double)(long)*(int *)(lVar11 + 0xd0) <= (double)iVar4) goto LAB_000500a0;
LAB_000500c8:
      if (dVar17 * dVar16 <= (double)(iVar14 - iVar7)) {
        lVar15 = 0;
        uVar10 = 1;
        goto LAB_0005011c;
      }
    }
    dVar16 = 0.0;
    if ((lVar15 != 0) && (*(long *)(lVar15 + 8) != 0)) {
      dVar16 = (double)(long)*(int *)(*(long *)(lVar15 + 8) + 0xd0);
    }
    uVar10 = (ulong)((double)(iVar4 - iVar7) < dVar17 * dVar16);
    lVar15 = 1;
  }
  else {
LAB_0004fe9c:
    lVar15 = 0;
    uVar10 = 0;
  }
LAB_0005011c:
  iVar7 = *(int *)((long)&DAT_0127a7b0 + uVar10 * 4);
  if (iVar5 != iVar7) {
    FUN_007d1e0c(param_1,iVar7);
  }
  uVar13 = *(uint *)(&PNG_00ce5d0a.field_0x16ed4e + DAT_0141a898);
  uVar2 = (uVar13 ^ uVar13 >> 1) & 0x44004401;
  DAT_0141a890 = (uVar2 | uVar2 << 1) ^ uVar13;
  if (DAT_0141a890 != 1) {
    *(uint *)(&UNK_012d3fa8 + DAT_0141a8a0) = DAT_0141a890;
  }
  iVar14 = *(int *)((long)&DAT_0127a7b0 + lVar15 * 4);
  if (iVar6 == iVar14) {
    if (iVar5 == iVar7) goto LAB_000501c4;
  }
  else {
    FUN_007d1d58(param_1,iVar14);
  }
  if (*(int *)(param_1 + 0x98) == 0) {
    FUN_00050360(param_1);
  }
LAB_000501c4:
  *(int *)(param_1 + 0x9c) = *(int *)(param_1 + 0x9c) + -1;
  return;
}


