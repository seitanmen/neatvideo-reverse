// FUN_0004c770 size=176

ulong FUN_0004c770(long param_1,ulong param_2)

{
  ulong uVar1;
  undefined4 uVar2;
  long lVar3;
  
  if ((*(long *)(param_1 + 0x148) == 0) ||
     (lVar3 = *(long *)(*(long *)(param_1 + 0x148) + 8), lVar3 == 0)) {
    uVar2 = 0;
  }
  else {
    uVar2 = *(undefined4 *)(lVar3 + 0xd0);
  }
  uVar1 = FUN_00052338(param_1,0,uVar2,param_2,
                       (*(int *)(param_1 + 0x1d8) - *(int *)(param_1 + 0x1d0)) + 1);
  if ((*(long *)(param_1 + 0x148) == 0) ||
     (lVar3 = *(long *)(*(long *)(param_1 + 0x148) + 8), lVar3 == 0)) {
    uVar2 = 0;
  }
  else {
    uVar2 = *(undefined4 *)(lVar3 + 0xd8);
  }
  lVar3 = FUN_00052338(param_1,0,uVar2,param_2 >> 0x20,
                       (*(int *)(param_1 + 0x1dc) - *(int *)(param_1 + 0x1d4)) + 1);
  return uVar1 & 0xffffffff | lVar3 << 0x20;
}


