// FUN_0014ee9c size=396

void FUN_0014ee9c(long *param_1)

{
  uint uVar1;
  uint uVar2;
  undefined1 uVar3;
  long lVar4;
  undefined1 auStack_138 [264];
  
  lVar4 = func_0x009b8db8(param_1[0xe],param_1[0x10]);
  (**(code **)(*param_1 + 0x70))(param_1,lVar4,0);
  uVar2 = *(uint *)(lRam000000000141c230 + 0x1a776a8);
  uVar1 = (uVar2 ^ uVar2 >> 1) & 0x41445001;
  uVar2 = (uVar1 | uVar1 << 1) ^ uVar2;
  if (uVar2 != 1) {
    *(uint *)(lRam000000000141c238 + 0x172adb8) = uVar2;
  }
  func_0x0099df24(param_1[0xe]);
  *(undefined4 *)((long)param_1 + 0x34) = 1;
  param_1[7] = lVar4;
  func_0x001c1114(auStack_138);
  func_0x009c7318();
  uVar3 = func_0x009af718(param_1[0xe],auStack_138,1);
  *(undefined1 *)((long)param_1 + 0x152) = uVar3;
  lVar4 = param_1[7];
  param_1[8] = param_1[8] + lVar4;
  if (*(int *)((long)param_1 + 0x34) == 1) {
    param_1[9] = param_1[9] + lVar4;
    param_1[7] = 0;
  }
  else {
    param_1[10] = param_1[10] + lVar4;
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
  func_0x009c3db4(auStack_138);
  return;
}


