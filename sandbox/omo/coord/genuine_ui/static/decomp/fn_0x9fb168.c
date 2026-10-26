// FUN_009fb168 size=1016

void FUN_009fb168(float param_1,float param_2,float param_3,float param_4,float param_5,
                 float param_6,float param_7,long param_8,long param_9,long param_10,long param_11,
                 undefined1 *param_12,ulong param_13,long param_14,ulong *param_15)

{
  undefined8 *puVar1;
  float *pfVar2;
  undefined1 auVar3 [16];
  undefined1 auVar4 [16];
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  int iVar9;
  ulong uVar10;
  long lVar11;
  undefined8 *puVar12;
  float *pfVar13;
  undefined8 *puVar14;
  float *pfVar15;
  undefined1 auVar16 [16];
  float fVar17;
  undefined8 uVar18;
  float fVar19;
  float fVar20;
  undefined8 uVar21;
  float fVar22;
  float fVar23;
  undefined8 uVar24;
  float fVar25;
  float fVar26;
  undefined8 uVar27;
  float fVar28;
  undefined1 in_q16 [16];
  undefined1 auVar29 [16];
  undefined1 auVar30 [16];
  undefined1 in_q17 [16];
  undefined1 auVar31 [16];
  undefined1 auVar32 [16];
  undefined1 in_q18 [16];
  undefined1 auVar33 [16];
  undefined1 auVar34 [16];
  float fVar35;
  float fVar41;
  float fVar42;
  float fVar43;
  undefined1 auVar36 [16];
  undefined1 auVar37 [16];
  undefined1 auVar38 [16];
  undefined1 auVar39 [16];
  undefined1 auVar40 [16];
  float fVar44;
  float fVar51;
  float fVar52;
  float fVar53;
  undefined1 auVar45 [16];
  undefined1 auVar46 [16];
  undefined1 auVar47 [16];
  undefined1 auVar48 [16];
  undefined1 auVar49 [16];
  undefined1 auVar50 [16];
  undefined1 auVar54 [16];
  undefined1 auVar55 [16];
  undefined1 auVar56 [16];
  undefined1 auVar57 [16];
  undefined1 auVar58 [16];
  undefined1 auVar59 [16];
  undefined1 auVar60 [16];
  undefined1 auVar61 [16];
  undefined1 auVar62 [16];
  undefined1 auVar63 [16];
  float fVar64;
  float fVar65;
  float fVar66;
  float fVar67;
  float fVar68;
  float fVar69;
  float fVar70;
  float fVar76;
  float fVar77;
  float fVar78;
  float fVar79;
  float fVar80;
  float fVar81;
  float fVar82;
  float fVar83;
  float fVar84;
  undefined1 auVar71 [16];
  undefined1 auVar72 [16];
  undefined1 auVar73 [16];
  undefined1 auVar74 [16];
  undefined1 auVar75 [16];
  float fVar85;
  float fVar86;
  float fVar87;
  float fVar88;
  float fVar89;
  float fVar90;
  float fVar91;
  float fVar92;
  undefined1 auVar93 [16];
  undefined1 auVar94 [16];
  undefined1 auVar95 [16];
  undefined1 auVar96 [16];
  undefined1 auVar97 [16];
  undefined1 auVar98 [16];
  undefined1 auVar99 [16];
  float fVar100;
  float fVar101;
  float fVar102;
  float fVar103;
  
  uVar10 = *param_15;
  param_13 = param_13 & 0xfffffffffffffff0;
  if (param_11 == 0) {
    if (0 < (long)param_13) {
      lVar11 = 0;
      pfVar13 = (float *)(param_8 + 0x20);
      puVar12 = (undefined8 *)(param_9 + 0x20);
      pfVar15 = (float *)(param_10 + 0x20);
      do {
        puVar14 = (undefined8 *)(param_14 + uVar10 * 4);
        uVar24 = puVar14[1];
        uVar21 = *puVar14;
        iVar9 = (int)uVar10;
        puVar14 = (undefined8 *)(param_14 + ((ulong)(iVar9 + 4) & 0xff) * 4);
        uVar18 = puVar14[1];
        fVar42 = (float)uVar18;
        fVar43 = (float)((ulong)uVar18 >> 0x20);
        uVar18 = *puVar14;
        fVar35 = (float)uVar18;
        fVar41 = (float)((ulong)uVar18 >> 0x20);
        pfVar2 = (float *)(param_14 + ((ulong)(iVar9 + 8) & 0xff) * 4);
        fVar5 = *pfVar2;
        fVar6 = pfVar2[1];
        fVar7 = pfVar2[2];
        fVar8 = pfVar2[3];
        pfVar2 = (float *)(param_14 + ((ulong)(iVar9 + 0xc) & 0xff) * 4);
        fVar17 = *pfVar2;
        fVar19 = pfVar2[1];
        fVar20 = pfVar2[2];
        fVar22 = pfVar2[3];
        fVar23 = (float)uVar21;
        fVar25 = (float)((ulong)uVar21 >> 0x20);
        fVar26 = (float)uVar24;
        fVar28 = (float)((ulong)uVar24 >> 0x20);
        auVar99._0_8_ =
             CONCAT44((int)(param_4 + param_1 * pfVar13[-7] + fVar25),
                      (int)(param_4 + param_1 * pfVar13[-8] + fVar23));
        auVar99._8_4_ = (int)(param_4 + param_1 * pfVar13[-6] + fVar26);
        auVar99._12_4_ = (int)(param_4 + param_1 * pfVar13[-5] + fVar28);
        auVar62._0_8_ =
             CONCAT44((int)(param_4 + param_1 * pfVar13[-3] + fVar41),
                      (int)(param_4 + param_1 * pfVar13[-4] + fVar35));
        auVar62._8_4_ = (int)(param_4 + param_1 * pfVar13[-2] + fVar42);
        auVar62._12_4_ = (int)(param_4 + param_1 * pfVar13[-1] + fVar43);
        auVar96._0_8_ =
             CONCAT44((int)(param_4 + param_1 * pfVar13[1] + fVar6),
                      (int)(param_4 + param_1 * *pfVar13 + fVar5));
        auVar96._8_4_ = (int)(param_4 + param_1 * pfVar13[2] + fVar7);
        auVar96._12_4_ = (int)(param_4 + param_1 * pfVar13[3] + fVar8);
        auVar39._0_8_ =
             CONCAT44((int)(param_4 + param_1 * pfVar13[5] + fVar19),
                      (int)(param_4 + param_1 * pfVar13[4] + fVar17));
        auVar39._8_4_ = (int)(param_4 + param_1 * pfVar13[6] + fVar20);
        auVar39._12_4_ = (int)(param_4 + param_1 * pfVar13[7] + fVar22);
        auVar49._0_8_ =
             CONCAT44((int)(param_5 + param_2 * *(float *)((long)puVar12 + -0x1c) + fVar25),
                      (int)(param_5 + param_2 * *(float *)(puVar12 + -4) + fVar23));
        auVar49._8_4_ = (int)(param_5 + param_2 * *(float *)(puVar12 + -3) + fVar26);
        auVar49._12_4_ = (int)(param_5 + param_2 * *(float *)((long)puVar12 + -0x14) + fVar28);
        auVar57._0_8_ =
             CONCAT44((int)(param_5 + param_2 * *(float *)((long)puVar12 + -0xc) + fVar41),
                      (int)(param_5 + param_2 * *(float *)(puVar12 + -2) + fVar35));
        auVar57._8_4_ = (int)(param_5 + param_2 * *(float *)(puVar12 + -1) + fVar42);
        auVar57._12_4_ = (int)(param_5 + param_2 * *(float *)((long)puVar12 + -4) + fVar43);
        auVar63._0_4_ = (int)(param_5 + param_2 * (float)*puVar12 + fVar5);
        auVar63._4_4_ = (int)(param_5 + param_2 * (float)((ulong)*puVar12 >> 0x20) + fVar6);
        auVar63._8_4_ = (int)(param_5 + param_2 * (float)puVar12[1] + fVar7);
        auVar63._12_4_ = (int)(param_5 + param_2 * (float)((ulong)puVar12[1] >> 0x20) + fVar8);
        auVar97._0_4_ = (int)(param_6 + param_3 * *pfVar15 + fVar5);
        auVar97._4_4_ = (int)(param_6 + param_3 * pfVar15[1] + fVar6);
        auVar97._8_4_ = (int)(param_6 + param_3 * pfVar15[2] + fVar7);
        auVar97._12_4_ = (int)(param_6 + param_3 * pfVar15[3] + fVar8);
        auVar75._0_4_ = (int)(param_6 + param_3 * pfVar15[4] + fVar17);
        auVar75._4_4_ = (int)(param_6 + param_3 * pfVar15[5] + fVar19);
        auVar75._8_4_ = (int)(param_6 + param_3 * pfVar15[6] + fVar20);
        auVar75._12_4_ = (int)(param_6 + param_3 * pfVar15[7] + fVar22);
        auVar98._8_8_ = auVar99._8_8_;
        auVar98._0_8_ = NEON_uqxtn(auVar99._0_8_,auVar99,4);
        auVar99 = NEON_uqxtn2(auVar98,auVar62,4);
        auVar74._8_8_ = auVar62._8_8_;
        auVar74._0_8_ = NEON_uqxtn(auVar62._0_8_,auVar96,4);
        auVar62 = NEON_uqxtn2(auVar74,auVar39,4);
        auVar16._8_8_ = auVar96._8_8_;
        auVar16._0_8_ = NEON_uqxtn(auVar96._0_8_,auVar49,4);
        auVar40._8_8_ = auVar39._8_8_;
        auVar40._0_8_ = NEON_uqxtn(auVar39._0_8_,auVar63,4);
        auVar74 = NEON_uqxtn2(auVar16,auVar57,4);
        auVar38._4_4_ = (int)(param_5 + param_2 * (float)((ulong)puVar12[2] >> 0x20) + fVar19);
        auVar38._0_4_ = (int)(param_5 + param_2 * (float)puVar12[2] + fVar17);
        auVar38._8_4_ = (int)(param_5 + param_2 * (float)puVar12[3] + fVar20);
        auVar38._12_4_ = (int)(param_5 + param_2 * (float)((ulong)puVar12[3] >> 0x20) + fVar22);
        auVar38 = NEON_uqxtn2(auVar40,auVar38,4);
        auVar50._8_8_ = auVar49._8_8_;
        auVar48._4_4_ = (int)(param_6 + param_3 * pfVar15[-7] + fVar25);
        auVar48._0_4_ = (int)(param_6 + param_3 * pfVar15[-8] + fVar23);
        auVar48._8_4_ = (int)(param_6 + param_3 * pfVar15[-6] + fVar26);
        auVar48._12_4_ = (int)(param_6 + param_3 * pfVar15[-5] + fVar28);
        auVar50._0_8_ = NEON_uqxtn(auVar49._0_8_,auVar48,4);
        auVar56._4_4_ =
             (int)(param_6 + param_3 * (float)((ulong)*(undefined8 *)(pfVar15 + -4) >> 0x20) +
                  fVar41);
        auVar56._0_4_ = (int)(param_6 + param_3 * (float)*(undefined8 *)(pfVar15 + -4) + fVar35);
        auVar56._8_4_ = (int)(param_6 + param_3 * (float)*(undefined8 *)(pfVar15 + -2) + fVar42);
        auVar56._12_4_ =
             (int)(param_6 + param_3 * (float)((ulong)*(undefined8 *)(pfVar15 + -2) >> 0x20) +
                  fVar43);
        auVar48 = NEON_uqxtn2(auVar50,auVar56,4);
        auVar58._8_8_ = auVar57._8_8_;
        auVar58._0_8_ = NEON_uqxtn(auVar57._0_8_,auVar97,4);
        auVar56 = NEON_uqxtn2(auVar58,auVar75,4);
        auVar30._8_8_ = in_q16._8_8_;
        auVar30._0_8_ = NEON_uqxtn(in_q16._0_8_,auVar48,2);
        in_q16 = NEON_uqxtn2(auVar30,auVar56,2);
        auVar32._8_8_ = in_q17._8_8_;
        auVar32._0_8_ = NEON_uqxtn(in_q17._0_8_,auVar74,2);
        in_q17 = NEON_uqxtn2(auVar32,auVar38,2);
        auVar34._8_8_ = in_q18._8_8_;
        auVar34._0_8_ = NEON_uqxtn(in_q18._0_8_,auVar99,2);
        in_q18 = NEON_uqxtn2(auVar34,auVar62,2);
        *param_12 = in_q16[0];
        param_12[1] = in_q17[0];
        param_12[2] = in_q18[0];
        param_12[3] = 0xff;
        param_12[4] = in_q16[1];
        param_12[5] = in_q17[1];
        param_12[6] = in_q18[1];
        param_12[7] = 0xff;
        param_12[8] = in_q16[2];
        param_12[9] = in_q17[2];
        param_12[10] = in_q18[2];
        param_12[0xb] = 0xff;
        param_12[0xc] = in_q16[3];
        param_12[0xd] = in_q17[3];
        param_12[0xe] = in_q18[3];
        param_12[0xf] = 0xff;
        param_12[0x10] = in_q16[4];
        param_12[0x11] = in_q17[4];
        param_12[0x12] = in_q18[4];
        param_12[0x13] = 0xff;
        param_12[0x14] = in_q16[5];
        param_12[0x15] = in_q17[5];
        param_12[0x16] = in_q18[5];
        param_12[0x17] = 0xff;
        param_12[0x18] = in_q16[6];
        param_12[0x19] = in_q17[6];
        param_12[0x1a] = in_q18[6];
        param_12[0x1b] = 0xff;
        param_12[0x1c] = in_q16[7];
        param_12[0x1d] = in_q17[7];
        param_12[0x1e] = in_q18[7];
        param_12[0x1f] = 0xff;
        param_12[0x20] = in_q16[8];
        param_12[0x21] = in_q17[8];
        param_12[0x22] = in_q18[8];
        param_12[0x23] = 0xff;
        param_12[0x24] = in_q16[9];
        param_12[0x25] = in_q17[9];
        param_12[0x26] = in_q18[9];
        param_12[0x27] = 0xff;
        param_12[0x28] = in_q16[10];
        param_12[0x29] = in_q17[10];
        param_12[0x2a] = in_q18[10];
        param_12[0x2b] = 0xff;
        param_12[0x2c] = in_q16[0xb];
        param_12[0x2d] = in_q17[0xb];
        param_12[0x2e] = in_q18[0xb];
        param_12[0x2f] = 0xff;
        param_12[0x30] = in_q16[0xc];
        param_12[0x31] = in_q17[0xc];
        param_12[0x32] = in_q18[0xc];
        param_12[0x33] = 0xff;
        param_12[0x34] = in_q16[0xd];
        param_12[0x35] = in_q17[0xd];
        param_12[0x36] = in_q18[0xd];
        param_12[0x37] = 0xff;
        param_12[0x38] = in_q16[0xe];
        param_12[0x39] = in_q17[0xe];
        param_12[0x3a] = in_q18[0xe];
        param_12[0x3b] = 0xff;
        param_12[0x3c] = in_q16[0xf];
        param_12[0x3d] = in_q17[0xf];
        param_12[0x3e] = in_q18[0xf];
        param_12[0x3f] = 0xff;
        param_12 = param_12 + 0x40;
        uVar10 = (ulong)(iVar9 + 0x10) & 0xff;
        lVar11 = lVar11 + 0x10;
        pfVar13 = pfVar13 + 0x10;
        puVar12 = puVar12 + 8;
        pfVar15 = pfVar15 + 0x10;
      } while (lVar11 < (long)param_13);
    }
  }
  else if (0 < (long)param_13) {
    lVar11 = 0;
    puVar12 = (undefined8 *)(param_8 + 0x20);
    puVar14 = (undefined8 *)(param_9 + 0x20);
    pfVar13 = (float *)(param_11 + 0x20);
    pfVar15 = (float *)(param_10 + 0x20);
    do {
      fVar68 = pfVar13[-8] * 0.003921569;
      fVar76 = pfVar13[-7] * 0.003921569;
      fVar79 = pfVar13[-6] * 0.003921569;
      fVar82 = pfVar13[-5] * 0.003921569;
      fVar85 = pfVar13[-4] * 0.003921569;
      fVar87 = pfVar13[-3] * 0.003921569;
      fVar89 = pfVar13[-2] * 0.003921569;
      fVar91 = pfVar13[-1] * 0.003921569;
      fVar35 = param_7 - fVar68 * param_7;
      fVar41 = param_7 - fVar76 * param_7;
      fVar42 = param_7 - fVar79 * param_7;
      fVar43 = param_7 - fVar82 * param_7;
      puVar1 = (undefined8 *)(param_14 + uVar10 * 4);
      uVar21 = puVar1[1];
      uVar18 = *puVar1;
      fVar44 = param_7 - fVar85 * param_7;
      fVar51 = param_7 - fVar87 * param_7;
      fVar52 = param_7 - fVar89 * param_7;
      fVar53 = param_7 - fVar91 * param_7;
      iVar9 = (int)uVar10;
      puVar1 = (undefined8 *)(param_14 + ((ulong)(iVar9 + 4) & 0xff) * 4);
      uVar27 = puVar1[1];
      uVar24 = *puVar1;
      fVar17 = (float)uVar18;
      fVar19 = (float)((ulong)uVar18 >> 0x20);
      fVar20 = (float)uVar21;
      fVar22 = (float)((ulong)uVar21 >> 0x20);
      fVar23 = (float)uVar24;
      fVar25 = (float)((ulong)uVar24 >> 0x20);
      fVar26 = (float)uVar27;
      fVar28 = (float)((ulong)uVar27 >> 0x20);
      fVar69 = *pfVar13 * 0.003921569;
      fVar77 = pfVar13[1] * 0.003921569;
      fVar80 = pfVar13[2] * 0.003921569;
      fVar83 = pfVar13[3] * 0.003921569;
      fVar86 = param_7 - fVar69 * param_7;
      fVar88 = param_7 - fVar77 * param_7;
      fVar90 = param_7 - fVar80 * param_7;
      fVar92 = param_7 - fVar83 * param_7;
      pfVar2 = (float *)(param_14 + ((ulong)(iVar9 + 8) & 0xff) * 4);
      fVar5 = *pfVar2;
      fVar6 = pfVar2[1];
      fVar7 = pfVar2[2];
      fVar8 = pfVar2[3];
      fVar70 = pfVar13[4] * 0.003921569;
      fVar78 = pfVar13[5] * 0.003921569;
      fVar81 = pfVar13[6] * 0.003921569;
      fVar84 = pfVar13[7] * 0.003921569;
      pfVar2 = (float *)(param_14 + ((ulong)(iVar9 + 0xc) & 0xff) * 4);
      fVar100 = *pfVar2;
      fVar101 = pfVar2[1];
      fVar102 = pfVar2[2];
      fVar103 = pfVar2[3];
      fVar64 = param_7 - fVar70 * param_7;
      fVar65 = param_7 - fVar78 * param_7;
      fVar66 = param_7 - fVar81 * param_7;
      fVar67 = param_7 - fVar84 * param_7;
      auVar54._0_8_ =
           CONCAT44((int)(fVar41 + fVar76 * (param_4 + param_1 * *(float *)((long)puVar12 + -0x1c) +
                                            fVar19)),
                    (int)(fVar35 + fVar68 * (param_4 + param_1 * *(float *)(puVar12 + -4) + fVar17))
                   );
      auVar54._8_4_ =
           (int)(fVar42 + fVar79 * (param_4 + param_1 * *(float *)(puVar12 + -3) + fVar20));
      auVar54._12_4_ =
           (int)(fVar43 + fVar82 * (param_4 + param_1 * *(float *)((long)puVar12 + -0x14) + fVar22))
      ;
      auVar59._0_4_ =
           (int)(fVar44 + fVar85 * (param_4 + param_1 * *(float *)(puVar12 + -2) + fVar23));
      auVar59._4_4_ =
           (int)(fVar51 + fVar87 * (param_4 + param_1 * *(float *)((long)puVar12 + -0xc) + fVar25));
      auVar59._8_4_ =
           (int)(fVar52 + fVar89 * (param_4 + param_1 * *(float *)(puVar12 + -1) + fVar26));
      auVar59._12_4_ =
           (int)(fVar53 + fVar91 * (param_4 + param_1 * *(float *)((long)puVar12 + -4) + fVar28));
      auVar55._8_8_ = auVar54._8_8_;
      auVar55._0_8_ = NEON_uqxtn(auVar54._0_8_,auVar54,4);
      auVar56 = NEON_uqxtn2(auVar55,auVar59,4);
      auVar60._0_8_ =
           CONCAT44((int)(fVar88 + fVar77 * (param_4 + param_1 * (float)((ulong)*puVar12 >> 0x20) +
                                            fVar6)),
                    (int)(fVar86 + fVar69 * (param_4 + param_1 * (float)*puVar12 + fVar5)));
      auVar60._8_4_ = (int)(fVar90 + fVar80 * (param_4 + param_1 * (float)puVar12[1] + fVar7));
      auVar60._12_4_ =
           (int)(fVar92 + fVar83 * (param_4 + param_1 * (float)((ulong)puVar12[1] >> 0x20) + fVar8))
      ;
      auVar71._0_4_ =
           (int)(fVar64 + fVar70 * (param_4 + param_1 * *(float *)(puVar12 + 2) + fVar100));
      auVar71._4_4_ =
           (int)(fVar65 + fVar78 * (param_4 + param_1 * *(float *)((long)puVar12 + 0x14) + fVar101))
      ;
      auVar71._8_4_ =
           (int)(fVar66 + fVar81 * (param_4 + param_1 * *(float *)(puVar12 + 3) + fVar102));
      auVar71._12_4_ =
           (int)(fVar67 + fVar84 * (param_4 + param_1 * *(float *)((long)puVar12 + 0x1c) + fVar103))
      ;
      auVar61._8_8_ = auVar60._8_8_;
      auVar61._0_8_ = NEON_uqxtn(auVar60._0_8_,auVar60,4);
      auVar62 = NEON_uqxtn2(auVar61,auVar71,4);
      auVar72._0_8_ =
           CONCAT44((int)(fVar41 + fVar76 * (param_5 + param_2 * *(float *)((long)puVar14 + -0x1c) +
                                            fVar19)),
                    (int)(fVar35 + fVar68 * (param_5 + param_2 * *(float *)(puVar14 + -4) + fVar17))
                   );
      auVar72._8_4_ =
           (int)(fVar42 + fVar79 * (param_5 + param_2 * *(float *)(puVar14 + -3) + fVar20));
      auVar72._12_4_ =
           (int)(fVar43 + fVar82 * (param_5 + param_2 * *(float *)((long)puVar14 + -0x14) + fVar22))
      ;
      auVar93._0_4_ = (int)(fVar44 + fVar85 * (param_5 + param_2 * (float)puVar14[-2] + fVar23));
      auVar93._4_4_ =
           (int)(fVar51 + fVar87 * (param_5 + param_2 * (float)((ulong)puVar14[-2] >> 0x20) + fVar25
                                   ));
      auVar93._8_4_ = (int)(fVar52 + fVar89 * (param_5 + param_2 * (float)puVar14[-1] + fVar26));
      auVar93._12_4_ =
           (int)(fVar53 + fVar91 * (param_5 + param_2 * (float)((ulong)puVar14[-1] >> 0x20) + fVar28
                                   ));
      auVar73._8_8_ = auVar72._8_8_;
      auVar73._0_8_ = NEON_uqxtn(auVar72._0_8_,auVar72,4);
      auVar74 = NEON_uqxtn2(auVar73,auVar93,4);
      auVar94._0_8_ =
           CONCAT44((int)(fVar88 + fVar77 * (param_5 + param_2 * (float)((ulong)*puVar14 >> 0x20) +
                                            fVar6)),
                    (int)(fVar86 + fVar69 * (param_5 + param_2 * (float)*puVar14 + fVar5)));
      auVar94._8_4_ = (int)(fVar90 + fVar80 * (param_5 + param_2 * (float)puVar14[1] + fVar7));
      auVar94._12_4_ =
           (int)(fVar92 + fVar83 * (param_5 + param_2 * (float)((ulong)puVar14[1] >> 0x20) + fVar8))
      ;
      auVar95._8_8_ = auVar94._8_8_;
      auVar95._0_8_ = NEON_uqxtn(auVar94._0_8_,auVar94,4);
      auVar4._4_4_ = (int)(fVar65 + fVar78 * (param_5 + param_2 * *(float *)((long)puVar14 + 0x14) +
                                             fVar101));
      auVar4._0_4_ = (int)(fVar64 + fVar70 * (param_5 + param_2 * *(float *)(puVar14 + 2) + fVar100)
                          );
      auVar4._8_4_ = (int)(fVar66 + fVar81 * (param_5 + param_2 * *(float *)(puVar14 + 3) + fVar102)
                          );
      auVar4._12_4_ =
           (int)(fVar67 + fVar84 * (param_5 + param_2 * *(float *)((long)puVar14 + 0x1c) + fVar103))
      ;
      auVar96 = NEON_uqxtn2(auVar95,auVar4,4);
      auVar36._0_8_ =
           CONCAT44((int)(fVar41 + fVar76 * (param_6 + param_3 * (float)((ulong)*(undefined8 *)
                                                                                 (pfVar15 + -8) >>
                                                                        0x20) + fVar19)),
                    (int)(fVar35 + fVar68 * (param_6 + param_3 * (float)*(undefined8 *)
                                                                         (pfVar15 + -8) + fVar17)));
      auVar36._8_4_ =
           (int)(fVar42 + fVar79 * (param_6 + param_3 * (float)*(undefined8 *)(pfVar15 + -6) +
                                   fVar20));
      auVar36._12_4_ =
           (int)(fVar43 + fVar82 * (param_6 + param_3 * (float)((ulong)*(undefined8 *)(pfVar15 + -6)
                                                               >> 0x20) + fVar22));
      auVar45._0_4_ =
           (int)(fVar44 + fVar85 * (param_6 + param_3 * (float)*(undefined8 *)(pfVar15 + -4) +
                                   fVar23));
      auVar45._4_4_ =
           (int)(fVar51 + fVar87 * (param_6 + param_3 * (float)((ulong)*(undefined8 *)(pfVar15 + -4)
                                                               >> 0x20) + fVar25));
      auVar45._8_4_ =
           (int)(fVar52 + fVar89 * (param_6 + param_3 * (float)*(undefined8 *)(pfVar15 + -2) +
                                   fVar26));
      auVar45._12_4_ =
           (int)(fVar53 + fVar91 * (param_6 + param_3 * (float)((ulong)*(undefined8 *)(pfVar15 + -2)
                                                               >> 0x20) + fVar28));
      auVar37._8_8_ = auVar36._8_8_;
      auVar37._0_8_ = NEON_uqxtn(auVar36._0_8_,auVar36,4);
      auVar38 = NEON_uqxtn2(auVar37,auVar45,4);
      auVar46._0_8_ =
           CONCAT44((int)(fVar88 + fVar77 * (param_6 + param_3 * pfVar15[1] + fVar6)),
                    (int)(fVar86 + fVar69 * (param_6 + param_3 * *pfVar15 + fVar5)));
      auVar46._8_4_ = (int)(fVar90 + fVar80 * (param_6 + param_3 * pfVar15[2] + fVar7));
      auVar46._12_4_ = (int)(fVar92 + fVar83 * (param_6 + param_3 * pfVar15[3] + fVar8));
      auVar47._8_8_ = auVar46._8_8_;
      auVar47._0_8_ = NEON_uqxtn(auVar46._0_8_,auVar46,4);
      auVar3._4_4_ = (int)(fVar65 + fVar78 * (param_6 + param_3 * (float)((ulong)*(undefined8 *)
                                                                                  (pfVar15 + 4) >>
                                                                         0x20) + fVar101));
      auVar3._0_4_ = (int)(fVar64 + fVar70 * (param_6 + param_3 * (float)*(undefined8 *)
                                                                          (pfVar15 + 4) + fVar100));
      auVar3._8_4_ = (int)(fVar66 + fVar81 * (param_6 + param_3 * (float)*(undefined8 *)
                                                                          (pfVar15 + 6) + fVar102));
      auVar3._12_4_ =
           (int)(fVar67 + fVar84 * (param_6 + param_3 * (float)((ulong)*(undefined8 *)(pfVar15 + 6)
                                                               >> 0x20) + fVar103));
      auVar48 = NEON_uqxtn2(auVar47,auVar3,4);
      auVar29._8_8_ = in_q16._8_8_;
      auVar29._0_8_ = NEON_uqxtn(in_q16._0_8_,auVar38,2);
      in_q16 = NEON_uqxtn2(auVar29,auVar48,2);
      auVar31._8_8_ = in_q17._8_8_;
      auVar31._0_8_ = NEON_uqxtn(in_q17._0_8_,auVar74,2);
      in_q17 = NEON_uqxtn2(auVar31,auVar96,2);
      auVar33._8_8_ = in_q18._8_8_;
      auVar33._0_8_ = NEON_uqxtn(in_q18._0_8_,auVar56,2);
      in_q18 = NEON_uqxtn2(auVar33,auVar62,2);
      uVar10 = (ulong)(iVar9 + 0x10) & 0xff;
      *param_12 = in_q16[0];
      param_12[1] = in_q17[0];
      param_12[2] = in_q18[0];
      param_12[3] = 0xff;
      param_12[4] = in_q16[1];
      param_12[5] = in_q17[1];
      param_12[6] = in_q18[1];
      param_12[7] = 0xff;
      param_12[8] = in_q16[2];
      param_12[9] = in_q17[2];
      param_12[10] = in_q18[2];
      param_12[0xb] = 0xff;
      param_12[0xc] = in_q16[3];
      param_12[0xd] = in_q17[3];
      param_12[0xe] = in_q18[3];
      param_12[0xf] = 0xff;
      param_12[0x10] = in_q16[4];
      param_12[0x11] = in_q17[4];
      param_12[0x12] = in_q18[4];
      param_12[0x13] = 0xff;
      param_12[0x14] = in_q16[5];
      param_12[0x15] = in_q17[5];
      param_12[0x16] = in_q18[5];
      param_12[0x17] = 0xff;
      param_12[0x18] = in_q16[6];
      param_12[0x19] = in_q17[6];
      param_12[0x1a] = in_q18[6];
      param_12[0x1b] = 0xff;
      param_12[0x1c] = in_q16[7];
      param_12[0x1d] = in_q17[7];
      param_12[0x1e] = in_q18[7];
      param_12[0x1f] = 0xff;
      param_12[0x20] = in_q16[8];
      param_12[0x21] = in_q17[8];
      param_12[0x22] = in_q18[8];
      param_12[0x23] = 0xff;
      param_12[0x24] = in_q16[9];
      param_12[0x25] = in_q17[9];
      param_12[0x26] = in_q18[9];
      param_12[0x27] = 0xff;
      param_12[0x28] = in_q16[10];
      param_12[0x29] = in_q17[10];
      param_12[0x2a] = in_q18[10];
      param_12[0x2b] = 0xff;
      param_12[0x2c] = in_q16[0xb];
      param_12[0x2d] = in_q17[0xb];
      param_12[0x2e] = in_q18[0xb];
      param_12[0x2f] = 0xff;
      param_12[0x30] = in_q16[0xc];
      param_12[0x31] = in_q17[0xc];
      param_12[0x32] = in_q18[0xc];
      param_12[0x33] = 0xff;
      param_12[0x34] = in_q16[0xd];
      param_12[0x35] = in_q17[0xd];
      param_12[0x36] = in_q18[0xd];
      param_12[0x37] = 0xff;
      param_12[0x38] = in_q16[0xe];
      param_12[0x39] = in_q17[0xe];
      param_12[0x3a] = in_q18[0xe];
      param_12[0x3b] = 0xff;
      param_12[0x3c] = in_q16[0xf];
      param_12[0x3d] = in_q17[0xf];
      param_12[0x3e] = in_q18[0xf];
      param_12[0x3f] = 0xff;
      param_12 = param_12 + 0x40;
      lVar11 = lVar11 + 0x10;
      puVar12 = puVar12 + 8;
      puVar14 = puVar14 + 8;
      pfVar13 = pfVar13 + 0x10;
      pfVar15 = pfVar15 + 0x10;
    } while (lVar11 < (long)param_13);
  }
  *param_15 = uVar10;
  return;
}


