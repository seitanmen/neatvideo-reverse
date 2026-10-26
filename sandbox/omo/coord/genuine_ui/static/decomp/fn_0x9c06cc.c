// FUN_009c06cc size=3388

/* WARNING: Possible PIC construction at 0x009c0954: Changing call to branch */
/* WARNING: Possible PIC construction at 0x009c0b20: Changing call to branch */
/* WARNING: Possible PIC construction at 0x009c0ab4: Changing call to branch */
/* WARNING: Possible PIC construction at 0x009c0a48: Changing call to branch */
/* WARNING: Possible PIC construction at 0x009c0d0c: Changing call to branch */
/* WARNING: Possible PIC construction at 0x009c0c90: Changing call to branch */
/* WARNING: Possible PIC construction at 0x009c09d0: Changing call to branch */
/* WARNING: Possible PIC construction at 0x009c0c20: Changing call to branch */
/* WARNING: Possible PIC construction at 0x009c0ba4: Changing call to branch */
/* WARNING: Removing unreachable block (ram,0x009c0c24) */
/* WARNING: Removing unreachable block (ram,0x009c0c38) */
/* WARNING: Removing unreachable block (ram,0x009c09d4) */
/* WARNING: Removing unreachable block (ram,0x009c09e8) */
/* WARNING: Removing unreachable block (ram,0x009c0c94) */
/* WARNING: Removing unreachable block (ram,0x009c0ca8) */
/* WARNING: Removing unreachable block (ram,0x009c0d10) */
/* WARNING: Removing unreachable block (ram,0x009c0d24) */
/* WARNING: Removing unreachable block (ram,0x009c0a4c) */
/* WARNING: Removing unreachable block (ram,0x009c0a60) */
/* WARNING: Removing unreachable block (ram,0x009c0ab8) */
/* WARNING: Removing unreachable block (ram,0x009c0acc) */
/* WARNING: Removing unreachable block (ram,0x009c0b24) */
/* WARNING: Removing unreachable block (ram,0x009c0b38) */
/* WARNING: Removing unreachable block (ram,0x009c0958) */
/* WARNING: Removing unreachable block (ram,0x009c096c) */
/* WARNING: Removing unreachable block (ram,0x009c0ba8) */
/* WARNING: Removing unreachable block (ram,0x009c0bbc) */
/* WARNING: Removing unreachable block (ram,0x009f05e8) */

void FUN_009c06cc(undefined1 (*param_1) [16],int param_2,ulong param_3,float *param_4,int param_5,
                 ulong param_6,ulong param_7,long param_8,int param_9)

