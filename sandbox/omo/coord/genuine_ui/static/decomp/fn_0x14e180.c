// FUN_0014e180 size=532

long FUN_0014e180(long param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4,long param_5
                 )

{
  uint uVar1;
  bool bVar2;
  uint uVar3;
  undefined8 *puVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  
  puVar4 = (undefined8 *)func_0x002802c8(param_1,0);
  *puVar4 = &UNK_013c8bc8;
  *(undefined8 *)(param_1 + 0x154) = 0;
  *(undefined8 *)(param_1 + 0x15c) = 0xffffffffffffffff;
  *(undefined8 *)(param_1 + 0x164) = 0;
  *(undefined8 *)(param_1 + 0x16c) = 0xffffffffffffffff;
  *(undefined1 *)((long)puVar4 + 0x6c) = 1;
  *(undefined2 *)((long)puVar4 + 0x152) = 0;
  *(undefined8 *)((long)puVar4 + 0x3c) = 0;
  *(undefined8 *)((long)puVar4 + 0x34) = 0;
  *(undefined8 *)((long)puVar4 + 0x4c) = 0;
  *(undefined8 *)((long)puVar4 + 0x44) = 0;
  *(undefined8 *)((long)puVar4 + 0x5c) = 0;
  *(undefined8 *)((long)puVar4 + 0x54) = 0;
  *(undefined4 *)((long)puVar4 + 100) = 0;
  uVar3 = *(uint *)(&UNK_01125120 + lRam000000000141c1b0);
  uVar1 = (uVar3 ^ uVar3 >> 1) & 0x41505414;
  uVar3 = (uVar1 | uVar1 << 1) ^ uVar3;
  iRam000000000141c1a8 =
       (int)(CONCAT44((uVar3 + (uVar3 >> 0x10)) * 0x101 + (uVar3 >> 0x18),uVar3) >> 0x10);
  if (iRam000000000141c1a8 != 0) {
    *(uint *)(&UNK_0006b250 + lRam000000000141c1b8) = uVar3;
  }
  *(undefined2 *)(param_1 + 0x139) = 0;
  *(undefined1 *)(param_1 + 0x13b) = 1;
  *(undefined1 *)(param_1 + 0x6d) = 0;
  *(undefined4 *)(param_1 + 0x188) = 1;
  *(undefined4 *)(param_1 + 0x180) = 3;
  func_0x00280478(param_1,0x200000);
  uVar5 = __Znwm(&UNK_0000d528);
  func_0x0099df20();
  *(undefined8 *)(param_1 + 0x10) = uVar5;
  uVar6 = __Znwm(0xb0c);
  *(undefined8 *)(param_1 + 0x18) = uVar6;
  func_0x0099fb7c(uVar5,param_2);
  _memcpy(*(undefined8 *)(param_1 + 0x18),param_3,0xb0c);
  *(undefined8 *)(param_1 + 0x20) = param_4;
  *(undefined4 *)(param_1 + 0x68) = 0;
  bVar2 = param_5 == 0;
  if (bVar2) {
    param_5 = __Znwm(&UNK_00025d00);
    func_0x0099e030();
    *(long *)(param_1 + 0x70) = param_5;
  }
  else {
    *(long *)(param_1 + 0x70) = param_5;
  }
  *(bool *)(param_1 + 0x78) = bVar2;
  func_0x0099e034(param_5,*(undefined8 *)(param_1 + 0x10));
  func_0x0099e0a8(*(undefined8 *)(param_1 + 0x70),&UNK_012445a8);
  func_0x0099e10c(*(undefined8 *)(param_1 + 0x70),*(undefined8 *)(param_1 + 0x18));
  *(undefined1 *)(*(long *)(param_1 + 0x70) + 0x80) = 0;
  *(undefined2 *)(param_1 + 0x150) = 0;
  *(undefined8 *)(param_1 + 0x98) = 0;
  *(undefined8 *)(param_1 + 0x90) = 0;
  *(undefined8 *)(param_1 + 0xa8) = 0;
  *(undefined8 *)(param_1 + 0xa0) = 0;
  *(undefined8 *)(param_1 + 0xb8) = 0;
  *(undefined8 *)(param_1 + 0xb0) = 0;
  *(undefined8 *)(param_1 + 200) = 0;
  *(undefined8 *)(param_1 + 0xc0) = 0;
  *(undefined8 *)(param_1 + 0xd8) = 0;
  *(undefined8 *)(param_1 + 0xd0) = 0;
  *(undefined8 *)(param_1 + 0xe8) = 0;
  *(undefined8 *)(param_1 + 0xe0) = 0;
  *(undefined8 *)(param_1 + 0xf8) = 0;
  *(undefined8 *)(param_1 + 0xf0) = 0;
  *(undefined8 *)(param_1 + 0x108) = 0;
  *(undefined8 *)(param_1 + 0x100) = 0;
  *(undefined8 *)(param_1 + 0x88) = 0;
  *(undefined8 *)(param_1 + 0x80) = 0;
  *(undefined8 *)(param_1 + 0x114) = 0;
  *(undefined8 *)(param_1 + 0x10c) = 0;
  *(undefined8 *)(param_1 + 0x128) = 0;
  *(undefined8 *)(param_1 + 0x130) = 0;
  *(undefined8 *)(param_1 + 0x120) = 0;
  *(undefined8 *)(param_1 + 0x28) = 0x7fffffffffffffff;
  *(undefined1 *)(param_1 + 0x30) = 0;
  return param_1;
}


