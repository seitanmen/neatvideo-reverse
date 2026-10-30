// FUN_0004d344 size=80

void FUN_0004d344(long param_1,undefined8 param_2)

{
  long lVar1;
  
  FUN_00052d04();
  lVar1 = FUN_003bbf58(&DAT_013c12b0,param_2);
  if (lVar1 != 0) {
    *(undefined1 *)(param_1 + 0x228) = *(undefined1 *)(lVar1 + 0x228);
    *(undefined1 *)(param_1 + 0x229) = *(undefined1 *)(lVar1 + 0x229);
    *(undefined1 *)(param_1 + 0x259) = *(undefined1 *)(lVar1 + 0x259);
  }
  return;
}


