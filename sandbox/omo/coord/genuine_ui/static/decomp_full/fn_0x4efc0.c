// FUN_0004efc0 size=64

ulong FUN_0004efc0(long param_1)

{
  ulong uVar1;
  
  uVar1 = FUN_00054cf0();
  if ((int)uVar1 != 0) {
    if (*(char *)(param_1 + 0x1e2) == '\0') {
      uVar1 = (ulong)(*(char *)(param_1 + 0x1e4) == '\0');
    }
    else {
      uVar1 = 0;
    }
  }
  return uVar1;
}


