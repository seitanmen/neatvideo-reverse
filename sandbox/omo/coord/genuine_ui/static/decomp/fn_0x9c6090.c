// FUN_009c6090 size=124

void FUN_009c6090(long param_1,float *param_2,float *param_3,undefined8 param_4)

{
  float fVar1;
  float fVar2;
  
  func_0x009c5258(param_1,param_4);
  fVar1 = *(float *)(param_1 + 0x10);
  if (*(float *)(param_1 + 0xc) <= *(float *)(param_1 + 0x10)) {
    fVar1 = *(float *)(param_1 + 0xc);
  }
  if (*(float *)(param_1 + 8) <= fVar1) {
    fVar1 = *(float *)(param_1 + 8);
  }
  *param_2 = fVar1;
  fVar1 = *(float *)(param_1 + 0x1c);
  if (*(float *)(param_1 + 0x1c) <= *(float *)(param_1 + 0x20)) {
    fVar1 = *(float *)(param_1 + 0x20);
  }
  fVar2 = *(float *)(param_1 + 0x18);
  if (*(float *)(param_1 + 0x18) <= fVar1) {
    fVar2 = fVar1;
  }
  *param_3 = fVar2;
  return;
}


