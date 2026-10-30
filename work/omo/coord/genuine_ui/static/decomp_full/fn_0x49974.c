// FUN_00049974 size=4028

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00049974(undefined1 param_1 [16],double param_2,double param_3,double param_4,long param_5)

{
  bool bVar1;
  int *piVar2;
  bool bVar3;
  bool bVar4;
  bool bVar5;
  bool bVar6;
  byte bVar7;
  byte bVar8;
  byte bVar9;
  uint uVar10;
  uint uVar11;
  uint uVar12;
  uint uVar13;
  int iVar14;
  long lVar15;
  undefined8 uVar16;
  int iVar17;
  int iVar18;
  int iVar19;
  int iVar20;
  int iVar21;
  long *plVar22;
  undefined8 uVar23;
  double dVar24;
  double dVar25;
  undefined1 auVar26 [16];
  undefined1 auVar27 [16];
  undefined1 auVar28 [16];
  undefined1 auVar29 [16];
  undefined1 auVar30 [16];
  undefined1 auVar31 [16];
  undefined1 auVar32 [16];
  double dVar33;
  double dVar34;
  double dVar35;
  double dVar36;
  undefined8 local_1e0;
  undefined8 uStack_1d8;
  double local_1d0;
  double dStack_1c8;
  undefined8 local_1a0;
  undefined8 uStack_198;
  undefined8 local_190;
  undefined8 local_188;
  double local_180;
  double local_178;
  double local_170;
  double local_168;
  double local_160;
  double local_158;
  double local_150;
  double local_148;
  undefined8 local_140;
  undefined8 uStack_138;
  double local_130;
  double local_128;
  double local_120;
  double dStack_118;
  int *local_110;
  undefined8 uStack_108;
  double local_100;
  double dStack_f8;
  undefined8 local_f0;
  undefined8 uStack_e8;
  undefined8 local_a0;
  undefined8 uStack_98;
  long local_90;
  
  local_90 = *(long *)PTR____stack_chk_guard_013b4320;
  FUN_0004cb48();
  local_130 = (double)FUN_0005102c(param_5,0);
  local_128 = param_2;
  local_120 = param_3;
  dStack_118 = param_4;
  if ((((*(char *)(param_5 + 0x69) == '\0') && (*(int *)(param_5 + 200) < 1)) &&
      (*(char *)(param_5 + 0x1e3) == '\0')) &&
     ((*(char *)(param_5 + 0x22a) == '\0' && (*(char *)(param_5 + 0x259) != '\0')))) {
    iVar14 = (int)*(undefined8 *)(param_5 + 0x1f8);
    local_100 = (double)iVar14;
    iVar17 = (int)((ulong)*(undefined8 *)(param_5 + 0x1f8) >> 0x20);
    dStack_f8 = (double)iVar17;
    auVar27._0_8_ = (long)(((int)*(undefined8 *)(param_5 + 0x200) - iVar14) + 1);
    auVar27._8_8_ = (long)(((int)((ulong)*(undefined8 *)(param_5 + 0x200) >> 0x20) - iVar17) + 1);
    auVar27 = NEON_scvtf(auVar27,8);
    uStack_e8 = auVar27._8_8_;
    local_f0 = auVar27._0_8_;
    iVar14 = FUN_002ad018(&local_100,&local_130);
    if ((iVar14 != 0) || (*(char *)(param_5 + 0x259) == '\0')) goto LAB_000499ec;
    dVar36 = (double)*(int *)(param_5 + 0x1f8);
    dVar33 = (double)*(int *)(param_5 + 0x1fc);
    dVar25 = (double)((*(int *)(param_5 + 0x200) - *(int *)(param_5 + 0x1f8)) + 1);
    dVar24 = (double)((*(int *)(param_5 + 0x204) - *(int *)(param_5 + 0x1fc)) + 1);
    uVar11 = 1;
  }
  else {
LAB_000499ec:
    uVar11 = 0;
    dVar33 = local_128;
    dVar25 = local_120;
    dVar24 = dStack_118;
    dVar36 = local_130;
  }
  auVar26._8_8_ = 0;
  auVar26._0_8_ = dVar36;
  FUN_00052168(auVar26,dVar33,dVar25,dVar24,param_5);
  if (*(long *)(param_5 + 0x148) == 0) goto LAB_0004a720;
  uVar10 = FUN_00928828(*(undefined8 *)(param_5 + 0x208));
  if ((uVar11 ^ uVar10) == 1) {
    FUN_009299d4(*(undefined8 *)(param_5 + 0x208),uVar11);
    FUN_009299d4(*(undefined8 *)(param_5 + 0x210),uVar11);
  }
  uVar10 = *(uint *)(DAT_0141a638 + 0x1e1bb68);
  uVar11 = (uVar10 ^ uVar10 >> 1) & 0x41041411;
  uVar10 = (uVar11 | uVar11 << 1) ^ uVar10;
  if (uVar10 != 1) {
    *(uint *)(DAT_0141a640 + 0x392c78) = uVar10;
  }
  DAT_0141a630 = uVar10;
  lVar15 = FUN_007d1c84(param_5);
  lVar15 = *(long *)(lVar15 + 0x28);
  uStack_98 = CONCAT44(*(int *)(lVar15 + 0x20) - *(int *)(lVar15 + 0x18),
                       *(int *)(lVar15 + 0x1c) - *(int *)(lVar15 + 0x14));
  local_a0 = 0;
  auVar27 = (**(code **)(**(long **)(param_5 + 0x150) + 0x18))();
  local_100 = auVar27._0_8_;
  dStack_f8 = dVar33;
  uVar16 = FUN_0095e7cc(param_5,&local_100);
  FUN_0092e77c(*(undefined8 *)(param_5 + 0x260),param_5 + 0x268);
  if (*(char *)(param_5 + 0x2b9) == '\0') {
LAB_00049b64:
    bVar6 = false;
  }
  else {
    if (*(char *)(param_5 + 0x1e0) == '\0') {
      iVar14 = *(int *)(param_5 + 0x2a8);
    }
    else {
      iVar17 = *(int *)(param_5 + 0x1d0);
      iVar14 = *(int *)(param_5 + 0x2a8);
      if (((iVar17 == *(int *)(param_5 + 0x2a8)) &&
          (iVar14 = iVar17, *(int *)(param_5 + 0x1d8) == *(int *)(param_5 + 0x2b0))) &&
         ((*(int *)(param_5 + 0x1d4) == *(int *)(param_5 + 0x2ac) &&
          (*(int *)(param_5 + 0x1dc) == *(int *)(param_5 + 0x2b4))))) goto LAB_00049b64;
    }
    if (((*(int *)(param_5 + 0x2b0) < iVar14) ||
        (*(int *)(param_5 + 0x2b4) < *(int *)(param_5 + 0x2ac))) ||
       (*(char *)(param_5 + 0x1e2) != '\0')) goto LAB_00049b64;
    bVar6 = *(char *)(param_5 + 0x1e4) == '\0';
  }
  FUN_009299d4(*(undefined8 *)(param_5 + 0x260),bVar6);
  FUN_00046cdc(*(undefined8 *)(param_5 + 0x260),*(undefined8 *)(param_5 + 0x2a8),
               *(undefined8 *)(param_5 + 0x2b0));
  dVar25 = _UNK_010b4468;
  dVar33 = _DAT_010b4460;
  uStack_138 = _UNK_010b4468;
  local_140 = _DAT_010b4460;
  iVar14 = (int)uVar16;
  iVar17 = (int)((ulong)uVar16 >> 0x20);
  if (bVar6 != false) {
    local_1d0 = _DAT_010b4460;
    dStack_f8 = _UNK_010b4468;
    dStack_1c8 = _UNK_010b4468;
    auVar28._0_8_ = (double)iVar14;
    auVar28._8_8_ = 0;
    local_100 = local_1d0;
    FUN_000456c4(auVar28,(double)iVar17,*(undefined8 *)(param_5 + 0x260),&local_a0,&local_100,
                 &local_140,&local_1d0);
    if ((int)local_140 <= (int)uStack_138) {
      if (local_140._4_4_ <= uStack_138._4_4_) {
        local_140 = (double)CONCAT44(local_140._4_4_ - iVar17,(int)local_140 - iVar14);
        uStack_138 = (double)CONCAT44(uStack_138._4_4_ - iVar17,(int)uStack_138 - iVar14);
      }
    }
  }
  dVar24 = (double)(long)*(int *)(param_5 + 0x200) + 1.0;
  if (local_130 + local_120 <= dVar24) {
    dVar24 = local_130 + local_120;
  }
  uVar16 = 0;
  dVar36 = local_128;
  if (local_128 <= (double)(long)*(int *)(param_5 + 0x1fc)) {
    dVar36 = (double)(long)*(int *)(param_5 + 0x1fc);
  }
  uVar23 = *(undefined8 *)(param_5 + 400);
  dVar34 = local_130;
  dVar35 = local_128;
  FUN_00965aa0(&local_100,param_5);
  auVar29._8_8_ = 0;
  auVar29._0_8_ = local_100;
  FUN_00056b04(auVar29,uVar23);
  bVar3 = true;
  auVar30._8_8_ = uVar16;
  auVar30._0_8_ = dVar24;
  FUN_000568b4(auVar30,dVar36,*(undefined8 *)(param_5 + 400),1);
  if (*(char *)(*(long *)(param_5 + 0x148) + 0x78) != '\0') {
    bVar3 = *(long *)(*(long *)(param_5 + 0x148) + 0x10) == 0;
  }
  FUN_00056734(*(undefined8 *)(param_5 + 400),bVar3,param_5 + 0x110);
  lVar15 = *(long *)(param_5 + 400);
  auVar31._0_8_ = *(ulong *)(lVar15 + 0x60);
  auVar31._8_8_ = 0;
  dVar24 = *(double *)(lVar15 + 0x68);
  auVar27 = FUN_00056318(auVar31,lVar15,*(undefined4 *)(lVar15 + 0x90),param_5 + 0x110);
  local_160 = auVar27._0_8_;
  if (((*(char *)(param_5 + 0x22a) == '\0') && (*(char *)(param_5 + 0x1e3) == '\0')) &&
     ((*(char *)(param_5 + 0x69) == '\0' && (*(int *)(param_5 + 200) < 1)))) {
    bVar3 = *(char *)(param_5 + 0x259) != '\0';
  }
  else {
    bVar3 = false;
  }
  local_158 = dVar24;
  local_150 = dVar34;
  local_148 = dVar35;
  local_180 = (double)(**(code **)(**(long **)(param_5 + 0x1e8) + 0x18))();
  if ((*(char *)(param_5 + 0x69) == '\0') && (*(int *)(param_5 + 200) < 1)) {
    bVar5 = *(char *)(param_5 + 0x1e3) == '\0';
  }
  else {
    bVar5 = false;
  }
  uStack_198 = dVar25;
  local_1a0 = dVar33;
  if (*(char *)(param_5 + 0x1e0) == '\0') {
    bVar4 = false;
  }
  else if (*(char *)(*(long *)(param_5 + 0x218) + 0x58) == '\0') {
    bVar4 = *(char *)(*(long *)(param_5 + 0x218) + 0x59) != '\0';
  }
  else {
    bVar4 = true;
  }
  local_190 = local_1a0;
  local_188 = uStack_198;
  local_178 = dVar24;
  local_170 = dVar34;
  local_168 = dVar35;
  FUN_00965aa0(&local_100,param_5);
  uVar16 = FUN_00652d1c(&local_100);
  FUN_005f9e08(&local_1d0,uVar16);
  uStack_1d8 = dVar25;
  local_1e0 = dVar33;
  if (bVar4) {
    lVar15 = *(long *)(param_5 + 0x218);
    if (*(char *)(lVar15 + 0x59) != '\0') {
      local_110 = &DAT_0142f0e0;
      if (*(char *)(param_5 + 0x259) != '\0') {
        if (0.0 <= local_160) {
          iVar19 = (int)(local_160 + 0.5);
        }
        else {
          iVar19 = (int)((local_160 - (double)(int)(local_160 + -1.0)) + 0.5) +
                   (int)(local_160 + -1.0);
        }
        if (0.0 <= local_158) {
          iVar21 = (int)(local_158 + 0.5);
        }
        else {
          dVar35 = 0.5;
          iVar21 = (int)((local_158 - (double)(int)(local_158 + -1.0)) + 0.5) +
                   (int)(local_158 + -1.0);
        }
        dVar33 = local_160 + local_150;
        if (0.0 <= dVar33) {
          iVar18 = (int)(dVar33 + 0.5);
        }
        else {
          iVar18 = (int)((dVar33 - (double)(int)(dVar33 + -1.0)) + 0.5) + (int)(dVar33 + -1.0);
        }
        dVar34 = 0.5;
        dVar33 = local_158 + local_148;
        if (0.0 <= dVar33) {
          iVar20 = (int)(dVar33 + 0.5);
        }
        else {
          iVar20 = (int)((dVar33 - (double)(int)(dVar33 + -1.0)) + 0.5) + (int)(dVar33 + -1.0);
        }
        local_100 = (double)CONCAT44(iVar21,iVar19);
        dStack_f8 = (double)CONCAT44(iVar20 + -1,iVar18 + -1);
        FUN_0004a9c8(&local_110,&local_100);
      }
      if (bVar5 != false) {
        if (0.0 <= local_180) {
          iVar19 = (int)(local_180 + 0.5);
        }
        else {
          iVar19 = (int)((local_180 - (double)(int)(local_180 + -1.0)) + 0.5) +
                   (int)(local_180 + -1.0);
        }
        if (0.0 <= local_178) {
          iVar21 = (int)(local_178 + 0.5);
        }
        else {
          dVar35 = 0.5;
          iVar21 = (int)((local_178 - (double)(int)(local_178 + -1.0)) + 0.5) +
                   (int)(local_178 + -1.0);
        }
        dVar33 = local_180 + local_170;
        if (0.0 <= dVar33) {
          iVar18 = (int)(dVar33 + 0.5);
        }
        else {
          iVar18 = (int)((dVar33 - (double)(int)(dVar33 + -1.0)) + 0.5) + (int)(dVar33 + -1.0);
        }
        dVar34 = 0.5;
        dVar33 = local_178 + local_168;
        if (0.0 <= dVar33) {
          iVar20 = (int)(dVar33 + 0.5);
        }
        else {
          iVar20 = (int)((dVar33 - (double)(int)(dVar33 + -1.0)) + 0.5) + (int)(dVar33 + -1.0);
        }
        local_100 = (double)CONCAT44(iVar21,iVar19);
        dStack_f8 = (double)CONCAT44(iVar20 + -1,iVar18 + -1);
        FUN_0004a9c8(&local_110,&local_100);
      }
      FUN_000471b8(*(undefined8 *)(param_5 + 0x218),&local_110);
      piVar2 = local_110;
      if (*local_110 != -1) {
        if (*local_110 != 0) {
          LOAcquire();
          iVar19 = *local_110;
          *local_110 = iVar19 + -1;
          LORelease();
          if (iVar19 != 1) goto LAB_0004a080;
        }
        iVar19 = local_110[3];
        if (iVar19 != local_110[2]) {
          lVar15 = (long)local_110[2] * 8 + (long)iVar19 * -8;
          plVar22 = (long *)(local_110 + (long)iVar19 * 2 + 2);
          do {
            if (*plVar22 != 0) {
              __ZdlPv();
            }
            plVar22 = plVar22 + -1;
            lVar15 = lVar15 + 8;
          } while (lVar15 != 0);
        }
        _free(piVar2);
      }
LAB_0004a080:
      lVar15 = *(long *)(param_5 + 0x218);
    }
    auVar32._0_8_ = (double)iVar14;
    auVar32._8_8_ = 0;
    dVar24 = (double)iVar17;
    FUN_000456c4(auVar32,lVar15,&local_a0,&local_1e0,&local_190,&local_1a0);
    if ((int)local_190 <= (int)local_188) {
      if (local_190._4_4_ <= local_188._4_4_) {
        local_190 = (double)CONCAT44(local_190._4_4_ - iVar17,(int)local_190 - iVar14);
        local_188 = (double)CONCAT44(local_188._4_4_ - iVar17,(int)local_188 - iVar14);
      }
    }
    if ((int)local_1a0 <= (int)uStack_198) {
      if (local_1a0._4_4_ <= uStack_198._4_4_) {
        local_1a0 = (double)CONCAT44(local_1a0._4_4_ - iVar17,(int)local_1a0 - iVar14);
        uStack_198 = (double)CONCAT44(uStack_198._4_4_ - iVar17,(int)uStack_198 - iVar14);
      }
    }
  }
  if ((int)uStack_1d8 < (int)local_1e0) {
LAB_0004a160:
    bVar1 = false;
  }
  else {
    if (uStack_1d8._4_4_ < local_1e0._4_4_) goto LAB_0004a160;
    if ((int)local_188 < (int)local_190) {
LAB_0004a150:
      if ((int)uStack_198 < (int)local_1a0) goto LAB_0004a160;
      bVar1 = local_1a0._4_4_ <= uStack_198._4_4_;
    }
    else {
      if (local_188._4_4_ < local_190._4_4_) goto LAB_0004a150;
      bVar1 = true;
    }
  }
  dVar33 = (double)FUN_005fa308(&local_1d0,&local_160);
  if (0.0 <= dVar33) {
    iVar14 = (int)(dVar33 + 0.5);
    if (dVar24 < 0.0) goto LAB_0004a1c8;
LAB_0004a1a4:
    iVar17 = (int)(dVar24 + 0.5);
  }
  else {
    iVar14 = (int)((dVar33 - (double)(int)(dVar33 + -1.0)) + 0.5) + (int)(dVar33 + -1.0);
    if (0.0 <= dVar24) goto LAB_0004a1a4;
LAB_0004a1c8:
    iVar17 = (int)((dVar24 - (double)(int)(dVar24 + -1.0)) + 0.5) + (int)(dVar24 + -1.0);
  }
  dVar33 = dVar33 + dVar34;
  if (0.0 <= dVar33) {
    iVar19 = (int)(dVar33 + 0.5);
  }
  else {
    iVar19 = (int)((dVar33 - (double)(int)(dVar33 + -1.0)) + 0.5) + (int)(dVar33 + -1.0);
  }
  dVar33 = 0.5;
  bVar4 = (bool)(bVar4 & bVar1);
  dVar24 = dVar24 + dVar35;
  if (0.0 <= dVar24) {
    iVar21 = (int)(dVar24 + 0.5);
  }
  else {
    iVar21 = (int)((dVar24 - (double)(int)(dVar24 + -1.0)) + 0.5) + (int)(dVar24 + -1.0);
  }
  dVar25 = 0.5;
  local_110 = (int *)CONCAT44(iVar17,iVar14);
  uStack_108 = CONCAT44(iVar21 + -1,iVar19 + -1);
  if (bVar4) {
    bVar7 = FUN_002acbac(&local_190,&local_110);
    bVar8 = FUN_002acbac(&local_1a0,&local_110);
    bVar7 = bVar7 | bVar8;
  }
  else {
    bVar7 = 0;
  }
  bVar7 = bVar3 & (bVar7 ^ 1);
  FUN_009299d4(*(undefined8 *)(param_5 + 400),bVar7);
  if (bVar6 != false) {
    if (bVar7 == 0) {
      uVar11 = 0;
    }
    else {
      uVar11 = FUN_002acbac(&local_140,&local_110);
    }
    if (bVar4) {
      uVar12 = FUN_002acbac(&local_140,&local_190);
      uVar13 = FUN_002acbac(&local_140,&local_1a0);
      uVar11 = uVar11 | uVar12 | uVar13;
    }
    FUN_00046d2c(*(undefined8 *)(param_5 + 0x260),uVar11 ^ 1);
  }
  if (bVar7 == 0) {
    bVar7 = 0;
  }
  else {
    bVar7 = FUN_002ad2b0(&local_160,&local_180);
  }
  if (bVar4) {
    dVar24 = (double)FUN_005fa308(&local_1d0,&local_180);
    if (0.0 <= dVar24) {
      iVar14 = (int)(dVar24 + 0.5);
      if (dVar25 < 0.0) goto LAB_0004a3a8;
LAB_0004a384:
      iVar17 = (int)(dVar25 + 0.5);
    }
    else {
      iVar14 = (int)((dVar24 - (double)(int)(dVar24 + -1.0)) + 0.5) + (int)(dVar24 + -1.0);
      if (0.0 <= dVar25) goto LAB_0004a384;
LAB_0004a3a8:
      iVar17 = (int)((dVar25 - (double)(int)(dVar25 + -1.0)) + 0.5) + (int)(dVar25 + -1.0);
    }
    dVar24 = dVar24 + dVar33;
    if (0.0 <= dVar24) {
      iVar19 = (int)(dVar24 + 0.5);
    }
    else {
      iVar19 = (int)((dVar24 - (double)(int)(dVar24 + -1.0)) + 0.5) + (int)(dVar24 + -1.0);
    }
    dVar33 = 0.5;
    dVar25 = dVar25 + dVar35;
    if (0.0 <= dVar25) {
      iVar21 = (int)(dVar25 + 0.5);
    }
    else {
      iVar21 = (int)((dVar25 - (double)(int)(dVar25 + -1.0)) + 0.5) + (int)(dVar25 + -1.0);
    }
    local_100 = (double)CONCAT44(iVar17,iVar14);
    dStack_f8 = (double)CONCAT44(iVar21 + -1,iVar19 + -1);
    bVar8 = FUN_002acbac(&local_190,&local_100);
    bVar9 = FUN_002acbac(&local_1a0,&local_100);
    bVar7 = bVar7 | bVar8 | bVar9;
  }
  FUN_009299d4(*(undefined8 *)(param_5 + 0x1e8),bVar5 & (bVar7 ^ 1));
  if ((uVar10 != 0) &&
     (uVar11 = *(uint *)(DAT_0141a650 + 0x198e578), *(int *)(DAT_0141a660 + 0xb43e98) != 1)) {
    if (DAT_0141a648 != 0) {
      uVar11 = uVar11 >> 0x14 | uVar11 << 0xc;
    }
    *(uint *)(DAT_0141a658 + 0x1c10e08) = uVar11;
  }
  if (*(char *)(param_5 + 0x118) == '\0') goto LAB_0004a720;
  if ((*(char *)(param_5 + 0x1e0) == '\0') || (*(char *)(param_5 + 0x1e3) != '\0')) {
LAB_0004a520:
    bVar6 = false;
  }
  else {
    if ((int)uStack_1d8 < (int)local_1e0) goto LAB_0004a520;
    if (uStack_1d8._4_4_ < local_1e0._4_4_) goto LAB_0004a520;
    dVar33 = 2.12199579145934e-314;
    iVar17 = ((int)*(undefined8 *)(param_5 + 0x1d8) - (int)*(undefined8 *)(param_5 + 0x1d0)) + 1;
    iVar14 = ((int)((ulong)*(undefined8 *)(param_5 + 0x1d8) >> 0x20) -
             (int)((ulong)*(undefined8 *)(param_5 + 0x1d0) >> 0x20)) + 1;
    if (iVar17 <= iVar14) {
      iVar14 = iVar17;
    }
    bVar6 = DAT_01488128 <= iVar14;
  }
  FUN_00965aa0(&local_100,param_5);
  dVar25 = local_100;
  if (bVar6) {
    FUN_0092e76c(&local_100,*(undefined8 *)(param_5 + 0x218));
    iVar14 = FUN_00639cb0(&local_100);
    dVar24 = (double)(iVar14 + 2) / dVar25;
    FUN_00638d94(&local_100);
    lVar15 = *(long *)(param_5 + 0x218);
    dVar35 = dVar24 + dVar24 + (double)((*(int *)(lVar15 + 0x54) - *(int *)(lVar15 + 0x4c)) + 1);
    dVar33 = 5.0 / dVar25;
    dVar36 = dVar33 + ((double)*(int *)(lVar15 + 0x48) - dVar24) +
                      dVar24 + dVar24 + (double)((*(int *)(lVar15 + 0x50) - *(int *)(lVar15 + 0x48))
                                                + 1);
    dVar24 = ((double)*(int *)(lVar15 + 0x4c) - dVar24) + dVar35;
    plVar22 = (long *)(*(long *)(param_5 + 0x2c0) + 0x10);
    (**(code **)(*plVar22 + 0x18))(plVar22);
    dStack_f8 = dVar24 + (3.0 - dVar35) / dVar25;
    local_100 = dVar36;
    FUN_0092a2fc(plVar22,&local_100);
    FUN_009299d4(*(long *)(param_5 + 0x2c0) + 0x10,1);
    if (*(char *)(*(long *)(param_5 + 0x218) + 0x58) != '\0') {
      if ((int)local_190 <= (int)local_188) {
        if (local_190._4_4_ <= local_188._4_4_) {
          bVar3 = *(char *)(param_5 + 0x2d8) != '\0';
          if (bVar3) {
            iVar14 = FUN_004e50a8(*(long *)(param_5 + 0x218) + 0x98);
            (**(code **)(*(long *)(*(long *)(param_5 + 0x2c8) + 0x10) + 0x18))();
            dVar24 = (double)iVar14 - dVar35;
            lVar15 = (long)(int)local_188;
            plVar22 = (long *)(*(long *)(param_5 + 0x2c8) + 0x10);
            (**(code **)(*plVar22 + 0x18))(plVar22);
            local_100 = (((double)lVar15 - dVar33) + 1.0) / dVar25;
            dStack_f8 = (dVar24 * 0.5 + 1.0 + (double)(long)local_190._4_4_) / dVar25;
            FUN_0092a2fc(plVar22,&local_100);
          }
          bVar6 = true;
          goto LAB_0004a648;
        }
      }
    }
    bVar3 = false;
    bVar6 = false;
  }
  else {
    FUN_009299d4(*(long *)(param_5 + 0x2c0) + 0x10,0);
    bVar3 = false;
  }
LAB_0004a648:
  FUN_009299d4(*(long *)(param_5 + 0x2c8) + 0x10,bVar3);
  if ((bVar6) && (*(char *)(param_5 + 0x2d9) != '\0')) {
    iVar14 = FUN_004e50a8(*(long *)(param_5 + 0x218) + 0x98);
    (**(code **)(*(long *)(*(long *)(param_5 + 0x2d0) + 0x10) + 0x18))();
    iVar17 = FUN_00928828(*(long *)(param_5 + 0x2c8) + 0x10);
    dVar24 = 0.0;
    if (iVar17 != 0) {
      iVar17 = FUN_004e50a8(*(long *)(param_5 + 0x218) + 0x98);
      dVar24 = (double)iVar17;
    }
    lVar15 = (long)(int)local_188;
    plVar22 = (long *)(*(long *)(param_5 + 0x2d0) + 0x10);
    (**(code **)(*plVar22 + 0x18))(plVar22);
    local_100 = (((double)lVar15 - dVar33) + 1.0) / dVar25;
    dStack_f8 = (((double)iVar14 - dVar35) * 0.5 + 1.0 + dVar24 + (double)(long)local_190._4_4_) /
                dVar25;
    FUN_0092a2fc(plVar22,&local_100);
    uVar16 = 1;
  }
  else {
    uVar16 = 0;
  }
  FUN_009299d4(*(long *)(param_5 + 0x2d0) + 0x10,uVar16);
LAB_0004a720:
  if (*(long *)PTR____stack_chk_guard_013b4320 == local_90) {
    return;
  }
                    /* WARNING: Subroutine does not return */
  ___stack_chk_fail();
}


