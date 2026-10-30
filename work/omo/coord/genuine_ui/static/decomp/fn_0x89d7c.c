// FUN_00089d7c size=176

undefined4 FUN_00089d7c(undefined8 param_1,int *param_2)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  
  uVar1 = *(uint *)(&UNK_00783418 + lRam000000000141ae88);
  uVar2 = *(uint *)(&LAB_003b6eb8 + lRam000000000141ae98);
  iVar3 = (int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10);
  if (iVar3 != 1) {
    if (iRam000000000141ae80 != 0) {
      uVar1 = uVar1 >> 0x18 | uVar1 << 8;
    }
    *(uint *)(lRam000000000141ae90 + 0x169bb68) = uVar1;
  }
  if (lRam00000000015646d0 != 0) {
    iVar3 = iVar3 + (uint)(byte)(&UNK_00002a1d)[lRam00000000015646d0];
  }
  iRam000000000141ae80 = iVar3;
  if (*param_2 - 1U < 6) {
    return *(undefined4 *)(&UNK_00c943f8 + (long)(int)(*param_2 - 1U) * 4);
  }
  return 0;
}


