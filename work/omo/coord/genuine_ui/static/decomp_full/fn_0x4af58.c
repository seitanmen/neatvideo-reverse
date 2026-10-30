// FUN_0004af58 size=108

void FUN_0004af58(long param_1)

{
  long lVar1;
  long lVar2;
  long *plVar3;
  
  if (*(long *)(param_1 + 0x1c8) != 0) {
    lVar1 = *(long *)(param_1 + 0x1b8);
    plVar3 = *(long **)(param_1 + 0x1c0);
    lVar2 = *plVar3;
    *(undefined8 *)(lVar2 + 8) = *(undefined8 *)(lVar1 + 8);
    **(long **)(lVar1 + 8) = lVar2;
    *(undefined8 *)(param_1 + 0x1c8) = 0;
    while (plVar3 != (long *)(param_1 + 0x1b8)) {
      plVar3 = (long *)plVar3[1];
      __ZdlPv();
    }
  }
  FUN_000492d4(param_1);
  return;
}


