// FUN_00052338 size=180

int FUN_00052338(undefined8 param_1,int param_2,int param_3,int param_4,int param_5)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  ulong uVar4;
  uint uVar5;
  
  uVar3 = DAT_0141a8e6;
  iVar1 = (param_3 + param_2) - param_5;
  if (param_5 + param_4 <= param_3 + param_2) {
    iVar1 = param_4;
  }
  iVar2 = param_2;
  if (param_2 <= param_4) {
    iVar2 = iVar1;
  }
  if (DAT_0141a8e0 >> 0x13 < 0x19) {
    uVar4 = (ulong)DAT_0141a8e6;
    *(undefined **)(DAT_0141a8f8 + 0x564bc28) = &DAT_0141a8e4;
    uVar5 = uVar3;
    if (*(long *)(DAT_0141a8f0 + 0x4ef4240) != 0) {
      uVar5 = -(uint)*(byte *)(*(long *)(DAT_0141a8f0 + 0x4ef4240) + (uVar4 & 0xffff));
    }
    iVar2 = iVar2 + ((uVar5 + (uVar3 >> 0x10 & 0xff)) * 2 + (uVar3 >> 0x18)) * 0x1a5;
  }
  DAT_0141a8e0 = (int)(CONCAT44((DAT_0141a8e0 + (DAT_0141a8e0 >> 0x10)) * 0x101 +
                                (DAT_0141a8e0 >> 0x18),DAT_0141a8e0) >> 0x10) + param_2;
  return iVar2;
}


