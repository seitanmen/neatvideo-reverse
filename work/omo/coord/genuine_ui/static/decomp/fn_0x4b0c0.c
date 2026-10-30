// FUN_0004b0c0 size=3468

/* WARNING: Possible PIC construction at 0x0004b120: Changing call to branch */
/* WARNING: Possible PIC construction at 0x00925638: Changing call to branch */
/* WARNING: Removing unreachable block (ram,0x0004b124) */
/* WARNING: Removing unreachable block (ram,0x0092563c) */

void FUN_0004b0c0(undefined1 param_1 [16],undefined1 param_2 [16],double param_3,long *param_4,
                 long *param_5,long *param_6)

{
  ulong uVar1;
  undefined1 *puVar2;
  undefined1 *puVar3;
  bool bVar5;
  uint uVar6;
  int iVar7;
  int *piVar8;
  long *plVar9;
  long lVar10;
  long *plVar11;
  undefined8 uVar12;
  bool bVar13;
  ulong uVar14;
  long *plVar15;
  int iVar16;
  uint *puVar17;
  ulong uVar18;
  uint uVar19;
  long lVar20;
  long lVar21;
  undefined8 *puVar22;
  int *unaff_x19;
  ulong unaff_x23;
  undefined8 unaff_x24;
  int *piVar23;
  ulong *unaff_x25;
  ulong unaff_x26;
  long *unaff_x27;
  undefined8 unaff_x28;
  undefined8 uVar24;
  undefined1 auStack_90 [8];
  int *piStack_88;
  int *piStack_80;
  int *piStack_78;
  int *piStack_70;
  uint *puStack_68;
  uint *puStack_60;
  uint *puStack_58;
  undefined1 *puVar4;
  
  iVar7 = *(int *)(*param_5 + 8);
  iVar16 = *(int *)(*param_5 + 0xc);
  if ((iVar16 == iVar7) && (*(int *)(*param_6 + 0xc) == *(int *)(*param_6 + 8))) {
    func_0x00046d2c(param_4[0x43],0);
    plVar11 = (long *)0x0;
    uVar24 = 0x4b124;
    plVar15 = (long *)(*(ulong *)(*(int **)(param_4[0x59] + 0x18) + 0x4e) >> 0x2e & 1);
    bVar13 = true;
    uVar14 = 1;
    puVar2 = auStack_90;
    piVar23 = *(int **)(param_4[0x59] + 0x18);
    puVar3 = (undefined1 *)register0x00000008;
    do {
      piVar8 = piVar23;
      puVar4 = puVar2;
      *(undefined8 *)(puVar4 + -0x60) = unaff_x28;
      *(long **)(puVar4 + -0x58) = unaff_x27;
      *(ulong *)(puVar4 + -0x50) = unaff_x26;
      *(ulong **)(puVar4 + -0x48) = unaff_x25;
      *(undefined8 *)(puVar4 + -0x40) = unaff_x24;
      *(ulong *)(puVar4 + -0x38) = unaff_x23;
      *(long **)(puVar4 + -0x30) = param_5;
      *(long **)(puVar4 + -0x28) = param_6;
      *(long **)(puVar4 + -0x20) = param_4;
      *(int **)(puVar4 + -0x18) = unaff_x19;
      *(undefined1 **)(puVar4 + -0x10) = puVar3 + -0x10;
      *(undefined8 *)(puVar4 + -8) = uVar24;
      param_4 = *(long **)(piVar8 + 0x52);
      unaff_x25 = (ulong *)(piVar8 + 0x4e);
      uVar18 = *(ulong *)(piVar8 + 0x4e);
      uVar6 = (uint)plVar11;
      if (bVar13) {
        uVar1 = 0;
        if (uVar6 == 0) {
          uVar1 = 0x40;
        }
        uVar18 = uVar18 & 0xffffffffffffffbf | uVar1;
        piVar8[0x50] = piVar8[0x50];
        *unaff_x25 = uVar18;
      }
      if (((uint)(uVar18 >> 5) & 1) == uVar6) {
        return;
      }
      if ((*(long *)(piVar8 + 0x20) != 0 && uVar6 != 0) &&
         (((uint)*(undefined8 *)(*(long *)(*(long *)(piVar8 + 0x20) + 8) + 0x138) >> 5 & 1) == 0)) {
        return;
      }
      func_0x003df028(puVar4 + -0xa0);
      (**(code **)(*param_4 + 0x108))(puVar4 + -0x78,param_4,2,puVar4 + -0xa0);
      func_0x003dec3c(puVar4 + -0xa0);
      plVar11 = (long *)func_0x003e1eb4(puVar4 + -0x78);
      uVar6 = (uint)plVar11;
      if ((uVar6 ^ (uint)(*unaff_x25 >> 5) & 1) != 1) goto LAB_009258e4;
      uVar18 = 0x20;
      if (uVar6 == 0) {
        uVar18 = 0;
      }
      piVar8[0x50] = piVar8[0x50];
      *unaff_x25 = *unaff_x25 & 0xffffffffffffffdf | uVar18;
      if ((int)uVar14 != 0) {
        lVar20 = *(long *)(piVar8 + 2);
        uVar18 = (ulong)*(uint *)(lVar20 + 4);
        if (0 < (int)*(uint *)(lVar20 + 4)) {
          lVar20 = *(long *)(lVar20 + 0x10) + lVar20 + 8;
          do {
            if (*(int *)(lVar20 + -8) == 2) {
              func_0x003decf4(puVar4 + -0xa0);
              goto LAB_0092513c;
            }
            lVar20 = lVar20 + 0x18;
            uVar18 = uVar18 - 1;
          } while (uVar18 != 0);
        }
        *(undefined4 *)(puVar4 + -0x98) = 0x80000000;
        *(undefined8 *)(puVar4 + -0xa0) = 0;
LAB_0092513c:
        iVar7 = func_0x003df6a8(puVar4 + -0xa0);
        if (iVar7 == 0x1f) {
          plVar9 = (long *)func_0x003dece0(puVar4 + -0xa0);
          lVar20 = *plVar9;
        }
        else {
          iVar7 = func_0x003e2fc0(puVar4 + -0xa0,0x1f,puVar4 + -0x68);
          lVar20 = *(long *)(puVar4 + -0x68);
          if (iVar7 == 0) {
            lVar20 = 0;
          }
        }
        func_0x003dec3c(puVar4 + -0xa0);
        if (lVar20 != 0) {
          func_0x009266c0(lVar20);
        }
        piVar23 = piVar8;
        if (*(long *)(piVar8 + 0x1e) != 0) {
          do {
            if ((*(long *)(piVar23 + 0x26) != 0) &&
               (uVar19 = piVar23[0x50], piVar23[0x50] = uVar19 | 1, (uVar19 >> 6 & 1) == 0)) {
              func_0x0097af5c(*(undefined8 *)
                               (*(long *)(*(long *)(*(long *)(piVar23 + 0x26) + 8) + 0x58) + 8),2);
            }
          } while ((*(long *)(piVar23 + 0x20) != 0) &&
                  (piVar23 = *(int **)(*(long *)(piVar23 + 0x20) + 8), piVar23 != (int *)0x0));
          uVar24 = *(undefined8 *)(*(long *)(piVar8 + 0x1e) + 8);
          uVar12 = *(undefined8 *)(piVar8 + 0x52);
          *(undefined8 *)(puVar4 + -0x98) = 0;
          *(undefined8 *)(puVar4 + -0xa0) = 0;
          *(undefined8 *)(puVar4 + -0x88) = 0;
          *(undefined8 *)(puVar4 + -0x90) = 0;
          func_0x0093c6f8(uVar24,uVar12,puVar4 + -0xa0,0,1,0,0,0);
        }
      }
      lVar20 = *(long *)(piVar8 + 0x52);
      do {
        lVar10 = lVar20;
        if ((*(long *)(*(long *)(lVar10 + 8) + 0x78) == 0) ||
           (iVar7 = func_0x0093e190(), iVar7 == 0)) goto LAB_00925240;
        lVar21 = *(long *)(lVar10 + 8);
        lVar20 = *(long *)(lVar21 + 0x108);
      } while (*(long *)(lVar21 + 0x108) != 0);
      lVar20 = *(long *)(*(long *)(lVar21 + 0x78) + 8);
      if (*(long *)(lVar20 + 0x130) == lVar10) {
        while (((*(ulong *)(lVar21 + 0x138) >> 0x2e & 1) == 0 &&
               (lVar10 = *(long *)(lVar21 + 0x80), lVar10 != 0))) {
          lVar21 = *(long *)(lVar10 + 8);
        }
        unaff_x26 = (ulong)(lVar10 == *(long *)(lVar20 + 0x150));
        if (((ulong)plVar11 & 1) == 0) goto LAB_009252e4;
LAB_00925248:
        uVar18 = *unaff_x25;
        *unaff_x25 = uVar18 | 0x50000000000000;
        if (*(long *)(piVar8 + 0x1e) != 0) {
          if (((uint)uVar18 >> 0x18 & 1) != 0) {
            lVar20 = 0;
            if (*(long *)(piVar8 + 0x52) != 0) {
              lVar20 = *(long *)(piVar8 + 0x52) + -0x10;
            }
            iVar7 = func_0x0096957c(lVar20);
            if (iVar7 == 9) {
              func_0x0093e6e8(*(undefined8 *)(*(long *)(piVar8 + 0x1e) + 8),lVar20);
            }
          }
          if (((*(ulong *)(param_4[1] + 0x138) >> 0x2e & 1) != 0) && (piVar8[0x4b] != 0)) {
            func_0x00941fd0(*(undefined8 *)(*(long *)(piVar8 + 0x1e) + 8),
                            *(undefined8 *)(piVar8 + 0x52),0);
          }
        }
      }
      else {
LAB_00925240:
        unaff_x26 = 0;
        if (((ulong)plVar11 & 1) != 0) goto LAB_00925248;
LAB_009252e4:
        if (*(long *)(piVar8 + 0x1e) != 0) {
          lVar20 = *(long *)(*(long *)(*(long *)(piVar8 + 0x1e) + 8) + 0x178);
          iVar7 = *(int *)(lVar20 + 8);
          iVar16 = *(int *)(lVar20 + 0xc);
          puVar22 = (undefined8 *)(lVar20 + 0x10 + (long)iVar7 * 8);
          if (iVar7 == iVar16) {
LAB_00925308:
            if (puVar22 != (undefined8 *)(lVar20 + 0x10 + (long)iVar16 * 8)) {
              if (*(long *)(param_4[1] + 0x78) == 0) {
                func_0x00c8c558();
              }
              else {
                func_0x0093d280(*(undefined8 *)(*(long *)(param_4[1] + 0x78) + 8),param_4,0);
              }
            }
          }
          else {
            lVar10 = (long)iVar16 * 8 + (long)iVar7 * -8;
            do {
              if ((long *)*puVar22 == param_4) goto LAB_00925308;
              puVar22 = puVar22 + 1;
              lVar10 = lVar10 + -8;
            } while (lVar10 != 0);
          }
          lVar20 = *(long *)(*(long *)(*(long *)(piVar8 + 0x1e) + 8) + 0x180);
          iVar7 = *(int *)(lVar20 + 8);
          iVar16 = *(int *)(lVar20 + 0xc);
          puVar22 = (undefined8 *)(lVar20 + 0x10 + (long)iVar7 * 8);
          if (iVar7 == iVar16) {
LAB_00925374:
            if (puVar22 != (undefined8 *)(lVar20 + 0x10 + (long)iVar16 * 8)) {
              if (*(long *)(param_4[1] + 0x78) == 0) {
                func_0x00c8c528();
              }
              else {
                func_0x0093d54c(*(undefined8 *)(*(long *)(param_4[1] + 0x78) + 8),param_4,0);
              }
            }
          }
          else {
            lVar10 = (long)iVar16 * 8 + (long)iVar7 * -8;
            do {
              if ((long *)*puVar22 == param_4) goto LAB_00925374;
              puVar22 = puVar22 + 1;
              lVar10 = lVar10 + -8;
            } while (lVar10 != 0);
          }
          if (((*(ulong *)(param_4[1] + 0x138) >> 0x2e & 1) != 0) && (piVar8[0x4b] != 0)) {
            func_0x0093d040(*(undefined8 *)(*(long *)(piVar8 + 0x1e) + 8),
                            *(undefined8 *)(piVar8 + 0x52));
          }
        }
        if (((int)unaff_x26 != 0) && (*(long *)(piVar8 + 0x1e) != 0)) {
          lVar20 = func_0x0093e1a4();
          if ((((uint)*unaff_x25 >> 0x18 & 1) != 0) &&
             (lVar10 = *(long *)(lVar20 + 8), (*(ulong *)(lVar10 + 0x138) >> 0x2e & 1) == 0)) {
            lVar21 = *(long *)(piVar8 + 0x52);
            while (lVar20 != lVar21) {
              do {
                lVar20 = *(long *)(lVar10 + 0x80);
                if (lVar20 == 0) goto LAB_00925460;
                lVar10 = *(long *)(lVar20 + 8);
              } while (((uint)*(ulong *)(lVar10 + 0x138) >> 0x18 & 1) == 0);
              if ((*(ulong *)(lVar10 + 0x138) >> 0x2e & 1) != 0) goto LAB_00925460;
            }
            plVar9 = (long *)0x0;
            if (lVar21 != 0) {
              plVar9 = (long *)(lVar21 + -0x10);
            }
            uVar18 = (**(code **)(*plVar9 + 0xf0))(plVar9,1);
            if ((uVar18 & 1) != 0) goto LAB_00925470;
          }
LAB_00925460:
          func_0x0092951c(piVar8,0,plVar15);
        }
LAB_00925470:
        lVar20 = (*(long **)(piVar8 + 0x52))[1];
        uVar19 = (uint)*(undefined8 *)(lVar20 + 0x138);
        unaff_x27 = *(long **)(piVar8 + 0x52);
        while ((uVar19 >> 0xc & 1) != 0) {
          do {
            plVar9 = *(long **)(lVar20 + 0x80);
            if (plVar9 == (long *)0x0) goto LAB_00925484;
            iVar7 = (**(code **)(*plVar9 + 0x58))(plVar9);
            lVar20 = plVar9[1];
          } while (iVar7 != 10);
          unaff_x27 = plVar9;
          uVar19 = (uint)*(undefined8 *)(lVar20 + 0x138);
        }
LAB_00925484:
        if (((uint)*(undefined8 *)(unaff_x27[1] + 0x138) >> 9 & 1) != 0) {
          func_0x0092830c(*(undefined8 *)(piVar8 + 0x52),0);
        }
      }
      if ((int)uVar14 != 0) {
        uVar14 = (ulong)((uint)((*unaff_x25 & 0x8001000000000) == 0) |
                        (uint)(*unaff_x25 >> 0x2a) & 1);
      }
      piVar23 = *(int **)(piVar8 + 0x22);
      *(int **)(puVar4 + -0xa0) = piVar23;
      if (*piVar23 != -1) {
        if (*piVar23 == 0) {
          func_0x002aa738(puVar4 + -0xa0,piVar23[1]);
          lVar21 = *(long *)(puVar4 + -0xa0);
          lVar20 = lVar21 + (long)*(int *)(lVar21 + 8) * 8;
          lVar10 = *(long *)(piVar8 + 0x22) + (long)*(int *)(*(long *)(piVar8 + 0x22) + 8) * 8;
          if ((lVar10 != lVar20) &&
             (lVar20 = lVar20 + 0x10,
             0 < ((lVar21 + (long)*(int *)(lVar21 + 0xc) * 8) - lVar20) + 0x10)) {
            _memcpy(lVar20,lVar10 + 0x10);
          }
        }
        else {
          LOAcquire();
          *piVar23 = *piVar23 + 1;
          LORelease();
        }
      }
      lVar20 = *(long *)(puVar4 + -0xa0);
      iVar7 = *(int *)(lVar20 + 8);
      plVar9 = (long *)(lVar20 + 0x10 + (long)iVar7 * 8);
      *(long **)(puVar4 + -0x98) = plVar9;
      iVar16 = *(int *)(lVar20 + 0xc);
      *(long *)(puVar4 + -0x90) = lVar20 + 0x10 + (long)iVar16 * 8;
      unaff_x24 = 1;
      *(undefined4 *)(puVar4 + -0x88) = 1;
      if (iVar7 == iVar16) goto LAB_00925594;
      while ((((ulong)plVar11 & 1) != 0 &&
             (((uint)*(undefined8 *)(*(int **)(*plVar9 + 8) + 0x4e) >> 6 & 1) != 0))) {
        plVar9 = plVar9 + 1;
        *(long **)(puVar4 + -0x98) = plVar9;
        *(undefined4 *)(puVar4 + -0x88) = 1;
        if (plVar9 == *(long **)(puVar4 + -0x90)) goto LAB_00925594;
      }
      bVar13 = false;
      uVar24 = 0x92563c;
      puVar2 = puVar4 + -0xb0;
      piVar23 = *(int **)(*plVar9 + 8);
      unaff_x19 = piVar8;
      param_6 = plVar11;
      param_5 = plVar15;
      unaff_x23 = uVar14;
      puVar3 = puVar4;
    } while( true );
  }
  lVar20 = param_4[0x43];
  piVar23 = (int *)0x142f030;
  if (iVar16 == iVar7) {
    bVar13 = false;
    puStack_68 = (uint *)0x142f030;
    piStack_78 = (int *)0x142f030;
  }
  else {
    unaff_x19 = (int *)func_0x002efa1c(", ",2);
    func_0x002faa10(&piStack_70,param_5,(long)unaff_x19 + *(long *)(unaff_x19 + 4),unaff_x19[1]);
    func_0x002efbd0(&puStack_68,"Not Uniform: ",0xd);
    func_0x002e97cc(&puStack_68,&piStack_70);
    if ((*(int *)(*param_5 + 0xc) == *(int *)(*param_5 + 8)) ||
       (*(int *)(*param_6 + 0xc) == *(int *)(*param_6 + 8))) {
      bVar13 = true;
      piStack_78 = (int *)0x142f030;
    }
    else {
      piStack_78 = (int *)func_0x002efa1c("\n",1);
      bVar13 = true;
    }
  }
  puStack_60 = puStack_68;
  puVar17 = puStack_68;
  if (1 < *puStack_68 + 1) {
    LOAcquire();
    uVar6 = *puStack_68;
    *puStack_68 = uVar6 + 1;
    puVar17 = (uint *)(ulong)uVar6;
    LORelease();
  }
  func_0x002e97cc(puVar17,&puStack_60,&piStack_78);
  bVar5 = *(int *)(*param_6 + 0xc) != *(int *)(*param_6 + 8);
  if (bVar5) {
    piVar23 = (int *)func_0x002efa1c(", ",2);
    func_0x002faa10(&piStack_88,param_6,(long)piVar23 + *(long *)(piVar23 + 4),piVar23[1]);
    func_0x002efbd0(&piStack_80,"Clipping: ",10);
    func_0x002e97cc(&piStack_80,&piStack_88);
  }
  else {
    piStack_80 = (int *)0x142f030;
  }
  puStack_58 = puStack_60;
  puVar17 = puStack_60;
  if (1 < *puStack_60 + 1) {
    LOAcquire();
    uVar6 = *puStack_60;
    *puStack_60 = uVar6 + 1;
    puVar17 = (uint *)(ulong)uVar6;
    LORelease();
  }
  func_0x002e97cc(puVar17,&puStack_58,&piStack_80);
  (**(code **)(*(long *)(param_4[0x59] + 0x10) + 0x18))();
  func_0x00046d70(lVar20,&puStack_58,(int)(param_3 + 10.0));
  if (*puStack_58 != 0xffffffff) {
    if (*puStack_58 != 0) {
      LOAcquire();
      uVar6 = *puStack_58;
      *puStack_58 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_0004b320;
    }
    func_0x00287df8(puStack_58,2,8);
  }
LAB_0004b320:
  if (*piStack_80 != -1) {
    if (*piStack_80 != 0) {
      LOAcquire();
      iVar7 = *piStack_80;
      *piStack_80 = iVar7 + -1;
      LORelease();
      if (iVar7 != 1) goto LAB_0004b354;
    }
    func_0x00287df8(piStack_80,2,8);
  }
LAB_0004b354:
  if (bVar5) {
    if (*piStack_88 != -1) {
      if (*piStack_88 != 0) {
        LOAcquire();
        iVar7 = *piStack_88;
        *piStack_88 = iVar7 + -1;
        LORelease();
        if (iVar7 != 1) goto LAB_0004b38c;
      }
      func_0x00287df8(piStack_88,2,8);
    }
LAB_0004b38c:
    if (*piVar23 != -1) {
      if (*piVar23 != 0) {
        LOAcquire();
        iVar7 = *piVar23;
        *piVar23 = iVar7 + -1;
        LORelease();
        if (iVar7 != 1) goto LAB_0004b3bc;
      }
      func_0x00287df8(piVar23,2,8);
    }
  }
LAB_0004b3bc:
  if (*puStack_60 != 0xffffffff) {
    if (*puStack_60 != 0) {
      LOAcquire();
      uVar6 = *puStack_60;
      *puStack_60 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_0004b3f0;
    }
    func_0x00287df8(puStack_60,2,8);
  }
LAB_0004b3f0:
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar7 = *piStack_78;
      *piStack_78 = iVar7 + -1;
      LORelease();
      if (iVar7 != 1) goto LAB_0004b424;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_0004b424:
  if (*puStack_68 != 0xffffffff) {
    if (*puStack_68 != 0) {
      LOAcquire();
      uVar6 = *puStack_68;
      *puStack_68 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_0004b458;
    }
    func_0x00287df8(puStack_68,2,8);
  }
LAB_0004b458:
  if (!bVar13) goto LAB_0004b4c0;
  if (*piStack_70 != -1) {
    if (*piStack_70 != 0) {
      LOAcquire();
      iVar7 = *piStack_70;
      *piStack_70 = iVar7 + -1;
      LORelease();
      if (iVar7 != 1) goto LAB_0004b490;
    }
    func_0x00287df8(piStack_70,2,8);
  }
LAB_0004b490:
  if (*unaff_x19 != -1) {
    if (*unaff_x19 != 0) {
      LOAcquire();
      iVar7 = *unaff_x19;
      *unaff_x19 = iVar7 + -1;
      LORelease();
      if (iVar7 != 1) goto LAB_0004b4c0;
    }
    func_0x00287df8(unaff_x19,2,8);
  }
LAB_0004b4c0:
  func_0x00046d2c(param_4[0x43],1);
  *(bool *)(param_4 + 0x5b) = *(int *)(*param_5 + 0xc) != *(int *)(*param_5 + 8);
  *(bool *)((long)param_4 + 0x2d9) = *(int *)(*param_6 + 0xc) != *(int *)(*param_6 + 8);
  (**(code **)(*param_4 + 0x1e8))(param_4);
  return;
LAB_00925594:
  piVar23 = *(int **)(puVar4 + -0xa0);
  if (*piVar23 != -1) {
    if (*piVar23 != 0) {
      LOAcquire();
      iVar7 = *piVar23;
      *piVar23 = iVar7 + -1;
      LORelease();
      if (iVar7 != 1) goto LAB_009255c0;
      piVar23 = *(int **)(puVar4 + -0xa0);
    }
    func_0x002aa848(piVar23);
  }
LAB_009255c0:
  if (*(long *)(piVar8 + 0x1e) == 0) goto LAB_00925894;
  if ((*(ulong *)(param_4[1] + 0x138) >> 0x2e & 1) != 0) {
    if (uVar6 == 0) {
      if ((*(long *)(param_4[1] + 0x78) != 0) &&
         (iVar7 = func_0x0093e190(), plVar11 = param_4, iVar7 != 0)) {
        do {
          if ((*(ulong *)(plVar11[1] + 0x138) >> 0x2e & 1) != 0) break;
          plVar11 = *(long **)(plVar11[1] + 0x80);
        } while (plVar11 != (long *)0x0);
        plVar15 = (long *)func_0x0094b564(*(undefined8 *)(param_4[1] + 0x78));
        if (plVar11 == plVar15) {
          lVar20 = *(long *)(piVar8 + 0x1e);
          plVar11 = *(long **)(piVar8 + 0x20);
LAB_00925754:
          func_0x00942544(lVar20,plVar11);
        }
      }
    }
    else {
      lVar20 = *(long *)(piVar8 + 0x20);
      if (((lVar20 != 0) && (*(long *)(*(long *)(lVar20 + 8) + 0x78) != 0)) &&
         (iVar7 = func_0x0093e190(), lVar10 = lVar20, iVar7 != 0)) {
        do {
          if ((*(ulong *)(*(long *)(lVar10 + 8) + 0x138) >> 0x2e & 1) != 0) break;
          lVar10 = *(long *)(*(long *)(lVar10 + 8) + 0x80);
        } while (lVar10 != 0);
        lVar20 = func_0x0094b564(*(undefined8 *)(*(long *)(lVar20 + 8) + 0x78));
        if (lVar10 == lVar20) {
          *(uint *)(param_4[1] + 0x140) = *(uint *)(param_4[1] + 0x140) | 4;
          *(uint *)(param_4[1] + 0x140) = *(uint *)(param_4[1] + 0x140) | 8;
          lVar20 = *(long *)(param_4[1] + 0x78);
          plVar11 = param_4;
          if (lVar20 != 0) goto LAB_00925754;
        }
      }
    }
  }
  if (*(long *)(piVar8 + 0x1e) == 0) goto LAB_00925894;
  if (uVar6 == 0) {
    if ((int)unaff_x26 != 0) {
      for (lVar20 = *(long *)(piVar8 + 0x20); lVar20 != 0; lVar20 = *(long *)(lVar20 + 0x80)) {
        lVar20 = *(long *)(lVar20 + 8);
        if ((*(ulong *)(lVar20 + 0x138) >> 0x2f & 1) != 0) {
          if (((uint)*(ulong *)(lVar20 + 0x138) >> 5 & 1) != 0) {
            uVar24 = 1;
            goto LAB_00925890;
          }
          break;
        }
      }
    }
    goto LAB_00925894;
  }
  for (lVar20 = *(long *)(piVar8 + 0x20); lVar20 != 0; lVar20 = *(long *)(lVar20 + 0x80)) {
    lVar20 = *(long *)(lVar20 + 8);
    if ((*(ulong *)(lVar20 + 0x138) >> 0x2f & 1) != 0) {
      lVar20 = *(long *)(lVar20 + 0x120);
      lVar10 = *(long *)(piVar8 + 0x52);
      if (lVar10 == lVar20) goto LAB_00925854;
      if (lVar20 != 0) {
        lVar21 = *(long *)(lVar20 + 8);
        iVar7 = *(int *)(lVar21 + 0x100);
        if (iVar7 == -1) {
          func_0x0092c4dc(lVar21);
          iVar7 = *(int *)(lVar21 + 0x100);
        }
        lVar21 = *(long *)(lVar10 + 8);
        iVar16 = *(int *)(lVar21 + 0x100);
        if (iVar16 == -1) {
          func_0x0092c4dc(lVar21);
          iVar16 = *(int *)(lVar21 + 0x100);
        }
        lVar21 = lVar20;
        if (iVar16 <= iVar7) goto LAB_00925840;
      }
      break;
    }
  }
LAB_009255fc:
  lVar20 = *(long *)(piVar8 + 0x46);
  if ((lVar20 != 0) && (lVar10 = func_0x0093e1a4(*(undefined8 *)(piVar8 + 0x1e)), lVar20 != lVar10))
  {
    func_0x00940410(*(undefined8 *)(piVar8 + 0x1e),lVar20,7);
    goto LAB_00925894;
  }
  if ((((*unaff_x25 >> 0x2f & 1) == 0) ||
      (lVar20 = func_0x0093e1a4(*(undefined8 *)(piVar8 + 0x1e)), lVar20 != 0)) ||
     (plVar11 = *(long **)(*(long *)(*(long *)(piVar8 + 0x1e) + 8) + 0x138),
     plVar11 == (long *)0x0 || plVar11 == param_4)) goto LAB_00925894;
  lVar20 = plVar11[1];
  iVar7 = *(int *)(lVar20 + 0x100);
  if (iVar7 == -1) {
    func_0x0092c4dc(lVar20);
    iVar7 = *(int *)(lVar20 + 0x100);
  }
  lVar20 = param_4[1];
  iVar16 = *(int *)(lVar20 + 0x100);
  if (iVar16 == -1) {
    func_0x0092c4dc(lVar20);
    iVar16 = *(int *)(lVar20 + 0x100);
  }
  if (iVar7 < iVar16) goto LAB_00925894;
  do {
    plVar11 = *(long **)(plVar11[1] + 0x80);
    if (plVar11 == (long *)0x0) goto LAB_00925894;
  } while (plVar11 != param_4);
  lVar20 = *(long *)(*(long *)(piVar8 + 0x52) + 8);
  goto LAB_00925870;
  while (lVar21 != lVar10) {
LAB_00925840:
    lVar21 = *(long *)(*(long *)(lVar21 + 8) + 0x80);
    if (lVar21 == 0) goto LAB_009255fc;
  }
LAB_00925854:
  lVar10 = *(long *)(lVar20 + 8);
  do {
    lVar20 = lVar10;
    if (*(long *)(lVar20 + 0x120) == 0) break;
    lVar10 = *(long *)(*(long *)(lVar20 + 0x120) + 8);
  } while (((uint)*(undefined8 *)(lVar10 + 0x138) >> 5 & 1) != 0);
LAB_00925870:
  uVar24 = 0;
LAB_00925890:
  func_0x00929768(lVar20,7,1,uVar24);
LAB_00925894:
  (**(code **)(**(long **)(piVar8 + 0x52) + 0x108))
            (puVar4 + -0xb0,*(long **)(piVar8 + 0x52),0xc,puVar4 + -0x78);
  func_0x003dec3c(puVar4 + -0xb0);
  if ((*unaff_x25 >> 0x38 & 1) != 0) {
    lVar20 = 0;
    if (*(long *)(piVar8 + 0x52) != 0) {
      lVar20 = *(long *)(piVar8 + 0x52) + -0x10;
    }
    func_0x003d4ac8(lVar20,&UNK_013fa3a8,2,0);
  }
LAB_009258e4:
  func_0x003dec3c(puVar4 + -0x78);
  return;
}


