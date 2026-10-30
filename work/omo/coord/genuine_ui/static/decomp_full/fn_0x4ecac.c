// FUN_0004ecac size=336

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0004ecac(long param_1,long param_2,undefined8 param_3,undefined8 param_4,ulong param_5)

{
  undefined8 uVar1;
  
  *(undefined1 *)(param_1 + 600) = 1;
  if (*(long *)(param_1 + 0x150) != 0) {
    FUN_00926964(*(undefined8 *)(param_1 + 0x1e8),0);
    FUN_00926964(*(undefined8 *)(param_1 + 0x208),0);
    FUN_00926964(*(undefined8 *)(param_1 + 0x210),0);
    FUN_00926964(*(undefined8 *)(param_1 + 0x218),0);
    FUN_000577a4(*(undefined8 *)(param_1 + 0x188),0);
  }
  if (((param_5 & 1) == 0) && (*(long *)(param_1 + 0x150) != 0)) {
    *(undefined1 *)(param_1 + 0x1e0) = 0;
    uVar1 = _DAT_010b4460;
    *(undefined8 *)(param_1 + 0x1d8) = _UNK_010b4468;
    *(undefined8 *)(param_1 + 0x1d0) = uVar1;
    FUN_001d1d68(param_1,0);
    FUN_0004d8dc(param_1);
  }
  FUN_000549dc(param_1,param_2,param_3,param_4,param_5);
  if (param_2 != 0) {
    FUN_00926964(*(undefined8 *)(param_1 + 0x208),*(undefined8 *)(param_1 + 0x150));
    FUN_0092b648(0x4018000000000000,*(undefined8 *)(param_1 + 0x208));
    FUN_00926964(*(undefined8 *)(param_1 + 0x210),*(undefined8 *)(param_1 + 0x150));
    FUN_0092b648(0x401a000000000000,*(undefined8 *)(param_1 + 0x210));
    FUN_00926964(*(undefined8 *)(param_1 + 0x1e8),*(undefined8 *)(param_1 + 0x150));
    FUN_0092b648(0x401c000000000000,*(undefined8 *)(param_1 + 0x1e8));
    FUN_00926964(*(undefined8 *)(param_1 + 0x218),*(undefined8 *)(param_1 + 0x150));
    FUN_0092b648(0x4022000000000000,*(undefined8 *)(param_1 + 0x218));
    FUN_000577a4(*(undefined8 *)(param_1 + 0x188),*(undefined8 *)(param_1 + 0x150));
    FUN_00057764(0x4008000000000000,*(undefined8 *)(param_1 + 0x188));
  }
  *(undefined1 *)(param_1 + 600) = 0;
  return;
}


