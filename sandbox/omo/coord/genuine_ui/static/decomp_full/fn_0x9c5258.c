// FUN_009c5258 size=3556

void FUN_009c5258(long param_1,ulong param_2)

{
  float *pfVar1;
  float *pfVar2;
  byte bVar3;
  uint uVar4;
  uint uVar5;
  byte bVar6;
  char cVar7;
  int iVar8;
  undefined8 uVar9;
  float *pfVar10;
  byte *pbVar11;
  byte bVar12;
  byte *pbVar13;
  byte bVar14;
  float *pfVar15;
  float *pfVar16;
  long lVar17;
  long lVar18;
  long lVar19;
  float *pfVar20;
  long lVar21;
  undefined1 auStack_f0 [16];
  undefined *local_e0;
  long lStack_d8;
  undefined *local_d0;
  long lStack_c8;
  undefined *local_c0;
  long lStack_b8;
  undefined *local_b0;
  long lStack_a8;
  long local_a0;
  long local_98;
  undefined **local_90 [4];
  long local_70;
  
  local_70 = *(long *)PTR____stack_chk_guard_013b4320;
  if (*(char *)(param_1 + 0x59) == '\0') {
    FUN_0097f9e8(param_1 + 0x88);
    if (*(char *)(param_1 + 0x59) == '\0') {
      FUN_009c8fcc(param_1,1);
    }
    FUN_0097fb60(param_1 + 0x88);
  }
  lVar19 = *(long *)(param_1 + 0x68);
  FUN_0097fc98(auStack_f0,param_1 + 0x40,1);
  cVar7 = *(char *)(param_1 + 0xe0);
  if ((cVar7 != '\0') && (*(long *)(param_1 + 0x28) == lVar19)) goto LAB_009c5f04;
  pfVar15 = (float *)(param_1 + 0x18);
  *(undefined8 *)(param_1 + 0x20) = 0;
  pfVar15[0] = 0.0;
  pfVar15[1] = 0.0;
  pfVar16 = (float *)(param_1 + 8);
  *(undefined8 *)(param_1 + 0x10) = 0;
  pfVar16[0] = 0.0;
  pfVar16[1] = 0.0;
  *(undefined8 *)(param_1 + 0x28) = 0;
  if (cVar7 == '\0') goto LAB_009c5f04;
  pfVar1 = (float *)(param_1 + 0x24);
  pfVar2 = (float *)(param_1 + 0x20);
  pfVar20 = (float *)(param_1 + 0x14);
  pfVar10 = (float *)(param_1 + 0x10);
  iVar8 = *(int *)(param_1 + 0xf0);
  if (iVar8 == 2) {
    cVar7 = *(char *)(param_1 + 0xf4);
    lVar21 = *(long *)(param_1 + 200);
    lVar17 = *(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0);
    if ((param_2 & 1) == 0) {
      uVar9 = *(undefined8 *)(param_1 + 0xb0);
      uVar5 = *(uint *)(DAT_01489648 + 0x1998e00);
      if (DAT_01489538 != 0) {
        uVar5 = uVar5 >> 0x16 | uVar5 << 10;
      }
      DAT_01489538 = (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >>
                          0x10);
      if (DAT_01489538 != 0) {
        *(uint *)(DAT_01489650 + 0x1c2fbf0) = uVar5;
      }
      FUN_009efdd4(uVar9,pfVar16,pfVar15,lVar17);
      if (cVar7 == '\0') {
        uVar9 = *(undefined8 *)(param_1 + 0xb8);
        uVar5 = *(uint *)(DAT_01489648 + 0x1998e00);
        if (DAT_01489538 != 0) {
          uVar5 = uVar5 >> 0x16 | uVar5 << 10;
        }
        DAT_01489538 = (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >>
                            0x10);
        if (DAT_01489538 != 0) {
          *(uint *)(DAT_01489650 + 0x1c2fbf0) = uVar5;
        }
        FUN_009efdd4(uVar9,(float *)(param_1 + 0xc),(float *)(param_1 + 0x1c),
                     *(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0));
        uVar9 = *(undefined8 *)(param_1 + 0xc0);
        uVar5 = *(uint *)(DAT_01489648 + 0x1998e00);
        if (DAT_01489538 != 0) {
          uVar5 = uVar5 >> 0x16 | uVar5 << 10;
        }
        DAT_01489538 = (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >>
                            0x10);
        if (DAT_01489538 != 0) {
          *(uint *)(DAT_01489650 + 0x1c2fbf0) = uVar5;
        }
        FUN_009efdd4(uVar9,pfVar10,pfVar2,*(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0));
      }
      if (lVar21 != 0) {
        uVar9 = *(undefined8 *)(param_1 + 200);
        uVar5 = *(uint *)(DAT_01489648 + 0x1998e00);
        if (DAT_01489538 != 0) {
          uVar5 = uVar5 >> 0x16 | uVar5 << 10;
        }
        DAT_01489538 = (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >>
                            0x10);
        if (DAT_01489538 != 0) {
          *(uint *)(DAT_01489650 + 0x1c2fbf0) = uVar5;
        }
        FUN_009efdd4(uVar9,pfVar20,pfVar1,*(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0));
      }
      goto LAB_009c5ed0;
    }
    local_b0 = &DAT_013fda20;
    local_c0 = &DAT_013fda50;
    local_d0 = &DAT_013fda80;
    local_e0 = &DAT_013fdab0;
    local_90[0] = &local_b0;
    if (cVar7 == '\0') {
      local_90[1] = &local_c0;
      local_90[2] = &local_d0;
      lVar18 = 3;
      if (lVar21 != 0) goto LAB_009c5474;
LAB_009c5aa4:
      if (lVar18 == 1) {
        uVar9 = *(undefined8 *)(param_1 + 0xb0);
        uVar5 = *(uint *)(DAT_01489648 + 0x1998e00);
        if (DAT_01489538 != 0) {
          uVar5 = uVar5 >> 0x16 | uVar5 << 10;
        }
        DAT_01489538 = (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >>
                            0x10);
        if (DAT_01489538 != 0) {
          *(uint *)(DAT_01489650 + 0x1c2fbf0) = uVar5;
        }
        lStack_d8 = param_1;
        lStack_c8 = param_1;
        lStack_b8 = param_1;
        lStack_a8 = param_1;
        FUN_009efdd4(uVar9,pfVar16,pfVar15,lVar17);
        goto LAB_009c5ed0;
      }
    }
    else {
      lVar18 = 1;
      if (lVar21 == 0) goto LAB_009c5aa4;
LAB_009c5474:
      local_90[lVar18] = &local_e0;
      lVar18 = lVar18 + 1;
    }
    lStack_d8 = param_1;
    lStack_c8 = param_1;
    lStack_b8 = param_1;
    lStack_a8 = param_1;
    if (((DAT_01557dc0 & 1) == 0) && (iVar8 = ___cxa_guard_acquire(&DAT_01557dc0), iVar8 != 0)) {
      thunk_FUN_009fb9f0(&DAT_0154ea40,"EnsureMinMax.32");
      ___cxa_guard_release(&DAT_01557dc0);
    }
    FUN_009fc3a8(local_90,lVar18,DAT_014a67c0,&DAT_0154ea40,lVar18 * lVar17);
LAB_009c5ed0:
    cVar7 = *(char *)(param_1 + 0xf4);
  }
  else {
    if (iVar8 != 1) {
      if (iVar8 == 0) {
        cVar7 = *(char *)(param_1 + 0xf4);
        pbVar11 = *(byte **)(param_1 + 200);
        lVar21 = *(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0);
        if ((param_2 & 1) == 0) {
          pbVar13 = *(byte **)(param_1 + 0xb0);
          uVar5 = *(uint *)(DAT_01489620 + 0x1a2cf0);
          if (DAT_01489530 != 0) {
            uVar5 = uVar5 >> 0x15 | uVar5 << 0xb;
          }
          DAT_01489530 = (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5)
                              >> 0x10);
          if (DAT_01489530 != 0) {
            *(uint *)(DAT_01489628 + 0x691b60) = uVar5;
          }
          lVar17 = DAT_01563528;
          if (DAT_01563528 != 0) {
            DAT_01489530 = DAT_01489530 + (uint)*(byte *)(DAT_01563528 + 0x838e);
          }
          if (lVar21 < 1) {
            bVar12 = 0;
            bVar14 = 0xff;
          }
          else {
            bVar12 = *pbVar13;
            bVar6 = *pbVar13;
            lVar18 = lVar21;
            while (bVar14 = bVar6, lVar18 = lVar18 + -1, lVar18 != 0) {
              pbVar13 = pbVar13 + 1;
              bVar6 = *pbVar13;
              bVar3 = bVar6;
              if (bVar6 <= bVar12) {
                bVar3 = bVar12;
              }
              if (bVar14 <= bVar6) {
                bVar12 = bVar3;
                bVar6 = bVar14;
              }
            }
          }
          *pfVar16 = (float)bVar14;
          *pfVar15 = (float)bVar12;
          if (cVar7 == '\0') {
            pbVar13 = *(byte **)(param_1 + 0xb8);
            uVar5 = *(uint *)(DAT_01489620 + 0x1a2cf0);
            if (DAT_01489530 != 0) {
              uVar5 = uVar5 >> 0x15 | uVar5 << 0xb;
            }
            DAT_01489530 = (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5)
                                >> 0x10);
            if (DAT_01489530 != 0) {
              *(uint *)(DAT_01489628 + 0x691b60) = uVar5;
            }
            if (lVar17 != 0) {
              DAT_01489530 = DAT_01489530 + (uint)*(byte *)(lVar17 + 0x838e);
            }
            if (lVar21 < 1) {
              bVar12 = 0;
              bVar14 = 0xff;
            }
            else {
              bVar12 = *pbVar13;
              bVar6 = *pbVar13;
              lVar18 = lVar21;
              while (bVar14 = bVar6, lVar18 = lVar18 + -1, lVar18 != 0) {
                pbVar13 = pbVar13 + 1;
                bVar6 = *pbVar13;
                bVar3 = bVar6;
                if (bVar6 <= bVar12) {
                  bVar3 = bVar12;
                }
                if (bVar14 <= bVar6) {
                  bVar12 = bVar3;
                  bVar6 = bVar14;
                }
              }
            }
            *(float *)(param_1 + 0xc) = (float)bVar14;
            *(float *)(param_1 + 0x1c) = (float)bVar12;
            pbVar13 = *(byte **)(param_1 + 0xc0);
            uVar5 = *(uint *)(DAT_01489620 + 0x1a2cf0);
            if (DAT_01489530 != 0) {
              uVar5 = uVar5 >> 0x15 | uVar5 << 0xb;
            }
            DAT_01489530 = (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5)
                                >> 0x10);
            if (DAT_01489530 != 0) {
              *(uint *)(DAT_01489628 + 0x691b60) = uVar5;
            }
            if (lVar17 != 0) {
              DAT_01489530 = DAT_01489530 + (uint)*(byte *)(lVar17 + 0x838e);
            }
            if (lVar21 < 1) {
              bVar12 = 0;
              bVar14 = 0xff;
            }
            else {
              bVar12 = *pbVar13;
              bVar6 = *pbVar13;
              lVar18 = lVar21;
              while (bVar14 = bVar6, lVar18 = lVar18 + -1, lVar18 != 0) {
                pbVar13 = pbVar13 + 1;
                bVar6 = *pbVar13;
                bVar3 = bVar6;
                if (bVar6 <= bVar12) {
                  bVar3 = bVar12;
                }
                if (bVar14 <= bVar6) {
                  bVar12 = bVar3;
                  bVar6 = bVar14;
                }
              }
            }
            *pfVar10 = (float)bVar14;
            *pfVar2 = (float)bVar12;
          }
          if (pbVar11 != (byte *)0x0) {
            uVar5 = *(uint *)(DAT_01489620 + 0x1a2cf0);
            if (DAT_01489530 != 0) {
              uVar5 = uVar5 >> 0x15 | uVar5 << 0xb;
            }
            DAT_01489530 = (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5)
                                >> 0x10);
            if (DAT_01489530 != 0) {
              *(uint *)(DAT_01489628 + 0x691b60) = uVar5;
            }
            if (lVar17 != 0) {
              DAT_01489530 = DAT_01489530 + (uint)*(byte *)(lVar17 + 0x838e);
            }
            if (lVar21 < 1) {
              bVar12 = 0;
              bVar14 = 0xff;
            }
            else {
              bVar12 = *pbVar11;
              bVar6 = *pbVar11;
              while (bVar14 = bVar6, lVar21 = lVar21 + -1, lVar21 != 0) {
                pbVar11 = pbVar11 + 1;
                bVar6 = *pbVar11;
                bVar3 = bVar6;
                if (bVar6 <= bVar12) {
                  bVar3 = bVar12;
                }
                if (bVar14 <= bVar6) {
                  bVar12 = bVar3;
                  bVar6 = bVar14;
                }
              }
            }
            *pfVar20 = (float)bVar14;
            *pfVar1 = (float)bVar12;
          }
        }
        else {
          local_b0 = &DAT_013fd8a0;
          local_c0 = &DAT_013fd8d0;
          local_d0 = &DAT_013fd900;
          local_e0 = &DAT_013fd930;
          local_90[0] = &local_b0;
          if (cVar7 == '\0') {
            local_90[1] = &local_c0;
            local_90[2] = &local_d0;
            lVar17 = 3;
            if (pbVar11 != (byte *)0x0) goto LAB_009c539c;
LAB_009c5b68:
            if (lVar17 == 1) {
              pbVar11 = *(byte **)(param_1 + 0xb0);
              uVar5 = *(uint *)(DAT_01489620 + 0x1a2cf0);
              if (DAT_01489530 != 0) {
                uVar5 = uVar5 >> 0x15 | uVar5 << 0xb;
              }
              DAT_01489530 = (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),
                                            uVar5) >> 0x10);
              if (DAT_01489530 != 0) {
                *(uint *)(DAT_01489628 + 0x691b60) = uVar5;
              }
              if (DAT_01563528 != 0) {
                DAT_01489530 = DAT_01489530 + (uint)*(byte *)(DAT_01563528 + 0x838e);
              }
              if (lVar21 < 1) {
                bVar12 = 0;
                bVar14 = 0xff;
              }
              else {
                bVar12 = *pbVar11;
                bVar6 = *pbVar11;
                while (bVar14 = bVar6, lVar21 = lVar21 + -1, lVar21 != 0) {
                  pbVar11 = pbVar11 + 1;
                  bVar6 = *pbVar11;
                  bVar3 = bVar6;
                  if (bVar6 <= bVar12) {
                    bVar3 = bVar12;
                  }
                  if (bVar14 <= bVar6) {
                    bVar12 = bVar3;
                    bVar6 = bVar14;
                  }
                }
              }
              *pfVar16 = (float)bVar14;
              *pfVar15 = (float)bVar12;
              cVar7 = *(char *)(param_1 + 0xf4);
              lStack_d8 = param_1;
              lStack_c8 = param_1;
              lStack_b8 = param_1;
              lStack_a8 = param_1;
              goto joined_r0x009c5f94;
            }
          }
          else {
            lVar17 = 1;
            if (pbVar11 == (byte *)0x0) goto LAB_009c5b68;
LAB_009c539c:
            local_90[lVar17] = &local_e0;
            lVar17 = lVar17 + 1;
          }
          lStack_d8 = param_1;
          lStack_c8 = param_1;
          lStack_b8 = param_1;
          lStack_a8 = param_1;
          if (((DAT_01557db0 & 1) == 0) && (iVar8 = ___cxa_guard_acquire(&DAT_01557db0), iVar8 != 0)
             ) {
            thunk_FUN_009fb9f0(&DAT_0154d6a0,"EnsureMinMax.8");
            ___cxa_guard_release(&DAT_01557db0);
          }
          FUN_009fc3a8(local_90,lVar17,DAT_014a67c0,&DAT_0154d6a0,lVar17 * lVar21);
        }
      }
      goto LAB_009c5ed0;
    }
    cVar7 = *(char *)(param_1 + 0xf4);
    lVar21 = *(long *)(param_1 + 200);
    lVar17 = *(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0);
    if ((param_2 & 1) != 0) {
      local_b0 = &DAT_013fd960;
      local_c0 = &DAT_013fd990;
      local_d0 = &DAT_013fd9c0;
      local_e0 = &DAT_013fd9f0;
      local_90[0] = &local_b0;
      if (cVar7 == '\0') {
        local_90[1] = &local_c0;
        local_90[2] = &local_d0;
        lVar18 = 3;
        if (lVar21 != 0) goto LAB_009c5408;
LAB_009c5998:
        if (lVar18 == 1) {
          uVar9 = *(undefined8 *)(param_1 + 0xb0);
          uVar5 = *(uint *)(DAT_01489630 + 0x7f778);
          uVar4 = *(uint *)(DAT_01489640 + 0x6d0638);
          iVar8 = (int)(CONCAT44((uVar4 + (uVar4 >> 0x10)) * 0x101 + (uVar4 >> 0x18),uVar4) >> 0x10)
          ;
          if (iVar8 != 1) {
            if (DAT_01489534 != 0) {
              uVar5 = uVar5 >> 0x1a | uVar5 << 6;
            }
            *(uint *)(DAT_01489638 + 0x14b13c8) = uVar5;
          }
          DAT_01489534 = iVar8;
          lStack_d8 = param_1;
          lStack_c8 = param_1;
          lStack_b8 = param_1;
          lStack_a8 = param_1;
          FUN_009efeb0(uVar9,&local_98,&local_a0,lVar17);
          *pfVar16 = (float)local_98 * 0.0625;
          *pfVar15 = (float)local_a0 * 0.0625;
          cVar7 = *(char *)(param_1 + 0xf4);
          goto joined_r0x009c5f94;
        }
      }
      else {
        lVar18 = 1;
        if (lVar21 == 0) goto LAB_009c5998;
LAB_009c5408:
        local_90[lVar18] = &local_e0;
        lVar18 = lVar18 + 1;
      }
      lStack_d8 = param_1;
      lStack_c8 = param_1;
      lStack_b8 = param_1;
      lStack_a8 = param_1;
      if (((DAT_01557db8 & 1) == 0) && (iVar8 = ___cxa_guard_acquire(&DAT_01557db8), iVar8 != 0)) {
        thunk_FUN_009fb9f0(&DAT_0154e070,"EnsureMinMax.Half");
        ___cxa_guard_release(&DAT_01557db8);
      }
      FUN_009fc3a8(local_90,lVar18,DAT_014a67c0,&DAT_0154e070,lVar18 * lVar17);
      goto LAB_009c5ed0;
    }
    uVar9 = *(undefined8 *)(param_1 + 0xb0);
    uVar5 = *(uint *)(DAT_01489630 + 0x7f778);
    uVar4 = *(uint *)(DAT_01489640 + 0x6d0638);
    iVar8 = (int)(CONCAT44((uVar4 + (uVar4 >> 0x10)) * 0x101 + (uVar4 >> 0x18),uVar4) >> 0x10);
    if (iVar8 != 1) {
      if (DAT_01489534 != 0) {
        uVar5 = uVar5 >> 0x1a | uVar5 << 6;
      }
      *(uint *)(DAT_01489638 + 0x14b13c8) = uVar5;
    }
    DAT_01489534 = iVar8;
    FUN_009efeb0(uVar9,local_90,&local_b0,lVar17);
    *pfVar16 = (float)(long)local_90[0] * 0.0625;
    *pfVar15 = (float)(long)local_b0 * 0.0625;
    if (cVar7 == '\0') {
      uVar9 = *(undefined8 *)(param_1 + 0xb8);
      uVar5 = *(uint *)(DAT_01489630 + 0x7f778);
      uVar4 = *(uint *)(DAT_01489640 + 0x6d0638);
      iVar8 = (int)(CONCAT44((uVar4 + (uVar4 >> 0x10)) * 0x101 + (uVar4 >> 0x18),uVar4) >> 0x10);
      if (iVar8 != 1) {
        if (DAT_01489534 != 0) {
          uVar5 = uVar5 >> 0x1a | uVar5 << 6;
        }
        *(uint *)(DAT_01489638 + 0x14b13c8) = uVar5;
      }
      DAT_01489534 = iVar8;
      FUN_009efeb0(uVar9,local_90,&local_b0,*(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0));
      *(float *)(param_1 + 0xc) = (float)(long)local_90[0] * 0.0625;
      *(float *)(param_1 + 0x1c) = (float)(long)local_b0 * 0.0625;
      uVar9 = *(undefined8 *)(param_1 + 0xc0);
      uVar5 = *(uint *)(DAT_01489630 + 0x7f778);
      uVar4 = *(uint *)(DAT_01489640 + 0x6d0638);
      iVar8 = (int)(CONCAT44((uVar4 + (uVar4 >> 0x10)) * 0x101 + (uVar4 >> 0x18),uVar4) >> 0x10);
      if (iVar8 != 1) {
        if (DAT_01489534 != 0) {
          uVar5 = uVar5 >> 0x1a | uVar5 << 6;
        }
        *(uint *)(DAT_01489638 + 0x14b13c8) = uVar5;
      }
      DAT_01489534 = iVar8;
      FUN_009efeb0(uVar9,local_90,&local_b0,*(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0));
      *pfVar10 = (float)(long)local_90[0] * 0.0625;
      *pfVar2 = (float)(long)local_b0 * 0.0625;
    }
    if (lVar21 == 0) goto LAB_009c5ed0;
    uVar9 = *(undefined8 *)(param_1 + 200);
    uVar5 = *(uint *)(DAT_01489630 + 0x7f778);
    uVar4 = *(uint *)(DAT_01489640 + 0x6d0638);
    iVar8 = (int)(CONCAT44((uVar4 + (uVar4 >> 0x10)) * 0x101 + (uVar4 >> 0x18),uVar4) >> 0x10);
    if (iVar8 != 1) {
      if (DAT_01489534 != 0) {
        uVar5 = uVar5 >> 0x1a | uVar5 << 6;
      }
      *(uint *)(DAT_01489638 + 0x14b13c8) = uVar5;
    }
    DAT_01489534 = iVar8;
    FUN_009efeb0(uVar9,local_90,&local_b0,*(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0));
    *pfVar20 = (float)(long)local_90[0] * 0.0625;
    *pfVar1 = (float)(long)local_b0 * 0.0625;
    cVar7 = *(char *)(param_1 + 0xf4);
  }
joined_r0x009c5f94:
  if (cVar7 != '\0') {
    *pfVar10 = *pfVar16;
    *(float *)(param_1 + 0xc) = *pfVar16;
    *pfVar2 = *pfVar15;
    *(float *)(param_1 + 0x1c) = *pfVar15;
  }
  *(long *)(param_1 + 0x28) = lVar19;
LAB_009c5f04:
  FUN_0097fe08(auStack_f0);
  if (*(long *)PTR____stack_chk_guard_013b4320 != local_70) {
                    /* WARNING: Subroutine does not return */
    ___stack_chk_fail();
  }
  return;
}


