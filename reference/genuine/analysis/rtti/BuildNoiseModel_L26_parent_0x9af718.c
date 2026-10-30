// FUN_009af718 @ 009af718 size=2596 tag=BuildNoiseModel_L26_parent

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_009af718(undefined8 param_1,undefined8 param_2,int param_3)

{
  undefined8 uVar1;
  undefined8 uVar2;
  undefined8 uVar3;
  undefined8 uVar4;
  bool bVar5;
  undefined8 *puVar6;
  undefined8 uVar7;
  ulong uVar8;
  int iVar9;
  long lVar10;
  long lVar11;
  int iVar12;
  uint uVar13;
  uint uVar14;
  undefined8 *puVar15;
  long lVar16;
  long lVar17;
  undefined1 auVar18 [16];
  float local_c328;
  float local_c324;
  float local_c320;
  undefined4 local_c31c;
  undefined8 local_c318;
  undefined *local_c310;
  undefined8 *puStack_c308;
  undefined8 *local_c300;
  long *plStack_c2f8;
  long *local_c2f0;
  undefined1 *puStack_c2e8;
  undefined1 *local_c2e0;
  undefined *local_c2d8;
  undefined8 *puStack_c2d0;
  undefined8 *local_c2c8;
  long *plStack_c2c0;
  long *local_c2b8;
  undefined1 *puStack_c2b0;
  undefined1 *local_c2a8;
  undefined *local_c2a0;
  undefined8 *puStack_c298;
  undefined8 *local_c290;
  long *plStack_c288;
  long *local_c280;
  undefined1 *puStack_c278;
  undefined1 *local_c270;
  undefined *local_c268;
  undefined8 *puStack_c260;
  undefined8 *local_c258;
  long *plStack_c250;
  long *local_c248;
  undefined1 *puStack_c240;
  undefined1 *local_c238;
  undefined *local_c230;
  undefined8 *puStack_c228;
  undefined8 *local_c220;
  long *plStack_c218;
  long *local_c210;
  undefined1 *puStack_c208;
  undefined1 *local_c200;
  undefined *local_c1f8;
  undefined8 *puStack_c1f0;
  undefined8 *local_c1e8;
  long *plStack_c1e0;
  long *local_c1d8;
  undefined1 *puStack_c1d0;
  undefined1 *local_c1c8;
  undefined8 local_c1c0;
  undefined8 local_c1b8;
  undefined8 local_c1b0;
  undefined8 local_c1a8;
  undefined8 local_c1a0;
  undefined8 local_c198;
  undefined8 local_c190;
  long local_c188;
  long lStack_c180;
  undefined1 auStack_c178 [8024];
  undefined1 auStack_a220 [8020];
  undefined1 auStack_82cc [32080];
  undefined1 auStack_57c [204];
  undefined1 auStack_4b0 [204];
  undefined1 auStack_3e4 [204];
  undefined1 auStack_318 [128];
  undefined4 local_298;
  undefined4 uStack_294;
  undefined4 local_290;
  undefined1 local_284;
  undefined1 local_283;
  undefined1 local_282;
  undefined **local_280;
  undefined **ppuStack_278;
  undefined **local_270;
  undefined **ppuStack_268;
  undefined **local_260;
  undefined **ppuStack_258;
  undefined1 auStack_250 [80];
  undefined1 auStack_200 [80];
  undefined1 auStack_1b0 [80];
  undefined1 auStack_160 [80];
  undefined1 auStack_110 [80];
  undefined1 auStack_c0 [80];
  long local_70;
  
  auVar18 = (*(code *)PTR____chkstk_darwin_013b4308)();
  lVar10 = auVar18._8_8_;
  puVar6 = auVar18._0_8_;
  local_70 = *(long *)PTR____stack_chk_guard_013b4320;
  _memcpy(auStack_c178,(void *)(puVar6[0x7f2] + 0x48),0xbef8);
  uVar14 = *(uint *)(DAT_01488c50 + 0x1535c78);
  uVar13 = (uVar14 ^ uVar14 >> 1) & 0x40154445;
  uVar14 = (uVar13 | uVar13 << 1) ^ uVar14;
  iVar12 = (int)(CONCAT44((uVar14 + (uVar14 >> 0x10)) * 0x101 + (uVar14 >> 0x18),uVar14) >> 0x10);
  if (iVar12 != 1) {
    *(uint *)(&UNK_01134d08 + DAT_01488c58) = uVar14;
  }
  *(undefined1 *)((long)puVar6 + 0x81) = 0;
  uVar13 = *(uint *)(DAT_014885c0 + 0x256f28);
  iVar9 = *(int *)(DAT_014885d0 + 0x161f9f8);
  if (iVar9 != 1) {
    if (DAT_014881dc != 0) {
      uVar13 = uVar13 >> 0x1d | uVar13 << 3;
    }
    *(uint *)(DAT_014885c8 + 0x8fa5f8) = uVar13;
  }
  DAT_014881dc = iVar9;
  FUN_00a22470(puVar6 + 1);
  puVar15 = puVar6 + 0xe;
  *puVar15 = 0;
  puVar6[0xf] = 0;
  uVar7 = FUN_00a22934(0x3f800000,puVar6 + 1,10);
  *puVar15 = uVar7;
  lVar17 = *(long *)(lVar10 + 0xd0);
  lVar16 = *(long *)(lVar10 + 0xd8);
  local_c190 = 0;
  if (iVar12 == 0) {
    iVar12 = 0;
  }
  else {
    iVar12 = *(int *)(DAT_01488c68 + 0x585658);
  }
  local_c188 = lVar16;
  lStack_c180 = lVar17;
  thunk_FUN_009d7bc4(auStack_250);
  thunk_FUN_009d7bc4();
  thunk_FUN_009d7bc4();
  thunk_FUN_009d7bc4(auStack_160);
  thunk_FUN_009d7bc4(auStack_110);
  thunk_FUN_009d7bc4(auStack_c0);
  FUN_0098718c(lVar10,auStack_318,auStack_250);
  lVar16 = lVar16 * lVar17;
  lVar17 = lVar16 * 4;
  local_c198 = FUN_009d7200(lVar17,0xb45a83cf);
  local_c1a0 = FUN_009d7200(lVar17,0x98578dc4);
  local_c1a8 = FUN_009d7200(lVar17,0xa86483bf);
  local_c1b0 = FUN_009d7200(lVar17,0xc85d8fc5);
  local_c1b8 = FUN_009d7200(lVar17,0x9f5583f2);
  local_c1c0 = FUN_009d7200(lVar17,0x415c8cc1);
  FUN_009c8c24(lVar10,local_c198,local_c1a0,local_c1a8,1);
  (**(code **)(&DAT_0147df50 + (ulong)*(uint *)(puVar6[0x7f2] + 0x48) * 8))
            (local_c198,local_c1a0,local_c1a8,local_c1b0,local_c1b8,local_c1c0,lVar16);
  uVar14 = *(uint *)(&UNK_000bc45c + DAT_01488c70);
  uVar13 = (uVar14 ^ uVar14 >> 1) & 0x40004551;
  uVar14 = (uVar13 | uVar13 << 1) ^ uVar14;
  if (iVar12 != 1) {
    *(uint *)(DAT_01488c78 + 0x1e76d74) = uVar14;
  }
  DAT_01488308 = uVar14;
  FUN_00a22868(*puVar15,4);
  puStack_c308 = &local_c190;
  local_c1f8 = &DAT_013fd1e0;
  local_c1e8 = &local_c198;
  plStack_c2f8 = &lStack_c180;
  local_c2f0 = &local_c188;
  puStack_c2e8 = auStack_c178;
  local_c2e0 = auStack_250;
  local_c230 = &DAT_013fd210;
  local_c220 = &local_c1a0;
  local_c268 = &DAT_013fd240;
  local_c258 = &local_c1a8;
  local_c2a0 = &DAT_013fd270;
  local_c290 = &local_c1b0;
  local_c2d8 = &DAT_013fd2a0;
  local_c2c8 = &local_c1b8;
  local_c310 = &DAT_013fd2d0;
  local_c300 = &local_c1c0;
  local_280 = &local_c1f8;
  ppuStack_278 = &local_c230;
  local_270 = &local_c268;
  ppuStack_268 = &local_c2a0;
  local_260 = &local_c2d8;
  ppuStack_258 = &local_c310;
  puStack_c2d0 = puStack_c308;
  plStack_c2c0 = plStack_c2f8;
  local_c2b8 = local_c2f0;
  puStack_c2b0 = puStack_c2e8;
  local_c2a8 = local_c2e0;
  puStack_c298 = puStack_c308;
  plStack_c288 = plStack_c2f8;
  local_c280 = local_c2f0;
  puStack_c278 = puStack_c2e8;
  local_c270 = local_c2e0;
  puStack_c260 = puStack_c308;
  plStack_c250 = plStack_c2f8;
  local_c248 = local_c2f0;
  puStack_c240 = puStack_c2e8;
  local_c238 = local_c2e0;
  puStack_c228 = puStack_c308;
  plStack_c218 = plStack_c2f8;
  local_c210 = local_c2f0;
  puStack_c208 = puStack_c2e8;
  local_c200 = local_c2e0;
  puStack_c1f0 = puStack_c308;
  plStack_c1e0 = plStack_c2f8;
  local_c1d8 = local_c2f0;
  puStack_c1d0 = puStack_c2e8;
  local_c1c8 = local_c2e0;
  FUN_009fc3a8(&local_280,6,DAT_014a67c0,0,0);
  uVar8 = (**(code **)*puVar6)(puVar6);
  if ((uVar8 & 1) != 0) {
    uVar7 = 0;
    goto LAB_009b0000;
  }
  FUN_00a22868(*puVar15,4);
  if (uVar14 == 0) {
    uVar13 = 0;
  }
  else {
    uVar13 = *(uint *)(&UNK_00cb74e8 + DAT_01488c80);
    if (DAT_0148830c != 0) {
      uVar13 = uVar13 >> 0xb | uVar13 << 0x15;
    }
    DAT_0148830c = uVar13;
    if (uVar13 != 1) {
      *(uint *)(DAT_01488c88 + 0x1945db8) = uVar13;
    }
  }
  local_c318 = 0;
  local_c31c = 0;
  FUN_009c4a4c(lVar10,(long)&local_c318 + 4,&local_c318,&local_c31c,1);
  local_298 = local_c318._4_4_;
  uStack_294 = (undefined4)local_c318;
  local_290 = local_c31c;
  FUN_009c6648(lVar10,&local_c320,&local_c324,&local_c328,1);
  local_284 = 0.33 < local_c320;
  local_283 = 0.33 < local_c324;
  local_282 = 0.33 < local_c328;
  uVar14 = 0;
  if (uVar13 != 0) {
    uVar14 = *(uint *)(DAT_01488c90 + 0xc42c58);
    uVar13 = uVar14 >> 0xd | uVar14 << 0x13;
    bVar5 = DAT_01488310 != 0;
    DAT_01488310 = uVar14;
    if (bVar5) {
      uVar14 = uVar13;
      DAT_01488310 = uVar13;
    }
  }
  FUN_009b2f34(puVar6,local_c198,*(undefined8 *)(lVar10 + 0xd0),*(undefined8 *)(lVar10 + 0xd8),
               (ulong)auStack_c178 | 4,auStack_57c,1,&local_c190,auStack_250);
  FUN_009b2f34(puVar6,local_c1a0,*(undefined8 *)(lVar10 + 0xd0),*(undefined8 *)(lVar10 + 0xd8),
               auStack_a220,auStack_4b0,1,&local_c190,auStack_250);
  FUN_009b2f34(puVar6,local_c1a8,*(undefined8 *)(lVar10 + 0xd0),*(undefined8 *)(lVar10 + 0xd8),
               auStack_82cc,auStack_3e4,1,&local_c190,auStack_250);
  FUN_00a22868(*puVar15,2);
  _memcpy((void *)(puVar6[0x7f2] + 0x48),auStack_c178,0xbef8);
  lVar17 = puVar6[0x7f2];
  *(undefined1 *)(lVar17 + 0xbf34) = 1;
  iVar12 = 0;
  if (uVar14 != 0) {
    uVar13 = *(uint *)(&PNG_01205e39.field_0x20f + DAT_01488c98);
    uVar14 = *(uint *)(DAT_01488ca8 + 0x1731c78);
    iVar12 = (int)(CONCAT44((uVar14 + (uVar14 >> 0x10)) * 0x101 + (uVar14 >> 0x18),uVar14) >> 0x10);
    if (iVar12 != 1) {
      uVar14 = (uVar13 ^ uVar13 >> 1) & 0x40111055;
      *(uint *)(&UNK_01259e18 + DAT_01488ca0) = (uVar14 | uVar14 << 1) ^ uVar13;
    }
  }
  if (param_3 != 0) {
    _memset_pattern16(&local_280,&DAT_00c962e0,0x14);
    if (iVar12 != 0) {
      uVar13 = *(uint *)(DAT_01488cb0 + 0x183dcd8);
      if (DAT_01488314 != 0) {
        uVar13 = uVar13 >> 0x1d | uVar13 << 3;
      }
      DAT_01488314 = (int)(CONCAT44((uVar13 + (uVar13 >> 0x10)) * 0x101 + (uVar13 >> 0x18),uVar13)
                          >> 0x10);
      if (DAT_01488314 != 1) {
        *(uint *)(DAT_01488cb8 + 0x687e28) = uVar13;
      }
      if (DAT_01566a50 != 0) {
        DAT_01488314 = DAT_01488314 + (uint)*(byte *)(DAT_01566a50 + 0x7c6d);
      }
    }
    FUN_009b3c84(lVar17 + 0xbf40);
    FUN_009b3e78(0x3f800000,0x3f800000,0x3f800000,local_c318._4_4_,(undefined4)local_c318,local_c31c
                 ,puVar6[0x7f2] + 0xbf40,&local_280,&local_280,&local_280,local_284,local_283,
                 local_282);
    lVar17 = puVar6[0x7f2];
  }
  FUN_009b41c4(lVar17,lVar10);
  uVar13 = *(uint *)(&UNK_001039e0 + DAT_01489458);
  uVar14 = *(uint *)(&UNK_00031988 + DAT_01489468);
  iVar12 = (int)(CONCAT44((uVar14 + (uVar14 >> 0x10)) * 0x101 + (uVar14 >> 0x18),uVar14) >> 0x10);
  if (iVar12 != 0) {
    if (DAT_01488474 != 0) {
      uVar13 = uVar13 >> 0x1c | uVar13 << 4;
    }
    *(uint *)(DAT_01489460 + 0x85b510) = uVar13;
  }
  lVar17 = local_c188;
  if (lStack_c180 <= local_c188) {
    lVar17 = lStack_c180;
  }
  if (lVar17 < DAT_01488128) {
    iVar9 = 0;
  }
  else {
    if (lVar17 < DAT_01488130) {
      lVar11 = 1;
      lVar16 = DAT_01488130;
    }
    else if (lVar17 < DAT_01488138) {
      lVar11 = 2;
      lVar16 = DAT_01488138;
    }
    else if (lVar17 < DAT_01488140) {
      lVar11 = 3;
      lVar16 = DAT_01488140;
    }
    else {
      if (DAT_01488148 <= lVar17) {
        iVar9 = 5;
        goto LAB_009affe0;
      }
      lVar11 = 4;
      lVar16 = DAT_01488148;
    }
    iVar9 = (int)((float)(lVar17 - (&DAT_01488120)[lVar11]) /
                  (float)(lVar16 - (&DAT_01488120)[lVar11]) + (float)lVar11);
  }
LAB_009affe0:
  lVar17 = puVar6[0x7f2];
  DAT_01488474 = iVar12;
  *(int *)(lVar17 + 0xbf38) = iVar9;
  *(ulong *)(lVar17 + 0xc2c0) =
       CONCAT44((int)*(undefined8 *)(lVar10 + 0xd8),(int)*(undefined8 *)(lVar10 + 0xd0));
  uVar7 = 1;
LAB_009b0000:
  uVar13 = *(uint *)(DAT_01488cc0 + 0x4731b0);
  if (*(int *)(DAT_01488cd0 + 0x1bd2bc8) != 0) {
    uVar14 = (uVar13 ^ uVar13 >> 1) & 0x40104550;
    *(uint *)(DAT_01488cc8 + 0x181d660) = (uVar14 | uVar14 << 1) ^ uVar13;
  }
  FUN_009d7910(local_c1b0);
  FUN_009d7910(local_c1b8);
  FUN_009d7910(local_c1c0);
  FUN_009d7910(local_c198);
  FUN_009d7910(local_c1a0);
  FUN_009d7910(local_c1a8);
  uVar4 = _DAT_012453d0;
  uVar3 = _DAT_012453c8;
  uVar2 = _DAT_012453c0;
  uVar1 = _DAT_012453b8;
  lVar10 = puVar6[0x7f2];
  *(undefined8 *)(lVar10 + 0xc2e8) = DAT_012453d8;
  *(undefined8 *)(FUN_0000c2cc + lVar10 + 4) = uVar2;
  *(undefined8 *)(lVar10 + 0xc2c8) = uVar1;
  *(undefined8 *)(lVar10 + 0xc2e0) = uVar4;
  *(undefined8 *)(lVar10 + 0xc2d8) = uVar3;
  lVar10 = puVar6[0x7f2];
  *(undefined4 *)(lVar10 + 0xc2f0) = 0;
  *(undefined1 *)(lVar10 + 0x2d) = 0;
  thunk_FUN_009d841c(auStack_c0);
  thunk_FUN_009d841c(auStack_110);
  thunk_FUN_009d841c(auStack_160);
  thunk_FUN_009d841c(auStack_1b0);
  thunk_FUN_009d841c(auStack_200);
  thunk_FUN_009d841c(auStack_250);
  if (*(long *)PTR____stack_chk_guard_013b4320 == local_70) {
    return uVar7;
  }
                    /* WARNING: Subroutine does not return */
  ___stack_chk_fail();
}