{
  undefined1 *puVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint3 uVar5;
  byte bVar6;
  undefined1 uVar7;
  undefined1 uVar8;
  undefined1 uVar9;
  byte bVar10;
  undefined1 uVar11;
  undefined1 uVar12;
  undefined1 uVar13;
  byte *pbVar14;
  ulong uVar15;
  long lVar16;
  undefined8 *puVar17;
  float *pfVar18;
  short *psVar19;
  ulong uVar20;
  ulong uVar21;
  undefined1 (*pauVar22) [16];
  undefined1 (*pauVar23) [16];
  ulong uVar24;
  long lVar25;
  undefined8 uVar26;
  undefined1 auVar27 [16];
  undefined1 auVar28 [16];
  undefined1 auVar29 [16];
  undefined1 auVar30 [16];
  short sVar31;
  float fVar32;
  undefined8 uVar33;
  short sVar44;
  short sVar45;
  short sVar46;
  short sVar47;
  short sVar48;
  short sVar49;
  undefined1 auVar34 [16];
  undefined1 auVar35 [16];
  undefined1 auVar36 [16];
  undefined1 auVar37 [16];
  undefined1 auVar38 [16];
  undefined1 auVar39 [16];
  undefined1 auVar40 [16];
  undefined1 auVar41 [16];
  undefined1 auVar42 [16];
  undefined1 auVar43 [16];
  short sVar50;
  undefined1 auVar51 [16];
  undefined1 auVar52 [16];
  undefined1 auVar53 [16];
  undefined1 auVar54 [16];
  undefined1 auVar55 [16];
  undefined1 auVar56 [16];
  undefined1 auVar57 [16];
  
  if (param_2 == 2) {
    if (param_5 != 2) {
      if (param_5 == 1) {
        if ((param_7 * 4 - param_3 == 0) && (param_7 * 2 - param_6 == 0)) {
          param_7 = param_8 * param_7;
        }
        else {
          if (param_8 < 1) {
            return;
          }
          if (param_3 + param_7 * -4 < 0x80) {
            do {
              func_0x009f0554(param_1,param_4,param_7);
              param_1 = (undefined1 (*) [16])(*param_1 + param_3);
              param_4 = (float *)((long)param_4 + param_6);
              param_8 = param_8 + -1;
            } while (0 < param_8);
            return;
          }
          puVar1 = *param_1 + param_3;
          Hint_Prefetch(puVar1,0,0,0);
          Hint_Prefetch((long)puVar1 + 0x40,0,0,0);
          Hint_Prefetch((long)puVar1 + 0x80,0,0,0);
          Hint_Prefetch((long)puVar1 + 0xc0,0,0,0);
        }
        if (cRam00000000014a67a8 == '\0') {
          uVar15 = 0;
          if ((long)param_7 < 1) {
            return;
          }
        }
        else {
          uVar15 = param_7 & 0xfffffffffffffff8;
          if (0 < (long)uVar15) {
            auVar35 = NEON_fmov(0x41800000,4);
            pauVar23 = param_1;
            pfVar18 = param_4;
            do {
              pauVar22 = pauVar23 + 2;
              auVar52._0_8_ =
                   CONCAT44((int)(*(float *)(*pauVar23 + 4) * auVar35._4_4_),
                            (int)(*(float *)*pauVar23 * auVar35._0_4_));
              auVar52._8_4_ = (int)(*(float *)(*pauVar23 + 8) * auVar35._8_4_);
              auVar52._12_4_ = (int)(*(float *)(*pauVar23 + 0xc) * auVar35._12_4_);
              auVar54._0_4_ = (int)(*(float *)pauVar23[1] * auVar35._0_4_);
              auVar54._4_4_ = (int)(*(float *)(pauVar23[1] + 4) * auVar35._4_4_);
              auVar54._8_4_ = (int)(*(float *)(pauVar23[1] + 8) * auVar35._8_4_);
              auVar54._12_4_ = (int)(*(float *)(pauVar23[1] + 0xc) * auVar35._12_4_);
              auVar39._8_8_ = auVar52._8_8_;
              auVar39._0_8_ = NEON_sqxtn(auVar52._0_8_,auVar52,4);
              auVar52 = NEON_sqxtn2(auVar39,auVar54,4);
              *(long *)(pfVar18 + 2) = auVar52._8_8_;
              *(long *)pfVar18 = auVar52._0_8_;
              pauVar23 = pauVar22;
              pfVar18 = pfVar18 + 4;
            } while (pauVar22 < (undefined1 (*) [16])(*param_1 + uVar15 * 4));
          }
          if ((long)param_7 <= (long)uVar15) {
            return;
          }
        }
        lVar16 = param_7 - uVar15;
        pfVar18 = (float *)(*param_1 + uVar15 * 4);
        psVar19 = (short *)((long)param_4 + uVar15 * 2);
        do {
          fVar32 = -32768.0;
          if (-2048.0 <= *pfVar18) {
            fVar32 = *pfVar18 * 16.0;
          }
          *psVar19 = (short)(long)(float)(int)fVar32;
          lVar16 = lVar16 + -1;
          pfVar18 = pfVar18 + 1;
          psVar19 = psVar19 + 1;
        } while (lVar16 != 0);
        return;
      }
      if (param_5 != 0) {
        return;
      }
      if ((param_7 == param_6) && (param_7 * 4 - param_3 == 0)) {
        param_7 = param_8 * param_7;
      }
      else {
        if (param_8 < 1) {
          return;
        }
        if (param_3 + param_7 * -4 < 0x80) {
          do {
            func_0x009f02b4(param_1,param_4,param_7);
            param_1 = (undefined1 (*) [16])(*param_1 + param_3);
            param_4 = (float *)((long)param_4 + param_6);
            param_8 = param_8 + -1;
          } while (0 < param_8);
          return;
        }
        puVar1 = *param_1 + param_3;
        Hint_Prefetch(puVar1,0,0,0);
        Hint_Prefetch((long)puVar1 + 0x40,0,0,0);
        Hint_Prefetch((long)puVar1 + 0x80,0,0,0);
        Hint_Prefetch((long)puVar1 + 0xc0,0,0,0);
      }
      if (cRam00000000014a67a8 == '\0') {
        uVar15 = 0;
        if ((long)param_7 < 1) {
          return;
        }
      }
      else {
        uVar15 = param_7 & 0xfffffffffffffff0;
        if (0 < (long)uVar15) {
          pauVar23 = param_1;
          pfVar18 = param_4;
          do {
            auVar28._0_8_ = CONCAT44((int)*(float *)(*pauVar23 + 4),(int)*(float *)*pauVar23);
            auVar28._8_4_ = (int)*(float *)(*pauVar23 + 8);
            auVar28._12_4_ = (int)*(float *)(*pauVar23 + 0xc);
            auVar37._0_8_ = CONCAT44((int)*(float *)(pauVar23[1] + 4),(int)*(float *)pauVar23[1]);
            auVar37._8_4_ = (int)*(float *)(pauVar23[1] + 8);
            auVar37._12_4_ = (int)*(float *)(pauVar23[1] + 0xc);
            auVar53._0_4_ = (int)*(float *)pauVar23[2];
            auVar53._4_4_ = (int)*(float *)(pauVar23[2] + 4);
            auVar53._8_4_ = (int)*(float *)(pauVar23[2] + 8);
            auVar53._12_4_ = (int)*(float *)(pauVar23[2] + 0xc);
            iVar2 = (int)(float)((ulong)*(undefined8 *)pauVar23[3] >> 0x20);
            iVar3 = (int)(float)*(undefined8 *)(pauVar23[3] + 8);
            iVar4 = (int)(float)((ulong)*(undefined8 *)(pauVar23[3] + 8) >> 0x20);
            auVar29._8_8_ = auVar28._8_8_;
            auVar29._0_8_ = NEON_uqxtn(auVar28._0_8_,auVar28,4);
            auVar52 = NEON_uqxtn2(auVar29,auVar37,4);
            auVar38._8_8_ = auVar37._8_8_;
            auVar38._0_8_ = NEON_uqxtn(auVar37._0_8_,auVar53,4);
            auVar35[4] = (char)iVar2;
            auVar35._0_4_ = (int)(float)*(undefined8 *)pauVar23[3];
            auVar35[5] = (char)((uint)iVar2 >> 8);
            auVar35[6] = (char)((uint)iVar2 >> 0x10);
            auVar35[7] = (char)((uint)iVar2 >> 0x18);
            auVar35[8] = (char)iVar3;
            auVar35[9] = (char)((uint)iVar3 >> 8);
            auVar35[10] = (char)((uint)iVar3 >> 0x10);
            auVar35[0xb] = (char)((uint)iVar3 >> 0x18);
            auVar35[0xc] = (char)iVar4;
            auVar35[0xd] = (char)((uint)iVar4 >> 8);
            auVar35[0xe] = (char)((uint)iVar4 >> 0x10);
            auVar35[0xf] = (char)((uint)iVar4 >> 0x18);
            auVar35 = NEON_uqxtn2(auVar38,auVar35,4);
            auVar30._8_8_ = auVar52._8_8_;
            auVar30._0_8_ = NEON_uqxtn(auVar52._0_8_,auVar52,2);
            auVar35 = NEON_uqxtn2(auVar30,auVar35,2);
            *(long *)(pfVar18 + 2) = auVar35._8_8_;
            *(long *)pfVar18 = auVar35._0_8_;
            pauVar23 = pauVar23 + 4;
            pfVar18 = pfVar18 + 4;
          } while (pauVar23 < (undefined1 (*) [16])(*param_1 + uVar15 * 4));
        }
        if ((long)param_7 <= (long)uVar15) {
          return;
        }
      }
      lVar16 = param_7 - uVar15;
      pfVar18 = (float *)(*param_1 + uVar15 * 4);
      pbVar14 = (byte *)((long)param_4 + uVar15);
      do {
        lVar25 = (long)(float)(int)*pfVar18;
        if (0xfe < lVar25) {
          lVar25 = 0xff;
        }
        *pbVar14 = (byte)lVar25 & ((byte)(lVar25 >> 0x3f) ^ 0xff);
        lVar16 = lVar16 + -1;
        pfVar18 = pfVar18 + 1;
        pbVar14 = pbVar14 + 1;
      } while (lVar16 != 0);
      return;
    }
    uVar15 = param_7 * 4;
    if ((param_3 != param_6) || (uVar15 - param_3 != 0)) {
      if (param_8 < 1) {
        return;
      }
      if (((long)(param_3 + param_7 * -4) < 0x80) || (param_9 == 2)) {
        do {
          func_0x009f14e4(param_4,param_1,uVar15,param_9);
          param_1 = (undefined1 (*) [16])(*param_1 + param_3);
          param_4 = (float *)((long)param_4 + param_6);
          param_8 = param_8 + -1;
        } while (0 < param_8);
        return;
      }
      puVar1 = *param_1 + param_3;
      Hint_Prefetch(puVar1,0,0,0);
      Hint_Prefetch((long)puVar1 + 0x40,0,0,0);
      Hint_Prefetch((long)puVar1 + 0x80,0,0,0);
      Hint_Prefetch((long)puVar1 + 0xc0,0,0,0);
      goto SUB_009f14e4;
    }
LAB_009c090c:
    uVar15 = param_8 * param_3;
  }
  else if (param_2 == 1) {
    if (param_5 == 2) {
      if ((param_7 * 2 - param_3 == 0) && (param_7 * 4 - param_6 == 0)) {
        param_7 = param_8 * param_7;
      }
      else {
        if (param_8 < 1) {
          return;
        }
        if (param_3 + param_7 * -2 < 0x40) {
          do {
            func_0x009f0618(param_1,param_4,param_7);
            param_1 = (undefined1 (*) [16])(*param_1 + param_3);
            param_4 = (float *)((long)param_4 + param_6);
            param_8 = param_8 + -1;
          } while (0 < param_8);
          return;
        }
        Hint_Prefetch(*param_1 + param_3,0,0,0);
        Hint_Prefetch((long)(*param_1 + param_3) + 0x40,0,0,0);
      }
      if (cRam00000000014a67a8 == '\0') {
        uVar15 = 0;
        if ((long)param_7 < 1) {
          return;
        }
      }
      else {
        uVar15 = param_7 & 0xfffffffffffffff8;
        if (0 < (long)uVar15) {
          pauVar23 = param_1;
          pfVar18 = param_4;
          do {
            pauVar22 = pauVar23 + 1;
            auVar55._0_4_ = (int)*(short *)*pauVar23;
            auVar55._4_4_ = (int)*(short *)(*pauVar23 + 2);
            auVar55._8_4_ = (int)*(short *)(*pauVar23 + 4);
            auVar55._12_4_ = (int)*(short *)(*pauVar23 + 6);
            auVar52 = NEON_scvtf(auVar55,4);
            auVar40._0_4_ = (int)*(short *)(*pauVar23 + 8);
            auVar40._4_4_ = (int)*(short *)(*pauVar23 + 10);
            auVar40._8_4_ = (int)*(short *)(*pauVar23 + 0xc);
            auVar40._12_4_ = (int)*(short *)(*pauVar23 + 0xe);
            auVar35 = NEON_scvtf(auVar40,4);
            pfVar18[2] = auVar52._8_4_ * 0.0625;
            pfVar18[3] = auVar52._12_4_ * 0.0625;
            *pfVar18 = auVar52._0_4_ * 0.0625;
            pfVar18[1] = auVar52._4_4_ * 0.0625;
            pfVar18[6] = auVar35._8_4_ * 0.0625;
            pfVar18[7] = auVar35._12_4_ * 0.0625;
            pfVar18[4] = auVar35._0_4_ * 0.0625;
            pfVar18[5] = auVar35._4_4_ * 0.0625;
            pauVar23 = pauVar22;
            pfVar18 = pfVar18 + 8;
          } while (pauVar22 < (undefined1 (*) [16])(*param_1 + uVar15 * 2));
        }
        if ((long)param_7 <= (long)uVar15) {
          return;
        }
      }
      uVar20 = param_7 - uVar15;
      if (7 < uVar20) {
        uVar24 = uVar20 & 0xfffffffffffffff8;
        puVar17 = (undefined8 *)(*param_1 + uVar15 * 2 + 8);
        pfVar18 = param_4 + uVar15 + 4;
        uVar21 = uVar24;
        do {
          uVar26 = puVar17[-1];
          uVar33 = *puVar17;
          auVar41._0_4_ = (int)(short)uVar26;
          auVar41._4_4_ = (int)(short)((ulong)uVar26 >> 0x10);
          auVar41._8_4_ = (int)(short)((ulong)uVar26 >> 0x20);
          auVar41._12_4_ = (int)(short)((ulong)uVar26 >> 0x30);
          auVar35 = NEON_scvtf(auVar41,4);
          auVar56._0_4_ = (int)(short)uVar33;
          auVar56._4_4_ = (int)(short)((ulong)uVar33 >> 0x10);
          auVar56._8_4_ = (int)(short)((ulong)uVar33 >> 0x20);
          auVar56._12_4_ = (int)(short)((ulong)uVar33 >> 0x30);
          auVar52 = NEON_scvtf(auVar56,4);
          pfVar18[-2] = auVar35._8_4_ * 0.0625;
          pfVar18[-1] = auVar35._12_4_ * 0.0625;
          pfVar18[-4] = auVar35._0_4_ * 0.0625;
          pfVar18[-3] = auVar35._4_4_ * 0.0625;
          pfVar18[2] = auVar52._8_4_ * 0.0625;
          pfVar18[3] = auVar52._12_4_ * 0.0625;
          *pfVar18 = auVar52._0_4_ * 0.0625;
          pfVar18[1] = auVar52._4_4_ * 0.0625;
          puVar17 = puVar17 + 2;
          pfVar18 = pfVar18 + 8;
          uVar21 = uVar21 - 8;
        } while (uVar21 != 0);
        uVar15 = uVar15 + uVar24;
        if (uVar20 == uVar24) {
          return;
        }
      }
      lVar16 = param_7 - uVar15;
      psVar19 = (short *)(*param_1 + uVar15 * 2);
      pfVar18 = param_4 + uVar15;
      do {
        *pfVar18 = (float)(int)*psVar19 * 0.0625;
        lVar16 = lVar16 + -1;
        psVar19 = psVar19 + 1;
        pfVar18 = pfVar18 + 1;
      } while (lVar16 != 0);
      return;
    }
    if (param_5 != 1) {
      if (param_5 != 0) {
        return;
      }
      if ((param_7 == param_6) && (param_7 * 2 - param_3 == 0)) {
        param_7 = param_8 * param_7;
      }
      else {
        if (param_8 < 1) {
          return;
        }
        if (param_3 + param_7 * -2 < 0x80) {
          do {
            func_0x009f0724(param_1,param_4,param_7);
            param_1 = (undefined1 (*) [16])(*param_1 + param_3);
            param_4 = (float *)((long)param_4 + param_6);
            param_8 = param_8 + -1;
          } while (0 < param_8);
          return;
        }
        puVar1 = *param_1 + param_3;
        Hint_Prefetch(puVar1,0,0,0);
        Hint_Prefetch((long)puVar1 + 0x40,0,0,0);
        Hint_Prefetch((long)puVar1 + 0x80,0,0,0);
        Hint_Prefetch((long)puVar1 + 0xc0,0,0,0);
      }
      if (cRam00000000014a67a8 == '\0') {
        uVar15 = 0;
        if ((long)param_7 < 1) {
          return;
        }
      }
      else {
        uVar15 = param_7 & 0xfffffffffffffff0;
        if (0 < (long)uVar15) {
          pauVar23 = param_1;
          pfVar18 = param_4;
          do {
            pauVar22 = pauVar23 + 2;
            auVar35 = NEON_smax(*pauVar23,ZEXT816(0),2);
            auVar52 = NEON_smax(pauVar23[1],ZEXT816(0),2);
            auVar42._0_8_ =
                 CONCAT26(auVar35._6_2_ >> 4,
                          CONCAT24(auVar35._4_2_ >> 4,
                                   CONCAT22(auVar35._2_2_ >> 4,auVar35._0_2_ >> 4)));
            auVar42._8_2_ = auVar35._8_2_ >> 4;
            auVar42._10_2_ = auVar35._10_2_ >> 4;
            auVar42._12_2_ = auVar35._12_2_ >> 4;
            auVar42._14_2_ = auVar35._14_2_ >> 4;
            auVar57._0_2_ = auVar52._0_2_ >> 4;
            auVar57._2_2_ = auVar52._2_2_ >> 4;
            auVar57._4_2_ = auVar52._4_2_ >> 4;
            auVar57._6_2_ = auVar52._6_2_ >> 4;
            auVar57._8_2_ = auVar52._8_2_ >> 4;
            auVar57._10_2_ = auVar52._10_2_ >> 4;
            auVar57._12_2_ = auVar52._12_2_ >> 4;
            auVar57._14_2_ = auVar52._14_2_ >> 4;
            auVar43._8_8_ = auVar42._8_8_;
            auVar43._0_8_ = NEON_uqxtn(auVar42._0_8_,auVar42,2);
            auVar35 = NEON_uqxtn2(auVar43,auVar57,2);
            *(long *)(pfVar18 + 2) = auVar35._8_8_;
            *(long *)pfVar18 = auVar35._0_8_;
            pauVar23 = pauVar22;
            pfVar18 = pfVar18 + 4;
          } while (pauVar22 < (undefined1 (*) [16])(*param_1 + uVar15 * 2));
        }
        if ((long)param_7 <= (long)uVar15) {
          return;
        }
      }
      uVar20 = param_7 - uVar15;
      if (7 < uVar20) {
        puVar17 = (undefined8 *)((long)param_4 + uVar15);
        lVar16 = uVar15 * 2;
        if ((*param_1 + param_7 * 2 <= puVar17) ||
           ((short *)((long)param_4 + param_7) <= *param_1 + lVar16)) {
          uVar24 = uVar20 & 0xfffffffffffffff8;
          uVar15 = uVar15 + uVar24;
          psVar19 = (short *)(*param_1 + lVar16);
          uVar21 = uVar24;
          do {
            sVar31 = *psVar19 >> 4;
            sVar44 = psVar19[1] >> 4;
            sVar45 = psVar19[2] >> 4;
            sVar46 = psVar19[3] >> 4;
            sVar47 = psVar19[4] >> 4;
            sVar48 = psVar19[5] >> 4;
            sVar49 = psVar19[6] >> 4;
            sVar50 = psVar19[7] >> 4;
            *puVar17 = CONCAT17(0xffU - (char)((int)sVar50 >> 7) & (byte)sVar50 |
                                0xffU - (char)((long)(int)sVar50 + -0xff >> 0x3f),
                                CONCAT16(0xffU - (char)((int)sVar49 >> 7) & (byte)sVar49 |
                                         0xffU - (char)((long)(int)sVar49 + -0xff >> 0x3f),
                                         CONCAT15(0xffU - (char)((int)sVar48 >> 7) & (byte)sVar48 |
                                                  0xffU - (char)((long)(int)sVar48 + -0xff >> 0x3f),
                                                  CONCAT14(0xffU - (char)((int)sVar47 >> 7) &
                                                           (byte)sVar47 |
                                                           0xffU - (char)((long)(int)sVar47 + -0xff
                                                                         >> 0x3f),
                                                           CONCAT13(0xffU - (char)((int)sVar46 >> 7)
                                                                    & (byte)sVar46 |
                                                                    0xffU - (char)((long)(int)sVar46
                                                                                   + -0xff >> 0x3f),
                                                                    CONCAT12(0xffU - (char)((int)
                                                  sVar45 >> 7) & (byte)sVar45 |
                                                  0xffU - (char)((long)(int)sVar45 + -0xff >> 0x3f),
                                                  CONCAT11(0xffU - (char)((int)sVar44 >> 7) &
                                                           (byte)sVar44 |
                                                           0xffU - (char)((long)(int)sVar44 + -0xff
                                                                         >> 0x3f),
                                                           -(char)((int)sVar31 >> 7) - 1U &
                                                           (byte)sVar31 |
                                                           -(char)((long)(int)sVar31 + -0xff >> 0x3f
                                                                  ) - 1U)))))));
            uVar21 = uVar21 - 8;
            puVar17 = puVar17 + 1;
            psVar19 = psVar19 + 8;
          } while (uVar21 != 0);
          if (uVar20 == uVar24) {
            return;
          }
        }
      }
      lVar16 = param_7 - uVar15;
      psVar19 = (short *)(*param_1 + uVar15 * 2);
      pbVar14 = (byte *)((long)param_4 + uVar15);
      do {
        sVar31 = *psVar19;
        *pbVar14 = -(char)(sVar31 >> 7) - 1U & (byte)(sVar31 >> 4) |
                   -(char)((((long)sVar31 << 0x20) >> 0x24) + -0xff >> 0x3f) - 1U;
        lVar16 = lVar16 + -1;
        psVar19 = psVar19 + 1;
        pbVar14 = pbVar14 + 1;
      } while (lVar16 != 0);
      return;
    }
    uVar15 = param_7 * 2;
    if ((param_3 == param_6) && (uVar20 = param_3, uVar15 - param_3 == 0)) goto LAB_009c0900;
    if (param_8 < 1) {
      return;
    }
    if ((long)(param_3 + param_7 * -2) < 0x80) {
      do {
        func_0x009f14e4(param_4,param_1,uVar15,0);
        param_1 = (undefined1 (*) [16])(*param_1 + param_3);
        param_4 = (float *)((long)param_4 + param_6);
        param_8 = param_8 + -1;
      } while (0 < param_8);
      return;
    }
    puVar1 = *param_1 + param_3;
    Hint_Prefetch(puVar1,0,0,0);
    Hint_Prefetch((long)puVar1 + 0x40,0,0,0);
    Hint_Prefetch((long)puVar1 + 0x80,0,0,0);
    Hint_Prefetch((long)puVar1 + 0xc0,0,0,0);
    param_9 = 0;
  }
  else {
    if (param_2 != 0) {
      return;
    }
    if (param_5 == 2) {
      if ((param_3 == param_7) && (param_7 * 4 - param_6 == 0)) {
        param_7 = param_8 * param_7;
      }
      else {
        if (param_8 < 1) {
          return;
        }
        if (param_3 - param_7 < 0x40) {
          do {
            func_0x009f0120(param_1,param_4,param_7);
            param_1 = (undefined1 (*) [16])(*param_1 + param_3);
            param_4 = (float *)((long)param_4 + param_6);
            param_8 = param_8 + -1;
          } while (0 < param_8);
          return;
        }
        Hint_Prefetch(*param_1 + param_3,0,0,0);
        Hint_Prefetch((long)(*param_1 + param_3) + 0x40,0,0,0);
      }
      if (cRam00000000014a67a8 == '\0') {
        uVar15 = 0;
        if ((long)param_7 < 1) {
          return;
        }
      }
      else {
        uVar15 = param_7 & 0xfffffffffffffff0;
        if (0 < (long)uVar15) {
          pauVar23 = param_1;
          pfVar18 = param_4;
          do {
            pauVar22 = pauVar23 + 1;
            bVar6 = (*pauVar23)[8];
            uVar7 = (*pauVar23)[9];
            uVar8 = (*pauVar23)[10];
            uVar9 = (*pauVar23)[0xb];
            bVar10 = (*pauVar23)[0xc];
            uVar11 = (*pauVar23)[0xd];
            uVar12 = (*pauVar23)[0xe];
            uVar13 = (*pauVar23)[0xf];
            auVar51._6_2_ = 0;
            auVar51._0_6_ =
                 (uint6)CONCAT14((*pauVar23)[1],
                                 (uint)CONCAT12((*pauVar23)[1],(ushort)(byte)(*pauVar23)[0])) &
                 0xffff0000ffff;
            auVar51[8] = (*pauVar23)[2];
            auVar51._9_3_ = 0;
            auVar51[0xc] = (*pauVar23)[3];
            auVar51._13_3_ = 0;
            auVar52 = NEON_ucvtf(auVar51,4);
            auVar34._1_3_ = 0;
            auVar34[0] = (*pauVar23)[4];
            auVar34[4] = (*pauVar23)[5];
            auVar34._5_3_ = 0;
            auVar34[8] = (*pauVar23)[6];
            auVar34._9_3_ = 0;
            auVar34[0xc] = (*pauVar23)[7];
            auVar34._13_3_ = 0;
            auVar35 = NEON_ucvtf(auVar34,4);
            *(long *)(pfVar18 + 2) = auVar52._8_8_;
            *(long *)pfVar18 = auVar52._0_8_;
            *(long *)(pfVar18 + 6) = auVar35._8_8_;
            *(long *)(pfVar18 + 4) = auVar35._0_8_;
            auVar36._6_2_ = 0;
            auVar36._0_6_ =
                 (uint6)CONCAT14(uVar7,(uint)CONCAT12(uVar7,(ushort)bVar6)) & 0xffff0000ffff;
            auVar36[8] = uVar8;
            auVar36._9_3_ = 0;
            auVar36[0xc] = uVar9;
            auVar36._13_3_ = 0;
            auVar52 = NEON_ucvtf(auVar36,4);
            auVar27._1_3_ = 0;
            auVar27[0] = bVar10;
            auVar27[4] = uVar11;
            auVar27._5_3_ = 0;
            auVar27[8] = uVar12;
            auVar27._9_3_ = 0;
            auVar27[0xc] = uVar13;
            auVar27._13_3_ = 0;
            auVar35 = NEON_ucvtf(auVar27,4);
            *(long *)(pfVar18 + 10) = auVar52._8_8_;
            *(long *)(pfVar18 + 8) = auVar52._0_8_;
            *(long *)(pfVar18 + 0xe) = auVar35._8_8_;
            *(long *)(pfVar18 + 0xc) = auVar35._0_8_;
            pfVar18 = pfVar18 + 0x10;
            pauVar23 = pauVar22;
          } while (pauVar22 < (undefined1 (*) [16])(*param_1 + uVar15));
        }
        if ((long)param_7 <= (long)uVar15) {
          return;
        }
      }
      lVar16 = param_7 - uVar15;
      pbVar14 = *param_1 + uVar15;
      pfVar18 = param_4 + uVar15;
      do {
        *pfVar18 = (float)*pbVar14;
        lVar16 = lVar16 + -1;
        pbVar14 = pbVar14 + 1;
        pfVar18 = pfVar18 + 1;
      } while (lVar16 != 0);
      return;
    }
    if (param_5 == 1) {
      if ((param_3 == param_7) && (param_7 * 2 - param_6 == 0)) {
        param_7 = param_8 * param_7;
      }
      else {
        if (param_8 < 1) {
          return;
        }
        if (param_3 - param_7 < 0x40) {
          do {
            func_0x009f01c4(param_1,param_4,param_7);
            param_1 = (undefined1 (*) [16])(*param_1 + param_3);
            param_4 = (float *)((long)param_4 + param_6);
            param_8 = param_8 + -1;
          } while (0 < param_8);
          return;
        }
        Hint_Prefetch(*param_1 + param_3,0,0,0);
        Hint_Prefetch((long)(*param_1 + param_3) + 0x40,0,0,0);
      }
      if (cRam00000000014a67a8 == '\0') {
        uVar15 = 0;
        if ((long)param_7 < 1) {
          return;
        }
      }
      else {
        uVar15 = param_7 & 0xfffffffffffffff0;
        if (0 < (long)uVar15) {
          pauVar23 = param_1;
          pfVar18 = param_4;
          do {
            pauVar22 = pauVar23 + 1;
            auVar35 = *pauVar23;
            uVar5 = CONCAT12(auVar35[9],auVar35._8_2_) & 0xff00ff;
            *(ushort *)(pfVar18 + 2) = (ushort)auVar35[4] << 4;
            *(ushort *)((long)pfVar18 + 10) = (ushort)auVar35[5] << 4;
            *(ushort *)(pfVar18 + 3) = (ushort)auVar35[6] << 4;
            *(ushort *)((long)pfVar18 + 0xe) = (ushort)auVar35[7] << 4;
            *(ushort *)pfVar18 = (ushort)auVar35[0] << 4;
            *(ushort *)((long)pfVar18 + 2) = (ushort)auVar35[1] << 4;
            *(ushort *)(pfVar18 + 1) = (ushort)auVar35[2] << 4;
            *(ushort *)((long)pfVar18 + 6) = (ushort)auVar35[3] << 4;
            *(ushort *)(pfVar18 + 6) = (ushort)auVar35[0xc] << 4;
            *(ushort *)((long)pfVar18 + 0x1a) = (ushort)auVar35[0xd] << 4;
            *(ushort *)(pfVar18 + 7) = (ushort)auVar35[0xe] << 4;
            *(ushort *)((long)pfVar18 + 0x1e) = (ushort)auVar35[0xf] << 4;
            *(short *)(pfVar18 + 4) = (short)uVar5 << 4;
            *(ushort *)((long)pfVar18 + 0x12) = (ushort)(byte)(uVar5 >> 0x10) << 4;
            *(ushort *)(pfVar18 + 5) = (ushort)auVar35[10] << 4;
            *(ushort *)((long)pfVar18 + 0x16) = (ushort)auVar35[0xb] << 4;
            pauVar23 = pauVar22;
            pfVar18 = pfVar18 + 8;
          } while (pauVar22 < (undefined1 (*) [16])(*param_1 + uVar15));
        }
        if ((long)param_7 <= (long)uVar15) {
          return;
        }
      }
      uVar21 = param_7 - uVar15;
      uVar20 = uVar15;
      if ((0xf < uVar21) &&
         ((*param_1 + param_7 <= (short *)((long)param_4 + uVar15 * 2) ||
          ((short *)((long)param_4 + param_7 * 2) <= *param_1 + uVar15)))) {
        uVar24 = uVar21 & 0xfffffffffffffff0;
        uVar20 = uVar15 + uVar24;
        puVar17 = (undefined8 *)(*param_1 + uVar15 + 8);
        psVar19 = (short *)((long)param_4 + (uVar15 + 8) * 2);
        uVar15 = uVar24;
        do {
          uVar26 = puVar17[-1];
          uVar33 = *puVar17;
          psVar19[-4] = (ushort)(byte)((ulong)uVar26 >> 0x20) << 4;
          psVar19[-3] = (ushort)(byte)((ulong)uVar26 >> 0x28) << 4;
          psVar19[-2] = (ushort)(byte)((ulong)uVar26 >> 0x30) << 4;
          psVar19[-1] = (ushort)(byte)((ulong)uVar26 >> 0x38) << 4;
          psVar19[-8] = (ushort)(byte)uVar26 << 4;
          psVar19[-7] = (ushort)(byte)((ulong)uVar26 >> 8) << 4;
          psVar19[-6] = (ushort)(byte)((ulong)uVar26 >> 0x10) << 4;
          psVar19[-5] = (ushort)(byte)((ulong)uVar26 >> 0x18) << 4;
          psVar19[4] = (ushort)(byte)((ulong)uVar33 >> 0x20) << 4;
          psVar19[5] = (ushort)(byte)((ulong)uVar33 >> 0x28) << 4;
          psVar19[6] = (ushort)(byte)((ulong)uVar33 >> 0x30) << 4;
          psVar19[7] = (ushort)(byte)((ulong)uVar33 >> 0x38) << 4;
          *psVar19 = (ushort)(byte)uVar33 << 4;
          psVar19[1] = (ushort)(byte)((ulong)uVar33 >> 8) << 4;
          psVar19[2] = (ushort)(byte)((ulong)uVar33 >> 0x10) << 4;
          psVar19[3] = (ushort)(byte)((ulong)uVar33 >> 0x18) << 4;
          puVar17 = puVar17 + 2;
          psVar19 = psVar19 + 0x10;
          uVar15 = uVar15 - 0x10;
        } while (uVar15 != 0);
        if (uVar21 == uVar24) {
          return;
        }
      }
      lVar16 = param_7 - uVar20;
      psVar19 = (short *)((long)param_4 + uVar20 * 2);
      pbVar14 = *param_1 + uVar20;
      do {
        *psVar19 = (ushort)*pbVar14 << 4;
        lVar16 = lVar16 + -1;
        psVar19 = psVar19 + 1;
        pbVar14 = pbVar14 + 1;
      } while (lVar16 != 0);
      return;
    }
    if (param_5 != 0) {
      return;
    }
    if ((param_3 == param_6) && (uVar20 = param_7, param_3 == param_7)) {
LAB_009c0900:
      param_9 = 0;
      param_3 = uVar20;
      goto LAB_009c090c;
    }
    if (param_8 < 1) {
      return;
    }
    if (param_3 - param_7 < 0x40) {
      do {
        func_0x009f14e4(param_4,param_1,param_7,0);
        param_1 = (undefined1 (*) [16])(*param_1 + param_3);
        param_4 = (float *)((long)param_4 + param_6);
        param_8 = param_8 + -1;
      } while (0 < param_8);
      return;
    }
    Hint_Prefetch(*param_1 + param_3,0,0,0);
    Hint_Prefetch((long)(*param_1 + param_3) + 0x40,0,0,0);
    param_9 = 0;
    uVar15 = param_7;
  }
SUB_009f14e4:
  uVar20 = 0;
  if ((0x1f < (long)uVar15) && (cRam00000000014a67a8 != '\0')) {
    if (param_9 != 2) {
      if (param_9 == 1) {
        uVar20 = uVar15 & 0xffffffffffffffe0;
        auVar35 = *param_1;
        auVar52 = param_1[1];
        pfVar18 = param_4;
        pauVar23 = param_1;
        while( true ) {
          pauVar22 = pauVar23 + 2;
          if ((undefined1 (*) [16])(*param_1 + uVar20) <= pauVar22) break;
          *(long *)(pfVar18 + 2) = auVar35._8_8_;
          *(long *)pfVar18 = auVar35._0_8_;
          *(long *)(pfVar18 + 6) = auVar52._8_8_;
          *(long *)(pfVar18 + 4) = auVar52._0_8_;
          auVar35 = *pauVar22;
          auVar52 = pauVar23[3];
          pfVar18 = pfVar18 + 8;
          pauVar23 = pauVar22;
        }
        *(long *)(pfVar18 + 2) = auVar35._8_8_;
        *(long *)pfVar18 = auVar35._0_8_;
        *(long *)(pfVar18 + 6) = auVar52._8_8_;
        *(long *)(pfVar18 + 4) = auVar52._0_8_;
        if ((long)uVar15 <= (long)uVar20) {
          return;
        }
      }
      else {
        uVar20 = 0;
        if ((long)uVar15 < 1) {
          return;
        }
      }
      goto LAB_009f1594;
    }
    uVar20 = uVar15 & 0xffffffffffffffe0;
    auVar35 = *param_1;
    auVar52 = param_1[1];
    pfVar18 = param_4;
    pauVar23 = param_1;
    while( true ) {
      pauVar22 = pauVar23 + 2;
      if ((undefined1 (*) [16])(*param_1 + uVar20) <= pauVar22) break;
      *(long *)(pfVar18 + 2) = auVar35._8_8_;
      *(long *)pfVar18 = auVar35._0_8_;
      *(long *)(pfVar18 + 6) = auVar52._8_8_;
      *(long *)(pfVar18 + 4) = auVar52._0_8_;
      auVar35 = *pauVar22;
      auVar52 = pauVar23[3];
      pfVar18 = pfVar18 + 8;
      pauVar23 = pauVar22;
    }
    *(long *)(pfVar18 + 2) = auVar35._8_8_;
    *(long *)pfVar18 = auVar35._0_8_;
    *(long *)(pfVar18 + 6) = auVar52._8_8_;
    *(long *)(pfVar18 + 4) = auVar52._0_8_;
  }
  if ((long)uVar15 <= (long)uVar20) {
    return;
  }
LAB_009f1594:
                    /* WARNING: Could not recover jumptable at 0x00c8fad4. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*(code *)PTR__memcpy_01411438)((long)param_4 + uVar20,*param_1 + uVar20,uVar15 - uVar20);
  return;
}


