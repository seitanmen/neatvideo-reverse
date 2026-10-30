// func entry: 000080a8 name: FUN_000080a8 size: 2836

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_000080a8(long *param_1,undefined8 param_2)

{
  char *pcVar1;
  undefined4 uVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  long lVar6;
  ulong uVar7;
  undefined8 uVar8;
  ulong uVar9;
  long lVar10;
  long *plVar11;
  undefined8 uVar12;
  undefined8 *puVar13;
  code *pcVar14;
  long lVar15;
  double *pdVar16;
  undefined8 uVar17;
  long lVar18;
  long lVar19;
  long lVar20;
  long lVar21;
  long lVar22;
  long lVar23;
  undefined8 uVar24;
  double dVar25;
  double dVar26;
  double dVar27;
  long local_d28;
  undefined1 auStack_d20 [24];
  long local_d08;
  undefined8 local_d00;
  undefined8 local_cf8;
  undefined8 local_cf0;
  undefined **local_ce8;
  long *plStack_ce0;
  long *local_cd8;
  undefined8 uStack_cd0;
  double local_cc8;
  long lStack_cc0;
  long local_cb8;
  long lStack_cb0;
  long local_ca8;
  long lStack_ca0;
  long local_c98;
  undefined1 auStack_c90 [512];
  long local_a90;
  long local_a88;
  long local_a80;
  undefined8 local_a78;
  undefined4 local_a70;
  undefined4 local_a6c;
  int local_a68;
  undefined4 local_a64;
  undefined4 local_a60;
  undefined4 local_a5c;
  undefined8 local_a58;
  undefined8 local_a50;
  undefined8 uStack_a48;
  undefined8 uStack_a40;
  undefined8 uStack_a38;
  undefined8 local_a30;
  undefined8 uStack_a28;
  undefined8 local_a20;
  undefined8 local_a18;
  double local_a10;
  double local_a08;
  int local_9fc;
  undefined8 local_9f8;
  undefined *local_9f0;
  undefined *local_9e8;
  char local_9d9;
  long local_9d8;
  undefined8 local_9d0;
  long local_9c8;
  undefined8 local_9c0;
  double local_9b8;
  long local_9b0 [64];
  long local_7b0 [64];
  long alStack_5b0 [3];
  long *local_598;
  double local_590 [64];
  undefined8 local_390 [65];
  undefined1 auStack_188 [256];
  long local_88;
  
  local_88 = *(long *)PTR____stack_chk_guard_013b4320;
  (**(code **)(DAT_0155b1f0 + 0x50))(param_2,"OfxPropTime",0,&local_9b8);
  (**(code **)(DAT_0155b1e8 + 0x18))(param_1[2],"Output",&local_9c0,0);
  local_9c8 = 0;
  (**(code **)(DAT_0155b1e8 + 0x28))(local_9b8,local_9c0,0,&local_9c8);
  (**(code **)(DAT_0155b1e8 + 0x18))(param_1[2],"Source",&local_9d0,0);
  local_9d8 = 0;
  if (DAT_0155c408 != 1) {
    FUN_00af4a8c(&DAT_0155c400);
  }
  lVar6 = FUN_001c05cc(DAT_0155c418,param_1[3]);
  if (local_9c8 == 0) {
    uVar8 = 1;
    goto joined_r0x000083e4;
  }
  if (DAT_0141a1c8 == 0) {
    FUN_00005ce0(param_1);
  }
  (**(code **)(*param_1 + 0x40))(&local_9f0,param_1,param_2,&local_9d9);
  if ((local_9d9 == '\0') || (local_9e8 != &DAT_0156a548)) {
    (**(code **)(DAT_0155b1e8 + 0x20))(local_9d0,&local_9f8);
    uVar7 = FUN_0000d238(local_9c8);
    uVar2 = FUN_0014bfcc(uVar7,*(undefined1 *)(*DAT_013b94b0 + 0x388));
    uVar8 = FUN_0000cf0c(local_9f8,"OfxImageClipPropFieldOrder",
                         DAT_0141a1c8 == 1 || DAT_0141a1c8 == 0xc);
    FUN_0000d4b4(local_9b8,uVar8,&local_9fc);
    dVar27 = local_9b8;
    uVar9 = (**(code **)(*param_1 + 0x48))(param_1,param_2);
    (**(code **)(DAT_0155b1f0 + 0x70))(local_9f8,"OfxImageEffectPropFrameRange",2,&local_a10);
    if (DAT_0141a1d4 != '\0') {
      local_a08 = local_a08 + -1.0;
    }
    local_a20 = 0;
    local_a18 = 0;
    (**(code **)(DAT_0155b1f0 + 0x78))(local_9c8,"OfxImagePropBounds",4,&local_a20);
    lVar18 = (long)(int)local_a18;
    lVar20 = (long)local_a18._4_4_;
    lVar15 = (long)(int)local_a20;
    lVar22 = (long)local_a20._4_4_;
    iVar3 = (**(code **)(*param_1 + 0x58))(param_1,local_9fc);
    lVar19 = (long)dVar27;
    lVar21 = (long)(dVar27 * 1000.0);
    lVar20 = lVar20 - lVar22;
    lVar22 = lVar20;
    if (lVar20 < 0) {
      lVar22 = lVar20 + 1;
    }
    lVar22 = lVar22 >> 1;
    if (iVar3 == 0) {
      lVar22 = lVar20;
    }
    pcVar1 = "OFX-CPU";
    if (local_9e8 != &DAT_0156a548) {
      pcVar1 = "OFX-GPU";
    }
    FUN_001bc07c(pcVar1,0xffffffffffffffff,lVar19,lVar21,auStack_188,0x100);
    lVar20 = FUN_001bf048(lVar6,auStack_188);
    lVar10 = FUN_001bfc28(local_9fc,uVar9);
    iVar3 = local_9fc;
    if ((local_9fc == 0) && (DAT_0141a1cd != '\0')) {
      if (*(int *)(lVar20 + 0xc2b0) == 0) {
        iVar3 = 0;
      }
      else {
        iVar3 = 3;
        local_9fc = 3;
      }
    }
    if (DAT_0155c430 != 1) {
      FUN_00af4a8c(&DAT_0155c428);
      iVar3 = local_9fc;
    }
    lVar18 = lVar18 - lVar15;
    FUN_001bac30(DAT_0155c440,lVar18,lVar22,iVar3,uVar9 & 0xffffffff);
    pcVar1 = "OFX-CPU";
    if (local_9e8 != &DAT_0156a548) {
      pcVar1 = "OFX-GPU";
    }
    FUN_001bc07c(pcVar1,lVar10,lVar19,lVar21,auStack_188,0x100);
    if (DAT_0156a4b0 != 1) {
      FUN_00af4a8c(&DAT_0156a4a8);
    }
    uVar4 = FUN_009f8054(DAT_0156a4c0);
    uVar5 = (**(code **)*param_1)(param_1,&local_9f0);
    uStack_a48 = _UNK_00c93af8;
    local_a50 = _DAT_00c93af0;
    uStack_a38 = _UNK_00c93b08;
    uStack_a40 = _DAT_00c93b00;
    local_a30 = _DAT_00c93b10;
    uStack_a28._2_6_ = (undefined6)((ulong)_UNK_00c93b18 >> 0x10);
    uStack_a28._0_2_ = CONCAT11((char)uVar5,(char)_UNK_00c93b18);
    iVar3 = FUN_001beac0(lVar6);
    if (iVar3 == 0) {
      FUN_001bf1e0(lVar6,lVar10,lVar19,lVar18,lVar22,uVar7 & 0xffffffff);
      local_390[3] = 0;
      local_390[2] = 0;
      local_390[1] = 0;
      local_390[0] = 0;
      if (DAT_0141a1d3 == '\0') {
        pcVar14 = *(code **)(DAT_0155b1e8 + 0x28);
LAB_00008690:
        puVar13 = (undefined8 *)0x0;
      }
      else {
        (**(code **)(DAT_0155b1e8 + 0x38))(local_9b8,local_9d0,local_390);
        pcVar14 = *(code **)(DAT_0155b1e8 + 0x28);
        if (DAT_0141a1d3 == '\0') goto LAB_00008690;
        puVar13 = local_390;
      }
      iVar3 = (*pcVar14)(local_9b8,local_9d0,puVar13,&local_9d8);
      if (((iVar3 == 0) && (local_9d8 != 0)) && (local_9c8 != 0)) {
        (**(code **)(*param_1 + 0x30))(param_1,local_9d8,local_9c8,&local_9f0);
        if (local_9e8 != &DAT_0156a548) goto LAB_00008ab4;
        goto LAB_00008adc;
      }
LAB_00008abc:
      uVar8 = 1;
    }
    else {
      if (*(char *)(lVar20 + 0xd5f5) == '\0') {
        lVar15 = 0;
      }
      else {
        lVar15 = (long)*(int *)(lVar20 + 0xd5f8);
      }
      local_a60 = *(undefined4 *)(lVar20 + 0xc2bc);
      local_a5c = *(undefined4 *)(lVar20 + 0xc2f4);
      local_a78 = 0;
      local_a68 = local_9fc;
      local_a58 = 0xffffffffffffffff;
      local_a88 = lVar18;
      local_a80 = lVar22;
      local_a70 = (int)uVar7;
      local_a6c = uVar2;
      local_a64 = (int)uVar9;
      lVar20 = FUN_001bf474(lVar15);
      if (0 < lVar20) {
        lVar23 = -lVar15;
        pdVar16 = local_590;
        puVar13 = local_390;
        do {
          uVar8 = (**(code **)(*param_1 + 0x60))(param_1,param_2,lVar21,lVar23,1000);
          *puVar13 = uVar8;
          dVar25 = local_9b8 + (double)lVar23;
          dVar27 = local_a08;
          if (dVar25 <= local_a08) {
            dVar27 = dVar25;
          }
          dVar26 = local_a10;
          if (local_a10 <= dVar25) {
            dVar26 = dVar27;
          }
          *pdVar16 = dVar26;
          lVar23 = lVar23 + 1;
          lVar20 = lVar20 + -1;
          pdVar16 = pdVar16 + 1;
          puVar13 = puVar13 + 1;
        } while (lVar20 != 0);
      }
      uVar8 = local_390[lVar15];
      local_ce8 = &local_9f0;
      plStack_ce0 = &local_a88;
      uStack_cd0 = local_9d0;
      local_cc8 = local_9b8;
      local_cd8 = param_1;
      lStack_cc0 = lVar18;
      local_cb8 = lVar22;
      lStack_cb0 = lVar6;
      local_ca8 = lVar10;
      lStack_ca0 = lVar19;
      local_c98 = lVar21;
      _memcpy(auStack_c90,local_590,0x200);
      local_598 = (long *)0x0;
      local_a90 = lVar15;
      plVar11 = (long *)__Znwm(0x268);
      *plVar11 = (long)&DAT_013b4e98;
      _memcpy(plVar11 + 1,&local_ce8,0x260);
      uVar2 = 3;
      if (DAT_0141a1dc != 1) {
        uVar2 = 1;
      }
      local_598 = plVar11;
      uVar12 = FUN_001bfc40(lVar6,lVar10,&local_a88,uVar8,1,alStack_5b0,uVar2);
      FUN_001bf244(lVar6,lVar10,lVar19,&local_a88);
      FUN_001bf484(lVar6,lVar10,&local_a88,uVar8,1000,1,lVar15,&local_a50,auStack_d20);
      if (DAT_0155c408 != 1) {
        FUN_00af4a8c(&DAT_0155c400);
      }
      FUN_001c0bd0(DAT_0155c418,&local_a50);
      lVar22 = lVar6 + lVar10 * 8;
      uVar24 = *(undefined8 *)(lVar22 + 0xe280);
      uVar17 = *(undefined8 *)(lVar22 + 0xe2a0);
      FUN_001bb83c(uVar24,&local_a88,local_390,1,local_d08,local_cf8,uVar8,uVar4,local_7b0);
      FUN_001bb83c(uVar17,&local_a88,local_390 + lVar15,1,local_d00,local_cf0,uVar8,uVar4 | uVar5,
                   local_9b0);
      FUN_001bf658(lVar6,lVar10,local_7b0,local_9b0,auStack_d20,lVar19);
      if (0 < local_d08) {
        lVar22 = 0;
        do {
          uVar9 = FUN_001bc3ec(uVar24,local_7b0[lVar22]);
          if ((uVar9 & 1) != 0) {
            uVar9 = (**(code **)(*param_1 + 8))
                              (param_1,local_9d0,local_7b0[lVar22] + 0x50,&local_a88,&local_9f0,
                               lVar6 + 0x254b8,lVar10,lVar19,lVar21,(long)local_590[lVar22]);
            if ((uVar9 & 1) == 0) {
              FUN_009c4318(local_7b0[lVar22] + 0x50,1);
              FUN_001bc6d8(uVar24,local_7b0[lVar22]);
            }
            FUN_001bf298(lVar6,local_7b0[lVar22] + 0x50,lVar10,lVar19,lVar22 - lVar15,lVar15);
            FUN_001bc5e8(uVar24,local_7b0[lVar22]);
          }
          lVar22 = lVar22 + 1;
        } while (lVar22 < local_d08);
      }
      if (local_9e8 == &DAT_0156a548) {
        (**(code **)(*param_1 + 0x20))(param_1,local_9c8,(int)uVar7);
      }
      FUN_001bc444(uVar17,local_9b0[0]);
      uVar8 = FUN_009c610c(local_7b0[lVar15] + 0x50);
      FUN_001bff78(lVar6,lVar10,uVar12,uVar8);
      lVar22 = local_9b0[0];
      local_d28 = 0;
      if (uVar5 != 0) {
        (**(code **)(DAT_0155b1e8 + 0x28))(local_9b8,local_9d0,0,&local_d28);
      }
      (**(code **)(*param_1 + 0x28))
                (param_1,local_9c8,lVar22 + 0x50,local_7b0[lVar15] + 0x50,&local_9f0,local_9b0[0],
                 local_d28);
      if (local_d28 != 0) {
        (**(code **)(DAT_0155b1e8 + 0x30))();
      }
      FUN_001bf9e8(lVar6,lVar22 + 0x50);
      FUN_001bc4b8(uVar24,local_7b0,local_d08);
      FUN_001bc4b8(uVar17,local_9b0,local_d00);
      if (alStack_5b0 == local_598) {
        (**(code **)(*local_598 + 0x20))();
      }
      else if (local_598 != (long *)0x0) {
        (**(code **)(*local_598 + 0x28))();
      }
      if (local_9e8 != &DAT_0156a548) {
LAB_00008ab4:
        if (local_9e8[0x140] != '\0') goto LAB_00008abc;
      }
LAB_00008adc:
      uVar8 = 0;
    }
    FUN_00a06c24(&local_9f0);
    FUN_001bf29c(lVar6,auStack_188);
  }
  else {
    uVar8 = 1;
  }
  local_9f0 = &DAT_013ff668;
  FUN_00a06c24(&local_9f0);
  local_9f0 = &DAT_013ff6a0;
  FUN_00a0696c(&local_9f0);
joined_r0x000083e4:
  if (local_9d8 != 0) {
    (**(code **)(DAT_0155b1e8 + 0x30))();
  }
  if (local_9c8 != 0) {
    (**(code **)(DAT_0155b1e8 + 0x30))();
  }
  if (DAT_0155c408 != 1) {
    FUN_00af4a8c(&DAT_0155c400);
  }
  FUN_001c087c(DAT_0155c418,lVar6);
  if (*(long *)PTR____stack_chk_guard_013b4320 != local_88) {
                    /* WARNING: Subroutine does not return */
    ___stack_chk_fail();
  }
  return uVar8;
}


