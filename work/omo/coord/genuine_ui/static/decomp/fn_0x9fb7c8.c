// FUN_009fb7c8 size=356

/* WARNING: Removing unreachable block (ram,0x009fb858) */

void FUN_009fb7c8(float param_1,float param_2,long param_3,long param_4,long param_5,long param_6,
                 long param_7,long param_8,long param_9)

{
  long lVar1;
  long lVar2;
  long lVar3;
  uint *puVar4;
  float *pfVar5;
  float *pfVar6;
  float *pfVar7;
  float *pfVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  
  if (param_9 < param_8) {
    param_8 = param_8 - param_9;
    param_9 = param_9 * 4;
    puVar4 = (uint *)(param_7 + param_9);
    pfVar5 = (float *)(param_6 + param_9);
    pfVar6 = (float *)(param_5 + param_9);
    pfVar7 = (float *)(param_4 + param_9);
    pfVar8 = (float *)(param_3 + param_9);
    do {
      fVar11 = *pfVar5 * 0.003921569;
      fVar9 = (param_1 / 3.0) * (*pfVar8 + *pfVar7 + *pfVar6) + param_2;
      fVar10 = 0.0;
      if (0.0 <= fVar9) {
        fVar10 = fVar9;
      }
      lVar1 = (long)(float)(int)((fVar11 * -0.7 + 1.0) * fVar10);
      lVar3 = (long)(float)(int)((fVar11 * -0.45 + 0.9) * fVar10);
      lVar2 = (long)(float)(int)(fVar11 * 127.0 + (fVar11 * -0.25 + 0.75) * fVar10);
      *puVar4 = ((-(int)(lVar3 >> 0x3f) - 1U & (uint)lVar3 | -(int)(lVar3 + -0xff >> 0x3f) - 1U) &
                0xff) << 8 |
                (0xffU - (int)(lVar1 >> 0x3f) & (uint)lVar1 | -(int)(lVar1 + -0xff >> 0x3f) - 1U) <<
                0x10 | (-(int)(lVar2 >> 0x3f) - 1U & (uint)lVar2 |
                       -(int)(lVar2 + -0xff >> 0x3f) - 1U) & 0xff | 0xff000000;
      param_8 = param_8 + -1;
      puVar4 = puVar4 + 1;
      pfVar5 = pfVar5 + 1;
      pfVar6 = pfVar6 + 1;
      pfVar7 = pfVar7 + 1;
      pfVar8 = pfVar8 + 1;
    } while (param_8 != 0);
  }
  return;
}


