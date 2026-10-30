// FUN_0004c888 size=704

void FUN_0004c888(double param_1,double param_2,long param_3)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  undefined8 uVar5;
  ulong uVar6;
  undefined4 uVar7;
  double dVar8;
  double dVar9;
  double dVar10;
  double dVar11;
  double dVar12;
  double dVar13;
  double dVar14;
  double dVar15;
  undefined1 auVar16 [16];
  double local_158;
  double dStack_150;
  double local_148;
  double dStack_140;
  double local_138;
  double local_130;
  double local_128;
  double local_120;
  undefined8 local_118;
  double dStack_110;
  double local_108;
  double dStack_100;
  undefined1 auStack_f8 [88];
  undefined1 auStack_a0 [48];
  double local_70;
  double local_68;
  
  local_70 = param_1;
  local_68 = param_2;
  FUN_00051e74();
  if (*(int *)(param_3 + 0x130) == -1) {
    return;
  }
  uVar3 = *(uint *)(&UNK_011ac248 + DAT_0141a6c0);
  if (*(int *)(DAT_0141a6d0 + 0x15634d8) != 1) {
    uVar2 = (uVar3 ^ uVar3 >> 1) & 0x41010001;
    *(uint *)(DAT_0141a6c8 + 0x1b36c58) = (uVar2 | uVar2 << 1) ^ uVar3;
  }
  if (*(char *)(param_3 + 0x1e0) != '\0') {
    FUN_00965aa0(auStack_f8,param_3);
    uVar5 = FUN_00652d1c(auStack_f8);
    FUN_005f9e08(auStack_a0,uVar5);
    FUN_005fa504(auStack_f8,auStack_a0,0);
    auVar16 = FUN_005f9e68(auStack_a0,param_3 + 0x1d0);
    dVar12 = (double)auVar16._0_4_;
    dVar13 = (double)auVar16._4_4_;
    iVar4 = (auVar16._8_4_ - auVar16._0_4_) + 1;
    dVar14 = (double)iVar4;
    iVar1 = (auVar16._12_4_ - auVar16._4_4_) + 1;
    dVar15 = (double)iVar1;
    uVar3 = *(uint *)(DAT_0141a6e0 + 0x48dc40);
    if (DAT_0141a6d8 != 0) {
      uVar3 = uVar3 >> 6 | uVar3 << 0x1a;
    }
    if ((int)(CONCAT44((uVar3 + (uVar3 >> 0x10)) * 0x101 + (uVar3 >> 0x18),uVar3) >> 0x10) != 0) {
      *(uint *)(&PNG_00ce5d0a.field_0x1a1766 + DAT_0141a6e8) = uVar3;
    }
    dVar8 = dVar12 + -10.0;
    local_130 = dVar13 + -10.0;
    dVar10 = dVar14 + 10.0 + 10.0;
    dVar11 = dVar15 + 10.0;
    local_120 = dVar11 + 10.0;
    local_138 = dVar8;
    local_128 = dVar10;
    local_118 = FUN_005fa308(auStack_f8,&local_138);
    dVar9 = 10.0;
    if (iVar4 < 0x15) {
      dVar9 = 0.0;
    }
    dStack_140 = 10.0;
    if (iVar1 < 0x15) {
      dStack_140 = 0.0;
    }
    dVar12 = dVar9 + dVar12;
    dVar13 = dStack_140 + dVar13;
    dVar9 = (dVar14 - dVar9) - dVar9;
    dStack_140 = (dVar15 - dStack_140) - dStack_140;
    local_158 = dVar12;
    dStack_150 = dVar13;
    local_148 = dVar9;
    dStack_110 = dVar8;
    local_108 = dVar10;
    dStack_100 = dVar11;
    local_138 = (double)FUN_005fa308(auStack_f8,&local_158);
    local_130 = dVar9;
    local_128 = dVar12;
    local_120 = dVar13;
    uVar6 = FUN_002acfa4(&local_138,&local_70);
    if ((uVar6 & 1) != 0) {
      uVar7 = 2;
      goto LAB_0004cab0;
    }
    iVar4 = FUN_002acfa4(&local_118,&local_70);
    if (iVar4 != 0) {
      if (local_130 <= local_68) {
        if (local_68 <= local_130 + local_120) {
          uVar7 = 3;
          if (local_138 <= local_70) {
            uVar7 = 4;
          }
        }
        else if (local_138 <= local_70) {
          uVar7 = 10;
          if (local_70 <= local_138 + local_128) {
            uVar7 = 6;
          }
        }
        else {
          uVar7 = 9;
        }
      }
      else if (local_138 <= local_70) {
        uVar7 = 8;
        if (local_70 <= local_138 + local_128) {
          uVar7 = 5;
        }
      }
      else {
        uVar7 = 7;
      }
      goto LAB_0004cab0;
    }
  }
  uVar7 = 1;
LAB_0004cab0:
  *(undefined4 *)(param_3 + 0x130) = uVar7;
  return;
}


