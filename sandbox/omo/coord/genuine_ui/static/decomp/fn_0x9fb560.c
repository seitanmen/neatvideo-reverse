// FUN_009fb560 size=616

void FUN_009fb560(float param_1,float param_2,long param_3,long param_4,long param_5,long param_6,
                 undefined1 *param_7,ulong param_8)

{
  undefined1 auVar1 [16];
  undefined1 auVar2 [16];
  long lVar3;
  float *pfVar4;
  undefined8 *puVar5;
  undefined8 *puVar6;
  float *pfVar7;
  float fVar8;
  float fVar11;
  float fVar12;
  float fVar13;
  undefined1 auVar9 [16];
  undefined1 auVar10 [16];
  float fVar14;
  float fVar17;
  float fVar18;
  float fVar19;
  undefined1 auVar15 [16];
  undefined1 auVar16 [16];
  undefined1 auVar20 [16];
  float fVar21;
  float fVar23;
  float fVar24;
  undefined1 auVar22 [16];
  float fVar25;
  undefined1 auVar26 [16];
  undefined1 in_q20 [16];
  undefined1 auVar27 [16];
  undefined1 in_q21 [16];
  undefined1 auVar28 [16];
  undefined1 in_q22 [16];
  undefined1 auVar29 [16];
  float fVar30;
  float fVar36;
  float fVar37;
  undefined1 auVar31 [16];
  undefined1 auVar32 [16];
  float fVar38;
  undefined1 auVar33 [16];
  undefined1 auVar34 [16];
  undefined1 auVar35 [16];
  float fVar39;
  float fVar45;
  float fVar46;
  undefined1 auVar40 [16];
  undefined1 auVar41 [16];
  float fVar47;
  undefined1 auVar42 [16];
  undefined1 auVar43 [16];
  undefined1 auVar44 [16];
  float fVar48;
  float fVar53;
  float fVar54;
  undefined1 auVar49 [16];
  undefined1 auVar50 [16];
  float fVar55;
  undefined1 auVar51 [16];
  undefined1 auVar52 [16];
  float fVar56;
  float fVar61;
  float fVar62;
  float fVar63;
  undefined1 auVar57 [16];
  undefined1 auVar58 [16];
  undefined1 auVar59 [16];
  undefined1 auVar60 [16];
  float fVar64;
  float fVar65;
  float fVar66;
  float fVar67;
  float fVar68;
  float fVar69;
  float fVar70;
  float fVar71;
  float fVar72;
  float fVar73;
  float fVar74;
  float fVar75;
  float fVar76;
  float fVar79;
  float fVar80;
  undefined1 auVar77 [16];
  undefined1 auVar78 [16];
  float fVar81;
  
  if (0 < (long)(param_8 & 0xfffffffffffffff0)) {
    lVar3 = 0;
    param_1 = param_1 / 3.0;
    pfVar4 = (float *)(param_3 + 0x20);
    puVar5 = (undefined8 *)(param_4 + 0x20);
    puVar6 = (undefined8 *)(param_6 + 0x20);
    pfVar7 = (float *)(param_5 + 0x20);
    auVar20 = NEON_fmov(0x3f800000,4);
    auVar22 = NEON_fmov(0xbe800000,4);
    auVar26 = NEON_fmov(0x3f400000,4);
    do {
      auVar31._0_4_ =
           param_2 + param_1 * (pfVar4[-4] + (float)puVar5[-2] + (float)*(undefined8 *)(pfVar7 + -4)
                               );
      auVar31._4_4_ =
           param_2 + param_1 * (pfVar4[-3] + (float)((ulong)puVar5[-2] >> 0x20) +
                               (float)((ulong)*(undefined8 *)(pfVar7 + -4) >> 0x20));
      auVar31._8_4_ =
           param_2 + param_1 * (pfVar4[-2] + (float)puVar5[-1] + (float)*(undefined8 *)(pfVar7 + -2)
                               );
      auVar31._12_4_ =
           param_2 + param_1 * (pfVar4[-1] + (float)((ulong)puVar5[-1] >> 0x20) +
                               (float)((ulong)*(undefined8 *)(pfVar7 + -2) >> 0x20));
      auVar40._0_4_ = param_2 + param_1 * (*pfVar4 + (float)*puVar5 + *pfVar7);
      auVar40._4_4_ = param_2 + param_1 * (pfVar4[1] + (float)((ulong)*puVar5 >> 0x20) + pfVar7[1]);
      auVar40._8_4_ = param_2 + param_1 * (pfVar4[2] + (float)puVar5[1] + pfVar7[2]);
      auVar40._12_4_ =
           param_2 + param_1 * (pfVar4[3] + (float)((ulong)puVar5[1] >> 0x20) + pfVar7[3]);
      auVar77._0_4_ =
           param_2 + param_1 * (pfVar4[4] + *(float *)(puVar5 + 2) +
                               (float)*(undefined8 *)(pfVar7 + 4));
      auVar77._4_4_ =
           param_2 + param_1 * (pfVar4[5] + *(float *)((long)puVar5 + 0x14) +
                               (float)((ulong)*(undefined8 *)(pfVar7 + 4) >> 0x20));
      auVar77._8_4_ =
           param_2 + param_1 * (pfVar4[6] + *(float *)(puVar5 + 3) +
                               (float)*(undefined8 *)(pfVar7 + 6));
      auVar77._12_4_ =
           param_2 + param_1 * (pfVar4[7] + *(float *)((long)puVar5 + 0x1c) +
                               (float)((ulong)*(undefined8 *)(pfVar7 + 6) >> 0x20));
      auVar2._4_4_ = param_2 + param_1 * (pfVar4[-7] + (float)((ulong)puVar5[-4] >> 0x20) +
                                         pfVar7[-7]);
      auVar2._0_4_ = param_2 + param_1 * (pfVar4[-8] + (float)puVar5[-4] + pfVar7[-8]);
      auVar2._8_4_ = param_2 + param_1 * (pfVar4[-6] + (float)puVar5[-3] + pfVar7[-6]);
      auVar2._12_4_ =
           param_2 + param_1 * (pfVar4[-5] + (float)((ulong)puVar5[-3] >> 0x20) + pfVar7[-5]);
      auVar49 = NEON_fmax(auVar2,ZEXT816(0),4);
      auVar32 = NEON_fmax(auVar31,ZEXT816(0),4);
      fVar56 = (float)puVar6[-4] * 0.003921569;
      fVar61 = (float)((ulong)puVar6[-4] >> 0x20) * 0.003921569;
      fVar62 = (float)puVar6[-3] * 0.003921569;
      fVar63 = (float)((ulong)puVar6[-3] >> 0x20) * 0.003921569;
      fVar72 = (float)puVar6[-2] * 0.003921569;
      fVar73 = (float)((ulong)puVar6[-2] >> 0x20) * 0.003921569;
      fVar74 = (float)puVar6[-1] * 0.003921569;
      fVar75 = (float)((ulong)puVar6[-1] >> 0x20) * 0.003921569;
      auVar41 = NEON_fmax(auVar40,ZEXT816(0),4);
      fVar64 = (float)*puVar6 * 0.003921569;
      fVar65 = (float)((ulong)*puVar6 >> 0x20) * 0.003921569;
      fVar66 = (float)puVar6[1] * 0.003921569;
      fVar67 = (float)((ulong)puVar6[1] >> 0x20) * 0.003921569;
      fVar68 = (float)puVar6[2] * 0.003921569;
      fVar69 = (float)((ulong)puVar6[2] >> 0x20) * 0.003921569;
      fVar70 = (float)puVar6[3] * 0.003921569;
      fVar71 = (float)((ulong)puVar6[3] >> 0x20) * 0.003921569;
      fVar8 = auVar20._0_4_;
      fVar11 = auVar20._4_4_;
      fVar12 = auVar20._8_4_;
      fVar13 = auVar20._12_4_;
      auVar78 = NEON_fmax(auVar77,ZEXT816(0),4);
      fVar21 = auVar22._0_4_;
      fVar23 = auVar22._4_4_;
      fVar24 = auVar22._8_4_;
      fVar25 = auVar22._12_4_;
      fVar14 = auVar26._0_4_;
      fVar17 = auVar26._4_4_;
      fVar18 = auVar26._8_4_;
      fVar19 = auVar26._12_4_;
      auVar50._8_4_ = 0x437f0000;
      auVar50._0_8_ = 0x437f0000437f0000;
      auVar50._12_4_ = 0x437f0000;
      auVar50 = NEON_fmin(auVar49,auVar50,4);
      fVar48 = auVar50._0_4_;
      fVar53 = auVar50._4_4_;
      fVar54 = auVar50._8_4_;
      fVar55 = auVar50._12_4_;
      auVar49._8_4_ = 0x437f0000;
      auVar49._0_8_ = 0x437f0000437f0000;
      auVar49._12_4_ = 0x437f0000;
      auVar50 = NEON_fmin(auVar32,auVar49,4);
      fVar30 = auVar50._0_4_;
      fVar36 = auVar50._4_4_;
      fVar37 = auVar50._8_4_;
      fVar38 = auVar50._12_4_;
      auVar32._8_4_ = 0x437f0000;
      auVar32._0_8_ = 0x437f0000437f0000;
      auVar32._12_4_ = 0x437f0000;
      auVar50 = NEON_fmin(auVar41,auVar32,4);
      fVar39 = auVar50._0_4_;
      fVar45 = auVar50._4_4_;
      fVar46 = auVar50._8_4_;
      fVar47 = auVar50._12_4_;
      auVar41._8_4_ = 0x437f0000;
      auVar41._0_8_ = 0x437f0000437f0000;
      auVar41._12_4_ = 0x437f0000;
      auVar50 = NEON_fmin(auVar78,auVar41,4);
      fVar76 = auVar50._0_4_;
      fVar79 = auVar50._4_4_;
      fVar80 = auVar50._8_4_;
      fVar81 = auVar50._12_4_;
      auVar9._0_8_ = CONCAT44((int)((fVar11 + fVar61 * -0.7) * fVar53),
                              (int)((fVar8 + fVar56 * -0.7) * fVar48));
      auVar9._8_4_ = (int)((fVar12 + fVar62 * -0.7) * fVar54);
      auVar9._12_4_ = (int)((fVar13 + fVar63 * -0.7) * fVar55);
      auVar10._8_8_ = auVar9._8_8_;
      auVar10._0_8_ = NEON_uqxtn(auVar9._0_8_,auVar9,4);
      auVar60._4_4_ = (int)((fVar11 + fVar73 * -0.7) * fVar36);
      auVar60._0_4_ = (int)((fVar8 + fVar72 * -0.7) * fVar30);
      auVar60._8_4_ = (int)((fVar12 + fVar74 * -0.7) * fVar37);
      auVar60._12_4_ = (int)((fVar13 + fVar75 * -0.7) * fVar38);
      auVar50 = NEON_uqxtn2(auVar10,auVar60,4);
      auVar15._0_8_ =
           CONCAT44((int)((fVar11 + fVar65 * -0.7) * fVar45),(int)((fVar8 + fVar64 * -0.7) * fVar39)
                   );
      auVar15._8_4_ = (int)((fVar12 + fVar66 * -0.7) * fVar46);
      auVar15._12_4_ = (int)((fVar13 + fVar67 * -0.7) * fVar47);
      auVar16._8_8_ = auVar15._8_8_;
      auVar16._0_8_ = NEON_uqxtn(auVar15._0_8_,auVar15,4);
      auVar78._4_4_ = (int)((fVar11 + fVar69 * -0.7) * fVar79);
      auVar78._0_4_ = (int)((fVar8 + fVar68 * -0.7) * fVar76);
      auVar78._8_4_ = (int)((fVar12 + fVar70 * -0.7) * fVar80);
      auVar78._12_4_ = (int)((fVar13 + fVar71 * -0.7) * fVar81);
      auVar49 = NEON_uqxtn2(auVar16,auVar78,4);
      auVar51._0_8_ =
           CONCAT44((int)((fVar61 * -0.45 + 0.9) * fVar53),(int)((fVar56 * -0.45 + 0.9) * fVar48));
      auVar51._8_4_ = (int)((fVar62 * -0.45 + 0.9) * fVar54);
      auVar51._12_4_ = (int)((fVar63 * -0.45 + 0.9) * fVar55);
      auVar33._0_4_ = (int)((fVar72 * -0.45 + 0.9) * fVar30);
      auVar33._4_4_ = (int)((fVar73 * -0.45 + 0.9) * fVar36);
      auVar33._8_4_ = (int)((fVar74 * -0.45 + 0.9) * fVar37);
      auVar33._12_4_ = (int)((fVar75 * -0.45 + 0.9) * fVar38);
      auVar52._8_8_ = auVar51._8_8_;
      auVar52._0_8_ = NEON_uqxtn(auVar51._0_8_,auVar51,4);
      auVar78 = NEON_uqxtn2(auVar52,auVar33,4);
      auVar34._0_8_ =
           CONCAT44((int)((fVar65 * -0.45 + 0.9) * fVar45),(int)((fVar64 * -0.45 + 0.9) * fVar39));
      auVar34._8_4_ = (int)((fVar66 * -0.45 + 0.9) * fVar46);
      auVar34._12_4_ = (int)((fVar67 * -0.45 + 0.9) * fVar47);
      auVar42._0_4_ = (int)((fVar68 * -0.45 + 0.9) * fVar76);
      auVar42._4_4_ = (int)((fVar69 * -0.45 + 0.9) * fVar79);
      auVar42._8_4_ = (int)((fVar70 * -0.45 + 0.9) * fVar80);
      auVar42._12_4_ = (int)((fVar71 * -0.45 + 0.9) * fVar81);
      auVar35._8_8_ = auVar34._8_8_;
      auVar35._0_8_ = NEON_uqxtn(auVar34._0_8_,auVar34,4);
      auVar32 = NEON_uqxtn2(auVar35,auVar42,4);
      auVar43._0_8_ =
           CONCAT44((int)(fVar61 * 127.0 + (fVar17 + fVar23 * fVar61) * fVar53),
                    (int)(fVar56 * 127.0 + (fVar14 + fVar21 * fVar56) * fVar48));
      auVar43._8_4_ = (int)(fVar62 * 127.0 + (fVar18 + fVar24 * fVar62) * fVar54);
      auVar43._12_4_ = (int)(fVar63 * 127.0 + (fVar19 + fVar25 * fVar63) * fVar55);
      auVar44._8_8_ = auVar43._8_8_;
      auVar44._0_8_ = NEON_uqxtn(auVar43._0_8_,auVar43,4);
      auVar57._0_4_ = (int)(fVar72 * 127.0 + (fVar14 + fVar21 * fVar72) * fVar30);
      auVar57._4_4_ = (int)(fVar73 * 127.0 + (fVar17 + fVar23 * fVar73) * fVar36);
      auVar57._8_4_ = (int)(fVar74 * 127.0 + (fVar18 + fVar24 * fVar74) * fVar37);
      auVar57._12_4_ = (int)(fVar75 * 127.0 + (fVar19 + fVar25 * fVar75) * fVar38);
      auVar41 = NEON_uqxtn2(auVar44,auVar57,4);
      auVar58._0_8_ =
           CONCAT44((int)(fVar65 * 127.0 + (fVar17 + fVar23 * fVar65) * fVar45),
                    (int)(fVar64 * 127.0 + (fVar14 + fVar21 * fVar64) * fVar39));
      auVar58._8_4_ = (int)(fVar66 * 127.0 + (fVar18 + fVar24 * fVar66) * fVar46);
      auVar58._12_4_ = (int)(fVar67 * 127.0 + (fVar19 + fVar25 * fVar67) * fVar47);
      auVar59._8_8_ = auVar58._8_8_;
      auVar59._0_8_ = NEON_uqxtn(auVar58._0_8_,auVar58,4);
      auVar1._4_4_ = (int)(fVar69 * 127.0 + (fVar17 + fVar23 * fVar69) * fVar79);
      auVar1._0_4_ = (int)(fVar68 * 127.0 + (fVar14 + fVar21 * fVar68) * fVar76);
      auVar1._8_4_ = (int)(fVar70 * 127.0 + (fVar18 + fVar24 * fVar70) * fVar80);
      auVar1._12_4_ = (int)(fVar71 * 127.0 + (fVar19 + fVar25 * fVar71) * fVar81);
      auVar60 = NEON_uqxtn2(auVar59,auVar1,4);
      auVar27._8_8_ = in_q20._8_8_;
      auVar27._0_8_ = NEON_uqxtn(in_q20._0_8_,auVar41,2);
      in_q20 = NEON_uqxtn2(auVar27,auVar60,2);
      auVar28._8_8_ = in_q21._8_8_;
      auVar28._0_8_ = NEON_uqxtn(in_q21._0_8_,auVar78,2);
      in_q21 = NEON_uqxtn2(auVar28,auVar32,2);
      auVar29._8_8_ = in_q22._8_8_;
      auVar29._0_8_ = NEON_uqxtn(in_q22._0_8_,auVar50,2);
      in_q22 = NEON_uqxtn2(auVar29,auVar49,2);
      *param_7 = in_q20[0];
      param_7[1] = in_q21[0];
      param_7[2] = in_q22[0];
      param_7[3] = 0xff;
      param_7[4] = in_q20[1];
      param_7[5] = in_q21[1];
      param_7[6] = in_q22[1];
      param_7[7] = 0xff;
      param_7[8] = in_q20[2];
      param_7[9] = in_q21[2];
      param_7[10] = in_q22[2];
      param_7[0xb] = 0xff;
      param_7[0xc] = in_q20[3];
      param_7[0xd] = in_q21[3];
      param_7[0xe] = in_q22[3];
      param_7[0xf] = 0xff;
      param_7[0x10] = in_q20[4];
      param_7[0x11] = in_q21[4];
      param_7[0x12] = in_q22[4];
      param_7[0x13] = 0xff;
      param_7[0x14] = in_q20[5];
      param_7[0x15] = in_q21[5];
      param_7[0x16] = in_q22[5];
      param_7[0x17] = 0xff;
      param_7[0x18] = in_q20[6];
      param_7[0x19] = in_q21[6];
      param_7[0x1a] = in_q22[6];
      param_7[0x1b] = 0xff;
      param_7[0x1c] = in_q20[7];
      param_7[0x1d] = in_q21[7];
      param_7[0x1e] = in_q22[7];
      param_7[0x1f] = 0xff;
      param_7[0x20] = in_q20[8];
      param_7[0x21] = in_q21[8];
      param_7[0x22] = in_q22[8];
      param_7[0x23] = 0xff;
      param_7[0x24] = in_q20[9];
      param_7[0x25] = in_q21[9];
      param_7[0x26] = in_q22[9];
      param_7[0x27] = 0xff;
      param_7[0x28] = in_q20[10];
      param_7[0x29] = in_q21[10];
      param_7[0x2a] = in_q22[10];
      param_7[0x2b] = 0xff;
      param_7[0x2c] = in_q20[0xb];
      param_7[0x2d] = in_q21[0xb];
      param_7[0x2e] = in_q22[0xb];
      param_7[0x2f] = 0xff;
      param_7[0x30] = in_q20[0xc];
      param_7[0x31] = in_q21[0xc];
      param_7[0x32] = in_q22[0xc];
      param_7[0x33] = 0xff;
      param_7[0x34] = in_q20[0xd];
      param_7[0x35] = in_q21[0xd];
      param_7[0x36] = in_q22[0xd];
      param_7[0x37] = 0xff;
      param_7[0x38] = in_q20[0xe];
      param_7[0x39] = in_q21[0xe];
      param_7[0x3a] = in_q22[0xe];
      param_7[0x3b] = 0xff;
      param_7[0x3c] = in_q20[0xf];
      param_7[0x3d] = in_q21[0xf];
      param_7[0x3e] = in_q22[0xf];
      param_7[0x3f] = 0xff;
      param_7 = param_7 + 0x40;
      lVar3 = lVar3 + 0x10;
      pfVar4 = pfVar4 + 0x10;
      puVar5 = puVar5 + 8;
      puVar6 = puVar6 + 8;
      pfVar7 = pfVar7 + 0x10;
    } while (lVar3 < (long)(param_8 & 0xfffffffffffffff0));
  }
  return;
}


