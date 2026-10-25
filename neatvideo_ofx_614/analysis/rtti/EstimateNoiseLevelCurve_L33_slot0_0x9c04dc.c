// FUN_009c04dc @ 009c04dc size=108 tag=EstimateNoiseLevelCurve_L33_slot0

void FUN_009c04dc(long param_1,long param_2,undefined1 *param_3)

{
  long lVar1;
  
  lVar1 = **(long **)(param_1 + 0x20) * param_2 * 8;
  FUN_009b76b0(*(undefined8 *)(param_1 + 8),**(undefined8 **)(param_1 + 0x10),
               **(long **)(param_1 + 0x18) + lVar1,**(long **)(param_1 + 0x28) + lVar1,
               **(long **)(param_1 + 0x20),*(undefined8 *)(param_1 + 0x30),
               *(long *)(*(long *)(param_1 + 0x38) + 0x10) + param_2 * 0x50);
  *param_3 = 1;
  return;
}


