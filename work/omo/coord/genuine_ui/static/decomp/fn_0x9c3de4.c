// FUN_009c3de4 size=244

void FUN_009c3de4(long *param_1,long param_2,long param_3,long param_4,long param_5,long param_6,
                 long param_7,uint param_8,undefined1 param_9)

{
  uint uVar1;
  uint uVar2;
  
  (**(code **)(*param_1 + 0x18))();
  *(uint *)(param_1 + 0x1e) = param_8;
  param_1[0x1a] = param_6;
  param_1[0x1b] = param_7;
  *(undefined1 *)((long)param_1 + 0xf4) = param_9;
  param_1[0x16] = param_2;
  param_1[0x17] = param_3;
  param_1[0x18] = param_4;
  param_1[0x19] = param_5;
  uVar2 = *(uint *)(lRam0000000001489588 + 0x1ba7f58);
  if (*(int *)(&UNK_006e47b8 + lRam0000000001489598) != 1) {
    uVar1 = (uVar2 ^ uVar2 >> 1) & 0x45405441;
    *(uint *)(lRam0000000001489590 + 0x1834ca8) = (uVar1 | uVar1 << 1) ^ uVar2;
  }
  param_1[0x1d] = *(long *)(&UNK_01247820 + (ulong)param_8 * 8);
  *(undefined1 *)(param_1 + 0x14) = 1;
  *(undefined1 *)(param_1 + 0x1c) = 1;
  return;
}


