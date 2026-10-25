// FUN_009c01c4 @ 009c01c4 size=80 tag=FindFreqLevs_L21_slot0

void FUN_009c01c4(long param_1)

{
  undefined4 uVar1;
  
  uVar1 = FUN_00997bcc(**(undefined4 **)(param_1 + 0x18),
                       *(undefined8 *)(**(long **)(param_1 + 0x10) + 0xb8),
                       *(long *)(param_1 + 0x20) + 4,0,1,**(undefined8 **)(param_1 + 0x28));
  *(undefined4 *)(**(long **)(param_1 + 8) + 4) = uVar1;
  return;
}


