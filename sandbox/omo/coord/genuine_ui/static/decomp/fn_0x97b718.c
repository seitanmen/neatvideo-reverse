// FUN_0097b718 size=84

void FUN_0097b718(float *param_1,float *param_2,float *param_3,float *param_4,float *param_5,
                 float *param_6,long param_7)

{
  float fVar1;
  float fVar2;
  float fVar3;
  
  if (0 < param_7) {
    do {
      fVar1 = *param_1;
      fVar3 = *param_3;
      fVar2 = (fVar1 + *param_2 + fVar3) / 3.0;
      *param_4 = fVar2;
      *param_5 = (fVar1 - fVar2) * 0.75;
      *param_6 = (fVar3 - *param_4) * 0.75;
      param_7 = param_7 + -1;
      param_1 = param_1 + 1;
      param_2 = param_2 + 1;
      param_3 = param_3 + 1;
      param_4 = param_4 + 1;
      param_5 = param_5 + 1;
      param_6 = param_6 + 1;
    } while (param_7 != 0);
  }
  return;
}


