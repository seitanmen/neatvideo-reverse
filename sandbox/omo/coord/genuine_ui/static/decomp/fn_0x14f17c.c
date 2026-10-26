// FUN_0014f17c size=804

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0014f17c(long *param_1)

{
  uint uVar1;
  undefined8 uVar2;
  undefined1 uVar3;
  byte bVar4;
  long lVar5;
  long lVar6;
  ulong uVar7;
  undefined1 auStack_138 [264];
  
  lVar5 = func_0x009b8db8(param_1[0xe],param_1[0x10]);
  lVar6 = func_0x009b8d20(param_1[0xe],param_1[0x10]);
  uVar1 = uRam000000000141c252;
  uVar7 = (ulong)uRam000000000141c252;
  *(undefined8 *)(lRam000000000141c260 + 0x60d6f78) = 0x141c250;
  if (*(long *)(lRam000000000141c258 + 0x553f500) != 0) {
    uVar7 = -(ulong)*(byte *)(*(long *)(lRam000000000141c258 + 0x553f500) + (uVar7 & 0xffff));
  }
  lVar5 = lVar5 + ((ulong)(uVar1 >> 0x18) + (uVar7 + (uVar1 >> 0x10 & 0xff)) * 2) * lVar6;
  (**(code **)(*param_1 + 0x70))(param_1,lVar5 + lVar6,0);
  func_0x009b8c24(param_1[0xe]);
  func_0x0099df24(param_1[0xe]);
  *(undefined4 *)((long)param_1 + 0x34) = 1;
  param_1[7] = lVar5;
  func_0x001c1114(auStack_138);
  func_0x009c7318();
  uVar3 = func_0x009af718(param_1[0xe],auStack_138,1);
  *(undefined1 *)((long)param_1 + 0x152) = uVar3;
  lVar5 = param_1[7];
  param_1[8] = param_1[8] + lVar5;
  if (*(int *)((long)param_1 + 0x34) == 1) {
    param_1[9] = param_1[9] + lVar5;
    param_1[7] = 0;
  }
  else {
    param_1[10] = param_1[10] + lVar5;
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
  uRam000000000141c268 = *(uint *)(&UNK_00bfb6d0 + lRam000000000141c270);
  uVar1 = (uRam000000000141c268 ^ uRam000000000141c268 >> 1) & 0x41551404;
  uRam000000000141c268 = (uVar1 | uVar1 << 1) ^ uRam000000000141c268;
  if (uRam000000000141c268 != 0) {
    *(uint *)(&UNK_001d4640 + lRam000000000141c278) = uRam000000000141c268;
  }
  func_0x0099df24(param_1[0xe]);
  *(undefined4 *)((long)param_1 + 0x34) = 1;
  param_1[7] = lVar6;
  bVar4 = *(byte *)((long)param_1 + 0x152) & (*(byte *)((long)param_1 + 0x6d) ^ 1);
  *(byte *)((long)param_1 + 0x152) = bVar4;
  if (bVar4 == 0) {
    bVar4 = 0;
    param_1[8] = param_1[8] + lVar6;
  }
  else {
    bVar4 = func_0x009a775c(param_1[0xe],param_1[0x10],0,0,0,0,0,0,0,0,1);
    *(byte *)((long)param_1 + 0x152) = bVar4;
    lVar6 = param_1[7];
    param_1[8] = param_1[8] + lVar6;
    if (*(int *)((long)param_1 + 0x34) != 1) {
      param_1[10] = param_1[10] + lVar6;
      param_1[7] = 0;
      if ((*(int *)((long)param_1 + 0x34) == 2) && ((char)param_1[6] != '\0')) {
        *(undefined1 *)(param_1 + 6) = 0;
        if (iRam000000000155b550 != 1) {
          func_0x00af4a8c(0x155b548);
        }
        func_0x0097fb60(uRam000000000155b560);
        bVar4 = *(byte *)((long)param_1 + 0x152);
      }
      goto LAB_0014f454;
    }
  }
  param_1[9] = param_1[9] + lVar6;
  param_1[7] = 0;
LAB_0014f454:
  *(undefined4 *)((long)param_1 + 0x34) = 0;
  bVar4 = bVar4 & (*(byte *)((long)param_1 + 0x6d) ^ 1);
  *(byte *)((long)param_1 + 0x152) = bVar4;
  if (bVar4 == 0) {
    func_0x009b8c84(param_1[0xe]);
    uVar2 = _UNK_010b4460;
    *(undefined8 *)((long)param_1 + 0x15c) = _UNK_010b4468;
    *(undefined8 *)((long)param_1 + 0x154) = uVar2;
  }
  func_0x009c3db4(auStack_138);
  return;
}


