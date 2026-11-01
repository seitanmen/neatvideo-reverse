// FUN_0015ec38 size=652

void FUN_0015ec38(char *param_1)

{
  int iVar1;
  char *pcVar2;
  char *pcVar3;
  int *piStack_28;
  
  if (param_1[0x80] == '\x01') {
    func_0x009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0x4fc3773d,0);
  }
  if (*param_1 == '\0') {
    if (param_1[0x30] == '\0') {
      func_0x009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0x632bc5c7,0);
    }
    pcVar3 = param_1 + 0x18;
    func_0x001c0fe4(&piStack_28,pcVar3,"/Neat Video v6 OpenFX/Profiles");
    func_0x002e79b0(param_1 + 0x68,&piStack_28);
    if (*piStack_28 != -1) {
      if (*piStack_28 != 0) {
        LOAcquire();
        iVar1 = *piStack_28;
        *piStack_28 = iVar1 + -1;
        LORelease();
        if (iVar1 != 1) goto LAB_0015edd4;
      }
      func_0x00287df8(piStack_28,2,8);
    }
LAB_0015edd4:
    if (param_1[0x30] == '\0') {
      func_0x009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0x632bc5c7,0);
    }
    pcVar2 = "/Neat Video v6 OpenFX/Presets";
  }
  else {
    if (param_1[0x10] == '\0') {
      func_0x009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0x87b8f436,0);
    }
    pcVar3 = param_1 + 8;
    func_0x001c0fe4(&piStack_28,pcVar3,"/Profiles");
    func_0x002e79b0(param_1 + 0x68,&piStack_28);
    if (*piStack_28 != -1) {
      if (*piStack_28 != 0) {
        LOAcquire();
        iVar1 = *piStack_28;
        *piStack_28 = iVar1 + -1;
        LORelease();
        if (iVar1 != 1) goto LAB_0015ed10;
      }
      func_0x00287df8(piStack_28,2,8);
    }
LAB_0015ed10:
    if (param_1[0x10] == '\0') {
      func_0x009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0x87b8f436,0);
    }
    pcVar2 = "/Presets";
  }
  func_0x001c0fe4(&piStack_28,pcVar3,pcVar2);
  func_0x002e79b0(param_1 + 0x78,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0015ee58;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_0015ee58:
  func_0x001c0fe4(&piStack_28,param_1 + 0x68,"/Generic Profiles");
  func_0x002e79b0(param_1 + 0x70,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0015eeac;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_0015eeac:
  param_1[0x80] = '\x01';
  return;
}


