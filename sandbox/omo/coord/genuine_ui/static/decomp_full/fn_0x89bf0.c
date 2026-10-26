// FUN_00089bf0 size=396

ulong FUN_00089bf0(undefined8 param_1,undefined4 *param_2,ulong param_3)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  
  uVar1 = *(uint *)(DAT_0141ae60 + 0x1d94a40);
  uVar3 = (uVar1 ^ uVar1 >> 1) & 0x41510004;
  uVar1 = (uVar3 | uVar3 << 1) ^ uVar1;
  DAT_0141ae58 = (int)(CONCAT44((uVar1 + (uVar1 >> 0x10)) * 0x101 + (uVar1 >> 0x18),uVar1) >> 0x10);
  if (DAT_0141ae58 != 0) {
    *(uint *)(DAT_0141ae68 + 0x8c7270) = uVar1;
  }
  iVar2 = (int)param_3;
  switch(*param_2) {
  case 1:
    if (iVar2 - 1U < 3) {
      return (ulong)(iVar2 + 0xf);
    }
    break;
  case 2:
    if (iVar2 == 3) {
      return param_3;
    }
    if (iVar2 == 2) {
      uVar3 = 1;
      if (param_2[1] != 0) {
        uVar3 = 2;
      }
      return (ulong)uVar3;
    }
    if (iVar2 == 1) {
      uVar3 = 1;
      if (param_2[1] == 0) {
        uVar3 = 2;
      }
      return (ulong)uVar3;
    }
    break;
  case 3:
    if (iVar2 - 1U < 3) {
      return (ulong)(iVar2 + 3);
    }
    break;
  case 4:
    if (iVar2 - 1U < 3) {
      return (ulong)(iVar2 + 6);
    }
    break;
  case 5:
    if (iVar2 - 1U < 3) {
      return (ulong)(iVar2 + 9);
    }
    break;
  case 6:
    if (iVar2 - 1U < 3) {
      return (ulong)(iVar2 + 0xc);
    }
  }
  uVar1 = *(uint *)(DAT_0141ae70 + 0x14a1a20);
  uVar3 = (uVar1 ^ uVar1 >> 1) & 0x41111554;
  uVar1 = (uVar3 | uVar3 << 1) ^ uVar1;
  if ((int)(CONCAT44((uVar1 + (uVar1 >> 0x10)) * 0x101 + (uVar1 >> 0x18),uVar1) >> 0x10) == 0) {
    return 0;
  }
  *(uint *)(DAT_0141ae78 + 0x34b350) = uVar1;
  return 0;
}


