// FUN_000435c4 size=6040

void FUN_000435c4(double param_1,long param_2,undefined8 param_3,double *param_4,int param_5)

{
  bool bVar1;
  float *pfVar2;
  bool bVar3;
  uint uVar4;
  char cVar5;
  bool bVar6;
  bool bVar7;
  bool bVar8;
  ulong uVar9;
  long lVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  uint uVar14;
  long lVar16;
  long lVar17;
  float *pfVar18;
  int iVar19;
  size_t sVar20;
  ulong uVar21;
  uint *puVar22;
  uint uVar23;
  ulong uVar24;
  ulong uVar25;
  long lVar26;
  ulong uVar27;
  float *pfVar28;
  uint uVar29;
  int iVar30;
  ulong uVar31;
  float *pfVar32;
  undefined8 *puVar33;
  uint uVar34;
  int iVar35;
  ulong uVar36;
  undefined8 *puVar37;
  long lVar38;
  long lVar39;
  ulong uVar40;
  undefined8 *puVar41;
  float *pfVar42;
  long lVar43;
  long lVar44;
  int iVar45;
  long lVar46;
  int iVar47;
  int iVar48;
  ulong *puVar49;
  int iVar50;
  int iVar51;
  int iVar52;
  int iVar53;
  int iVar54;
  ulong uVar55;
  undefined8 uVar56;
  ulong uVar57;
  float fVar58;
  float fVar59;
  float fVar60;
  double dVar61;
  float fVar62;
  float fVar63;
  undefined8 uVar64;
  float fVar65;
  undefined8 uVar66;
  undefined8 uVar67;
  float fVar68;
  float fVar69;
  undefined4 uVar70;
  undefined4 uVar71;
  float fVar72;
  float fVar73;
  float fVar74;
  undefined1 auVar75 [16];
  long local_358;
  ulong local_350;
  int local_32c;
  long local_328;
  long local_310;
  uint local_308;
  int local_304;
  long local_2c0;
  long local_2b8;
  int local_2b0;
  int local_29c;
  undefined8 local_248;
  int local_234;
  ulong local_1f8;
  int local_1e4;
  ulong local_1e0;
  ulong local_1d0;
  undefined4 local_1c4;
  ulong local_1c0;
  undefined8 local_1b8;
  int local_1ac;
  undefined8 local_1a8;
  undefined1 auStack_1a0 [32];
  ulong local_180 [4];
  undefined1 auStack_160 [32];
  ulong local_140 [4];
  long local_120 [4];
  undefined8 local_100;
  ulong uStack_f8;
  undefined8 local_f0;
  ulong uStack_e8;
  long local_e0 [9];
  long local_98;
  int iVar15;
  
  local_98 = *(long *)PTR____stack_chk_guard_013b4320;
  lVar16 = *(long *)(param_2 + 8);
  if (lVar16 == 0) goto LAB_00044928;
  iVar47 = (int)*(undefined8 *)(lVar16 + 0xd0);
  iVar11 = (int)*(undefined8 *)(lVar16 + 0xd8);
  if (iVar47 == 0 || iVar11 == 0) goto LAB_00044928;
  uVar56 = *(undefined8 *)
            ("/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit" +
            param_2 + (long)param_5 * 8 + 0x20);
  uVar9 = FUN_00496dd0(param_3);
  local_234 = FUN_00496884(param_3);
  uVar14 = DAT_0141a4ae;
  dVar61 = *param_4 * param_1;
  if (0.0 <= dVar61) {
    iVar50 = (int)(dVar61 + 0.5);
  }
  else {
    iVar50 = (int)((dVar61 - (double)(int)(dVar61 + -1.0)) + 0.5) + (int)(dVar61 + -1.0);
  }
  dVar61 = param_4[1] * param_1;
  if (0.0 <= dVar61) {
    iVar12 = (int)(dVar61 + 0.5);
  }
  else {
    iVar12 = (int)((dVar61 - (double)(int)(dVar61 + -1.0)) + 0.5) + (int)(dVar61 + -1.0);
  }
  dVar61 = (double)iVar47 * param_1;
  if (0.0 <= dVar61) {
    iVar52 = (int)(dVar61 + 0.5);
  }
  else {
    iVar52 = (int)((dVar61 - (double)(int)(dVar61 + -1.0)) + 0.5) + (int)(dVar61 + -1.0);
  }
  dVar61 = (double)iVar11 * param_1;
  if (0.0 <= dVar61) {
    iVar13 = (int)(dVar61 + 0.5);
  }
  else {
    iVar13 = (int)((dVar61 - (double)(int)(dVar61 + -1.0)) + 0.5) + (int)(dVar61 + -1.0);
  }
  local_e0[1] = 0;
  local_e0[0] = 0;
  local_e0[3] = 0;
  local_e0[2] = 0;
  local_e0[5] = 0;
  local_e0[4] = 0;
  local_e0[7] = 0;
  local_e0[6] = 0;
  local_100 = 0;
  uStack_f8 = 0;
  local_f0 = 0;
  uStack_e8 = 0;
  lVar16 = *(long *)(param_2 + 8);
  cVar5 = *(char *)(lVar16 + 0xf4);
  uVar29 = *(uint *)(param_2 + 0x58);
  if (cVar5 == '\0') {
    if ((uVar29 != 0) && ((int)uVar29 < 4)) goto LAB_0004379c;
LAB_000437c0:
    if ((uVar29 < 7) && ((1 << (ulong)(uVar29 & 0x1f) & 0x71U) != 0)) {
      local_e0[5] = *(undefined8 *)(lVar16 + 0xb8);
      local_e0[4] = *(long *)(lVar16 + 0xb0);
      local_e0[6] = *(undefined8 *)(lVar16 + 0xc0);
      uVar29 = *(uint *)(lVar16 + 0xf0);
      uStack_e8 = (ulong)uVar29;
      local_f0 = CONCAT44(uVar29,uVar29);
      lVar16 = *(long *)(param_2 + 0x10);
      if (lVar16 != 0) {
        local_e0[1] = *(undefined8 *)(lVar16 + 0xb8);
        local_e0[0] = *(long *)(lVar16 + 0xb0);
        local_e0[2] = *(undefined8 *)(lVar16 + 0xc0);
        uVar29 = *(uint *)(lVar16 + 0xf0);
        uStack_f8 = (ulong)uVar29;
        local_100 = CONCAT44(uVar29,uVar29);
      }
      uVar55 = 3;
      local_2c0 = 1;
      local_2b8 = 2;
    }
    else {
      if (uVar29 - 7 < 0xc) {
        lVar17 = *(long *)(param_2 + 0x18);
        if (lVar17 == 0) {
LAB_00044bec:
          bVar3 = false;
          bVar6 = false;
          lVar38 = 0;
        }
        else {
          if (*(char *)(lVar17 + 0xe0) == '\0') {
            lVar17 = 0;
            goto LAB_00044bec;
          }
          lVar38 = *(long *)(param_2 + 0x20);
          local_f0 = (ulong)*(uint *)(lVar17 + 0xf0);
          bVar3 = lVar38 != 0;
          if (bVar3) {
            local_100 = (ulong)*(uint *)(lVar38 + 0xf0);
          }
          bVar6 = true;
        }
        switch(uVar29 - 7) {
        case 0:
        case 3:
        case 6:
        case 9:
          if (lVar16 != 0) {
            if (bVar6) {
              local_e0[4] = *(long *)(lVar17 + 0xb0);
            }
            else {
              local_e0[4] = 0;
            }
          }
          if (*(long *)(param_2 + 0x10) != 0) {
            if (bVar3) {
              local_e0[0] = *(long *)(lVar38 + 0xb0);
            }
            else {
LAB_00044d48:
              local_e0[0] = 0;
            }
          }
          break;
        case 1:
          if (lVar16 != 0) {
            if (bVar6) {
              local_e0[4] = *(long *)(lVar17 + 0xb8);
            }
            else {
              local_e0[4] = 0;
            }
          }
          if (*(long *)(param_2 + 0x10) != 0) {
            if (!bVar3) goto LAB_00044d48;
            local_e0[0] = *(long *)(lVar38 + 0xb8);
          }
          break;
        case 2:
          if (lVar16 != 0) {
            if (bVar6) {
              local_e0[4] = *(long *)(lVar17 + 0xc0);
            }
            else {
              local_e0[4] = 0;
            }
          }
          if (*(long *)(param_2 + 0x10) != 0) {
            if (!bVar3) goto LAB_00044d48;
            local_e0[0] = *(long *)(lVar38 + 0xc0);
          }
          break;
        case 4:
        case 7:
        case 10:
          if (lVar16 != 0) {
            if (bVar6) {
              lVar16 = *(long *)(lVar17 + 0xb8);
              local_e0[4] = lVar16;
            }
            else {
              lVar16 = 0;
              local_e0[4] = lVar16;
            }
          }
          if (*(long *)(param_2 + 0x10) != 0) {
            local_e0[0] = 0;
            if (cVar5 != '\0') {
              local_e0[0] = lVar16;
            }
            if ((bool)(cVar5 == '\0' & bVar3)) {
              local_e0[0] = *(long *)(lVar38 + 0xb8);
            }
          }
          break;
        case 5:
        case 8:
        case 0xb:
          if (lVar16 != 0) {
            if (bVar6) {
              lVar16 = *(long *)(lVar17 + 0xc0);
              local_e0[4] = lVar16;
            }
            else {
              lVar16 = 0;
              local_e0[4] = lVar16;
            }
          }
          if (*(long *)(param_2 + 0x10) != 0) {
            local_e0[0] = 0;
            if (cVar5 != '\0') {
              local_e0[0] = lVar16;
            }
            if ((bool)(cVar5 == '\0' & bVar3)) {
              local_e0[0] = *(long *)(lVar38 + 0xc0);
            }
          }
        }
        goto switchD_00044c1c_default;
      }
      local_2c0 = 0;
      local_2b8 = 0;
      uVar55 = 0;
    }
  }
  else {
    if ((6 < (int)uVar29) && (5 < uVar29 - 10)) goto LAB_000437c0;
LAB_0004379c:
    if (uVar29 < 2) {
      local_e0[4] = *(long *)(lVar16 + 0xb0);
      lVar17 = *(long *)(param_2 + 0x10);
      if (lVar17 == 0) goto LAB_0004388c;
      local_e0[0] = *(long *)(lVar17 + 0xb0);
LAB_00043870:
      local_f0 = (ulong)*(uint *)(lVar16 + 0xf0);
      local_100 = (ulong)*(uint *)(lVar17 + 0xf0);
    }
    else {
      if (uVar29 != 2) {
        local_e0[4] = *(long *)(lVar16 + 0xc0);
        lVar17 = *(long *)(param_2 + 0x10);
        if (lVar17 == 0) goto LAB_0004388c;
        local_e0[0] = *(long *)(lVar17 + 0xc0);
        goto LAB_00043870;
      }
      local_e0[4] = *(long *)(lVar16 + 0xb8);
      lVar17 = *(long *)(param_2 + 0x10);
      if (lVar17 != 0) {
        local_e0[0] = *(long *)(lVar17 + 0xb8);
        goto LAB_00043870;
      }
LAB_0004388c:
      local_f0 = (ulong)*(uint *)(lVar16 + 0xf0);
    }
switchD_00044c1c_default:
    local_2b8 = 0;
    local_2c0 = 0;
    uVar55 = 1;
  }
  local_29c = iVar47;
  if (DAT_0141a4a8 >> 0x18 == 0) {
    uVar24 = (ulong)DAT_0141a4ae;
    *(undefined **)(DAT_0141a4c0 + 0x655e528) = &DAT_0141a4ac;
    uVar29 = uVar14;
    if (*(long *)(DAT_0141a4b8 + 0x690b480) != 0) {
      uVar29 = -(uint)*(byte *)(*(long *)(DAT_0141a4b8 + 0x690b480) + (uVar24 & 0xffff));
    }
    local_29c = iVar47 + (uVar14 >> 0x18) + (uVar29 + (uVar14 >> 0x10 & 0xff)) * 2;
  }
  iVar53 = (int)(long)(param_1 * 10.0);
  DAT_0141a4a8 = (int)(CONCAT44((DAT_0141a4a8 + (DAT_0141a4a8 >> 0x10)) * 0x101 +
                                (DAT_0141a4a8 >> 0x18),DAT_0141a4a8) >> 0x10) + iVar53;
  if (*(char *)(param_2 + 0x79) == '\0') {
LAB_0004399c:
    iVar54 = -1;
    if ((int)uVar55 != 0) {
      uVar24 = 0xffffffff;
      local_1d0 = uVar55;
      goto LAB_000439ac;
    }
    bVar3 = false;
    local_1d0 = 0;
  }
  else {
    lVar16 = *(long *)(param_2 + 8);
    lVar17 = *(long *)(lVar16 + 200);
    if (lVar17 == 0) goto LAB_0004399c;
    local_e0[uVar55 + 4] = lVar17;
    local_1d0 = (ulong)((int)uVar55 + 1);
    *(undefined4 *)((long)&local_f0 + uVar55 * 4) = *(undefined4 *)(lVar16 + 0xf0);
    lVar16 = *(long *)(param_2 + 0x10);
    uVar24 = uVar55;
    if (lVar16 != 0) {
      local_e0[uVar55] = *(long *)(lVar16 + 200);
      *(undefined4 *)((long)&local_100 + uVar55 * 4) = *(undefined4 *)(lVar16 + 0xf0);
    }
LAB_000439ac:
    iVar54 = (int)uVar24;
    uVar55 = 0;
    do {
      lVar16 = FUN_009d8200(uVar56,-(uVar9 >> 0x1f & 1) & 0xfffffffc00000000 |
                                   (uVar9 & 0xffffffff) << 2,0xbf07ac01);
      local_120[uVar55] = lVar16;
      uVar55 = uVar55 + 1;
    } while (local_1d0 != uVar55);
    bVar3 = true;
  }
  if ((((*(char *)(param_2 + 0x78) == '\0') || (*(long *)(param_2 + 0x10) == 0)) ||
      (*(int *)(param_2 + 0x58) != 0)) ||
     ((*(long *)(param_2 + 0x30) == 0 || (*(char *)(*(long *)(param_2 + 0x30) + 0xe0) == '\0')))) {
    bVar6 = false;
    local_248 = 0;
    uVar55 = local_1d0;
  }
  else {
    local_248 = FUN_009d8200(uVar56,-(uVar9 >> 0x1f & 1) & 0xfffffffc00000000 |
                                    (uVar9 & 0xffffffff) << 2,0xbf07ac02);
    uVar55 = local_1d0 + 1;
    bVar6 = true;
  }
  if (*(int *)(param_2 + 0x58) - 4U < 3) {
    uVar24 = -(uVar9 >> 0x1f & 1) & 0xfffffffc00000000 | (uVar9 & 0xffffffff) << 2;
    local_310 = FUN_009d8200(uVar56,uVar24,0xbf07ac03);
    local_358 = FUN_009d8200(uVar56,uVar24,0xbf07ac04);
    local_328 = FUN_009d8200(uVar56,uVar24,0xbf07ac05);
    uVar55 = uVar55 + 3;
  }
  else {
    local_328 = 0;
    local_358 = 0;
    local_310 = 0;
  }
  lVar16 = (long)iVar52;
  uVar24 = 0;
  if (lVar16 != 0) {
    uVar24 = ((long)iVar47 * (long)iVar50) / lVar16;
  }
  if ((*(long *)(param_2 + 0x28) == 0) || (*(char *)(*(long *)(param_2 + 0x28) + 0xe0) == '\0')) {
    bVar7 = false;
    auVar75 = *(undefined1 (*) [16])(param_2 + 0x48);
  }
  else {
    auVar75 = FUN_002aca90(param_2 + 0x38,param_2 + 0x48);
    bVar7 = true;
  }
  iVar30 = (int)uVar24;
  iVar47 = (int)((long)iVar47 * (long)iVar50) - iVar30 * iVar52;
  iVar51 = auVar75._0_4_;
  uVar14 = (uint)uVar9;
  iVar15 = auVar75._12_4_;
  iVar19 = auVar75._4_4_;
  if (((((*(char *)(param_2 + 0x78) == '\0') || (*(long *)(param_2 + 0x10) == 0)) ||
       (iVar45 = auVar75._8_4_, iVar45 < iVar51)) ||
      ((iVar15 < iVar19 || ((double)iVar45 < *param_4)))) ||
     (*param_4 + param_4[2] < (double)iVar51)) {
    local_1e0 = 0;
    local_350 = 0;
    iVar48 = -1;
    bVar8 = true;
    local_304 = 0;
    local_2b0 = -1;
    local_308 = uVar14;
  }
  else {
    local_308 = 0;
    local_1ac = iVar47;
    if (iVar30 < iVar51) {
      do {
        if (*(long *)(param_2 + 8) == 0) {
          iVar48 = 0;
        }
        else {
          iVar48 = *(int *)(*(long *)(param_2 + 8) + 0xd0);
        }
        local_308 = local_308 + 1;
        for (local_1ac = local_1ac + iVar48; iVar52 <= local_1ac; local_1ac = local_1ac - iVar52) {
          uVar24 = (ulong)((int)uVar24 + 1);
        }
      } while ((int)uVar24 < iVar51);
    }
    lVar17 = *(long *)(param_2 + 8);
    iVar48 = (int)uVar24;
    if (iVar45 < iVar48) {
      uVar29 = 0;
    }
    else {
      uVar29 = 0;
      do {
        if (lVar17 == 0) {
          iVar35 = 0;
        }
        else {
          iVar35 = *(int *)(lVar17 + 0xd0);
        }
        uVar29 = uVar29 + 1;
        for (local_1ac = local_1ac + iVar35; iVar52 <= local_1ac; local_1ac = local_1ac - iVar52) {
          uVar24 = (ulong)((int)uVar24 + 1);
        }
      } while ((int)uVar24 <= iVar45);
    }
    iVar45 = *(int *)(lVar17 + 0xd0);
    local_2b0 = (int)uVar24;
    local_304 = 0;
    if (local_2b0 < iVar45) {
      iVar35 = 0;
      if (lVar17 != 0) {
        iVar35 = iVar45;
      }
      do {
        local_304 = local_304 + 1;
        for (local_1ac = local_1ac + iVar35; iVar52 <= local_1ac; local_1ac = local_1ac - iVar52) {
          uVar24 = (ulong)((int)uVar24 + 1);
        }
      } while ((int)uVar24 < iVar45);
    }
    if ((int)uVar14 <= (int)local_308) {
      local_308 = uVar14;
    }
    uVar34 = uVar14 - local_308;
    if ((int)uVar34 <= (int)uVar29) {
      uVar29 = uVar34;
    }
    local_1e0 = (ulong)uVar29;
    if ((int)(uVar34 - uVar29) <= local_304) {
      local_304 = uVar34 - uVar29;
    }
    if (bVar7) {
      uVar24 = -(ulong)(uVar29 >> 0x1f) & 0xfffffffc00000000 | local_1e0 << 2;
      if (bVar3) {
        uVar55 = uVar55 + ((int)local_1d0 - 1) + 1;
        puVar49 = local_140;
        uVar31 = local_1d0;
        do {
          uVar25 = FUN_009d8200(uVar56,uVar24,0xbf07ac06);
          *puVar49 = uVar25;
          uVar31 = uVar31 - 1;
          puVar49 = puVar49 + 1;
        } while (uVar31 != 0);
      }
      local_350 = FUN_009d8200(uVar56,uVar24,0xbf07ac07);
      bVar8 = false;
      uVar55 = uVar55 + 1;
      bVar7 = true;
    }
    else {
      bVar8 = false;
      bVar7 = false;
      local_350 = 0;
    }
  }
  uVar29 = DAT_0141a4ce;
  uVar24 = 0;
  if ((long)iVar13 != 0) {
    uVar24 = ((long)iVar11 * (long)iVar12) / (long)iVar13;
  }
  if (DAT_0141a4c8 >> 0x18 == 0) {
    uVar31 = (ulong)DAT_0141a4ce;
    *(undefined **)(DAT_0141a4e0 + 0x61146a8) = &DAT_0141a4cc;
    uVar34 = uVar29;
    if (*(long *)(DAT_0141a4d8 + 0x50218e0) != 0) {
      uVar34 = -(uint)*(byte *)(*(long *)(DAT_0141a4d8 + 0x50218e0) + (uVar31 & 0xffff));
    }
    local_234 = local_234 + (uVar29 >> 0x18) + (uVar34 + (uVar29 >> 0x10 & 0xff)) * 2;
  }
  uVar29 = 0x8c10fc91;
  DAT_0141a4c8 = (int)(CONCAT44((DAT_0141a4c8 + (DAT_0141a4c8 >> 0x10)) * 0x101 +
                                (DAT_0141a4c8 >> 0x18),DAT_0141a4c8) >> 0x10) + iVar53;
  uVar34 = 0;
  if (0 < iVar12) {
    uVar23 = 0x8c10fc91;
    iVar52 = iVar12;
    do {
      uVar29 = (uint)(CONCAT44((uVar23 + (uVar23 >> 0x10)) * 0x101 + (uVar23 >> 0x18),uVar23) >>
                     0x10);
      uVar34 = (uVar23 & 0x7f) + uVar34 + 8 & 0xf8;
      iVar52 = iVar52 + -1;
      uVar23 = uVar29;
    } while (iVar52 != 0);
  }
  if (((int)uVar24 < iVar11) && (0 < local_234)) {
    local_1e4 = 0;
    iVar52 = (int)((long)iVar11 * (long)iVar12) - (int)uVar24 * iVar13;
    local_1f8 = (ulong)(iVar12 % 4);
    lVar43 = (long)(int)uVar14;
    uVar23 = uVar14 & 0xfffffff0;
    lVar38 = (long)(int)uVar23;
    sVar20 = local_1d0 << 3;
    uVar25 = local_1e0 & 0xfffffff8;
    uVar31 = -(ulong)(uVar14 >> 0x1f) & 0xfffffffc00000000 | (ulong)uVar23 << 2;
    pfVar2 = (float *)(local_120[0] + uVar31);
    lVar17 = lVar43 - lVar38;
    do {
      uVar34 = (uVar29 & 0x7f) + uVar34 + 8 & 0xf8;
      uVar57 = uVar24;
      for (iVar52 = iVar52 + iVar11; iVar13 <= iVar52; iVar52 = iVar52 - iVar13) {
        uVar57 = (ulong)((int)uVar57 + 1);
      }
      local_1f8 = (ulong)((int)local_1f8 + 1) & 3;
      iVar12 = *(int *)((long)&dylib_command_000010e0.cmdsize + param_2);
      iVar53 = 0;
      if (iVar12 != 0) {
        iVar53 = local_1e4 / iVar12;
      }
      if (local_1e4 - iVar53 * iVar12 == param_5) {
        lVar10 = FUN_004968f0(param_3);
        if (bVar3) {
          _memcpy(auStack_160,local_120,sVar20);
        }
        local_1b8 = local_248;
        iVar12 = (int)uVar24;
        if (iVar12 < iVar19 || bVar8) {
          bVar1 = false;
          uVar24 = uVar9;
        }
        else {
          bVar1 = iVar12 <= iVar15;
          uVar4 = local_308;
          if (iVar15 < iVar12) {
            uVar4 = uVar14;
          }
          uVar24 = (ulong)uVar4;
        }
        iVar53 = iVar12 * local_29c;
        local_1ac = iVar47;
        FUN_00043290(param_2,auStack_160,local_e0 + 4,local_1d0,uVar24,lVar16,&local_1ac,&local_f0,
                     iVar53 + iVar30);
        if (bVar6) {
          local_180[0] = CONCAT44(local_180[0]._4_4_,iVar47);
          FUN_00043290(param_2,&local_1b8,*(long *)(param_2 + 0x30) + 0xb0,1,uVar9,lVar16,local_180,
                       *(long *)(param_2 + 0x30) + 0xf0,iVar53 + iVar30);
        }
        if (bVar1) {
          if ((bVar7) && (local_32c = local_1ac, bVar3)) {
            _memcpy(local_180,auStack_160,sVar20);
          }
          FUN_00043290(param_2,auStack_160,local_e0,local_1d0,local_1e0,lVar16,&local_1ac,&local_100
                       ,(iVar48 - iVar51) +
                        (iVar12 - iVar19) * *(int *)(*(long *)(param_2 + 0x10) + 0xd0));
          if (bVar7) {
            if (bVar3) {
              _memcpy(auStack_1a0,local_140,sVar20);
            }
            local_1c0 = local_350;
            local_1ac = local_32c;
            FUN_00043290(param_2,auStack_1a0,local_e0 + 4,local_1d0,local_1e0,lVar16,&local_1ac,
                         &local_f0,iVar53 + iVar48);
            lVar26 = *(long *)(param_2 + 0x28);
            local_1ac = local_32c;
            local_1a8 = *(undefined8 *)(lVar26 + 0xb0);
            local_1c4 = *(undefined4 *)(lVar26 + 0xf0);
            FUN_00043290(param_2,&local_1c0,&local_1a8,1,local_1e0,lVar16,&local_1ac,&local_1c4,
                         (iVar48 - *(int *)(param_2 + 0x38)) +
                         (iVar12 - *(int *)(param_2 + 0x3c)) * *(int *)(lVar26 + 0xd0));
            if (bVar3) {
              uVar24 = 0;
              do {
                if (0 < (int)(uint)local_1e0) {
                  uVar21 = local_180[uVar24];
                  uVar27 = local_140[uVar24];
                  if ((uint)local_1e0 < 8) {
LAB_00044060:
                    uVar36 = 0;
                  }
                  else {
                    uVar40 = uVar21 + local_1e0 * 4;
                    if (uVar21 < local_350 + local_1e0 * 4 && local_350 < uVar40) goto LAB_00044060;
                    uVar36 = 0;
                    if (uVar27 + local_1e0 * 4 <= uVar21 || uVar40 <= uVar27) {
                      puVar33 = (undefined8 *)(uVar21 + 0x10);
                      puVar37 = (undefined8 *)(uVar27 + 0x10);
                      uVar40 = uVar25;
                      puVar41 = (undefined8 *)(local_350 + 0x10);
                      do {
                        fVar58 = (float)puVar41[-2];
                        fVar59 = (float)((ulong)puVar41[-2] >> 0x20);
                        fVar60 = (float)puVar41[-1];
                        fVar62 = (float)((ulong)puVar41[-1] >> 0x20);
                        fVar63 = (float)*puVar41;
                        fVar65 = (float)((ulong)*puVar41 >> 0x20);
                        fVar68 = (float)puVar41[1];
                        fVar73 = (float)((ulong)puVar41[1] >> 0x20);
                        uVar64 = puVar37[-2];
                        uVar67 = puVar37[1];
                        uVar66 = *puVar37;
                        puVar33[-1] = CONCAT44(((float)((ulong)puVar33[-1] >> 0x20) * fVar62 +
                                               (float)((ulong)puVar37[-1] >> 0x20) *
                                               (255.0 - fVar62)) * 0.003921569,
                                               ((float)puVar33[-1] * fVar60 +
                                               (float)puVar37[-1] * (255.0 - fVar60)) * 0.003921569)
                        ;
                        puVar33[-2] = CONCAT44(((float)((ulong)puVar33[-2] >> 0x20) * fVar59 +
                                               (float)((ulong)uVar64 >> 0x20) * (255.0 - fVar59)) *
                                               0.003921569,
                                               ((float)puVar33[-2] * fVar58 +
                                               (float)uVar64 * (255.0 - fVar58)) * 0.003921569);
                        puVar33[1] = CONCAT44(((float)((ulong)puVar33[1] >> 0x20) * fVar73 +
                                              (float)((ulong)uVar67 >> 0x20) * (255.0 - fVar73)) *
                                              0.003921569,
                                              ((float)puVar33[1] * fVar68 +
                                              (float)uVar67 * (255.0 - fVar68)) * 0.003921569);
                        *puVar33 = CONCAT44(((float)((ulong)*puVar33 >> 0x20) * fVar65 +
                                            (float)((ulong)uVar66 >> 0x20) * (255.0 - fVar65)) *
                                            0.003921569,
                                            ((float)*puVar33 * fVar63 +
                                            (float)uVar66 * (255.0 - fVar63)) * 0.003921569);
                        puVar33 = puVar33 + 4;
                        puVar41 = puVar41 + 4;
                        puVar37 = puVar37 + 4;
                        uVar40 = uVar40 - 8;
                      } while (uVar40 != 0);
                      uVar36 = uVar25;
                      if (uVar25 == local_1e0) goto LAB_00044020;
                    }
                  }
                  lVar39 = uVar36 * 4;
                  lVar26 = local_1e0 - uVar36;
                  pfVar18 = (float *)(uVar21 + lVar39);
                  pfVar28 = (float *)(uVar27 + lVar39);
                  pfVar32 = (float *)(local_350 + lVar39);
                  do {
                    *pfVar18 = (*pfVar18 * *pfVar32 + *pfVar28 * (255.0 - *pfVar32)) * 0.003921569;
                    lVar26 = lVar26 + -1;
                    pfVar18 = pfVar18 + 1;
                    pfVar28 = pfVar28 + 1;
                    pfVar32 = pfVar32 + 1;
                  } while (lVar26 != 0);
                }
LAB_00044020:
                uVar24 = uVar24 + 1;
              } while (uVar24 != local_1d0);
            }
          }
          FUN_00043290(param_2,auStack_160,local_e0 + 4,local_1d0,local_304,lVar16,&local_1ac,
                       &local_f0,iVar53 + local_2b0);
        }
        local_180[0] = (ulong)(uVar34 + iVar50) & 0xff;
        if (bVar6) {
          uVar70 = *(undefined4 *)(param_2 + 0x60);
          uVar71 = *(undefined4 *)(param_2 + 0x6c);
          lVar26 = local_120[local_2c0];
          lVar39 = local_120[local_2b8];
          FUN_009fb560(uVar70,uVar71,local_120[0],lVar26,lVar39,local_248,lVar10);
          FUN_009fb7c8(uVar70,uVar71,local_120[0],lVar26,lVar39,local_248,lVar10,lVar43,lVar38);
        }
        else {
          lVar26 = param_2 + local_1f8 * 0x410 + 0x7c;
          iVar12 = *(int *)(param_2 + 0x58);
          if (iVar12 == 0) {
            if (*(char *)(*(long *)(param_2 + 8) + 0xf4) != '\0') {
              iVar12 = 0;
              goto LAB_000442d8;
            }
            fVar58 = *(float *)(param_2 + 0x60);
            fVar59 = *(float *)(param_2 + 0x6c);
            lVar44 = local_120[local_2c0];
            lVar39 = local_120[local_2b8];
            if (iVar54 == -1) {
              FUN_009fb168(fVar58,fVar58,fVar58,fVar59,fVar59,fVar59,0,local_120[0],lVar44,lVar39,0,
                           lVar10,lVar43,lVar26,local_180);
              if ((int)uVar23 < (int)uVar14) {
                pfVar18 = (float *)(lVar44 + lVar38 * 4);
                pfVar28 = (float *)(lVar39 + lVar38 * 4);
                puVar22 = (uint *)(lVar10 + uVar31);
                lVar10 = lVar17;
                pfVar32 = pfVar2;
                do {
                  fVar60 = *(float *)(param_2 + local_1f8 * 0x410 + local_180[0] * 4 + 0x7c);
                  lVar26 = (long)(float)(int)(fVar59 + fVar58 * *pfVar32 + fVar60);
                  lVar39 = (long)(float)(int)(fVar60 + fVar59 + fVar58 * *pfVar18);
                  lVar44 = (long)(float)(int)(fVar60 + fVar59 + fVar58 * *pfVar28);
                  *puVar22 = ((-(int)(lVar39 >> 0x3f) - 1U & (uint)lVar39 |
                              -(int)(lVar39 + -0xff >> 0x3f) - 1U) & 0xff) << 8 |
                             (0xffU - (int)(lVar26 >> 0x3f) & (uint)lVar26 |
                             -(int)(lVar26 + -0xff >> 0x3f) - 1U) << 0x10 |
                             (-(int)(lVar44 >> 0x3f) - 1U & (uint)lVar44 |
                             -(int)(lVar44 + -0xff >> 0x3f) - 1U) & 0xff | 0xff000000;
                  local_180[0] = (ulong)((int)local_180[0] + 1) & 0xff;
                  lVar10 = lVar10 + -1;
                  pfVar18 = pfVar18 + 1;
                  pfVar28 = pfVar28 + 1;
                  puVar22 = puVar22 + 1;
                  pfVar32 = pfVar32 + 1;
                } while (lVar10 != 0);
              }
            }
            else {
              lVar46 = local_120[iVar54];
              fVar60 = *(float *)(DAT_0155b2d0 + 0x3c8);
              FUN_009fb168(fVar58,fVar58,fVar58,fVar59,fVar59,fVar59,fVar60,local_120[0],lVar44,
                           lVar39,lVar46,lVar10,lVar43,lVar26,local_180);
              if ((int)uVar23 < (int)uVar14) {
                lVar26 = lVar38 * 4;
                pfVar18 = (float *)(lVar46 + lVar26);
                pfVar28 = (float *)(lVar44 + lVar26);
                pfVar32 = (float *)(lVar39 + lVar26);
                puVar22 = (uint *)(lVar10 + uVar31);
                lVar10 = lVar17;
                pfVar42 = pfVar2;
                do {
                  fVar62 = *pfVar18 * 0.003921569;
                  fVar63 = fVar60 * (1.0 - fVar62);
                  fVar65 = *(float *)(param_2 + local_1f8 * 0x410 + local_180[0] * 4 + 0x7c);
                  lVar26 = (long)(float)(int)(fVar63 + fVar62 * (fVar59 + fVar58 * *pfVar42 + fVar65
                                                                ));
                  lVar39 = (long)(float)(int)(fVar63 + fVar62 * (fVar65 + fVar59 + fVar58 * *pfVar28
                                                                ));
                  lVar44 = (long)(float)(int)(fVar63 + fVar62 * (fVar65 + fVar59 + fVar58 * *pfVar32
                                                                ));
                  *puVar22 = ((-(int)(lVar39 >> 0x3f) - 1U & (uint)lVar39 |
                              -(int)(lVar39 + -0xff >> 0x3f) - 1U) & 0xff) << 8 |
                             (0xffU - (int)(lVar26 >> 0x3f) & (uint)lVar26 |
                             -(int)(lVar26 + -0xff >> 0x3f) - 1U) << 0x10 |
                             (-(int)(lVar44 >> 0x3f) - 1U & (uint)lVar44 |
                             -(int)(lVar44 + -0xff >> 0x3f) - 1U) & 0xff | 0xff000000;
                  local_180[0] = (ulong)((int)local_180[0] + 1) & 0xff;
                  lVar10 = lVar10 + -1;
                  pfVar18 = pfVar18 + 1;
                  pfVar28 = pfVar28 + 1;
                  pfVar32 = pfVar32 + 1;
                  puVar22 = puVar22 + 1;
                  pfVar42 = pfVar42 + 1;
                } while (lVar10 != 0);
              }
            }
          }
          else {
            if (iVar12 - 4U < 3) {
              FUN_0097b5d0(local_120[0],local_120[local_2c0],local_120[local_2b8],local_310,
                           local_358,local_328,lVar43);
              iVar12 = *(int *)(param_2 + 0x58);
            }
            lVar39 = local_310;
            if (((iVar12 != 4) && (lVar39 = local_328, iVar12 != 6)) &&
               (lVar39 = local_358, iVar12 != 5)) {
LAB_000442d8:
              lVar39 = local_120[0];
              if (((iVar54 != -1) && (iVar12 == 0)) &&
                 (*(char *)(*(long *)(param_2 + 8) + 0xf4) != '\0')) {
                lVar39 = local_120[iVar54];
                fVar58 = *(float *)(DAT_0155b2d0 + 0x3c8);
                FUN_009fb168(*(undefined4 *)(param_2 + 0x60),*(undefined4 *)(param_2 + 100),
                             *(undefined4 *)(param_2 + 0x68),*(undefined4 *)(param_2 + 0x6c),
                             *(undefined4 *)(param_2 + 0x70),*(undefined4 *)(param_2 + 0x74),fVar58,
                             local_120[0],local_120[0],local_120[0],lVar39,lVar10,lVar43,lVar26,
                             local_180);
                if ((int)uVar23 < (int)uVar14) {
                  fVar59 = *(float *)(param_2 + 0x68);
                  fVar60 = *(float *)(param_2 + 0x6c);
                  fVar62 = *(float *)(param_2 + 0x60);
                  fVar63 = *(float *)(param_2 + 100);
                  fVar65 = *(float *)(param_2 + 0x70);
                  fVar68 = *(float *)(param_2 + 0x74);
                  pfVar18 = (float *)(lVar39 + lVar38 * 4);
                  puVar22 = (uint *)(lVar10 + uVar31);
                  lVar10 = lVar17;
                  pfVar28 = pfVar2;
                  do {
                    fVar73 = *pfVar28;
                    fVar69 = *pfVar18 * 0.003921569;
                    fVar72 = fVar58 * (1.0 - fVar69);
                    fVar74 = *(float *)(param_2 + local_1f8 * 0x410 + local_180[0] * 4 + 0x7c);
                    lVar39 = (long)(float)(int)(fVar72 + fVar69 * (fVar73 * fVar62 + fVar60 + fVar74
                                                                  ));
                    lVar44 = (long)(float)(int)(fVar72 + fVar69 * (fVar74 + fVar73 * fVar63 + fVar65
                                                                  ));
                    lVar26 = (long)(float)(int)(fVar72 + fVar69 * (fVar74 + fVar73 * fVar59 + fVar68
                                                                  ));
                    *puVar22 = ((-(int)(lVar44 >> 0x3f) - 1U & (uint)lVar44 |
                                -(int)(lVar44 + -0xff >> 0x3f) - 1U) & 0xff) << 8 |
                               (0xffU - (int)(lVar39 >> 0x3f) & (uint)lVar39 |
                               -(int)(lVar39 + -0xff >> 0x3f) - 1U) << 0x10 |
                               (-(int)(lVar26 >> 0x3f) - 1U & (uint)lVar26 |
                               -(int)(lVar26 + -0xff >> 0x3f) - 1U) & 0xff | 0xff000000;
                    local_180[0] = (ulong)((int)local_180[0] + 1) & 0xff;
                    lVar10 = lVar10 + -1;
                    pfVar18 = pfVar18 + 1;
                    puVar22 = puVar22 + 1;
                    pfVar28 = pfVar28 + 1;
                  } while (lVar10 != 0);
                }
                goto LAB_000445c8;
              }
            }
            FUN_009fb168(*(undefined4 *)(param_2 + 0x60),*(undefined4 *)(param_2 + 100),
                         *(undefined4 *)(param_2 + 0x68),*(undefined4 *)(param_2 + 0x6c),
                         *(undefined4 *)(param_2 + 0x70),*(undefined4 *)(param_2 + 0x74),0,lVar39,
                         lVar39,lVar39,0,lVar10,lVar43,lVar26,local_180);
            if ((int)uVar23 < (int)uVar14) {
              fVar58 = *(float *)(param_2 + 0x68);
              fVar59 = *(float *)(param_2 + 0x6c);
              fVar60 = *(float *)(param_2 + 0x60);
              fVar62 = *(float *)(param_2 + 100);
              fVar63 = *(float *)(param_2 + 0x70);
              fVar65 = *(float *)(param_2 + 0x74);
              pfVar18 = (float *)(lVar39 + lVar38 * 4);
              puVar22 = (uint *)(lVar10 + uVar31);
              lVar10 = lVar17;
              do {
                fVar68 = *pfVar18;
                fVar73 = *(float *)(param_2 + local_1f8 * 0x410 + local_180[0] * 4 + 0x7c);
                lVar26 = (long)(float)(int)(fVar68 * fVar60 + fVar59 + fVar73);
                lVar39 = (long)(float)(int)(fVar73 + fVar68 * fVar62 + fVar63);
                lVar44 = (long)(float)(int)(fVar73 + fVar68 * fVar58 + fVar65);
                *puVar22 = ((-(int)(lVar39 >> 0x3f) - 1U & (uint)lVar39 |
                            -(int)(lVar39 + -0xff >> 0x3f) - 1U) & 0xff) << 8 |
                           (0xffU - (int)(lVar26 >> 0x3f) & (uint)lVar26 |
                           -(int)(lVar26 + -0xff >> 0x3f) - 1U) << 0x10 |
                           (-(int)(lVar44 >> 0x3f) - 1U & (uint)lVar44 |
                           -(int)(lVar44 + -0xff >> 0x3f) - 1U) & 0xff | 0xff000000;
                local_180[0] = (ulong)((int)local_180[0] + 1) & 0xff;
                lVar10 = lVar10 + -1;
                pfVar18 = pfVar18 + 1;
                puVar22 = puVar22 + 1;
              } while (lVar10 != 0);
            }
          }
        }
      }
LAB_000445c8:
      if (iVar11 <= (int)uVar57) break;
      uVar29 = (uint)(CONCAT44((uVar29 + (uVar29 >> 0x10)) * 0x101 + (uVar29 >> 0x18),uVar29) >>
                     0x10);
      local_1e4 = local_1e4 + 1;
      uVar24 = uVar57;
    } while (local_1e4 < local_234);
  }
  for (; uVar55 != 0; uVar55 = uVar55 - 1) {
    FUN_009d8290(uVar56);
  }
LAB_00044928:
  if (*(long *)PTR____stack_chk_guard_013b4320 == local_98) {
    return;
  }
                    /* WARNING: Subroutine does not return */
  ___stack_chk_fail();
}


