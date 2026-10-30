// FUN_00051b10 size=3028

/* WARNING: Possible PIC construction at 0x00051d6c: Changing call to branch */
/* WARNING: Possible PIC construction at 0x00051d34: Changing call to branch */
/* WARNING: Possible PIC construction at 0x00925638: Changing call to branch */
/* WARNING: Removing unreachable block (ram,0x00051d38) */
/* WARNING: Removing unreachable block (ram,0x00051d70) */
/* WARNING: Removing unreachable block (ram,0x00051d80) */
/* WARNING: Removing unreachable block (ram,0x00051d84) */
/* WARNING: Removing unreachable block (ram,0x00051d94) */
/* WARNING: Removing unreachable block (ram,0x00051d98) */
/* WARNING: Removing unreachable block (ram,0x00051da4) */
/* WARNING: Removing unreachable block (ram,0x0092563c) */

void FUN_00051b10(undefined1 param_1 [16],double param_2,double param_3,double param_4,long param_5)

{
  undefined1 *puVar1;
  bool bVar2;
  uint uVar3;
  int iVar4;
  long lVar5;
  long *plVar6;
  undefined8 uVar7;
  long lVar8;
  long *plVar9;
  ulong uVar10;
  undefined8 uVar11;
  ulong uVar12;
  ulong uVar13;
  int iVar14;
  long lVar15;
  uint *puVar16;
  ulong uVar17;
  int *piVar18;
  uint uVar19;
  long lVar20;
  undefined8 *puVar21;
  long unaff_x19;
  long *unaff_x20;
  ulong unaff_x21;
  ulong unaff_x22;
  ulong unaff_x23;
  undefined8 unaff_x24;
  ulong *unaff_x25;
  ulong unaff_x26;
  long *unaff_x27;
  undefined8 unaff_x28;
  undefined1 *unaff_x29;
  undefined8 unaff_x30;
  double dVar22;
  double dVar23;
  undefined1 auStack_a0 [8];
  uint *puStack_98;
  uint *puStack_90;
  undefined8 auStack_88 [11];
  
  puVar1 = &stack0xfffffffffffffff0;
  if (*(long *)(param_5 + 0x148) == 0) {
    lVar15 = *(long *)(param_5 + 0xd8);
    uVar10 = 0;
    goto SUB_009299d4;
  }
  uVar3 = *(uint *)(lRam000000000141a8b0 + 0x1a15690);
  bVar2 = uRam000000000141a8a8 != 0;
  uRam000000000141a8a8 = uVar3;
  if (bVar2) {
    uRam000000000141a8a8 = uVar3 >> 0x15 | uVar3 << 0xb;
  }
  if (uRam000000000141a8a8 != 0) {
    *(uint *)(lRam000000000141a8b8 + 0x16dc800) = uRam000000000141a8a8;
  }
  if (lRam0000000001562f68 != 0) {
    uRam000000000141a8a8 = uRam000000000141a8a8 + (byte)(&UNK_0000cee6)[lRam0000000001562f68];
  }
  dVar22 = (double)func_0x0005102c(param_5,0);
  lVar15 = *(long *)(param_5 + 0x148);
  if (lVar15 == 0) {
    dVar23 = 0.0;
    iVar4 = 0;
  }
  else {
    iVar4 = 0;
    if (*(long *)(lVar15 + 8) != 0) {
      iVar4 = *(int *)(*(long *)(lVar15 + 8) + 0xd0);
    }
    dVar23 = (double)iVar4;
    if (dVar22 + param_3 <= (double)iVar4) {
      dVar23 = dVar22 + param_3;
    }
    if (dVar23 <= 0.0) {
      dVar23 = 0.0;
    }
    iVar4 = 0;
    if (*(long *)(lVar15 + 8) != 0) {
      iVar4 = *(int *)(*(long *)(lVar15 + 8) + 0xd8);
    }
  }
  dVar22 = (double)iVar4;
  if (param_2 + param_4 <= (double)iVar4) {
    dVar22 = param_2 + param_4;
  }
  if (dVar22 <= 0.0) {
    dVar22 = 0.0;
  }
  unaff_x20 = *(long **)(param_5 + 0xd8);
  func_0x00965aa0(auStack_88,param_5);
  func_0x00056b04(auStack_88[0],unaff_x20);
  func_0x000568b4(dVar23,dVar22,*(undefined8 *)(param_5 + 0xd8),3);
  unaff_x19 = param_5;
  unaff_x29 = puVar1;
  if (*(int *)(param_5 + 0xcc) == 0) {
    lVar15 = *(long *)(param_5 + 0xd8);
    uVar10 = 0;
    unaff_x30 = 0x51d38;
    register0x00000008 = (BADSPACEBASE *)auStack_a0;
    goto SUB_009299d4;
  }
  FUN_00042c44(auStack_88);
  if (*(char *)(param_5 + 0x6b) == '\0') {
    unaff_x20 = *(long **)(param_5 + 0xd8);
LAB_00051d40:
    func_0x000567e4(unaff_x20,auStack_88);
  }
  else {
    unaff_x20 = *(long **)(param_5 + 0xd8);
    if (*(long *)(param_5 + 0x148) == 0) goto LAB_00051d40;
    func_0x003bbff0(&puStack_98,&UNK_013c1668,"Updating - ",0,0xffffffff);
    puStack_90 = puStack_98;
    puVar16 = puStack_98;
    if (1 < *puStack_98 + 1) {
      LOAcquire();
      uVar3 = *puStack_98;
      *puStack_98 = uVar3 + 1;
      puVar16 = (uint *)(ulong)uVar3;
      LORelease();
    }
    func_0x002e97cc(puVar16,&puStack_90,auStack_88);
    func_0x000567e4(unaff_x20,&puStack_90);
    if (*puStack_90 != 0xffffffff) {
      if (*puStack_90 != 0) {
        LOAcquire();
        uVar3 = *puStack_90;
        *puStack_90 = uVar3 - 1;
        LORelease();
        if (uVar3 != 1) goto LAB_00051cf4;
      }
      func_0x00287df8(puStack_90,2,8);
    }
LAB_00051cf4:
    if (*puStack_98 != 0xffffffff) {
      if (*puStack_98 != 0) {
        LOAcquire();
        uVar3 = *puStack_98;
        *puStack_98 = uVar3 - 1;
        LORelease();
        if (uVar3 != 1) goto LAB_00051d4c;
      }
      func_0x00287df8(puStack_98,2,8);
    }
  }
LAB_00051d4c:
  lVar15 = *(long *)(param_5 + 0xd8);
  if (*(char *)(param_5 + 0x69) == '\0') {
    uVar10 = (ulong)(*(int *)(param_5 + 200) < 1);
  }
  else {
    uVar10 = 0;
  }
  unaff_x30 = 0x51d70;
  register0x00000008 = (BADSPACEBASE *)auStack_a0;
SUB_009299d4:
  uVar13 = *(ulong *)(*(long *)(lVar15 + 8) + 0x138) >> 0x2e & 1;
  bVar2 = true;
  uVar12 = 1;
  puVar1 = (undefined1 *)register0x00000008;
  lVar15 = *(long *)(lVar15 + 8);
  do {
    lVar5 = lVar15;
    *(undefined8 *)(puVar1 + -0x60) = unaff_x28;
    *(long **)(puVar1 + -0x58) = unaff_x27;
    *(ulong *)(puVar1 + -0x50) = unaff_x26;
    *(ulong **)(puVar1 + -0x48) = unaff_x25;
    *(undefined8 *)(puVar1 + -0x40) = unaff_x24;
    *(ulong *)(puVar1 + -0x38) = unaff_x23;
    *(ulong *)(puVar1 + -0x30) = unaff_x22;
    *(ulong *)(puVar1 + -0x28) = unaff_x21;
    *(long **)(puVar1 + -0x20) = unaff_x20;
    *(long *)(puVar1 + -0x18) = unaff_x19;
    *(undefined1 **)(puVar1 + -0x10) = unaff_x29;
    *(undefined8 *)(puVar1 + -8) = unaff_x30;
    unaff_x29 = puVar1 + -0x10;
    unaff_x20 = *(long **)(lVar5 + 0x148);
    unaff_x25 = (ulong *)(lVar5 + 0x138);
    uVar17 = *(ulong *)(lVar5 + 0x138);
    uVar3 = (uint)uVar10;
    if (bVar2) {
      uVar10 = 0;
      if (uVar3 == 0) {
        uVar10 = 0x40;
      }
      uVar17 = uVar17 & 0xffffffffffffffbf | uVar10;
      *(undefined4 *)(lVar5 + 0x140) = *(undefined4 *)(lVar5 + 0x140);
      *unaff_x25 = uVar17;
    }
    if (((uint)(uVar17 >> 5) & 1) == uVar3) {
      return;
    }
    if ((*(long *)(lVar5 + 0x80) != 0 && uVar3 != 0) &&
       (((uint)*(undefined8 *)(*(long *)(*(long *)(lVar5 + 0x80) + 8) + 0x138) >> 5 & 1) == 0)) {
      return;
    }
    func_0x003df028(puVar1 + -0xa0);
    (**(code **)(*unaff_x20 + 0x108))(puVar1 + -0x78,unaff_x20,2,puVar1 + -0xa0);
    func_0x003dec3c(puVar1 + -0xa0);
    uVar10 = func_0x003e1eb4(puVar1 + -0x78);
    uVar3 = (uint)uVar10;
    if ((uVar3 ^ (uint)(*unaff_x25 >> 5) & 1) != 1) goto LAB_009258e4;
    uVar17 = 0x20;
    if (uVar3 == 0) {
      uVar17 = 0;
    }
    *(undefined4 *)(lVar5 + 0x140) = *(undefined4 *)(lVar5 + 0x140);
    *unaff_x25 = *unaff_x25 & 0xffffffffffffffdf | uVar17;
    if ((int)uVar12 != 0) {
      lVar15 = *(long *)(lVar5 + 8);
      uVar17 = (ulong)*(uint *)(lVar15 + 4);
      if (0 < (int)*(uint *)(lVar15 + 4)) {
        lVar15 = *(long *)(lVar15 + 0x10) + lVar15 + 8;
        do {
          if (*(int *)(lVar15 + -8) == 2) {
            func_0x003decf4(puVar1 + -0xa0);
            goto LAB_0092513c;
          }
          lVar15 = lVar15 + 0x18;
          uVar17 = uVar17 - 1;
        } while (uVar17 != 0);
      }
      *(undefined4 *)(puVar1 + -0x98) = 0x80000000;
      *(undefined8 *)(puVar1 + -0xa0) = 0;
LAB_0092513c:
      iVar4 = func_0x003df6a8(puVar1 + -0xa0);
      if (iVar4 == 0x1f) {
        plVar6 = (long *)func_0x003dece0(puVar1 + -0xa0);
        lVar15 = *plVar6;
      }
      else {
        iVar4 = func_0x003e2fc0(puVar1 + -0xa0,0x1f,puVar1 + -0x68);
        lVar15 = *(long *)(puVar1 + -0x68);
        if (iVar4 == 0) {
          lVar15 = 0;
        }
      }
      func_0x003dec3c(puVar1 + -0xa0);
      if (lVar15 != 0) {
        func_0x009266c0(lVar15);
      }
      lVar15 = lVar5;
      if (*(long *)(lVar5 + 0x78) != 0) {
        do {
          if ((*(long *)(lVar15 + 0x98) != 0) &&
             (uVar19 = *(uint *)(lVar15 + 0x140), *(uint *)(lVar15 + 0x140) = uVar19 | 1,
             (uVar19 >> 6 & 1) == 0)) {
            func_0x0097af5c(*(undefined8 *)
                             (*(long *)(*(long *)(*(long *)(lVar15 + 0x98) + 8) + 0x58) + 8),2);
          }
        } while ((*(long *)(lVar15 + 0x80) != 0) &&
                (lVar15 = *(long *)(*(long *)(lVar15 + 0x80) + 8), lVar15 != 0));
        uVar7 = *(undefined8 *)(*(long *)(lVar5 + 0x78) + 8);
        uVar11 = *(undefined8 *)(lVar5 + 0x148);
        *(undefined8 *)(puVar1 + -0x98) = 0;
        *(undefined8 *)(puVar1 + -0xa0) = 0;
        *(undefined8 *)(puVar1 + -0x88) = 0;
        *(undefined8 *)(puVar1 + -0x90) = 0;
        func_0x0093c6f8(uVar7,uVar11,puVar1 + -0xa0,0,1,0,0,0);
      }
    }
    lVar15 = *(long *)(lVar5 + 0x148);
    do {
      lVar8 = lVar15;
      if ((*(long *)(*(long *)(lVar8 + 8) + 0x78) == 0) || (iVar4 = func_0x0093e190(), iVar4 == 0))
      goto LAB_00925240;
      lVar20 = *(long *)(lVar8 + 8);
      lVar15 = *(long *)(lVar20 + 0x108);
    } while (*(long *)(lVar20 + 0x108) != 0);
    lVar15 = *(long *)(*(long *)(lVar20 + 0x78) + 8);
    if (*(long *)(lVar15 + 0x130) == lVar8) {
      while (((*(ulong *)(lVar20 + 0x138) >> 0x2e & 1) == 0 &&
             (lVar8 = *(long *)(lVar20 + 0x80), lVar8 != 0))) {
        lVar20 = *(long *)(lVar8 + 8);
      }
      unaff_x26 = (ulong)(lVar8 == *(long *)(lVar15 + 0x150));
      if ((uVar10 & 1) == 0) goto LAB_009252e4;
LAB_00925248:
      uVar17 = *unaff_x25;
      *unaff_x25 = uVar17 | 0x50000000000000;
      if (*(long *)(lVar5 + 0x78) != 0) {
        if (((uint)uVar17 >> 0x18 & 1) != 0) {
          lVar15 = 0;
          if (*(long *)(lVar5 + 0x148) != 0) {
            lVar15 = *(long *)(lVar5 + 0x148) + -0x10;
          }
          iVar4 = func_0x0096957c(lVar15);
          if (iVar4 == 9) {
            func_0x0093e6e8(*(undefined8 *)(*(long *)(lVar5 + 0x78) + 8),lVar15);
          }
        }
        if (((*(ulong *)(unaff_x20[1] + 0x138) >> 0x2e & 1) != 0) && (*(int *)(lVar5 + 300) != 0)) {
          func_0x00941fd0(*(undefined8 *)(*(long *)(lVar5 + 0x78) + 8),
                          *(undefined8 *)(lVar5 + 0x148),0);
        }
      }
    }
    else {
LAB_00925240:
      unaff_x26 = 0;
      if ((uVar10 & 1) != 0) goto LAB_00925248;
LAB_009252e4:
      if (*(long *)(lVar5 + 0x78) != 0) {
        lVar15 = *(long *)(*(long *)(*(long *)(lVar5 + 0x78) + 8) + 0x178);
        iVar4 = *(int *)(lVar15 + 8);
        iVar14 = *(int *)(lVar15 + 0xc);
        puVar21 = (undefined8 *)(lVar15 + 0x10 + (long)iVar4 * 8);
        if (iVar4 == iVar14) {
LAB_00925308:
          if (puVar21 != (undefined8 *)(lVar15 + 0x10 + (long)iVar14 * 8)) {
            if (*(long *)(unaff_x20[1] + 0x78) == 0) {
              func_0x00c8c558();
            }
            else {
              func_0x0093d280(*(undefined8 *)(*(long *)(unaff_x20[1] + 0x78) + 8),unaff_x20,0);
            }
          }
        }
        else {
          lVar8 = (long)iVar14 * 8 + (long)iVar4 * -8;
          do {
            if ((long *)*puVar21 == unaff_x20) goto LAB_00925308;
            puVar21 = puVar21 + 1;
            lVar8 = lVar8 + -8;
          } while (lVar8 != 0);
        }
        lVar15 = *(long *)(*(long *)(*(long *)(lVar5 + 0x78) + 8) + 0x180);
        iVar4 = *(int *)(lVar15 + 8);
        iVar14 = *(int *)(lVar15 + 0xc);
        puVar21 = (undefined8 *)(lVar15 + 0x10 + (long)iVar4 * 8);
        if (iVar4 == iVar14) {
LAB_00925374:
          if (puVar21 != (undefined8 *)(lVar15 + 0x10 + (long)iVar14 * 8)) {
            if (*(long *)(unaff_x20[1] + 0x78) == 0) {
              func_0x00c8c528();
            }
            else {
              func_0x0093d54c(*(undefined8 *)(*(long *)(unaff_x20[1] + 0x78) + 8),unaff_x20,0);
            }
          }
        }
        else {
          lVar8 = (long)iVar14 * 8 + (long)iVar4 * -8;
          do {
            if ((long *)*puVar21 == unaff_x20) goto LAB_00925374;
            puVar21 = puVar21 + 1;
            lVar8 = lVar8 + -8;
          } while (lVar8 != 0);
        }
        if (((*(ulong *)(unaff_x20[1] + 0x138) >> 0x2e & 1) != 0) && (*(int *)(lVar5 + 300) != 0)) {
          func_0x0093d040(*(undefined8 *)(*(long *)(lVar5 + 0x78) + 8),
                          *(undefined8 *)(lVar5 + 0x148));
        }
      }
      if (((int)unaff_x26 != 0) && (*(long *)(lVar5 + 0x78) != 0)) {
        lVar15 = func_0x0093e1a4();
        if ((((uint)*unaff_x25 >> 0x18 & 1) != 0) &&
           (lVar8 = *(long *)(lVar15 + 8), (*(ulong *)(lVar8 + 0x138) >> 0x2e & 1) == 0)) {
          lVar20 = *(long *)(lVar5 + 0x148);
          while (lVar15 != lVar20) {
            do {
              lVar15 = *(long *)(lVar8 + 0x80);
              if (lVar15 == 0) goto LAB_00925460;
              lVar8 = *(long *)(lVar15 + 8);
            } while (((uint)*(ulong *)(lVar8 + 0x138) >> 0x18 & 1) == 0);
            if ((*(ulong *)(lVar8 + 0x138) >> 0x2e & 1) != 0) goto LAB_00925460;
          }
          plVar6 = (long *)0x0;
          if (lVar20 != 0) {
            plVar6 = (long *)(lVar20 + -0x10);
          }
          uVar17 = (**(code **)(*plVar6 + 0xf0))(plVar6,1);
          if ((uVar17 & 1) != 0) goto LAB_00925470;
        }
LAB_00925460:
        func_0x0092951c(lVar5,0,uVar13);
      }
LAB_00925470:
      lVar15 = (*(long **)(lVar5 + 0x148))[1];
      uVar19 = (uint)*(undefined8 *)(lVar15 + 0x138);
      unaff_x27 = *(long **)(lVar5 + 0x148);
      while ((uVar19 >> 0xc & 1) != 0) {
        do {
          plVar6 = *(long **)(lVar15 + 0x80);
          if (plVar6 == (long *)0x0) goto LAB_00925484;
          iVar4 = (**(code **)(*plVar6 + 0x58))(plVar6);
          lVar15 = plVar6[1];
        } while (iVar4 != 10);
        unaff_x27 = plVar6;
        uVar19 = (uint)*(undefined8 *)(lVar15 + 0x138);
      }
LAB_00925484:
      if (((uint)*(undefined8 *)(unaff_x27[1] + 0x138) >> 9 & 1) != 0) {
        func_0x0092830c(*(undefined8 *)(lVar5 + 0x148),0);
      }
    }
    if ((int)uVar12 != 0) {
      uVar12 = (ulong)((uint)((*unaff_x25 & 0x8001000000000) == 0) | (uint)(*unaff_x25 >> 0x2a) & 1)
      ;
    }
    piVar18 = *(int **)(lVar5 + 0x88);
    *(int **)(puVar1 + -0xa0) = piVar18;
    if (*piVar18 != -1) {
      if (*piVar18 == 0) {
        func_0x002aa738(puVar1 + -0xa0,piVar18[1]);
        lVar20 = *(long *)(puVar1 + -0xa0);
        lVar15 = lVar20 + (long)*(int *)(lVar20 + 8) * 8;
        lVar8 = *(long *)(lVar5 + 0x88) + (long)*(int *)(*(long *)(lVar5 + 0x88) + 8) * 8;
        if ((lVar8 != lVar15) &&
           (lVar15 = lVar15 + 0x10,
           0 < ((lVar20 + (long)*(int *)(lVar20 + 0xc) * 8) - lVar15) + 0x10)) {
          _memcpy(lVar15,lVar8 + 0x10);
        }
      }
      else {
        LOAcquire();
        *piVar18 = *piVar18 + 1;
        LORelease();
      }
    }
    lVar15 = *(long *)(puVar1 + -0xa0);
    iVar4 = *(int *)(lVar15 + 8);
    plVar6 = (long *)(lVar15 + 0x10 + (long)iVar4 * 8);
    *(long **)(puVar1 + -0x98) = plVar6;
    iVar14 = *(int *)(lVar15 + 0xc);
    *(long *)(puVar1 + -0x90) = lVar15 + 0x10 + (long)iVar14 * 8;
    unaff_x24 = 1;
    *(undefined4 *)(puVar1 + -0x88) = 1;
    if (iVar4 == iVar14) break;
    while (((uVar10 & 1) != 0 &&
           (((uint)*(undefined8 *)(*(long *)(*plVar6 + 8) + 0x138) >> 6 & 1) != 0))) {
      plVar6 = plVar6 + 1;
      *(long **)(puVar1 + -0x98) = plVar6;
      *(undefined4 *)(puVar1 + -0x88) = 1;
      if (plVar6 == *(long **)(puVar1 + -0x90)) goto LAB_00925594;
    }
    bVar2 = false;
    unaff_x30 = 0x92563c;
    puVar1 = puVar1 + -0xb0;
    lVar15 = *(long *)(*plVar6 + 8);
    unaff_x19 = lVar5;
    unaff_x21 = uVar10;
    unaff_x22 = uVar13;
    unaff_x23 = uVar12;
  } while( true );
LAB_00925594:
  piVar18 = *(int **)(puVar1 + -0xa0);
  if (*piVar18 != -1) {
    if (*piVar18 != 0) {
      LOAcquire();
      iVar4 = *piVar18;
      *piVar18 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_009255c0;
      piVar18 = *(int **)(puVar1 + -0xa0);
    }
    func_0x002aa848(piVar18);
  }
LAB_009255c0:
  if (*(long *)(lVar5 + 0x78) == 0) goto LAB_00925894;
  if ((*(ulong *)(unaff_x20[1] + 0x138) >> 0x2e & 1) != 0) {
    if (uVar3 == 0) {
      if ((*(long *)(unaff_x20[1] + 0x78) != 0) &&
         (iVar4 = func_0x0093e190(), plVar6 = unaff_x20, iVar4 != 0)) {
        do {
          if ((*(ulong *)(plVar6[1] + 0x138) >> 0x2e & 1) != 0) break;
          plVar6 = *(long **)(plVar6[1] + 0x80);
        } while (plVar6 != (long *)0x0);
        plVar9 = (long *)func_0x0094b564(*(undefined8 *)(unaff_x20[1] + 0x78));
        if (plVar6 == plVar9) {
          lVar15 = *(long *)(lVar5 + 0x78);
          plVar6 = *(long **)(lVar5 + 0x80);
LAB_00925754:
          func_0x00942544(lVar15,plVar6);
        }
      }
    }
    else {
      lVar15 = *(long *)(lVar5 + 0x80);
      if (((lVar15 != 0) && (*(long *)(*(long *)(lVar15 + 8) + 0x78) != 0)) &&
         (iVar4 = func_0x0093e190(), lVar8 = lVar15, iVar4 != 0)) {
        do {
          if ((*(ulong *)(*(long *)(lVar8 + 8) + 0x138) >> 0x2e & 1) != 0) break;
          lVar8 = *(long *)(*(long *)(lVar8 + 8) + 0x80);
        } while (lVar8 != 0);
        lVar15 = func_0x0094b564(*(undefined8 *)(*(long *)(lVar15 + 8) + 0x78));
        if (lVar8 == lVar15) {
          *(uint *)(unaff_x20[1] + 0x140) = *(uint *)(unaff_x20[1] + 0x140) | 4;
          *(uint *)(unaff_x20[1] + 0x140) = *(uint *)(unaff_x20[1] + 0x140) | 8;
          lVar15 = *(long *)(unaff_x20[1] + 0x78);
          plVar6 = unaff_x20;
          if (lVar15 != 0) goto LAB_00925754;
        }
      }
    }
  }
  if (*(long *)(lVar5 + 0x78) == 0) goto LAB_00925894;
  if (uVar3 == 0) {
    if ((int)unaff_x26 != 0) {
      for (lVar15 = *(long *)(lVar5 + 0x80); lVar15 != 0; lVar15 = *(long *)(lVar15 + 0x80)) {
        lVar15 = *(long *)(lVar15 + 8);
        if ((*(ulong *)(lVar15 + 0x138) >> 0x2f & 1) != 0) {
          if (((uint)*(ulong *)(lVar15 + 0x138) >> 5 & 1) != 0) {
            uVar7 = 1;
            goto LAB_00925890;
          }
          break;
        }
      }
    }
    goto LAB_00925894;
  }
  for (lVar15 = *(long *)(lVar5 + 0x80); lVar15 != 0; lVar15 = *(long *)(lVar15 + 0x80)) {
    lVar15 = *(long *)(lVar15 + 8);
    if ((*(ulong *)(lVar15 + 0x138) >> 0x2f & 1) != 0) {
      lVar15 = *(long *)(lVar15 + 0x120);
      lVar8 = *(long *)(lVar5 + 0x148);
      if (lVar8 == lVar15) goto LAB_00925854;
      if (lVar15 != 0) {
        lVar20 = *(long *)(lVar15 + 8);
        iVar4 = *(int *)(lVar20 + 0x100);
        if (iVar4 == -1) {
          func_0x0092c4dc(lVar20);
          iVar4 = *(int *)(lVar20 + 0x100);
        }
        lVar20 = *(long *)(lVar8 + 8);
        iVar14 = *(int *)(lVar20 + 0x100);
        if (iVar14 == -1) {
          func_0x0092c4dc(lVar20);
          iVar14 = *(int *)(lVar20 + 0x100);
        }
        lVar20 = lVar15;
        if (iVar14 <= iVar4) goto LAB_00925840;
      }
      break;
    }
  }
LAB_009255fc:
  lVar15 = *(long *)(lVar5 + 0x118);
  if ((lVar15 != 0) && (lVar8 = func_0x0093e1a4(*(undefined8 *)(lVar5 + 0x78)), lVar15 != lVar8)) {
    func_0x00940410(*(undefined8 *)(lVar5 + 0x78),lVar15,7);
    goto LAB_00925894;
  }
  if ((((*unaff_x25 >> 0x2f & 1) == 0) ||
      (lVar15 = func_0x0093e1a4(*(undefined8 *)(lVar5 + 0x78)), lVar15 != 0)) ||
     (plVar6 = *(long **)(*(long *)(*(long *)(lVar5 + 0x78) + 8) + 0x138),
     plVar6 == (long *)0x0 || plVar6 == unaff_x20)) goto LAB_00925894;
  lVar15 = plVar6[1];
  iVar4 = *(int *)(lVar15 + 0x100);
  if (iVar4 == -1) {
    func_0x0092c4dc(lVar15);
    iVar4 = *(int *)(lVar15 + 0x100);
  }
  lVar15 = unaff_x20[1];
  iVar14 = *(int *)(lVar15 + 0x100);
  if (iVar14 == -1) {
    func_0x0092c4dc(lVar15);
    iVar14 = *(int *)(lVar15 + 0x100);
  }
  if (iVar4 < iVar14) goto LAB_00925894;
  do {
    plVar6 = *(long **)(plVar6[1] + 0x80);
    if (plVar6 == (long *)0x0) goto LAB_00925894;
  } while (plVar6 != unaff_x20);
  lVar15 = *(long *)(*(long *)(lVar5 + 0x148) + 8);
  goto LAB_00925870;
  while (lVar20 != lVar8) {
LAB_00925840:
    lVar20 = *(long *)(*(long *)(lVar20 + 8) + 0x80);
    if (lVar20 == 0) goto LAB_009255fc;
  }
LAB_00925854:
  lVar8 = *(long *)(lVar15 + 8);
  do {
    lVar15 = lVar8;
    if (*(long *)(lVar15 + 0x120) == 0) break;
    lVar8 = *(long *)(*(long *)(lVar15 + 0x120) + 8);
  } while (((uint)*(undefined8 *)(lVar8 + 0x138) >> 5 & 1) != 0);
LAB_00925870:
  uVar7 = 0;
LAB_00925890:
  func_0x00929768(lVar15,7,1,uVar7);
LAB_00925894:
  (**(code **)(**(long **)(lVar5 + 0x148) + 0x108))
            (puVar1 + -0xb0,*(long **)(lVar5 + 0x148),0xc,puVar1 + -0x78);
  func_0x003dec3c(puVar1 + -0xb0);
  if ((*unaff_x25 >> 0x38 & 1) != 0) {
    lVar15 = 0;
    if (*(long *)(lVar5 + 0x148) != 0) {
      lVar15 = *(long *)(lVar5 + 0x148) + -0x10;
    }
    func_0x003d4ac8(lVar15,&UNK_013fa3a8,2,0);
  }
LAB_009258e4:
  func_0x003dec3c(puVar1 + -0x78);
  return;
}


