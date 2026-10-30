// FUN_0004d14c size=504

void FUN_0004d14c(long param_1,undefined8 param_2,long param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  char cVar7;
  char cVar8;
  long lVar9;
  undefined8 uVar10;
  
  lVar9 = FUN_003bbf58(&DAT_013c12b0);
  *(undefined1 *)(param_1 + 0x22b) = *(undefined1 *)(lVar9 + 0x22b);
  FUN_00321cf0(param_1 + 0x230,lVar9 + 0x230);
  *(undefined1 *)(param_1 + 0x22a) = *(undefined1 *)(lVar9 + 0x22a);
  FUN_00052a94(param_1,param_2,param_3);
  uVar5 = *(uint *)(DAT_0141a708 + 0x7e2980);
  uVar6 = *(uint *)(DAT_0141a718 + 0x1774e08);
  DAT_0141a700 = (int)(CONCAT44((uVar6 + (uVar6 >> 0x10)) * 0x101 + (uVar6 >> 0x18),uVar6) >> 0x10);
  if (DAT_0141a700 != 0) {
    uVar6 = (uVar5 ^ uVar5 >> 1) & 0x44005114;
    *(uint *)(DAT_0141a710 + 0x7df7b0) = (uVar6 | uVar6 << 1) ^ uVar5;
  }
  iVar3 = *(int *)(param_1 + 0x1d0);
  iVar1 = *(int *)(param_1 + 0x1d4);
  iVar2 = *(int *)(param_1 + 0x1d8);
  iVar4 = *(int *)(param_1 + 0x1dc);
  cVar7 = *(char *)(param_1 + 0x1a8);
  cVar8 = *(char *)(param_1 + 0x1e0);
  uVar10 = *(undefined8 *)(lVar9 + 0x1d0);
  *(undefined8 *)(param_1 + 0x1d8) = *(undefined8 *)(lVar9 + 0x1d8);
  *(undefined8 *)(param_1 + 0x1d0) = uVar10;
  *(undefined1 *)(param_1 + 0x1e0) = *(undefined1 *)(lVar9 + 0x1e0);
  *(undefined1 *)(param_1 + 0x1a8) = *(undefined1 *)(lVar9 + 0x1a8);
  *(undefined1 *)(param_1 + 0x1e2) = *(undefined1 *)(lVar9 + 0x1e2);
  *(undefined1 *)(param_1 + 0x1e1) = *(undefined1 *)(lVar9 + 0x1e1);
  *(undefined1 *)(param_1 + 0x1e3) = *(undefined1 *)(lVar9 + 0x1e3);
  *(undefined1 *)(param_1 + 0x1e4) = *(undefined1 *)(lVar9 + 0x1e4);
  FUN_002e79b0(param_1 + 0x1f0,lVar9 + 0x1f0);
  if (*(char *)(param_3 + 8) != '\0') {
    uVar10 = *(undefined8 *)(lVar9 + 0x1f8);
    *(undefined8 *)(param_1 + 0x200) = *(undefined8 *)(lVar9 + 0x200);
    *(undefined8 *)(param_1 + 0x1f8) = uVar10;
  }
  *(undefined1 *)(param_1 + 0x228) = *(undefined1 *)(lVar9 + 0x228);
  *(undefined1 *)(param_1 + 0x229) = *(undefined1 *)(lVar9 + 0x229);
  FUN_00056870(*(undefined8 *)(param_1 + 400),*(long *)(lVar9 + 400) + 0x50);
  *(undefined1 *)(param_1 + 0x259) = *(undefined1 *)(lVar9 + 0x259);
  if ((((iVar3 == *(int *)(param_1 + 0x1d0)) && (iVar2 == *(int *)(param_1 + 0x1d8))) &&
      (iVar1 == *(int *)(param_1 + 0x1d4))) &&
     (((iVar4 == *(int *)(param_1 + 0x1dc) && (cVar7 == *(char *)(param_1 + 0x1a8))) &&
      (cVar8 == *(char *)(param_1 + 0x1e0))))) {
    return;
  }
  FUN_001d1d68(param_1,*(undefined1 *)(param_1 + 0x1a8));
  return;
}


