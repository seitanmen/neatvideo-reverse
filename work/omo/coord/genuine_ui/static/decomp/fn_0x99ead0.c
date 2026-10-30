// FUN_0099ead0 size=3092

void FUN_0099ead0(long param_1,uint param_2)

{
  float *pfVar1;
  uint uVar2;
  uint uVar3;
  byte bVar4;
  undefined8 *puVar5;
  byte *pbVar6;
  ulong uVar7;
  long lVar8;
  long lVar9;
  long *plVar10;
  long lVar11;
  int iVar12;
  long lVar13;
  long lVar14;
  long lVar15;
  undefined8 uVar16;
  undefined8 uVar17;
  float fVar18;
  undefined8 uVar19;
  undefined8 uVar20;
  float fVar21;
  float fVar22;
  undefined8 uVar23;
  undefined8 uVar24;
  undefined8 uVar25;
  
  uVar3 = *(uint *)(&UNK_00afd278 + lRam0000000001488578);
  uVar2 = (uVar3 ^ uVar3 >> 1) & 0x44554545;
  uVar3 = (uVar2 | uVar2 << 1) ^ uVar3;
  if (uVar3 != 1) {
    *(uint *)(&UNK_00a71308 + lRam0000000001488580) = uVar3;
  }
  puVar5 = *(undefined8 **)(&UNK_00003d08 + param_1);
  uVar16 = *puVar5;
  *(undefined8 *)(&UNK_00003d18 + param_1) = puVar5[1];
  *(undefined8 *)(&UNK_00003d10 + param_1) = uVar16;
  uVar16 = puVar5[2];
  uVar19 = puVar5[5];
  uVar17 = puVar5[4];
  uVar23 = puVar5[7];
  uVar20 = puVar5[6];
  uVar25 = puVar5[9];
  uVar24 = puVar5[8];
  *(undefined8 *)(&UNK_00003d28 + param_1) = puVar5[3];
  *(undefined8 *)(&UNK_00003d20 + param_1) = uVar16;
  *(undefined8 *)(&UNK_00003d58 + param_1) = uVar25;
  *(undefined8 *)(&UNK_00003d50 + param_1) = uVar24;
  *(undefined8 *)(&UNK_00003d48 + param_1) = uVar23;
  *(undefined8 *)(&UNK_00003d40 + param_1) = uVar20;
  *(undefined8 *)(&UNK_00003d38 + param_1) = uVar19;
  *(undefined8 *)(&UNK_00003d30 + param_1) = uVar17;
  uVar16 = puVar5[10];
  uVar19 = puVar5[0xd];
  uVar17 = puVar5[0xc];
  uVar23 = puVar5[0xf];
  uVar20 = puVar5[0xe];
  uVar25 = puVar5[0x11];
  uVar24 = puVar5[0x10];
  *(undefined8 *)(&UNK_00003d68 + param_1) = puVar5[0xb];
  *(undefined8 *)(&UNK_00003d60 + param_1) = uVar16;
  *(undefined8 *)(&UNK_00003d98 + param_1) = uVar25;
  *(undefined8 *)(&UNK_00003d90 + param_1) = uVar24;
  *(undefined8 *)(&UNK_00003d88 + param_1) = uVar23;
  *(undefined8 *)(&UNK_00003d80 + param_1) = uVar20;
  *(undefined8 *)(&UNK_00003d78 + param_1) = uVar19;
  *(undefined8 *)(&UNK_00003d70 + param_1) = uVar17;
  uVar16 = puVar5[0x12];
  uVar19 = puVar5[0x15];
  uVar17 = puVar5[0x14];
  uVar23 = puVar5[0x17];
  uVar20 = puVar5[0x16];
  uVar25 = puVar5[0x19];
  uVar24 = puVar5[0x18];
  *(undefined8 *)(&UNK_00003da8 + param_1) = puVar5[0x13];
  *(undefined8 *)(&UNK_00003da0 + param_1) = uVar16;
  *(undefined8 *)(&UNK_00003dd8 + param_1) = uVar25;
  *(undefined8 *)(&UNK_00003dd0 + param_1) = uVar24;
  *(undefined8 *)(&UNK_00003dc8 + param_1) = uVar23;
  *(undefined8 *)(&UNK_00003dc0 + param_1) = uVar20;
  *(undefined8 *)(&UNK_00003db8 + param_1) = uVar19;
  *(undefined8 *)(&UNK_00003db0 + param_1) = uVar17;
  uVar16 = *puVar5;
  *(undefined8 *)(&UNK_00003de8 + param_1) = puVar5[1];
  *(undefined8 *)(&UNK_00003de0 + param_1) = uVar16;
  uVar17 = puVar5[3];
  uVar16 = puVar5[2];
  uVar20 = puVar5[5];
  uVar19 = puVar5[4];
  uVar24 = puVar5[7];
  uVar23 = puVar5[6];
  uVar25 = puVar5[8];
  *(undefined8 *)(&UNK_00003e28 + param_1) = puVar5[9];
  *(undefined8 *)(&UNK_00003e20 + param_1) = uVar25;
  *(undefined8 *)(&UNK_00003e18 + param_1) = uVar24;
  *(undefined8 *)(&UNK_00003e10 + param_1) = uVar23;
  *(undefined8 *)(&UNK_00003e08 + param_1) = uVar20;
  *(undefined8 *)(param_1 + 0x3e00) = uVar19;
  *(undefined8 *)(&UNK_00003df8 + param_1) = uVar17;
  *(undefined8 *)(&UNK_00003df0 + param_1) = uVar16;
  uVar17 = puVar5[0xb];
  uVar16 = puVar5[10];
  uVar20 = puVar5[0xd];
  uVar19 = puVar5[0xc];
  uVar24 = puVar5[0xf];
  uVar23 = puVar5[0xe];
  uVar25 = puVar5[0x10];
  *(undefined8 *)(&UNK_00003e68 + param_1) = puVar5[0x11];
  *(undefined8 *)(&UNK_00003e60 + param_1) = uVar25;
  *(undefined8 *)(&UNK_00003e58 + param_1) = uVar24;
  *(undefined8 *)(&UNK_00003e50 + param_1) = uVar23;
  *(undefined8 *)(&UNK_00003e48 + param_1) = uVar20;
  *(undefined8 *)(&UNK_00003e40 + param_1) = uVar19;
  *(undefined8 *)(&UNK_00003e38 + param_1) = uVar17;
  *(undefined8 *)(&UNK_00003e30 + param_1) = uVar16;
  uVar17 = puVar5[0x13];
  uVar16 = puVar5[0x12];
  uVar20 = puVar5[0x15];
  uVar19 = puVar5[0x14];
  uVar24 = puVar5[0x17];
  uVar23 = puVar5[0x16];
  uVar25 = puVar5[0x18];
  *(undefined8 *)(&UNK_00003ea8 + param_1) = puVar5[0x19];
  *(undefined8 *)(&UNK_00003ea0 + param_1) = uVar25;
  *(undefined8 *)(&UNK_00003e98 + param_1) = uVar24;
  *(undefined8 *)(&UNK_00003e90 + param_1) = uVar23;
  *(undefined8 *)(&UNK_00003e88 + param_1) = uVar20;
  *(undefined8 *)(&UNK_00003e80 + param_1) = uVar19;
  *(undefined8 *)(&UNK_00003e78 + param_1) = uVar17;
  *(undefined8 *)(&UNK_00003e70 + param_1) = uVar16;
  uVar16 = *puVar5;
  *(undefined8 *)(&UNK_00003eb8 + param_1) = puVar5[1];
  *(undefined8 *)(&UNK_00003eb0 + param_1) = uVar16;
  uVar17 = puVar5[3];
  uVar16 = puVar5[2];
  uVar20 = puVar5[5];
  uVar19 = puVar5[4];
  uVar24 = puVar5[7];
  uVar23 = puVar5[6];
  uVar25 = puVar5[8];
  *(undefined8 *)(&UNK_00003ef8 + param_1) = puVar5[9];
  *(undefined8 *)(&UNK_00003ef0 + param_1) = uVar25;
  *(undefined8 *)(&UNK_00003ee8 + param_1) = uVar24;
  *(undefined8 *)(&UNK_00003ee0 + param_1) = uVar23;
  *(undefined8 *)(&UNK_00003ed8 + param_1) = uVar20;
  *(undefined8 *)(&UNK_00003ed0 + param_1) = uVar19;
  *(undefined8 *)(&UNK_00003ec8 + param_1) = uVar17;
  *(undefined8 *)(&UNK_00003ec0 + param_1) = uVar16;
  uVar17 = puVar5[0xb];
  uVar16 = puVar5[10];
  uVar20 = puVar5[0xd];
  uVar19 = puVar5[0xc];
  uVar24 = puVar5[0xf];
  uVar23 = puVar5[0xe];
  uVar25 = puVar5[0x10];
  *(undefined8 *)(&UNK_00003f38 + param_1) = puVar5[0x11];
  *(undefined8 *)(&UNK_00003f30 + param_1) = uVar25;
  *(undefined8 *)(&UNK_00003f28 + param_1) = uVar24;
  *(undefined8 *)(&UNK_00003f20 + param_1) = uVar23;
  *(undefined8 *)(&UNK_00003f18 + param_1) = uVar20;
  *(undefined8 *)(&UNK_00003f10 + param_1) = uVar19;
  *(undefined8 *)(&UNK_00003f08 + param_1) = uVar17;
  *(undefined8 *)(param_1 + 0x3f00) = uVar16;
  uVar17 = puVar5[0x13];
  uVar16 = puVar5[0x12];
  uVar20 = puVar5[0x15];
  uVar19 = puVar5[0x14];
  uVar24 = puVar5[0x17];
  uVar23 = puVar5[0x16];
  uVar25 = puVar5[0x18];
  *(undefined8 *)(&UNK_00003f78 + param_1) = puVar5[0x19];
  *(undefined8 *)(&UNK_00003f70 + param_1) = uVar25;
  *(undefined8 *)(&UNK_00003f68 + param_1) = uVar24;
  *(undefined8 *)(&UNK_00003f60 + param_1) = uVar23;
  *(undefined8 *)(&UNK_00003f58 + param_1) = uVar20;
  *(undefined8 *)(&UNK_00003f50 + param_1) = uVar19;
  *(undefined8 *)(&UNK_00003f48 + param_1) = uVar17;
  *(undefined8 *)(&UNK_00003f40 + param_1) = uVar16;
  pbVar6 = *(byte **)(&UNK_00003d00 + param_1);
  if (pbVar6[0xcd] == 0) {
    fVar18 = 0.0;
  }
  else {
    fVar18 = *(float *)(pbVar6 + 0x128);
    fVar21 = *(float *)(pbVar6 + 0xec) * (float)*(int *)(pbVar6 + 0xd0) * fVar18 * fVar18;
    fVar18 = fVar18 * *(float *)(pbVar6 + 0xec) * (float)*(int *)(pbVar6 + 0xd0);
    fVar18 = SQRT(fVar21 + fVar21 + 1.0) / (fVar18 + fVar18 + 1.0);
  }
  bVar4 = *pbVar6;
  fVar21 = (float)bVar4;
  if (bVar4 == 0) {
    uVar2 = *(uint *)(lRam0000000001488588 + 0x1cf5b40);
    uVar2 = ((uVar2 ^ uVar2 >> 1) & 0x44400544) * 3 ^ uVar2;
    if ((int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10) != 0) {
      *(uint *)(&UNK_002a7b70 + lRam0000000001488590) = uVar2;
    }
    *(float *)(&UNK_00003d24 + param_1) =
         *(float *)(&UNK_00003d24 + param_1) *
         *(float *)(pbVar6 + 0x3c) * *(float *)(pbVar6 + 0x1c) * fVar21;
    *(float *)(&UNK_00003df4 + param_1) =
         *(float *)(&UNK_00003df4 + param_1) *
         *(float *)(pbVar6 + 0x40) * *(float *)(pbVar6 + 0x1c) * fVar21;
    *(float *)(&UNK_00003ec4 + param_1) =
         *(float *)(&UNK_00003ec4 + param_1) *
         *(float *)(pbVar6 + 0x44) * *(float *)(pbVar6 + 0x1c) * fVar21;
    uVar2 = *(uint *)(lRam0000000001488588 + 0x1cf5b40);
    uVar2 = ((uVar2 ^ uVar2 >> 1) & 0x44400544) * 3 ^ uVar2;
    if ((int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10) != 0) {
      *(uint *)(&UNK_002a7b70 + lRam0000000001488590) = uVar2;
    }
    *(float *)(&UNK_00003d28 + param_1) =
         *(float *)(&UNK_00003d28 + param_1) *
         *(float *)(pbVar6 + 0x3c) * *(float *)(pbVar6 + 0x20) * fVar21;
    *(float *)(&UNK_00003df8 + param_1) =
         *(float *)(&UNK_00003df8 + param_1) *
         *(float *)(pbVar6 + 0x40) * *(float *)(pbVar6 + 0x20) * fVar21;
    *(float *)(&UNK_00003ec8 + param_1) =
         *(float *)(&UNK_00003ec8 + param_1) *
         *(float *)(pbVar6 + 0x44) * *(float *)(pbVar6 + 0x20) * fVar21;
    uVar2 = *(uint *)(lRam0000000001488588 + 0x1cf5b40);
    uVar2 = ((uVar2 ^ uVar2 >> 1) & 0x44400544) * 3 ^ uVar2;
    if ((int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10) != 0) {
      *(uint *)(&UNK_002a7b70 + lRam0000000001488590) = uVar2;
    }
    *(float *)(&UNK_00003d2c + param_1) =
         *(float *)(&UNK_00003d2c + param_1) *
         *(float *)(pbVar6 + 0x3c) * *(float *)(pbVar6 + 0x24) * fVar21;
    *(float *)(&UNK_00003dfc + param_1) =
         *(float *)(&UNK_00003dfc + param_1) *
         *(float *)(pbVar6 + 0x40) * *(float *)(pbVar6 + 0x24) * fVar21;
    *(float *)(&UNK_00003ecc + param_1) =
         *(float *)(&UNK_00003ecc + param_1) *
         *(float *)(pbVar6 + 0x44) * *(float *)(pbVar6 + 0x24) * fVar21;
    uVar2 = *(uint *)(lRam0000000001488588 + 0x1cf5b40);
    uVar2 = ((uVar2 ^ uVar2 >> 1) & 0x44400544) * 3 ^ uVar2;
    if ((int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10) != 0) {
      *(uint *)(&UNK_002a7b70 + lRam0000000001488590) = uVar2;
    }
    *(float *)(&UNK_00003d30 + param_1) =
         *(float *)(&UNK_00003d30 + param_1) *
         *(float *)(pbVar6 + 0x3c) * *(float *)(pbVar6 + 0x28) * fVar21;
    *(float *)(param_1 + 0x3e00) =
         *(float *)(param_1 + 0x3e00) *
         *(float *)(pbVar6 + 0x40) * *(float *)(pbVar6 + 0x28) * fVar21;
    *(float *)(&UNK_00003ed0 + param_1) =
         *(float *)(&UNK_00003ed0 + param_1) *
         *(float *)(pbVar6 + 0x44) * *(float *)(pbVar6 + 0x28) * fVar21;
    uVar2 = *(uint *)(lRam0000000001488588 + 0x1cf5b40);
    uVar2 = ((uVar2 ^ uVar2 >> 1) & 0x44400544) * 3 ^ uVar2;
    iRam00000000014881d0 =
         (int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10);
    if (iRam00000000014881d0 != 0) {
      *(uint *)(&UNK_002a7b70 + lRam0000000001488590) = uVar2;
    }
    *(float *)(&UNK_00003d34 + param_1) =
         *(float *)(&UNK_00003d34 + param_1) *
         *(float *)(pbVar6 + 0x3c) * *(float *)(pbVar6 + 0x2c) * fVar21;
    *(float *)(&UNK_00003e04 + param_1) =
         *(float *)(&UNK_00003e04 + param_1) *
         *(float *)(pbVar6 + 0x40) * *(float *)(pbVar6 + 0x2c) * fVar21;
    *(float *)(&UNK_00003ed4 + param_1) =
         *(float *)(&UNK_00003ed4 + param_1) *
         *(float *)(pbVar6 + 0x44) * *(float *)(pbVar6 + 0x2c) * fVar21;
  }
  else {
    lVar13 = 0;
    do {
      fVar22 = *(float *)(pbVar6 + 4);
      pfVar1 = (float *)(&UNK_00003d10 + lVar13 + param_1);
      *pfVar1 = *pfVar1 * fVar22 * *(float *)(pbVar6 + lVar13 + 8) * *(float *)(pbVar6 + 0x30);
      pfVar1[0x34] = pfVar1[0x34] *
                     fVar22 * *(float *)(pbVar6 + lVar13 + 8) * *(float *)(pbVar6 + 0x34);
      pfVar1[0x68] = pfVar1[0x68] *
                     fVar22 * *(float *)(pbVar6 + lVar13 + 8) * *(float *)(pbVar6 + 0x38);
      uVar2 = *(uint *)(lRam0000000001488588 + 0x1cf5b40);
      uVar2 = ((uVar2 ^ uVar2 >> 1) & 0x44400544) * 3 ^ uVar2;
      iRam00000000014881d0 =
           (int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10);
      if (iRam00000000014881d0 != 0) {
        *(uint *)(&UNK_002a7b70 + lRam0000000001488590) = uVar2;
      }
      pfVar1[5] = pfVar1[5] *
                  *(float *)(pbVar6 + 0x3c) * *(float *)(pbVar6 + lVar13 + 0x1c) * fVar21;
      pfVar1[0x39] = pfVar1[0x39] *
                     *(float *)(pbVar6 + 0x40) * *(float *)(pbVar6 + lVar13 + 0x1c) * fVar21;
      pfVar1[0x6d] = pfVar1[0x6d] *
                     *(float *)(pbVar6 + 0x44) * *(float *)(pbVar6 + lVar13 + 0x1c) * fVar21;
      lVar13 = lVar13 + 4;
    } while (lVar13 != 0x14);
  }
  fVar21 = (float)(byte)(bVar4 * pbVar6[0x48]);
  fVar22 = *(float *)(pbVar6 + 0x54) * *(float *)(pbVar6 + 0x4c) * fVar21;
  *(float *)(&UNK_00003d38 + param_1) = *(float *)(&UNK_00003d38 + param_1) * fVar22;
  *(float *)(&UNK_00003e08 + param_1) = fVar22 * *(float *)(&UNK_00003e08 + param_1);
  *(float *)(&UNK_00003ed8 + param_1) = fVar22 * *(float *)(&UNK_00003ed8 + param_1);
  fVar22 = *(float *)(pbVar6 + 0x50) * fVar21 * *(float *)(pbVar6 + 0x54);
  *(float *)(&UNK_00003d4c + param_1) = *(float *)(&UNK_00003d4c + param_1) * fVar22;
  *(float *)(&UNK_00003e1c + param_1) = fVar22 * *(float *)(&UNK_00003e1c + param_1);
  *(float *)(&UNK_00003eec + param_1) = fVar22 * *(float *)(&UNK_00003eec + param_1);
  fVar22 = *(float *)(pbVar6 + 0x58) * *(float *)(pbVar6 + 0x4c) * fVar21;
  *(float *)(&UNK_00003d3c + param_1) = *(float *)(&UNK_00003d3c + param_1) * fVar22;
  *(float *)(&UNK_00003e0c + param_1) = fVar22 * *(float *)(&UNK_00003e0c + param_1);
  *(float *)(&UNK_00003edc + param_1) = fVar22 * *(float *)(&UNK_00003edc + param_1);
  fVar22 = *(float *)(pbVar6 + 0x50) * fVar21 * *(float *)(pbVar6 + 0x58);
  *(float *)(&UNK_00003d50 + param_1) = *(float *)(&UNK_00003d50 + param_1) * fVar22;
  *(float *)(&UNK_00003e20 + param_1) = fVar22 * *(float *)(&UNK_00003e20 + param_1);
  *(float *)(&UNK_00003ef0 + param_1) = fVar22 * *(float *)(&UNK_00003ef0 + param_1);
  fVar22 = *(float *)(pbVar6 + 0x5c) * *(float *)(pbVar6 + 0x4c) * fVar21;
  *(float *)(&UNK_00003d40 + param_1) = *(float *)(&UNK_00003d40 + param_1) * fVar22;
  *(float *)(&UNK_00003e10 + param_1) = fVar22 * *(float *)(&UNK_00003e10 + param_1);
  *(float *)(&UNK_00003ee0 + param_1) = fVar22 * *(float *)(&UNK_00003ee0 + param_1);
  fVar21 = *(float *)(pbVar6 + 0x50) * fVar21 * *(float *)(pbVar6 + 0x5c);
  *(float *)(&UNK_00003d54 + param_1) = *(float *)(&UNK_00003d54 + param_1) * fVar21;
  *(float *)(&UNK_00003e24 + param_1) = fVar21 * *(float *)(&UNK_00003e24 + param_1);
  *(float *)(&UNK_00003ef4 + param_1) = fVar21 * *(float *)(&UNK_00003ef4 + param_1);
  bVar4 = *pbVar6;
  fVar21 = (float)(byte)(bVar4 * pbVar6[0x60]);
  *(float *)(&UNK_00003d88 + param_1) = *(float *)(pbVar6 + 100) * fVar21;
  *(float *)(&UNK_00003e58 + param_1) = *(float *)(pbVar6 + 100) * fVar21;
  *(float *)(&UNK_00003f28 + param_1) = *(float *)(pbVar6 + 100) * fVar21;
  *(float *)(&UNK_00003d8c + param_1) = *(float *)(pbVar6 + 0x68) * fVar21;
  *(float *)(&UNK_00003e5c + param_1) = *(float *)(pbVar6 + 0x68) * fVar21;
  *(float *)(&UNK_00003f2c + param_1) = *(float *)(pbVar6 + 0x68) * fVar21;
  *(float *)(&UNK_00003d90 + param_1) = *(float *)(pbVar6 + 0x6c) * fVar21;
  *(float *)(&UNK_00003e60 + param_1) = *(float *)(pbVar6 + 0x6c) * fVar21;
  *(float *)(&UNK_00003f30 + param_1) = *(float *)(pbVar6 + 0x6c) * fVar21;
  fVar21 = (float)(byte)(bVar4 * pbVar6[0x70]);
  *(float *)(&UNK_00003d60 + param_1) =
       *(float *)(&UNK_00003d60 + param_1) * *(float *)(pbVar6 + 0x74) * fVar21;
  *(undefined4 *)(&UNK_00003e30 + param_1) = 0;
  *(undefined4 *)(param_1 + 0x3f00) = 0;
  fVar22 = *(float *)(pbVar6 + 0x80) * *(float *)(pbVar6 + 0x84);
  *(float *)(&UNK_00003d74 + param_1) = *(float *)(&UNK_00003d74 + param_1) * fVar22;
  *(float *)(&UNK_00003e44 + param_1) = fVar22 * *(float *)(&UNK_00003e44 + param_1);
  *(float *)(&UNK_00003f14 + param_1) = fVar22 * *(float *)(&UNK_00003f14 + param_1);
  *(float *)(&UNK_00003d64 + param_1) =
       *(float *)(&UNK_00003d64 + param_1) * *(float *)(pbVar6 + 0x74) * fVar21;
  *(undefined4 *)(&UNK_00003e34 + param_1) = 0;
  *(undefined4 *)(&UNK_00003f04 + param_1) = 0;
  fVar22 = *(float *)(pbVar6 + 0x80) * *(float *)(pbVar6 + 0x88);
  *(float *)(&UNK_00003d78 + param_1) = *(float *)(&UNK_00003d78 + param_1) * fVar22;
  *(float *)(&UNK_00003e48 + param_1) = fVar22 * *(float *)(&UNK_00003e48 + param_1);
  *(float *)(&UNK_00003f18 + param_1) = fVar22 * *(float *)(&UNK_00003f18 + param_1);
  *(float *)(&UNK_00003d68 + param_1) =
       *(float *)(&UNK_00003d68 + param_1) * *(float *)(pbVar6 + 0x74) * fVar21;
  *(undefined4 *)(&UNK_00003e38 + param_1) = 0;
  *(undefined4 *)(&UNK_00003f08 + param_1) = 0;
  fVar21 = *(float *)(pbVar6 + 0x80) * *(float *)(pbVar6 + 0x8c);
  *(float *)(&UNK_00003d7c + param_1) = *(float *)(&UNK_00003d7c + param_1) * fVar21;
  *(float *)(&UNK_00003e4c + param_1) = fVar21 * *(float *)(&UNK_00003e4c + param_1);
  *(float *)(&UNK_00003f1c + param_1) = fVar21 * *(float *)(&UNK_00003f1c + param_1);
  fVar21 = (float)(byte)(*pbVar6 * pbVar6[0x90]);
  *(float *)(&UNK_00003da0 + param_1) =
       *(float *)(&UNK_00003da0 + param_1) *
       *(float *)(pbVar6 + 0x94) * *(float *)(pbVar6 + 0xa0) * fVar21;
  *(float *)(&UNK_00003e70 + param_1) =
       *(float *)(&UNK_00003e70 + param_1) *
       *(float *)(pbVar6 + 0xa0) * fVar21 * *(float *)(pbVar6 + 0x98);
  *(float *)(&UNK_00003f40 + param_1) =
       *(float *)(&UNK_00003f40 + param_1) *
       *(float *)(pbVar6 + 0xa0) * fVar21 * *(float *)(pbVar6 + 0x9c);
  *(undefined4 *)(&UNK_00003db4 + param_1) = *(undefined4 *)(pbVar6 + 0x154);
  *(undefined4 *)(&UNK_00003e84 + param_1) = *(undefined4 *)(pbVar6 + 0x154);
  *(undefined4 *)(&UNK_00003f54 + param_1) = *(undefined4 *)(pbVar6 + 0x154);
  *(float *)(&UNK_00003da4 + param_1) =
       *(float *)(&UNK_00003da4 + param_1) *
       *(float *)(pbVar6 + 0x94) * *(float *)(pbVar6 + 0xa4) * fVar21;
  *(float *)(&UNK_00003e74 + param_1) =
       *(float *)(&UNK_00003e74 + param_1) *
       *(float *)(pbVar6 + 0xa4) * fVar21 * *(float *)(pbVar6 + 0x98);
  *(float *)(&UNK_00003f44 + param_1) =
       *(float *)(&UNK_00003f44 + param_1) *
       *(float *)(pbVar6 + 0xa4) * fVar21 * *(float *)(pbVar6 + 0x9c);
  *(undefined4 *)(&UNK_00003db8 + param_1) = *(undefined4 *)(pbVar6 + 0x158);
  *(undefined4 *)(&UNK_00003e88 + param_1) = *(undefined4 *)(pbVar6 + 0x158);
  *(undefined4 *)(&UNK_00003f58 + param_1) = *(undefined4 *)(pbVar6 + 0x158);
  *(float *)(&UNK_00003da8 + param_1) =
       *(float *)(&UNK_00003da8 + param_1) *
       *(float *)(pbVar6 + 0x94) * *(float *)(pbVar6 + 0xa8) * fVar21;
  *(float *)(&UNK_00003e78 + param_1) =
       *(float *)(&UNK_00003e78 + param_1) *
       *(float *)(pbVar6 + 0xa8) * fVar21 * *(float *)(pbVar6 + 0x98);
  *(float *)(&UNK_00003f48 + param_1) =
       *(float *)(&UNK_00003f48 + param_1) *
       *(float *)(pbVar6 + 0xa8) * fVar21 * *(float *)(pbVar6 + 0x9c);
  *(undefined4 *)(&UNK_00003dbc + param_1) = *(undefined4 *)(pbVar6 + 0x15c);
  *(undefined4 *)(&UNK_00003e8c + param_1) = *(undefined4 *)(pbVar6 + 0x15c);
  *(undefined4 *)(&UNK_00003f5c + param_1) = *(undefined4 *)(pbVar6 + 0x15c);
  bVar4 = pbVar6[0x91];
  (&UNK_00003d9c)[param_1] = bVar4;
  (&UNK_00003e6c)[param_1] = bVar4;
  (&UNK_00003f3c)[param_1] = bVar4;
  uVar2 = *(uint *)(&UNK_0071e960 + lRam0000000001488598);
  if (iRam00000000014881d4 != 0) {
    uVar2 = uVar2 >> 8 | uVar2 << 0x18;
  }
  if ((int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10) != 0) {
    *(uint *)(lRam00000000014885a0 + 0x742c50) = uVar2;
  }
  plVar10 = (long *)(&UNK_01246c48 +
                    (long)*(int *)(pbVar6 + 0x104) * 0x18 +
                    (ulong)*(uint *)(*(long *)(&UNK_00003f90 + param_1) + 0x48) * 0x48);
  lVar13 = *plVar10;
  lVar9 = plVar10[1];
  *(long *)(&UNK_00003dd8 + param_1) = lVar13;
  *(long *)(&UNK_00003ea8 + param_1) = lVar9;
  lVar11 = plVar10[2];
  *(long *)(&UNK_00003f78 + param_1) = lVar11;
  *(float *)(&UNK_00003dc8 + param_1) = fVar18;
  *(float *)(&UNK_00003e98 + param_1) = fVar18;
  *(float *)(&UNK_00003f68 + param_1) = fVar18;
  if ((param_2 & 1) == 0) {
    if (*(float *)(pbVar6 + 0x2c) <= 0.0) {
      if (*(float *)(pbVar6 + 0x28) <= 0.0) {
        if (0.0 < *(float *)(pbVar6 + 0x24)) {
LAB_0099f62c:
          iVar12 = 2;
        }
        else {
          if (*(float *)(pbVar6 + 0x20) <= 0.0) {
            if (*(float *)(pbVar6 + 0x1c) <= 0.0) {
              lVar15 = -1;
            }
            else {
              lVar15 = 0;
            }
          }
          else {
            lVar15 = 1;
          }
          fVar18 = (float)NEON_ucvtf((uint)pbVar6[0x90]);
          lVar8 = 0x2a;
          iVar12 = 2;
          do {
            if (0.0 < *(float *)(pbVar6 + lVar8 * 4) * fVar18) {
              iVar12 = (int)lVar8 + -0x28;
              if (iVar12 < 3) goto LAB_0099f62c;
              break;
            }
            lVar14 = lVar8 + -0x29;
            lVar8 = lVar8 + -1;
          } while (lVar15 < lVar14);
        }
      }
      else {
        iVar12 = 3;
      }
    }
    else {
      iVar12 = 4;
    }
    uVar7 = (ulong)(iVar12 + 1);
  }
  else {
    uVar7 = 5;
  }
  *(ulong *)(&UNK_00003dd0 + param_1) = uVar7;
  *(ulong *)(&UNK_00003ea0 + param_1) = uVar7;
  *(ulong *)(&UNK_00003f70 + param_1) = uVar7;
  lVar8 = (uVar7 - 1) * 8;
  lVar15 = *(long *)(&UNK_01243808 + lVar8 + lVar9 * 0x28);
  if (lVar15 < *(long *)(&UNK_01243808 + lVar8 + lVar13 * 0x28)) {
    lVar15 = *(long *)(&UNK_01243808 + lVar8 + lVar13 * 0x28);
  }
  if (lVar15 <= *(long *)(&UNK_01243808 + lVar8 + lVar11 * 0x28)) {
    lVar15 = *(long *)(&UNK_01243808 + lVar8 + lVar11 * 0x28);
  }
  *(long *)(&UNK_00003f88 + param_1) = lVar15;
  lVar9 = *(long *)(&UNK_01243740 + lVar8 + lVar9 * 0x28);
  if (lVar9 < *(long *)(&UNK_01243740 + lVar8 + lVar13 * 0x28)) {
    lVar9 = *(long *)(&UNK_01243740 + lVar8 + lVar13 * 0x28);
  }
  if (lVar9 <= *(long *)(&UNK_01243740 + lVar8 + lVar11 * 0x28)) {
    lVar9 = *(long *)(&UNK_01243740 + lVar8 + lVar11 * 0x28);
  }
  *(long *)(param_1 + 0x3f80) = lVar9;
  return;
}


