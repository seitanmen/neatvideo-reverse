// FUN_0004cb48 size=772

void FUN_0004cb48(undefined1 param_1 [16],undefined1 param_2 [16],double param_3,double param_4,
                 long param_5)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  long *plVar7;
  long lVar8;
  double dVar9;
  double dVar10;
  double dVar11;
  double dVar12;
  double dVar13;
  double dVar14;
  double dVar15;
  double dVar16;
  double local_68;
  double dStack_60;
  double local_58;
  double dStack_50;
  double local_48;
  
  FUN_00052150();
  plVar7 = (long *)(*(long *)(param_5 + 0x60) + 0x10);
  iVar6 = (**(code **)(*plVar7 + 0x20))(plVar7,0xc);
  dVar9 = (double)FUN_00051afc(param_5);
  FUN_0092e76c(&local_68,*(undefined8 *)(param_5 + 0x208));
  dVar10 = (double)FUN_00639cf8(&local_68);
  dVar11 = 1.52587890625e-05;
  dVar9 = (double)(long)((double)iVar6 * 1.52587890625e-05) / dVar9;
  FUN_00638d94(&local_68);
  if (dVar9 != dVar10) {
    FUN_0092e76c(&local_68,*(undefined8 *)(param_5 + 0x208));
    FUN_00639d6c(dVar9,&local_68);
    FUN_0092e77c(*(undefined8 *)(param_5 + 0x208),&local_68);
    FUN_0092e76c(&local_48,*(undefined8 *)(param_5 + 0x210));
    dVar10 = local_68;
    local_68 = local_48;
    local_48 = dVar10;
    FUN_00638d94(&local_48);
    FUN_00639d6c(dVar9,&local_68);
    FUN_0092e77c(*(undefined8 *)(param_5 + 0x210),&local_68);
    FUN_00638d94(&local_68);
  }
  dVar9 = (double)FUN_0092f108(*(undefined8 *)(param_5 + 0x208));
  iVar6 = *(int *)(param_5 + 0x1f8);
  iVar2 = *(int *)(param_5 + 0x1fc);
  dVar13 = (double)iVar6;
  dVar14 = (double)iVar2;
  iVar4 = *(int *)(param_5 + 0x200) - iVar6;
  dVar15 = (double)iVar4;
  iVar5 = *(int *)(param_5 + 0x204) - iVar2;
  dVar16 = (double)iVar5;
  dVar10 = dVar9 - dVar13;
  if (dVar10 < 0.0) {
    dVar10 = -dVar10;
  }
  if (dVar9 < 0.0) {
    dVar9 = -dVar9;
  }
  dVar12 = dVar13;
  if (iVar6 < 0) {
    dVar12 = -dVar13;
  }
  if (dVar12 <= dVar9) {
    dVar9 = dVar12;
  }
  if (dVar10 * DAT_011cb700 <= dVar9) {
    dVar9 = dVar11 - dVar14;
    if (dVar9 < 0.0) {
      dVar9 = -dVar9;
    }
    if (dVar11 < 0.0) {
      dVar11 = -dVar11;
    }
    dVar10 = dVar14;
    if (iVar2 < 0) {
      dVar10 = -dVar14;
    }
    if (dVar10 <= dVar11) {
      dVar11 = dVar10;
    }
    if (dVar9 * DAT_011cb700 <= dVar11) {
      dVar11 = param_3 - dVar15;
      if (dVar11 < 0.0) {
        dVar11 = -dVar11;
      }
      if (param_3 < 0.0) {
        param_3 = -param_3;
      }
      dVar9 = dVar15;
      if (iVar4 < 0) {
        dVar9 = -dVar15;
      }
      if (dVar9 <= param_3) {
        param_3 = dVar9;
      }
      if (dVar11 * DAT_011cb700 <= param_3) {
        dVar11 = param_4 - dVar16;
        if (dVar11 < 0.0) {
          dVar11 = -dVar11;
        }
        if (param_4 < 0.0) {
          param_4 = -param_4;
        }
        dVar9 = dVar16;
        if (iVar5 < 0) {
          dVar9 = -dVar16;
        }
        if (dVar9 <= param_4) {
          param_4 = dVar9;
        }
        if (dVar11 * DAT_011cb700 <= param_4) goto LAB_0004cd84;
      }
    }
  }
  local_68 = dVar13;
  dStack_60 = dVar14;
  local_58 = dVar15;
  dStack_50 = dVar16;
  FUN_0092ed08(*(undefined8 *)(param_5 + 0x208),&local_68);
  local_68 = dVar13;
  dStack_60 = dVar14;
  local_58 = dVar15;
  dStack_50 = dVar16;
  FUN_0092ed08(*(undefined8 *)(param_5 + 0x210),&local_68);
LAB_0004cd84:
  if (*(long *)(param_5 + 0x148) != 0) {
    uVar3 = *(uint *)(&UNK_0015e010 + DAT_0141a6f0);
    uVar1 = (uVar3 ^ uVar3 >> 1) & 0x41401114;
    uVar3 = (uVar1 | uVar1 << 1) ^ uVar3;
    if ((int)(CONCAT44((uVar3 + (uVar3 >> 0x10)) * 0x101 + (uVar3 >> 0x18),uVar3) >> 0x10) != 0) {
      *(uint *)(&JPEG_00fab426.field_0x115a + DAT_0141a6f8) = uVar3;
    }
    lVar8 = FUN_007d1c84(param_5);
    iVar2 = *(int *)(*(long *)(lVar8 + 0x28) + 0x1c) - *(int *)(*(long *)(lVar8 + 0x28) + 0x14);
    iVar6 = iVar2 + 2;
    if (-1 < iVar2 + 1) {
      iVar6 = iVar2 + 1;
    }
    local_68 = (double)(ulong)(uint)(iVar6 >> 1);
    FUN_0095d6ec(param_5,&local_68);
    FUN_000568b4(*(undefined8 *)(param_5 + 0x1e8),4);
    FUN_000567e4(*(undefined8 *)(param_5 + 0x1e8),param_5 + 0x1f0);
  }
  return;
}


