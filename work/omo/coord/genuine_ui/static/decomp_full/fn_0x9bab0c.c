// FUN_009bab0c size=2260

void FUN_009bab0c(long param_1,float *param_2,float *param_3,undefined8 *param_4,float *param_5,
                 float *param_6,float *param_7)

{
  long lVar1;
  float *pfVar2;
  float *pfVar3;
  uint uVar4;
  uint uVar5;
  long lVar6;
  long lVar7;
  long lVar8;
  float *pfVar9;
  long lVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  float fVar18;
  float fVar19;
  float fVar20;
  float fVar21;
  float fVar22;
  float fVar23;
  float fVar24;
  float fVar25;
  float local_ac;
  float local_a8;
  float local_a4;
  float local_a0;
  float local_9c;
  float fStack_98;
  float local_94;
  float local_90;
  float afStack_8c [3];
  
  *param_2 = 0.0;
  *param_3 = 0.0;
  *param_5 = 0.0;
  *param_6 = 0.0;
  *param_7 = 0.0;
  *param_4 = 0;
  param_4[1] = 0;
  *(undefined4 *)(param_4 + 2) = 0;
  uVar4 = *(uint *)(DAT_01488fb8 + 0x2cde58);
  if (DAT_014883a4 != 0) {
    uVar4 = uVar4 >> 8 | uVar4 << 0x18;
  }
  if (uVar4 != 1) {
    *(uint *)(&PNG_00ce5d0a.field_0x952de + DAT_01488fc0) = uVar4;
  }
  lVar7 = 0;
  lVar8 = param_1 + 0xbf48;
  fVar11 = 0.0;
  while( true ) {
    lVar6 = lVar7 * 4;
    lVar1 = param_1 + 0x48 + lVar6;
    pfVar9 = (float *)(lVar1 + 4);
    lVar10 = param_1 + lVar6;
    fVar13 = *pfVar9 * *(float *)(lVar10 + 0xbf48) * *(float *)(param_1 + 0xbf44);
    fVar15 = *(float *)(&DAT_00001f58 + lVar1) * *(float *)(lVar10 + 0xc068) *
             *(float *)(param_1 + 0xc064);
    fVar16 = *(float *)(lVar1 + 0x3eac) * *(float *)(lVar10 + 0xc188) * *(float *)(param_1 + 0xc184)
    ;
    fVar11 = fVar11 + (fVar13 * fVar13 + fVar15 * fVar15 + fVar16 * fVar16) / 9.0;
    *(float *)((long)param_4 + lVar6) = fVar11;
    fVar13 = *pfVar9 * *(float *)(lVar10 + 49000) * *(float *)(param_1 + 0xbf64);
    fVar15 = *(float *)(&DAT_00001f58 + lVar1) * *(float *)(lVar10 + 0xc088) *
             *(float *)(param_1 + 0xc084);
    fVar16 = *(float *)(lVar1 + 0x3eac) * *(float *)(lVar10 + 0xc1a8) * *(float *)(param_1 + 0xc1a4)
    ;
    fVar11 = fVar11 + (fVar13 * fVar13 + fVar15 * fVar15 + fVar16 * fVar16) / 9.0;
    *(float *)((long)param_4 + lVar6) = fVar11;
    fVar13 = *pfVar9 * *(float *)(lVar10 + 0xbf88) * *(float *)(param_1 + 0xbf84);
    fVar15 = *(float *)(&DAT_00001f58 + lVar1) * *(float *)(lVar10 + 0xc0a8) *
             *(float *)(param_1 + 0xc0a4);
    fVar16 = *(float *)(lVar1 + 0x3eac) * *(float *)(lVar10 + 0xc1c8) * *(float *)(param_1 + 0xc1c4)
    ;
    fVar11 = fVar11 + (fVar13 * fVar13 + fVar15 * fVar15 + fVar16 * fVar16) / 9.0;
    *(float *)((long)param_4 + lVar6) = fVar11;
    fVar13 = *pfVar9 * *(float *)(lVar10 + 0xbfa8) * *(float *)(param_1 + 0xbfa4);
    fVar15 = *(float *)(&DAT_00001f58 + lVar1) * *(float *)(lVar10 + 0xc0c8) *
             *(float *)(param_1 + 0xc0c4);
    fVar16 = *(float *)(lVar1 + 0x3eac) * *(float *)(lVar10 + 0xc1e8) * *(float *)(param_1 + 0xc1e4)
    ;
    fVar11 = fVar11 + (fVar13 * fVar13 + fVar15 * fVar15 + fVar16 * fVar16) / 9.0;
    *(float *)((long)param_4 + lVar6) = fVar11;
    fVar13 = *pfVar9 * *(float *)(lVar10 + 0xbfc8) * *(float *)(param_1 + 0xbfc4);
    fVar15 = *(float *)(&DAT_00001f58 + lVar1) * *(float *)(lVar10 + 0xc0e8) *
             *(float *)(FUN_0000c0e4 + param_1);
    fVar16 = *(float *)(lVar1 + 0x3eac) * *(float *)(lVar10 + 0xc208) * *(float *)(param_1 + 0xc204)
    ;
    fVar11 = fVar11 + (fVar13 * fVar13 + fVar15 * fVar15 + fVar16 * fVar16) / 9.0;
    *(float *)((long)param_4 + lVar6) = fVar11;
    fVar13 = *pfVar9 * *(float *)(lVar10 + 0xbfe8) * *(float *)(param_1 + 0xbfe4);
    fVar15 = *(float *)(&DAT_00001f58 + lVar1) * *(float *)(lVar10 + 0xc108) *
             *(float *)(param_1 + 0xc104);
    fVar16 = *(float *)(lVar1 + 0x3eac) * *(float *)(lVar10 + 0xc228) * *(float *)(param_1 + 0xc224)
    ;
    fVar11 = fVar11 + (fVar13 * fVar13 + fVar15 * fVar15 + fVar16 * fVar16) / 9.0;
    *(float *)((long)param_4 + lVar6) = fVar11;
    fVar13 = *pfVar9 * *(float *)(&DAT_0000c008 + lVar10) * *(float *)(&DAT_0000c004 + param_1);
    fVar15 = *(float *)(&DAT_00001f58 + lVar1) * *(float *)(lVar10 + 0xc128) *
             *(float *)(param_1 + 0xc124);
    fVar16 = *(float *)(lVar1 + 0x3eac) * *(float *)(lVar10 + 0xc248) * *(float *)(param_1 + 0xc244)
    ;
    fVar11 = fVar11 + (fVar13 * fVar13 + fVar15 * fVar15 + fVar16 * fVar16) / 9.0;
    *(float *)((long)param_4 + lVar6) = fVar11;
    fVar13 = *pfVar9 * *(float *)(&DAT_0000c028 + lVar10) * *(float *)(&DAT_0000c024 + param_1);
    fVar15 = *(float *)(&DAT_00001f58 + lVar1) * *(float *)(lVar10 + 0xc148) *
             *(float *)(param_1 + 0xc144);
    fVar16 = *(float *)(lVar1 + 0x3eac) * *(float *)(lVar10 + 0xc268) * *(float *)(param_1 + 0xc264)
    ;
    fVar11 = fVar11 + (fVar13 * fVar13 + fVar15 * fVar15 + fVar16 * fVar16) / 9.0;
    *(float *)((long)param_4 + lVar6) = fVar11;
    fVar13 = *pfVar9 * *(float *)(lVar10 + 0xc048) * *(float *)(param_1 + 0xc044);
    fVar15 = *(float *)(&DAT_00001f58 + lVar1) * *(float *)(lVar10 + 0xc168) *
             *(float *)(param_1 + 0xc164);
    fVar16 = *(float *)(lVar1 + 0x3eac) * *(float *)(lVar10 + 0xc288) * *(float *)(param_1 + 0xc284)
    ;
    fVar11 = fVar11 + (fVar13 * fVar13 + fVar15 * fVar15 + fVar16 * fVar16) / 9.0;
    *(float *)((long)param_4 + lVar6) = fVar11;
    *param_2 = fVar11 + *param_2;
    *(float *)((long)param_4 + lVar6) = SQRT(*(float *)((long)param_4 + lVar6));
    *param_3 = *param_3 +
               *pfVar9 * *pfVar9 +
               *(float *)(&DAT_00001f58 + lVar1) * *(float *)(&DAT_00001f58 + lVar1) +
               *(float *)(lVar1 + 0x3eac) * *(float *)(lVar1 + 0x3eac);
    (**(code **)(&DAT_0147df80 + (ulong)*(uint *)(param_1 + 0x48) * 8))
              (afStack_8c,&local_90,&local_94,&fStack_98,&local_9c,&local_a0,&local_a4,&local_a8,
               &local_ac);
    fVar21 = *pfVar9 * *pfVar9;
    fVar22 = *(float *)(&DAT_00001f58 + lVar1) * *(float *)(&DAT_00001f58 + lVar1);
    fVar23 = *(float *)(lVar1 + 0x3eac) * *(float *)(lVar1 + 0x3eac);
    fVar11 = *(float *)(lVar1 + 0x5e00);
    fVar15 = *(float *)(lVar1 + 0x7d54);
    fVar13 = *(float *)(lVar1 + 0x9ca8);
    fVar16 = fVar21 * afStack_8c[0];
    fVar18 = fVar22 * local_90;
    fVar19 = fVar23 * local_94;
    fVar20 = fVar21 * fStack_98;
    fVar24 = fVar22 * local_9c;
    fVar25 = fVar23 * local_a0;
    fVar21 = fVar21 * local_a4;
    fVar22 = fVar22 * local_a8;
    fVar23 = fVar23 * local_ac;
    uVar5 = *(uint *)(&PNG_00ce5d0a.field_0x1344e6 + DAT_01488fc8);
    uVar4 = (uVar5 ^ uVar5 >> 1) & 0x45104044;
    uVar5 = (uVar4 | uVar4 << 1) ^ uVar5;
    if (uVar5 != 0) {
      *(uint *)(&UNK_003f6860 + DAT_01488fd0) = uVar5;
    }
    lVar10 = 0;
    DAT_014883a8 = uVar5;
    do {
      (**(code **)(&DAT_0147df80 + (ulong)*(uint *)(param_1 + 0x48) * 8))
                (afStack_8c,&local_90,&local_94,&fStack_98,&local_9c,&local_a0,&local_a4,&local_a8,
                 &local_ac);
      pfVar2 = (float *)(lVar8 + lVar10);
      pfVar3 = (float *)(param_1 + 0xbf44 + lVar10);
      fVar12 = *pfVar9 * *pfVar2 * *pfVar3;
      fVar14 = *(float *)(&DAT_00001f58 + lVar1) * pfVar2[0x48] * pfVar3[0x48];
      fVar12 = fVar12 * fVar12;
      fVar17 = *(float *)(lVar1 + 0x3eac) * pfVar2[0x90] * pfVar3[0x90];
      fVar14 = fVar14 * fVar14;
      fVar17 = fVar17 * fVar17;
      *param_5 = (((fVar11 * fVar11) / (fVar16 + fVar18 + fVar19)) *
                 (fVar12 * afStack_8c[0] + fVar14 * local_90 + fVar17 * local_94)) / 9.0 + *param_5;
      *param_6 = (((fVar15 * fVar15) / (fVar20 + fVar24 + fVar25)) *
                 (fVar12 * fStack_98 + fVar14 * local_9c + fVar17 * local_a0)) / 9.0 + *param_6;
      *param_7 = (((fVar13 * fVar13) / (fVar21 + fVar22 + fVar23)) *
                 (fVar12 * local_a4 + fVar14 * local_a8 + fVar17 * local_ac)) / 9.0 + *param_7;
      lVar10 = lVar10 + 0x20;
    } while (lVar10 != 0x120);
    lVar7 = lVar7 + 1;
    if (lVar7 == 5) break;
    fVar11 = *(float *)((long)param_4 + lVar7 * 4);
    lVar8 = lVar8 + 4;
  }
  *param_5 = SQRT(*param_5);
  *param_6 = SQRT(*param_6);
  *param_7 = SQRT(*param_7);
  *param_2 = SQRT(*param_2);
  *param_3 = SQRT(*param_3);
  if (uVar5 != 0) {
    uVar4 = *(uint *)(
                     "QNativeSocketEngine::waitForWrite() was called in QAbstractSocket::UnconnectedState"
                     + DAT_01488fd8 + 0x2e);
    uVar5 = *(uint *)(DAT_01488fe8 + 0x1880378);
    if ((int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10) != 1) {
      uVar5 = (uVar4 ^ uVar4 >> 1) & 0x45014145;
      *(uint *)(DAT_01488fe0 + 0x6440d8) = (uVar5 | uVar5 << 1) ^ uVar4;
    }
  }
  fVar11 = *param_5;
  fVar13 = SQRT(fVar11 * fVar11 + *param_6 * *param_6 + *param_7 * *param_7);
  if (fVar13 != 0.0) {
    fVar13 = *param_2 / fVar13;
    *param_5 = fVar11 * fVar13;
    *param_6 = fVar13 * *param_6;
    *param_7 = fVar13 * *param_7;
  }
  return;
}


