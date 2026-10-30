// FUN_009b2814 @ 009b2814 size=1764 tag=FindFreqLevs_L15_parent

void FUN_009b2814(undefined8 *param_1,undefined **param_2,undefined **param_3,undefined **param_4,
                 uint param_5)

{
  long lVar1;
  bool bVar2;
  int iVar3;
  ulong uVar4;
  undefined8 uVar5;
  undefined *puVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  undefined *local_210;
  undefined ***pppuStack_208;
  undefined ***local_200;
  float **ppfStack_1f8;
  undefined ***local_1f0;
  undefined ***pppuStack_1e8;
  undefined **local_1e0;
  undefined ***pppuStack_1d8;
  undefined ***local_1d0;
  float **ppfStack_1c8;
  undefined ***local_1c0;
  undefined ***pppuStack_1b8;
  undefined **local_1b0;
  undefined ***pppuStack_1a8;
  undefined ***local_1a0;
  float **ppfStack_198;
  undefined ***local_190;
  undefined ***pppuStack_188;
  float *local_180;
  float *pfStack_178;
  float **local_170;
  undefined *local_168;
  undefined ***pppuStack_160;
  undefined ***local_158;
  float **ppfStack_150;
  undefined ***local_148;
  undefined ***pppuStack_140;
  float *local_138;
  float *pfStack_130;
  float **local_128;
  undefined4 local_120;
  undefined4 local_11c;
  undefined **local_118;
  undefined **local_110;
  undefined **local_108;
  undefined **local_100;
  undefined **local_f8;
  undefined ***pppuStack_f0;
  undefined ***local_e8;
  undefined ***pppuStack_e0;
  undefined ***local_d8;
  undefined1 *puStack_d0;
  undefined **local_c8;
  undefined ***pppuStack_c0;
  undefined ***local_b8;
  float **ppfStack_b0;
  undefined ***local_a8;
  undefined ***pppuStack_a0;
  undefined **local_98;
  undefined ***pppuStack_90;
  undefined ***local_88;
  float **ppfStack_80;
  undefined ***local_78;
  undefined ***pppuStack_70;
  float *local_68;
  float *pfStack_60;
  float **local_58;
  undefined8 local_50;
  float local_48;
  undefined8 local_44;
  float local_3c;
  long local_38;
  
  local_38 = *(long *)PTR____stack_chk_guard_013b4320;
  local_118 = (undefined **)((long)param_2[0x1b] * (long)param_2[0x1a]);
  local_11c = 0x3e99999a;
  local_120 = 0x3f333333;
  local_110 = param_4;
  local_108 = param_3;
  local_100 = param_2;
  if (*(int *)(param_2 + 0x1e) == 2) {
    pppuStack_1d8 = &local_108;
    local_168 = &DAT_013fd060;
    local_200 = &local_100;
    ppfStack_1c8 = (float **)&local_11c;
    local_1c0 = (undefined ***)&local_44;
    pppuStack_1e8 = &local_118;
    pppuStack_208 = &local_110;
    local_1b0 = (undefined **)&DAT_013fd090;
    ppfStack_1f8 = (float **)&local_120;
    local_1f0 = (undefined ***)&local_50;
    local_98 = (undefined **)&DAT_013fd0c0;
    local_c8 = (undefined **)&DAT_013fd0f0;
    local_1e0 = (undefined **)&DAT_013fd120;
    local_210 = &DAT_013fd150;
    local_1d0 = local_200;
    pppuStack_1b8 = pppuStack_1e8;
    pppuStack_1a8 = pppuStack_208;
    local_1a0 = local_200;
    ppfStack_198 = ppfStack_1f8;
    local_190 = local_1f0;
    pppuStack_188 = pppuStack_1e8;
    pppuStack_160 = pppuStack_1d8;
    local_158 = local_200;
    ppfStack_150 = ppfStack_1c8;
    local_148 = local_1c0;
    pppuStack_140 = pppuStack_1e8;
    pppuStack_c0 = pppuStack_208;
    local_b8 = local_200;
    ppfStack_b0 = ppfStack_1f8;
    local_a8 = local_1f0;
    pppuStack_a0 = pppuStack_1e8;
    pppuStack_90 = pppuStack_1d8;
    local_88 = local_200;
    ppfStack_80 = ppfStack_1c8;
    local_78 = local_1c0;
    pppuStack_70 = pppuStack_1e8;
    if (((DAT_015487d8 & 1) == 0) && (iVar3 = ___cxa_guard_acquire(&DAT_015487d8), iVar3 != 0)) {
      thunk_FUN_009fb9f0(&DAT_01546088,"FindFreqLevs.32");
      ___cxa_guard_release(&DAT_015487d8);
    }
    local_f8 = &local_168;
    pppuStack_f0 = &local_1b0;
    local_e8 = &local_98;
    pppuStack_e0 = &local_c8;
    local_d8 = &local_1e0;
    puVar6 = &DAT_01546088;
    lVar1 = -0xe8;
    uVar5 = 6;
    puStack_d0 = (undefined1 *)&local_210;
  }
  else {
    if (*(int *)(param_2 + 0x1e) != 1) goto LAB_009b2a28;
    pppuStack_1a8 = &local_100;
    local_168 = &DAT_013fcfd0;
    local_1a0 = (undefined ***)&local_11c;
    ppfStack_198 = (float **)&local_120;
    local_190 = &local_108;
    pppuStack_188 = &local_110;
    local_180 = (float *)&local_44;
    pfStack_178 = (float *)&local_50;
    local_170 = (float **)&local_118;
    local_1b0 = (undefined **)&DAT_013fd000;
    local_98 = (undefined **)&DAT_013fd030;
    pppuStack_160 = pppuStack_1a8;
    local_158 = local_1a0;
    ppfStack_150 = ppfStack_198;
    local_148 = local_190;
    pppuStack_140 = pppuStack_188;
    local_138 = local_180;
    pfStack_130 = pfStack_178;
    local_128 = local_170;
    pppuStack_90 = pppuStack_1a8;
    local_88 = local_1a0;
    ppfStack_80 = ppfStack_198;
    local_78 = local_190;
    pppuStack_70 = pppuStack_188;
    local_68 = local_180;
    pfStack_60 = pfStack_178;
    local_58 = local_170;
    if (((DAT_015487d0 & 1) == 0) && (iVar3 = ___cxa_guard_acquire(&DAT_015487d0), iVar3 != 0)) {
      thunk_FUN_009fb9f0(&DAT_015456b8,"FindFreqLevs.Half");
      ___cxa_guard_release(&DAT_015487d0);
    }
    local_c8 = &local_168;
    pppuStack_c0 = &local_1b0;
    local_b8 = &local_98;
    puVar6 = &DAT_015456b8;
    lVar1 = -0xb8;
    uVar5 = 3;
  }
  FUN_009fc3a8(&stack0xfffffffffffffff0 + lVar1,uVar5,DAT_014a67c0,puVar6,local_118);
LAB_009b2a28:
  uVar4 = (**(code **)*param_1)(param_1);
  if ((uVar4 & 1) == 0) {
    fVar7 = 1.0;
    if (param_5 == 0) {
      fVar7 = 0.25;
    }
    fVar8 = *(float *)local_110;
    fVar9 = *(float *)local_108;
    fVar10 = ABS(fVar8 - fVar9);
    bVar2 = fVar10 * 20.0 < ABS((float)local_50 - (float)local_44);
    if ((param_5 & 1) == 0) {
      if (bVar2) {
        *(float *)local_108 = (fVar9 + (float)local_44) * 0.5;
        fVar8 = (*(float *)local_110 + (float)local_50) * 0.5;
        *(float *)local_110 = fVar8;
        fVar9 = *(float *)local_108;
        fVar10 = ABS(fVar8 - fVar9);
      }
      if (fVar10 <= fVar7) {
        fVar10 = fVar7;
      }
      fVar8 = (fVar8 + fVar9) * 0.5;
      fVar10 = fVar10 * 0.5 * 8.0;
      *(float *)local_108 = fVar8 - fVar10;
      *(float *)local_110 = fVar8 + fVar10;
      fVar8 = *(float *)((long)local_110 + 4);
      fVar9 = *(float *)((long)local_108 + 4);
      fVar10 = ABS(fVar8 - fVar9);
      if (fVar10 * 20.0 < ABS(local_50._4_4_ - local_44._4_4_)) {
        *(float *)((long)local_108 + 4) = (fVar9 + local_44._4_4_) * 0.5;
        fVar8 = (*(float *)((long)local_110 + 4) + local_50._4_4_) * 0.5;
        *(float *)((long)local_110 + 4) = fVar8;
        fVar9 = *(float *)((long)local_108 + 4);
        fVar10 = ABS(fVar8 - fVar9);
      }
      if (fVar10 <= fVar7) {
        fVar10 = fVar7;
      }
      fVar8 = (fVar8 + fVar9) * 0.5;
      fVar10 = fVar10 * 0.5 * 8.0;
      *(float *)((long)local_108 + 4) = fVar8 - fVar10;
      *(float *)((long)local_110 + 4) = fVar8 + fVar10;
      fVar8 = *(float *)(local_110 + 1);
      fVar9 = *(float *)(local_108 + 1);
      fVar10 = ABS(fVar8 - fVar9);
      if (fVar10 * 20.0 < ABS(local_48 - local_3c)) {
        *(float *)(local_108 + 1) = (fVar9 + local_3c) * 0.5;
        fVar8 = (*(float *)(local_110 + 1) + local_48) * 0.5;
        *(float *)(local_110 + 1) = fVar8;
        fVar9 = *(float *)(local_108 + 1);
        fVar10 = ABS(fVar8 - fVar9);
      }
      if (fVar10 <= fVar7) {
        fVar10 = fVar7;
      }
      fVar8 = (fVar8 + fVar9) * 0.5;
      fVar7 = fVar10 * 0.5 * 8.0;
      *(float *)(local_108 + 1) = fVar8 - fVar7;
      *(float *)(local_110 + 1) = fVar8 + fVar7;
    }
    else {
      if (bVar2) {
        *(float *)local_108 = (fVar9 + (float)local_44) * 0.5;
        fVar8 = (*(float *)local_110 + (float)local_50) * 0.5;
        *(float *)local_110 = fVar8;
        fVar9 = *(float *)local_108;
        fVar10 = ABS(fVar8 - fVar9);
      }
      if (fVar10 <= fVar7) {
        fVar10 = fVar7;
      }
      fVar8 = (fVar8 + fVar9) * 0.5;
      fVar10 = fVar10 * 0.5 * 8.0;
      *(float *)local_108 = fVar8 - fVar10;
      *(float *)local_110 = fVar8 + fVar10;
      fVar8 = *(float *)((long)local_110 + 4);
      fVar9 = *(float *)((long)local_108 + 4);
      fVar10 = ABS(fVar8 - fVar9);
      if (fVar10 * 20.0 < ABS(local_50._4_4_ - local_44._4_4_)) {
        *(float *)((long)local_108 + 4) = (fVar9 + local_44._4_4_) * 0.5;
        fVar8 = (*(float *)((long)local_110 + 4) + local_50._4_4_) * 0.5;
        *(float *)((long)local_110 + 4) = fVar8;
        fVar9 = *(float *)((long)local_108 + 4);
        fVar10 = ABS(fVar8 - fVar9);
      }
      if (fVar10 <= fVar7) {
        fVar10 = fVar7;
      }
      fVar8 = (fVar8 + fVar9) * 0.5;
      fVar10 = fVar10 * 0.5 * 8.0;
      fVar10 = fVar10 + fVar10;
      *(float *)((long)local_108 + 4) = fVar8 - fVar10;
      *(float *)((long)local_110 + 4) = fVar8 + fVar10;
      fVar8 = *(float *)(local_110 + 1);
      fVar9 = *(float *)(local_108 + 1);
      fVar10 = ABS(fVar8 - fVar9);
      if (fVar10 * 20.0 < ABS(local_48 - local_3c)) {
        *(float *)(local_108 + 1) = (fVar9 + local_3c) * 0.5;
        fVar8 = (*(float *)(local_110 + 1) + local_48) * 0.5;
        *(float *)(local_110 + 1) = fVar8;
        fVar9 = *(float *)(local_108 + 1);
        fVar10 = ABS(fVar8 - fVar9);
      }
      if (fVar10 <= fVar7) {
        fVar10 = fVar7;
      }
      fVar8 = (fVar8 + fVar9) * 0.5;
      fVar7 = fVar10 * 0.5 * 8.0;
      fVar7 = fVar7 + fVar7;
      *(float *)(local_108 + 1) = fVar8 - fVar7;
      *(float *)(local_110 + 1) = fVar8 + fVar7;
      if (*(int *)(local_100 + 0x1e) == 2) {
        pppuStack_160 = &local_c8;
        local_168 = &DAT_013fd180;
        local_1a0 = &local_100;
        ppfStack_198 = (float **)&local_118;
        pppuStack_1a8 = &local_1e0;
        local_1b0 = (undefined **)&DAT_013fd1b0;
        local_158 = local_1a0;
        ppfStack_150 = ppfStack_198;
        if (((DAT_015487e0 & 1) == 0) && (iVar3 = ___cxa_guard_acquire(&DAT_015487e0), iVar3 != 0))
        {
          thunk_FUN_009fb9f0(&DAT_01546a58,"FindFreqLevs.32.1&99P");
          ___cxa_guard_release(&DAT_015487e0);
        }
        local_98 = &local_168;
        pppuStack_90 = &local_1b0;
        FUN_009fc3a8(&local_98,2,DAT_014a67c0,&DAT_01546a58,local_118);
      }
      else if (*(int *)(local_100 + 0x1e) == 1) {
        FUN_0099caa8(0x3c23d70a,0x3f7d70a4,local_100[0x16],&local_c8,&local_1e0,0,0,local_118);
      }
      else {
        local_1e0 = (undefined **)((ulong)local_1e0 & 0xffffffff00000000);
        local_c8 = (undefined **)((ulong)local_c8 & 0xffffffff00000000);
      }
      if (0.01 < ABS(local_1e0._0_4_ - local_c8._0_4_)) {
        fVar7 = *(float *)local_108;
        if (*(float *)local_108 <= local_c8._0_4_) {
          fVar7 = local_c8._0_4_;
        }
        *(float *)local_108 = fVar7;
        fVar7 = local_1e0._0_4_;
        if (*(float *)local_110 <= local_1e0._0_4_) {
          fVar7 = *(float *)local_110;
        }
        *(float *)local_110 = fVar7;
      }
    }
  }
  if (*(long *)PTR____stack_chk_guard_013b4320 == local_38) {
    return;
  }
                    /* WARNING: Subroutine does not return */
  ___stack_chk_fail();
}


