// FUN_00157638 size=20

void FUN_00157638(long param_1)

{
  *(undefined1 *)(param_1 + 0x40) = 0;
                    /* WARNING: Could not recover jumptable at 0x00157648. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(**(long **)(param_1 + 0x10) + 0x18))();
  return;
}


