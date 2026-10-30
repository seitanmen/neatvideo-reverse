// FUN_0004f000 size=60

void FUN_0004f000(undefined4 *param_1,long param_2)

{
  undefined4 uVar1;
  long lVar2;
  undefined8 uVar3;
  
  if (*(long *)(param_2 + 0x1c8) != 0) {
    lVar2 = *(long *)(param_2 + 0x1c0);
    uVar1 = *(undefined4 *)(lVar2 + 0x20);
    uVar3 = *(undefined8 *)(lVar2 + 0x24);
    *(undefined2 *)(param_1 + 3) = *(undefined2 *)(lVar2 + 0x2c);
    *(undefined8 *)(param_1 + 1) = uVar3;
    *param_1 = uVar1;
    return;
  }
  *(undefined2 *)(param_1 + 3) = 0;
  *(undefined8 *)(param_1 + 1) = 0;
  *param_1 = 1;
  return;
}


