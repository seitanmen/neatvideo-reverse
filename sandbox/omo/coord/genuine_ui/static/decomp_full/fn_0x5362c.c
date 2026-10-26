// FUN_0005362c size=904

void FUN_0005362c(undefined1 param_1 [16],double param_2,long *param_3,long param_4)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  ulong uVar9;
  undefined8 uVar10;
  undefined8 uVar11;
  undefined8 *puVar12;
  int iVar13;
  undefined8 *puVar14;
  double dVar15;
  long lVar16;
  undefined8 local_78;
  
  dVar15 = *(double *)(param_4 + 0x20);
  if (0.0 <= dVar15) {
    iVar6 = (int)(dVar15 + 0.5);
  }
  else {
    param_2 = (double)(int)(dVar15 + -1.0);
    iVar6 = (int)((dVar15 - param_2) + 0.5) + (int)(dVar15 + -1.0);
  }
  dVar15 = *(double *)(param_4 + 0x28);
  if (0.0 <= dVar15) {
    iVar13 = (int)(dVar15 + 0.5);
  }
  else {
    param_2 = (double)(int)(dVar15 + -1.0);
    iVar13 = (int)((dVar15 - param_2) + 0.5) + (int)(dVar15 + -1.0);
  }
  local_78 = CONCAT44(iVar13,iVar6);
  uVar9 = FUN_00961ed8(param_3,&local_78);
  puVar12 = (undefined8 *)param_3[0x33];
  uVar4 = *(uint *)(puVar12 + 4);
  if (uVar4 != 0) {
    uVar2 = *(uint *)((long)puVar12 + 0x24) ^ (uint)(uVar9 >> 0x1f) ^ (uint)uVar9;
    uVar5 = 0;
    if (uVar4 != 0) {
      uVar5 = uVar2 / uVar4;
    }
    for (puVar14 = *(undefined8 **)(puVar12[1] + (ulong)(uVar2 - uVar5 * uVar4) * 8);
        puVar14 != puVar12; puVar14 = (undefined8 *)*puVar14) {
      if ((*(uint *)(puVar14 + 1) == uVar2) && (puVar14[2] == uVar9)) {
        if (puVar14 != puVar12) goto LAB_00053784;
        break;
      }
    }
  }
  if (param_3[0x2a] == 0) {
LAB_00053784:
    FUN_00963f64(param_3,param_4);
  }
  else {
    *(ushort *)(param_4 + 0x12) = *(ushort *)(param_4 + 0x12) | 4;
    dVar15 = *(double *)(param_4 + 0x20);
    if (0.0 <= dVar15) {
      iVar6 = (int)(dVar15 + 0.5);
    }
    else {
      param_2 = (double)(int)(dVar15 + -1.0);
      iVar6 = (int)((dVar15 - param_2) + 0.5) + (int)(dVar15 + -1.0);
    }
    dVar15 = *(double *)(param_4 + 0x28);
    if (0.0 <= dVar15) {
      iVar13 = (int)(dVar15 + 0.5);
    }
    else {
      param_2 = (double)(int)(dVar15 + -1.0);
      iVar13 = (int)((dVar15 - param_2) + 0.5) + (int)(dVar15 + -1.0);
    }
    local_78 = CONCAT44(iVar13,iVar6);
    lVar16 = FUN_0095d6ec(param_3,&local_78);
    param_3[0xe] = lVar16;
    param_3[0xf] = (long)param_2;
    FUN_00052788(param_3);
    iVar6 = (**(code **)(*param_3 + 0x208))(param_3);
    if (iVar6 == 0) {
      if ((*(char *)((long)param_3 + 0x69) != '\0') && ((int)param_3[0x11] != 2)) {
        uVar10 = FUN_007d1ea8(param_3);
        uVar11 = FUN_007d1df4(param_3);
        dVar15 = *(double *)(param_4 + 0x20);
        if (0.0 <= dVar15) {
          iVar6 = (int)(dVar15 + 0.5);
        }
        else {
          iVar6 = (int)((dVar15 - (double)(int)(dVar15 + -1.0)) + 0.5) + (int)(dVar15 + -1.0);
        }
        dVar15 = *(double *)(param_4 + 0x28);
        if (0.0 <= dVar15) {
          iVar13 = (int)(dVar15 + 0.5);
        }
        else {
          iVar13 = (int)((dVar15 - (double)(int)(dVar15 + -1.0)) + 0.5) + (int)(dVar15 + -1.0);
        }
        iVar3 = *(int *)((long)param_3 + 0x84);
        iVar6 = iVar6 - (int)param_3[0x10];
        iVar7 = FUN_007d81e8(uVar10);
        iVar8 = FUN_0074d0bc(param_3);
        iVar1 = -iVar6;
        if (iVar8 == 1) {
          iVar1 = iVar6;
        }
        FUN_007d7958(uVar10,iVar1 + iVar7);
        iVar6 = FUN_007d81e8(uVar11);
        FUN_007d7958(uVar11,(iVar3 - iVar13) + iVar6);
        dVar15 = *(double *)(param_4 + 0x20);
        if (0.0 <= dVar15) {
          iVar6 = (int)(dVar15 + 0.5);
        }
        else {
          iVar6 = (int)((dVar15 - (double)(int)(dVar15 + -1.0)) + 0.5) + (int)(dVar15 + -1.0);
        }
        dVar15 = *(double *)(param_4 + 0x28);
        if (0.0 <= dVar15) {
          iVar13 = (int)(dVar15 + 0.5);
        }
        else {
          iVar13 = (int)((dVar15 - (double)(int)(dVar15 + -1.0)) + 0.5) + (int)(dVar15 + -1.0);
        }
        param_3[0x10] = CONCAT44(iVar13,iVar6);
        FUN_00050548(param_3);
      }
    }
    else {
      FUN_00051f04(lVar16,param_2,param_3,1);
      FUN_000506f8(param_3);
    }
  }
  return;
}


