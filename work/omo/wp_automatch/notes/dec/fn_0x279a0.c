// FUN_000279a0 size=216

ulong FUN_000279a0(long param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4,
                  undefined8 param_5)

{
  int iVar1;
  ulong uVar2;
  undefined8 uVar3;
  int *piVar4;
  int *piStack_38;
  
  piStack_38 = *(int **)(param_1 + 0x18);
  piVar4 = piStack_38;
  if (1 < *piStack_38 + 1U) {
    LOAcquire();
    iVar1 = *piStack_38;
    *piStack_38 = iVar1 + 1;
    piVar4 = (int *)CONCAT44((int)((ulong)piStack_38 >> 0x20),iVar1);
    LORelease();
  }
  uVar2 = func_0x00026954(piVar4,param_1,&piStack_38,param_2,param_3,0,0xffffffff,param_4,param_5);
  if (*piStack_38 != -1) {
    if (*piStack_38 != 0) {
      LOAcquire();
      iVar1 = *piStack_38;
      *piStack_38 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_00027a34;
    }
    func_0x00287df8(piStack_38,2,8);
  }
LAB_00027a34:
  if ((uVar2 & 1) == 0) {
    uVar3 = func_0x00026858((undefined8 *)(param_1 + 0x18));
    func_0x0000f43c(param_2,0,uVar3,0,0,0xffffffff,0xffffffff);
  }
  return uVar2;
}


