// FUN_00051130 size=2132

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00051130(long param_1)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  undefined8 uVar4;
  long lVar5;
  long lVar6;
  ulong uVar7;
  ulong uVar8;
  ulong uVar9;
  long unaff_x19;
  undefined8 *unaff_x20;
  undefined8 uVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  ulong uVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  float fVar18;
  float fVar19;
  float fVar20;
  float fVar21;
  undefined1 auVar22 [16];
  undefined8 uStack_100;
  undefined8 uStack_f8;
  undefined8 uStack_f0;
  undefined8 uStack_e8;
  undefined8 *puStack_e0;
  long lStack_d8;
  undefined1 *puStack_d0;
  undefined8 uStack_c8;
  ulong uStack_c0;
  undefined8 uStack_b8;
  float fStack_a8;
  float fStack_a4;
  float fStack_a0;
  float fStack_98;
  float fStack_94;
  float fStack_90;
  undefined8 uStack_88;
  float fStack_80;
  undefined8 uStack_78;
  float fStack_70;
  undefined8 uStack_68;
  float fStack_60;
  undefined8 uStack_58;
  float fStack_50;
  long lStack_48;
  
  lStack_48 = *(long *)PTR____stack_chk_guard_013b4320;
  lVar5 = *(long *)(param_1 + 0x148);
  if (lVar5 == 0) goto LAB_000519c8;
  uVar1 = *(uint *)(param_1 + 0xcc);
  switch(uVar1) {
  case 1:
    uVar10 = 0x4348000043480000;
    uVar4 = _UNK_00c94018;
    goto code_r0x00051238;
  case 2:
    uVar10 = _UNK_00c94008;
    uVar4 = _UNK_00c94010;
code_r0x00051238:
    fStack_a0 = 200.5;
    break;
  case 3:
    uVar4 = 0x4348800043488000;
    fStack_a0 = 255.5;
    uVar10 = _UNK_00c94000;
    break;
  default:
    fStack_a0 = 255.5;
    uVar4 = 0x437f8000437f8000;
    uVar10 = 0x437f0000437f0000;
    break;
  case 5:
  case 10:
  case 0xb:
  case 0xc:
  case 0x11:
    fStack_90 = -0.5;
    fStack_98 = 31.5;
    fStack_a0 = 225.5;
    uVar10 = _UNK_00c94030;
    uVar4 = _UNK_00c94038;
    goto code_r0x000512a0;
  case 6:
  case 0xd:
  case 0xe:
  case 0xf:
  case 0x12:
    fStack_98 = -0.5;
    fStack_90 = 15.5;
    fStack_a0 = 255.5;
    uVar10 = _UNK_00c94020;
    uVar4 = _UNK_00c94028;
    goto code_r0x000512a0;
  }
  fStack_90 = -0.5;
  fStack_98 = -0.5;
