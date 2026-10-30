// FUN_009b1a5c @ 009b1a5c size=3400 tag=FindFlatArea_L34_parent

undefined4
FUN_009b1a5c(undefined8 param_1,undefined8 param_2,uint param_3,ulong *param_4,ulong *param_5,
            long *param_6,long *param_7)

{
  undefined8 *puVar1;
  byte bVar2;
  ulong uVar3;
  ulong uVar4;
  ulong uVar5;
  ulong uVar6;
  uint uVar7;
  long lVar8;
  bool bVar9;
  bool bVar10;
  double dVar11;
  bool bVar12;
  undefined8 *puVar13;
  undefined8 uVar14;
  undefined8 uVar15;
  undefined8 uVar16;
  undefined8 uVar17;
  undefined8 uVar18;
  ulong uVar19;
  ulong uVar20;
  bool bVar21;
  undefined4 uVar22;
  undefined8 *puVar23;
  ulong uVar24;
  long lVar25;
  undefined4 uVar26;
  ulong uVar27;
  long lVar28;
  ulong uVar29;
  long lVar30;
  long lVar31;
  int iVar32;
  ulong uVar33;
  long lVar34;
  float fVar35;
  float fVar36;
  float fVar37;
  double dVar38;
  float fVar39;
  float fVar40;
  float fVar41;
  float fVar42;
  float fVar43;
  undefined1 auVar44 [16];
  char *local_6738;
  long local_6730;
  undefined *local_66e8;
  undefined8 *puStack_66e0;
  undefined8 *local_66d8;
  long *plStack_66d0;
  long *local_66c8;
  float *pfStack_66c0;
  undefined *local_66b8;
  undefined8 *puStack_66b0;
  undefined8 *local_66a8;
  long *plStack_66a0;
  long *local_6698;
  float *pfStack_6690;
  undefined *local_6688;
  undefined8 *local_6680;
  undefined8 *puStack_6678;
  long *local_6670;
  long *plStack_6668;
  float *local_6660;
  float local_6654;
  float local_6650;
  float local_664c;
  long local_6648;
  long local_6640;
  undefined1 auStack_6638 [264];
  float local_6530;
  float local_652c;
  float local_6528;
  float local_6524;
  float local_6520;
  float local_45d8;
  float local_45d4;
  float local_45d0;
  float local_45cc;
  float local_45c8;
  float local_2680;
  float local_267c;
  float local_2678;
  float local_2674;
  float local_2670;
  undefined *local_728 [8];
  undefined8 local_6e8;
  undefined8 local_6e0;
  undefined8 local_6d8;
  undefined8 local_6d0;
  undefined **local_6c8;
  undefined **local_6c0;
  undefined **local_6b8;
  float local_6b0 [64];
  float local_5b0 [64];
  long local_4b0 [64];
  long local_2b0 [64];
  long local_b0;
  
  auVar44 = (*(code *)PTR____chkstk_darwin_013b4308)();
  lVar25 = auVar44._8_8_;
  puVar13 = auVar44._0_8_;
  local_b0 = *(long *)PTR____stack_chk_guard_013b4320;
  *(undefined1 *)((long)puVar13 + 0x81) = 0;
  uVar7 = *(uint *)(DAT_014885c0 + 0x256f28);
  iVar32 = *(int *)(DAT_014885d0 + 0x161f9f8);
  if (iVar32 != 1) {
    if (DAT_014881dc != 0) {
      uVar7 = uVar7 >> 0x1d | uVar7 << 3;
    }
    *(uint *)(DAT_014885c8 + 0x8fa5f8) = uVar7;
  }
  puVar1 = puVar13 + 1;
  DAT_014881dc = iVar32;
  FUN_00a22470(puVar1);
  puVar23 = puVar13 + 0xe;
  *puVar23 = 0;
  puVar13[0xf] = 0;
  uVar19 = *(ulong *)(lVar25 + 0xd0);
  uVar20 = *(ulong *)(lVar25 + 0xd8);
  uVar33 = 7;
  do {
    uVar33 = uVar33 + 1;
    lVar8 = 0;
    if (uVar33 != 0) {
      lVar8 = (long)uVar19 / (long)uVar33;
    }
    lVar34 = 0;
    if (uVar33 != 0) {
      lVar34 = (long)uVar20 / (long)uVar33;
    }
    lVar30 = lVar34 * lVar8;
  } while (((4 < lVar34) && (4 < lVar8)) && (0x40000 < lVar30));
  uVar7 = *(uint *)(DAT_01488de0 + 0x982378);
  if (DAT_01488344 != 0) {
    uVar7 = uVar7 >> 0xe | uVar7 << 0x12;
  }
  if (uVar7 != 1) {
    *(uint *)(DAT_01488de8 + 0x1c55c08) = uVar7;
  }
  if ((lVar8 != 0) && (lVar34 != 0)) {
    lVar28 = uVar20 * uVar19;
    if (lVar28 < 1) {
      local_6738 = "6astfqg";
    }
    else if (lVar28 < 0x280001) {
      local_6738 = "";
    }
    else {
      local_6738 = "";
      if (lVar28 < 0xa00001) {
        local_6738 = "";
      }
    }
    if (param_3 == 0) {
      lVar28 = 0;
      uVar29 = *(ulong *)(local_6738 + 0x18);
      do {
        if ((uVar29 <= uVar19) && (uVar29 <= uVar20)) goto LAB_009b1ca4;
        uVar29 = *(ulong *)(local_6738 + lVar28 * 8 + 0x20);
        lVar28 = lVar28 + 1;
      } while (uVar29 != 0);
    }
    else {
      uVar29 = (ulong)((float)(long)uVar19 * 0.18);
      if (0x3f < (long)uVar29) {
        lVar28 = 0;
LAB_009b1ca4:
        local_6d0 = 0;
        lVar31 = uVar29 * uVar29 * 4;
        local_6d8 = FUN_009d7200(lVar31,0x1041d06c);
        local_6e0 = FUN_009d7200(lVar31,0x848fb0c1);
        local_6e8 = FUN_009d7200(lVar31,0x49f00815);
        lVar31 = lVar30 * 4;
        uVar14 = FUN_009d7200(lVar31,0x9501fac1);
        uVar15 = FUN_009d7200(lVar31,0x81b850cd);
        uVar16 = FUN_009d7200(lVar31,0x901476c1);
        uVar17 = FUN_009d7200(lVar31,0x93bf8c90);
        uVar18 = FUN_00a22934(0x3f4ccccd,puVar1,*(long *)(lVar25 + 0xd8) * *(long *)(lVar25 + 0xd0))
        ;
        puVar13[0xe] = uVar18;
        FUN_00aed9f0(local_728);
        uVar19 = FUN_009b6c8c(puVar13,lVar25,local_728);
        if ((uVar19 & 1) == 0) {
LAB_009b2734:
          uVar22 = 5;
        }
        else {
          uVar18 = FUN_00a22934(0x3dcccccd,puVar1,10);
          *puVar23 = uVar18;
          FUN_00a22868(uVar18,3);
          FUN_009b7e58(lVar25,uVar15,uVar16,uVar17,uVar33);
          FUN_00a22868(*puVar23,4);
          FUN_0097bb88(uVar15,uVar16,uVar17,uVar14,lVar30);
          FUN_00a22868(*puVar23,3);
          uVar7 = DAT_014884a2;
          if (DAT_01488348 >> 0x1e == 0) {
            uVar19 = (ulong)DAT_014884a2;
            *(undefined **)(DAT_01488df8 + 0x1371da28) = &DAT_014884a0;
            if (*(long *)(DAT_01488df0 + 0x26d6bec0) != 0) {
              uVar19 = -(ulong)*(byte *)(*(long *)(DAT_01488df0 + 0x26d6bec0) + (uVar19 & 0xffff));
            }
            lVar34 = lVar34 + ((ulong)(uVar7 >> 0x18) + (uVar19 + (uVar7 >> 0x10 & 0xff)) * 2) *
                              0x40;
          }
          DAT_01488348 = (int)(CONCAT44((DAT_01488348 + (DAT_01488348 >> 0x10)) * 0x101 +
                                        (DAT_01488348 >> 0x18),DAT_01488348) >> 0x10) + (int)uVar17;
          FUN_0099ce14(&local_2680);
          FUN_0099ce14(&local_45d8);
          FUN_0099ce14(&local_6530);
          if ((param_3 & 1) == 0) {
            local_6730 = *(long *)(local_6738 + 0x10) - lVar28;
          }
          else {
            local_6730 = 1;
          }
          uVar18 = FUN_00a22934(0x3dcccccd,puVar1);
          bVar9 = false;
          bVar12 = false;
          bVar10 = false;
          puVar13[0xe] = uVar18;
          dVar11 = DAT_0127a7a0;
          fVar43 = 10000.0;
          uVar19 = 0;
          if (uVar33 != 0) {
            uVar19 = 0x10 / uVar33;
          }
          while (0 < (long)uVar29) {
            uVar20 = (**(code **)*puVar13)();
            if ((uVar20 & 1) != 0) goto LAB_009b2734;
            lVar30 = (long)(((float)(long)uVar29 / (float)(long)uVar33 + -1.0) * 0.5);
            if (lVar30 < 2) {
              lVar30 = 1;
            }
            uVar20 = uVar19;
            if (uVar19 < 2) {
              uVar20 = 1;
            }
            FUN_009999cc(&local_6d0,uVar14,lVar8,lVar34,lVar30,lVar30,uVar20,0x10,4,local_2b0,
                         local_4b0,local_6b0,local_5b0);
            lVar30 = 0;
            while (0.0 <= local_5b0[lVar30]) {
              uVar27 = -(uVar29 >> 1) + local_2b0[lVar30] * uVar33;
              uVar27 = uVar27 & ((long)uVar27 >> 0x3f ^ 0xffffffffffffffffU);
              uVar24 = -(uVar29 >> 1) + local_4b0[lVar30] * uVar33;
              uVar24 = uVar24 & ((long)uVar24 >> 0x3f ^ 0xffffffffffffffffU);
              uVar20 = uVar27 + uVar29;
              uVar5 = *(ulong *)(lVar25 + 0xd0);
              uVar6 = *(ulong *)(lVar25 + 0xd8);
              uVar3 = uVar27;
              if (uVar29 <= uVar5) {
                uVar3 = uVar5 - uVar29;
              }
              if ((long)uVar20 <= (long)uVar5) {
                uVar3 = uVar27;
              }
              uVar27 = uVar24 + uVar29;
              uVar4 = uVar24;
              if (uVar29 <= uVar6) {
                uVar4 = uVar6 - uVar29;
              }
              if ((long)uVar27 <= (long)uVar6) {
                uVar4 = uVar24;
              }
              FUN_001c1114(auStack_6638);
              if ((long)uVar27 <= (long)uVar6) {
                uVar6 = uVar27;
              }
              if ((long)uVar20 <= (long)uVar5) {
                uVar5 = uVar20;
              }
              local_6640 = uVar5 - uVar3;
              local_6648 = uVar6 - uVar4;
              FUN_009c7318(lVar25,uVar3,uVar4,local_6640,local_6648,auStack_6638,
                           *(undefined4 *)(lVar25 + 0xf0),1);
              FUN_009c6648(auStack_6638,&local_664c,&local_6650,&local_6654,1);
              bVar2 = (0.33 < local_6650) + (0.33 < local_664c) + (0.33 < local_6654);
              if (bVar2 < 2) {
                FUN_009c8c24(auStack_6638,local_6d8,local_6e0,local_6e8,1);
                (**(code **)(&DAT_0147df50 + (ulong)*(uint *)(puVar13[0x7f2] + 0x48) * 8))();
                local_6688 = &DAT_013fd3c0;
                puStack_66e0 = &local_6d0;
                puStack_6678 = &local_6d8;
                plStack_66d0 = &local_6640;
                local_66c8 = &local_6648;
                local_6660 = &local_2680;
                local_66b8 = &DAT_013fd3f0;
                local_66a8 = &local_6e0;
                pfStack_6690 = &local_45d8;
                local_66e8 = &DAT_013fd420;
                local_66d8 = &local_6e8;
                pfStack_66c0 = &local_6530;
                local_6c8 = &local_6688;
                local_6c0 = &local_66b8;
                local_6b8 = &local_66e8;
                puStack_66b0 = puStack_66e0;
                plStack_66a0 = plStack_66d0;
                local_6698 = local_66c8;
                local_6680 = puStack_66e0;
                local_6670 = plStack_66d0;
                plStack_6668 = local_66c8;
                FUN_009fc3a8(&local_6c8,3,DAT_014a67c0,0,0);
                lVar31 = (ulong)*(uint *)(puVar13[0x7f2] + 0x48) * 3;
                fVar35 = (float)FUN_009b096c(&local_2680,(&DAT_0147dfc8)[lVar31]);
                fVar36 = (float)FUN_009b096c(&local_45d8,(&DAT_0147dfc9)[lVar31]);
                fVar37 = (float)FUN_009b096c(&local_6530,(&DAT_0147dfca)[lVar31]);
                dVar38 = (double)FUN_00aedb18((double)local_6b0[lVar30],local_728);
                if ((float)dVar38 < 1e-06) goto LAB_009b22c8;
                fVar40 = 0.0;
                if ((local_2680 < 0.0) || (local_45d8 < 0.0)) {
                  fVar39 = 0.0;
                }
                else {
                  fVar39 = 0.0;
                  if (0.0 <= local_6530) {
                    fVar42 = ABS(local_2680) + ABS(local_45d8) + ABS(local_6530);
                    fVar40 = 0.0;
                    fVar39 = fVar42 + 0.0;
                    if (((local_267c < 0.0) || (local_45d4 < 0.0)) || (local_652c < 0.0)) {
                      lVar31 = 1;
                    }
                    else {
                      fVar41 = ABS(local_267c) + ABS(local_45d4) + ABS(local_652c);
                      fVar39 = fVar39 + fVar41 + fVar41;
                      fVar40 = fVar42 / fVar41 + 0.0;
                      if (((local_2678 < 0.0) || (local_45d0 < 0.0)) || (local_6528 < 0.0)) {
                        lVar31 = 2;
                      }
                      else {
                        fVar42 = ABS(local_2678) + ABS(local_45d0) + ABS(local_6528);
                        fVar39 = fVar39 + fVar42 * 4.0;
                        fVar40 = fVar40 + fVar41 / fVar42;
                        if (((local_2674 < 0.0) || (local_45cc < 0.0)) || (local_6524 < 0.0)) {
                          lVar31 = 3;
                        }
                        else {
                          fVar41 = ABS(local_2674) + ABS(local_45cc) + ABS(local_6524);
                          fVar39 = fVar39 + fVar41 * 8.0;
                          fVar40 = fVar40 + fVar42 / fVar41;
                          if (((local_2670 < 0.0) || (local_45c8 < 0.0)) || (local_6520 < 0.0)) {
                            lVar31 = 4;
                          }
                          else {
                            fVar42 = ABS(local_2670) + ABS(local_45c8) + ABS(local_6520);
                            fVar39 = fVar39 + fVar42 * 16.0;
                            fVar40 = fVar40 + fVar41 / fVar42;
                            lVar31 = 5;
                          }
                        }
                      }
                      fVar40 = fVar40 / (float)(lVar31 + -1);
                    }
                    fVar39 = fVar39 / (float)lVar31;
                  }
                }
                if ((dVar11 < (double)local_664c) || (dVar11 < (double)local_6650)) {
                  bVar21 = true;
                }
                else {
                  bVar21 = dVar11 < (double)local_6654;
                }
                fVar40 = 2.0 - fVar40;
                if (fVar40 <= 0.0) {
                  fVar40 = 0.0;
                }
                fVar40 = (fVar40 + fVar40 + 1.0) *
                         *(float *)(local_6738 + lVar28 * 4 + 0x58) *
                         ((float)bVar2 * 4.0 + 1.0) *
                         fVar35 * fVar36 * fVar37 * (fVar39 / (float)dVar38) *
                         (ABS(local_6b0[lVar30] + -96.0) * 0.001953125 + 1.0) *
                         ((float)bVar21 + 1.0);
                iVar32 = 0;
                if (fVar40 < fVar43) {
                  *param_4 = uVar3;
                  *param_5 = uVar4;
                  *param_6 = local_6640;
                  *param_7 = local_6648;
                  bVar9 = (fVar35 < 2.0 && fVar36 < 2.0) && fVar37 < 2.0;
                  bVar12 = bVar2 != 0;
                  bVar10 = true;
                  fVar43 = fVar40;
                }
              }
              else {
LAB_009b22c8:
                iVar32 = 0x11;
              }
              thunk_FUN_009c3cdc(auStack_6638);
              if (((iVar32 != 0x11) && (iVar32 != 0)) || (lVar30 = lVar30 + 1, lVar30 == 0x40))
              break;
            }
            FUN_00a22868(*puVar23,1);
            local_6730 = local_6730 + -1;
            if ((param_3 & 1) != 0) break;
            lVar28 = lVar28 + 1;
            uVar29 = *(ulong *)(local_6738 + lVar28 * 8 + 0x18);
          }
          FUN_009d7910(local_6d8);
          FUN_009d7910(local_6e0);
          FUN_009d7910(local_6e8);
          FUN_009d7910(uVar15);
          FUN_009d7910(uVar16);
          FUN_009d7910(uVar17);
          FUN_009d7910(uVar14);
          if (bVar10) {
            lVar25 = *param_6;
            if (0x200 < lVar25) {
              lVar8 = lVar25 + -0x1ff;
              if (-1 < lVar25 + -0x200) {
                lVar8 = lVar25 + -0x200;
              }
              *param_4 = *param_4 + (lVar8 >> 1);
              *param_6 = 0x200;
            }
            lVar25 = *param_7;
            if (0x200 < lVar25) {
              lVar8 = lVar25 + -0x1ff;
              if (-1 < lVar25 + -0x200) {
                lVar8 = lVar25 + -0x200;
              }
              *param_5 = *param_5 + (lVar8 >> 1);
              *param_7 = 0x200;
            }
          }
          FUN_00a22868(*puVar23,local_6730);
          uVar26 = 3;
          if (!bVar10) {
            uVar26 = 1;
          }
          uVar22 = 4;
          if (!(bool)(bVar9 & bVar10)) {
            uVar22 = uVar26;
          }
          if ((!bVar12 && (bool)(bVar9 & bVar10)) &&
             (uVar22 = 2, 0x7f < *param_7 && 0x7f < *param_6)) {
            uVar22 = 0;
          }
        }
        local_728[0] = &DAT_013fd360;
        FUN_00aed9b4(local_728);
        goto LAB_009b2750;
      }
    }
  }
  FUN_00a22868(0,100);
  uVar22 = 1;
LAB_009b2750:
  if (*(long *)PTR____stack_chk_guard_013b4320 != local_b0) {
                    /* WARNING: Subroutine does not return */
    ___stack_chk_fail();
  }
  return uVar22;
}


