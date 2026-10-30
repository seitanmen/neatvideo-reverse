// FUN_009c0214 @ 009c0214 size=80 tag=FindFreqLevs_L22_slot0

void FUN_009c0214(long param_1)

{
  undefined4 uVar1;
  
  uVar1 = FUN_00997bcc(**(undefined4 **)(param_1 + 0x18),
                       *(undefined8 *)(**(long **)(param_1 + 0x10) + 0xc0),
                       *(long *)(param_1 + 0x20) + 8,0,1,**(undefined8 **)(param_1 + 0x28));
  *(undefined4 *)(**(long **)(param_1 + 8) + 8) = uVar1;
  return;
}


