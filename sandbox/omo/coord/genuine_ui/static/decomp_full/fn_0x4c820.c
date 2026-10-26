// FUN_0004c820 size=52

bool FUN_0004c820(long param_1,undefined4 *param_2,undefined4 *param_3)

{
  undefined4 uVar1;
  long lVar2;
  
  lVar2 = *(long *)(param_1 + 0x1c8);
  if (lVar2 != 0) {
    uVar1 = *(undefined4 *)(*(long *)(param_1 + 0x1b8) + 0x14);
    *param_2 = *(undefined4 *)(*(long *)(param_1 + 0x1b8) + 0x10);
    *param_3 = uVar1;
    return lVar2 != 0;
  }
  *param_3 = 0;
  *param_2 = 0;
  return false;
}


