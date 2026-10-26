// FUN_009b2814 size=1764

void FUN_009b2814(undefined8 *param_1,undefined **param_2,undefined **param_3,undefined **param_4,
                 uint param_5)

{
  long lVar1;
  bool bVar2;
  int iVar3;
  ulong uVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  undefined *puStack_210;
  undefined ***pppuStack_208;
  undefined ***pppuStack_200;
  float **ppfStack_1f8;
  undefined ***pppuStack_1f0;
  undefined ***pppuStack_1e8;
  undefined **ppuStack_1e0;
  undefined ***pppuStack_1d8;
  undefined ***pppuStack_1d0;
  float **ppfStack_1c8;
  undefined ***pppuStack_1c0;
  undefined ***pppuStack_1b8;
  undefined **ppuStack_1b0;
  undefined ***pppuStack_1a8;
  undefined ***pppuStack_1a0;
  float **ppfStack_198;
  undefined ***pppuStack_190;
  undefined ***pppuStack_188;
  float *pfStack_180;
  float *pfStack_178;
  float **ppfStack_170;
  undefined *puStack_168;
  undefined ***pppuStack_160;
  undefined ***pppuStack_158;
  float **ppfStack_150;
  undefined ***pppuStack_148;
  undefined ***pppuStack_140;
  float *pfStack_138;
  float *pfStack_130;
  float **ppfStack_128;
  undefined4 uStack_120;
  undefined4 uStack_11c;
  undefined **ppuStack_118;
  undefined **ppuStack_110;
  undefined **ppuStack_108;
  undefined **ppuStack_100;
  undefined **ppuStack_f8;
  undefined ***pppuStack_f0;
  undefined ***pppuStack_e8;
  undefined ***pppuStack_e0;
  undefined ***pppuStack_d8;
  undefined1 *puStack_d0;
  undefined **ppuStack_c8;
  undefined ***pppuStack_c0;
  undefined ***pppuStack_b8;
  float **ppfStack_b0;
  undefined ***pppuStack_a8;
  undefined ***pppuStack_a0;
  undefined **ppuStack_98;
  undefined ***pppuStack_90;
  undefined ***pppuStack_88;
  float **ppfStack_80;
  undefined ***pppuStack_78;
  undefined ***pppuStack_70;
  float *pfStack_68;
  float *pfStack_60;
  float **ppfStack_58;
  undefined8 uStack_50;
  float fStack_48;
  undefined8 uStack_44;
  float fStack_3c;
  long lStack_38;
  
  lStack_38 = *(long *)PTR____stack_chk_guard_013b4320;
  ppuStack_118 = (undefined **)((long)param_2[0x1b] * (long)param_2[0x1a]);
  uStack_11c = 0x3e99999a;
  uStack_120 = 0x3f333333;
  ppuStack_110 = param_4;
  ppuStack_108 = param_3;
  ppuStack_100 = param_2;
  if (*(int *)(param_2 + 0x1e) != 2) {
    if (*(int *)(param_2 + 0x1e) != 1) goto LAB_009b2a28;
    pppuStack_1a8 = &ppuStack_100;
    puStack_168 = &UNK_013fcfd0;
    pppuStack_1a0 = (undefined ***)&uStack_11c;
    ppfStack_198 = (float **)&uStack_120;
    pppuStack_190 = &ppuStack_108;
    pppuStack_188 = &ppuStack_110;
    pfStack_180 = (float *)&uStack_44;
    pfStack_178 = (float *)&uStack_50;
    ppfStack_170 = (float **)&ppuStack_118;
    ppuStack_1b0 = (undefined **)&UNK_013fd000;
    ppuStack_98 = (undefined **)&UNK_013fd030;
    pppuStack_160 = pppuStack_1a8;
    pppuStack_158 = pppuStack_1a0;
    ppfStack_150 = ppfStack_198;
    pppuStack_148 = pppuStack_190;
    pppuStack_140 = pppuStack_188;
    pfStack_138 = pfStack_180;
    pfStack_130 = pfStack_178;
    ppfStack_128 = ppfStack_170;
    pppuStack_90 = pppuStack_1a8;
    pppuStack_88 = pppuStack_1a0;
    ppfStack_80 = ppfStack_198;
    pppuStack_78 = pppuStack_190;
    pppuStack_70 = pppuStack_188;
    pfStack_68 = pfStack_180;
    pfStack_60 = pfStack_178;
    ppfStack_58 = ppfStack_170;
    if ((bRam00000000015487d0 & 1) == 0) goto LAB_009b2e5c;
    while( true ) {
      ppuStack_c8 = &puStack_168;
      pppuStack_c0 = &ppuStack_1b0;
      pppuStack_b8 = &ppuStack_98;
      uVar6 = 0x15456b8;
      lVar1 = -0xb8;
      uVar5 = 3;
LAB_009b2a24:
      func_0x009fc3a8(&stack0xfffffffffffffff0 + lVar1,uVar5,uRam00000000014a67c0,uVar6,ppuStack_118
                     );
LAB_009b2a28:
      uVar4 = (**(code **)*param_1)(param_1);
      if ((uVar4 & 1) == 0) {
        fVar7 = 1.0;
        if (param_5 == 0) {
          fVar7 = 0.25;
        }
        fVar8 = *(float *)ppuStack_110;
        fVar9 = *(float *)ppuStack_108;
        fVar10 = ABS(fVar8 - fVar9);
        bVar2 = fVar10 * 20.0 < ABS((float)uStack_50 - (float)uStack_44);
        if ((param_5 & 1) == 0) {
          if (bVar2) {
            *(float *)ppuStack_108 = (fVar9 + (float)uStack_44) * 0.5;
            fVar8 = (*(float *)ppuStack_110 + (float)uStack_50) * 0.5;
            *(float *)ppuStack_110 = fVar8;
            fVar9 = *(float *)ppuStack_108;
            fVar10 = ABS(fVar8 - fVar9);
          }
          if (fVar10 <= fVar7) {
            fVar10 = fVar7;
          }
          fVar8 = (fVar8 + fVar9) * 0.5;
          fVar10 = fVar10 * 0.5 * 8.0;
          *(float *)ppuStack_108 = fVar8 - fVar10;
          *(float *)ppuStack_110 = fVar8 + fVar10;
          fVar8 = *(float *)((long)ppuStack_110 + 4);
          fVar9 = *(float *)((long)ppuStack_108 + 4);
          fVar10 = ABS(fVar8 - fVar9);
          if (fVar10 * 20.0 < ABS(uStack_50._4_4_ - uStack_44._4_4_)) {
            *(float *)((long)ppuStack_108 + 4) = (fVar9 + uStack_44._4_4_) * 0.5;
            fVar8 = (*(float *)((long)ppuStack_110 + 4) + uStack_50._4_4_) * 0.5;
            *(float *)((long)ppuStack_110 + 4) = fVar8;
            fVar9 = *(float *)((long)ppuStack_108 + 4);
            fVar10 = ABS(fVar8 - fVar9);
          }
          if (fVar10 <= fVar7) {
            fVar10 = fVar7;
          }
          fVar8 = (fVar8 + fVar9) * 0.5;
          fVar10 = fVar10 * 0.5 * 8.0;
          *(float *)((long)ppuStack_108 + 4) = fVar8 - fVar10;
          *(float *)((long)ppuStack_110 + 4) = fVar8 + fVar10;
          fVar8 = *(float *)(ppuStack_110 + 1);
          fVar9 = *(float *)(ppuStack_108 + 1);
          fVar10 = ABS(fVar8 - fVar9);
          if (fVar10 * 20.0 < ABS(fStack_48 - fStack_3c)) {
            *(float *)(ppuStack_108 + 1) = (fVar9 + fStack_3c) * 0.5;
            fVar8 = (*(float *)(ppuStack_110 + 1) + fStack_48) * 0.5;
            *(float *)(ppuStack_110 + 1) = fVar8;
            fVar9 = *(float *)(ppuStack_108 + 1);
            fVar10 = ABS(fVar8 - fVar9);
          }
          if (fVar10 <= fVar7) {
            fVar10 = fVar7;
          }
          fVar8 = (fVar8 + fVar9) * 0.5;
          fVar7 = fVar10 * 0.5 * 8.0;
          *(float *)(ppuStack_108 + 1) = fVar8 - fVar7;
          *(float *)(ppuStack_110 + 1) = fVar8 + fVar7;
        }
        else {
          if (bVar2) {
            *(float *)ppuStack_108 = (fVar9 + (float)uStack_44) * 0.5;
            fVar8 = (*(float *)ppuStack_110 + (float)uStack_50) * 0.5;
            *(float *)ppuStack_110 = fVar8;
            fVar9 = *(float *)ppuStack_108;
            fVar10 = ABS(fVar8 - fVar9);
          }
          if (fVar10 <= fVar7) {
            fVar10 = fVar7;
          }
          fVar8 = (fVar8 + fVar9) * 0.5;
          fVar10 = fVar10 * 0.5 * 8.0;
          *(float *)ppuStack_108 = fVar8 - fVar10;
          *(float *)ppuStack_110 = fVar8 + fVar10;
          fVar8 = *(float *)((long)ppuStack_110 + 4);
          fVar9 = *(float *)((long)ppuStack_108 + 4);
          fVar10 = ABS(fVar8 - fVar9);
          if (fVar10 * 20.0 < ABS(uStack_50._4_4_ - uStack_44._4_4_)) {
            *(float *)((long)ppuStack_108 + 4) = (fVar9 + uStack_44._4_4_) * 0.5;
            fVar8 = (*(float *)((long)ppuStack_110 + 4) + uStack_50._4_4_) * 0.5;
            *(float *)((long)ppuStack_110 + 4) = fVar8;
            fVar9 = *(float *)((long)ppuStack_108 + 4);
            fVar10 = ABS(fVar8 - fVar9);
          }
          if (fVar10 <= fVar7) {
            fVar10 = fVar7;
          }
          fVar8 = (fVar8 + fVar9) * 0.5;
          fVar10 = fVar10 * 0.5 * 8.0;
          fVar10 = fVar10 + fVar10;
          *(float *)((long)ppuStack_108 + 4) = fVar8 - fVar10;
          *(float *)((long)ppuStack_110 + 4) = fVar8 + fVar10;
          fVar8 = *(float *)(ppuStack_110 + 1);
          fVar9 = *(float *)(ppuStack_108 + 1);
          fVar10 = ABS(fVar8 - fVar9);
          if (fVar10 * 20.0 < ABS(fStack_48 - fStack_3c)) {
            *(float *)(ppuStack_108 + 1) = (fVar9 + fStack_3c) * 0.5;
            fVar8 = (*(float *)(ppuStack_110 + 1) + fStack_48) * 0.5;
            *(float *)(ppuStack_110 + 1) = fVar8;
            fVar9 = *(float *)(ppuStack_108 + 1);
            fVar10 = ABS(fVar8 - fVar9);
          }
          if (fVar10 <= fVar7) {
            fVar10 = fVar7;
          }
          fVar8 = (fVar8 + fVar9) * 0.5;
          fVar7 = fVar10 * 0.5 * 8.0;
          fVar7 = fVar7 + fVar7;
          *(float *)(ppuStack_108 + 1) = fVar8 - fVar7;
          *(float *)(ppuStack_110 + 1) = fVar8 + fVar7;
          if (*(int *)(ppuStack_100 + 0x1e) == 2) {
            pppuStack_160 = &ppuStack_c8;
            puStack_168 = &UNK_013fd180;
            pppuStack_1a0 = &ppuStack_100;
            ppfStack_198 = (float **)&ppuStack_118;
            pppuStack_1a8 = &ppuStack_1e0;
            ppuStack_1b0 = (undefined **)&UNK_013fd1b0;
            pppuStack_158 = pppuStack_1a0;
            ppfStack_150 = ppfStack_198;
            if (((bRam00000000015487e0 & 1) == 0) &&
               (iVar3 = ___cxa_guard_acquire(0x15487e0), iVar3 != 0)) {
              func_0x009fbba8(0x1546a58,"FindFreqLevs.32.1&99P");
              ___cxa_guard_release(0x15487e0);
            }
            ppuStack_98 = &puStack_168;
            pppuStack_90 = &ppuStack_1b0;
            func_0x009fc3a8(&ppuStack_98,2,uRam00000000014a67c0,0x1546a58,ppuStack_118);
          }
          else if (*(int *)(ppuStack_100 + 0x1e) == 1) {
            func_0x0099caa8(0x3c23d70a,0x3f7d70a4,ppuStack_100[0x16],&ppuStack_c8,&ppuStack_1e0,0,0,
                            ppuStack_118);
          }
          else {
            ppuStack_1e0 = (undefined **)((ulong)ppuStack_1e0 & 0xffffffff00000000);
            ppuStack_c8 = (undefined **)((ulong)ppuStack_c8 & 0xffffffff00000000);
          }
          if (0.01 < ABS(ppuStack_1e0._0_4_ - ppuStack_c8._0_4_)) {
            fVar7 = *(float *)ppuStack_108;
            if (*(float *)ppuStack_108 <= ppuStack_c8._0_4_) {
              fVar7 = ppuStack_c8._0_4_;
            }
            *(float *)ppuStack_108 = fVar7;
            fVar7 = ppuStack_1e0._0_4_;
            if (*(float *)ppuStack_110 <= ppuStack_1e0._0_4_) {
              fVar7 = *(float *)ppuStack_110;
            }
            *(float *)ppuStack_110 = fVar7;
          }
        }
      }
      if (*(long *)PTR____stack_chk_guard_013b4320 == lStack_38) break;
      ___stack_chk_fail();
LAB_009b2e5c:
      iVar3 = ___cxa_guard_acquire(0x15487d0);
      if (iVar3 != 0) {
        func_0x009fbba8(0x15456b8,"FindFreqLevs.Half");
        ___cxa_guard_release(0x15487d0);
      }
    }
    return;
  }
  pppuStack_1d8 = &ppuStack_108;
  puStack_168 = &UNK_013fd060;
  pppuStack_200 = &ppuStack_100;
  ppfStack_1c8 = (float **)&uStack_11c;
  pppuStack_1c0 = (undefined ***)&uStack_44;
  pppuStack_1e8 = &ppuStack_118;
  pppuStack_208 = &ppuStack_110;
  ppuStack_1b0 = (undefined **)&UNK_013fd090;
  ppfStack_1f8 = (float **)&uStack_120;
  pppuStack_1f0 = (undefined ***)&uStack_50;
  ppuStack_98 = (undefined **)&UNK_013fd0c0;
  ppuStack_c8 = (undefined **)&UNK_013fd0f0;
  ppuStack_1e0 = (undefined **)&UNK_013fd120;
  puStack_210 = &UNK_013fd150;
  pppuStack_1d0 = pppuStack_200;
  pppuStack_1b8 = pppuStack_1e8;
  pppuStack_1a8 = pppuStack_208;
  pppuStack_1a0 = pppuStack_200;
  ppfStack_198 = ppfStack_1f8;
  pppuStack_190 = pppuStack_1f0;
  pppuStack_188 = pppuStack_1e8;
  pppuStack_160 = pppuStack_1d8;
  pppuStack_158 = pppuStack_200;
  ppfStack_150 = ppfStack_1c8;
  pppuStack_148 = pppuStack_1c0;
  pppuStack_140 = pppuStack_1e8;
  pppuStack_c0 = pppuStack_208;
  pppuStack_b8 = pppuStack_200;
  ppfStack_b0 = ppfStack_1f8;
  pppuStack_a8 = pppuStack_1f0;
  pppuStack_a0 = pppuStack_1e8;
  pppuStack_90 = pppuStack_1d8;
  pppuStack_88 = pppuStack_200;
  ppfStack_80 = ppfStack_1c8;
  pppuStack_78 = pppuStack_1c0;
  pppuStack_70 = pppuStack_1e8;
  if (((bRam00000000015487d8 & 1) == 0) && (iVar3 = ___cxa_guard_acquire(0x15487d8), iVar3 != 0)) {
    func_0x009fbba8(0x1546088,"FindFreqLevs.32");
    ___cxa_guard_release(0x15487d8);
  }
  ppuStack_f8 = &puStack_168;
  pppuStack_f0 = &ppuStack_1b0;
  pppuStack_e8 = &ppuStack_98;
  pppuStack_e0 = &ppuStack_c8;
  pppuStack_d8 = &ppuStack_1e0;
  uVar6 = 0x1546088;
  lVar1 = -0xe8;
  uVar5 = 6;
  puStack_d0 = (undefined1 *)&puStack_210;
  goto LAB_009b2a24;
}


