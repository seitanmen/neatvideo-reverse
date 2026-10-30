// FUN_0014fbb0 size=304

void FUN_0014fbb0(long param_1)

{
  uint uVar1;
  uint uVar2;
  long lVar3;
  undefined8 *puVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  undefined8 uVar7;
  undefined8 uVar8;
  
  *(undefined1 *)(param_1 + 0x6c) = 0;
  if (*(char *)(param_1 + 0x6d) == '\0') {
    if (*(long *)(param_1 + 0x20) != 0) {
      func_0x0099e1f0(*(undefined8 *)(param_1 + 0x70));
    }
    switch(*(undefined4 *)(param_1 + 0x68)) {
    case 1:
    case 2:
      func_0x0014e4a4(param_1);
      break;
    case 3:
      func_0x0014ebb4(param_1);
      break;
    case 4:
      func_0x0014ed44(param_1);
      break;
    case 5:
      func_0x0014ee9c(param_1);
      break;
    case 6:
      func_0x0014f040(param_1);
      break;
    case 7:
      FUN_0014f17c(param_1);
      break;
    case 8:
      FUN_0014f4bc(param_1);
      break;
    case 9:
      FUN_0014f9a0(param_1);
      break;
    default:
      func_0x009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0x92b29812);
    }
    puVar4 = *(undefined8 **)(param_1 + 0x20);
    if (puVar4 != (undefined8 *)0x0) {
      lVar3 = *(long *)(param_1 + 0x70);
      uVar1 = *(uint *)(&UNK_013f0298 + lRam0000000001488530);
      uVar2 = *(uint *)(&UNK_007da118 + lRam0000000001488540);
      if ((int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10) != 1) {
        uVar2 = (uVar1 ^ uVar1 >> 1) & 0x44151105;
        *(uint *)(&UNK_00fed228 + lRam0000000001488538) = (uVar2 | uVar2 << 1) ^ uVar1;
      }
      uVar6 = *(undefined8 *)(&UNK_000114c8 + lVar3);
      uVar5 = *(undefined8 *)(&DAT_000114c0 + lVar3);
      uVar8 = *(undefined8 *)(&UNK_000114d8 + lVar3);
      uVar7 = *(undefined8 *)(&UNK_000114d0 + lVar3);
      puVar4[4] = *(undefined8 *)(&UNK_000114e0 + lVar3);
      puVar4[1] = uVar6;
      *puVar4 = uVar5;
      puVar4[3] = uVar8;
      puVar4[2] = uVar7;
      return;
    }
  }
  else {
    *(undefined1 *)(param_1 + 0x152) = 0;
  }
  return;
}


