// FUN_0004aebc size=156

void FUN_0004aebc(long param_1,long *param_2)

{
  undefined4 uVar1;
  long lVar2;
  long *plVar3;
  undefined8 uVar4;
  long lVar5;
  long lVar6;
  
  lVar5 = param_2[2];
  lVar2 = param_2[4];
  uVar1 = *(undefined4 *)((long)param_2 + 0x24);
  plVar3 = (long *)__Znwm(0x48);
  lVar6 = *param_2;
  plVar3[3] = param_2[1];
  plVar3[2] = lVar6;
  *(int *)(plVar3 + 4) = (int)lVar5;
  uVar4 = *(undefined8 *)((long)param_2 + 0x14);
  *(undefined2 *)((long)plVar3 + 0x2c) = *(undefined2 *)((long)param_2 + 0x1c);
  *(undefined8 *)((long)plVar3 + 0x24) = uVar4;
  *(char *)(plVar3 + 6) = (char)lVar2;
  *(undefined4 *)((long)plVar3 + 0x34) = uVar1;
  lVar5 = param_2[6];
  plVar3[7] = param_2[5];
  *(short *)(plVar3 + 8) = (short)lVar5;
  lVar5 = *(long *)(param_1 + 0x1b8);
  *plVar3 = lVar5;
  plVar3[1] = param_1 + 0x1b8;
  *(long **)(lVar5 + 8) = plVar3;
  *(long **)(param_1 + 0x1b8) = plVar3;
  *(long *)(param_1 + 0x1c8) = *(long *)(param_1 + 0x1c8) + 1;
  FUN_000492d4(param_1);
  return;
}


