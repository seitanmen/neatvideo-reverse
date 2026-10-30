// FUN_0004acf0 size=140

void FUN_0004acf0(long param_1,undefined8 param_2,undefined8 param_3)

{
  long lVar1;
  undefined8 uVar2;
  
  lVar1 = FUN_003bbf58(&DAT_013c0ef8);
  FUN_0004d14c(param_1,param_2,param_3);
  uVar2 = *(undefined8 *)(lVar1 + 0x2a8);
  *(undefined8 *)(param_1 + 0x2b0) = *(undefined8 *)(lVar1 + 0x2b0);
  *(undefined8 *)(param_1 + 0x2a8) = uVar2;
  *(undefined1 *)(param_1 + 0x2b9) = *(undefined1 *)(lVar1 + 0x2b9);
  if (*(char *)(param_1 + 0x2b8) == *(char *)(lVar1 + 0x2b8)) {
    return;
  }
  *(char *)(param_1 + 0x2b8) = *(char *)(lVar1 + 0x2b8);
  FUN_000495e8(param_1);
  return;
}


