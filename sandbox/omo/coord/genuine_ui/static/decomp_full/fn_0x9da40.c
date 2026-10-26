// FUN_0009da40 size=416

void FUN_0009da40(long *param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  long *plVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  long *plVar7;
  long lVar8;
  
  iVar3 = (**(code **)(*param_1 + 0x1c0))();
  plVar7 = (long *)param_1[(long)iVar3 + 0x56];
  FUN_0008d0e8(plVar7,param_2,param_3,0);
  plVar4 = (long *)param_1[0x56];
  if (plVar4 != (long *)0x0 && plVar4 != plVar7) {
    FUN_0008d0e8(plVar4,(int)plVar4[0x3b],param_3,0);
  }
  plVar4 = (long *)param_1[0x57];
  if (plVar4 != (long *)0x0 && plVar4 != plVar7) {
    FUN_0008d0e8(plVar4,(int)plVar4[0x3b],param_3,0);
  }
  plVar4 = (long *)param_1[0x58];
  if ((plVar4 != (long *)0x0) && (plVar4 != plVar7)) {
    FUN_0008d0e8(plVar4,(int)plVar4[0x3b],param_3,0);
  }
  plVar4 = (long *)param_1[0x59];
  if ((plVar4 != (long *)0x0) && (plVar4 != plVar7)) {
    FUN_0008d0e8(plVar4,(int)plVar4[0x3b],param_3,0);
  }
  uVar2 = *(uint *)(DAT_0141b618 + 0x17b9868);
  uVar1 = (uVar2 ^ uVar2 >> 1) & 0x44050415;
  DAT_0141b610 = (uVar1 | uVar1 << 1) ^ uVar2;
  if (DAT_015656f0 != 0) {
    DAT_0141b610 = DAT_0141b610 + *(byte *)(DAT_015656f0 + 0x88e9);
  }
  lVar8 = param_1[0x5d];
  iVar3 = (**(code **)(*param_1 + 0x1c0))(param_1);
  uVar5 = FUN_00089d7c(plVar7,plVar7 + 0x3b);
  uVar6 = (**(code **)(*plVar7 + 0x220))(plVar7);
  FUN_00179968(lVar8,(long)iVar3,uVar5,uVar6,param_4);
  if (param_1[0x20] != 0) {
    FUN_000a7b4c(param_1[0x20],plVar7[0x34]);
    return;
  }
  return;
}