code_r0x000512a0:
  fStack_a4 = (float)uVar10;
  fStack_a8 = (float)uVar4;
  fStack_94 = -0.5;
  fVar15 = (float)((ulong)uVar4 >> 0x20);
  *(float *)((ulong)&fStack_a8 | 4) = fVar15;
  uVar3 = uRam000000000141a826;
  if (uVar1 < 5) {
    func_0x009c6090(*(undefined8 *)(lVar5 + 8),&uStack_78,&uStack_88,1);
    uVar1 = uRam000000000141a806;
    if ((float)uStack_78 <= -2048.0) {
      uStack_78._0_4_ = -2048.0;
    }
    if (2048.0 < (float)uStack_88) {
      uStack_88._0_4_ = 2048.0;
    }
    iVar2 = *(int *)(param_1 + 0xec);
    fVar11 = (float)(iVar2 + -0x32) / 50.0;
    fVar18 = (float)(0x32 - iVar2) / 50.0;
    fVar19 = (float)uStack_78 * fVar11;
    fVar15 = fVar11 * ((float)uStack_78 + ((float)uStack_88 - (float)uStack_78) * 0.33);
    if (iVar2 < 0x33) {
      fVar19 = fVar18 * ((float)uStack_88 + ((float)uStack_88 - (float)uStack_78) * -0.33);
      fVar15 = (float)uStack_88 * fVar18;
      fVar11 = fVar18;
    }
    fVar12 = 1.0 - fVar11;
    fVar15 = fVar15 + fVar12 * 255.0;
    if (uRam000000000141a800 >> 0x11 < 0x333) {
      uVar9 = (ulong)uRam000000000141a806;
      *(undefined8 *)(lRam000000000141a818 + 0x7a31a28) = 0x141a804;
      if (*(long *)(lRam000000000141a810 + 0x6dbf9c0) != 0) {
        uVar9 = -(ulong)*(byte *)(*(long *)(lRam000000000141a810 + 0x6dbf9c0) + (uVar9 & 0xffff));
      }
      fVar15 = fVar15 + fVar15 * 10.0 *
                        (float)(long)((ulong)(uVar1 >> 0x18) + (uVar9 + (uVar1 >> 0x10 & 0xff)) * 2)
      ;
    }
    uRam000000000141a800 =
         (uint)((float)uStack_78 +
               (float)(CONCAT44((uRam000000000141a800 + (uRam000000000141a800 >> 0x10)) * 0x101 +
                                (uRam000000000141a800 >> 0x18),uRam000000000141a800) >> 0x10 &
                      0xffffffff));
    if (fVar15 == fVar19) {
      fStack_50 = 1.0;
      uStack_58 = NEON_fmov(0x3f800000,4);
      uStack_68 = CONCAT44(fStack_94,fStack_98);
      fStack_60 = fStack_90;
      fVar15 = fStack_98;
      fVar18 = fStack_94;
LAB_000518c0:
      uVar14 = (ulong)(uint)fVar15;
      uVar9 = (ulong)(uint)fStack_50;
      fVar15 = fStack_50;
    }
    else {
      fVar19 = fVar19 + -0.5;
      fVar20 = fVar11 * 32.0;
      fStack_50 = (fVar15 + 0.5) - fVar19;
      fVar18 = (fStack_a8 + fStack_98) * 0.5;
      fVar16 = (fStack_a8 - fStack_98) * 0.5;
      fVar15 = fVar18 + fVar16;
      fVar13 = fVar20 + fVar18;
      if (iVar2 < 0x33) {
        fVar13 = fVar18;
        fVar15 = fVar12 * fVar15 + fVar11 * (fVar15 + -32.0);
      }
      fVar13 = fVar13 - fVar16;
      fVar17 = (fVar15 - fVar13) / fStack_50;
      uVar9 = (ulong)(uint)fVar17;
      fVar13 = fVar13 - fVar19 * fVar17;
      uVar14 = (ulong)(uint)fVar13;
      fVar18 = (fStack_a4 + fStack_94) * 0.5;
      fVar16 = (fStack_a4 - fStack_94) * 0.5;
      fVar21 = fVar18 + fVar16;
      fVar15 = fVar12 * fVar21 + fVar11 * (fVar21 + -32.0);
      if (0x32 < iVar2) {
        fVar18 = fVar20 + fVar18;
        fVar15 = fVar21;
      }
      fVar18 = fVar18 - fVar16;
      fVar15 = (fVar15 - fVar18) / fStack_50;
      fVar18 = fVar18 - fVar19 * fVar15;
      fStack_60 = (fStack_a0 + fStack_90) * 0.5;
      fVar16 = (fStack_a0 - fStack_90) * 0.5;
      fVar21 = fStack_60 + fVar16;
      fVar11 = fVar12 * fVar21 + fVar11 * (fVar21 + -32.0);
      if (0x32 < iVar2) {
        fStack_60 = fVar20 + fStack_60;
        fVar11 = fVar21;
      }
      fStack_60 = fStack_60 - fVar16;
      fStack_50 = (fVar11 - fStack_60) / fStack_50;
      uStack_58 = CONCAT44(fVar15,fVar17);
      fStack_60 = fStack_60 - fVar19 * fStack_50;
      uStack_68 = CONCAT44(fVar18,fVar13);
    }
  }
  else {
    if (uVar1 - 7 < 0xc) {
      fVar18 = (float)*(int *)(param_1 + 0xec);
      fVar15 = fVar18 * 0.013333334 + 0.33333334;
      if (0x32 < *(int *)(param_1 + 0xec)) {
        fVar15 = fVar18 * 0.04 + -1.0;
      }
      lVar5 = *(long *)(lVar5 + 0x18);
      fVar18 = -0.5;
      if ((lVar5 == 0) || (*(char *)(lVar5 + 0xe0) == '\0')) {
        fStack_50 = 255.5;
      }
      else {
        unaff_x20 = &uStack_58;
        uStack_b8 = 0;
        uStack_c0 = (ulong)(uint)fVar15;
        func_0x001482a8(lVar5,&uStack_58,&uStack_68);
        fVar15 = (float)uStack_c0;
        fVar11 = *(float *)((long)unaff_x20 +
                           (ulong)*(uint *)(&UNK_00c93e98 + (ulong)*(uint *)(param_1 + 0xcc) * 4) *
                           4);
        fVar19 = *(float *)((long)&uStack_68 +
                           (ulong)*(uint *)(&UNK_00c93e98 + (ulong)*(uint *)(param_1 + 0xcc) * 4) *
                           4);
        fVar18 = fVar11 + 1.0;
        fStack_50 = fVar19 + -1.0;
        if (fVar19 != fVar11) {
          fVar18 = fVar11;
          fStack_50 = fVar19;
        }
        uVar4 = CONCAT44(fStack_a4,fStack_a8);
      }
      fStack_50 = fStack_50 - fVar18;
      fVar13 = (float)((ulong)uVar4 >> 0x20);
      fVar11 = ((float)uVar4 + fStack_98) * 0.5;
      fVar19 = (fVar13 + fStack_94) * 0.5;
      fVar12 = ((float)uVar4 - fStack_98) * fVar15 * 0.5;
      fVar13 = (fVar13 - fStack_94) * fVar15 * 0.5;
      fVar16 = fVar11 - fVar12;
      fVar20 = fVar19 - fVar13;
      fVar11 = ((fVar11 + fVar12) - fVar16) / fStack_50;
      fVar19 = ((fVar19 + fVar13) - fVar20) / fStack_50;
      uVar9 = CONCAT44(fVar19,fVar11);
      uVar14 = CONCAT44(fVar20 - fVar19 * fVar18,fVar16 - fVar11 * fVar18);
      fVar11 = (fStack_a0 + fStack_90) * 0.5;
      fVar15 = fVar15 * (fStack_a0 - fStack_90) * 0.5;
      fStack_60 = fVar11 - fVar15;
      fStack_50 = ((fVar11 + fVar15) - fStack_60) / fStack_50;
      fStack_60 = fStack_60 - fVar18 * fStack_50;
    }
    else {
      fStack_50 = (float)*(int *)(param_1 + 0xec) / 50.0;
      if (1 < uVar1 - 5) {
        uStack_58 = CONCAT44(fStack_50,fStack_50);
        fVar15 = 128.0;
        fStack_60 = 128.0;
        uStack_68 = 0x4300000043000000;
        if (uRam000000000141a820 >> 0x12 < 0x1af) {
          uVar9 = (ulong)uRam000000000141a826;
          *(undefined8 *)(lRam000000000141a838 + 0x608acf8) = 0x141a824;
          if (*(long *)(lRam000000000141a830 + 0x565b160) != 0) {
            uVar9 = -(ulong)*(byte *)(*(long *)(lRam000000000141a830 + 0x565b160) + (uVar9 & 0xffff)
                                     );
          }
          fVar15 = (float)(long)(((ulong)(uVar3 >> 0x18) + (uVar9 + (uVar3 >> 0x10 & 0xff)) * 2) *
                                0x400) + 128.0;
          uStack_68 = CONCAT44(0x43000000,fVar15);
        }
        uRam000000000141a820 =
             (uint)(fStack_50 +
                   (float)(CONCAT44((uRam000000000141a820 + (uRam000000000141a820 >> 0x10)) * 0x101
                                    + (uRam000000000141a820 >> 0x18),uRam000000000141a820) >> 0x10 &
                          0xffffffff));
        fVar18 = 128.0;
        goto LAB_000518c0;
      }
      fVar18 = (fStack_a8 + fStack_98) * 0.5;
      fVar11 = (fVar15 + -0.5) * 0.5;
      fVar19 = (fStack_a8 - fStack_98) * fStack_50 * 0.5;
      fVar12 = (fVar15 + 0.5) * fStack_50 * 0.5;
      fVar13 = fVar18 - fVar19;
      fVar16 = fVar11 - fVar12;
      fVar15 = ((fVar18 + fVar19) - fVar13) / 129.0;
      fVar18 = ((fVar11 + fVar12) - fVar16) / 129.0;
      uVar9 = CONCAT44(fVar18,fVar15);
      uVar14 = CONCAT44(fVar16 + fVar18 * 64.5,fVar13 + fVar15 * 64.5);
      fVar18 = (fStack_a0 + fStack_90) * 0.5;
      fVar15 = fStack_50 * (fStack_a0 - fStack_90) * 0.5;
      fStack_60 = fVar18 - fVar15;
      fStack_50 = ((fVar18 + fVar15) - fStack_60) / 129.0;
      fStack_60 = fStack_60 + fStack_50 * 64.5;
    }
    fVar15 = (float)(uVar9 >> 0x20);
    fVar18 = (float)(uVar14 >> 0x20);
    uStack_68 = uVar14;
    uStack_58 = uVar9;
  }
  uVar1 = *(uint *)(lRam000000000141a840 + 0x18df890);
  uVar3 = *(uint *)(lRam000000000141a850 + 0x1c86948);
  if ((int)(CONCAT44((uVar3 + (uVar3 >> 0x10)) * 0x101 + (uVar3 >> 0x18),uVar3) >> 0x10) != 0) {
    uVar3 = (uVar1 ^ uVar1 >> 1) & 0x41545040;
    *(uint *)(lRam000000000141a848 + 0x17bd640) = (uVar3 | uVar3 << 1) ^ uVar1;
  }
  lVar5 = *(long *)(param_1 + 0x148);
  uVar7 = *(ulong *)(lVar5 + 0x60);
  fStack_70 = *(float *)(lVar5 + 0x68);
  uVar8 = *(ulong *)(lVar5 + 0x6c);
  fStack_80 = *(float *)(lVar5 + 0x74);
  uStack_78._0_4_ = (float)uVar7;
  unaff_x19 = param_1;
  uStack_88 = uVar8;
  uStack_78 = uVar7;
  if ((((((float)uVar9 == (float)uStack_78) &&
        (uStack_88._0_4_ = (float)uVar8, (float)uVar14 == (float)uStack_88)) &&
       (uStack_78._4_4_ = (float)(uVar7 >> 0x20), fVar15 == uStack_78._4_4_)) &&
      ((uStack_88._4_4_ = (float)(uVar8 >> 0x20), fVar18 == uStack_88._4_4_ &&
       (fStack_50 == fStack_70)))) && (fStack_60 == fStack_80)) {
LAB_000519c8:
    uVar4 = 0;
  }
  else {
    *(float *)(lVar5 + 0x68) = fStack_50;
    *(ulong *)(lVar5 + 0x60) = uStack_58;
    *(float *)(lVar5 + 0x74) = fStack_60;
    *(ulong *)(lVar5 + 0x6c) = uStack_68;
    uVar4 = 1;
  }
  if (*(long *)PTR____stack_chk_guard_013b4320 != lStack_48) {
    auVar22 = ___stack_chk_fail(uVar4);
    lVar5 = auVar22._0_8_;
    uStack_c8 = 0x51a00;
    lVar6 = *(long *)(lVar5 + 0x148);
    if ((lVar6 != 0) && (((uint)(*(char *)(lVar6 + 0x78) != '\0') ^ auVar22._8_4_) == 1)) {
      *(char *)(lVar6 + 0x78) = auVar22[8];
      puStack_e0 = unaff_x20;
      lStack_d8 = unaff_x19;
      puStack_d0 = &stack0xfffffffffffffff0;
      if (*(long *)(lVar5 + 0x150) != 0) {
        uStack_f8 = 0;
        uStack_100 = 0;
        uStack_e8 = 0;
        uStack_f0 = 0;
        func_0x009285d8(*(long *)(lVar5 + 0x150),&uStack_100);
      }
      func_0x001d1dcc(lVar5,auVar22._8_8_);
    }
    return;
  }
  return;
}


