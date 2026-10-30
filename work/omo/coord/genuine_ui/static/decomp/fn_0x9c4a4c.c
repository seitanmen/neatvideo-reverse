// FUN_009c4a4c size=2000

void FUN_009c4a4c(float *param_1,float *param_2,float *param_3,float *param_4,ulong param_5)

{
  float fVar1;
  uint uVar2;
  uint uVar3;
  char cVar4;
  bool bVar5;
  int iVar6;
  undefined8 uVar7;
  float *pfVar8;
  long lVar9;
  byte *pbVar10;
  short *psVar11;
  float *unaff_x19;
  float *unaff_x20;
  float *unaff_x21;
  float *unaff_x22;
  long unaff_x23;
  double dVar12;
  double dVar13;
  long lStack_d0;
  undefined *puStack_c8;
  float *pfStack_c0;
  float *pfStack_b8;
  undefined1 *puStack_b0;
  undefined *puStack_a8;
  float *pfStack_a0;
  float *pfStack_98;
  undefined1 *puStack_90;
  undefined *puStack_88;
  float *pfStack_80;
  float *pfStack_78;
  undefined1 *puStack_70;
  undefined **ppuStack_68;
  undefined **ppuStack_60;
  undefined **ppuStack_58;
  long lStack_48;
  
  lStack_48 = *(long *)PTR____stack_chk_guard_013b4320;
  if (*(char *)(param_1 + 0x38) == '\0') goto LAB_009c514c;
  unaff_x22 = param_1;
  if (*(char *)((long)param_1 + 0xe1) != '\0') goto LAB_009c514c;
  uVar2 = *(uint *)(&UNK_00190468 + lRam0000000001489600);
  bVar5 = uRam0000000001489524 != 0;
  uRam0000000001489524 = uVar2;
  if (bVar5) {
    uRam0000000001489524 = uVar2 >> 0x12 | uVar2 << 0xe;
  }
  fVar1 = param_1[0x3c];
  if (fVar1 == 2.8026e-45) {
    unaff_x23 = *(long *)(param_1 + 0x36) * *(long *)(param_1 + 0x34);
    cVar4 = *(char *)(param_1 + 0x3d);
    lStack_d0 = unaff_x23;
    if ((param_5 & 1) == 0) {
      func_0x009ef3fc(*(undefined8 *)(param_1 + 0x2c),param_2,unaff_x23);
      if (cVar4 != '\0') goto LAB_009c5138;
      func_0x009ef3fc(*(undefined8 *)(param_1 + 0x2e),param_3,unaff_x23);
      uVar7 = *(undefined8 *)(param_1 + 0x30);
      pfVar8 = param_4;
    }
    else {
      puStack_88 = &UNK_013fd810;
      puStack_a8 = &UNK_013fd840;
      puStack_c8 = &UNK_013fd870;
      ppuStack_68 = &puStack_88;
      pfStack_c0 = param_1;
      pfStack_b8 = param_4;
      pfStack_a0 = param_1;
      pfStack_98 = param_3;
      pfStack_80 = param_1;
      pfStack_78 = param_2;
      if (cVar4 == '\0') {
        ppuStack_60 = &puStack_a8;
        ppuStack_58 = &puStack_c8;
        puStack_b0 = (undefined1 *)&lStack_d0;
        puStack_90 = (undefined1 *)&lStack_d0;
        puStack_70 = (undefined1 *)&lStack_d0;
        if (((bRam0000000001557da8 & 1) == 0) &&
           (puStack_b0 = (undefined1 *)&lStack_d0, puStack_90 = (undefined1 *)&lStack_d0,
           puStack_70 = (undefined1 *)&lStack_d0, iVar6 = ___cxa_guard_acquire(0x1557da8),
           iVar6 != 0)) {
          func_0x009fbba8(0x154ccd0,"Mean.32");
          ___cxa_guard_release(0x1557da8);
        }
        lVar9 = unaff_x23 * 3;
        uVar7 = 0x154ccd0;
        goto LAB_009c512c;
      }
      uVar7 = *(undefined8 *)(param_1 + 0x2c);
      pfVar8 = param_2;
      puStack_b0 = (undefined1 *)&lStack_d0;
      puStack_90 = (undefined1 *)&lStack_d0;
      puStack_70 = (undefined1 *)&lStack_d0;
    }
    func_0x009ef3fc(uVar7,pfVar8,unaff_x23);
    goto LAB_009c5138;
  }
  if (fVar1 == 1.4013e-45) {
    cVar4 = *(char *)(param_1 + 0x3d);
    unaff_x23 = *(long *)(param_1 + 0x36) * *(long *)(param_1 + 0x34);
    if ((param_5 & 1) == 0) {
      psVar11 = *(short **)(param_1 + 0x2c);
      uVar3 = *(uint *)(&UNK_005bba50 + lRam00000000014894b8);
      uVar2 = (uVar3 ^ uVar3 >> 1) & 0x45511110;
      uVar3 = (uVar2 | uVar2 << 1) ^ uVar3;
      iRam00000000014894b0 =
           (int)(CONCAT44((uVar3 + (uVar3 >> 0x10)) * 0x101 + (uVar3 >> 0x18),uVar3) >> 0x10);
      if (iRam00000000014894b0 != 0) {
        *(uint *)(&UNK_00086a00 + lRam00000000014894c0) = uVar3;
      }
      dVar12 = 0.0;
      lVar9 = unaff_x23;
      if (0 < unaff_x23) {
        do {
          dVar12 = dVar12 + (double)(int)*psVar11;
          lVar9 = lVar9 + -1;
          psVar11 = psVar11 + 1;
        } while (lVar9 != 0);
      }
      dVar13 = (double)unaff_x23;
      *param_2 = (float)((dVar12 / dVar13) * 0.0625);
      if (cVar4 != '\0') goto LAB_009c5138;
      psVar11 = *(short **)(param_1 + 0x2e);
      uVar2 = *(uint *)(&UNK_005bba50 + lRam00000000014894b8);
      uVar2 = ((uVar2 ^ uVar2 >> 1) & 0x45511110) * 3 ^ uVar2;
      if ((int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10) != 0) {
        *(uint *)(&UNK_00086a00 + lRam00000000014894c0) = uVar2;
      }
      dVar12 = 0.0;
      lVar9 = unaff_x23;
      if (0 < unaff_x23) {
        do {
          dVar12 = dVar12 + (double)(int)*psVar11;
          lVar9 = lVar9 + -1;
          psVar11 = psVar11 + 1;
        } while (lVar9 != 0);
      }
      *param_3 = (float)((dVar12 / dVar13) * 0.0625);
      psVar11 = *(short **)(param_1 + 0x30);
      uVar2 = *(uint *)(&UNK_005bba50 + lRam00000000014894b8);
      uVar2 = ((uVar2 ^ uVar2 >> 1) & 0x45511110) * 3 ^ uVar2;
      iRam00000000014894b0 =
           (int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10);
      if (iRam00000000014894b0 != 0) {
        *(uint *)(&UNK_00086a00 + lRam00000000014894c0) = uVar2;
      }
      dVar12 = 0.0;
      lVar9 = unaff_x23;
      if (0 < unaff_x23) {
        do {
          dVar12 = dVar12 + (double)(int)*psVar11;
          unaff_x23 = unaff_x23 + -1;
          lVar9 = 0;
          psVar11 = psVar11 + 1;
        } while (unaff_x23 != 0);
      }
      dVar12 = (dVar12 / dVar13) * 0.0625;
LAB_009c507c:
      *param_4 = (float)dVar12;
      unaff_x23 = lVar9;
      goto LAB_009c5138;
    }
    puStack_88 = &UNK_013fd780;
    puStack_a8 = &UNK_013fd7b0;
    puStack_c8 = &UNK_013fd7e0;
    ppuStack_68 = &puStack_88;
    if (cVar4 == '\0') {
      ppuStack_60 = &puStack_a8;
      ppuStack_58 = &puStack_c8;
      pfStack_c0 = param_4;
      pfStack_b8 = param_1;
      pfStack_a0 = param_3;
      pfStack_98 = param_1;
      pfStack_80 = param_2;
      pfStack_78 = param_1;
      if ((bRam0000000001557da0 & 1) == 0) goto LAB_009c5180;
      goto LAB_009c50a4;
    }
    psVar11 = *(short **)(param_1 + 0x2c);
    uVar3 = *(uint *)(&UNK_005bba50 + lRam00000000014894b8);
    uVar2 = (uVar3 ^ uVar3 >> 1) & 0x45511110;
    uVar3 = (uVar2 | uVar2 << 1) ^ uVar3;
    iRam00000000014894b0 =
         (int)(CONCAT44((uVar3 + (uVar3 >> 0x10)) * 0x101 + (uVar3 >> 0x18),uVar3) >> 0x10);
    if (iRam00000000014894b0 != 0) {
      *(uint *)(&UNK_00086a00 + lRam00000000014894c0) = uVar3;
    }
    dVar12 = 0.0;
    lVar9 = unaff_x23;
    if (0 < unaff_x23) {
      do {
        dVar12 = dVar12 + (double)(int)*psVar11;
        lVar9 = lVar9 + -1;
        psVar11 = psVar11 + 1;
      } while (lVar9 != 0);
    }
    dVar12 = (dVar12 / (double)unaff_x23) * 0.0625;
  }
  else {
    if (fVar1 != 0.0) goto LAB_009c5138;
    cVar4 = *(char *)(param_1 + 0x3d);
    unaff_x23 = *(long *)(param_1 + 0x36) * *(long *)(param_1 + 0x34);
    if ((param_5 & 1) == 0) {
      pbVar10 = *(byte **)(param_1 + 0x2c);
      uVar3 = *(uint *)(&UNK_005bba50 + lRam00000000014894a0);
      uVar2 = (uVar3 ^ uVar3 >> 1) & 0x45511110;
      uVar3 = (uVar2 | uVar2 << 1) ^ uVar3;
      iRam0000000001489498 =
           (int)(CONCAT44((uVar3 + (uVar3 >> 0x10)) * 0x101 + (uVar3 >> 0x18),uVar3) >> 0x10);
      if (iRam0000000001489498 != 0) {
        *(uint *)(&UNK_00086a00 + lRam00000000014894a8) = uVar3;
      }
      dVar12 = 0.0;
      lVar9 = unaff_x23;
      if (0 < unaff_x23) {
        do {
          dVar12 = dVar12 + (double)*pbVar10;
          lVar9 = lVar9 + -1;
          pbVar10 = pbVar10 + 1;
        } while (lVar9 != 0);
      }
      dVar13 = (double)unaff_x23;
      *param_2 = (float)(dVar12 / dVar13);
      if (cVar4 != '\0') goto LAB_009c5138;
      pbVar10 = *(byte **)(param_1 + 0x2e);
      uVar2 = *(uint *)(&UNK_005bba50 + lRam00000000014894a0);
      uVar2 = ((uVar2 ^ uVar2 >> 1) & 0x45511110) * 3 ^ uVar2;
      if ((int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10) != 0) {
        *(uint *)(&UNK_00086a00 + lRam00000000014894a8) = uVar2;
      }
      dVar12 = 0.0;
      lVar9 = unaff_x23;
      if (0 < unaff_x23) {
        do {
          dVar12 = dVar12 + (double)*pbVar10;
          lVar9 = lVar9 + -1;
          pbVar10 = pbVar10 + 1;
        } while (lVar9 != 0);
      }
      *param_3 = (float)(dVar12 / dVar13);
      pbVar10 = *(byte **)(param_1 + 0x30);
      uVar2 = *(uint *)(&UNK_005bba50 + lRam00000000014894a0);
      uVar2 = ((uVar2 ^ uVar2 >> 1) & 0x45511110) * 3 ^ uVar2;
      iRam0000000001489498 =
           (int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10);
      if (iRam0000000001489498 != 0) {
        *(uint *)(&UNK_00086a00 + lRam00000000014894a8) = uVar2;
      }
      dVar12 = 0.0;
      lVar9 = unaff_x23;
      if (0 < unaff_x23) {
        do {
          dVar12 = dVar12 + (double)*pbVar10;
          unaff_x23 = unaff_x23 + -1;
          lVar9 = 0;
          pbVar10 = pbVar10 + 1;
        } while (unaff_x23 != 0);
      }
      dVar12 = dVar12 / dVar13;
      goto LAB_009c507c;
    }
    puStack_88 = &UNK_013fd6f0;
    puStack_a8 = &UNK_013fd720;
    puStack_c8 = &UNK_013fd750;
    ppuStack_68 = &puStack_88;
    if (cVar4 == '\0') {
      ppuStack_60 = &puStack_a8;
      ppuStack_58 = &puStack_c8;
      pfStack_c0 = param_4;
      pfStack_b8 = param_1;
      pfStack_a0 = param_3;
      pfStack_98 = param_1;
      pfStack_80 = param_2;
      pfStack_78 = param_1;
      if (((bRam0000000001557d98 & 1) == 0) && (iVar6 = ___cxa_guard_acquire(0x1557d98), iVar6 != 0)
         ) {
        func_0x009fbba8(0x154b930,"Mean.8");
        ___cxa_guard_release(0x1557d98);
      }
      lVar9 = unaff_x23 * 3;
      uVar7 = 0x154b930;
      goto LAB_009c512c;
    }
    pbVar10 = *(byte **)(param_1 + 0x2c);
    uVar3 = *(uint *)(&UNK_005bba50 + lRam00000000014894a0);
    uVar2 = (uVar3 ^ uVar3 >> 1) & 0x45511110;
    uVar3 = (uVar2 | uVar2 << 1) ^ uVar3;
    iRam0000000001489498 =
         (int)(CONCAT44((uVar3 + (uVar3 >> 0x10)) * 0x101 + (uVar3 >> 0x18),uVar3) >> 0x10);
    if (iRam0000000001489498 != 0) {
      *(uint *)(&UNK_00086a00 + lRam00000000014894a8) = uVar3;
    }
    dVar12 = 0.0;
    lVar9 = unaff_x23;
    if (0 < unaff_x23) {
      do {
        dVar12 = dVar12 + (double)*pbVar10;
        lVar9 = lVar9 + -1;
        pbVar10 = pbVar10 + 1;
      } while (lVar9 != 0);
    }
    dVar12 = dVar12 / (double)unaff_x23;
  }
  ppuStack_68 = &puStack_88;
  *param_2 = (float)dVar12;
  pfStack_c0 = param_4;
  pfStack_b8 = param_1;
  pfStack_a0 = param_3;
  pfStack_98 = param_1;
  pfStack_80 = param_2;
  pfStack_78 = param_1;
LAB_009c5138:
  while( true ) {
    unaff_x19 = param_3;
    unaff_x20 = param_4;
    unaff_x21 = param_2;
    if (*(char *)(unaff_x22 + 0x3d) != '\0') {
      fVar1 = *param_2;
      *param_4 = fVar1;
      *param_3 = fVar1;
    }
LAB_009c514c:
    param_2 = unaff_x21;
    param_4 = unaff_x20;
    param_3 = unaff_x19;
    if (*(long *)PTR____stack_chk_guard_013b4320 == lStack_48) break;
    ___stack_chk_fail();
    param_1 = unaff_x22;
LAB_009c5180:
    iVar6 = ___cxa_guard_acquire(0x1557da0);
    if (iVar6 != 0) {
      func_0x009fbba8(0x154c300,"Mean.Half");
      ___cxa_guard_release(0x1557da0);
    }
LAB_009c50a4:
    lVar9 = unaff_x23 * 3;
    uVar7 = 0x154c300;
LAB_009c512c:
    func_0x009fc3a8(&ppuStack_68,3,uRam00000000014a67c0,uVar7,lVar9);
    unaff_x22 = param_1;
  }
  return;
}


