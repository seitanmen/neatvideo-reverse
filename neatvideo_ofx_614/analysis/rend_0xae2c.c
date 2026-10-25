// func entry: 0000ae2c name: FUN_0000ae2c size: 3328

void FUN_0000ae2c(char *param_1,undefined8 *param_2,undefined8 param_3,undefined8 param_4)

{
  int *piVar1;
  int iVar2;
  undefined8 uVar3;
  long lVar4;
  long lVar5;
  size_t sVar6;
  int *piVar7;
  code *pcVar8;
  char *pcVar9;
  long *local_60;
  undefined8 local_58;
  undefined4 local_50;
  long local_48;
  
  local_48 = *(long *)PTR____stack_chk_guard_013b4320;
  iVar2 = _strcmp(param_1,"OfxActionLoad");
  if (iVar2 == 0) {
    if (DAT_0155b1e0 == (undefined8 *)0x0) goto LAB_0000b0e4;
    DAT_0155b1e8 = (undefined8 *)(*(code *)DAT_0155b1e0[1])(*DAT_0155b1e0,"OfxImageEffectSuite",1);
    DAT_0155b1f0 = (*(code *)DAT_0155b1e0[1])(*DAT_0155b1e0,"OfxPropertySuite",1);
    DAT_0155b1f8 = (*(code *)DAT_0155b1e0[1])(*DAT_0155b1e0,"OfxParameterSuite",1);
    DAT_0155b200 = (*(code *)DAT_0155b1e0[1])(*DAT_0155b1e0,"OfxDialogSuite",1);
    uVar3 = 4;
    if (((DAT_0155b1e8 == (undefined8 *)0x0) || (DAT_0155b1f0 == 0)) || (DAT_0155b1f8 == 0))
    goto LAB_0000b1ec;
    lVar4 = FUN_0000cf0c(*DAT_0155b1e0,"OfxPropName",0);
    local_50 = 0;
    local_58 = 0;
    (**(code **)(DAT_0155b1f0 + 0x78))(*DAT_0155b1e0,"OfxPropVersion",3,&local_58);
    if (lVar4 != 0) {
      DAT_0141a1c8 = FUN_00004edc(&local_58,lVar4);
      if (((DAT_0141a1c8 == 10) &&
          (DAT_0155b208 = (undefined8 *)
                          (*(code *)DAT_0155b1e0[1])(*DAT_0155b1e0,"OfxFilmLightSuite",1),
          DAT_0155b208 != (undefined8 *)0x0)) &&
         (iVar2 = (*(code *)*DAT_0155b208)(&DAT_0141a1e8), iVar2 == 0)) {
        DAT_0141a1e1 = '\x01';
      }
      lVar5 = _CFBundleGetBundleWithIdentifier(&cf_com_absoft_NeatVideo6_OFX);
      if (lVar5 != 0) {
        _CFRetain();
      }
      if (DAT_0141a1c8 != 0xd) {
        pcVar9 = *(char **)(&DAT_01419f30 + (ulong)DAT_0141a1c8 * 0x30);
        if (pcVar9 == (char *)0x0) {
          sVar6 = 0xffffffff;
        }
        else {
          sVar6 = _strlen(pcVar9);
        }
        piVar7 = (int *)FUN_002efa1c(pcVar9,sVar6);
        piVar1 = DAT_01538ac0;
        if (*DAT_01538ac0 != -1) {
          if (*DAT_01538ac0 != 0) {
            LOAcquire();
            iVar2 = *DAT_01538ac0;
            *DAT_01538ac0 = iVar2 + -1;
            DAT_01538ac0 = piVar7;
            LORelease();
            piVar7 = DAT_01538ac0;
            if (iVar2 != 1) goto LAB_0000b384;
          }
          DAT_01538ac0 = piVar7;
          FUN_00287df8(piVar1,2,8);
          piVar7 = DAT_01538ac0;
        }
LAB_0000b384:
        DAT_01538ac0 = piVar7;
        pcVar9 = *(char **)(&DAT_01419f28 + (ulong)DAT_0141a1c8 * 0x30);
        if (pcVar9 == (char *)0x0) {
          sVar6 = 0xffffffff;
        }
        else {
          sVar6 = _strlen(pcVar9);
        }
        piVar7 = (int *)FUN_002efa1c(pcVar9,sVar6);
        piVar1 = DAT_01538ab8;
        if (*DAT_01538ab8 != -1) {
          if (*DAT_01538ab8 != 0) {
            LOAcquire();
            iVar2 = *DAT_01538ab8;
            *DAT_01538ab8 = iVar2 + -1;
            DAT_01538ab8 = piVar7;
            LORelease();
            piVar7 = DAT_01538ab8;
            if (iVar2 != 1) goto LAB_0000b3f0;
          }
          DAT_01538ab8 = piVar7;
          FUN_00287df8(piVar1,2,8);
          piVar7 = DAT_01538ab8;
        }
LAB_0000b3f0:
        DAT_01538ab8 = piVar7;
        DAT_0141a1cc = DAT_0141a1c8 == 0;
        DAT_0141a1ce = DAT_0141a1c8 != 4;
        DAT_0141a1d3 = (DAT_0141a1c8 & 0xfffffff7) == 0;
        DAT_0141a1d1 = DAT_0141a1c8 == 10;
        DAT_0141a1cf = (bool)DAT_0141a1d1 || (bool)DAT_0141a1d3;
        DAT_0141a1d0 = DAT_0141a1ce;
        if (DAT_0141a1c8 == 3) {
          DAT_0141a1d4 = true;
LAB_0000b46c:
          DAT_0141a1d5 = true;
        }
        else {
          if (DAT_0141a1c8 == 9) {
            DAT_0141a1d4 = (int)local_58 == 4;
            goto LAB_0000b46c;
          }
          if (DAT_0141a1c8 == 10) {
            iVar2 = FUN_0000d55c(&local_58,"5.3.17711");
            DAT_0141a1d4 = iVar2 < 1;
          }
          else {
            DAT_0141a1d4 = false;
          }
          if (DAT_0141a1c8 < 0xb) {
            if ((1 << (ulong)(DAT_0141a1c8 & 0x1f) & 0x798U) == 0) {
              if ((DAT_0141a1c8 != 1) || ((int)local_58 < 0x10)) goto LAB_0000b790;
              DAT_0141a1d5 = true;
              goto LAB_0000b798;
            }
            DAT_0141a1d5 = true;
            if (DAT_0141a1c8 == 10) {
              DAT_0141a1dc = 1;
              DAT_0141a1e0 = 1;
            }
          }
          else {
LAB_0000b790:
            DAT_0141a1d5 = DAT_0141a1c8 == 0xc;
LAB_0000b798:
            if ((DAT_0141a1c8 == 1) && (8 < (int)local_58)) {
              DAT_0141a1d8 = 1;
            }
          }
        }
        uVar3 = FUN_000050ec(&local_58,lVar4,&DAT_0141a1c8);
        if ((int)uVar3 != 0) goto LAB_0000b1ec;
        (**(code **)(DAT_0155b1f0 + 0x58))
                  (*DAT_0155b1e0,"OfxImageEffectPropTemporalClipAccess",0,&local_60);
        if ((int)local_60 != 0) {
          if (DAT_0141a1c8 != 10) {
            FUN_000054e0();
          }
          goto LAB_0000b1e8;
        }
      }
    }
LAB_0000b0e4:
    uVar3 = 4;
  }
  else {
    iVar2 = _strcmp(param_1,"OfxActionUnload");
    if (iVar2 == 0) {
      FUN_00007f68();
      if (DAT_0141a1c8 == 10 && DAT_0141a1e1 != '\0') {
        (*(code *)DAT_0155b208[3])(&DAT_0141a1e8);
      }
      goto LAB_0000b1e8;
    }
    iVar2 = _strcmp(param_1,"OfxActionCreateInstance");
    if (iVar2 == 0) {
      uVar3 = 0;
      if (*(long *)PTR____stack_chk_guard_013b4320 == local_48) {
        FUN_00004e90(param_2);
        return;
      }
      goto LAB_0000bb28;
    }
    iVar2 = _strcmp(param_1,"OfxActionDestroyInstance");
    if (iVar2 == 0) {
      iVar2 = (*(code *)*DAT_0155b1e8)(param_2,&local_58);
      if (iVar2 != 0) {
        do {
          FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b32,(long)iVar2);
        } while( true );
      }
      local_60 = (long *)0x0;
      iVar2 = (**(code **)(DAT_0155b1f0 + 0x40))(local_58,"OfxPropInstanceData",0,&local_60);
      if ((iVar2 != 0 || local_60 == (long *)0x0) || ((undefined8 *)local_60[2] != param_2)) {
        do {
          FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b33,(long)iVar2);
        } while( true );
      }
      (**(code **)(*local_60 + 0x78))(local_60);
    }
    else {
      iVar2 = _strcmp(param_1,"OfxActionDescribe");
      if (iVar2 == 0) {
        FUN_0000ab68(param_2);
      }
      else {
        iVar2 = _strcmp(param_1,"OfxImageEffectActionDescribeInContext");
        if (iVar2 != 0) {
          iVar2 = _strcmp(param_1,"OfxActionInstanceChanged");
          if (iVar2 == 0) {
            iVar2 = (*(code *)*DAT_0155b1e8)(param_2,&local_58);
            if (iVar2 != 0) {
              do {
                FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b32,(long)iVar2);
              } while( true );
            }
            local_60 = (long *)0x0;
            iVar2 = (**(code **)(DAT_0155b1f0 + 0x40))(local_58,"OfxPropInstanceData",0,&local_60);
            if (((iVar2 != 0) || (local_60 == (long *)0x0)) ||
               ((undefined8 *)local_60[2] != param_2)) {
              do {
                FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b33,(long)iVar2);
              } while( true );
            }
            uVar3 = (**(code **)(*local_60 + 0x80))(local_60,param_3);
          }
          else {
            iVar2 = _strcmp(param_1,"OfxActionSyncPrivateData");
            if (iVar2 == 0) {
              iVar2 = (*(code *)*DAT_0155b1e8)(param_2,&local_58);
              if (iVar2 != 0) {
                do {
                  FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b32,(long)iVar2);
                } while( true );
              }
              local_60 = (long *)0x0;
              iVar2 = (**(code **)(DAT_0155b1f0 + 0x40))(local_58,"OfxPropInstanceData",0,&local_60)
              ;
              if (((iVar2 != 0) || (local_60 == (long *)0x0)) ||
                 ((undefined8 *)local_60[2] != param_2)) {
                do {
                  FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b33,(long)iVar2);
                } while( true );
              }
              FUN_00008d30(local_60);
              goto LAB_0000b1e8;
            }
            iVar2 = _strcmp(param_1,"OfxImageEffectActionGetFramesNeeded");
            if (iVar2 == 0) {
              iVar2 = (*(code *)*DAT_0155b1e8)(param_2,&local_58);
              if (iVar2 != 0) {
                do {
                  FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b32,(long)iVar2);
                } while( true );
              }
              local_60 = (long *)0x0;
              iVar2 = (**(code **)(DAT_0155b1f0 + 0x40))(local_58,"OfxPropInstanceData",0,&local_60)
              ;
              if (((iVar2 != 0) || (local_60 == (long *)0x0)) ||
                 ((undefined8 *)local_60[2] != param_2)) {
                do {
                  FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b33,(long)iVar2);
                } while( true );
              }
              pcVar8 = *(code **)(*local_60 + 0x88);
