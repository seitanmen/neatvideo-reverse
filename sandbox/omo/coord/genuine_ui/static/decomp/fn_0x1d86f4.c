// FUN_001d86f4 size=3784

/* WARNING: Removing unreachable block (ram,0x003d53e4) */
/* WARNING: Removing unreachable block (ram,0x003d4bc8) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_001d86f4(undefined8 *param_1)

{
  long *plVar1;
  undefined2 uVar2;
  undefined2 uVar3;
  ushort uVar4;
  uint uVar5;
  code *pcVar6;
  bool bVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  int iVar11;
  undefined8 *puVar12;
  undefined8 *puVar13;
  long *plVar14;
  undefined8 *puVar15;
  long lVar16;
  undefined8 *puVar17;
  undefined *puVar18;
  long lVar19;
  undefined8 uVar20;
  long lVar21;
  int *piVar22;
  long *plVar23;
  long lVar24;
  uint *puVar25;
  long *unaff_x22;
  long lVar26;
  undefined8 *puVar27;
  long in_xzr;
  uint *puStack_f0;
  uint *puStack_e0;
  uint *puStack_d0;
  long *plStack_c8;
  undefined8 uStack_98;
  undefined8 *puStack_90;
  undefined8 *puStack_88;
  char *pcStack_80;
  undefined8 uStack_78;
  long lStack_70;
  
  if (_UNK_013c4d88 == (long *)0x0) {
    uVar10 = 0;
  }
  else {
    uVar10 = 0;
    plVar14 = _UNK_013c4d88;
    do {
      uVar10 = *(int *)(plVar14[2] + 0x34) + uVar10;
      plVar14 = (long *)*plVar14;
    } while (plVar14 != (long *)0x0);
  }
  if (puRam000000000155c540 == (undefined8 *)0x0) {
    lStack_70 = *(long *)PTR____stack_chk_guard_013b4320;
    lVar21 = param_1[1];
    if ((*(byte *)(lVar21 + 0x20) >> 1 & 1) == 0) {
      if ((*(long *)(lVar21 + 0x48) != 0 && pcRam000000000155c570 != (code *)0x0) &&
         (iVar11 = (*pcRam000000000155c570)
                             (*(long *)(lVar21 + 0x48),*(undefined8 *)(lVar21 + 8),uVar10),
         iVar11 != 0 && pcRam000000000155c560 != (code *)0x0)) {
        (*pcRam000000000155c560)(*(undefined8 *)(lVar21 + 0x48),param_1,uVar10,0);
      }
      uStack_78 = 0;
      lVar19 = *(long *)(lVar21 + 0x40);
      if ((lVar19 != 0) && (*(long *)(lVar19 + 8) != 0)) {
        plVar14 = (long *)(*(long *)(lVar19 + 8) + 0x10);
        if (*plVar14 == 0) {
          if (*(long *)(lVar19 + 8) == 0) {
            if (0xfffffffe < uVar10) goto LAB_003d5964;
          }
          else if (*(uint *)(*(long *)(lVar19 + 8) + 8) <= uVar10) goto LAB_003d5964;
          if (plVar14[(long)(int)uVar10 * 2 + 2] == 0) goto LAB_003d5964;
        }
        puStack_d0 = *(uint **)(lVar21 + 0x40);
        if (puStack_d0 != (uint *)0x0) {
          LOAcquire();
          puStack_d0[1] = puStack_d0[1] + 1;
          LORelease();
        }
        puStack_e0 = puStack_d0 + 2;
        plVar14 = (long *)(*(long *)(puStack_d0 + 2) + 0x10);
        plVar23 = plVar14 + (long)(int)uVar10 * 2 + 2;
        if (*(int *)(*(long *)(puStack_d0 + 2) + 8) <= (int)uVar10) {
          plVar23 = plVar14;
        }
        lVar19 = func_0x002813dc();
        lVar16 = *(long *)(*(long *)(param_1[1] + 0x38) + 0x40);
        uVar9 = *puStack_d0;
        puVar17 = (undefined8 *)((ulong)&uStack_98 | 4);
        plVar1 = (long *)((ulong)((int)param_1 +
                                  (int)(((ulong)param_1 & 0xffffffff) / 0x418) * -0x418 & 0x7f8) +
                         0x1540a30);
        do {
          lVar26 = *plVar23;
          if (lVar26 != 0) {
            do {
              puVar27 = *(undefined8 **)(lVar26 + 0x28);
              if ((puVar27 != (undefined8 *)0x0) && (lVar24 = *(long *)(lVar26 + 0x30), lVar24 != 0)
                 ) {
                if (lVar19 == lVar16) {
                  lVar24 = *(long *)(lVar24 + 0x40);
                }
                else {
                  iVar11 = ((int)puVar27 + (int)(((ulong)puVar27 & 0xffffffff) / 0x418) * -0x418 &
                           0x7f8U) + 0x1540a30;
                  func_0x00281bec(iVar11);
                  lVar24 = *(long *)(lVar24 + 0x40);
                  func_0x00281b90(iVar11);
                }
                uVar8 = *(uint *)(lVar26 + 0x54);
                if (lVar19 == lVar24 || (uVar8 & 0x38000000) != 0) {
                  uVar5 = uVar8 >> 0x1b & 7;
                  if (uVar5 == 3) {
                    if (lVar19 == lVar24) {
                      uStack_98 = CONCAT44(uStack_98._4_4_,2);
                      *puVar17 = 0;
                      puVar17[1] = 0;
                      *(undefined4 *)(puVar17 + 2) = 0;
                      pcStack_80 = "default";
                      (**(code **)*param_1)(param_1);
                      func_0x003bbb68();
                      (**(code **)*puVar27)(puVar27);
                      func_0x003bbb68();
                      func_0x00c78434(&uStack_98,
                                      "Qt: Dead lock detected while activating a BlockingQueuedConnection: Sender is %s(%p), receiver is %s(%p)"
                                     );
                    }
                    func_0x002846b0(&uStack_98,0);
                    if (*plVar1 == 0) {
                      *plVar1 = 1;
                    }
                    else {
                      func_0x00281cac(plVar1);
                    }
                    if (*(long *)(lVar26 + 0x28) == 0) {
                      if (*plVar1 == 1) {
                        *plVar1 = in_xzr;
                      }
                      else {
                        func_0x0028232c(plVar1);
                      }
                    }
                    else {
                      if ((*(byte *)(lVar26 + 0x57) >> 6 & 1) == 0) {
                        puVar13 = (undefined8 *)__Znwm(0x70);
                        uVar2 = *(undefined2 *)(lVar26 + 0x50);
                        uVar3 = *(undefined2 *)(lVar26 + 0x52);
                        uVar20 = *(undefined8 *)(lVar26 + 0x38);
                        func_0x003bb49c(puVar13,0x2b);
                        *(uint *)((long)puVar13 + 0x14) = uVar10;
                        puVar13[3] = param_1;
                        puVar13[4] = &uStack_98;
                        puVar18 = &UNK_013d2610;
                        *puVar13 = &UNK_013d2610;
                        puVar13[5] = 0;
                        puVar13[6] = &uStack_78;
                        puVar13[7] = uVar20;
                        *(undefined4 *)(puVar13 + 8) = 0;
                        *(undefined2 *)((long)puVar13 + 0x44) = uVar2;
                        *(undefined2 *)((long)puVar13 + 0x46) = uVar3;
                        puVar13[10] = 0;
                        puVar13[9] = 0;
                        puVar13[0xc] = 0;
                        puVar13[0xb] = 0;
                        *(undefined4 *)(puVar13 + 0xd) = 0;
                      }
                      else {
                        puVar13 = (undefined8 *)__Znwm(0x70);
                        puVar25 = *(uint **)(lVar26 + 0x38);
                        func_0x003bb49c(puVar13,0x2b);
                        *(uint *)((long)puVar13 + 0x14) = uVar10;
                        puVar13[3] = param_1;
                        puVar13[4] = &uStack_98;
                        *puVar13 = &UNK_013d2610;
                        puVar13[5] = puVar25;
                        puVar13[6] = &uStack_78;
                        puVar18 = (undefined *)0xffff000000000000;
                        puVar13[7] = 0;
                        puVar13[8] = 0xffff000000000000;
                        puVar13[10] = 0;
                        puVar13[9] = 0;
                        puVar13[0xc] = 0;
                        puVar13[0xb] = 0;
                        *(undefined4 *)(puVar13 + 0xd) = 0;
                        if (puVar25 != (uint *)0x0) {
                          LOAcquire();
                          uVar8 = *puVar25;
                          *puVar25 = uVar8 + 1;
                          puVar18 = (undefined *)(ulong)uVar8;
                          LORelease();
                        }
                      }
                      func_0x003b73bc(puVar18,puVar27,puVar13,0);
                      if (*plVar1 == 1) {
                        *plVar1 = in_xzr;
                      }
                      else {
                        func_0x0028232c(plVar1);
                      }
                      func_0x00284750(&uStack_98,1);
                    }
                    func_0x00284714(&uStack_98);
                  }
                  else {
                    if (uVar5 == 2) goto LAB_003d5550;
                    puStack_90 = puVar27;
                    if (lVar19 != lVar24) {
                      puStack_90 = (undefined8 *)0x0;
                    }
                    pcStack_80 = (char *)CONCAT44(pcStack_80._4_4_,uVar10);
                    if (puStack_90 != (undefined8 *)0x0) {
                      uStack_98 = *(undefined8 *)(*(long *)(puStack_90[1] + 0x40) + 0x18);
                      *(undefined8 **)(*(long *)(puStack_90[1] + 0x40) + 0x18) = &uStack_98;
                      uVar8 = *(uint *)(lVar26 + 0x54);
                    }
                    if ((uVar8 >> 0x1e & 1) == 0) {
                      puStack_88 = param_1;
                      if (*(long *)(lVar26 + 0x38) != 0) {
                        uVar4 = *(ushort *)(lVar26 + 0x50);
                        (**(code **)*puVar27)(puVar27);
                        iVar11 = func_0x003bc080();
                        if ((int)(uint)uVar4 <= iVar11) {
                          (**(code **)(lVar26 + 0x38))
                                    (puVar27,0,*(undefined2 *)(lVar26 + 0x52),&uStack_78);
                          goto LAB_003d5840;
                        }
                      }
                      func_0x003bbf14(puVar27,0,
                                      (uint)*(ushort *)(lVar26 + 0x50) +
                                      (uint)*(ushort *)(lVar26 + 0x52),&uStack_78);
                    }
                    else {
                      LOAcquire();
                      **(int **)(lVar26 + 0x38) = **(int **)(lVar26 + 0x38) + 1;
                      LORelease();
                      piVar22 = *(int **)(lVar26 + 0x38);
                      puStack_88 = param_1;
                      (**(code **)(piVar22 + 2))(1,piVar22,puVar27,&uStack_78,0);
                      if (piVar22 != (int *)0x0) {
                        LOAcquire();
                        iVar11 = *piVar22;
                        *piVar22 = iVar11 + -1;
                        LORelease();
                        if (iVar11 == 1) {
                          (**(code **)(piVar22 + 2))(0,piVar22,0,0,0);
                        }
                      }
                    }
LAB_003d5840:
                    if (puStack_90 != (undefined8 *)0x0) {
                      *(undefined8 *)(*(long *)(puStack_90[1] + 0x40) + 0x18) = uStack_98;
                    }
                  }
                }
                else {
LAB_003d5550:
                  func_0x003d8bdc(param_1,uVar10,lVar26,&uStack_78);
                }
              }
              lVar26 = *(long *)(lVar26 + 0x10);
            } while ((lVar26 != 0) && (*(uint *)(lVar26 + 0x4c) <= uVar9));
          }
          bVar7 = plVar23 != plVar14;
          plVar23 = plVar14;
        } while (bVar7);
        uVar10 = *puStack_d0;
        if (puStack_d0 != (uint *)0x0) {
          LOAcquire();
          uVar9 = puStack_d0[1];
          puStack_d0[1] = uVar9 - 1;
          LORelease();
          if (uVar9 == 1) {
            func_0x003cd314(*(undefined8 *)(puStack_d0 + 8));
            if (*(long *)(puStack_d0 + 2) != 0) {
              _free();
            }
            __ZdlPv(puStack_d0);
          }
        }
        if (((uVar10 != 0) && (lVar21 = *(long *)(lVar21 + 0x40), *(long *)(lVar21 + 0x20) != 0)) &&
           (*(int *)(lVar21 + 4) == 1)) {
          if (*plVar1 == 0) {
            *plVar1 = 1;
          }
          else {
            func_0x00281cac(plVar1);
          }
          iVar11 = *(int *)(lVar21 + 4);
          if (iVar11 < 2) {
            uVar20 = *(undefined8 *)(lVar21 + 0x20);
            *(undefined8 *)(lVar21 + 0x20) = 0;
          }
          else {
            uVar20 = 0;
          }
          if (*plVar1 == 1) {
            *plVar1 = in_xzr;
          }
          else {
            func_0x0028232c(plVar1);
          }
          if (iVar11 < 2) {
            func_0x003cd314(uVar20);
          }
        }
      }
    }
LAB_003d5964:
    if (*(long *)PTR____stack_chk_guard_013b4320 == lStack_70) {
      return;
    }
    uVar20 = ___stack_chk_fail();
    if (puStack_90 != (undefined8 *)0x0) {
      *(undefined8 *)(*(long *)(puStack_90[1] + 0x40) + 0x18) = uStack_98;
    }
    if (puStack_d0 != (uint *)0x0) {
      LOAcquire();
      uVar10 = puStack_d0[1];
      puStack_d0[1] = uVar10 - 1;
      LORelease();
      if (uVar10 == 1) {
        func_0x00c804b8(puStack_d0,puStack_e0,puStack_d0);
      }
    }
    __Unwind_Resume(uVar20);
                    /* WARNING: Does not return */
    pcVar6 = (code *)SoftwareBreakpoint(1,0x3d5aa0);
    (*pcVar6)();
  }
  lStack_70 = *(long *)PTR____stack_chk_guard_013b4320;
  lVar21 = param_1[1];
  if ((*(byte *)(lVar21 + 0x20) >> 1 & 1) != 0) goto LAB_003d51cc;
  if ((*(long *)(lVar21 + 0x48) != 0 && pcRam000000000155c570 != (code *)0x0) &&
     (iVar11 = (*pcRam000000000155c570)(*(long *)(lVar21 + 0x48),*(undefined8 *)(lVar21 + 8),uVar10)
     , iVar11 != 0 && pcRam000000000155c560 != (code *)0x0)) {
    (*pcRam000000000155c560)(*(undefined8 *)(lVar21 + 0x48),param_1,uVar10,0);
  }
  puVar17 = puRam000000000155c540;
  uStack_78 = 0;
  lVar19 = *(long *)(lVar21 + 0x40);
  if ((lVar19 == 0) || (*(long *)(lVar19 + 8) == 0)) goto LAB_003d51a0;
  plVar14 = (long *)(*(long *)(lVar19 + 8) + 0x10);
  if (*plVar14 == 0) {
    if (*(long *)(lVar19 + 8) == 0) {
      if (uVar10 == 0xffffffff) goto LAB_003d51a0;
LAB_003d5178:
      pcVar6 = (code *)*puRam000000000155c540;
      if (plVar14[(long)(int)uVar10 * 2 + 2] != 0) goto joined_r0x003d518c;
    }
    else {
      if (uVar10 < *(uint *)(*(long *)(lVar19 + 8) + 8)) goto LAB_003d5178;
LAB_003d51a0:
      pcVar6 = (code *)*puRam000000000155c540;
    }
    if (pcVar6 != (code *)0x0) {
      (*pcVar6)(param_1,uVar10,&uStack_78);
    }
  }
  else {
    pcVar6 = (code *)*puRam000000000155c540;
joined_r0x003d518c:
    if (pcVar6 != (code *)0x0) {
      (*pcVar6)(param_1,uVar10,&uStack_78);
    }
    puStack_e0 = *(uint **)(lVar21 + 0x40);
    if (puStack_e0 != (uint *)0x0) {
      LOAcquire();
      puStack_e0[1] = puStack_e0[1] + 1;
      LORelease();
    }
    puStack_f0 = puStack_e0 + 2;
    unaff_x22 = (long *)(*(long *)(puStack_e0 + 2) + 0x10);
    plVar14 = unaff_x22 + (long)(int)uVar10 * 2 + 2;
    if (*(int *)(*(long *)(puStack_e0 + 2) + 8) <= (int)uVar10) {
      plVar14 = unaff_x22;
    }
    lVar19 = func_0x002813dc();
    lVar16 = *(long *)(*(long *)(param_1[1] + 0x38) + 0x40);
    uVar9 = *puStack_e0;
    puVar27 = (undefined8 *)((ulong)&uStack_98 | 4);
    plStack_c8 = (long *)((ulong)((int)param_1 +
                                  (int)(((ulong)param_1 & 0xffffffff) / 0x418) * -0x418 & 0x7f8) +
                         0x1540a30);
    do {
      lVar26 = *plVar14;
      if (lVar26 != 0) {
        do {
          puVar13 = *(undefined8 **)(lVar26 + 0x28);
          if ((puVar13 != (undefined8 *)0x0) && (lVar24 = *(long *)(lVar26 + 0x30), lVar24 != 0)) {
            if (lVar19 == lVar16) {
              lVar24 = *(long *)(lVar24 + 0x40);
            }
            else {
              iVar11 = ((int)puVar13 + (int)(((ulong)puVar13 & 0xffffffff) / 0x418) * -0x418 &
                       0x7f8U) + 0x1540a30;
              func_0x00281bec(iVar11);
              lVar24 = *(long *)(lVar24 + 0x40);
              func_0x00281b90(iVar11);
            }
            uVar8 = *(uint *)(lVar26 + 0x54);
            if (lVar19 == lVar24 || (uVar8 & 0x38000000) != 0) {
              uVar5 = uVar8 >> 0x1b & 7;
              if (uVar5 == 3) {
                if (lVar19 == lVar24) {
                  uStack_98 = CONCAT44(uStack_98._4_4_,2);
                  *puVar27 = 0;
                  puVar27[1] = 0;
                  *(undefined4 *)(puVar27 + 2) = 0;
                  pcStack_80 = "default";
                  (**(code **)*param_1)(param_1);
                  func_0x003bbb68();
                  (**(code **)*puVar13)(puVar13);
                  func_0x003bbb68();
                  func_0x00c78434(&uStack_98,
                                  "Qt: Dead lock detected while activating a BlockingQueuedConnection: Sender is %s(%p), receiver is %s(%p)"
                                 );
                }
                func_0x002846b0(&uStack_98,0);
                if (*plStack_c8 == 0) {
                  *plStack_c8 = 1;
                }
                else {
                  func_0x00281cac(plStack_c8);
                }
                if (*(long *)(lVar26 + 0x28) == 0) {
                  if (*plStack_c8 == 1) {
                    *plStack_c8 = in_xzr;
                  }
                  else {
                    func_0x0028232c(plStack_c8);
                  }
                }
                else {
                  if ((*(byte *)(lVar26 + 0x57) >> 6 & 1) == 0) {
                    puVar12 = (undefined8 *)__Znwm(0x70);
                    uVar2 = *(undefined2 *)(lVar26 + 0x50);
                    uVar3 = *(undefined2 *)(lVar26 + 0x52);
                    uVar20 = *(undefined8 *)(lVar26 + 0x38);
                    func_0x003bb49c(puVar12,0x2b);
                    *(uint *)((long)puVar12 + 0x14) = uVar10;
                    puVar12[3] = param_1;
                    puVar12[4] = &uStack_98;
                    *puVar12 = &UNK_013d2610;
                    puVar12[5] = 0;
                    puVar12[6] = &uStack_78;
                    puVar12[7] = uVar20;
                    *(undefined4 *)(puVar12 + 8) = 0;
                    *(undefined2 *)((long)puVar12 + 0x44) = uVar2;
                    *(undefined2 *)((long)puVar12 + 0x46) = uVar3;
                    puVar12[10] = 0;
                    puVar12[9] = 0;
                    puVar12[0xc] = 0;
                    puVar12[0xb] = 0;
                    *(undefined4 *)(puVar12 + 0xd) = 0;
                    puVar15 = &uStack_78;
                  }
                  else {
                    puVar12 = (undefined8 *)__Znwm(0x70);
                    puVar25 = *(uint **)(lVar26 + 0x38);
                    func_0x003bb49c(puVar12,0x2b);
                    *(uint *)((long)puVar12 + 0x14) = uVar10;
                    puVar12[3] = param_1;
                    puVar12[4] = &uStack_98;
                    *puVar12 = &UNK_013d2610;
                    puVar12[5] = puVar25;
                    puVar12[6] = &uStack_78;
                    puVar15 = (undefined8 *)0xffff000000000000;
                    puVar12[7] = 0;
                    puVar12[8] = 0xffff000000000000;
                    puVar12[10] = 0;
                    puVar12[9] = 0;
                    puVar12[0xc] = 0;
                    puVar12[0xb] = 0;
                    *(undefined4 *)(puVar12 + 0xd) = 0;
                    if (puVar25 != (uint *)0x0) {
                      LOAcquire();
                      uVar8 = *puVar25;
                      *puVar25 = uVar8 + 1;
                      puVar15 = (undefined8 *)(ulong)uVar8;
                      LORelease();
                    }
                  }
                  func_0x003b73bc(puVar15,puVar13,puVar12,0);
                  if (*plStack_c8 == 1) {
                    *plStack_c8 = in_xzr;
                  }
                  else {
                    func_0x0028232c(plStack_c8);
                  }
                  func_0x00284750(&uStack_98,1);
                }
                func_0x00284714(&uStack_98);
              }
              else {
                if (uVar5 == 2) goto LAB_003d4d54;
                puStack_90 = puVar13;
                if (lVar19 != lVar24) {
                  puStack_90 = (undefined8 *)0x0;
                }
                pcStack_80 = (char *)CONCAT44(pcStack_80._4_4_,uVar10);
                if (puStack_90 != (undefined8 *)0x0) {
                  uStack_98 = *(undefined8 *)(*(long *)(puStack_90[1] + 0x40) + 0x18);
                  *(undefined8 **)(*(long *)(puStack_90[1] + 0x40) + 0x18) = &uStack_98;
                  uVar8 = *(uint *)(lVar26 + 0x54);
                }
                if ((uVar8 >> 0x1e & 1) == 0) {
                  puStack_88 = param_1;
                  if (*(long *)(lVar26 + 0x38) != 0) {
                    uVar4 = *(ushort *)(lVar26 + 0x50);
                    (**(code **)*puVar13)(puVar13);
                    iVar11 = func_0x003bc080();
                    if ((int)(uint)uVar4 <= iVar11) {
                      uVar4 = *(ushort *)(lVar26 + 0x52);
                      pcVar6 = *(code **)(lVar26 + 0x38);
                      iVar11 = (uint)*(ushort *)(lVar26 + 0x50) + (uint)uVar4;
                      if ((code *)puVar17[1] != (code *)0x0) {
                        (*(code *)puVar17[1])(puVar13,iVar11,&uStack_78);
                      }
                      (*pcVar6)(puVar13,0,uVar4,&uStack_78);
                      if ((code *)puVar17[3] != (code *)0x0) {
                        (*(code *)puVar17[3])(puVar13,iVar11);
                      }
                      goto LAB_003d50a8;
                    }
                  }
                  iVar11 = (uint)*(ushort *)(lVar26 + 0x50) + (uint)*(ushort *)(lVar26 + 0x52);
                  if ((code *)puVar17[1] != (code *)0x0) {
                    (*(code *)puVar17[1])(puVar13,iVar11,&uStack_78);
                  }
                  func_0x003bbf14(puVar13,0,iVar11,&uStack_78);
                  if ((code *)puVar17[3] != (code *)0x0) {
                    (*(code *)puVar17[3])(puVar13,iVar11);
                  }
                }
                else {
                  LOAcquire();
                  **(int **)(lVar26 + 0x38) = **(int **)(lVar26 + 0x38) + 1;
                  LORelease();
                  piVar22 = *(int **)(lVar26 + 0x38);
                  puStack_88 = param_1;
                  (**(code **)(piVar22 + 2))(1,piVar22,puVar13,&uStack_78,0);
                  if (piVar22 != (int *)0x0) {
                    LOAcquire();
                    iVar11 = *piVar22;
                    *piVar22 = iVar11 + -1;
                    LORelease();
                    if (iVar11 == 1) {
                      (**(code **)(piVar22 + 2))(0,piVar22,0,0,0);
                    }
                  }
                }
LAB_003d50a8:
                if (puStack_90 != (undefined8 *)0x0) {
                  *(undefined8 *)(*(long *)(puStack_90[1] + 0x40) + 0x18) = uStack_98;
                }
              }
            }
            else {
LAB_003d4d54:
              func_0x003d8bdc(param_1,uVar10,lVar26,&uStack_78);
            }
          }
          lVar26 = *(long *)(lVar26 + 0x10);
        } while ((lVar26 != 0) && (*(uint *)(lVar26 + 0x4c) <= uVar9));
      }
      bVar7 = plVar14 != unaff_x22;
      plVar14 = unaff_x22;
    } while (bVar7);
    uVar9 = *puStack_e0;
    if (puStack_e0 != (uint *)0x0) {
      LOAcquire();
      uVar8 = puStack_e0[1];
      puStack_e0[1] = uVar8 - 1;
      LORelease();
      if (uVar8 == 1) {
        func_0x003cd314(*(undefined8 *)(puStack_e0 + 8));
        if (*(long *)(puStack_e0 + 2) != 0) {
          _free();
        }
        __ZdlPv(puStack_e0);
      }
    }
    if (uVar9 == 0) goto LAB_003d51cc;
    lVar21 = *(long *)(lVar21 + 0x40);
    if ((*(long *)(lVar21 + 0x20) != 0) && (*(int *)(lVar21 + 4) == 1)) {
      if (*plStack_c8 == 0) {
        *plStack_c8 = 1;
      }
      else {
        func_0x00281cac();
      }
      iVar11 = *(int *)(lVar21 + 4);
      if (iVar11 < 2) {
        uVar20 = *(undefined8 *)(lVar21 + 0x20);
        *(undefined8 *)(lVar21 + 0x20) = 0;
      }
      else {
        uVar20 = 0;
      }
      if (*plStack_c8 == 1) {
        *plStack_c8 = in_xzr;
      }
      else {
        func_0x0028232c();
      }
      if (iVar11 < 2) {
        func_0x003cd314(uVar20);
      }
    }
  }
  if ((code *)puVar17[2] != (code *)0x0) {
    (*(code *)puVar17[2])(param_1,uVar10);
  }
LAB_003d51cc:
  if (*(long *)PTR____stack_chk_guard_013b4320 == lStack_70) {
    return;
  }
  ___stack_chk_fail();
  uVar20 = func_0x00006028();
  __ZdlPv(unaff_x22);
  if (*plStack_c8 == 1) {
    *plStack_c8 = in_xzr;
  }
  else {
    func_0x0028232c(plStack_c8);
  }
  func_0x00284714(&uStack_98);
  if (puStack_e0 != (uint *)0x0) {
    LOAcquire();
    uVar10 = puStack_e0[1];
    puStack_e0[1] = uVar10 - 1;
    LORelease();
    if (uVar10 == 1) {
      func_0x00c8048c(puStack_e0,puStack_f0,puStack_e0);
    }
  }
  __Unwind_Resume(uVar20);
                    /* WARNING: Does not return */
  pcVar6 = (code *)SoftwareBreakpoint(1,0x3d5344);
  (*pcVar6)();
}


