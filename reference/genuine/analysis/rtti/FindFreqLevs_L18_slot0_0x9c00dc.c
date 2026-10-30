// FUN_009c00dc @ 009c00dc size=76 tag=FindFreqLevs_L18_slot0

void FUN_009c00dc(long param_1)

{
  undefined4 uVar1;
  
  uVar1 = FUN_00997bcc(**(undefined4 **)(param_1 + 0x18),
                       *(undefined8 *)(**(long **)(param_1 + 0x10) + 0xb0),
                       *(undefined8 *)(param_1 + 0x20),0,1,**(undefined8 **)(param_1 + 0x28));
  *(undefined4 *)**(undefined8 **)(param_1 + 8) = uVar1;
  return;
}


