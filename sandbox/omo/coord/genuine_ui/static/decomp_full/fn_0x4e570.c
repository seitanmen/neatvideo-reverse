// FUN_0004e570 size=776

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0004e570(long *param_1,long param_2)

{
  undefined1 (*pauVar1) [16];
  uint uVar2;
  uint uVar3;
  char cVar4;
  undefined1 auVar5 [16];
  undefined1 auVar6 [16];
  long lVar7;
  bool bVar8;
  int iVar9;
  int iVar10;
  long lVar11;
  undefined8 uVar12;
  double dVar13;
  double dVar14;
  undefined1 auVar15 [16];
  undefined1 local_70 [16];
  undefined1 local_60 [16];
  
  FUN_00053d6c();
  if (*(char *)((long)param_1 + 0x1a9) == '\0') {
    return;
  }
  pauVar1 = (undefined1 (*) [16])(param_1 + 0x3f);
  local_60._8_8_ = param_1[0x40];
  local_60._0_8_ = *(undefined8 *)*pauVar1;
  if ((*(int *)(param_2 + 0x50) != 1) ||
     ((*(char *)((long)param_1 + 0x1e2) == '\0' && (*(char *)((long)param_1 + 0x1e4) == '\0'))))
  goto LAB_0004e74c;
  cVar4 = *(char *)((long)param_1 + 0x1e3);
  uVar3 = *(uint *)(DAT_0141a790 + 0x1c1f298);
  uVar2 = (uVar3 ^ uVar3 >> 1) & 0x44151051;
  uVar3 = (uVar2 | uVar2 << 1) ^ uVar3;
  if ((int)(CONCAT44((uVar3 + (uVar3 >> 0x10)) * 0x101 + (uVar3 >> 0x18),uVar3) >> 0x10) != 1) {
    *(uint *)(DAT_0141a798 + 0x14cc5e8) = uVar3;
  }
  dVar14 = *(double *)(param_2 + 0x20);
  dVar13 = -1.0;
  if (0.0 <= dVar14) {
    iVar9 = (int)(dVar14 + 0.5);
  }
  else {
    iVar9 = (int)((dVar14 - (double)(int)(dVar14 + -1.0)) + 0.5) + (int)(dVar14 + -1.0);
  }
  dVar14 = *(double *)(param_2 + 0x28);
  if (0.0 <= dVar14) {
    iVar10 = (int)(dVar14 + 0.5);
  }
  else {
    dVar13 = dVar14 - (double)(int)(dVar14 + -1.0);
    iVar10 = (int)(dVar13 + 0.5) + (int)(dVar14 + -1.0);
  }
  local_70._0_8_ = CONCAT44(iVar10,iVar9);
  uVar12 = FUN_0095d6ec(param_1,local_70);
  *(undefined4 *)((long)param_1 + 0x1e1) = 0;
  lVar11 = param_1[0x3b];
  lVar7 = param_1[0x3a];
  iVar9 = (**(code **)(*param_1 + 0x2b0))(param_1);
  if (((int)lVar11 - (int)lVar7) + 1 < iVar9) {
    *(undefined1 *)(param_1 + 0x3c) = 0;
LAB_0004e71c:
    lVar11 = _DAT_010b4460;
    param_1[0x3b] = _UNK_010b4468;
    param_1[0x3a] = lVar11;
  }
  else {
    iVar9 = (*(int *)((long)param_1 + 0x1dc) - *(int *)((long)param_1 + 0x1d4)) + 1;
    iVar10 = (**(code **)(*param_1 + 0x2b8))(param_1);
    *(bool *)(param_1 + 0x3c) = iVar10 <= iVar9;
    if (iVar9 < iVar10) goto LAB_0004e71c;
  }
  FUN_00051f04(uVar12,dVar13,param_1,1);
  if (cVar4 != '\0') {
    FUN_001d1d68(param_1,0);
  }
LAB_0004e74c:
  auVar6._8_8_ = local_60._8_8_;
  auVar6._0_8_ = local_60._0_8_;
  auVar5._8_8_ = local_60._8_8_;
  auVar5._0_8_ = local_60._0_8_;
  auVar15._8_8_ = local_60._8_8_;
  auVar15._0_8_ = local_60._0_8_;
  *(undefined1 *)((long)param_1 + 0x22b) = 0;
  local_60 = auVar6;
  if (*(int *)(param_2 + 0x50) == 4 || *(int *)(param_2 + 0x50) == 1) {
    local_70._8_8_ = param_1[0x40];
    local_70._0_8_ = *(undefined8 *)*pauVar1;
    lVar11 = param_1[0x29];
    local_60 = auVar15;
    auVar15 = *pauVar1;
    if (((lVar11 != 0) && (local_60 = auVar5, auVar15 = *pauVar1, *(long *)(lVar11 + 0x28) != 0)) &&
       (local_60 = auVar6, auVar15 = *pauVar1, *(char *)(*(long *)(lVar11 + 0x28) + 0xe0) != '\0'))
    {
      local_60 = FUN_002aca90(local_60,lVar11 + 0x38);
      auVar15 = FUN_002aca90(local_70,param_1[0x29] + 0x38);
    }
    local_70._0_4_ = auVar15._0_4_;
    bVar8 = local_60._0_4_ == local_70._0_4_;
    local_70 = auVar15;
    if (bVar8) {
      local_70._8_4_ = auVar15._8_4_;
      bVar8 = local_60._8_4_ == local_70._8_4_;
      if (bVar8) {
        local_70._4_4_ = auVar15._4_4_;
        bVar8 = local_60._4_4_ == local_70._4_4_;
        if (bVar8) {
          local_70._12_4_ = auVar15._12_4_;
          bVar8 = local_60._12_4_ == local_70._12_4_;
          if (bVar8) {
            if (((char)param_1[0x45] == '\0') || (*(char *)((long)param_1 + 0x229) != '\0')) {
              bVar8 = *(char *)((long)param_1 + 0x259) != '\0';
            }
            else {
              bVar8 = false;
            }
            (**(code **)(*param_1 + 0x250))(param_1,bVar8);
            lVar11 = FUN_009ec270(param_1 + 0x36);
            if (lVar11 < 0x65) {
              FUN_001d107c(param_1);
            }
          }
        }
      }
    }
    FUN_00053344(param_1);
  }
  FUN_000506f8(param_1);
  (**(code **)(*param_1 + 0x1e8))(param_1);
  *(ushort *)(param_2 + 0x12) = *(ushort *)(param_2 + 0x12) | 4;
  return;
}


