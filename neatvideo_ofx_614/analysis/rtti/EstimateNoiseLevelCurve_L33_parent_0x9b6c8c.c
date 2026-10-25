// FUN_009b6c8c @ 009b6c8c size=2400 tag=EstimateNoiseLevelCurve_L33_parent

/* WARNING: Type propagation algorithm not settling */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

ulong FUN_009b6c8c(undefined1 param_1 [16],double param_2,undefined8 param_3,undefined8 param_4,
                  long *param_5)

{
  undefined8 uVar1;
  int iVar2;
  ulong uVar4;
  long extraout_x8;
  long extraout_x8_00;
  long lVar5;
  long lVar6;
  long extraout_x8_01;
  long extraout_x8_02;
  long extraout_x8_03;
  long extraout_x8_04;
  undefined1 *puVar7;
  undefined1 *puVar8;
  double *pdVar9;
  long *plVar10;
  long lVar11;
  long lVar12;
  long lVar13;
  double *pdVar14;
  double *pdVar15;
  long lVar16;
  double *pdVar17;
  long lVar18;
  long **pplVar19;
  undefined1 *puVar20;
  undefined1 *puVar21;
  undefined1 *puVar22;
  long *plVar23;
  float fVar24;
  double dVar25;
  double dVar26;
  undefined4 uVar27;
  undefined4 uVar28;
  undefined1 auVar29 [16];
  undefined1 auVar30 [16];
  undefined1 auVar31 [16];
  long *local_5e00 [6];
  long *plStack_5dd0;
  undefined1 **local_5dc8;
  long *plStack_5dc0;
  undefined4 *local_5db8;
  long *plStack_5db0;
  undefined4 local_5da8 [5940];
  long local_d8;
  long local_d0;
  undefined1 *local_c8;
  undefined1 *local_c0;
  long local_b8;
  long local_b0;
  void *local_a8;
  undefined1 *local_a0;
  undefined1 auStack_98 [8];
  long local_90;
  undefined4 local_88 [6];
  long local_70;
  undefined8 uVar3;
  
  auVar31 = (*(code *)PTR____chkstk_darwin_013b4308)();
  lVar6 = DAT_014a67c0;
  local_90 = auVar31._8_8_;
  uVar3 = auVar31._0_8_;
  pplVar19 = local_5e00;
  local_70 = *(long *)PTR____stack_chk_guard_013b4320;
  local_b0 = DAT_014a67c0;
  if (DAT_014a67c0 < 1) {
    local_c0 = auStack_98;
    local_a8 = (void *)0x0;
    if (DAT_014a67c0 != 0) goto LAB_009b6d28;
  }
  else {
    local_a8 = _malloc(DAT_014a67c0 * 0x50 | 8);
    local_c0 = (undefined1 *)((long)local_a8 + 7U & 0xfffffffffffffff8);
LAB_009b6d28:
    lVar16 = 0;
    local_a0 = local_c0;
    do {
      thunk_FUN_009d7bc4(local_c0 + lVar16);
      lVar16 = lVar16 + 0x50;
    } while (lVar6 * 0x50 - lVar16 != 0);
  }
  local_b8 = 0;
  lVar16 = *(long *)(local_90 + 0xd0);
  lVar18 = *(long *)(local_90 + 0xd8);
  local_c8 = DAT_012445a0;
  local_a0 = local_c0;
  FUN_009c80d4(&local_c0,0xb31fd8c7);
  local_5da8[0] = 0;
  FUN_009f13bc(local_d0,local_5da8,lVar6 * 8 * (long)local_c8);
  local_5da8[0] = 0;
  FUN_009f13bc(local_d8,local_5da8,lVar6 * 8 * (long)local_c8);
  FUN_009b4428(uVar3);
  thunk_FUN_00ac0ff0(local_5da8);
  FUN_00ac1194(local_5da8,lVar16,lVar18,DAT_01244570,DAT_01244570,DAT_01244578,0,0x30,0,0,lVar16,
               lVar18,0);
  FUN_00ac0d94(local_5da8);
  local_5e00[3] = (long *)&DAT_013fd330;
  local_5e00[5] = &local_90;
  plStack_5dd0 = &local_d0;
  local_5dc8 = &local_c8;
  plStack_5dc0 = &local_d8;
  plStack_5db0 = &local_b0;
  local_5e00[4] = (long *)uVar3;
  local_5db8 = local_5da8;
  if (((DAT_015487f0 & 1) == 0) && (iVar2 = ___cxa_guard_acquire(&DAT_015487f0), iVar2 != 0)) {
    thunk_FUN_009fb9f0(&DAT_01547df8,"EstimateNoiseLevelCurve");
    ___cxa_guard_release(&DAT_015487f0);
  }
  lVar18 = lVar18 * lVar16;
  uVar4 = FUN_009fc6e8(local_5e00 + 3,lVar6,&DAT_01547df8,*(long *)(local_90 + 0xe8) * lVar18);
  plVar23 = (long *)local_c8;
  if ((uVar4 & 1) == 0) goto LAB_009b75e4;
  (*(code *)PTR____chkstk_darwin_013b4308)((long)local_c8 * 8 + 0xfU & 0xfffffffffffffff0);
  lVar16 = (long)local_5e00 + -extraout_x8;
  (*(code *)PTR____chkstk_darwin_013b4308)((long)local_c8 * 8 + 0xfU & 0xfffffffffffffff0);
  pdVar15 = (double *)(lVar16 - extraout_x8_00);
  local_88[0] = 0;
  FUN_009f13bc(lVar16,local_88);
  local_88[0] = 0;
  dVar25 = (double)FUN_009f13bc(pdVar15,local_88,(long)local_c8 << 3);
  puVar22 = local_c8;
  if ((long)local_c8 < 1) {
    (*(code *)PTR____chkstk_darwin_013b4308)((long)local_c8 * 8 + 0xfU & 0xfffffffffffffff0);
    lVar6 = (long)pdVar15 - extraout_x8_01;
    (*(code *)PTR____chkstk_darwin_013b4308)();
    pdVar17 = (double *)(lVar6 - extraout_x8_02);
    dVar25 = 1.0;
joined_r0x009b6fe0:
    if (0 < (long)puVar22) {
LAB_009b7384:
      if ((undefined1 *)((long)&MACH_HEADER.magic + 3) < puVar22) {
        puVar7 = (undefined1 *)((ulong)puVar22 & 0xfffffffffffffffc);
        pdVar9 = pdVar15 + 2;
        puVar8 = puVar7;
        do {
          pdVar9[-1] = dVar25;
          pdVar9[-2] = dVar25;
          pdVar9[1] = dVar25;
          *pdVar9 = dVar25;
          pdVar9 = pdVar9 + 4;
          puVar8 = puVar8 + -4;
          if (puVar8 == (undefined1 *)0x0) goto LAB_009b76a4;
        } while( true );
      }
      puVar7 = (undefined1 *)0x0;
      do {
        pdVar15[(long)puVar7] = dVar25;
        puVar7 = puVar7 + 1;
LAB_009b76a4:
      } while (puVar22 != puVar7);
    }
  }
  else {
    if (0 < lVar6) {
      lVar5 = 0;
      do {
        lVar11 = 0;
        dVar25 = pdVar15[lVar5];
        lVar12 = *(long *)(lVar16 + lVar5 * 8);
        lVar13 = lVar6;
        do {
          param_2 = *(double *)(local_d0 + lVar11);
          dVar25 = param_2 + dVar25;
          pdVar15[lVar5] = dVar25;
          lVar12 = lVar12 + *(long *)(local_d8 + lVar11);
          *(long *)(lVar16 + lVar5 * 8) = lVar12;
          lVar11 = lVar11 + (long)local_c8 * 8;
          lVar13 = lVar13 + -1;
        } while (lVar13 != 0);
        lVar5 = lVar5 + 1;
        local_d8 = local_d8 + 8;
        local_d0 = local_d0 + 8;
      } while (lVar5 < (long)local_c8);
    }
    lVar6 = lVar18 + 0x3ff;
    if (-1 < lVar18) {
      lVar6 = lVar18;
    }
    lVar6 = lVar6 >> 10;
    if ((undefined1 *)((long)&MACH_HEADER.magic + 1) < local_c8) {
      puVar8 = (undefined1 *)((ulong)local_c8 & 0xfffffffffffffffe);
      plVar10 = (long *)((long)local_5e00 + -extraout_x8 + 8);
      pdVar17 = pdVar15 + 1;
      puVar7 = puVar8;
      do {
        lVar18 = plVar10[-1];
        lVar5 = *plVar10;
        if (lVar6 <= lVar18) {
          dVar25 = pdVar17[-1];
        }
        if (lVar6 <= lVar5) {
          param_2 = *pdVar17;
        }
        if (lVar6 <= lVar18) {
          dVar25 = dVar25 / (double)lVar18;
          pdVar17[-1] = dVar25;
        }
        if (lVar6 <= lVar5) {
          dVar25 = param_2 / (double)lVar5;
          *pdVar17 = dVar25;
        }
        if (lVar18 < lVar6) {
          pdVar17[-1] = -1.0;
        }
        if (lVar5 < lVar6) {
          *pdVar17 = -1.0;
        }
        plVar10 = plVar10 + 2;
        pdVar17 = pdVar17 + 2;
        puVar7 = puVar7 + -2;
      } while (puVar7 != (undefined1 *)0x0);
      if (local_c8 != puVar8) goto LAB_009b70a4;
    }
    else {
      puVar8 = (undefined1 *)0x0;
LAB_009b70a4:
      lVar18 = (long)local_c8 - (long)puVar8;
      plVar10 = (long *)(lVar16 + (long)puVar8 * 8);
      pdVar17 = pdVar15 + (long)puVar8;
      do {
        while (lVar6 <= *plVar10) {
          *pdVar17 = *pdVar17 / (double)*plVar10;
          plVar10 = plVar10 + 1;
          lVar18 = lVar18 + -1;
          pdVar17 = pdVar17 + 1;
          if (lVar18 == 0) goto LAB_009b70f4;
        }
        *pdVar17 = -1.0;
        plVar10 = plVar10 + 1;
        lVar18 = lVar18 + -1;
        pdVar17 = pdVar17 + 1;
      } while (lVar18 != 0);
    }
LAB_009b70f4:
    local_5e00[1] = plVar23;
    local_5e00[2] = (long *)local_5e00;
    (*(code *)PTR____chkstk_darwin_013b4308)((long)local_c8 * 8 + 0xfU & 0xfffffffffffffff0);
    lVar6 = (long)pdVar15 - extraout_x8_03;
    (*(code *)PTR____chkstk_darwin_013b4308)();
    pdVar17 = (double *)(lVar6 - extraout_x8_04);
    puVar7 = (undefined1 *)0x0;
    puVar8 = (undefined1 *)0x0;
    puVar21 = (undefined1 *)0xffffffffffffffff;
    do {
      dVar25 = pdVar15[(long)puVar7];
      puVar20 = puVar21;
      if (0.0 < dVar25) {
        puVar20 = puVar7;
        if (-1 < (long)puVar21) {
          puVar20 = puVar21;
        }
        *(double *)(lVar6 + (long)puVar8 * 8) = (double)(long)puVar7;
        pdVar17[(long)puVar8] = dVar25;
        puVar8 = puVar8 + 1;
        pplVar19 = (long **)puVar7;
      }
      puVar7 = puVar7 + 1;
      puVar21 = puVar20;
    } while (puVar22 != puVar7);
    plVar23 = local_5e00[1];
    if (puVar8 <= (undefined1 *)((long)&MACH_HEADER.magic + 1)) {
      if (puVar8 != (undefined1 *)0x0) {
        dVar25 = *pdVar17;
        goto joined_r0x009b6fe0;
      }
      dVar25 = 1.0;
      if ((long)puVar22 < 1) goto LAB_009b74a8;
      goto LAB_009b7384;
    }
    if (puVar8 != puVar22) {
      uVar3 = 2;
      if (2 < (long)puVar8) {
        uVar3 = 3;
      }
      local_5e00[0] = param_5;
      FUN_009d43f4(lVar6,pdVar17,puVar8,local_88,uVar3);
      puVar22 = puVar20 + 1;
      if ((long)puVar22 < (long)pplVar19) {
        do {
          if (pdVar15[(long)puVar22] <= 0.0) {
            dVar25 = (double)FUN_009d4890((double)(long)puVar22,local_88,uVar3);
            pdVar15[(long)puVar22] = dVar25;
          }
          puVar22 = puVar22 + 1;
        } while (pplVar19 != (long **)puVar22);
      }
      if (0 < (long)puVar20) {
        lVar16 = SUB168(SEXT816((long)local_c8) * SEXT816(0x5555555555555556),8);
        lVar16 = lVar16 - (lVar16 >> 0x3f);
        uVar3 = 2;
        if (2 < (long)puVar8) {
          uVar3 = 3;
        }
        if (lVar16 <= (long)puVar20) {
          uVar3 = 1;
        }
        uVar1 = 1;
        if ((long)puVar20 < lVar16) {
          uVar1 = 2;
        }
        FUN_009d43f4(lVar6,pdVar17,uVar3,local_88,uVar1);
        puVar22 = puVar20 + 1;
        pdVar9 = pdVar15 + (long)puVar20;
        do {
          pdVar14 = pdVar9 + -1;
          dVar25 = (double)FUN_009d4890((double)(long)(puVar22 + -2),local_88,uVar1);
          *pdVar14 = dVar25;
          if (dVar25 <= 0.0) {
            *pdVar14 = *pdVar9;
          }
          puVar22 = puVar22 + -1;
          pdVar9 = pdVar14;
        } while (1 < (long)puVar22);
      }
      puVar7 = (undefined1 *)((long)pplVar19 + 1);
      puVar22 = local_c8;
      param_5 = local_5e00[0];
      plVar23 = local_5e00[1];
      if ((long)puVar7 < (long)local_c8) {
        lVar16 = SUB168(SEXT816((long)local_c8 << 1) * SEXT816(0x5555555555555556),8);
        lVar16 = lVar16 - (lVar16 >> 0x3f);
        lVar18 = 2;
        if (2 < (long)puVar8) {
          lVar18 = 3;
        }
        if ((long)pplVar19 < lVar16) {
          lVar18 = 1;
        }
        uVar3 = 1;
        if (lVar16 <= (long)pplVar19) {
          uVar3 = 2;
        }
        FUN_009d43f4(lVar6 + (long)puVar8 * 8 + lVar18 * -8,pdVar17 + ((long)puVar8 - lVar18),lVar18
                     ,local_88,uVar3);
        puVar22 = local_c8;
        param_5 = local_5e00[0];
        plVar23 = local_5e00[1];
        if ((long)puVar7 < (long)local_c8) {
          pdVar9 = pdVar15 + (long)pplVar19;
          do {
            pdVar14 = pdVar9 + 1;
            dVar25 = (double)FUN_009d4890((double)(long)puVar7,local_88,uVar3);
            *pdVar14 = dVar25;
            if (dVar25 <= 0.0) {
              *pdVar14 = *pdVar9;
            }
            puVar7 = puVar7 + 1;
            puVar22 = local_c8;
            param_5 = local_5e00[0];
            plVar23 = local_5e00[1];
            pdVar9 = pdVar14;
          } while ((long)puVar7 < (long)local_c8);
        }
      }
    }
  }
LAB_009b74a8:
  dVar25 = *pdVar15;
  *pdVar17 = dVar25;
  pdVar17[(long)(puVar22 + -1)] = pdVar15[(long)(puVar22 + -1)];
  if (2 < (long)puVar22) {
    uVar27 = SUB84(dVar25,0);
    uVar28 = (undefined4)((ulong)dVar25 >> 0x20);
    puVar7 = puVar22 + -2;
    pdVar9 = pdVar15 + 2;
    dVar25 = pdVar15[1];
    pdVar15 = pdVar17;
    do {
      pdVar15 = pdVar15 + 1;
      dVar26 = *pdVar9;
      *pdVar15 = (dVar25 + ((double)CONCAT44(uVar28,uVar27) + dVar26) * 0.5) * 0.5;
      uVar27 = SUB84(dVar25,0);
      uVar28 = (undefined4)((ulong)dVar25 >> 0x20);
      puVar7 = puVar7 + -1;
      pdVar9 = pdVar9 + 1;
      dVar25 = dVar26;
    } while (puVar7 != (undefined1 *)0x0);
  }
  if (0 < (long)puVar22) {
    fVar24 = 256.0 / (float)(long)plVar23;
    if ((undefined1 *)((long)&MACH_HEADER.magic + 3) < puVar22) {
      puVar7 = (undefined1 *)((ulong)puVar22 & 0xfffffffffffffffc);
      pdVar15 = (double *)(lVar6 + 0x10);
      puVar8 = puVar7;
      lVar18 = _UNK_00c95958;
      lVar16 = _DAT_00c95950;
      do {
        auVar29._0_8_ = lVar16 + 2;
        auVar29._8_8_ = lVar18 + 2;
        auVar31._8_8_ = lVar18;
        auVar31._0_8_ = lVar16;
        auVar30 = NEON_scvtf(auVar31,8);
        auVar31 = NEON_scvtf(auVar29,8);
        pdVar15[-1] = (double)(fVar24 * ((float)auVar30._8_8_ + 0.5));
        pdVar15[-2] = (double)(fVar24 * ((float)auVar30._0_8_ + 0.5));
        pdVar15[1] = (double)(fVar24 * ((float)auVar31._8_8_ + 0.5));
        *pdVar15 = (double)(fVar24 * ((float)auVar31._0_8_ + 0.5));
        lVar16 = lVar16 + 4;
        lVar18 = lVar18 + 4;
        pdVar15 = pdVar15 + 4;
        puVar8 = puVar8 + -4;
      } while (puVar8 != (undefined1 *)0x0);
      if (puVar22 == puVar7) goto LAB_009b75c8;
    }
    else {
      puVar7 = (undefined1 *)0x0;
    }
    do {
      *(double *)(lVar6 + (long)puVar7 * 8) = (double)(fVar24 * ((float)(long)puVar7 + 0.5));
      puVar7 = puVar7 + 1;
    } while (puVar22 != puVar7);
  }
LAB_009b75c8:
  (**(code **)(*param_5 + 0x18))(param_5,lVar6,pdVar17);
LAB_009b75e4:
  FUN_00ac12c4(local_5da8);
  if (0 < local_b8) {
    lVar6 = 0;
    do {
      FUN_009d8290(local_c0);
      lVar6 = lVar6 + 1;
    } while (lVar6 < local_b8);
  }
  if (0 < local_b0) {
    lVar16 = 0;
    lVar6 = 0;
    do {
      thunk_FUN_009d841c(local_a0 + lVar16);
      lVar6 = lVar6 + 1;
      lVar16 = lVar16 + 0x50;
    } while (lVar6 < local_b0);
  }
  if (local_a8 != (void *)0x0) {
    _free(local_a8);
  }
  if (*(long *)PTR____stack_chk_guard_013b4320 != local_70) {
                    /* WARNING: Subroutine does not return */
    ___stack_chk_fail();
  }
  return uVar4;
}


