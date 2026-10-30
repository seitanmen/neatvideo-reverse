// FUN_0099caa8 size=912

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8
FUN_0099caa8(undefined1 param_1 [16],undefined8 param_2,undefined8 param_3,undefined8 param_4,
            float *param_5,float *param_6,float *param_7,long param_8)

{
  short *psVar1;
  undefined8 uVar2;
  float *extraout_x1;
  long lVar3;
  long lVar4;
  float fVar5;
  float extraout_s0;
  float extraout_s0_00;
  undefined8 extraout_d0;
  long lVar6;
  long lVar7;
  long lVar8;
  long lVar9;
  long lVar10;
  long lVar11;
  long lVar12;
  long lVar13;
  float fVar14;
  float fVar15;
  long lVar16;
  long lVar17;
  ulong uVar18;
  ulong auStack_40078 [16384];
  int aiStack_20078 [32770];
  
  psVar1 = (short *)(*(code *)PTR____chkstk_darwin_013b4308)();
  lVar3 = *(long *)PTR____stack_chk_guard_013b4320;
  uVar2 = _bzero(auStack_40078,0x40000);
  lVar4 = param_8;
  if (0 < param_8) {
    do {
      aiStack_20078[*psVar1] = aiStack_20078[*psVar1] + 1;
      lVar4 = lVar4 + -1;
      psVar1 = psVar1 + 1;
    } while (lVar4 != 0);
  }
  lVar4 = 0;
  lVar6 = 0;
  lVar7 = 0;
  lVar10 = 0;
  lVar11 = 0;
  lVar8 = _UNK_00c95950;
  lVar9 = _UNK_00c95958;
  do {
    uVar18 = *(ulong *)((long)auStack_40078 + lVar4 + 8);
    lVar6 = (lVar8 + -0x8000) * (*(ulong *)((long)auStack_40078 + lVar4) & 0xffffffff) + lVar6;
    lVar7 = (lVar9 + -0x8000) * (*(ulong *)((long)auStack_40078 + lVar4) >> 0x20) + lVar7;
    lVar10 = (lVar8 + -0x7ffe) * (uVar18 & 0xffffffff) + lVar10;
    lVar11 = (lVar9 + -0x7ffe) * (uVar18 >> 0x20) + lVar11;
    lVar8 = lVar8 + 4;
    lVar9 = lVar9 + 4;
    lVar4 = lVar4 + 0x10;
  } while (lVar4 != 0x40000);
  lVar4 = 0;
  lVar9 = lVar10 + lVar6 + lVar11 + lVar7;
  fVar5 = (float)param_8;
  lVar8 = 0;
  if (param_8 != 0) {
    lVar8 = lVar9 / param_8;
  }
  lVar8 = -0x8000 - lVar8;
  lVar10 = 0;
  lVar11 = 0;
  lVar12 = 0;
  lVar13 = 0;
  lVar6 = _UNK_00c95950;
  lVar7 = _UNK_00c95958;
  do {
    lVar16 = lVar6 + lVar8 + 2;
    lVar17 = lVar7 + lVar8 + 2;
    uVar18 = *(ulong *)((long)auStack_40078 + lVar4 + 8);
    lVar10 = (lVar8 + lVar6) * (lVar8 + lVar6) *
             (*(ulong *)((long)auStack_40078 + lVar4) & 0xffffffff) + lVar10;
    lVar11 = (lVar8 + lVar7) * (lVar8 + lVar7) * (*(ulong *)((long)auStack_40078 + lVar4) >> 0x20) +
             lVar11;
    lVar12 = lVar16 * lVar16 * (uVar18 & 0xffffffff) + lVar12;
    lVar13 = lVar17 * lVar17 * (uVar18 >> 0x20) + lVar13;
    lVar6 = lVar6 + 4;
    lVar7 = lVar7 + 4;
    lVar4 = lVar4 + 0x10;
  } while (lVar4 != 0x40000);
  lVar4 = 0;
  lVar8 = 0;
  fVar14 = ((float)lVar9 / fVar5) * 0.0625;
  fVar15 = SQRT((float)(lVar12 + lVar10 + lVar13 + lVar11) / (float)(param_8 + -1)) * 0.0625;
  lVar9 = (long)(float)(int)(fVar5 * (float)extraout_d0);
  lVar7 = (long)(float)(int)(fVar5 * (float)param_2);
  lVar6 = 0;
  while( true ) {
    lVar8 = lVar8 + (ulong)*(uint *)((long)auStack_40078 + lVar4 * 4);
    lVar12 = lVar4 + -0x8000;
    lVar10 = lVar12;
    lVar11 = 0x7fffffffffffffff;
    if (lVar8 <= lVar9) {
      lVar10 = lVar6;
      lVar11 = lVar9;
    }
    lVar9 = lVar11;
    if (lVar7 < lVar8) break;
    lVar4 = lVar4 + 1;
    lVar6 = lVar10;
    if (lVar4 == 0x10000) {
      lVar12 = 0x8000;
      if (lVar9 != 0x7fffffffffffffff) {
        lVar10 = 0x8000;
      }
      if (lVar7 == 0x7fffffffffffffff) {
        lVar12 = 0;
      }
LAB_0099cd70:
      if (param_6 != (float *)0x0) {
        uVar2 = func_0x009d3e40(extraout_d0);
        *param_6 = fVar14 + fVar15 * extraout_s0;
      }
      if (param_7 != (float *)0x0) {
        uVar2 = func_0x009d3e40(param_2);
        *param_7 = fVar14 + fVar15 * extraout_s0_00;
      }
      if (extraout_x1 != (float *)0x0) {
        *extraout_x1 = (float)lVar10 * 0.0625;
      }
      if (param_5 != (float *)0x0) {
        *param_5 = (float)lVar12 * 0.0625;
      }
      if (*(long *)PTR____stack_chk_guard_013b4320 != lVar3) {
        uVar2 = ___stack_chk_fail();
        func_0x009979fc();
        return uVar2;
      }
      return uVar2;
    }
  }
  if (lVar9 != 0x7fffffffffffffff) {
    lVar10 = 0x8000;
  }
  goto LAB_0099cd70;
}


