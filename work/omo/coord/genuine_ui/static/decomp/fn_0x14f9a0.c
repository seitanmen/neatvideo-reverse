// FUN_0014f9a0 size=448

void FUN_0014f9a0(long *param_1)

{
  undefined1 uVar1;
  long lVar2;
  undefined1 auStack_138 [264];
  
  (**(code **)(*param_1 + 0x70))(param_1,0,1);
  func_0x0099df24(param_1[0xe]);
  *(undefined4 *)((long)param_1 + 0x34) = 2;
  param_1[7] = 1;
  if ((int)param_1[0xd] != 9) {
    if (iRam000000000155b550 != 1) {
      func_0x00af4a8c(0x155b548);
    }
    uVar1 = func_0x0097fa84(uRam000000000155b560,param_1[5]);
    *(undefined1 *)(param_1 + 6) = uVar1;
  }
  uVar1 = func_0x009b0b68(param_1[0xe],param_1[0x10],param_1[0x24],param_1[0x25],param_1[0x26],
                          *(undefined1 *)((long)param_1 + 0x13c),
                          *(undefined1 *)((long)param_1 + 0x13d),
                          *(undefined4 *)((long)param_1 + 0x11c));
  *(undefined1 *)((long)param_1 + 0x152) = uVar1;
  if (*(char *)((long)param_1 + 0x6d) == '\0') {
    if (*(char *)((long)param_1 + 0x13c) == '\0') {
      lVar2 = param_1[0x24];
      if ((lVar2 == 0) && (lVar2 = param_1[0x25], lVar2 == 0)) {
        lVar2 = param_1[0x26];
      }
      func_0x009b2814(param_1[0xe],lVar2,param_1[0x28],param_1[0x29],0);
    }
    else {
      func_0x001c1114(auStack_138);
      func_0x009c3de4(auStack_138,*(undefined8 *)(param_1[0x24] + 0xb0),
                      *(undefined8 *)(param_1[0x25] + 0xb0),*(undefined8 *)(param_1[0x26] + 0xb0),0,
                      *(undefined8 *)(param_1[0x10] + 0xd0),*(undefined8 *)(param_1[0x10] + 0xd8),
                      *(undefined4 *)((long)param_1 + 0x11c),0);
      func_0x009b2814(param_1[0xe],auStack_138,param_1[0x28],param_1[0x29],
                      *(undefined1 *)((long)param_1 + 0x13c));
      func_0x009c3db4(auStack_138);
    }
  }
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


