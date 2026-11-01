// FUN_0015e87c size=772

void FUN_0015e87c(long param_1)

{
  int iVar1;
  int *piVar2;
  ulong uVar3;
  long lVar4;
  undefined8 uVar5;
  int *piStack_38;
  int *piStack_30;
  int *piStack_28;
  
  if (*(char *)(param_1 + 0x30) == '\x01') {
    func_0x009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0x97e1061f,0);
  }
  func_0x0044d170(&piStack_30,9);
  func_0x0033db70(&piStack_28,&piStack_30);
  piVar2 = *(int **)(param_1 + 0x20);
  *(int **)(param_1 + 0x20) = piStack_28;
  piStack_28 = piVar2;
  if (*piVar2 != -1) {
    if (*piVar2 != 0) {
      LOAcquire();
      iVar1 = *piVar2;
      *piVar2 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0015e91c;
    }
    func_0x00287df8(piVar2,2,8);
  }
LAB_0015e91c:
  if (*piStack_30 != -1) {
    if (*piStack_30 != 0) {
      LOAcquire();
      iVar1 = *piStack_30;
      *piStack_30 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0015e950;
    }
    func_0x00287df8(piStack_30,2,8);
  }
LAB_0015e950:
  uVar3 = func_0x00984b5c();
  if ((uVar3 & 1) == 0) {
    func_0x0044d170(&piStack_30,1);
    func_0x0033db70(&piStack_28,&piStack_30);
    piVar2 = *(int **)(param_1 + 0x18);
    *(int **)(param_1 + 0x18) = piStack_28;
    piStack_28 = piVar2;
    if (*piVar2 != -1) {
      if (*piVar2 != 0) {
        LOAcquire();
        iVar1 = *piVar2;
        *piVar2 = iVar1 + -1;
        LORelease();
        if (iVar1 != 1) goto LAB_0015e9d0;
      }
      func_0x00287df8(piVar2,2,8);
    }
LAB_0015e9d0:
    if (*piStack_30 != -1) {
      if (*piStack_30 != 0) {
        LOAcquire();
        iVar1 = *piStack_30;
        *piStack_30 = iVar1 + -1;
        LORelease();
        goto joined_r0x0015e9f0;
      }
      goto LAB_0015eac8;
    }
  }
  else {
    _getuid();
    lVar4 = _getpwuid();
    lVar4 = *(long *)(lVar4 + 0x30);
    if (lVar4 == 0) {
      uVar5 = 0xffffffff;
    }
    else {
      uVar5 = _strlen(lVar4);
    }
    piStack_38 = (int *)func_0x002efa1c(lVar4,uVar5);
    func_0x001c0fe4(&piStack_30,&piStack_38,"/Documents");
    func_0x0033db70(&piStack_28,&piStack_30);
    piVar2 = *(int **)(param_1 + 0x18);
    *(int **)(param_1 + 0x18) = piStack_28;
    piStack_28 = piVar2;
    if (*piVar2 != -1) {
      if (*piVar2 != 0) {
        LOAcquire();
        iVar1 = *piVar2;
        *piVar2 = iVar1 + -1;
        LORelease();
        if (iVar1 != 1) goto LAB_0015ea6c;
      }
      func_0x00287df8(piVar2,2,8);
    }
LAB_0015ea6c:
    if (*piStack_30 != -1) {
      if (*piStack_30 != 0) {
        LOAcquire();
        iVar1 = *piStack_30;
        *piStack_30 = iVar1 + -1;
        LORelease();
        if (iVar1 != 1) goto LAB_0015eaa0;
      }
      func_0x00287df8(piStack_30,2,8);
    }
LAB_0015eaa0:
    if (*piStack_38 != -1) {
      piStack_30 = piStack_38;
      if (*piStack_38 != 0) {
        LOAcquire();
        iVar1 = *piStack_38;
        *piStack_38 = iVar1 + -1;
        LORelease();
joined_r0x0015e9f0:
        if (iVar1 != 1) goto LAB_0015ead4;
      }
LAB_0015eac8:
      func_0x00287df8(piStack_30,2,8);
    }
  }
LAB_0015ead4:
  piStack_30 = (int *)func_0x002efa1c("/Library/Application Support/ABSoft/NeatVideo6-OFX",0x32);
  func_0x0033db70(&piStack_28,&piStack_30);
  piVar2 = *(int **)(param_1 + 0x28);
  *(int **)(param_1 + 0x28) = piStack_28;
  piStack_28 = piVar2;
  if (*piVar2 != -1) {
    if (*piVar2 != 0) {
      LOAcquire();
      iVar1 = *piVar2;
      *piVar2 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0015eb34;
    }
    func_0x00287df8(piVar2,2,8);
  }
LAB_0015eb34:
  if (*piStack_30 != -1) {
    if (*piStack_30 != 0) {
      LOAcquire();
      iVar1 = *piStack_30;
      *piStack_30 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0015eb68;
    }
    func_0x00287df8(piStack_30,2,8);
  }
LAB_0015eb68:
  *(undefined1 *)(param_1 + 0x30) = 1;
  return;
}