LAB_0000b9ec:
              uVar3 = (*pcVar8)(local_60,param_3,param_4);
            }
            else {
              iVar2 = _strcmp(param_1,"OfxImageEffectActionIsIdentity");
              if (iVar2 == 0) {
                iVar2 = (*(code *)*DAT_0155b1e8)(param_2,&local_58);
                if (iVar2 != 0) {
                  do {
                    FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b32,(long)iVar2)
                    ;
                  } while( true );
                }
                local_60 = (long *)0x0;
                iVar2 = (**(code **)(DAT_0155b1f0 + 0x40))
                                  (local_58,"OfxPropInstanceData",0,&local_60);
                if (((iVar2 != 0) || (local_60 == (long *)0x0)) ||
                   ((undefined8 *)local_60[2] != param_2)) {
                  do {
                    FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b33,(long)iVar2)
                    ;
                  } while( true );
                }
                uVar3 = FUN_00007104(local_60,param_3,param_4);
              }
              else {
                iVar2 = _strcmp(param_1,"OfxImageEffectActionRender");
                if (iVar2 != 0) {
                  iVar2 = _strcmp(param_1,"OfxActionDialog");
                  if (iVar2 == 0) {
                    FUN_00008dd4(param_2[2],param_2[1],*param_2,*(undefined4 *)(param_2 + 3));
                  }
                  else {
                    iVar2 = _strcmp(param_1,"OfxImageEffectActionBeginSequenceRender");
                    if (iVar2 == 0) {
                      iVar2 = (*(code *)*DAT_0155b1e8)(param_2,&local_58);
                      if (iVar2 != 0) {
                        do {
                          FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b32,
                                       (long)iVar2);
                        } while( true );
                      }
                      local_60 = (long *)0x0;
                      iVar2 = (**(code **)(DAT_0155b1f0 + 0x40))
                                        (local_58,"OfxPropInstanceData",0,&local_60);
                      if (((iVar2 != 0) || (local_60 == (long *)0x0)) ||
                         ((undefined8 *)local_60[2] != param_2)) {
                        do {
                          FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b33,
                                       (long)iVar2);
                        } while( true );
                      }
                      pcVar8 = *(code **)(*local_60 + 0x90);
                      goto LAB_0000b9ec;
                    }
                    iVar2 = _strcmp(param_1,"OfxImageEffectActionEndSequenceRender");
                    if (iVar2 != 0) {
                      uVar3 = 0xe;
                      goto LAB_0000b1ec;
                    }
                    iVar2 = (*(code *)*DAT_0155b1e8)(param_2,&local_58);
                    if (iVar2 != 0) {
                      do {
                        FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b32,
                                     (long)iVar2);
                      } while( true );
                    }
                    local_60 = (long *)0x0;
                    iVar2 = (**(code **)(DAT_0155b1f0 + 0x40))
                                      (local_58,"OfxPropInstanceData",0,&local_60);
                    if (((iVar2 != 0) || (local_60 == (long *)0x0)) ||
                       ((undefined8 *)local_60[2] != param_2)) {
                      do {
                        FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b33,
                                     (long)iVar2);
                      } while( true );
                    }
                  }
                  goto LAB_0000b1e8;
                }
                iVar2 = (*(code *)*DAT_0155b1e8)(param_2,&local_58);
                if (iVar2 != 0) {
                  do {
                    FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b32,(long)iVar2)
                    ;
                  } while( true );
                }
                local_60 = (long *)0x0;
                iVar2 = (**(code **)(DAT_0155b1f0 + 0x40))
                                  (local_58,"OfxPropInstanceData",0,&local_60);
                if (((iVar2 != 0) || (local_60 == (long *)0x0)) ||
                   ((undefined8 *)local_60[2] != param_2)) {
                  do {
                    FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xdfa35b33,(long)iVar2)
                    ;
                  } while( true );
                }
                uVar3 = FUN_000080a8(local_60,param_3);
              }
            }
          }
          goto LAB_0000b1ec;
        }
        FUN_0000a22c(param_2);
      }
    }
LAB_0000b1e8:
    uVar3 = 0;
  }
LAB_0000b1ec:
  if (*(long *)PTR____stack_chk_guard_013b4320 == local_48) {
    return;
  }
LAB_0000bb28:
                    /* WARNING: Subroutine does not return */
  ___stack_chk_fail(uVar3);
}


