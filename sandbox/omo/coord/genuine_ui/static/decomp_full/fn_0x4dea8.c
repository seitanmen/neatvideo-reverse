// FUN_0004dea8 size=936

void FUN_0004dea8(long *param_1,long param_2)

{
  int iVar1;
  uint uVar2;
  char cVar3;
  int iVar4;
  long lVar5;
  int iVar6;
  long lVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  uint uVar11;
  uint uVar12;
  double dVar13;
  double dVar14;
  undefined8 local_38;
  
  FUN_0005362c();
  dVar13 = *(double *)(param_2 + 0x20);
  if (0.0 <= dVar13) {
    iVar4 = (int)(dVar13 + 0.5);
  }
  else {
    iVar4 = (int)((dVar13 - (double)(int)(dVar13 + -1.0)) + 0.5) + (int)(dVar13 + -1.0);
  }
  dVar13 = *(double *)(param_2 + 0x28);
  if (0.0 <= dVar13) {
    iVar6 = (int)(dVar13 + 0.5);
  }
  else {
    iVar6 = (int)((dVar13 - (double)(int)(dVar13 + -1.0)) + 0.5) + (int)(dVar13 + -1.0);
  }
  dVar14 = 0.5;
  local_38 = CONCAT44(iVar6,iVar4);
  dVar13 = (double)FUN_0095d6ec(param_1,&local_38);
  if (0.0 <= dVar13) {
    uVar11 = (uint)(dVar13 + 0.5);
    if (0.0 <= dVar14) goto LAB_0004df94;
LAB_0004dfc0:
    uVar12 = (int)((dVar14 - (double)(int)(dVar14 + -1.0)) + 0.5) + (int)(dVar14 + -1.0);
    cVar3 = *(char *)((long)param_1 + 0x1e2);
    if (cVar3 != '\0') goto LAB_0004dff4;
LAB_0004dfec:
    if (*(char *)((long)param_1 + 0x1e4) != '\0') goto LAB_0004dff4;
  }
  else {
    uVar11 = (int)((dVar13 - (double)(int)(dVar13 + -1.0)) + 0.5) + (int)(dVar13 + -1.0);
    if (dVar14 < 0.0) goto LAB_0004dfc0;
LAB_0004df94:
    uVar12 = (uint)(dVar14 + 0.5);
    cVar3 = *(char *)((long)param_1 + 0x1e2);
    if (cVar3 == '\0') goto LAB_0004dfec;
LAB_0004dff4:
    if ((*(char *)((long)param_1 + 0x1e3) == '\0') &&
       ((int)param_1[0x34] != uVar11 - (int)param_1[0x3a] ||
        *(int *)((long)param_1 + 0x1a4) != uVar12 - *(int *)((long)param_1 + 0x1d4))) {
      *(undefined1 *)((long)param_1 + 0x1e3) = 1;
      FUN_00051f04((double)(int)uVar11,(double)(int)uVar12,param_1,0);
      (**(code **)(*param_1 + 0x1e8))(param_1);
      FUN_000506f8(param_1);
      FUN_001d0ff8(param_1);
      cVar3 = *(char *)((long)param_1 + 0x1e2);
    }
    if (cVar3 != '\0') {
      FUN_0004c29c(param_1,CONCAT44(uVar12,uVar11));
      goto LAB_0004e220;
    }
  }
  if (*(char *)((long)param_1 + 0x1e4) == '\0') {
    return;
  }
  lVar5 = param_1[0x39];
  if (lVar5 == 0) {
    iVar6 = 0;
    iVar4 = 0;
  }
  else {
    iVar4 = *(int *)(param_1[0x37] + 0x10);
    iVar6 = *(int *)(param_1[0x37] + 0x14);
  }
  uVar9 = *(uint *)(DAT_0141a778 + 0x1ffdba0);
  DAT_0141a770 = *(int *)(DAT_0141a788 + 0x183e428);
  if (DAT_0141a770 != 0) {
    uVar10 = (uVar9 ^ uVar9 >> 1) & 0x44554000;
    *(uint *)(DAT_0141a780 + 0x17fac90) = (uVar10 | uVar10 << 1) ^ uVar9;
  }
  if (DAT_01569308 != 0) {
    DAT_0141a770 = DAT_0141a770 + (uint)*(byte *)(DAT_01569308 + 0x7202);
  }
  if (lVar5 == 0) {
    uVar9 = 0;
    lVar7 = param_1[0x29];
    if (lVar7 != 0) goto LAB_0004e140;
LAB_0004e16c:
    uVar10 = 0;
    if (lVar5 != 0) goto LAB_0004e174;
LAB_0004e150:
    uVar8 = 0;
    if ((int)uVar11 <= (int)uVar10) {
      uVar10 = uVar11;
    }
    if ((int)uVar9 <= (int)uVar11) {
      uVar9 = uVar10;
    }
    if (lVar7 != 0) goto LAB_0004e1a4;
    uVar10 = 0;
  }
  else {
    uVar9 = (int)param_1[0x34] - iVar4;
    uVar9 = uVar9 & ((int)uVar9 >> 0x1f ^ 0xffffffffU);
    lVar7 = param_1[0x29];
    if (lVar7 == 0) goto LAB_0004e16c;
LAB_0004e140:
    if (*(long *)(lVar7 + 8) == 0) goto LAB_0004e16c;
    uVar10 = *(uint *)(*(long *)(lVar7 + 8) + 0xd0);
    if (lVar5 == 0) goto LAB_0004e150;
LAB_0004e174:
    uVar8 = (int)param_1[0x34] + iVar4;
    if ((int)uVar8 <= (int)uVar10) {
      uVar10 = uVar8;
    }
    if ((int)uVar11 <= (int)uVar10) {
      uVar10 = uVar11;
    }
    if ((int)uVar9 <= (int)uVar11) {
      uVar9 = uVar10;
    }
    uVar8 = *(int *)((long)param_1 + 0x1a4) - iVar6;
    uVar8 = uVar8 & ((int)uVar8 >> 0x1f ^ 0xffffffffU);
    uVar10 = 0;
    if (lVar7 != 0) {
LAB_0004e1a4:
      uVar10 = 0;
      if (*(long *)(lVar7 + 8) != 0) {
        uVar10 = *(uint *)(*(long *)(lVar7 + 8) + 0xd8);
      }
    }
  }
  uVar2 = *(uint *)((long)param_1 + 0x1a4);
  if ((int)(uVar2 + iVar6) <= (int)uVar10 && lVar5 != 0) {
    uVar10 = uVar2 + iVar6;
  }
  if ((int)uVar12 <= (int)uVar10) {
    uVar10 = uVar12;
  }
  if ((int)uVar8 <= (int)uVar12) {
    uVar8 = uVar10;
  }
  uVar10 = *(uint *)(param_1 + 0x34);
  iVar4 = uVar9 - uVar10;
  if ((int)uVar10 <= (int)uVar9) {
    uVar9 = uVar10;
  }
  iVar6 = uVar8 - uVar2;
  if ((int)uVar2 <= (int)uVar8) {
    uVar8 = uVar2;
  }
  iVar1 = -iVar4;
  if (-1 < iVar4) {
    iVar1 = iVar4;
  }
  iVar4 = -iVar6;
  if (-1 < iVar6) {
    iVar4 = iVar6;
  }
  *(uint *)(param_1 + 0x3a) = uVar9;
  *(uint *)((long)param_1 + 0x1d4) = uVar8;
  *(uint *)(param_1 + 0x3b) = uVar9 + iVar1 + -1;
  *(uint *)((long)param_1 + 0x1dc) = uVar8 + iVar4 + -1;
  *(undefined1 *)(param_1 + 0x3c) = 1;
LAB_0004e220:
  FUN_001d1d68(param_1,1);
  FUN_00052788((double)(int)uVar11,(double)(int)uVar12,param_1);
  return;
}


