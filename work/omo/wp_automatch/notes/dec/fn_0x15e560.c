// FUN_0015e560 size=660

void FUN_0015e560(char *param_1,undefined8 param_2)

{
  undefined8 uVar1;
  int iVar2;
  int *piVar3;
  int *piStack_38;
  int *piStack_30;
  int *piStack_28;
  
  if (param_1[0x10] == '\x01') {
    func_0x009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0x44905e3f,0);
  }
  uVar1 = uRam000000000155b5c0;
  if (*param_1 == '\0') {
    piStack_30 = (int *)func_0x002efa1c("InitDir",7);
    piStack_38 = (int *)func_0x002efa1c("",0);
    func_0x00180d2c(&piStack_28,uVar1,&piStack_30,&piStack_38);
    piVar3 = *(int **)(param_1 + 8);
    *(int **)(param_1 + 8) = piStack_28;
    piStack_28 = piVar3;
    if (*piVar3 != -1) {
      if (*piVar3 != 0) {
        LOAcquire();
        iVar2 = *piVar3;
        *piVar3 = iVar2 + -1;
        LORelease();
        if (iVar2 != 1) goto LAB_0015e67c;
      }
      func_0x00287df8(piVar3,2,8);
    }
LAB_0015e67c:
    if (*piStack_38 != -1) {
      if (*piStack_38 != 0) {
        LOAcquire();
        iVar2 = *piStack_38;
        *piStack_38 = iVar2 + -1;
        LORelease();
        if (iVar2 != 1) goto LAB_0015e6b0;
      }
      func_0x00287df8(piStack_38,2,8);
    }
LAB_0015e6b0:
    if (*piStack_30 != -1) {
      if (*piStack_30 != 0) {
        LOAcquire();
        iVar2 = *piStack_30;
        *piStack_30 = iVar2 + -1;
        LORelease();
        if (iVar2 != 1) goto LAB_0015e6e4;
      }
      func_0x00287df8(piStack_30,2,8);
    }
LAB_0015e6e4:
    uVar1 = uRam000000000155b5a8;
    if (*(int *)(*(long *)(param_1 + 8) + 4) != 0) goto LAB_0015e7dc;
    piStack_30 = (int *)func_0x002efa1c("InitDir",7);
    piStack_38 = (int *)func_0x002efa1c("",0);
    func_0x00180d2c(&piStack_28,uVar1,&piStack_30,&piStack_38);
    piVar3 = *(int **)(param_1 + 8);
    *(int **)(param_1 + 8) = piStack_28;
    piStack_28 = piVar3;
    if (*piVar3 != -1) {
      if (*piVar3 != 0) {
        LOAcquire();
        iVar2 = *piVar3;
        *piVar3 = iVar2 + -1;
        LORelease();
        if (iVar2 != 1) goto LAB_0015e774;
      }
      func_0x00287df8(piVar3,2,8);
    }
LAB_0015e774:
    if (*piStack_38 != -1) {
      if (*piStack_38 != 0) {
        LOAcquire();
        iVar2 = *piStack_38;
        *piStack_38 = iVar2 + -1;
        LORelease();
        if (iVar2 != 1) goto LAB_0015e7a8;
      }
      func_0x00287df8(piStack_38,2,8);
    }
LAB_0015e7a8:
    if (*piStack_30 == -1) goto LAB_0015e7dc;
    piVar3 = piStack_30;
    if (*piStack_30 != 0) {
      LOAcquire();
      iVar2 = *piStack_30;
      *piStack_30 = iVar2 + -1;
      LORelease();
      goto joined_r0x0015e7c8;
    }
  }
  else {
    func_0x0033cfd0(&piStack_28,param_2);
    piVar3 = *(int **)(param_1 + 8);
    *(int **)(param_1 + 8) = piStack_28;
    if (*piVar3 == -1) goto LAB_0015e7dc;
    piStack_28 = piVar3;
    if (*piVar3 != 0) {
      LOAcquire();
      iVar2 = *piVar3;
      *piVar3 = iVar2 + -1;
      LORelease();
joined_r0x0015e7c8:
      if (iVar2 != 1) goto LAB_0015e7dc;
    }
  }
  func_0x00287df8(piVar3,2,8);
LAB_0015e7dc:
  param_1[0x10] = '\x01';
  return;
}


