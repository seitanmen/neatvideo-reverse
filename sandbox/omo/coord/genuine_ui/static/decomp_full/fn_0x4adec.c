// FUN_0004adec size=208

void FUN_0004adec(long *param_1,long param_2,undefined8 param_3,undefined8 param_4,
                 undefined8 param_5)

{
  uint uVar1;
  
  if (param_1[0x2a] != 0) {
    FUN_00926964(param_1[0x4c],0);
  }
  FUN_0004ecac(param_1,param_2,param_3,param_4,param_5);
  uVar1 = *(uint *)(DAT_0141a670 + 0x136450);
  if (DAT_0141a668 != 0) {
    uVar1 = uVar1 >> 0x18 | uVar1 << 8;
  }
  if (uVar1 != 0) {
    *(uint *)(DAT_0141a678 + 0x6247c0) = uVar1;
  }
  if (param_2 != 0) {
    FUN_00926964(param_1[0x4c],param_1[0x2a]);
    FUN_0092b648(0x4014000000000000,param_1[0x4c]);
  }
                    /* WARNING: Could not recover jumptable at 0x0004aeb8. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(*param_1 + 0x1e8))(param_1);
  return;
}


