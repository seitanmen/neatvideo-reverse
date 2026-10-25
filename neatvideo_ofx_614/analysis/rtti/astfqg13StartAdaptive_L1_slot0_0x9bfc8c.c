// FUN_009bfc8c @ 009bfc8c size=124 tag=astfqg13StartAdaptive_L1_slot0

void FUN_009bfc8c(long param_1)

{
  long lVar1;
  undefined1 uVar2;
  
  lVar1 = *(long *)(param_1 + 0x88);
  uVar2 = FUN_009a775c(lVar1,*(undefined8 *)(param_1 + 0x90),1,2,*(undefined8 *)(param_1 + 0x98),
                       lVar1 + 0x25cf0,lVar1 + 0x25cf4,lVar1 + 0x25cf8,
                       *(undefined8 *)(param_1 + 0xa0),*(undefined8 *)(lVar1 + 0x24358),0);
  *(undefined1 *)(lVar1 + 0x25cfc) = uVar2;
  return;
}


