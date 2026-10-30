// FUN_009a17b8 @ 009a17b8 size=7644 tag=astfqg16FilterVideoField_L7_parent

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_009a17b8(undefined8 *param_1,long *param_2,long param_3,long param_4,undefined8 param_5)

{
  undefined8 *puVar1;
  undefined8 *puVar2;
  byte *pbVar3;
  long *plVar4;
  long *plVar5;
  undefined8 *puVar6;
  undefined8 *puVar7;
  long *plVar8;
  undefined8 *puVar9;
  undefined8 *puVar10;
  undefined8 *puVar11;
  undefined8 *puVar12;
  undefined8 *puVar13;
  undefined8 *puVar14;
  undefined4 uVar15;
  ulong uVar16;
  uint uVar17;
  undefined4 uVar18;
  byte bVar19;
  byte bVar20;
  bool bVar21;
  bool bVar22;
  bool bVar23;
  bool bVar24;
  long lVar25;
  undefined8 uVar26;
  byte bVar27;
  byte bVar28;
  byte bVar29;
  byte bVar30;
  byte bVar31;
  byte bVar32;
  byte *pbVar33;
  bool bVar34;
  bool bVar35;
  bool bVar36;
  bool bVar37;
  bool bVar38;
  int iVar39;
  int iVar40;
  uint uVar41;
  int iVar42;
  uint uVar43;
  ulong uVar44;
  long lVar45;
  undefined8 uVar46;
  ulong uVar47;
  undefined8 uVar48;
  undefined8 uVar49;
  undefined8 *puVar50;
  long lVar51;
  undefined2 uVar52;
  uint uVar53;
  ulong uVar54;
  long lVar55;
  long lVar56;
  ulong uVar57;
  long lVar58;
  long lVar59;
  ulong uVar60;
  long *plVar61;
  long lVar62;
  long lVar63;
  char cVar64;
  undefined8 *puVar65;
  long lVar66;
  ulong uVar67;
  long *plVar68;
  ulong uVar69;
  ulong uVar70;
  long lVar71;
  long *plVar72;
  undefined2 uVar73;
  float fVar74;
  double dVar75;
  undefined8 uVar76;
  float fVar77;
  undefined8 uVar78;
  undefined8 uVar79;
  undefined8 uVar80;
  float fVar81;
  undefined8 uVar82;
  int iVar83;
  float fVar84;
  float fVar85;
  byte *in_stack_fffffffffffffa30;
  ulong uVar86;
  float local_568;
  long local_4a8;
  uint local_46c;
  long local_390;
  byte local_372;
  byte local_371;
  undefined *local_370;
  undefined8 *local_368;
  long *local_360;
  long **local_358;
  long *local_350;
  long *local_348;
  uint *local_340;
  undefined8 *local_338;
  byte local_32a;
  byte local_329;
  long local_328;
  long local_320;
  byte local_312;
  byte local_311;
  long local_310;
  undefined8 local_308;
  undefined1 local_300;
  undefined1 local_2ff;
  undefined2 local_2fe;
  undefined4 local_2fc;
  undefined2 local_2f8;
  undefined1 local_2f6;
  uint local_2f0;
  undefined1 local_2ec;
  long local_2e8;
  long local_2e0;
  undefined8 uStack_2d8;
  undefined8 uStack_2d0;
  undefined8 uStack_2c8;
  undefined8 local_2c0;
  undefined8 local_2b8;
  long local_2b0;
  long *local_2a8;
  uint local_2a0;
  undefined1 local_29c;
  undefined2 local_298;
  undefined1 local_296;
  long local_290;
  undefined8 local_288;
  undefined2 local_280;
  undefined2 local_27e;
  undefined4 local_27c;
  ulong local_278;
  byte *local_228 [7];
  undefined8 local_1f0;
  undefined1 local_1e8;
  undefined1 auStack_1e0 [16];
  char local_1d0;
  uint local_1cc;
  char local_1c8;
  undefined4 local_1b0;
  undefined4 uStack_1ac;
  uint local_19c;
  char local_198;
  uint local_188;
  long local_b8;
  
  local_b8 = *(long *)PTR____stack_chk_guard_013b4320;
  lVar51 = *param_2;
  uVar16 = *(ulong *)(lVar51 + 0xd0);
  lVar66 = *(long *)(lVar51 + 0xd8);
  if (*(char *)(param_1[0x7a0] + 0xcd) == '\0') {
    local_390 = 0;
    if (*(int *)(lVar51 + 0xfc) == 2) goto LAB_009a1838;
LAB_009a1850:
    bVar34 = *(int *)(lVar51 + 0xf8) != 2;
  }
  else {
    local_390 = (long)*(int *)(param_1[0x7a0] + 0xd0);
    if (*(int *)(lVar51 + 0xfc) != 2) goto LAB_009a1850;
LAB_009a1838:
    bVar34 = false;
  }
  uVar54 = (ulong)*(uint *)(lVar51 + 0xf0);
  local_2b8 = param_5;
  local_2b0 = param_3;
  local_2a8 = param_2;
  if (((long)uVar16 < 0x30) || (lVar66 < 0x30)) {
    uVar43 = 1;
    FUN_009c3ed8(param_3,lVar51,0,1);
    goto LAB_009a3544;
  }
  uVar53 = *(uint *)(DAT_01488608 + 0x22c8c8);
  iVar39 = FUN_009c610c(lVar51);
  plVar4 = param_1 + 0x3d4a;
  plVar5 = param_1 + 0x2298;
  uVar43 = (uVar53 ^ uVar53 >> 1) & 0x45011441;
  uVar53 = (uVar43 | uVar43 << 1) ^ uVar53;
  DAT_014881f0 = DAT_014881f0 + iVar39;
  if (param_4 == 0x7fffffffffffffff || !bVar34) {
    bVar35 = true;
  }
  else {
    bVar35 = false;
    lVar51 = param_1[0x2299];
    if (((lVar51 != param_4) && (lVar51 + -4 <= param_4)) && (param_4 <= lVar51 + 4)) {
      if (uVar16 != param_1[0x4884]) {
        bVar35 = false;
        param_1[0x2299] = param_4;
        goto joined_r0x009a3448;
      }
      bVar35 = lVar66 == param_1[0x4885];
    }
    param_1[0x2299] = param_4;
  }
joined_r0x009a3448:
  if ((int)(CONCAT44((uVar53 + (uVar53 >> 0x10)) * 0x101 + (uVar53 >> 0x18),uVar53) >> 0x10) != 0) {
    uVar43 = *(uint *)(DAT_01488610 + 0x1b0bb08);
    DAT_014881f8 = *(int *)(DAT_01488620 + 0x1a3cbd8);
    if (DAT_014881f8 != 1) {
      uVar53 = (uVar43 ^ uVar43 >> 1) & 0x45050511;
      *(uint *)(&UNK_0011ff18 + DAT_01488618) = (uVar53 | uVar53 << 1) ^ uVar43;
    }
  }
  lVar51 = FUN_009d3278(param_1[0x7a0],0xb0c,0);
  if ((uVar16 == param_1[0x4884]) && (lVar66 == param_1[0x4885])) {
    bVar36 = param_1[0x4886] != uVar54;
  }
  else {
    bVar36 = true;
  }
  iVar39 = *(int *)(param_1[0x7a0] + 0xe8);
  uVar43 = *(uint *)(DAT_01488628 + 0x1ad16b0);
  uVar53 = *(uint *)(DAT_01488638 + 0xaa7f68);
  iVar83 = (int)(CONCAT44((uVar53 + (uVar53 >> 0x10)) * 0x101 + (uVar53 >> 0x18),uVar53) >> 0x10);
  if (iVar83 != 0) {
    if (DAT_014881fc != 0) {
      uVar43 = uVar43 >> 10 | uVar43 << 0x16;
    }
    *(uint *)(DAT_01488630 + 0xa59720) = uVar43;
  }
  DAT_014881fc = iVar83;
  uVar44 = (**(code **)*param_1)(param_1);
  if ((uVar44 & 1) != 0) {
    uVar43 = 0;
    goto LAB_009a3544;
  }
  uVar43 = (DAT_014881f4 + DAT_014881f0) * 0x10001 + (DAT_014881f4 >> 0x10);
  DAT_014881f0 = DAT_014881f4;
  uStack_2d8 = param_1[0x2299];
  local_2e0 = *plVar5;
  uStack_2c8 = param_1[0x229b];
  uStack_2d0 = param_1[0x229a];
  local_2c0 = param_1[0x229c];
  lVar55 = param_1[0x7a0];
  cVar64 = *(char *)(lVar55 + 0xd8);
  uVar41 = *(uint *)(DAT_01488ea0 + 0x6af200);
  uVar53 = (uVar41 ^ uVar41 >> 1) & 0x44401100;
  uVar41 = (uVar53 | uVar53 << 1) ^ uVar41;
  DAT_014881f4 = uVar43;
  if (uVar41 != 0) {
    *(uint *)(DAT_01488ea8 + 0x1fb3ff0) = uVar41;
  }
  fVar81 = (float)(long)(lVar66 * uVar16);
  lVar59 = (long)(((fVar81 * 0.2) / (float)DAT_014a67c0) * 0.5 * 3.5) >> 10;
  if (lVar59 < 2) {
    lVar59 = 1;
  }
  iVar83 = *(int *)(lVar55 + 0xd0);
  if (DAT_0156a4b0 != 1) {
    FUN_00af4a8c(&DAT_0156a4a8);
  }
  FUN_009f7fb8(&local_1b0,DAT_0156a4c0,0);
  dVar75 = (double)(long)iVar83 * DAT_0127a7a0;
  lVar55 = 0;
  if ((local_19c & 0x10000) != 0) {
    lVar55 = CONCAT44(uStack_1ac,local_1b0);
  }
  lVar55 = (long)((fVar81 * (float)(dVar75 + dVar75)) /
                 (float)(lVar55 + DAT_014a67c8 *
                                  ((long)local_198 + (long)(int)-(local_19c >> 0x10 & 1)) * 2)) >>
           10;
  if (lVar55 < 2) {
    lVar55 = 1;
  }
  iVar83 = *(int *)(param_1[0x7a0] + 0xd0);
  if (DAT_0156a4b0 != 1) {
    FUN_00af4a8c(&DAT_0156a4a8);
  }
  FUN_009f7fb8(&local_1b0,DAT_0156a4c0,0);
  fVar81 = (float)iVar83 * 0.1;
  lVar56 = 0;
  if ((local_19c & 0x10000) != 0) {
    lVar56 = CONCAT44(uStack_1ac,local_1b0);
  }
  lVar56 = (long)(((fVar81 + fVar81) * (float)(long)uVar16 * (float)lVar66) /
                 (float)(lVar56 + ((long)local_198 + (long)(int)-(local_19c >> 0x10 & 1)) *
                                  DAT_014a67c8)) >> 10;
  if (lVar56 < 2) {
    lVar56 = 1;
  }
  lVar45 = FUN_009a3694(param_1,uVar16,lVar66,*(undefined1 *)(*local_2a8 + 0xf4),0);
  uVar26 = _UNK_01244848;
  lVar25 = _DAT_01244840;
  uVar43 = 0;
  bVar21 = false;
  lVar71 = 0;
  bVar24 = false;
  uVar53 = 0;
  local_4a8 = 0;
  bVar38 = false;
  bVar22 = (bool)(bVar34 & iVar39 == 1);
  bVar23 = (bool)(!bVar35 & bVar34 & cVar64 != '\0');
  lVar62 = lVar59;
  if (!bVar23) {
    lVar62 = 0;
  }
  fVar81 = (float)(lVar56 + lVar45 + lVar62 + lVar55);
  puVar6 = param_1 + 0x4b3a;
  puVar7 = param_1 + 0x4afa;
  plVar8 = param_1 + 0x4883;
  puVar9 = param_1 + 0x3d53;
  local_568 = ((float)bVar23 * (float)lVar59) / fVar81;
  puVar10 = param_1 + 0x4b23;
  puVar11 = param_1 + 0x4889;
  puVar12 = param_1 + 0x47f9;
  fVar77 = (float)(lVar56 + lVar45 + lVar59);
  puVar13 = param_1 + 0x4880;
  uVar60 = local_390 * 2;
  uVar82 = NEON_fmov(0x3f800000,4);
  fVar84 = (float)lVar56 / fVar81;
  fVar85 = (float)lVar45 / fVar81;
  puVar1 = param_1 + 1;
  puVar2 = param_1 + 0xe;
  pbVar3 = (byte *)((long)param_1 + 0x25bca);
  puVar14 = param_1 + 0x486c;
  uVar44 = uVar60;
  if ((long)uVar60 < 2) {
    uVar44 = 1;
  }
  uVar57 = uVar44 & 0x7ffffffffffffffe;
  uVar69 = uVar16;
  do {
    local_2e8 = 0;
    FUN_00ac1660(puVar6,&local_2e8);
    param_1[0x229c] = local_2c0;
    param_1[0x2299] = uStack_2d8;
    *plVar5 = local_2e0;
    param_1[0x229b] = uStack_2c8;
    param_1[0x229a] = uStack_2d0;
    uVar41 = *(uint *)(DAT_01488640 + 0x56ed70);
    bVar37 = DAT_01488200 != 0;
    DAT_01488200 = uVar41;
    if (bVar37) {
      DAT_01488200 = uVar41 >> 1 | uVar41 << 0x1f;
    }
    if (DAT_01488200 != 0) {
      *(uint *)(DAT_01488648 + 0x1c817e0) = DAT_01488200;
    }
    uVar41 = *(uint *)(DAT_014885c0 + 0x256f28);
    iVar83 = *(int *)(DAT_014885d0 + 0x161f9f8);
    if (iVar83 != 1) {
      if (DAT_014881dc != 0) {
        uVar41 = uVar41 >> 0x1d | uVar41 << 3;
      }
      *(uint *)(DAT_014885c8 + 0x8fa5f8) = uVar41;
    }
    DAT_014881dc = iVar83;
    FUN_00a22470(puVar1);
    *puVar2 = 0;
    param_1[0xf] = 0;
    local_1b0 = 0;
    FUN_009f13bc(puVar7,&local_1b0,0x70);
    local_1b0 = 0;
    FUN_009f13bc(param_1 + 0x4b08,&local_1b0,0x68);
    local_1b0 = 0;
    FUN_009f13bc(param_1 + 0x4b15,&local_1b0,0x68);
    if ((bVar36) || (lVar51 != param_1[0x4888])) {
      FUN_0099ead0(param_1,0);
      if (0 < local_390) {
        lVar62 = 0;
        do {
          if (puVar14[lVar62] == 0) {
            uVar46 = __Znwm(0x108);
            FUN_001c1114();
            puVar14[lVar62] = uVar46;
          }
          lVar62 = lVar62 + 1;
        } while (lVar62 < (long)uVar60);
      }
      if ((local_390 == 0) || (*plVar8 != 0)) {
        cVar64 = *(char *)(param_1[0x7a0] + 0xd8);
      }
      else {
        lVar62 = __Znwm(0x108);
        FUN_001c1114();
        *plVar8 = lVar62;
        cVar64 = *(char *)(param_1[0x7a0] + 0xd8);
      }
      if (cVar64 == '\0') {
        param_1[0x229a] = uVar82;
        *(undefined4 *)(param_1 + 0x229b) = 0x3f800000;
        param_1[0x2299] = uVar26;
        *plVar5 = lVar25;
        param_1[0x229c] = 0;
        FUN_0099e264(param_1,1);
        lVar62 = param_1[0x486b];
        if (lVar62 != 0) {
          if (*(long *)(lVar62 + -8) != 0) {
            lVar63 = *(long *)(lVar62 + -8) * 0x50;
            do {
              thunk_FUN_009d841c(lVar62 + -0x50 + lVar63);
              lVar63 = lVar63 + -0x50;
            } while (lVar63 != 0);
          }
          __ZdaPv(lVar62 + -0x10);
          param_1[0x486b] = 0;
        }
      }
      param_1[0x4884] = uVar16;
      param_1[0x4885] = lVar66;
      param_1[0x4886] = uVar54;
      param_1[0x4888] = lVar51;
    }
    lVar62 = param_1[0x7a0];
    uVar86 = (ulong)*(uint *)(lVar62 + 0x130);
    if (*(char *)(lVar62 + 0xd9) != '\0' || *(int *)(lVar62 + 0x168) != 0) {
      uVar86 = 1;
    }
    lVar63 = *(long *)(&DAT_012453e0 + uVar86 * 0x10) * 2 + 0x60;
    lVar58 = param_1[0x7f0] + param_1[0x7f1] * 2;
    uVar86 = 0x200000000;
    FUN_00addf74(&local_1b0,param_1,local_2a8 + 1,*local_2a8,*(undefined4 *)(local_2b0 + 0xf0),
                 local_390,iVar39 == 1,0,(ulong)in_stack_fffffffffffffa30 & 0xffffffffffffff00,
                 (ulong)*(uint *)(lVar62 + 0x130),(long)*(int *)(lVar62 + 0x104),
                 (long)((float)(lVar63 * lVar63) * 4.0),(long)((float)(lVar58 * lVar58) * 4.0),
                 0x200000000,param_1[0x3d52],puVar9);
    bVar20 = *pbVar3 | bVar38;
    bVar38 = bVar20 != 0;
    *pbVar3 = 0;
    if (param_1[0x3d4d] != 0) {
      *(undefined1 *)(param_1[0x3d4d] + 0x5b4) = 0;
    }
    uVar46 = FUN_009a3988(param_1[0x7a0]);
    uVar41 = *(uint *)((long)param_1 + 0x25bcc);
    if (DAT_0156a4b0 != 1) {
      FUN_00af4a8c(&DAT_0156a4a8);
    }
    FUN_009f7fb8(auStack_1e0,DAT_0156a4c0,0);
    uVar67 = 0;
    iVar83 = 0;
    local_290 = param_1[0x4b7b];
    local_1f0 = param_1[0x4b7c];
    local_1e8 = *(undefined1 *)(param_1 + 0x4b7d);
    uVar52 = *(undefined2 *)((long)param_1 + 0x25bea);
    uVar41 = uVar41 & 0x1ffff;
    if ((uVar41 == 0) && (local_290 == 0)) {
      if (local_1d0 == '\0') {
        iVar83 = 0;
        local_290 = 0;
        local_2f8 = 0;
        uVar67 = 0;
        local_2f0 = 0;
      }
      else {
        local_2a0 = 0;
        local_29c = 0;
        local_298 = 0;
        local_296 = 0;
        local_290 = 0;
        local_288 = 0;
        local_280 = 0;
        local_27c = 0x3f800000;
        local_278 = 0;
        lVar62 = *local_2a8;
        local_370 = *(undefined **)(lVar62 + 0xd0);
        local_368 = (undefined8 *)(*(long *)(lVar62 + 0xd8) >> (*(int *)(lVar62 + 0xfc) == 3));
        local_358 = (long **)((long)&MACH_HEADER.magic + 1);
        if (*(int *)(lVar62 + 0xfc) != 0) {
          local_358 = (long **)((long)&MACH_HEADER.magic + 2);
        }
        local_360 = (long *)CONCAT44(local_360._4_4_,*(undefined4 *)(lVar62 + 0xf0));
        FUN_00a30708(puVar10,param_1[0x49c1],&local_370,uVar46,&local_2a0,"");
        uVar53 = 0;
        local_1e8 = (undefined1)local_280;
        local_1f0 = local_288;
        iVar83 = 1;
        uVar67 = local_278;
        uVar52 = local_27e;
        local_2f0 = local_2a0 & 0x1ffff;
        local_2f8 = local_298;
      }
    }
    else {
      local_2f8 = 0;
      local_2f0 = uVar41;
    }
    local_2fc = *(undefined4 *)(param_1 + 0x4b22);
    uVar73 = NEON_uaddlv((ulong)CONCAT12(POPCOUNT((char)(local_2f0 >> 0x10)),
                                         CONCAT11(POPCOUNT((char)(local_2f0 >> 8)),
                                                  POPCOUNT((char)local_2f0))),1);
    local_2ec = (undefined1)uVar73;
    uVar73 = NEON_uaddlv((ulong)CONCAT11(POPCOUNT((char)((ushort)local_2f8 >> 8)),
                                         POPCOUNT((char)local_2f8)),1);
    local_2f6 = (undefined1)uVar73;
    local_300 = local_1e8;
    local_308 = local_1f0;
    local_2ff = 1;
    in_stack_fffffffffffffa30 = pbVar3;
    local_310 = local_290;
    local_2fe = uVar52;
    FUN_00a06540(&local_2a0,param_1,&local_2f0,&local_2f8,&local_310,&local_1b0,iVar83,
                 *(undefined1 *)((long)param_1 + 0x25bcb),pbVar3,"");
    FUN_009f7c54(param_1 + 0x4b7f,local_228);
    if (*pbVar3 == 0) {
      uVar47 = (**(code **)*param_1)(param_1);
      pbVar33 = local_228[0];
      if ((uVar47 & 1) == 0) {
        uVar69 = param_1[0x49c1];
        if (iVar83 != 0) {
          iVar40 = FUN_00a314b8(puVar10,&local_2a0);
          if (iVar40 == 0) {
            uVar67 = 0;
          }
          uVar69 = uVar67 | uVar69;
        }
        if ((0 < local_390) && (local_4a8 == 0)) {
          lVar62 = *(long *)(&DAT_01247820 + (ulong)local_188 * 8) * lVar66 * uVar16;
          if ((long)local_1c8 + (long)(int)-(local_1cc >> 0x10 & 1) < 1) {
            local_4a8 = FUN_009d8200(param_5,lVar62,0x519c929f);
          }
          else {
            local_4a8 = FUN_009d8248(param_5,lVar62,0x519c829f);
          }
          lVar71 = lVar71 + 1;
        }
        *(ulong *)(param_1[0x3d4d] + 0x278) = uVar69;
        lVar62 = param_1[0x7f0] + param_1[0x7f1] * 2;
        FUN_00ac2088(puVar11,uVar69,lVar62 * lVar62,uVar16,lVar66,uVar54,uVar46,&local_2a0);
        lVar62 = param_1[0x7a0];
        uVar67 = (ulong)*(uint *)(lVar62 + 0x130);
        if (*(char *)(lVar62 + 0xd9) != '\0' || *(int *)(lVar62 + 0x168) != 0) {
          uVar67 = 1;
        }
        lVar62 = *(long *)(&DAT_012453e0 + uVar67 * 0x10) * 2 + 0x60;
        FUN_00ac2088(param_1 + 0x4925,uVar69,lVar62 * lVar62,uVar16,lVar66,uVar54,uVar46,&local_2a0)
        ;
        FUN_00ac14d4(puVar9,0,0,uVar16,lVar66,param_1[0x7f0],param_1[0x7f0],param_1[0x7f1],0x30,
                     -uVar16,-lVar66,param_1 + 0x48af,*(int *)(*local_2a8 + 0xfc) != 0);
        lVar62 = param_1[0x7a0];
        uVar69 = (ulong)*(uint *)(lVar62 + 0x130);
        if (*(char *)(lVar62 + 0xd9) != '\0' || *(int *)(lVar62 + 0x168) != 0) {
          uVar69 = 1;
        }
        in_stack_fffffffffffffa30 = (byte *)0x0;
        FUN_00ac1194(param_1[0x3d52],uVar16,lVar66,0x60,0x60,
                     *(undefined8 *)(&DAT_012453e0 + uVar69 * 0x10),0,0x30,0,0,uVar16,lVar66,
                     param_1 + 0x494b,uVar86 & 0xffffffffffffff00);
        lVar63 = param_1[0x7a0];
        cVar64 = *(char *)(lVar63 + 0xd8);
        lVar62 = -1;
        if ((0 < local_390) && (cVar64 != '\0')) {
          lVar62 = (long)(*(float *)((long)param_1 + 0x114dc) + 0.95);
          lVar58 = local_390;
          if (lVar62 <= local_390) {
            lVar58 = lVar62;
          }
          lVar62 = lVar58 * 2 + -1;
          if (lVar58 < 1) {
            lVar62 = lVar58 * -2;
          }
          lVar62 = lVar62 + -1;
        }
        uVar41 = *(uint *)(DAT_01488658 + 0x78dcb0);
        uVar17 = *(uint *)(DAT_01488668 + 0x9ca7c8);
        iVar40 = (int)(CONCAT44((uVar17 + (uVar17 >> 0x10)) * 0x101 + (uVar17 >> 0x18),uVar17) >>
                      0x10);
        if (iVar40 != 0) {
          uVar17 = (uVar41 ^ uVar41 >> 1) & 0x45415510;
          *(uint *)(DAT_01488660 + 0x16ba960) = (uVar17 | uVar17 << 1) ^ uVar41;
        }
        if (DAT_01565dd8 != 0) {
          iVar40 = iVar40 + (uint)*(byte *)(DAT_01565dd8 + 0x3af0);
        }
        DAT_01488208 = iVar40;
        if (local_390 < 1) {
          local_46c = 1;
LAB_009a2be4:
          bVar36 = (uVar43 & 1) == 0;
          bVar21 = bVar35 && bVar36;
          puVar50 = puVar1;
          if (cVar64 != '\0') {
            if (!bVar35 || !bVar36) {
              *plVar5 = 1;
            }
            if ((bVar34) &&
               (lVar58 = *plVar5, lVar63 = lVar58 + -1, *plVar5 = lVar63, lVar63 == 0 || lVar58 < 1)
               ) {
              *plVar5 = 3;
              if (lVar62 < 0) {
                uVar46 = 0;
              }
              else {
                uVar46 = param_1[lVar62 + 0x486c];
              }
              fVar74 = (float)FUN_00a22690(puVar1);
              fVar74 = 1.0 - fVar74;
              if (!(bool)((bVar35 && (uVar43 & 1) == 0) ^ bVar23)) {
                local_568 = (fVar74 * (float)lVar59) / fVar77;
                fVar84 = (fVar74 * (float)lVar56) / fVar77;
                fVar85 = (fVar74 * (float)lVar45) / fVar77;
              }
              if (iVar40 != 0) {
                uVar41 = *(uint *)(DAT_01488670 + 0xad0168);
                uVar17 = *(uint *)(DAT_01488680 + 0x5fcf58);
                DAT_0148820c = (int)(CONCAT44((uVar17 + (uVar17 >> 0x10)) * 0x101 + (uVar17 >> 0x18)
                                              ,uVar17) >> 0x10);
                if (DAT_0148820c != 1) {
                  uVar17 = (uVar41 ^ uVar41 >> 1) & 0x45551411;
                  *(uint *)(DAT_01488678 + 0x2d4278) = (uVar17 | uVar17 << 1) ^ uVar41;
                }
              }
              if (bVar35 && bVar36) {
                uVar48 = FUN_00a224e4(fVar74,puVar1,2);
                uVar49 = FUN_00a224e4(fVar74,uVar48,0);
                param_1[0xf] = uVar49;
                puVar50 = (undefined8 *)FUN_00a224e4(fVar74,uVar48,1);
              }
              else {
                uVar48 = FUN_00a224e4(local_568,puVar1,0);
                param_1[0xf] = uVar48;
              }
              FUN_009a00bc(param_1,*local_2a8,uVar46,DAT_014a67c0);
              if (!bVar35 || !bVar36) {
                if (*plVar4 != 0) {
                  FUN_009d6064();
                  if ((long *)*plVar4 != (long *)0x0) {
                    (**(code **)(*(long *)*plVar4 + 0x10))();
                  }
                  *plVar4 = 0;
                }
                if (*(char *)((long)param_1 + 0x25cfc) == '\0') {
                  bVar24 = true;
                  goto LAB_009a2c54;
                }
                fVar74 = *(float *)(param_1 + 0x4b9e);
                if (0.0 <= fVar74) {
                  *(float *)(param_1 + 0x229a) = fVar74 + *(float *)(param_1 + 0x229a) * 0.0;
                }
                cVar64 = 0.0 > fVar74;
                if (0.0 <= *(float *)((long)param_1 + 0x25cf4)) {
                  *(float *)((long)param_1 + 0x114d4) =
                       *(float *)((long)param_1 + 0x25cf4) +
                       *(float *)((long)param_1 + 0x114d4) * 0.0;
                }
                else {
                  cVar64 = cVar64 + '\x01';
                }
                if (0.0 <= *(float *)(param_1 + 0x4b9f)) {
                  *(float *)(param_1 + 0x229b) =
                       *(float *)(param_1 + 0x4b9f) + *(float *)(param_1 + 0x229b) * 0.0;
                  FUN_0099e264(param_1,1);
                  if (cVar64 == '\0') goto LAB_009a3368;
                }
                else {
                  FUN_0099e264(param_1,1);
                }
                *plVar5 = 1;
              }
LAB_009a3368:
              bVar24 = true;
            }
          }
LAB_009a2c54:
          FUN_00a06664(&local_2a0,&local_311,&local_312);
          FUN_00a06880(&local_2a0);
          if (local_390 < 1) {
            lVar62 = 0;
            plVar61 = local_2a8;
          }
          else {
            uVar46 = FUN_00a224e4(fVar84,puVar50,0);
            param_1[0xe] = uVar46;
            if (*(int *)(param_1[0x7a0] + 0xe8) == 0) {
              fVar74 = *(float *)(param_1[0x7a0] + 0xec);
              if (fVar74 <= 0.1) {
                fVar74 = 0.1;
              }
              *(float *)((long)param_1 + 0x114dc) = 1.0 / fVar74;
            }
            if (bVar22) {
              fVar74 = (float)FUN_009a4510(uVar46,local_2a8,puVar12,0,local_390);
              if (bVar35) {
                fVar74 = fVar74 * 0.1 + *(float *)((long)param_1 + 0x114dc) * 0.9;
              }
              *(float *)((long)param_1 + 0x114dc) = fVar74;
            }
            in_stack_fffffffffffffa30 = pbVar33;
            local_46c = FUN_009a4800(param_1,*local_2a8,param_1[0x4883],0,local_4a8,0,
                                     param_1[0x3d52],param_1 + 0x4925,pbVar33,&local_2a0,local_2b8);
            if (*pbVar3 != 0) goto LAB_009a2fec;
            uVar41 = (**(code **)*param_1)(param_1);
            lVar62 = local_4a8;
            plVar61 = plVar8;
            if (((uVar41 | local_46c ^ 1) & 1) != 0) goto LAB_009a2e34;
          }
          local_320 = *plVar61;
          uVar41 = *(uint *)(DAT_01488688 + 0x1a2cf0);
          if (DAT_01488210 != 0) {
            uVar41 = uVar41 >> 0x15 | uVar41 << 0xb;
          }
          DAT_01488210 = (int)(CONCAT44((uVar41 + (uVar41 >> 0x10)) * 0x101 + (uVar41 >> 0x18),
                                        uVar41) >> 0x10);
          if (DAT_01488210 != 0) {
            *(uint *)(DAT_01488690 + 0x691b60) = uVar41;
          }
          if (DAT_01563528 != 0) {
            DAT_01488210 = DAT_01488210 + (uint)*(byte *)(DAT_01563528 + 0x838e);
          }
          local_328 = lVar62;
          uVar69 = (**(code **)*param_1)(param_1);
          if ((uVar69 & 1) != 0) goto LAB_009a2e34;
          FUN_00a06664(&local_2a0,&local_329,&local_32a);
          FUN_00a06880(&local_2a0);
          uVar46 = FUN_00a22934(fVar85,puVar50,param_1[0x3d59] * param_1[0x3d58]);
          *puVar2 = uVar46;
          FUN_00ac0d94(puVar9);
          FUN_00ac2b20(puVar11);
          local_370 = &DAT_013fceb0;
          local_360 = &local_320;
          local_358 = &local_2a8;
          local_350 = &local_2b0;
          local_348 = &local_328;
          local_340 = &local_2a0;
          local_338 = &local_2b8;
          local_368 = param_1;
          uVar41 = FUN_009fc6e8(&local_370,pbVar33,0,0);
          uVar18 = *(undefined4 *)(&UNK_013d5a10 + DAT_01488698);
          uVar15 = (uVar18 ^ (uint)uVar18 >> 1) & 0x45540040;
          uVar18 = (uVar15 | uVar15 << 1) ^ uVar18;
          DAT_01488214 = (int)(CONCAT44((uVar18 + ((uint)uVar18 >> 0x10)) * 0x101 +
                                        ((uint)uVar18 >> 0x18),uVar18) >> 0x10);
          if (DAT_01488214 != 0) {
            *(undefined4 *)(&rebase_opcode_0158ebc0 + DAT_014886a0) = uVar18;
          }
          iVar40 = (**(code **)*param_1)(param_1);
          iVar42 = FUN_00ac17a4(puVar6);
          uVar69 = 0x80;
          if (iVar40 == 0) {
            uVar69 = 0;
          }
          uVar86 = 2;
          if (iVar42 == 0) {
            uVar86 = 0;
          }
          puVar50 = (undefined8 *)FUN_00ac2c0c(puVar11,puVar9,uVar86 | uVar69,&local_2a0);
          local_46c = uVar41 & local_46c;
          uVar48 = puVar50[1];
          uVar46 = *puVar50;
          uVar49 = puVar50[2];
          uVar78 = puVar50[5];
          uVar76 = puVar50[4];
          param_1[0x4b18] = puVar50[3];
          param_1[0x4b17] = uVar49;
          param_1[0x4b1a] = uVar78;
          param_1[0x4b19] = uVar76;
          param_1[0x4b16] = uVar48;
          param_1[0x4b15] = uVar46;
          uVar48 = puVar50[7];
          uVar46 = puVar50[6];
          uVar76 = puVar50[9];
          uVar49 = puVar50[8];
          uVar79 = puVar50[0xb];
          uVar78 = puVar50[10];
          *(undefined8 *)((long)param_1 + 0x25905) = *(undefined8 *)((long)puVar50 + 0x5d);
          param_1[0x4b1e] = uVar76;
          param_1[0x4b1d] = uVar49;
          param_1[0x4b20] = uVar79;
          param_1[0x4b1f] = uVar78;
          param_1[0x4b1c] = uVar48;
          param_1[0x4b1b] = uVar46;
          uVar41 = (DAT_014881f4 + DAT_014881f0 + *(int *)(param_1 + 0x4b16)) * 0x10001 +
                   (DAT_014881f4 >> 0x10);
          DAT_014881f0 = DAT_014881f4;
          DAT_014881f4 = uVar41;
          if (*pbVar3 == 0) {
            uVar69 = (**(code **)*param_1)(param_1);
            if (local_46c == 0 || (uVar69 & 1) != 0) goto LAB_009a2e34;
            FUN_009a4c10(uVar69,local_2b0,*(int *)(*local_2a8 + 0xfc) != 0);
            FUN_00a067d8(&local_2a0);
            if (*pbVar3 != 0) goto LAB_009a2fec;
            uVar69 = (**(code **)*param_1)(param_1);
            if ((uVar69 & 1) != 0) goto LAB_009a2e34;
            FUN_00a06664(&local_2a0,&local_371,&local_372);
            bVar32 = local_311;
            bVar31 = local_312;
            bVar30 = local_329;
            bVar29 = local_32a;
            bVar28 = local_371;
            bVar27 = local_372;
            bVar19 = *pbVar3;
            iVar40 = FUN_00ac17a4(puVar6);
            uVar41 = 2;
            if (iVar40 == 0) {
              uVar41 = 0;
            }
            uVar53 = uVar41 | uVar53 |
                     ((uint)(byte)(bVar30 | bVar32 | bVar28) << 4 | (uint)bVar19 << 6 |
                     (uint)(byte)(bVar29 | bVar31 | bVar27) << 5) & 0xf0;
            iVar40 = 3;
          }
          else {
LAB_009a2fec:
            iVar40 = 2;
          }
        }
        else {
          fVar74 = ABS(*(float *)((long)param_1 + 0x17344)) + ABS(*(float *)(param_1 + 0x2e69)) +
                   ABS(*(float *)((long)param_1 + 0x1734c)) + ABS(*(float *)(param_1 + 0x2e6a)) +
                   ABS(*(float *)((long)param_1 + 0x17354));
          bVar36 = (ABS(*(float *)(param_1 + 0x3253)) + ABS(*(float *)((long)param_1 + 0x1929c)) +
                    ABS(*(float *)(param_1 + 0x3254)) + ABS(*(float *)((long)param_1 + 0x192a4)) +
                   ABS(*(float *)(param_1 + 0x3255))) * 100.0 < fVar74 &&
                   (ABS(*(float *)((long)param_1 + 0x1b1ec)) + ABS(*(float *)(param_1 + 0x363e)) +
                    ABS(*(float *)((long)param_1 + 0x1b1f4)) + ABS(*(float *)(param_1 + 0x363f)) +
                   ABS(*(float *)((long)param_1 + 0x1b1fc))) * 100.0 < fVar74;
          uVar69 = uVar57;
          lVar58 = (long)param_1 + 0x24069;
          if (uVar44 < 2) {
            uVar69 = 0;
LAB_009a295c:
            lVar58 = (long)param_1 + uVar69 * 0x58 + 0x24011;
            do {
              *(bool *)(lVar58 + -1) = bVar22;
              *(bool *)lVar58 = bVar36;
              uVar69 = uVar69 + 1;
              lVar58 = lVar58 + 0x58;
            } while ((long)uVar69 < (long)uVar60);
          }
          else {
            do {
              *(bool *)(lVar58 + -0x59) = bVar22;
              *(bool *)(lVar58 + -1) = bVar22;
              *(bool *)(lVar58 + -0x58) = bVar36;
              *(bool *)lVar58 = bVar36;
              uVar69 = uVar69 - 2;
              lVar58 = lVar58 + 0xb0;
            } while (uVar69 != 0);
            uVar69 = uVar57;
            if (uVar44 != uVar57) goto LAB_009a295c;
          }
          if ((0.0 < *(float *)(lVar63 + 0x160)) || (*(char *)(lVar63 + 0xd9) != '\0')) {
            bVar36 = true;
          }
          else {
            bVar36 = *(int *)(lVar63 + 0x168) != 0;
          }
          uVar46 = FUN_00a224e4((float)lVar55 / fVar81,puVar1,0);
          *puVar2 = uVar46;
          in_stack_fffffffffffffa30 = pbVar33;
          local_46c = FUN_00987f88(param_1[0x3d4d],local_2a8 + 1,puVar14,*local_2a8,uVar60,bVar36,
                                   lVar62,puVar12,pbVar33,uVar46,&local_2a0,local_2b8);
          *pbVar3 = *pbVar3 | *(byte *)(param_1[0x3d4d] + 0x5b4);
          if (*pbVar3 != 0) goto LAB_009a2fec;
          uVar41 = (**(code **)*param_1)(param_1);
          if (((uVar41 | local_46c ^ 1) & 1) == 0) {
            lVar63 = 0;
            lVar58 = param_1[0x3d4d];
            uVar48 = *(undefined8 *)(lVar58 + 0x288);
            uVar46 = *(undefined8 *)(lVar58 + 0x280);
            uVar49 = *(undefined8 *)(lVar58 + 0x290);
            uVar78 = *(undefined8 *)(lVar58 + 0x2a8);
            uVar76 = *(undefined8 *)(lVar58 + 0x2a0);
            param_1[0x4afd] = *(undefined8 *)(lVar58 + 0x298);
            param_1[0x4afc] = uVar49;
            param_1[0x4aff] = uVar78;
            param_1[0x4afe] = uVar76;
            param_1[0x4afb] = uVar48;
            *puVar7 = uVar46;
            uVar48 = *(undefined8 *)(lVar58 + 0x2b8);
            uVar46 = *(undefined8 *)(lVar58 + 0x2b0);
            uVar76 = *(undefined8 *)(lVar58 + 0x2c8);
            uVar49 = *(undefined8 *)(lVar58 + 0x2c0);
            uVar79 = *(undefined8 *)(lVar58 + 0x2d8);
            uVar78 = *(undefined8 *)(lVar58 + 0x2d0);
            uVar80 = *(undefined8 *)(lVar58 + 0x2d9);
            *(undefined8 *)((long)param_1 + 0x25831) = *(undefined8 *)(lVar58 + 0x2e1);
            *(undefined8 *)((long)param_1 + 0x25829) = uVar80;
            param_1[0x4b03] = uVar76;
            param_1[0x4b02] = uVar49;
            param_1[0x4b05] = uVar79;
            param_1[0x4b04] = uVar78;
            param_1[0x4b01] = uVar48;
            param_1[0x4b00] = uVar46;
            do {
              lVar58 = FUN_009c610c(local_2a8[lVar63 + 1]);
              plVar61 = local_2a8;
              *(bool *)((long)puVar13 + lVar63) = lVar58 == 1;
              lVar63 = lVar63 + 1;
            } while (lVar63 < (long)uVar60);
            uVar69 = (long)*(int *)(param_1[0x7a0] + 0xd0) << 1 | 1;
            if (0 < (int)uVar69) {
              puVar50 = puVar13;
              uVar67 = 0;
              uVar86 = uVar69;
              plVar72 = local_2a8;
              do {
                plVar72 = plVar72 + 1;
                uVar86 = uVar86 - 1;
                uVar47 = uVar67 + 1;
                puVar65 = puVar50;
                plVar68 = plVar72;
                uVar70 = uVar86;
                if ((long)uVar47 < (long)uVar69) {
                  do {
                    lVar63 = FUN_009c610c(plVar61[uVar67]);
                    lVar58 = FUN_009c610c(*plVar68);
                    if (lVar63 == lVar58) {
                      *(undefined1 *)puVar65 = 1;
                    }
                    uVar70 = uVar70 - 1;
                    puVar65 = (undefined8 *)((long)puVar65 + 1);
                    plVar68 = plVar68 + 1;
                  } while (uVar70 != 0);
                }
                puVar50 = (undefined8 *)((long)puVar50 + 1);
                uVar67 = uVar47;
              } while (uVar47 != uVar69);
            }
            uVar43 = FUN_009a3cac(param_1,local_2a8,puVar12,puVar13,local_390);
            FUN_009a4018(param_1,local_2a8,puVar13,param_1 + 0x4876,param_1 + 0x4882);
            fVar74 = *(float *)(param_1[0x7a0] + 0x164) * 0.16;
            FUN_009a4348(fVar74 * fVar74,param_1,puVar12,param_1 + 0x4867,local_390);
            cVar64 = *(char *)(param_1[0x7a0] + 0xd8);
            goto LAB_009a2be4;
          }
LAB_009a2e34:
          iVar40 = 3;
        }
        uVar69 = (ulong)(local_46c != 0);
      }
      else {
        iVar40 = 3;
      }
    }
    else {
      iVar40 = 2;
    }
    FUN_00a067d8(&local_2a0);
    if (local_2e8 != 0) {
      FUN_00ac17dc();
    }
    bVar36 = false;
  } while (iVar40 == 2);
  if (*plVar4 != 0) {
    FUN_009d6064();
    if ((long *)*plVar4 != (long *)0x0) {
      (**(code **)(*(long *)*plVar4 + 0x10))();
    }
    *plVar4 = 0;
    if (*(char *)((long)param_1 + 0x25cfc) != '\0') {
      fVar81 = 0.0;
      if (bVar21) {
        fVar81 = 0.7;
      }
      if (0.0 <= *(float *)(param_1 + 0x4b9e)) {
        lVar66 = 0;
        *(float *)(param_1 + 0x229a) =
             (1.0 - fVar81) * *(float *)(param_1 + 0x4b9e) + fVar81 * *(float *)(param_1 + 0x229a);
        fVar77 = *(float *)((long)param_1 + 0x25cf4);
        if (0.0 <= fVar77) goto LAB_009a33e0;
LAB_009a347c:
        lVar66 = lVar66 + 1;
        fVar77 = *(float *)(param_1 + 0x4b9f);
        if (0.0 <= fVar77) goto LAB_009a3408;
LAB_009a348c:
        FUN_0099e264(param_1,1);
      }
      else {
        lVar66 = 1;
        fVar77 = *(float *)((long)param_1 + 0x25cf4);
        if (fVar77 < 0.0) goto LAB_009a347c;
LAB_009a33e0:
        *(float *)((long)param_1 + 0x114d4) =
             (1.0 - fVar81) * fVar77 + fVar81 * *(float *)((long)param_1 + 0x114d4);
        fVar77 = *(float *)(param_1 + 0x4b9f);
        if (fVar77 < 0.0) goto LAB_009a348c;
LAB_009a3408:
        *(float *)(param_1 + 0x229b) =
             (1.0 - fVar81) * fVar77 + fVar81 * *(float *)(param_1 + 0x229b);
        FUN_0099e264(param_1,1);
        if (lVar66 == 0) goto LAB_009a34a0;
      }
      *plVar5 = 1;
    }
  }
LAB_009a34a0:
  *pbVar3 = bVar20;
  uVar43 = (**(code **)*param_1)();
  if (iVar83 != 0) {
    iVar39 = (**(code **)*param_1)();
    uVar41 = 0x80;
    if (iVar39 == 0) {
      uVar41 = 0;
    }
    uVar17 = 0x400;
    if (!bVar24) {
      uVar17 = 0;
    }
    FUN_00a31634(puVar10,puVar7,param_1 + 0x4b08,param_1 + 0x4b15,uVar53 | uVar17 | uVar41);
  }
  uVar43 = (uint)uVar69 & (uVar43 ^ 1);
  if (0 < lVar71) {
    do {
      FUN_009d8290(param_5);
      lVar71 = lVar71 + -1;
    } while (lVar71 != 0);
  }
LAB_009a3544:
  if (*(long *)PTR____stack_chk_guard_013b4320 != local_b8) {
                    /* WARNING: Subroutine does not return */
    ___stack_chk_fail();
  }
  return uVar43;
}


