// FUN_00178198 size=5204

/* WARNING: Possible PIC construction at 0x001786f8: Changing call to branch */
/* WARNING: Possible PIC construction at 0x00178840: Changing call to branch */
/* WARNING: Possible PIC construction at 0x00179394: Changing call to branch */
/* WARNING: Possible PIC construction at 0x001789b4: Changing call to branch */
/* WARNING: Possible PIC construction at 0x001dd7fc: Changing call to branch */
/* WARNING: Removing unreachable block (ram,0x001789b8) */
/* WARNING: Removing unreachable block (ram,0x001786fc) */
/* WARNING: Removing unreachable block (ram,0x001dd800) */
/* WARNING: Removing unreachable block (ram,0x001dd824) */
/* WARNING: Removing unreachable block (ram,0x001dd838) */
/* WARNING: Removing unreachable block (ram,0x001dd834) */
/* WARNING: Removing unreachable block (ram,0x003ccc54) */
/* WARNING: Removing unreachable block (ram,0x003ccc7c) */
/* WARNING: Removing unreachable block (ram,0x001dd818) */
/* WARNING: Removing unreachable block (ram,0x00152b2c) */
/* WARNING: Type propagation algorithm not settling */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00178198(long param_1)

{
  undefined1 (*pauVar1) [16];
  undefined8 *******pppppppuVar2;
  uint *puVar3;
  undefined1 *puVar4;
  uint uVar5;
  undefined4 uVar6;
  undefined2 uVar7;
  undefined2 uVar8;
  ushort uVar9;
  uint uVar10;
  undefined1 auVar11 [16];
  double dVar12;
  code *pcVar13;
  uint uVar14;
  int iVar15;
  int iVar16;
  undefined8 uVar17;
  undefined8 uVar18;
  undefined8 uVar19;
  undefined8 *puVar20;
  undefined *puVar21;
  long lVar22;
  undefined1 *puVar23;
  long lVar24;
  long lVar25;
  ulong uVar26;
  long *plVar27;
  undefined8 *puVar28;
  undefined8 *puVar29;
  uint uVar30;
  ulong uVar31;
  long lVar32;
  long lVar33;
  long lVar34;
  long unaff_x19;
  undefined *unaff_x20;
  undefined *puVar35;
  uint *puVar36;
  int *piVar37;
  undefined *unaff_x21;
  undefined *puVar38;
  long *plVar39;
  uint *unaff_x22;
  long lVar40;
  undefined8 *unaff_x23;
  undefined8 uVar41;
  undefined8 *unaff_x24;
  long *plVar42;
  ulong unaff_x25;
  undefined8 *unaff_x26;
  undefined *unaff_x27;
  ulong unaff_x28;
  undefined8 *puVar43;
  undefined8 *******unaff_x29;
  undefined8 unaff_x30;
  long in_xzr;
  undefined4 uVar44;
  undefined4 extraout_s0;
  long lVar45;
  undefined1 auVar46 [16];
  undefined1 auVar47 [12];
  undefined1 auVar48 [16];
  undefined1 auStack_110 [8];
  undefined8 uStack_108;
  undefined8 uStack_100;
  uint *puStack_f8;
  ulong uStack_f0;
  long lStack_e8;
  undefined8 *puStack_e0;
  undefined8 *puStack_d8;
  uint *puStack_d0;
  undefined *puStack_c8;
  undefined *puStack_c0;
  long lStack_b8;
  undefined8 *******pppppppuStack_b0;
  undefined8 uStack_a8;
  undefined8 uStack_a0;
  undefined8 *puStack_98;
  undefined8 uStack_88;
  undefined8 uStack_80;
  undefined8 *puStack_78;
  undefined8 uStack_70;
  undefined8 *puStack_68;
  ulong uStack_60;
  long lStack_58;
  
  auVar48._8_8_ = unaff_x21;
  auVar48._0_8_ = param_1;
  pppppppuVar2 = (undefined8 *******)&stack0xfffffffffffffff0;
  lStack_58 = *(long *)PTR____stack_chk_guard_013b4320;
  iVar16 = *(int *)(param_1 + 0x10);
  puVar35 = unaff_x20;
  if (iVar16 == 6) {
    *(undefined4 *)(param_1 + 0x10) = 5;
    func_0x00152fe8(&UNK_0000d9b8 + param_1);
    func_0x00168304(param_1 + 0x38);
    puVar20 = unaff_x23;
    if (*(long *)PTR____stack_chk_guard_013b4320 != lStack_58) goto LAB_00178700;
SUB_001e0e2c:
    plVar27 = (long *)&UNK_013c9140;
    uVar30 = 0xd;
    puVar23 = (undefined1 *)0x0;
  }
  else {
    puVar20 = (undefined8 *)(&UNK_000424d8 + param_1);
    if (iVar16 == 4) {
      func_0x00177038(param_1,1);
      func_0x00177220(param_1,*(undefined8 *)(&UNK_00042608 + param_1));
      (**(code **)(**(long **)(&UNK_00042608 + param_1) + 0xb8))();
      *(undefined8 *)(&UNK_00042608 + param_1) = 0;
LAB_001783a8:
      puVar35 = unaff_x20;
      if (*(long *)PTR____stack_chk_guard_013b4320 == lStack_58) {
        return;
      }
LAB_00178700:
      unaff_x23 = puVar20;
      auVar47 = ___stack_chk_fail();
      unaff_x19 = auVar47._0_8_;
      uStack_a8 = 0x178704;
      unaff_x29 = &pppppppuStack_b0;
      lStack_e8 = *(long *)PTR____stack_chk_guard_013b4320;
      puStack_e0 = unaff_x24;
      puStack_d8 = unaff_x23;
      puStack_d0 = unaff_x22;
      puStack_c8 = unaff_x21;
      puStack_c0 = puVar35;
      lStack_b8 = param_1;
      pppppppuStack_b0 = pppppppuVar2;
      if (*(int *)(unaff_x19 + 0x10) != 2) {
        if (*(long *)PTR____stack_chk_guard_013b4320 == lStack_e8) {
          return;
        }
        uVar17 = 0x178878;
        auVar48 = ___stack_chk_fail();
        puVar23 = auStack_110;
        while( true ) {
          puVar38 = auVar48._8_8_;
          unaff_x19 = auVar48._0_8_;
          *(ulong *)(puVar23 + -0x60) = unaff_x28;
          *(undefined **)(puVar23 + -0x58) = unaff_x27;
          *(undefined8 **)(puVar23 + -0x50) = unaff_x26;
          *(ulong *)(puVar23 + -0x48) = unaff_x25;
          *(undefined8 **)(puVar23 + -0x40) = unaff_x24;
          *(undefined8 **)(puVar23 + -0x38) = unaff_x23;
          *(uint **)(puVar23 + -0x30) = unaff_x22;
          *(undefined **)(puVar23 + -0x28) = unaff_x21;
          *(undefined **)(puVar23 + -0x20) = puVar35;
          *(long *)(puVar23 + -0x18) = param_1;
          *(undefined8 ********)(puVar23 + -0x10) = unaff_x29;
          *(undefined8 *)(puVar23 + -8) = uVar17;
          unaff_x29 = (undefined8 *******)(puVar23 + -0x10);
          register0x00000008 = (BADSPACEBASE *)(puVar23 + -0xbc0);
          *(undefined8 *)(puVar23 + -0x70) = *(undefined8 *)PTR____stack_chk_guard_013b4320;
          uVar30 = *(uint *)(lRam000000000141c668 + 0x1951680);
          uVar5 = *(uint *)(&UNK_01259208 + lRam000000000141c678);
          if ((int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10) !=
              0) {
            uVar5 = (uVar30 ^ uVar30 >> 1) & 0x44554154;
            *(uint *)(lRam000000000141c670 + 0x163fcb0) = (uVar5 | uVar5 << 1) ^ uVar30;
          }
          if (puVar38 == (undefined *)0x0) {
            uVar17 = *(undefined8 *)(unaff_x19 + 0x18);
            *(undefined **)(puVar23 + -0xb80) = &UNK_0000d9b8 + unaff_x19;
            *(undefined8 *)(puVar23 + -0xb78) = _UNK_00c93b20;
            *(undefined4 *)(puVar23 + -0xb70) = 0;
            func_0x001e0d14(unaff_x19,uVar17,puVar23 + -0xb80);
          }
          iVar16 = *(int *)(unaff_x19 + 0x10);
          if (iVar16 == 6) break;
          unaff_x26 = (undefined8 *)(&UNK_000424b0 + unaff_x19);
          if (iVar16 == 4) {
            func_0x00177038(unaff_x19,1);
            func_0x00177220(unaff_x19,*(undefined8 *)(&UNK_00042608 + unaff_x19));
            (**(code **)(**(long **)(&UNK_00042608 + unaff_x19) + 0xb8))();
            *(undefined8 *)(&UNK_00042608 + unaff_x19) = 0;
          }
          else if (iVar16 == 3) {
            if ((&UNK_00042610)[unaff_x19] == '\0') {
              iVar16 = func_0x00177a1c(unaff_x19);
              if (iVar16 == 0) {
                func_0x00177038(unaff_x19,1);
                *(undefined1 *)(unaff_x19 + 0x42613) = 0;
              }
              else {
                lVar40 = *(long *)(unaff_x19 + 0x28);
                uVar26 = *(ulong *)(unaff_x19 + (long)(&UNK_000084a4 + lVar40 * 4) * 8);
                lVar24 = *(long *)(unaff_x19 + (long)(&UNK_000084a5 + lVar40 * 4) * 8);
                pauVar1 = (undefined1 (*) [16])(unaff_x19 + (long)(&UNK_000084a6 + lVar40 * 4) * 8);
                lVar22 = *(long *)(unaff_x19 + (long)(&UNK_000084a7 + lVar40 * 4) * 8);
                lVar40 = *(long *)*pauVar1;
                lVar25 = *(long *)(unaff_x19 + 0x18);
                if ((*(long *)(lVar25 + 0x6d8) == 0) ||
                   (*(char *)(*(long *)(lVar25 + 0x6d8) + 0xe0) == '\0')) {
                  *(long *)(puVar23 + -0xba8) = lVar22;
                  *(long *)(puVar23 + -0xbb0) = lVar40;
                }
                else {
                  iVar16 = (int)*(undefined8 *)(lVar25 + 0x6e0);
                  iVar15 = (int)((ulong)*(undefined8 *)(lVar25 + 0x6e0) >> 0x20);
                  if ((long)uVar26 <= (long)iVar16) {
                    uVar26 = (long)iVar16;
                  }
                  lVar33 = (long)iVar16 +
                           (long)(((int)*(undefined8 *)(lVar25 + 0x6e8) - iVar16) + 1) + -1;
                  lVar45 = (long)iVar15 +
                           (long)(((int)((ulong)*(undefined8 *)(lVar25 + 0x6e8) >> 0x20) - iVar15) +
                                 1) + -1;
                  if (lVar24 <= iVar15) {
                    lVar24 = (long)iVar15;
                  }
                  auVar46._0_8_ = -(ulong)(lVar33 < lVar40);
                  auVar46._8_8_ = -(ulong)(lVar45 < lVar22);
                  auVar11._8_8_ = lVar45;
                  auVar11._0_8_ = lVar33;
                  auVar48 = *pauVar1 ^ (*pauVar1 ^ auVar11) & auVar46;
                  *(long *)(puVar23 + -0xba8) = auVar48._8_8_;
                  *(long *)(puVar23 + -0xbb0) = auVar48._0_8_;
                }
                puVar36 = (uint *)(&UNK_000425b0 + unaff_x19);
                uVar17 = func_0x009c610c(*(undefined8 *)
                                          (lVar25 + *(long *)(lVar25 + 0xd0) * 8 + 0x58));
                unaff_x23 = *(undefined8 **)(*(long *)(unaff_x19 + 0x18) + 0xd0);
                unaff_x24 = (undefined8 *)
                            func_0x009b9a98(*(undefined8 *)
                                             (*(long *)(*(long *)(unaff_x19 + 0x18) + 0x350) + 0x28)
                                           );
                func_0x009bf950(puVar23 + -0xb80,
                                *(undefined8 *)
                                 (*(long *)(*(long *)(unaff_x19 + 0x18) + 0x510) + 0x28),
                                *(undefined4 *)
                                 (&UNK_0000d9a8 + unaff_x19 + *(long *)(unaff_x19 + 0x28) * 4));
                uVar18 = func_0x009bf4d4(puVar23 + -0xb80);
                lVar40 = unaff_x19 + *(long *)(unaff_x19 + 0x28) * 4;
                func_0x001514d0(puVar23 + -0xb98,uVar17,unaff_x23,unaff_x24,uVar18,
                                *(undefined4 *)(&UNK_0000d930 + lVar40),
                                *(undefined4 *)(&UNK_0000d940 + lVar40));
                unaff_x27 = (undefined *)(uVar26 & 0xffffffff | lVar24 << 0x20);
                unaff_x28 = *(ulong *)(puVar23 + -0xbb0) & 0xffffffff |
                            *(long *)(puVar23 + -0xba8) << 0x20;
                *(undefined **)(puVar23 + -0xb90) = unaff_x27;
                *(ulong *)(puVar23 + -0xb88) = unaff_x28;
                *(undefined8 *)(puVar23 + -0xb80) = 0;
                *(undefined8 *)(puVar23 + -0xb78) = 0;
                *(undefined8 *)(puVar23 + -0xb70) = 0xffffffffffffffff;
                iVar16 = func_0x0016b8ac(puVar36,puVar23 + -0xb98,puVar23 + -0xb90,puVar23 + -0xb80)
                ;
                if (iVar16 == 0) {
                  uVar5 = *(uint *)(&UNK_0037a4a0 + lRam000000000141c680);
                  uVar30 = (uVar5 ^ uVar5 >> 1) & 0x44144140;
                  uVar5 = (uVar30 | uVar30 << 1) ^ uVar5;
                  uVar26 = CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5);
                  puVar35 = (undefined *)(uVar26 >> 0x10 & 0xffffffff);
                  puVar23[-0xb98] = 0;
                  func_0x001e0cb0(unaff_x19,puVar23 + -0xb98);
                  if ((int)(uVar26 >> 0x10) == 0) {
                    unaff_x25 = 0;
                  }
                  else {
                    uVar30 = *(uint *)(&UNK_01024218 + lRam000000000141c690);
                    uVar5 = *(uint *)(&UNK_00113718 + lRam000000000141c6a0);
                    uVar26 = CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5);
                    unaff_x25 = uVar26 >> 0x10 & 0xffffffff;
                    if ((int)(uVar26 >> 0x10) != 1) {
                      if (iRam000000000141c688 != 0) {
                        uVar30 = uVar30 >> 6 | uVar30 << 0x1a;
                      }
                      *(uint *)(lRam000000000141c698 + 0x19fd5a8) = uVar30;
                    }
                  }
                  if (puVar23[-0xb98] == '\0') {
                    *(undefined8 *)(&UNK_00042618 + unaff_x19) = 0;
                    iVar16 = func_0x009ec270(&UNK_000425a8 + unaff_x19);
                    uRam000000000141c6a8 = uRam000000000141c6a8 + iVar16;
                    uRam000000000141c6a8 =
                         (uint)(CONCAT44((uRam000000000141c6a8 + (uRam000000000141c6a8 >> 0x10)) *
                                         0x101 + (uRam000000000141c6a8 >> 0x18),uRam000000000141c6a8
                                        ) >> 0x10);
                    uVar17 = func_0x009c610c(*(undefined8 *)
                                              (*(long *)(unaff_x19 + 0x18) +
                                               *(long *)(*(long *)(unaff_x19 + 0x18) + 0xd0) * 8 +
                                              0x58));
                    uVar41 = *(undefined8 *)(*(long *)(unaff_x19 + 0x18) + 0xd0);
                    uVar18 = func_0x009b9a98(*(undefined8 *)
                                              (*(long *)(*(long *)(unaff_x19 + 0x18) + 0x350) + 0x28
                                              ));
                    func_0x009bf950(puVar23 + -0xb80,
                                    *(undefined8 *)
                                     (*(long *)(*(long *)(unaff_x19 + 0x18) + 0x510) + 0x28),
                                    *(undefined4 *)
                                     (&UNK_0000d9a8 + unaff_x19 + *(long *)(unaff_x19 + 0x28) * 4));
                    uVar19 = func_0x009bf4d4(puVar23 + -0xb80);
                    lVar24 = unaff_x19 + *(long *)(unaff_x19 + 0x28) * 4;
                    func_0x001514d0(puVar23 + -0xb90,uVar17,uVar41,uVar18,uVar19,
                                    *(undefined4 *)(&UNK_0000d930 + lVar24),
                                    *(undefined4 *)(&UNK_0000d940 + lVar24));
                    *unaff_x26 = *(undefined8 *)(puVar23 + -0xb90);
                    lVar40 = *(long *)(unaff_x19 + 0x28);
                    uVar17 = *(undefined8 *)(unaff_x19 + (long)(&UNK_000084a4 + lVar40 * 4) * 8);
                    auVar48 = *(undefined1 (*) [16])
                               (unaff_x19 + (long)(&UNK_000084a6 + lVar40 * 4) * 8);
                    *(undefined8 *)(&UNK_000424c0 + unaff_x19) =
                         *(undefined8 *)(unaff_x19 + (long)(&UNK_000084a5 + lVar40 * 4) * 8);
                    *(undefined8 *)(&UNK_000424b8 + unaff_x19) = uVar17;
                    *(long *)(&UNK_000424d0 + unaff_x19) = auVar48._8_8_;
                    *(long *)(&UNK_000424c8 + unaff_x19) = auVar48._0_8_;
                    uVar26 = *(ulong *)(&UNK_000424b8 + unaff_x19);
                    lVar24 = *(long *)(&UNK_000424c8 + unaff_x19);
                    if ((long)uVar26 <= lVar24) {
                      uVar31 = *(ulong *)(&UNK_000424c0 + unaff_x19);
                      lVar25 = *(long *)(&UNK_000424d0 + unaff_x19);
                      if ((long)uVar31 <= lVar25) {
                        lVar22 = (lVar24 - uVar26) + 1;
                        lVar33 = (lVar25 - uVar31) + 1;
                        if ((lVar33 * lVar22 != 0) && ((&UNK_000425a0)[unaff_x19 + lVar40] == '\0'))
                        {
                          lVar45 = **(long **)(*(long *)(unaff_x19 + 0x18) + 0x540);
                          lVar32 = *(long *)(lVar45 + 0xd0) + -1;
                          lVar45 = *(long *)(lVar45 + 0xd8) + -1;
                          puVar35 = (undefined *)
                                    ((long)((float)lVar33 * 1.6) * (long)((float)lVar22 * 1.6));
                          if ((long)puVar35 < 0x7e9001) {
                            puVar35 = &UNK_007e9000;
                          }
                          if (uVar31 != 0 || uVar26 != 0) goto LAB_00178ff0;
                          while ((lVar24 != lVar32 || (lVar25 != lVar45))) {
LAB_00178ff0:
                            do {
                              lVar22 = (lVar24 - uVar26) + 1;
                              lVar33 = (lVar25 - uVar31) + 1;
                              lVar34 = lVar33 * lVar22;
                              if ((long)puVar35 <= lVar34) goto LAB_0017900c;
                              dVar12 = (SQRT((double)(long)puVar35) - SQRT((double)lVar34)) * 0.25;
                              if (dVar12 <= 1.0) {
                                dVar12 = 1.0;
                              }
                              lVar22 = (long)(float)(int)(((float)lVar22 / (float)lVar33) *
                                                         (float)(int)dVar12);
                              lVar33 = (long)(float)(int)(float)(int)dVar12;
                              uVar26 = uVar26 - lVar22;
                              uVar31 = uVar31 - lVar33;
                              lVar22 = lVar24 + lVar22;
                              lVar33 = lVar25 + lVar33;
                              uVar26 = uVar26 & ((long)uVar26 >> 0x3f ^ 0xffffffffffffffffU);
                              uVar31 = uVar31 & ((long)uVar31 >> 0x3f ^ 0xffffffffffffffffU);
                              lVar24 = lVar32;
                              if (lVar22 <= lVar32) {
                                lVar24 = lVar22;
                              }
                              *(ulong *)(&UNK_000424b8 + unaff_x19) = uVar26;
                              *(ulong *)(&UNK_000424c0 + unaff_x19) = uVar31;
                              lVar25 = lVar45;
                              if (lVar33 <= lVar45) {
                                lVar25 = lVar33;
                              }
                              *(long *)(&UNK_000424c8 + unaff_x19) = lVar24;
                              *(long *)(&UNK_000424d0 + unaff_x19) = lVar25;
                            } while (uVar31 != 0 || uVar26 != 0);
                          }
                        }
                      }
                    }
LAB_0017900c:
                    func_0x001e0bd4(unaff_x19,lVar40,1,*(undefined1 *)(unaff_x19 + 0x42613));
                    uVar17 = *(undefined8 *)(unaff_x19 + 0x18);
                    unaff_x22 = (uint *)(&UNK_0000d9b8 + unaff_x19);
                    *(uint **)(puVar23 + -0xb80) = unaff_x22;
                    *(undefined8 *)(puVar23 + -0xb78) = 0;
                    *(undefined4 *)(puVar23 + -0xb70) = 0;
                    func_0x001e0d14(unaff_x19,uVar17,puVar23 + -0xb80);
                    if ((uRam000000000141c6a8 & 0x3f) == 0) {
                      func_0x0015e370(puVar23 + -0xb80);
                      lVar24 = func_0x009d3924(puVar23 + -0xb80);
                      if (lVar24 != lRam0000000001569f68) {
                        uVar5 = *(uint *)(&UNK_00e17b6c + lRam000000000141c6c0);
                        uVar30 = (uVar5 ^ uVar5 >> 1) & 0x44014410;
                        uRam000000000141c6b8 = *(uint *)(lRam000000000141c6d0 + 0x14db140);
                        uVar30 = (uVar30 | uVar30 << 1) ^ uVar5;
                        if (uRam000000000141c6b8 != 0) {
                          uVar30 = uRam000000000141c6b8;
                        }
                        *(uint *)(&UNK_00d02e5c + lRam000000000141c6c8) = uVar30;
                      }
                    }
                    unaff_x23 = (undefined8 *)(&UNK_000424b8 + unaff_x19);
                    lVar40 = *(long *)(unaff_x19 + 0x28);
                    lVar24 = *(long *)(unaff_x19 + (long)(&UNK_0000849f + lVar40) * 8);
                    if (lVar24 == 0) {
                      lVar24 = func_0x0016d654(puVar36);
                      lVar40 = *(long *)(unaff_x19 + 0x28);
                      *(long *)(unaff_x19 + (long)(&UNK_0000849f + lVar40) * 8) = lVar24;
                    }
                    if (*(int *)(&UNK_0000d930 + unaff_x19 + lVar40 * 4) == 0) {
                      func_0x009c4920(lVar24 + 0x110);
                    }
                    func_0x009ec2d8(&UNK_000425a8 + unaff_x19);
                    puVar35 = *(undefined **)(unaff_x19 + 0x18);
                    lVar24 = *(long *)(unaff_x19 +
                                      (long)(&UNK_0000849f + *(long *)(unaff_x19 + 0x28)) * 8);
                    puVar38 = (undefined *)(lVar24 + 8);
                    uVar30 = 1;
                    if (*(int *)(**(long **)(puVar35 + 0x540) + 0xf0) == 2) {
                      uVar30 = 2;
                    }
                    unaff_x24 = (undefined8 *)(ulong)uVar30;
                    unaff_x25 = lVar24 + 0x110;
                    unaff_x26 = (undefined8 *)(lVar24 + 0x218);
                    unaff_x27 = &UNK_0000d9a8 + unaff_x19;
                    func_0x009bf950(puVar23 + -0xb80,
                                    *(undefined8 *)(*(long *)(puVar35 + 0x510) + 0x28),
                                    *(undefined4 *)(unaff_x27 + *(long *)(unaff_x19 + 0x28) * 4));
                    lVar40 = *(long *)(unaff_x19 + 0x28) * 4;
                    lVar24 = unaff_x19 + lVar40;
                    uVar44 = *(undefined4 *)(&UNK_0000d930 + lVar24);
                    uVar6 = *(undefined4 *)(unaff_x27 + lVar40);
                    *(undefined4 *)(puVar23 + -0xbbc) = *(undefined4 *)(&UNK_0000d940 + lVar24);
                    *(undefined4 *)(puVar23 + -3000) = uVar6;
                    *(undefined4 *)(puVar23 + -0xbc0) = uVar44;
                    func_0x00152bcc(unaff_x22,puVar35,puVar38,unaff_x24,unaff_x25,unaff_x26,
                                    puVar23 + -0xb80,unaff_x23);
                    goto LAB_001789e8;
                  }
                  uVar17 = func_0x00177b0c(unaff_x19);
                  *(undefined8 *)(&UNK_00042618 + unaff_x19) = uVar17;
                  func_0x003dc230(100,unaff_x19,"1onPreviewDelayTimerTimeout()");
                }
                else {
                  puVar35 = &UNK_000424d8 + unaff_x19;
                  *(undefined8 *)(puVar23 + -0xbb0) =
                       *(undefined8 *)(puVar35 + *(long *)(unaff_x19 + 0x28) * 8);
                  unaff_x23 = (undefined8 *)
                              func_0x009c610c(*(undefined8 *)
                                               (*(long *)(unaff_x19 + 0x18) +
                                                *(long *)(*(long *)(unaff_x19 + 0x18) + 0xd0) * 8 +
                                               0x58));
                  unaff_x24 = *(undefined8 **)(*(long *)(unaff_x19 + 0x18) + 0xd0);
                  unaff_x25 = func_0x009b9a98(*(undefined8 *)
                                               (*(long *)(*(long *)(unaff_x19 + 0x18) + 0x350) +
                                               0x28));
                  func_0x009bf950(puVar23 + -0xb80,
                                  *(undefined8 *)
                                   (*(long *)(*(long *)(unaff_x19 + 0x18) + 0x510) + 0x28),
                                  *(undefined4 *)
                                   (&UNK_0000d9a8 + unaff_x19 + *(long *)(unaff_x19 + 0x28) * 4));
                  uVar17 = func_0x009bf4d4(puVar23 + -0xb80);
                  lVar24 = unaff_x19 + *(long *)(unaff_x19 + 0x28) * 4;
                  func_0x001514d0(puVar23 + -0xb90,unaff_x23,unaff_x24,unaff_x25,uVar17,
                                  *(undefined4 *)(&UNK_0000d930 + lVar24),
                                  *(undefined4 *)(&UNK_0000d940 + lVar24));
                  *(undefined **)(puVar23 + -0xb80) = unaff_x27;
                  *(ulong *)(puVar23 + -0xb78) = unaff_x28;
                  lVar40 = func_0x0016cc2c(puVar36,puVar23 + -0xb90,puVar23 + -0xb80);
                  lVar25 = *(long *)(unaff_x19 + 0x28);
                  *(long *)(puVar35 + lVar25 * 8) = lVar40;
                  lVar24 = *(long *)(lVar40 + 0x220);
                  if (lVar24 == 0) {
                    lVar24 = 0;
                    lVar22 = 0;
                  }
                  else {
                    lVar22 = lVar24 + 0x120;
                    if (1 < *(int *)(&UNK_0000d9a8 + unaff_x19 + lVar25 * 4) - 5U) {
                      lVar22 = 0;
                    }
                    lVar24 = lVar24 + 0x18;
                  }
                  *(undefined **)(puVar23 + -0xb80) = unaff_x27;
                  *(ulong *)(puVar23 + -0xb78) = unaff_x28;
                  func_0x001e0b4c(unaff_x19,lVar40 + 8,lVar40 + 0x110,lVar22,lVar24,puVar23 + -0xb80
                                  ,puVar38);
                  func_0x001e0bd4(unaff_x19,*(undefined8 *)(unaff_x19 + 0x28),0,0);
                  *(undefined1 *)(unaff_x19 + 0x42613) = 0;
                  func_0x0016d658(puVar36,*(undefined8 *)(puVar23 + -0xbb0));
                }
                func_0x00177038(unaff_x19,1);
                unaff_x22 = puVar36;
              }
            }
            else {
              *(undefined4 *)(unaff_x19 + 0x10) = 2;
              (&UNK_00042610)[unaff_x19] = 0;
              func_0x00148228(unaff_x19 + 0x90,0);
            }
          }
LAB_001789e8:
          if (*(long *)PTR____stack_chk_guard_013b4320 == *(long *)(puVar23 + -0x70)) {
            return;
          }
          auVar47 = ___stack_chk_fail();
          param_1 = auVar47._0_8_;
          *(uint **)(puVar23 + -0xbf0) = unaff_x22;
          *(undefined **)(puVar23 + -0xbe8) = puVar38;
          *(undefined **)(puVar23 + -0xbe0) = puVar35;
          *(long *)(puVar23 + -0xbd8) = unaff_x19;
          *(undefined8 ********)(puVar23 + -0xbd0) = unaff_x29;
          *(undefined8 *)(puVar23 + -0xbc8) = 0x1791e0;
          unaff_x29 = (undefined8 *******)(puVar23 + -0xbd0);
          *(undefined8 *)(puVar23 + -0xbf8) = *(undefined8 *)PTR____stack_chk_guard_013b4320;
          if (*(int *)(param_1 + 0x10) != 3) {
            if (*(long *)PTR____stack_chk_guard_013b4320 == *(long *)(puVar23 + -0xbf8)) {
              return;
            }
            lVar24 = ___stack_chk_fail();
            *(undefined8 ********)(puVar23 + -0xc20) = unaff_x29;
            *(undefined8 *)(puVar23 + -0xc18) = 0x1793c8;
            uVar17 = *(undefined8 *)(lVar24 + 0x18);
            *(undefined **)(puVar23 + -0xc38) = &UNK_0000d9b8 + lVar24;
            *(undefined8 *)(puVar23 + -0xc30) = _UNK_010b4ab8;
            *(undefined4 *)(puVar23 + -0xc28) = extraout_s0;
            func_0x001e0d14(lVar24,uVar17,puVar23 + -0xc38);
            return;
          }
          if (auVar47._8_4_ == 0) {
            puVar21 = (undefined *)0x0;
          }
          else {
            unaff_x22 = (uint *)(&UNK_000424b8 + param_1);
            puVar35 = *(undefined **)
                       (param_1 + (long)(&UNK_0000849f + *(long *)(param_1 + 0x28)) * 8);
            *(undefined8 *)(param_1 + (long)(&UNK_0000849f + *(long *)(param_1 + 0x28)) * 8) = 0;
            if ((&UNK_000426c0)[param_1] == '\0') {
              puVar38 = &UNK_00042690 + param_1;
              if (puVar35 != (undefined *)0x0) goto LAB_0017927c;
LAB_0017928c:
              *(undefined8 **)(puVar23 + -0xc08) = _UNK_010b4468;
              *(undefined8 *)(puVar23 + -0xc10) = _UNK_010b4460;
              func_0x001e0b4c(param_1,0,0,0,0,puVar23 + -0xc10,0);
              func_0x001e0bd4(param_1,*(undefined8 *)(param_1 + 0x28),0,0);
              uVar17 = *(undefined8 *)(param_1 + 0x18);
              *(undefined **)(puVar23 + -0xc10) = &UNK_0000d9b8 + param_1;
              *(undefined8 *)(puVar23 + -0xc08) = _UNK_00c93b20;
              *(undefined4 *)(puVar23 + -0xc00) = 0;
              func_0x001e0d14(param_1,uVar17,puVar23 + -0xc10);
              puVar38 = (undefined *)0x0;
              puVar21 = (undefined *)0x0;
              if (puVar35 == (undefined *)0x0) goto LAB_00179390;
            }
            else {
              puVar38 = &UNK_00042620 + param_1;
              if ((&UNK_00042650)[param_1] != '\0') {
                puVar38 = (undefined *)0x0;
              }
              if (puVar35 == (undefined *)0x0) goto LAB_0017928c;
LAB_0017927c:
              if ((puVar38 != (undefined *)0x0) && (*(int *)(*(long *)(puVar38 + 0x28) + 0x14) != 0)
                 ) goto LAB_0017928c;
              puVar38 = (undefined *)func_0x009ec270(&UNK_000425a8 + param_1);
              uVar17 = *(undefined8 *)(param_1 + 0x18);
              *(undefined **)(puVar23 + -0xc10) = &UNK_0000d9b8 + param_1;
              *(undefined8 *)(puVar23 + -0xc08) = _UNK_0127a8b8;
              *(undefined4 *)(puVar23 + -0xc00) = 0x3f800000;
              func_0x001e0d14(param_1,uVar17,puVar23 + -0xc10);
            }
            puVar21 = puVar38;
            uVar30 = *(uint *)(&UNK_000424c8 + param_1);
            lVar24 = *(long *)(&UNK_000424d0 + param_1);
            *(ulong *)(puVar23 + -0xc10) =
                 (ulong)*unaff_x22 | *(long *)(&UNK_000424c0 + param_1) << 0x20;
            *(ulong *)(puVar23 + -0xc08) = (ulong)uVar30 | lVar24 << 0x20;
            func_0x0016dec0(&UNK_000425b0 + param_1,&UNK_000424b0 + param_1,puVar23 + -0xc10,puVar35
                           );
            puVar38 = puVar21;
          }
LAB_00179390:
          auVar48._8_8_ = puVar21;
          auVar48._0_8_ = param_1;
          uVar17 = 0x179398;
          puVar23 = puVar23 + -0xc10;
          unaff_x21 = puVar38;
        }
        *(undefined4 *)(unaff_x19 + 0x10) = 5;
        func_0x00152fe8(&UNK_0000d9b8 + unaff_x19);
        func_0x00168304(unaff_x19 + 0x38);
        unaff_x30 = 0x1789b8;
        unaff_x20 = puVar35;
        goto SUB_001e0e2c;
      }
      unaff_x20 = puVar35;
      if (auVar47._8_4_ != 0) {
        lVar24 = *(long *)(unaff_x19 + 0x28);
        unaff_x20 = *(undefined **)(unaff_x19 + (long)(&UNK_00001b22 + lVar24) * 8);
        *(undefined8 *)(unaff_x19 + (long)(&UNK_00001b22 + lVar24) * 8) = 0;
        func_0x001e0a54(unaff_x19,lVar24,unaff_x20);
        func_0x001e0ac0(unaff_x19,*(undefined8 *)(unaff_x19 + 0x28),0,
                        *(undefined1 *)(unaff_x19 + 0x42613));
        lVar24 = *(long *)(*(long *)(unaff_x19 + 0x18) +
                           *(long *)(*(long *)(unaff_x19 + 0x18) + 0xd0) * 8 + 0x58);
        iVar16 = *(int *)(lVar24 + 0xd0);
        lVar40 = *(long *)(lVar24 + 0xd8);
        uStack_100 = func_0x009c610c();
        lVar24 = unaff_x19 + *(long *)(unaff_x19 + 0x28) * 4;
        puStack_f8 = (uint *)(ulong)*(uint *)(&UNK_0000d930 + lVar24);
        uStack_f0 = (ulong)*(uint *)(&UNK_0000d940 + lVar24);
        uStack_108 = func_0x009d326c(&uStack_100,3);
        unaff_x21 = &UNK_0000d950 + unaff_x19;
        unaff_x24 = (undefined8 *)(&UNK_0000d8f0 + unaff_x19);
        func_0x0016ad84(unaff_x21,unaff_x24[*(long *)(unaff_x19 + 0x28)]);
        unaff_x23 = (undefined8 *)((ulong)(iVar16 - 1) | lVar40 << 0x20);
        unaff_x22 = (uint *)(unaff_x23 + -0x20000000);
        uStack_100 = 0;
        puStack_f8 = unaff_x22;
        func_0x0016b5f4(unaff_x21,&uStack_108,&uStack_100,unaff_x20);
        uStack_100 = 0;
        puStack_f8 = unaff_x22;
        uVar17 = func_0x0016a234(unaff_x21,&uStack_108,&uStack_100);
        unaff_x24[*(long *)(unaff_x19 + 0x28)] = uVar17;
      }
      *(undefined4 *)(unaff_x19 + 0x10) = 3;
      puVar35 = &UNK_0000d9b8 + unaff_x19;
      unaff_x30 = 0x178844;
      register0x00000008 = (BADSPACEBASE *)auStack_110;
    }
    else {
      if (iVar16 != 2) goto LAB_001783a8;
      lVar24 = *(long *)(param_1 + 0x18);
      if (((((lVar24 == 0) ||
            (lVar24 = *(long *)(lVar24 + *(long *)(lVar24 + 0xd0) * 8 + 0x58), lVar24 == 0)) ||
           (*(char *)(lVar24 + 0xe0) == '\0')) ||
          ((*(char *)(param_1 + 0x42614) != '\0' || (*(int *)(param_1 + 0x30) != 1)))) ||
         (*(int *)(&UNK_0000d930 + param_1 + *(long *)(param_1 + 0x28) * 4) == 0)) {
        if (*(long *)(param_1 + (long)(&UNK_00001b1e + *(long *)(param_1 + 0x28)) * 8) != 0) {
          func_0x001e0a54(param_1,*(long *)(param_1 + 0x28),0);
          puVar35 = &UNK_0000d8f0 + param_1;
          func_0x0016ad84(&UNK_0000d950 + param_1,
                          *(undefined8 *)(puVar35 + *(long *)(param_1 + 0x28) * 8));
          *(undefined8 *)(puVar35 + *(long *)(param_1 + 0x28) * 8) = 0;
        }
        *(undefined4 *)(param_1 + 0x10) = 3;
        if (*(long *)PTR____stack_chk_guard_013b4320 != lStack_58) goto LAB_00178700;
        puVar35 = &UNK_0000d9b8 + param_1;
      }
      else {
        uStack_70 = func_0x009c610c();
        lVar24 = param_1 + *(long *)(param_1 + 0x28) * 4;
        puStack_68 = (undefined8 *)(ulong)*(uint *)(&UNK_0000d930 + lVar24);
        uStack_60 = (ulong)*(uint *)(&UNK_0000d940 + lVar24);
        unaff_x22 = (uint *)((long)&MACH_HEADER.magic + 3);
        uStack_88 = func_0x009d326c(&uStack_70,3);
        unaff_x21 = *(undefined **)
                     (*(long *)(param_1 + 0x18) + *(long *)(*(long *)(param_1 + 0x18) + 0xd0) * 8 +
                     0x58);
        unaff_x20 = &UNK_0000d950 + param_1;
        unaff_x24 = (undefined8 *)
                    (((ulong)(*(int *)(unaff_x21 + 0xd0) - 1) | *(long *)(unaff_x21 + 0xd8) << 0x20)
                    - 0x100000000);
        uStack_80 = 0;
        uStack_70 = 0;
        puStack_68 = (undefined8 *)0x0;
        uStack_60 = 0xffffffffffffffff;
        puStack_78 = unaff_x24;
        iVar16 = func_0x00168eac(unaff_x20,&uStack_88,&uStack_80,&uStack_70);
        if (iVar16 == 0) {
          func_0x001e0ac0(param_1,*(long *)(param_1 + 0x28),1,*(undefined1 *)(param_1 + 0x42613));
          lVar24 = *(long *)(param_1 + (long)(&UNK_00001b22 + *(long *)(param_1 + 0x28)) * 8);
          if (lVar24 == 0) {
            lVar24 = func_0x0016ac78(unaff_x20);
            *(long *)(param_1 + (long)(&UNK_00001b22 + *(long *)(param_1 + 0x28)) * 8) = lVar24;
          }
          uVar26 = func_0x009c45a0(lVar24,*(undefined8 *)(unaff_x21 + 0xd0),
                                   *(undefined8 *)(unaff_x21 + 0xd8),1,0,0,1);
          if ((uVar26 & 1) == 0) {
            unaff_x24 = (undefined8 *)(&UNK_0000d8f0 + param_1);
            unaff_x22 = (uint *)(&UNK_000425b0 + param_1);
            func_0x001e0a54(param_1,0,0);
            func_0x0016ad84(unaff_x20,*unaff_x24);
            *unaff_x24 = 0;
            puStack_98 = _UNK_010b4468;
            uStack_a0 = _UNK_010b4460;
            puStack_68 = _UNK_010b4468;
            uStack_70 = _UNK_010b4460;
            func_0x001e0b4c(param_1,0,0,0,0,&uStack_70,0);
            func_0x0016d658(unaff_x22,*puVar20);
            *puVar20 = 0;
            func_0x001e0a54(param_1,1,0);
            func_0x0016ad84(unaff_x20,*(undefined8 *)(&UNK_0000d8f8 + param_1));
            *(undefined8 *)(&UNK_0000d8f8 + param_1) = 0;
            puStack_68 = puStack_98;
            uStack_70 = uStack_a0;
            func_0x001e0b4c(param_1,0,0,0,0,&uStack_70,0);
            func_0x0016d658(unaff_x22,*(undefined8 *)(&UNK_000424e0 + param_1));
            *(undefined8 *)(&UNK_000424e0 + param_1) = 0;
            func_0x001e0a54(param_1,2,0);
            func_0x0016ad84(unaff_x20,*(undefined8 *)(&UNK_0000d900 + param_1));
            *(undefined8 *)(&UNK_0000d900 + param_1) = 0;
            puStack_68 = puStack_98;
            uStack_70 = uStack_a0;
            func_0x001e0b4c(param_1,0,0,0,0,&uStack_70,0);
            func_0x0016d658(unaff_x22,*(undefined8 *)(&UNK_000424e8 + param_1));
            *(undefined8 *)(&UNK_000424e8 + param_1) = 0;
            unaff_x25 = 3;
            func_0x001e0a54(param_1,3,0);
            func_0x0016ad84(unaff_x20,*(undefined8 *)(&UNK_0000d908 + param_1));
            *(undefined8 *)(&UNK_0000d908 + param_1) = 0;
            puStack_68 = puStack_98;
            uStack_70 = uStack_a0;
            func_0x001e0b4c(param_1,0,0,0,0,&uStack_70,0);
            func_0x0016d658(unaff_x22,*(undefined8 *)(&UNK_000424f0 + param_1));
            *(undefined8 *)(&UNK_000424f0 + param_1) = 0;
            func_0x00168304(param_1 + 0x38);
            uVar26 = func_0x009c45a0(*(undefined8 *)
                                      (param_1 +
                                      (long)(&UNK_00001b22 + *(long *)(param_1 + 0x28)) * 8),
                                     *(undefined8 *)(unaff_x21 + 0xd0),
                                     *(undefined8 *)(unaff_x21 + 0xd8),1,0,0,1);
            if ((uVar26 & 1) == 0) {
              FUN_0016b55c(unaff_x20,param_1 + (long)(&UNK_00001b22 + *(long *)(param_1 + 0x28)) * 8
                          );
              func_0x001e0ac0(param_1,*(undefined8 *)(param_1 + 0x28),0,0);
              func_0x00177038(param_1,1);
              func_0x001e0b38(param_1);
              *(undefined4 *)(param_1 + 0x10) = 3;
              goto LAB_001786ec;
            }
          }
          lVar24 = param_1 + *(long *)(param_1 + 0x28) * 4;
          FUN_00147f2c(param_1 + 0x90,unaff_x21,
                       *(undefined8 *)
                        (param_1 + (long)(&UNK_00001b22 + *(long *)(param_1 + 0x28)) * 8),
                       *(undefined4 *)(&UNK_0000d930 + lVar24),
                       *(undefined4 *)(&UNK_0000d940 + lVar24));
          goto LAB_001783a8;
        }
        puVar20 = (undefined8 *)(&UNK_0000d8f0 + param_1);
        unaff_x21 = (undefined *)puVar20[*(long *)(param_1 + 0x28)];
        uStack_70 = 0;
        puStack_68 = unaff_x24;
        uVar17 = func_0x0016a234(unaff_x20,&uStack_88,&uStack_70);
        lVar24 = *(long *)(param_1 + 0x28);
        puVar20[lVar24] = uVar17;
        func_0x001e0a54(param_1,lVar24,uVar17);
        func_0x001e0ac0(param_1,*(undefined8 *)(param_1 + 0x28),0,0);
        func_0x0016ad84(unaff_x20,unaff_x21);
        *(undefined4 *)(param_1 + 0x10) = 3;
LAB_001786ec:
        puVar35 = &UNK_0000d9b8 + param_1;
        unaff_x30 = 0x1786fc;
        register0x00000008 = (BADSPACEBASE *)&uStack_a0;
        unaff_x19 = param_1;
        unaff_x23 = puVar20;
        unaff_x29 = pppppppuVar2;
      }
    }
    *(uint **)((long)register0x00000008 + -0x30) = unaff_x22;
    *(undefined **)((long)register0x00000008 + -0x28) = unaff_x21;
    *(undefined **)((long)register0x00000008 + -0x20) = unaff_x20;
    *(long *)((long)register0x00000008 + -0x18) = unaff_x19;
    *(undefined8 ********)((long)register0x00000008 + -0x10) = unaff_x29;
    *(undefined8 *)((long)register0x00000008 + -8) = unaff_x30;
    func_0x00a9e5ac(*(undefined8 *)(&UNK_008717a0 + lRam000000000141c3a0),0x1539700);
    func_0x003db9bc(&UNK_00034ad0 + (long)puVar35);
    if (*(long *)(&UNK_00025d60 + (long)puVar35) == 0) {
      unaff_x29 = *(undefined8 ********)((long)register0x00000008 + -0x10);
      unaff_x30 = *(undefined8 *)((long)register0x00000008 + -8);
      unaff_x20 = *(undefined **)((long)register0x00000008 + -0x20);
      unaff_x19 = *(long *)((long)register0x00000008 + -0x18);
      unaff_x22 = *(uint **)((long)register0x00000008 + -0x30);
      auVar48._8_8_ = *(undefined8 *)((long)register0x00000008 + -0x28);
      auVar48._0_8_ = puVar35;
      plVar27 = (long *)&UNK_013c8c68;
      uVar30 = 0;
      puVar23 = (undefined1 *)0x0;
    }
    else {
      puVar35[0x51] = 1;
      func_0x00150200();
      lVar24 = *(long *)(&UNK_00025d60 + (long)puVar35);
      uVar44 = 0;
      if (((lVar24 != 0) && (*(char *)(lVar24 + 0x6d) == '\0')) && (*(int *)(lVar24 + 0x34) == 2)) {
        if ((*(uint *)(lVar24 + 0x68) & 0xfffffffe) != 2) {
          do {
            func_0x009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0x184de3ae,0);
          } while( true );
        }
        uVar44 = func_0x00150324();
      }
      unaff_x20 = *(undefined **)((long)register0x00000008 + -0x20);
      unaff_x19 = *(long *)((long)register0x00000008 + -0x18);
      unaff_x22 = *(uint **)((long)register0x00000008 + -0x30);
      auVar48._8_8_ = *(undefined8 *)((long)register0x00000008 + -0x28);
      auVar48._0_8_ = puVar35;
      *(undefined8 *)((long)register0x00000008 + -0x10) =
           *(undefined8 *)((long)register0x00000008 + -0x10);
      *(undefined8 *)((long)register0x00000008 + -8) =
           *(undefined8 *)((long)register0x00000008 + -8);
      unaff_x29 = (undefined8 *******)((long)register0x00000008 + -0x10);
      *(undefined8 *)((long)register0x00000008 + -0x18) =
           *(undefined8 *)PTR____stack_chk_guard_013b4320;
      *(undefined4 *)((long)register0x00000008 + -0x2c) = uVar44;
      *(undefined8 *)((long)register0x00000008 + -0x28) = 0;
      *(undefined1 **)((long)register0x00000008 + -0x20) =
           (undefined1 *)((long)register0x00000008 + -0x2c);
      plVar27 = (long *)&UNK_013c8c68;
      puVar23 = (undefined1 *)((long)register0x00000008 + -0x28);
      uVar30 = 2;
      unaff_x30 = 0x1dd800;
      register0x00000008 = (BADSPACEBASE *)((long)register0x00000008 + -0x30);
    }
  }
  puVar20 = auVar48._0_8_;
  plVar27 = (long *)*plVar27;
  if (plVar27 != (long *)0x0) {
    iVar16 = 0;
    do {
      iVar16 = *(int *)(plVar27[2] + 0x34) + iVar16;
      plVar27 = (long *)*plVar27;
    } while (plVar27 != (long *)0x0);
    uVar30 = iVar16 + uVar30;
  }
  if (puRam000000000155c540 == (undefined8 *)0x0) {
    *(ulong *)((long)register0x00000008 + -0x60) = unaff_x28;
    *(undefined **)((long)register0x00000008 + -0x58) = unaff_x27;
    *(undefined8 **)((long)register0x00000008 + -0x50) = unaff_x26;
    *(ulong *)((long)register0x00000008 + -0x48) = unaff_x25;
    *(undefined8 **)((long)register0x00000008 + -0x40) = unaff_x24;
    *(undefined8 **)((long)register0x00000008 + -0x38) = unaff_x23;
    *(uint **)((long)register0x00000008 + -0x30) = unaff_x22;
    *(long *)((long)register0x00000008 + -0x28) = auVar48._8_8_;
    *(undefined **)((long)register0x00000008 + -0x20) = unaff_x20;
    *(long *)((long)register0x00000008 + -0x18) = unaff_x19;
    *(undefined8 ********)((long)register0x00000008 + -0x10) = unaff_x29;
    *(undefined8 *)((long)register0x00000008 + -8) = unaff_x30;
    *(undefined8 *)((long)register0x00000008 + -0x70) =
         *(undefined8 *)PTR____stack_chk_guard_013b4320;
    lVar24 = puVar20[1];
    if ((*(byte *)(lVar24 + 0x20) >> 1 & 1) == 0) {
      if ((*(long *)(lVar24 + 0x48) != 0 && pcRam000000000155c570 != (code *)0x0) &&
         (iVar16 = (*pcRam000000000155c570)
                             (*(long *)(lVar24 + 0x48),*(undefined8 *)(lVar24 + 8),uVar30),
         iVar16 != 0 && pcRam000000000155c560 != (code *)0x0)) {
        (*pcRam000000000155c560)(*(undefined8 *)(lVar24 + 0x48),puVar20,uVar30,puVar23);
      }
      *(undefined8 *)((long)register0x00000008 + -0x78) = 0;
      puVar4 = (undefined1 *)((long)register0x00000008 + -0x78);
      if (puVar23 != (undefined1 *)0x0) {
        puVar4 = puVar23;
      }
      lVar40 = *(long *)(lVar24 + 0x40);
      if ((lVar40 != 0) && (*(long *)(lVar40 + 8) != 0)) {
        plVar27 = (long *)(*(long *)(lVar40 + 8) + 0x10);
        if (*plVar27 == 0) {
          if (*(long *)(lVar40 + 8) == 0) {
            if (0xfffffffe < uVar30) goto LAB_003d5964;
          }
          else if (*(uint *)(*(long *)(lVar40 + 8) + 8) <= uVar30) goto LAB_003d5964;
          if (plVar27[(long)(int)uVar30 * 2 + 2] == 0) goto LAB_003d5964;
        }
        *(long *)((long)register0x00000008 + -0xd8) = lVar24;
        puVar36 = *(uint **)(lVar24 + 0x40);
        if (puVar36 != (uint *)0x0) {
          LOAcquire();
          puVar36[1] = puVar36[1] + 1;
          LORelease();
        }
        *(uint **)((long)register0x00000008 + -0xe0) = puVar36 + 2;
        plVar27 = (long *)(*(long *)(puVar36 + 2) + 0x10);
        plVar39 = plVar27 + (long)(int)uVar30 * 2 + 2;
        if (*(int *)(*(long *)(puVar36 + 2) + 8) <= (int)uVar30) {
          plVar39 = plVar27;
        }
        uVar17 = func_0x002813dc();
        *(undefined8 *)((long)register0x00000008 + -0xa8) =
             *(undefined8 *)(*(long *)(puVar20[1] + 0x38) + 0x40);
        *(undefined8 *)((long)register0x00000008 + -0xa0) = uVar17;
        *(uint **)((long)register0x00000008 + -0xd0) = puVar36;
        *(long **)((long)register0x00000008 + -200) = plVar27;
        uVar5 = *puVar36;
        *(ulong *)((long)register0x00000008 + -0xb8) = (ulong)((long)register0x00000008 + -0x98) | 4
        ;
        plVar42 = (long *)((ulong)(auVar48._0_4_ +
                                   (int)(((ulong)puVar20 & 0xffffffff) / 0x418) * -0x418 & 0x7f8) +
                          0x1540a30);
        *(long **)((long)register0x00000008 + -0xc0) = plVar42;
        do {
          *(long **)((long)register0x00000008 + -0xb0) = plVar39;
          lVar24 = *plVar39;
          plVar39 = plVar27;
          if (lVar24 != 0) {
            do {
              puVar43 = *(undefined8 **)(lVar24 + 0x28);
              if ((puVar43 != (undefined8 *)0x0) && (lVar40 = *(long *)(lVar24 + 0x30), lVar40 != 0)
                 ) {
                if (*(long *)((long)register0x00000008 + -0xa0) ==
                    *(long *)((long)register0x00000008 + -0xa8)) {
                  lVar40 = *(long *)(lVar40 + 0x40);
                }
                else {
                  iVar16 = ((int)puVar43 + (int)(((ulong)puVar43 & 0xffffffff) / 0x418) * -0x418 &
                           0x7f8U) + 0x1540a30;
                  func_0x00281bec(iVar16);
                  lVar40 = *(long *)(lVar40 + 0x40);
                  func_0x00281b90(iVar16);
                }
                uVar14 = *(uint *)(lVar24 + 0x54);
                if (*(long *)((long)register0x00000008 + -0xa0) == lVar40 ||
                    (uVar14 & 0x38000000) != 0) {
                  uVar10 = uVar14 >> 0x1b & 7;
                  if (uVar10 == 3) {
                    if (*(long *)((long)register0x00000008 + -0xa0) == lVar40) {
                      *(undefined4 *)((long)register0x00000008 + -0x98) = 2;
                      puVar29 = *(undefined8 **)((long)register0x00000008 + -0xb8);
                      *puVar29 = 0;
                      puVar29[1] = 0;
                      *(undefined4 *)(puVar29 + 2) = 0;
                      *(char **)((long)register0x00000008 + -0x80) = "default";
                      (**(code **)*puVar20)(puVar20);
                      uVar17 = func_0x003bbb68();
                      (**(code **)*puVar43)(puVar43);
                      uVar18 = func_0x003bbb68();
                      *(undefined8 *)((long)register0x00000008 + -0xf0) = uVar18;
                      *(undefined8 **)((long)register0x00000008 + -0xe8) = puVar43;
                      *(undefined8 *)((long)register0x00000008 + -0x100) = uVar17;
                      *(undefined8 **)((long)register0x00000008 + -0xf8) = puVar20;
                      func_0x00c78434((undefined1 *)((long)register0x00000008 + -0x98),
                                      "Qt: Dead lock detected while activating a BlockingQueuedConnection: Sender is %s(%p), receiver is %s(%p)"
                                     );
                    }
                    func_0x002846b0((undefined1 *)((long)register0x00000008 + -0x98),0);
                    if (*plVar42 == 0) {
                      *plVar42 = 1;
                    }
                    else {
                      func_0x00281cac(plVar42);
                    }
                    if (*(long *)(lVar24 + 0x28) == 0) {
                      if (*plVar42 == 1) {
                        *plVar42 = in_xzr;
                      }
                      else {
                        func_0x0028232c(plVar42);
                      }
                    }
                    else {
                      if ((*(byte *)(lVar24 + 0x57) >> 6 & 1) == 0) {
                        puVar29 = (undefined8 *)__Znwm(0x70);
                        uVar7 = *(undefined2 *)(lVar24 + 0x50);
                        uVar8 = *(undefined2 *)(lVar24 + 0x52);
                        uVar17 = *(undefined8 *)(lVar24 + 0x38);
                        func_0x003bb49c(puVar29,0x2b);
                        *(uint *)((long)puVar29 + 0x14) = uVar30;
                        puVar29[3] = puVar20;
                        puVar29[4] = (undefined1 *)((long)register0x00000008 + -0x98);
                        puVar35 = &UNK_013d2610;
                        *puVar29 = &UNK_013d2610;
                        puVar29[5] = 0;
                        puVar29[6] = puVar4;
                        puVar29[7] = uVar17;
                        *(undefined4 *)(puVar29 + 8) = 0;
                        *(undefined2 *)((long)puVar29 + 0x44) = uVar7;
                        *(undefined2 *)((long)puVar29 + 0x46) = uVar8;
                        puVar29[10] = 0;
                        puVar29[9] = 0;
                        puVar29[0xc] = 0;
                        puVar29[0xb] = 0;
                        *(undefined4 *)(puVar29 + 0xd) = 0;
                        plVar39 = *(long **)((long)register0x00000008 + -200);
                        plVar42 = *(long **)((long)register0x00000008 + -0xc0);
                      }
                      else {
                        puVar29 = (undefined8 *)__Znwm(0x70);
                        puVar36 = *(uint **)(lVar24 + 0x38);
                        func_0x003bb49c(puVar29,0x2b);
                        *(uint *)((long)puVar29 + 0x14) = uVar30;
                        puVar29[3] = puVar20;
                        puVar29[4] = (undefined1 *)((long)register0x00000008 + -0x98);
                        *puVar29 = &UNK_013d2610;
                        puVar29[5] = puVar36;
                        puVar29[6] = puVar4;
                        puVar35 = (undefined *)0xffff000000000000;
                        puVar29[7] = 0;
                        puVar29[8] = 0xffff000000000000;
                        puVar29[10] = 0;
                        puVar29[9] = 0;
                        puVar29[0xc] = 0;
                        puVar29[0xb] = 0;
                        *(undefined4 *)(puVar29 + 0xd) = 0;
                        if (puVar36 != (uint *)0x0) {
                          LOAcquire();
                          uVar14 = *puVar36;
                          *puVar36 = uVar14 + 1;
                          puVar35 = (undefined *)(ulong)uVar14;
                          LORelease();
                        }
                      }
                      func_0x003b73bc(puVar35,puVar43,puVar29,0);
                      if (*plVar42 == 1) {
                        *plVar42 = in_xzr;
                      }
                      else {
                        func_0x0028232c(plVar42);
                      }
                      func_0x00284750((undefined1 *)((long)register0x00000008 + -0x98),1);
                    }
                    func_0x00284714((undefined1 *)((long)register0x00000008 + -0x98));
                  }
                  else {
                    if (uVar10 == 2) goto LAB_003d5550;
                    puVar29 = puVar43;
                    if (*(long *)((long)register0x00000008 + -0xa0) != lVar40) {
                      puVar29 = (undefined8 *)0x0;
                    }
                    *(undefined8 **)((long)register0x00000008 + -0x90) = puVar29;
                    *(undefined8 **)((long)register0x00000008 + -0x88) = puVar20;
                    *(uint *)((long)register0x00000008 + -0x80) = uVar30;
                    if (puVar29 != (undefined8 *)0x0) {
                      lVar40 = *(long *)(puVar29[1] + 0x40);
                      *(undefined8 *)((long)register0x00000008 + -0x98) =
                           *(undefined8 *)(lVar40 + 0x18);
                      *(undefined1 **)(lVar40 + 0x18) =
                           (undefined1 *)((long)register0x00000008 + -0x98);
                      uVar14 = *(uint *)(lVar24 + 0x54);
                    }
                    if ((uVar14 >> 0x1e & 1) == 0) {
                      if (*(long *)(lVar24 + 0x38) != 0) {
                        uVar9 = *(ushort *)(lVar24 + 0x50);
                        (**(code **)*puVar43)(puVar43);
                        iVar16 = func_0x003bc080();
                        if ((int)(uint)uVar9 <= iVar16) {
                          (**(code **)(lVar24 + 0x38))
                                    (puVar43,0,*(undefined2 *)(lVar24 + 0x52),puVar4);
                          goto LAB_003d5840;
                        }
                      }
                      func_0x003bbf14(puVar43,0,
                                      (uint)*(ushort *)(lVar24 + 0x50) +
                                      (uint)*(ushort *)(lVar24 + 0x52),puVar4);
                    }
                    else {
                      LOAcquire();
                      **(int **)(lVar24 + 0x38) = **(int **)(lVar24 + 0x38) + 1;
                      LORelease();
                      piVar37 = *(int **)(lVar24 + 0x38);
                      (**(code **)(piVar37 + 2))(1,piVar37,puVar43,puVar4,0);
                      if (piVar37 != (int *)0x0) {
                        LOAcquire();
                        iVar16 = *piVar37;
                        *piVar37 = iVar16 + -1;
                        LORelease();
                        if (iVar16 == 1) {
                          (**(code **)(piVar37 + 2))(0,piVar37,0,0,0);
                        }
                      }
                    }
LAB_003d5840:
                    if (*(long *)((long)register0x00000008 + -0x90) != 0) {
                      *(undefined8 *)
                       (*(long *)(*(long *)(*(long *)((long)register0x00000008 + -0x90) + 8) + 0x40)
                       + 0x18) = *(undefined8 *)((long)register0x00000008 + -0x98);
                    }
                  }
                }
                else {
LAB_003d5550:
                  func_0x003d8bdc(puVar20,uVar30,lVar24,puVar4);
                }
              }
              lVar24 = *(long *)(lVar24 + 0x10);
            } while ((lVar24 != 0) && (*(uint *)(lVar24 + 0x4c) <= uVar5));
          }
          plVar27 = plVar39;
        } while (*(long **)((long)register0x00000008 + -0xb0) != plVar39);
        piVar37 = *(int **)((long)register0x00000008 + -0xd0);
        iVar16 = *piVar37;
        if (piVar37 != (int *)0x0) {
          LOAcquire();
          iVar15 = piVar37[1];
          piVar37[1] = iVar15 + -1;
          LORelease();
          if (iVar15 == 1) {
            func_0x003cd314(*(undefined8 *)(piVar37 + 8));
            if (*(long *)(piVar37 + 2) != 0) {
              _free();
            }
            __ZdlPv(piVar37);
          }
        }
        if (((iVar16 != 0) &&
            (lVar24 = *(long *)(*(long *)((long)register0x00000008 + -0xd8) + 0x40),
            *(long *)(lVar24 + 0x20) != 0)) && (*(int *)(lVar24 + 4) == 1)) {
          if (*plVar42 == 0) {
            *plVar42 = 1;
          }
          else {
            func_0x00281cac(plVar42);
          }
          iVar16 = *(int *)(lVar24 + 4);
          if (iVar16 < 2) {
            uVar17 = *(undefined8 *)(lVar24 + 0x20);
            *(undefined8 *)(lVar24 + 0x20) = 0;
          }
          else {
            uVar17 = 0;
          }
          if (*plVar42 == 1) {
            *plVar42 = in_xzr;
          }
          else {
            func_0x0028232c(plVar42);
          }
          if (iVar16 < 2) {
            func_0x003cd314(uVar17);
          }
        }
      }
    }
LAB_003d5964:
    if (*(long *)PTR____stack_chk_guard_013b4320 == *(long *)((long)register0x00000008 + -0x70)) {
      return;
    }
    uVar17 = ___stack_chk_fail();
    if (*(long *)((long)register0x00000008 + -0x90) != 0) {
      *(undefined8 *)
       (*(long *)(*(long *)(*(long *)((long)register0x00000008 + -0x90) + 8) + 0x40) + 0x18) =
           *(undefined8 *)((long)register0x00000008 + -0x98);
    }
    if (*(long *)((long)register0x00000008 + -0xd0) != 0) {
      LOAcquire();
      iVar16 = *(int *)(*(long *)((long)register0x00000008 + -0xd0) + 4);
      *(int *)(*(long *)((long)register0x00000008 + -0xd0) + 4) = iVar16 + -1;
      LORelease();
      if (iVar16 == 1) {
        func_0x00c804b8(*(undefined8 *)((long)register0x00000008 + -0xd0),
                        *(undefined8 *)((long)register0x00000008 + -0xe0),
                        *(undefined8 *)((long)register0x00000008 + -0xd0));
      }
    }
    __Unwind_Resume(uVar17);
                    /* WARNING: Does not return */
    pcVar13 = (code *)SoftwareBreakpoint(1,0x3d5aa0);
    (*pcVar13)();
  }
  *(ulong *)((long)register0x00000008 + -0x60) = unaff_x28;
  *(undefined **)((long)register0x00000008 + -0x58) = unaff_x27;
  *(undefined8 **)((long)register0x00000008 + -0x50) = unaff_x26;
  *(ulong *)((long)register0x00000008 + -0x48) = unaff_x25;
  *(undefined8 **)((long)register0x00000008 + -0x40) = unaff_x24;
  *(undefined8 **)((long)register0x00000008 + -0x38) = unaff_x23;
  *(uint **)((long)register0x00000008 + -0x30) = unaff_x22;
  *(long *)((long)register0x00000008 + -0x28) = auVar48._8_8_;
  *(undefined **)((long)register0x00000008 + -0x20) = unaff_x20;
  *(long *)((long)register0x00000008 + -0x18) = unaff_x19;
  *(undefined8 ********)((long)register0x00000008 + -0x10) = unaff_x29;
  *(undefined8 *)((long)register0x00000008 + -8) = unaff_x30;
  *(undefined8 *)((long)register0x00000008 + -0x70) = *(undefined8 *)PTR____stack_chk_guard_013b4320
  ;
  lVar24 = puVar20[1];
  if ((*(byte *)(lVar24 + 0x20) >> 1 & 1) != 0) goto LAB_003d51cc;
  if ((*(long *)(lVar24 + 0x48) != 0 && pcRam000000000155c570 != (code *)0x0) &&
     (iVar16 = (*pcRam000000000155c570)(*(long *)(lVar24 + 0x48),*(undefined8 *)(lVar24 + 8),uVar30)
     , iVar16 != 0 && pcRam000000000155c560 != (code *)0x0)) {
    (*pcRam000000000155c560)(*(undefined8 *)(lVar24 + 0x48),puVar20,uVar30,puVar23);
  }
  puVar43 = puRam000000000155c540;
  *(undefined8 *)((long)register0x00000008 + -0x78) = 0;
  puVar4 = (undefined1 *)((long)register0x00000008 + -0x78);
  if (puVar23 != (undefined1 *)0x0) {
    puVar4 = puVar23;
  }
  *(undefined1 **)((long)register0x00000008 + -0xa0) = puVar4;
  lVar40 = *(long *)(lVar24 + 0x40);
  if ((lVar40 == 0) || (*(long *)(lVar40 + 8) == 0)) goto LAB_003d51a0;
  plVar27 = (long *)(*(long *)(lVar40 + 8) + 0x10);
  if (*plVar27 == 0) {
    if (*(long *)(lVar40 + 8) == 0) {
      if (uVar30 == 0xffffffff) goto LAB_003d51a0;
LAB_003d5178:
      pcVar13 = (code *)*puRam000000000155c540;
      if (plVar27[(long)(int)uVar30 * 2 + 2] != 0) goto joined_r0x003d518c;
    }
    else {
      if (uVar30 < *(uint *)(*(long *)(lVar40 + 8) + 8)) goto LAB_003d5178;
LAB_003d51a0:
      pcVar13 = (code *)*puRam000000000155c540;
    }
    if (pcVar13 != (code *)0x0) {
      (*pcVar13)(puVar20,uVar30,*(undefined8 *)((long)register0x00000008 + -0xa0));
    }
  }
  else {
    pcVar13 = (code *)*puRam000000000155c540;
joined_r0x003d518c:
    if (pcVar13 != (code *)0x0) {
      (*pcVar13)(puVar20,uVar30,*(undefined8 *)((long)register0x00000008 + -0xa0));
    }
    *(long *)((long)register0x00000008 + -0xe8) = lVar24;
    puVar36 = *(uint **)(lVar24 + 0x40);
    if (puVar36 != (uint *)0x0) {
      LOAcquire();
      puVar36[1] = puVar36[1] + 1;
      LORelease();
    }
    *(uint **)((long)register0x00000008 + -0xf0) = puVar36 + 2;
    iVar16 = *(int *)(*(long *)(puVar36 + 2) + 8);
    puVar3 = (uint *)(*(long *)(puVar36 + 2) + 0x10);
    *(uint **)((long)register0x00000008 + -0xb8) = puVar3;
    unaff_x22 = puVar3 + (long)(int)uVar30 * 4 + 4;
    if (iVar16 <= (int)uVar30) {
      unaff_x22 = puVar3;
    }
    lVar24 = func_0x002813dc();
    *(undefined8 *)((long)register0x00000008 + -0xa8) =
         *(undefined8 *)(*(long *)(puVar20[1] + 0x38) + 0x40);
    *(uint **)((long)register0x00000008 + -0xe0) = puVar36;
    uVar5 = *puVar36;
    *(ulong *)((long)register0x00000008 + -0xc0) = (ulong)((long)register0x00000008 + -0x98) | 4;
    plVar27 = (long *)((ulong)(auVar48._0_4_ + (int)(((ulong)puVar20 & 0xffffffff) / 0x418) * -0x418
                              & 0x7f8) + 0x1540a30);
    *(undefined8 **)((long)register0x00000008 + -0xd0) = puVar43;
    *(long **)((long)register0x00000008 + -200) = plVar27;
    do {
      *(uint **)((long)register0x00000008 + -0xb0) = unaff_x22;
      lVar40 = *(long *)unaff_x22;
      if (lVar40 != 0) {
        do {
          puVar29 = *(undefined8 **)(lVar40 + 0x28);
          if ((puVar29 != (undefined8 *)0x0) && (lVar25 = *(long *)(lVar40 + 0x30), lVar25 != 0)) {
            if (lVar24 == *(long *)((long)register0x00000008 + -0xa8)) {
              lVar25 = *(long *)(lVar25 + 0x40);
            }
            else {
              iVar16 = ((int)puVar29 + (int)(((ulong)puVar29 & 0xffffffff) / 0x418) * -0x418 &
                       0x7f8U) + 0x1540a30;
              func_0x00281bec(iVar16);
              lVar25 = *(long *)(lVar25 + 0x40);
              func_0x00281b90(iVar16);
            }
            uVar14 = *(uint *)(lVar40 + 0x54);
            if (lVar24 == lVar25 || (uVar14 & 0x38000000) != 0) {
              uVar10 = uVar14 >> 0x1b & 7;
              if (uVar10 == 3) {
                if (lVar24 == lVar25) {
                  *(undefined4 *)((long)register0x00000008 + -0x98) = 2;
                  puVar28 = *(undefined8 **)((long)register0x00000008 + -0xc0);
                  *puVar28 = 0;
                  puVar28[1] = 0;
                  *(undefined4 *)(puVar28 + 2) = 0;
                  *(char **)((long)register0x00000008 + -0x80) = "default";
                  (**(code **)*puVar20)(puVar20);
                  uVar17 = func_0x003bbb68();
                  (**(code **)*puVar29)(puVar29);
                  uVar18 = func_0x003bbb68();
                  *(undefined8 *)((long)register0x00000008 + -0x100) = uVar18;
                  *(undefined8 **)((long)register0x00000008 + -0xf8) = puVar29;
                  *(undefined8 *)((long)register0x00000008 + -0x110) = uVar17;
                  *(undefined8 **)((long)register0x00000008 + -0x108) = puVar20;
                  func_0x00c78434((undefined1 *)((long)register0x00000008 + -0x98),
                                  "Qt: Dead lock detected while activating a BlockingQueuedConnection: Sender is %s(%p), receiver is %s(%p)"
                                 );
                }
                func_0x002846b0((undefined1 *)((long)register0x00000008 + -0x98),0);
                if (*plVar27 == 0) {
                  *plVar27 = 1;
                }
                else {
                  func_0x00281cac(plVar27);
                }
                if (*(long *)(lVar40 + 0x28) == 0) {
                  if (*plVar27 == 1) {
                    *plVar27 = in_xzr;
                  }
                  else {
                    func_0x0028232c(plVar27);
                  }
                }
                else {
                  if ((*(byte *)(lVar40 + 0x57) >> 6 & 1) == 0) {
                    puVar28 = (undefined8 *)__Znwm(0x70);
                    uVar7 = *(undefined2 *)(lVar40 + 0x50);
                    uVar8 = *(undefined2 *)(lVar40 + 0x52);
                    uVar17 = *(undefined8 *)(lVar40 + 0x38);
                    func_0x003bb49c(puVar28,0x2b);
                    *(uint *)((long)puVar28 + 0x14) = uVar30;
                    puVar28[3] = puVar20;
                    puVar28[4] = (undefined1 *)((long)register0x00000008 + -0x98);
                    *puVar28 = &UNK_013d2610;
                    uVar26 = *(ulong *)((long)register0x00000008 + -0xa0);
                    puVar28[5] = 0;
                    puVar28[6] = uVar26;
                    puVar28[7] = uVar17;
                    *(undefined4 *)(puVar28 + 8) = 0;
                    *(undefined2 *)((long)puVar28 + 0x44) = uVar7;
                    *(undefined2 *)((long)puVar28 + 0x46) = uVar8;
                    puVar28[10] = 0;
                    puVar28[9] = 0;
                    puVar28[0xc] = 0;
                    puVar28[0xb] = 0;
                    *(undefined4 *)(puVar28 + 0xd) = 0;
                    puVar43 = *(undefined8 **)((long)register0x00000008 + -0xd0);
                    plVar27 = *(long **)((long)register0x00000008 + -200);
                  }
                  else {
                    puVar28 = (undefined8 *)__Znwm(0x70);
                    puVar36 = *(uint **)(lVar40 + 0x38);
                    func_0x003bb49c(puVar28,0x2b);
                    *(uint *)((long)puVar28 + 0x14) = uVar30;
                    puVar28[3] = puVar20;
                    puVar28[4] = (undefined1 *)((long)register0x00000008 + -0x98);
                    *puVar28 = &UNK_013d2610;
                    uVar17 = *(undefined8 *)((long)register0x00000008 + -0xa0);
                    puVar28[5] = puVar36;
                    puVar28[6] = uVar17;
                    uVar26 = 0xffff000000000000;
                    puVar28[7] = 0;
                    puVar28[8] = 0xffff000000000000;
                    puVar28[10] = 0;
                    puVar28[9] = 0;
                    puVar28[0xc] = 0;
                    puVar28[0xb] = 0;
                    *(undefined4 *)(puVar28 + 0xd) = 0;
                    if (puVar36 != (uint *)0x0) {
                      LOAcquire();
                      uVar14 = *puVar36;
                      *puVar36 = uVar14 + 1;
                      uVar26 = (ulong)uVar14;
                      LORelease();
                    }
                  }
                  func_0x003b73bc(uVar26,puVar29,puVar28,0);
                  if (*plVar27 == 1) {
                    *plVar27 = in_xzr;
                  }
                  else {
                    func_0x0028232c(plVar27);
                  }
                  func_0x00284750((undefined1 *)((long)register0x00000008 + -0x98),1);
                }
                func_0x00284714((undefined1 *)((long)register0x00000008 + -0x98));
              }
              else {
                if (uVar10 == 2) goto LAB_003d4d54;
                puVar28 = puVar29;
                if (lVar24 != lVar25) {
                  puVar28 = (undefined8 *)0x0;
                }
                *(undefined8 **)((long)register0x00000008 + -0x90) = puVar28;
                *(undefined8 **)((long)register0x00000008 + -0x88) = puVar20;
                *(uint *)((long)register0x00000008 + -0x80) = uVar30;
                if (puVar28 != (undefined8 *)0x0) {
                  lVar25 = *(long *)(puVar28[1] + 0x40);
                  *(undefined8 *)((long)register0x00000008 + -0x98) = *(undefined8 *)(lVar25 + 0x18)
                  ;
                  *(undefined1 **)(lVar25 + 0x18) = (undefined1 *)((long)register0x00000008 + -0x98)
                  ;
                  uVar14 = *(uint *)(lVar40 + 0x54);
                }
                if ((uVar14 >> 0x1e & 1) == 0) {
                  if (*(long *)(lVar40 + 0x38) != 0) {
                    uVar9 = *(ushort *)(lVar40 + 0x50);
                    (**(code **)*puVar29)(puVar29);
                    iVar16 = func_0x003bc080();
                    if ((int)(uint)uVar9 <= iVar16) {
                      uVar9 = *(ushort *)(lVar40 + 0x52);
                      pcVar13 = *(code **)(lVar40 + 0x38);
                      *(uint *)((long)register0x00000008 + -0xd4) =
                           (uint)*(ushort *)(lVar40 + 0x50) + (uint)uVar9;
                      if ((code *)puVar43[1] != (code *)0x0) {
                        (*(code *)puVar43[1])
                                  (puVar29,*(undefined4 *)((long)register0x00000008 + -0xd4),
                                   *(undefined8 *)((long)register0x00000008 + -0xa0));
                      }
                      (*pcVar13)(puVar29,0,uVar9,*(undefined8 *)((long)register0x00000008 + -0xa0));
                      if ((code *)puVar43[3] != (code *)0x0) {
                        (*(code *)puVar43[3])
                                  (puVar29,*(undefined4 *)((long)register0x00000008 + -0xd4));
                      }
                      goto LAB_003d50a8;
                    }
                  }
                  iVar16 = (uint)*(ushort *)(lVar40 + 0x50) + (uint)*(ushort *)(lVar40 + 0x52);
                  if ((code *)puVar43[1] != (code *)0x0) {
                    (*(code *)puVar43[1])
                              (puVar29,iVar16,*(undefined8 *)((long)register0x00000008 + -0xa0));
                  }
                  func_0x003bbf14(puVar29,0,iVar16,*(undefined8 *)((long)register0x00000008 + -0xa0)
                                 );
                  if ((code *)puVar43[3] != (code *)0x0) {
                    (*(code *)puVar43[3])(puVar29,iVar16);
                  }
                }
                else {
                  LOAcquire();
                  **(int **)(lVar40 + 0x38) = **(int **)(lVar40 + 0x38) + 1;
                  LORelease();
                  piVar37 = *(int **)(lVar40 + 0x38);
                  (**(code **)(piVar37 + 2))
                            (1,piVar37,puVar29,*(undefined8 *)((long)register0x00000008 + -0xa0),0);
                  if (piVar37 != (int *)0x0) {
                    LOAcquire();
                    iVar16 = *piVar37;
                    *piVar37 = iVar16 + -1;
                    LORelease();
                    if (iVar16 == 1) {
                      (**(code **)(piVar37 + 2))(0,piVar37,0,0,0);
                    }
                  }
                }
LAB_003d50a8:
                if (*(long *)((long)register0x00000008 + -0x90) != 0) {
                  *(undefined8 *)
                   (*(long *)(*(long *)(*(long *)((long)register0x00000008 + -0x90) + 8) + 0x40) +
                   0x18) = *(undefined8 *)((long)register0x00000008 + -0x98);
                }
              }
            }
            else {
LAB_003d4d54:
              func_0x003d8bdc(puVar20,uVar30,lVar40,
                              *(undefined8 *)((long)register0x00000008 + -0xa0));
            }
          }
          lVar40 = *(long *)(lVar40 + 0x10);
        } while ((lVar40 != 0) && (*(uint *)(lVar40 + 0x4c) <= uVar5));
      }
      unaff_x22 = *(uint **)((long)register0x00000008 + -0xb8);
    } while (*(uint **)((long)register0x00000008 + -0xb0) != unaff_x22);
    piVar37 = *(int **)((long)register0x00000008 + -0xe0);
    iVar16 = *piVar37;
    if (piVar37 != (int *)0x0) {
      LOAcquire();
      iVar15 = piVar37[1];
      piVar37[1] = iVar15 + -1;
      LORelease();
      if (iVar15 == 1) {
        func_0x003cd314(*(undefined8 *)(piVar37 + 8));
        if (*(long *)(piVar37 + 2) != 0) {
          _free();
        }
        __ZdlPv(piVar37);
      }
    }
    if (iVar16 == 0) goto LAB_003d51cc;
    lVar24 = *(long *)(*(long *)((long)register0x00000008 + -0xe8) + 0x40);
    if ((*(long *)(lVar24 + 0x20) != 0) && (*(int *)(lVar24 + 4) == 1)) {
      if (**(long **)((long)register0x00000008 + -200) == 0) {
        **(long **)((long)register0x00000008 + -200) = 1;
      }
      else {
        func_0x00281cac();
      }
      iVar16 = *(int *)(lVar24 + 4);
      if (iVar16 < 2) {
        uVar17 = *(undefined8 *)(lVar24 + 0x20);
        *(undefined8 *)(lVar24 + 0x20) = 0;
      }
      else {
        uVar17 = 0;
      }
      if (**(long **)((long)register0x00000008 + -200) == 1) {
        **(long **)((long)register0x00000008 + -200) = in_xzr;
      }
      else {
        func_0x0028232c();
      }
      if (iVar16 < 2) {
        func_0x003cd314(uVar17);
      }
    }
  }
  if ((code *)puVar43[2] != (code *)0x0) {
    (*(code *)puVar43[2])(puVar20,uVar30);
  }
