// FUN_0014f040 size=316

void FUN_0014f040(long *param_1)

{
  undefined1 uVar1;
  long lVar2;
  long lVar3;
  
  lVar2 = func_0x009b8d20(param_1[0xe],param_1[0x10]);
  (**(code **)(*param_1 + 0x70))(param_1,lVar2,0);
  func_0x0099df24(param_1[0xe]);
  *(undefined4 *)((long)param_1 + 0x34) = 1;
  param_1[7] = lVar2;
  lVar3 = *(long *)(&UNK_0000c2a8 + param_1[2]);
  lVar2 = func_0x009c923c(param_1[0x10]);
  uVar1 = func_0x009a775c(param_1[0xe],param_1[0x10],0,lVar3 != lVar2,0,0,0,0,0,0,1);
  *(undefined1 *)((long)param_1 + 0x152) = uVar1;
  lVar2 = param_1[7];
  param_1[8] = param_1[8] + lVar2;
  if (*(int *)((long)param_1 + 0x34) == 1) {
    param_1[9] = param_1[9] + lVar2;
    param_1[7] = 0;
  }
  else {
    param_1[10] = param_1[10] + lVar2;
    param_1[7] = 0;
    if ((*(int *)((long)param_1 + 0x34) == 2) && ((char)param_1[6] != '\0')) {
      *(undefined1 *)(param_1 + 6) = 0;
      if (iRam000000000155b550 != 1) {
        func_0x00af4a8c(0x155b548);
      }
      func_0x0097fb60(uRam000000000155b560);
    }
  }
  *(undefined4 *)((long)param_1 + 0x34) = 0;
  return;
}


