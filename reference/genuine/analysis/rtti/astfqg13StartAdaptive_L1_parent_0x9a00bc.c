// FUN_009a00bc @ 009a00bc size=252 tag=astfqg13StartAdaptive_L1_parent

void FUN_009a00bc(long param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4)

{
  undefined1 auVar1 [16];
  ulong uVar2;
  undefined8 *puVar3;
  undefined8 uVar4;
  long lVar5;
  ulong uVar6;
  
  uVar2 = DAT_014a67b8;
  if (*(long *)(param_1 + 0x24358) == 0) {
    auVar1._8_8_ = 0;
    auVar1._0_8_ = DAT_014a67b8;
    uVar6 = DAT_014a67b8 * 0x50;
    lVar5 = uVar6 + 0x10;
    if (SUB168(auVar1 * ZEXT816(0x50),8) != 0 || 0xffffffffffffffef < uVar6) {
      lVar5 = -1;
    }
    puVar3 = (undefined8 *)__Znam(lVar5);
    *puVar3 = 0x50;
    puVar3[1] = uVar2;
    if (uVar2 != 0) {
      lVar5 = 0;
      do {
        thunk_FUN_009d7bc4((long)puVar3 + lVar5 + 0x10);
        lVar5 = lVar5 + 0x50;
      } while (uVar6 - lVar5 != 0);
    }
    *(undefined8 **)(param_1 + 0x24358) = puVar3 + 2;
  }
  uVar4 = __Znwm(0xa8);
  puVar3 = (undefined8 *)FUN_009d5ebc(uVar4,0xffffffffffffffff,0xffffffffffffffff);
  *puVar3 = &DAT_013fce70;
  puVar3[0x11] = param_1;
  puVar3[0x12] = param_2;
  puVar3[0x13] = param_3;
  puVar3[0x14] = param_4;
  *(undefined8 **)(param_1 + 0x1ea50) = puVar3;
  FUN_009d6098();
  return;
}