LAB_003d51cc:
  if (*(long *)PTR____stack_chk_guard_013b4320 == *(long *)((long)register0x00000008 + -0x70)) {
    return;
  }
  ___stack_chk_fail();
  uVar17 = func_0x00006028();
  __ZdlPv(unaff_x22);
  if (**(long **)((long)register0x00000008 + -200) == 1) {
    **(long **)((long)register0x00000008 + -200) = in_xzr;
  }
  else {
    func_0x0028232c(*(undefined8 *)((long)register0x00000008 + -200));
  }
  func_0x00284714((undefined1 *)((long)register0x00000008 + -0x98));
  if (*(long *)((long)register0x00000008 + -0xe0) != 0) {
    LOAcquire();
    iVar16 = *(int *)(*(long *)((long)register0x00000008 + -0xe0) + 4);
    *(int *)(*(long *)((long)register0x00000008 + -0xe0) + 4) = iVar16 + -1;
    LORelease();
    if (iVar16 == 1) {
      func_0x00c8048c(*(undefined8 *)((long)register0x00000008 + -0xe0),
                      *(undefined8 *)((long)register0x00000008 + -0xf0),
                      *(undefined8 *)((long)register0x00000008 + -0xe0));
    }
  }
  __Unwind_Resume(uVar17);
                    /* WARNING: Does not return */
  pcVar13 = (code *)SoftwareBreakpoint(1,0x3d5344);
  (*pcVar13)();
}


