// FUN_0004d500 size=916

void FUN_0004d500(undefined1 param_1 [16],double param_2,double param_3,double param_4,long *param_5
                 )

{
  uint uVar1;
  uint uVar2;
  code *pcVar3;
  bool bVar4;
  int iVar5;
  ulong uVar6;
  long lVar7;
  long *plVar8;
  int *piVar9;
  long lVar10;
  double dVar11;
  double dVar12;
  double dVar13;
  double dVar14;
  float fVar15;
  float fVar16;
  undefined8 local_d8;
  double dStack_d0;
  double local_c8;
  double dStack_c0;
  undefined8 local_b8;
  double dStack_b0;
  double local_a8;
  double dStack_a0;
  int *local_98;
  double local_90;
  double local_88;
  double dStack_80;
  double local_78;
  double local_70;
  double local_68;
  double dStack_60;
  double local_58;
  
  uVar1 = *(uint *)(DAT_0141a748 + 0x561a38);
  bVar4 = DAT_0141a740 != 0;
  DAT_0141a740 = uVar1;
  if (bVar4) {
    DAT_0141a740 = uVar1 >> 9 | uVar1 << 0x17;
  }
  if (DAT_0141a740 != 1) {
    *(uint *)(&PNG_00ce5d0a.field_0x7237e + DAT_0141a750) = DAT_0141a740;
  }
  plVar8 = param_5;
  if (((char)param_5[0x23] == '\0') && (lVar7 = param_5[0x2c], *(int *)(lVar7 + 4) != 0)) {
    plVar8 = *(long **)(lVar7 + *(long *)(lVar7 + 0x10));
  }
  local_70 = (double)FUN_0005102c(plVar8,8);
  local_68 = param_2;
  dStack_60 = param_3;
  local_58 = param_4;
  local_90 = (double)FUN_0005102c(plVar8,0);
  local_88 = param_2;
  dStack_80 = param_3;
  local_78 = param_4;
  if ((int)param_5[0x25] == 0) goto LAB_0004d800;
  piVar9 = (int *)param_5[0x2c];
  if (*piVar9 == 0) {
    if (piVar9[2] < 0) {
      piVar9 = (int *)FUN_00287b7c(0x10,8,piVar9[2] & 0x7fffffff,0);
      local_98 = piVar9;
      if (piVar9 == (int *)0x0) {
        FUN_00c78400();
      }
      piVar9[2] = piVar9[2] | 0x80000000;
    }
    else {
      piVar9 = (int *)FUN_00287b7c(0x10,8,(long)piVar9[1],0);
      local_98 = piVar9;
      if (piVar9 == (int *)0x0) {
        FUN_00c78400();
                    /* WARNING: Does not return */
        pcVar3 = (code *)SoftwareBreakpoint(1,0x4d894);
        (*pcVar3)();
      }
    }
    if ((piVar9[2] & 0x7fffffffU) != 0) {
      lVar7 = param_5[0x2c];
      _memcpy((void *)((long)piVar9 + *(long *)(piVar9 + 4)),
              (void *)(lVar7 + *(long *)(lVar7 + 0x10)),(long)*(int *)(lVar7 + 4) << 4);
      piVar9[1] = *(int *)(param_5[0x2c] + 4);
    }
  }
  else {
    local_98 = piVar9;
    if (*piVar9 != -1) {
      LOAcquire();
      *piVar9 = *piVar9 + 1;
      LORelease();
      piVar9 = (int *)param_5[0x2c];
      local_98 = piVar9;
    }
  }
  lVar7 = (long)piVar9[1] << 4;
  if (lVar7 == 0) {
    fVar16 = 0.0;
    fVar15 = 0.0;
  }
  else {
    plVar8 = (long *)((long)piVar9 + *(long *)(piVar9 + 4));
    fVar15 = 0.0;
    fVar16 = 0.0;
    do {
      lVar10 = *plVar8;
      dVar11 = param_2;
      if (*(char *)(lVar10 + 0x118) == '\0') {
        local_b8 = FUN_0005102c(lVar10,0);
        if ((double)fVar15 <= param_3) {
          fVar15 = (float)param_3;
        }
        dVar11 = (double)(ulong)(uint)(float)param_4;
        if ((double)fVar16 <= param_4) {
          fVar16 = (float)param_4;
        }
        dStack_b0 = param_2;
        local_a8 = param_3;
        dStack_a0 = param_4;
        if (((char)param_5[0x3c] != '\0') &&
           (uVar6 = (**(code **)(*param_5 + 0x2a8))(param_5), (uVar6 & 1) == 0)) {
          local_d8 = FUN_0005102c(lVar10,8);
          dStack_d0 = dVar11;
          local_c8 = param_3;
          dStack_c0 = param_4;
          local_70 = (double)FUN_002ad0c0(&local_70,&local_d8);
          local_68 = dVar11;
          dStack_60 = param_3;
          local_58 = param_4;
          local_90 = (double)FUN_002ad0c0(&local_90,&local_b8);
          local_88 = dVar11;
          dStack_80 = param_3;
          local_78 = param_4;
        }
      }
      plVar8 = plVar8 + 2;
      lVar7 = lVar7 + -0x10;
      param_2 = dVar11;
    } while (lVar7 != 0);
  }
  if (*piVar9 != -1) {
    if (*piVar9 != 0) {
      LOAcquire();
      iVar5 = *piVar9;
      *piVar9 = iVar5 + -1;
      LORelease();
      piVar9 = local_98;
      if (iVar5 != 1) goto LAB_0004d784;
    }
    FUN_00287df8(piVar9,0x10,8);
  }
LAB_0004d784:
  dVar12 = (double)(fVar15 * -0.5);
  dVar11 = (double)(fVar16 * -0.5);
  local_70 = local_70 + dVar12;
  dVar14 = (double)(fVar15 * 0.5) - dVar12;
  dVar13 = (double)(fVar16 * 0.5) - dVar11;
  local_68 = local_68 + dVar11;
  dStack_60 = dStack_60 + dVar14;
  local_58 = dVar13 + local_58;
  local_90 = local_90 + dVar12;
  local_88 = local_88 + dVar11;
  dStack_80 = dStack_80 + dVar14;
  local_78 = dVar13 + local_78;
LAB_0004d800:
  FUN_0004d394(param_5,&local_70);
  uVar2 = *(uint *)(DAT_0141a760 + 0x16e5a70);
  uVar1 = (uVar2 ^ uVar2 >> 1) & 0x44055040;
  uVar2 = (uVar1 | uVar1 << 1) ^ uVar2;
  DAT_0141a758 = (int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10);
  if (DAT_0141a758 != 0) {
    *(uint *)(DAT_0141a768 + 0x1801920) = uVar2;
  }
  return;
}


