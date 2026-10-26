// FUN_0004b8a8 size=388

void FUN_0004b8a8(undefined1 param_1 [16],double param_2,long *param_3,undefined8 param_4)

{
  undefined8 uVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  long lVar6;
  double dVar7;
  int local_38;
  uint uStack_34;
  
  if (((int)*(uint *)(param_3 + 0x54) < 1) ||
     (iVar3 = *(int *)((long)param_3 + 0x2a4), uVar4 = *(uint *)(param_3 + 0x54),
     *(int *)((long)param_3 + 0x2a4) < 1)) {
    FUN_0004c820(param_3,&uStack_34,&local_38);
    iVar3 = local_38;
    uVar4 = uStack_34;
  }
  uStack_34 = uVar4;
  local_38 = iVar3;
  dVar7 = (double)FUN_0095d6ec(param_3,param_4);
  uVar4 = uStack_34;
  if ((int)uStack_34 < 0) {
    uVar4 = uStack_34 + 1;
  }
  iVar2 = (int)(dVar7 - (double)((int)uVar4 >> 1));
  iVar3 = local_38;
  if (local_38 < 0) {
    iVar3 = local_38 + 1;
  }
  iVar3 = (int)(param_2 - (double)(iVar3 >> 1));
  if ((param_3[0x29] == 0) || (lVar6 = *(long *)(param_3[0x29] + 8), lVar6 == 0)) {
    iVar5 = 0;
    uVar4 = uStack_34 & (int)uStack_34 >> 0x1f;
  }
  else {
    uVar4 = *(uint *)(lVar6 + 0xd0);
    if ((int)uStack_34 <= (int)*(uint *)(lVar6 + 0xd0)) {
      uVar4 = uStack_34;
    }
    iVar5 = *(int *)(lVar6 + 0xd8);
  }
  if (local_38 <= iVar5) {
    iVar5 = local_38;
  }
  *(int *)(param_3 + 0x3a) = iVar2;
  *(int *)((long)param_3 + 0x1d4) = iVar3;
  *(uint *)(param_3 + 0x3b) = iVar2 + uVar4 + -1;
  *(int *)((long)param_3 + 0x1dc) = iVar3 + iVar5 + -1;
  uVar1 = FUN_0004c770(param_3,CONCAT44(iVar3,iVar2));
  *(int *)(param_3 + 0x3b) = ((int)uVar1 - (int)param_3[0x3a]) + (int)param_3[0x3b];
  *(int *)(param_3 + 0x3a) = (int)uVar1;
  iVar3 = (int)((ulong)uVar1 >> 0x20);
  *(int *)((long)param_3 + 0x1dc) =
       (iVar3 - *(int *)((long)param_3 + 0x1d4)) + *(int *)((long)param_3 + 0x1dc);
  *(int *)((long)param_3 + 0x1d4) = iVar3;
  *(undefined2 *)((long)param_3 + 0x1e2) = 0;
  *(undefined1 *)((long)param_3 + 0x1e4) = 0;
  *(undefined1 *)(param_3 + 0x3c) = 1;
  FUN_00051f04(dVar7,param_2,param_3,1);
  FUN_001d1d68(param_3,0);
  (**(code **)(*param_3 + 0x1e8))(param_3);
  FUN_000506f8(param_3);
  return;
}


