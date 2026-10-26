// FUN_009c5258 size=3556

void FUN_009c5258(long param_1,ulong param_2)

{
  float *pfVar1;
  byte bVar2;
  float fVar3;
  uint uVar4;
  uint uVar5;
  byte bVar6;
  char cVar7;
  long lVar8;
  int iVar9;
  undefined8 uVar10;
  byte *pbVar11;
  byte bVar12;
  byte *pbVar13;
  byte bVar14;
  float *unaff_x20;
  float *unaff_x21;
  long lVar15;
  float *pfVar16;
  long unaff_x28;
  long lVar17;
  float *pfStack_110;
  float *pfStack_108;
  float *pfStack_100;
  float *pfStack_f8;
  undefined1 auStack_f0 [16];
  undefined *puStack_e0;
  long lStack_d8;
  undefined *puStack_d0;
  long lStack_c8;
  undefined *puStack_c0;
  long lStack_b8;
  undefined *puStack_b0;
  long lStack_a8;
  long lStack_a0;
  long lStack_98;
  undefined **appuStack_90 [4];
  long lStack_70;
  
  lStack_70 = *(long *)PTR____stack_chk_guard_013b4320;
  if (*(char *)(param_1 + 0x59) == '\0') {
    unaff_x20 = (float *)(param_1 + 0x88);
    func_0x0097f9e8(unaff_x20);
    if (*(char *)(param_1 + 0x59) == '\0') {
      func_0x009c8fcc(param_1,1);
    }
    func_0x0097fb60(unaff_x20);
  }
  lVar15 = *(long *)(param_1 + 0x68);
  func_0x0097fc98(auStack_f0,param_1 + 0x40,1);
  cVar7 = *(char *)(param_1 + 0xe0);
  lVar17 = unaff_x28;
  if ((cVar7 != '\0') && (*(long *)(param_1 + 0x28) == lVar15)) goto LAB_009c5f04;
  unaff_x20 = (float *)(param_1 + 0x18);
  *(undefined8 *)(param_1 + 0x20) = 0;
  unaff_x20[0] = 0.0;
  unaff_x20[1] = 0.0;
  unaff_x21 = (float *)(param_1 + 8);
  *(undefined8 *)(param_1 + 0x10) = 0;
  unaff_x21[0] = 0.0;
  unaff_x21[1] = 0.0;
  *(undefined8 *)(param_1 + 0x28) = 0;
  if (cVar7 == '\0') goto LAB_009c5f04;
  pfVar1 = (float *)(param_1 + 0x24);
  pfStack_f8 = (float *)(param_1 + 0x20);
  pfStack_108 = (float *)(param_1 + 0x1c);
  pfVar16 = (float *)(param_1 + 0x14);
  pfStack_100 = (float *)(param_1 + 0x10);
  pfStack_110 = (float *)(param_1 + 0xc);
  iVar9 = *(int *)(param_1 + 0xf0);
  if (iVar9 != 2) {
    if (iVar9 == 1) {
      cVar7 = *(char *)(param_1 + 0xf4);
      lVar8 = *(long *)(param_1 + 200);
      lVar17 = *(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0);
      if ((param_2 & 1) != 0) {
        puStack_b0 = &UNK_013fd960;
        puStack_c0 = &UNK_013fd990;
        puStack_d0 = &UNK_013fd9c0;
        puStack_e0 = &UNK_013fd9f0;
        appuStack_90[0] = &puStack_b0;
        if (cVar7 == '\0') {
          appuStack_90[1] = &puStack_c0;
          appuStack_90[2] = &puStack_d0;
          param_2 = 3;
        }
        else {
          param_2 = 1;
        }
        if (lVar8 == 0) {
          if (param_2 == 1) {
            uVar10 = *(undefined8 *)(param_1 + 0xb0);
            uVar5 = *(uint *)(&UNK_0007f778 + lRam0000000001489630);
            uVar4 = *(uint *)(&UNK_006d0638 + lRam0000000001489640);
            iVar9 = (int)(CONCAT44((uVar4 + (uVar4 >> 0x10)) * 0x101 + (uVar4 >> 0x18),uVar4) >>
                         0x10);
            if (iVar9 != 1) {
              if (iRam0000000001489534 != 0) {
                uVar5 = uVar5 >> 0x1a | uVar5 << 6;
              }
              *(uint *)(lRam0000000001489638 + 0x14b13c8) = uVar5;
            }
            iRam0000000001489534 = iVar9;
            lStack_d8 = param_1;
            lStack_c8 = param_1;
            lStack_b8 = param_1;
            lStack_a8 = param_1;
            func_0x009efeb0(uVar10,&lStack_98,&lStack_a0,lVar17);
            *unaff_x21 = (float)lStack_98 * 0.0625;
            *unaff_x20 = (float)lStack_a0 * 0.0625;
            cVar7 = *(char *)(param_1 + 0xf4);
            goto joined_r0x009c573c;
          }
        }
        else {
          appuStack_90[param_2] = &puStack_e0;
          param_2 = param_2 + 1;
        }
        unaff_x28 = lVar17;
        lStack_d8 = param_1;
        lStack_c8 = param_1;
        lStack_b8 = param_1;
        lStack_a8 = param_1;
        if ((bRam0000000001557db8 & 1) == 0) goto LAB_009c5fa0;
        goto LAB_009c5a68;
      }
      uVar10 = *(undefined8 *)(param_1 + 0xb0);
      param_2 = 0x1489534;
      uVar5 = *(uint *)(&UNK_0007f778 + lRam0000000001489630);
      uVar4 = *(uint *)(&UNK_006d0638 + lRam0000000001489640);
      iVar9 = (int)(CONCAT44((uVar4 + (uVar4 >> 0x10)) * 0x101 + (uVar4 >> 0x18),uVar4) >> 0x10);
      if (iVar9 != 1) {
        if (iRam0000000001489534 != 0) {
          uVar5 = uVar5 >> 0x1a | uVar5 << 6;
        }
        *(uint *)(lRam0000000001489638 + 0x14b13c8) = uVar5;
      }
      unaff_x28 = 0x1489000;
      iRam0000000001489534 = iVar9;
      func_0x009efeb0(uVar10,appuStack_90,&puStack_b0,lVar17);
      *unaff_x21 = (float)(long)appuStack_90[0] * 0.0625;
      *unaff_x20 = (float)(long)puStack_b0 * 0.0625;
      if (cVar7 == '\0') {
        uVar10 = *(undefined8 *)(param_1 + 0xb8);
        uVar5 = *(uint *)(&UNK_0007f778 + lRam0000000001489630);
        uVar4 = *(uint *)(&UNK_006d0638 + lRam0000000001489640);
        iVar9 = (int)(CONCAT44((uVar4 + (uVar4 >> 0x10)) * 0x101 + (uVar4 >> 0x18),uVar4) >> 0x10);
        if (iVar9 != 1) {
          if (iRam0000000001489534 != 0) {
            uVar5 = uVar5 >> 0x1a | uVar5 << 6;
          }
          *(uint *)(lRam0000000001489638 + 0x14b13c8) = uVar5;
        }
        iRam0000000001489534 = iVar9;
        func_0x009efeb0(uVar10,appuStack_90,&puStack_b0,
                        *(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0));
        *(float *)(param_1 + 0xc) = (float)(long)appuStack_90[0] * 0.0625;
        *(float *)(param_1 + 0x1c) = (float)(long)puStack_b0 * 0.0625;
        uVar10 = *(undefined8 *)(param_1 + 0xc0);
        uVar5 = *(uint *)(&UNK_0007f778 + lRam0000000001489630);
        uVar4 = *(uint *)(&UNK_006d0638 + lRam0000000001489640);
        iVar9 = (int)(CONCAT44((uVar4 + (uVar4 >> 0x10)) * 0x101 + (uVar4 >> 0x18),uVar4) >> 0x10);
        if (iVar9 != 1) {
          if (iRam0000000001489534 != 0) {
            uVar5 = uVar5 >> 0x1a | uVar5 << 6;
          }
          *(uint *)(lRam0000000001489638 + 0x14b13c8) = uVar5;
        }
        iRam0000000001489534 = iVar9;
        func_0x009efeb0(uVar10,appuStack_90,&puStack_b0,
                        *(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0));
        *pfStack_100 = (float)(long)appuStack_90[0] * 0.0625;
        *pfStack_f8 = (float)(long)puStack_b0 * 0.0625;
      }
      if (lVar8 != 0) {
        uVar10 = *(undefined8 *)(param_1 + 200);
        uVar5 = *(uint *)(&UNK_0007f778 + lRam0000000001489630);
        uVar4 = *(uint *)(&UNK_006d0638 + lRam0000000001489640);
        iVar9 = (int)(CONCAT44((uVar4 + (uVar4 >> 0x10)) * 0x101 + (uVar4 >> 0x18),uVar4) >> 0x10);
        if (iVar9 != 1) {
          if (iRam0000000001489534 != 0) {
            uVar5 = uVar5 >> 0x1a | uVar5 << 6;
          }
          *(uint *)(lRam0000000001489638 + 0x14b13c8) = uVar5;
        }
        iRam0000000001489534 = iVar9;
        func_0x009efeb0(uVar10,appuStack_90,&puStack_b0,
                        *(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0));
        *pfVar16 = (float)(long)appuStack_90[0] * 0.0625;
        *pfVar1 = (float)(long)puStack_b0 * 0.0625;
        cVar7 = *(char *)(param_1 + 0xf4);
        param_2 = 0x1489534;
        lVar17 = unaff_x28;
        goto joined_r0x009c573c;
      }
      goto LAB_009c5ed0;
    }
    if (iVar9 != 0) goto LAB_009c5ed0;
    cVar7 = *(char *)(param_1 + 0xf4);
    pbVar11 = *(byte **)(param_1 + 200);
    unaff_x28 = *(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0);
    if ((param_2 & 1) == 0) {
      pbVar13 = *(byte **)(param_1 + 0xb0);
      uVar5 = *(uint *)(&UNK_001a2cf0 + lRam0000000001489620);
      if (iRam0000000001489530 != 0) {
        uVar5 = uVar5 >> 0x15 | uVar5 << 0xb;
      }
      iRam0000000001489530 =
           (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10);
      if (iRam0000000001489530 != 0) {
        *(uint *)(&UNK_00691b60 + lRam0000000001489628) = uVar5;
      }
      lVar17 = lRam0000000001563528;
      if (lRam0000000001563528 != 0) {
        iRam0000000001489530 =
             iRam0000000001489530 + (uint)(byte)(&UNK_0000838e)[lRam0000000001563528];
      }
      if (unaff_x28 < 1) {
        bVar12 = 0;
        bVar14 = 0xff;
      }
      else {
        bVar12 = *pbVar13;
        bVar6 = *pbVar13;
        lVar8 = unaff_x28;
        while (bVar14 = bVar6, lVar8 = lVar8 + -1, lVar8 != 0) {
          pbVar13 = pbVar13 + 1;
          bVar6 = *pbVar13;
          bVar2 = bVar6;
          if (bVar6 <= bVar12) {
            bVar2 = bVar12;
          }
          if (bVar14 <= bVar6) {
            bVar12 = bVar2;
            bVar6 = bVar14;
          }
        }
      }
      *unaff_x21 = (float)bVar14;
      *unaff_x20 = (float)bVar12;
      if (cVar7 == '\0') {
        pbVar13 = *(byte **)(param_1 + 0xb8);
        uVar5 = *(uint *)(&UNK_001a2cf0 + lRam0000000001489620);
        if (iRam0000000001489530 != 0) {
          uVar5 = uVar5 >> 0x15 | uVar5 << 0xb;
        }
        iRam0000000001489530 =
             (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10);
        if (iRam0000000001489530 != 0) {
          *(uint *)(&UNK_00691b60 + lRam0000000001489628) = uVar5;
        }
        if (lVar17 != 0) {
          iRam0000000001489530 = iRam0000000001489530 + (uint)(byte)(&UNK_0000838e)[lVar17];
        }
        if (unaff_x28 < 1) {
          bVar12 = 0;
          bVar14 = 0xff;
        }
        else {
          bVar12 = *pbVar13;
          bVar6 = *pbVar13;
          lVar8 = unaff_x28;
          while (bVar14 = bVar6, lVar8 = lVar8 + -1, lVar8 != 0) {
            pbVar13 = pbVar13 + 1;
            bVar6 = *pbVar13;
            bVar2 = bVar6;
            if (bVar6 <= bVar12) {
              bVar2 = bVar12;
            }
            if (bVar14 <= bVar6) {
              bVar12 = bVar2;
              bVar6 = bVar14;
            }
          }
        }
        *(float *)(param_1 + 0xc) = (float)bVar14;
        *(float *)(param_1 + 0x1c) = (float)bVar12;
        pbVar13 = *(byte **)(param_1 + 0xc0);
        uVar5 = *(uint *)(&UNK_001a2cf0 + lRam0000000001489620);
        if (iRam0000000001489530 != 0) {
          uVar5 = uVar5 >> 0x15 | uVar5 << 0xb;
        }
        iRam0000000001489530 =
             (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10);
        if (iRam0000000001489530 != 0) {
          *(uint *)(&UNK_00691b60 + lRam0000000001489628) = uVar5;
        }
        if (lVar17 != 0) {
          iRam0000000001489530 = iRam0000000001489530 + (uint)(byte)(&UNK_0000838e)[lVar17];
        }
        if (unaff_x28 < 1) {
          bVar12 = 0;
          bVar14 = 0xff;
        }
        else {
          bVar12 = *pbVar13;
          bVar6 = *pbVar13;
          lVar8 = unaff_x28;
          while (bVar14 = bVar6, lVar8 = lVar8 + -1, lVar8 != 0) {
            pbVar13 = pbVar13 + 1;
            bVar6 = *pbVar13;
            bVar2 = bVar6;
            if (bVar6 <= bVar12) {
              bVar2 = bVar12;
            }
            if (bVar14 <= bVar6) {
              bVar12 = bVar2;
              bVar6 = bVar14;
            }
          }
        }
        *pfStack_100 = (float)bVar14;
        *pfStack_f8 = (float)bVar12;
      }
      if (pbVar11 != (byte *)0x0) {
        uVar5 = *(uint *)(&UNK_001a2cf0 + lRam0000000001489620);
        if (iRam0000000001489530 != 0) {
          uVar5 = uVar5 >> 0x15 | uVar5 << 0xb;
        }
        iRam0000000001489530 =
             (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10);
        if (iRam0000000001489530 != 0) {
          *(uint *)(&UNK_00691b60 + lRam0000000001489628) = uVar5;
        }
        if (lVar17 != 0) {
          iRam0000000001489530 = iRam0000000001489530 + (uint)(byte)(&UNK_0000838e)[lVar17];
        }
        if (unaff_x28 < 1) {
          bVar12 = 0;
          bVar14 = 0xff;
        }
        else {
          bVar12 = *pbVar11;
          bVar6 = *pbVar11;
          lVar17 = unaff_x28;
          while (bVar14 = bVar6, lVar17 = lVar17 + -1, lVar17 != 0) {
            pbVar11 = pbVar11 + 1;
            bVar6 = *pbVar11;
            bVar2 = bVar6;
            if (bVar6 <= bVar12) {
              bVar2 = bVar12;
            }
            if (bVar14 <= bVar6) {
              bVar12 = bVar2;
              bVar6 = bVar14;
            }
          }
        }
        *pfVar16 = (float)bVar14;
        *pfVar1 = (float)bVar12;
      }
      goto LAB_009c5ed0;
    }
    puStack_b0 = &UNK_013fd8a0;
    puStack_c0 = &UNK_013fd8d0;
    puStack_d0 = &UNK_013fd900;
    puStack_e0 = &UNK_013fd930;
    appuStack_90[0] = &puStack_b0;
    if (cVar7 == '\0') {
      appuStack_90[1] = &puStack_c0;
      appuStack_90[2] = &puStack_d0;
      param_2 = 3;
      if (pbVar11 == (byte *)0x0) goto LAB_009c5b68;
LAB_009c539c:
      appuStack_90[param_2] = &puStack_e0;
      param_2 = param_2 + 1;
    }
    else {
      param_2 = 1;
      if (pbVar11 != (byte *)0x0) goto LAB_009c539c;
LAB_009c5b68:
      if (param_2 == 1) {
        pbVar11 = *(byte **)(param_1 + 0xb0);
        uVar5 = *(uint *)(&UNK_001a2cf0 + lRam0000000001489620);
        if (iRam0000000001489530 != 0) {
          uVar5 = uVar5 >> 0x15 | uVar5 << 0xb;
        }
        iRam0000000001489530 =
             (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10);
        if (iRam0000000001489530 != 0) {
          *(uint *)(&UNK_00691b60 + lRam0000000001489628) = uVar5;
        }
        if (lRam0000000001563528 != 0) {
          iRam0000000001489530 =
               iRam0000000001489530 + (uint)(byte)(&UNK_0000838e)[lRam0000000001563528];
        }
        if (unaff_x28 < 1) {
          bVar12 = 0;
          bVar14 = 0xff;
        }
        else {
          bVar12 = *pbVar11;
          bVar6 = *pbVar11;
          lVar17 = unaff_x28;
          while (bVar14 = bVar6, lVar17 = lVar17 + -1, lVar17 != 0) {
            pbVar11 = pbVar11 + 1;
            bVar6 = *pbVar11;
            bVar2 = bVar6;
            if (bVar6 <= bVar12) {
              bVar2 = bVar12;
            }
            if (bVar14 <= bVar6) {
              bVar12 = bVar2;
              bVar6 = bVar14;
            }
          }
        }
        *unaff_x21 = (float)bVar14;
        *unaff_x20 = (float)bVar12;
        cVar7 = *(char *)(param_1 + 0xf4);
        lVar17 = unaff_x28;
        lStack_d8 = param_1;
        lStack_c8 = param_1;
        lStack_b8 = param_1;
        lStack_a8 = param_1;
        goto joined_r0x009c573c;
      }
    }
    lStack_d8 = param_1;
    lStack_c8 = param_1;
    lStack_b8 = param_1;
    lStack_a8 = param_1;
    if (((bRam0000000001557db0 & 1) == 0) && (iVar9 = ___cxa_guard_acquire(0x1557db0), iVar9 != 0))
    {
      func_0x009fbba8(0x154d6a0,"EnsureMinMax.8");
      ___cxa_guard_release(0x1557db0);
    }
    func_0x009fc3a8(appuStack_90,param_2,uRam00000000014a67c0,0x154d6a0,param_2 * unaff_x28);
    goto LAB_009c5ed0;
  }
  cVar7 = *(char *)(param_1 + 0xf4);
  lVar17 = *(long *)(param_1 + 200);
  unaff_x28 = *(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0);
  if ((param_2 & 1) == 0) {
    uVar10 = *(undefined8 *)(param_1 + 0xb0);
    param_2 = 0x1489538;
    uVar5 = *(uint *)(lRam0000000001489648 + 0x1998e00);
    if (iRam0000000001489538 != 0) {
      uVar5 = uVar5 >> 0x16 | uVar5 << 10;
    }
    iRam0000000001489538 =
         (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10);
    if (iRam0000000001489538 != 0) {
      *(uint *)(lRam0000000001489650 + 0x1c2fbf0) = uVar5;
    }
    func_0x009efdd4(uVar10,unaff_x21,unaff_x20,unaff_x28);
    if (cVar7 == '\0') {
      uVar10 = *(undefined8 *)(param_1 + 0xb8);
      uVar5 = *(uint *)(lRam0000000001489648 + 0x1998e00);
      if (iRam0000000001489538 != 0) {
        uVar5 = uVar5 >> 0x16 | uVar5 << 10;
      }
      iRam0000000001489538 =
           (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10);
      if (iRam0000000001489538 != 0) {
        *(uint *)(lRam0000000001489650 + 0x1c2fbf0) = uVar5;
      }
      func_0x009efdd4(uVar10,pfStack_110,pfStack_108,
                      *(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0));
      uVar10 = *(undefined8 *)(param_1 + 0xc0);
      uVar5 = *(uint *)(lRam0000000001489648 + 0x1998e00);
      if (iRam0000000001489538 != 0) {
        uVar5 = uVar5 >> 0x16 | uVar5 << 10;
      }
      iRam0000000001489538 =
           (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10);
      if (iRam0000000001489538 != 0) {
        *(uint *)(lRam0000000001489650 + 0x1c2fbf0) = uVar5;
      }
      func_0x009efdd4(uVar10,pfStack_100,pfStack_f8,
                      *(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0));
    }
    if (lVar17 != 0) {
      uVar10 = *(undefined8 *)(param_1 + 200);
      uVar5 = *(uint *)(lRam0000000001489648 + 0x1998e00);
      if (iRam0000000001489538 != 0) {
        uVar5 = uVar5 >> 0x16 | uVar5 << 10;
      }
      iRam0000000001489538 =
           (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10);
      if (iRam0000000001489538 != 0) {
        *(uint *)(lRam0000000001489650 + 0x1c2fbf0) = uVar5;
      }
      func_0x009efdd4(uVar10,pfVar16,pfVar1,*(long *)(param_1 + 0xd8) * *(long *)(param_1 + 0xd0));
    }
    goto LAB_009c5ed0;
  }
  puStack_b0 = &UNK_013fda20;
  puStack_c0 = &UNK_013fda50;
  puStack_d0 = &UNK_013fda80;
  puStack_e0 = &UNK_013fdab0;
  appuStack_90[0] = &puStack_b0;
  if (cVar7 == '\0') {
    appuStack_90[1] = &puStack_c0;
    appuStack_90[2] = &puStack_d0;
    param_2 = 3;
    if (lVar17 == 0) goto LAB_009c5aa4;
LAB_009c5474:
    appuStack_90[param_2] = &puStack_e0;
    param_2 = param_2 + 1;
  }
  else {
    param_2 = 1;
    if (lVar17 != 0) goto LAB_009c5474;
LAB_009c5aa4:
    if (param_2 == 1) {
      uVar10 = *(undefined8 *)(param_1 + 0xb0);
      uVar5 = *(uint *)(lRam0000000001489648 + 0x1998e00);
      if (iRam0000000001489538 != 0) {
        uVar5 = uVar5 >> 0x16 | uVar5 << 10;
      }
      iRam0000000001489538 =
           (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10);
      if (iRam0000000001489538 != 0) {
        *(uint *)(lRam0000000001489650 + 0x1c2fbf0) = uVar5;
      }
      lStack_d8 = param_1;
      lStack_c8 = param_1;
      lStack_b8 = param_1;
      lStack_a8 = param_1;
      func_0x009efdd4(uVar10,unaff_x21,unaff_x20,unaff_x28);
      goto LAB_009c5ed0;
    }
  }
  lStack_d8 = param_1;
  lStack_c8 = param_1;
  lStack_b8 = param_1;
  lStack_a8 = param_1;
  if (((bRam0000000001557dc0 & 1) == 0) && (iVar9 = ___cxa_guard_acquire(0x1557dc0), iVar9 != 0)) {
    func_0x009fbba8(0x154ea40,"EnsureMinMax.32");
    ___cxa_guard_release(0x1557dc0);
  }
  func_0x009fc3a8(appuStack_90,param_2,uRam00000000014a67c0,0x154ea40,param_2 * unaff_x28);
LAB_009c5ed0:
  while( true ) {
    cVar7 = *(char *)(param_1 + 0xf4);
    lVar17 = unaff_x28;
joined_r0x009c573c:
    if (cVar7 != '\0') {
      fVar3 = *unaff_x21;
      *pfStack_100 = fVar3;
      *pfStack_110 = fVar3;
      fVar3 = *unaff_x20;
      *pfStack_f8 = fVar3;
      *pfStack_108 = fVar3;
    }
    *(long *)(param_1 + 0x28) = lVar15;
LAB_009c5f04:
    func_0x0097fe08(auStack_f0);
    if (*(long *)PTR____stack_chk_guard_013b4320 == lStack_70) break;
    ___stack_chk_fail();
LAB_009c5fa0:
    iVar9 = ___cxa_guard_acquire(0x1557db8);
    unaff_x28 = lVar17;
    if (iVar9 != 0) {
      func_0x009fbba8(0x154e070,"EnsureMinMax.Half");
      ___cxa_guard_release(0x1557db8);
    }
LAB_009c5a68:
    func_0x009fc3a8(appuStack_90,param_2,uRam00000000014a67c0,0x154e070,param_2 * unaff_x28);
  }
  return;
}


