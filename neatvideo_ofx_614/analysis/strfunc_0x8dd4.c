// func entry: 00008dd4 name: FUN_00008dd4 size: 4176

undefined8 FUN_00008dd4(undefined8 param_1,undefined8 param_2,undefined4 param_3)

{
  byte bVar1;
  bool bVar2;
  bool bVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  long *plVar7;
  long lVar8;
  long lVar9;
  size_t sVar10;
  int *piVar11;
  long *plVar12;
  ulong uVar13;
  undefined8 uVar14;
  undefined8 uVar15;
  ulong uVar16;
  long lVar17;
  long lVar18;
  int *piVar19;
  long extraout_x1;
  int **ppiVar20;
  undefined **ppuVar21;
  code *pcVar22;
  char *pcVar23;
  int *piVar24;
  long lVar25;
  double extraout_d0;
  double dVar26;
  double dVar27;
  int *local_5318;
  long local_5310;
  long lStack_5308;
  undefined8 local_5300;
  undefined4 local_52f8;
  undefined4 uStack_52f4;
  int local_52f0;
  undefined4 uStack_52ec;
  uint local_52e8;
  undefined4 uStack_52e4;
  undefined8 local_52e0;
  undefined8 local_52d0;
  undefined8 uStack_52c8;
  int local_52c0;
  char local_52b9;
  long local_52b8;
  undefined8 local_52b0;
  double local_52a8;
  double local_52a0;
  int *local_5298;
  undefined *local_5290 [3];
  undefined **local_5278;
  undefined *local_5270;
  undefined *puStack_5268;
  undefined8 uStack_5260;
  undefined8 uStack_5258;
  undefined4 local_5228;
  long local_4f20;
  long local_4d60;
  undefined4 local_4670;
  char local_466c;
  int **local_4668;
  undefined *local_4660;
  undefined1 *local_4658;
  undefined1 *local_4650;
  undefined1 *local_4648;
  undefined1 *local_4640;
  undefined1 *local_4638;
  undefined1 *local_4630;
  undefined1 *local_4628;
  undefined1 *local_4620;
  undefined1 *local_4618;
  undefined1 *local_4610;
  undefined1 *local_4608;
  undefined1 *local_4600;
  undefined1 *local_45f8;
  int *local_45f0;
  undefined8 uStack_45e8;
  undefined8 local_45e0;
  undefined8 uStack_45d8;
  long local_4528;
  undefined auStack_4150 [200];
  long local_4088;
  undefined1 auStack_3cb0 [200];
  long local_3be8;
  undefined1 auStack_3810 [200];
  long local_3748;
  undefined1 auStack_3370 [200];
  long local_32a8;
  undefined1 auStack_2ed0 [200];
  long local_2e08;
  undefined1 auStack_2a30 [200];
  long local_2968;
  undefined1 auStack_2590 [200];
  char *local_24c8;
  long local_24c0;
  long local_24b8;
  char cStack_24b0;
  long local_24a8;
  undefined1 auStack_20f0 [200];
  long local_2028;
  undefined1 auStack_1c50 [200];
  long local_1b88;
  undefined1 auStack_17b0 [200];
  long local_16e8;
  undefined1 auStack_1310 [200];
  long local_1248;
  undefined1 auStack_e70 [200];
  long local_da8;
  undefined1 auStack_9d0 [200];
  long local_908;
  undefined1 auStack_530 [200];
  long local_468;
  long local_88;
  
  plVar7 = (long *)(*(code *)PTR____chkstk_darwin_013b4308)();
  local_88 = *(long *)PTR____stack_chk_guard_013b4320;
  if (DAT_0141a1d5 != '\0') {
    (**(code **)(DAT_0155b1f0 + 0x40))(*DAT_0155b1e0,"OfxPropHostOSHandle",0,plVar7 + 4);
  }
  lVar8 = plVar7[4];
  if (lVar8 == 0) {
    lVar8 = FUN_000035bc(DAT_0141a1c8 == 0);
    plVar7[4] = lVar8;
  }
  local_5298 = &DAT_0142f0e0;
  FUN_0015b6fc(lVar8,&local_5298);
  piVar19 = local_5298;
  if (*local_5298 != -1) {
    if (*local_5298 != 0) {
      LOAcquire();
      iVar4 = *local_5298;
      *local_5298 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_00008ed4;
    }
    iVar4 = local_5298[3];
    if (iVar4 != local_5298[2]) {
      lVar8 = (long)local_5298[2] * 8 + (long)iVar4 * -8;
      piVar24 = local_5298 + (long)iVar4 * 2 + 2;
      do {
        piVar11 = *(int **)piVar24;
        if (*piVar11 != -1) {
          if (*piVar11 != 0) {
            LOAcquire();
            iVar4 = *piVar11;
            *piVar11 = iVar4 + -1;
            LORelease();
            if (iVar4 != 1) goto LAB_0000903c;
            piVar11 = *(int **)piVar24;
          }
          FUN_00287df8(piVar11,2,8);
        }
LAB_0000903c:
        piVar24 = piVar24 + -2;
        lVar8 = lVar8 + 8;
      } while (lVar8 != 0);
    }
    _free(piVar19);
  }
LAB_00008ed4:
  if (DAT_0155c408 != 1) {
    FUN_00af4a8c(&DAT_0155c400);
  }
  lVar8 = FUN_001c05cc(DAT_0155c418,plVar7[3]);
  lVar25 = plVar7[3];
  FUN_00085ebc();
  if (extraout_x1 == 0) {
    (**(code **)(*plVar7 + 0x38))(plVar7);
    FUN_00083944();
  }
  else {
    FUN_001c0ef8();
    local_52b8 = 0;
    (**(code **)(DAT_0155b1e8 + 0x20))(extraout_x1,&local_52b0);
    (**(code **)(DAT_0155b1f0 + 0x70))(local_52b0,"OfxImageEffectPropFrameRange",2,&local_52a8);
    if (DAT_0141a1d4 != '\0') {
      local_52a0 = local_52a0 + -1.0;
    }
    lVar9 = FUN_001b45e0((long)extraout_d0,(long)local_52a8,(long)((local_52a0 - local_52a8) + 1.0),
                         5,1,&local_52b9);
    if (local_52b9 != '\0') {
      FUN_00083944(
                  "You have not selected a frame within the clip before opening Neat Video window.\nFrames from the beginning of the clip will be used.\n"
                  );
    }
    pcVar23 = *(char **)((ulong)DAT_0141a1c8 * 0x30 + 0x1419f48);
    if (pcVar23 == (char *)0x0) {
      sVar10 = 0xffffffff;
    }
    else {
      sVar10 = _strlen(pcVar23);
    }
    local_45f0 = (int *)FUN_002efa1c(pcVar23,sVar10);
    plVar12 = (long *)FUN_001b464c(&local_45f0);
    dVar26 = (double)lVar9;
    if (*local_45f0 != -1) {
      if (*local_45f0 != 0) {
        LOAcquire();
        iVar4 = *local_45f0;
        *local_45f0 = iVar4 + -1;
        LORelease();
        if (iVar4 != 1) goto LAB_000090c4;
      }
      FUN_00287df8(local_45f0,2,8);
    }
LAB_000090c4:
    bVar2 = true;
    bVar3 = false;
    if (local_52a8 <= dVar26) {
      bVar2 = false;
      bVar3 = true;
      if (!NAN(local_52a0) && !NAN(dVar26)) {
        bVar2 = local_52a0 < dVar26;
        bVar3 = false;
      }
    }
    if (bVar2 == bVar3) {
      uStack_45d8 = 0;
      local_45e0 = 0;
      uStack_45e8 = 0;
      local_45f0 = (int *)0x0;
      if (DAT_0141a1d3 == '\0') {
        pcVar22 = *(code **)(DAT_0155b1e8 + 0x28);
LAB_00009134:
        ppiVar20 = (int **)0x0;
      }
      else {
        (**(code **)(DAT_0155b1e8 + 0x38))(dVar26,extraout_x1,&local_45f0);
        pcVar22 = *(code **)(DAT_0155b1e8 + 0x28);
        if (DAT_0141a1d3 == '\0') goto LAB_00009134;
        ppiVar20 = &local_45f0;
      }
      iVar4 = (*pcVar22)(dVar26,extraout_x1,ppiVar20,&local_52b8);
      if (iVar4 != 0) {
        if (dVar26 == extraout_d0) {
          dVar27 = dVar26;
          if (DAT_0141a1d3 != '\0') goto LAB_0000917c;
        }
        else {
          bVar2 = 1 < DAT_0141a1c8 - 5;
          dVar27 = extraout_d0;
          if (bVar2) {
            dVar27 = dVar26;
          }
          if ((DAT_0141a1d3 != '\0') || (dVar26 = dVar27, !bVar2)) {
LAB_0000917c:
            iVar4 = (**(code **)(DAT_0155b1e8 + 0x28))(dVar27,extraout_x1,0,&local_52b8);
            dVar26 = dVar27;
          }
        }
      }
    }
    else {
      iVar4 = 0;
    }
    if ((iVar4 == 0) && (local_52b8 != 0)) {
      if (DAT_0141a1c8 - 5 < 2) {
        uVar13 = FUN_0000d3f8();
        if ((uVar13 & 1) == 0) {
          (**(code **)(DAT_0155b1e8 + 0x30))(local_52b8);
          local_52b8 = 0;
        }
        else if (local_52b8 != 0) goto LAB_000091e8;
      }
      else {
        if ((DAT_0141a1c8 != 7) || (0.0 < local_52a0 - local_52a8)) {
LAB_000091e8:
          FUN_008cd4a4(plVar12,1);
          uVar14 = FUN_0000d238(local_52b8);
          uVar5 = FUN_0014bfcc(uVar14,*(undefined1 *)(*DAT_013b94b0 + 0x388));
          uVar15 = FUN_0000cf0c(local_52b0,"OfxImageClipPropFieldOrder",DAT_0141a1c8 == 1);
          FUN_0000d4b4(dVar26,uVar15,&local_52c0);
          local_52d0 = 0;
          uStack_52c8 = 0;
          iVar4 = (**(code **)(DAT_0155b1f0 + 0x78))(local_52b8,"OfxImagePropBounds",4,&local_52d0);
          if (iVar4 != 0) {
            do {
              FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,(long)iVar4,0xcc2c7cff);
            } while( true );
          }
          lVar9 = FUN_001bdcf8(lVar8);
          local_52f0 = local_52c0;
          if (((local_52c0 == 0) && (DAT_0141a1cd != 0)) &&
             (local_52f0 = 0, *(int *)(lVar9 + 0xc2b0) != 0)) {
            local_52f0 = 3;
            local_52c0 = 3;
          }
          local_5310 = (long)((int)uStack_52c8 - (int)local_52d0);
          lStack_5308 = (long)((int)((ulong)uStack_52c8 >> 0x20) - (int)((ulong)local_52d0 >> 0x20))
          ;
          local_52e8 = (uint)DAT_0141a1ce;
          if (DAT_0141a1ce != 0) {
            local_52e8 = *(uint *)(lVar9 + 0xc2bc);
          }
          uStack_52e4 = *(undefined4 *)(lVar9 + 0xc2f4);
          local_5300 = 0;
          uStack_52ec = 0;
          local_52e0 = 0xffffffffffffffff;
          local_52f8 = (int)uVar14;
          uStack_52f4 = uVar5;
          FUN_001bdd20(lVar8);
          FUN_009cbea4(&local_45f0);
          FUN_009cbea4();
          FUN_009cbea4();
          FUN_009cbea4();
          FUN_009cbea4();
          FUN_009cbea4();
          FUN_009cbea4();
          FUN_009cbea4();
          FUN_009cbea4();
          FUN_009cbea4();
          FUN_009cbea4();
          FUN_009cbea4();
          FUN_009cbea4();
          FUN_009cbea4();
          FUN_009cbea4(auStack_530);
          FUN_001ba73c(&local_5310,auStack_2590,1,1);
          local_5270 = &DAT_013ff668;
          puStack_5268 = &DAT_0156a548;
          iVar4 = (**(code **)(*plVar7 + 0x10))
                            (plVar7,local_52b8,auStack_2590,&local_5310,&local_5270);
          local_5270 = &DAT_013ff668;
          FUN_00a06c24(&local_5270);
          local_5270 = &DAT_013ff6a0;
          FUN_00a0696c(&local_5270);
          (**(code **)(DAT_0155b1e8 + 0x30))(local_52b8);
          if (iVar4 == 0) {
            if (plVar12 != (long *)0x0) {
              (**(code **)(*plVar12 + 0x20))(plVar12);
            }
            (**(code **)(*plVar7 + 0x38))(plVar7);
            plVar12 = (long *)0x0;
            FUN_00083944();
          }
          else {
            uVar13 = 0;
            ppiVar20 = &local_45f0;
            do {
              if (uVar13 - 7 != 0) {
                dVar27 = dVar26 + (double)(long)(uVar13 - 7);
                bVar2 = false;
                bVar3 = true;
                if (local_52a8 <= dVar27) {
                  bVar2 = false;
                  bVar3 = true;
                  if (!NAN(dVar27) && !NAN(local_52a0)) {
                    bVar2 = dVar27 == local_52a0;
                    bVar3 = local_52a0 <= dVar27;
                  }
                }
                if (!bVar3 || bVar2) {
                  local_52b8 = 0;
                  puStack_5268 = (undefined *)0x0;
                  local_5270 = (undefined *)0x0;
                  uStack_5258 = 0;
                  uStack_5260 = 0;
                  if (DAT_0141a1d3 == '\0') {
                    pcVar22 = *(code **)(DAT_0155b1e8 + 0x28);
LAB_00009584:
                    ppuVar21 = (undefined **)0x0;
                  }
                  else {
                    (**(code **)(DAT_0155b1e8 + 0x38))(dVar27,extraout_x1,&local_5270);
                    pcVar22 = *(code **)(DAT_0155b1e8 + 0x28);
                    if (DAT_0141a1d3 == '\0') goto LAB_00009584;
                    ppuVar21 = &local_5270;
                  }
                  iVar4 = (*pcVar22)(dVar27,extraout_x1,ppuVar21,&local_52b8);
                  if (iVar4 == 0) {
LAB_000095dc:
                    FUN_001ba73c(&local_5310,ppiVar20,1,1);
                    local_4668 = (int **)&DAT_013ff668;
                    local_4660 = &DAT_0156a548;
                    iVar6 = (**(code **)(*plVar7 + 0x10))
                                      (plVar7,local_52b8,ppiVar20,&local_5310,&local_4668);
                    local_4668 = (int **)&DAT_013ff668;
                    FUN_00a06c24(&local_4668);
                    local_4668 = (int **)&DAT_013ff6a0;
                    FUN_00a0696c(&local_4668);
                    iVar4 = 0;
                  }
                  else {
                    if (DAT_0141a1d3 == '\0') {
                      iVar4 = 1;
                    }
                    else {
                      iVar4 = (**(code **)(DAT_0155b1e8 + 0x28))(dVar27,extraout_x1,0,&local_52b8);
                      if (iVar4 == 0) goto LAB_000095dc;
                    }
                    iVar6 = 0;
                  }
                  if (local_52b8 != 0) {
                    (**(code **)(DAT_0155b1e8 + 0x30))();
                  }
                  if ((iVar4 != 0) || (iVar6 != 1)) {
                    FUN_001ba73c(&local_5310,ppiVar20,1,1);
                    FUN_009c4318(ppiVar20,1);
                  }
                }
                else {
                  FUN_001ba73c(&local_5310,ppiVar20,1,1);
                  FUN_009c4318(ppiVar20,1);
                }
                iVar4 = 1;
                if (uVar13 < 7) {
                  iVar4 = 2;
                }
                uVar16 = FUN_008cd3ec(plVar12);
                if ((uVar16 & 1) != 0) goto LAB_00009ca0;
                FUN_008cd4a4(plVar12,iVar4 + (int)uVar13);
              }
              lVar9 = local_4528;
              uVar13 = uVar13 + 1;
              ppiVar20 = ppiVar20 + 0x94;
            } while (uVar13 != 0xf);
            if (DAT_0141a1c8 == 3) {
LAB_00009758:
              local_4528 = 0;
              if (lVar9 != 0) {
                FUN_009d7910();
              }
              lVar9 = local_4088;
              local_4088 = 0;
              if (lVar9 != 0) {
                FUN_009d7910();
              }
              lVar9 = local_3be8;
              local_3be8 = 0;
              if (lVar9 != 0) {
                FUN_009d7910();
              }
              lVar9 = local_3748;
              local_3748 = 0;
              if (lVar9 != 0) {
                FUN_009d7910();
              }
              lVar9 = local_32a8;
              local_32a8 = 0;
              if (lVar9 != 0) {
                FUN_009d7910();
              }
              lVar9 = local_2e08;
              local_2e08 = 0;
              if (lVar9 != 0) {
                FUN_009d7910();
              }
              lVar9 = local_2968;
              local_2968 = 0;
              if (lVar9 != 0) {
                FUN_009d7910();
              }
              pcVar23 = local_24c8;
              local_24c8 = (char *)0x0;
              if (pcVar23 != (char *)0x0) {
                FUN_009d7910();
              }
              lVar9 = local_2028;
              local_2028 = 0;
              if (lVar9 != 0) {
                FUN_009d7910();
              }
              lVar9 = local_1b88;
              local_1b88 = 0;
              if (lVar9 != 0) {
                FUN_009d7910();
              }
              lVar9 = local_16e8;
              local_16e8 = 0;
              if (lVar9 != 0) {
                FUN_009d7910();
              }
              lVar9 = local_1248;
              local_1248 = 0;
              if (lVar9 != 0) {
                FUN_009d7910();
              }
              lVar9 = local_da8;
              local_da8 = 0;
              if (lVar9 != 0) {
                FUN_009d7910();
              }
              lVar9 = local_908;
              local_908 = 0;
              if (lVar9 != 0) {
                FUN_009d7910();
              }
              lVar9 = local_468;
              local_468 = 0;
              if (lVar9 != 0) {
                FUN_009d7910();
              }
            }
            else if ((cStack_24b0 != '\0') && (local_24c8 != (char *)0x0)) {
              pcVar23 = local_24c8;
              for (local_24a8 = local_24b8 * local_24c0 * local_24a8; local_24a8 != 0;
                  local_24a8 = local_24a8 + -1) {
                if (*pcVar23 != '\0') goto LAB_000098b0;
                pcVar23 = pcVar23 + 1;
              }
              goto LAB_00009758;
            }
LAB_000098b0:
            local_4668 = &local_45f0;
            local_4660 = auStack_4150;
            local_4658 = auStack_3cb0;
            local_4650 = auStack_3810;
            local_4648 = auStack_3370;
            local_4640 = auStack_2ed0;
            local_4638 = auStack_2a30;
            local_4630 = auStack_2590;
            local_4628 = auStack_20f0;
            local_4620 = auStack_1c50;
            local_4618 = auStack_17b0;
            local_4610 = auStack_1310;
            local_4608 = auStack_e70;
            local_4600 = auStack_9d0;
            local_45f8 = auStack_530;
            iVar4 = FUN_00085e28();
            if (iVar4 != 0) {
              thunk_FUN_001530c4(&local_5270);
              local_4670 = param_3;
              FUN_001563b0(&local_5270,&local_4668);
              lVar9 = FUN_001bdcf8(lVar8);
              FUN_0017cd18(local_4f20,lVar9,lVar9 + 0xe038);
              FUN_00166d78(local_4d60,lVar9 + 0xd528,lVar9 + 0xe040);
              FUN_001bdd20(lVar8);
              local_5278 = local_5290;
              local_5290[0] = &DAT_013b4f28;
              bVar1 = DAT_0141a1cd;
              if (DAT_0141a1ce != 0) {
                bVar1 = DAT_0141a1cd | 2;
              }
              local_5228 = (int)uVar14;
              FUN_00096138(0,&local_5270,bVar1 | 0x30,local_5290);
              iVar4 = FUN_001c0db8(lVar8 + 0x254d8,lVar8 + 0x254d8);
              if (iVar4 == 0) {
                if (local_466c != '\0') goto LAB_00009a28;
                if (DAT_0141a1c8 == 9) {
                  FUN_0000d0a8(plVar7[2],"ParamsHash3",1);
                  goto LAB_00009c18;
                }
              }
              else {
                if (DAT_0155c408 != 1) {
                  FUN_00af4a8c(&DAT_0155c400);
                }
                FUN_001c0ce8(DAT_0155c418);
LAB_00009a28:
                if (DAT_0141a1dc == 1) {
                  lVar9 = FUN_001bdcf8(lVar8);
                  lVar17 = FUN_0000594c(1);
                  plVar7[3] = lVar17;
                  if (DAT_0155c408 != 1) {
                    FUN_00af4a8c(&DAT_0155c400);
                    lVar17 = plVar7[3];
                  }
                  lVar17 = FUN_001c070c(DAT_0155c418,lVar17,FUN_00005c78);
                  lVar18 = FUN_001bfaf0();
                  FUN_0099fb7c(lVar18,lVar9);
                  _memcpy((void *)(lVar18 + 0xd528),(void *)(lVar9 + 0xd528),0xb0c);
                  FUN_002e79b0(lVar18 + 0xe038,lVar9 + 0xe038);
                  FUN_002e79b0(lVar18 + 0xe040,lVar9 + 0xe040);
                  *(undefined8 *)(lVar18 + 0xe048) = *(undefined8 *)(lVar9 + 0xe048);
                  FUN_001bdd20(lVar17);
                  FUN_001bdd20(lVar8);
                  if (DAT_0155c408 != 1) {
                    FUN_00af4a8c(&DAT_0155c400);
                  }
                  FUN_001c087c(DAT_0155c418,lVar8);
                  lVar8 = lVar17;
                }
                if (local_466c != '\0') {
                  lVar9 = FUN_001bfaf0(lVar8);
                  FUN_0099fb7c(lVar9,*(undefined8 *)(local_4f20 + 0x28));
                  _memcpy((void *)(lVar9 + 0xd528),*(void **)(local_4d60 + 0x28),0xb0c);
                  FUN_0017b1ec(&local_5318,local_4f20);
                  piVar19 = *(int **)(lVar9 + 0xe038);
                  *(int **)(lVar9 + 0xe038) = local_5318;
                  local_5318 = piVar19;
                  if (*piVar19 != -1) {
                    if (*piVar19 != 0) {
                      LOAcquire();
                      iVar4 = *piVar19;
                      *piVar19 = iVar4 + -1;
                      LORelease();
                      if (iVar4 != 1) goto LAB_00009bbc;
                    }
                    FUN_00287df8(piVar19,2,8);
                  }
LAB_00009bbc:
                  FUN_00166090(&local_5318,local_4d60);
                  piVar19 = *(int **)(lVar9 + 0xe040);
                  *(int **)(lVar9 + 0xe040) = local_5318;
                  local_5318 = piVar19;
                  if (*piVar19 != -1) {
                    if (*piVar19 != 0) {
                      LOAcquire();
                      iVar4 = *piVar19;
                      *piVar19 = iVar4 + -1;
                      LORelease();
                      if (iVar4 != 1) goto LAB_00009c08;
                    }
                    FUN_00287df8(piVar19,2,8);
                  }
LAB_00009c08:
                  FUN_001bdd20(lVar8);
                }
                FUN_0000610c(plVar7);
LAB_00009c18:
                if ((local_466c != '\0') && ((DAT_01538ab0 & 1) == 0)) {
                  DAT_01538ab0 = 1;
                  FUN_00083944(
                              "This Demo edition of Neat Video has the following limitations:\n\n1. It will process up to 15 minutes of video data;\n\n2. It will filter up to 1800x1000 pixels in each frame;\n\n3. It will put a \"Neat Video Demo\" label in each filtered frame."
                              );
                }
              }
              if (local_5290 == local_5278) {
                pcVar22 = *(code **)(*local_5278 + 0x20);
LAB_00009c94:
                (*pcVar22)();
              }
              else if (local_5278 != (undefined **)0x0) {
                pcVar22 = *(code **)(*local_5278 + 0x28);
                goto LAB_00009c94;
              }
              thunk_FUN_00153b30(&local_5270);
            }
          }
LAB_00009ca0:
          thunk_FUN_009cc09c(auStack_530);
          thunk_FUN_009cc09c(auStack_9d0);
          thunk_FUN_009cc09c(auStack_e70);
          thunk_FUN_009cc09c(auStack_1310);
          thunk_FUN_009cc09c(auStack_17b0);
          thunk_FUN_009cc09c(auStack_1c50);
          thunk_FUN_009cc09c(auStack_20f0);
          thunk_FUN_009cc09c(auStack_2590);
          thunk_FUN_009cc09c(auStack_2a30);
          thunk_FUN_009cc09c(auStack_2ed0);
          thunk_FUN_009cc09c(auStack_3370);
          thunk_FUN_009cc09c(auStack_3810);
          thunk_FUN_009cc09c(auStack_3cb0);
          thunk_FUN_009cc09c(auStack_4150);
          thunk_FUN_009cc09c(&local_45f0);
          goto joined_r0x00009d2c;
        }
        (**(code **)(DAT_0155b1e8 + 0x30))();
        local_52b8 = 0;
      }
      if (plVar12 != (long *)0x0) {
        (**(code **)(*plVar12 + 0x20))(plVar12);
      }
      (**(code **)(*plVar7 + 0x38))(plVar7);
      FUN_00083944();
    }
    else {
      if (plVar12 != (long *)0x0) {
        (**(code **)(*plVar12 + 0x20))(plVar12);
      }
      (**(code **)(*plVar7 + 0x38))(plVar7);
      FUN_00083944();
    }
  }
  plVar12 = (long *)0x0;
joined_r0x00009d2c:
  if (DAT_0155c408 != 1) {
    FUN_00af4a8c(&DAT_0155c400);
  }
  FUN_001c0784(DAT_0155c418,lVar25);
  if (plVar12 != (long *)0x0) {
    (**(code **)(*plVar12 + 0x20))(plVar12);
  }
  FUN_0015c654();
  DAT_0141a201 = 0;
  if (*(long *)PTR____stack_chk_guard_013b4320 == local_88) {
    return 0;
  }
                    /* WARNING: Subroutine does not return */
  ___stack_chk_fail();
}


