// func entry: 0011fa48 name: FUN_0011fa48 size: 1136

void FUN_0011fa48(long param_1,undefined8 param_2)

{
  int iVar1;
  undefined8 uVar2;
  int *local_28;
  
  FUN_003b8988(&local_28,"RegistrationDialog","xxx",0,0xffffffff);
  FUN_00750130(param_2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011fac0;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0011fac0:
  uVar2 = *(undefined8 *)(param_1 + 0x18);
  FUN_003b8988(&local_28,"RegistrationDialog","Number of copies:",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011fb24;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0011fb24:
  uVar2 = *(undefined8 *)(param_1 + 0x20);
  FUN_003b8988(&local_28,"RegistrationDialog","Registration code:",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011fb88;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0011fb88:
  uVar2 = *(undefined8 *)(param_1 + 0x28);
  FUN_003b8988(&local_28,"RegistrationDialog",
               "Please enter user data, number of copies\nand registration code from your license for:"
               ,0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011fbec;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0011fbec:
  uVar2 = *(undefined8 *)(param_1 + 0x30);
  FUN_003b8988(&local_28,"RegistrationDialog","Neat Image v6.0 Pro plug-in",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011fc50;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0011fc50:
  uVar2 = *(undefined8 *)(param_1 + 0x38);
  FUN_003b8988(&local_28,"RegistrationDialog","Name:",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011fcb4;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0011fcb4:
  uVar2 = *(undefined8 *)(param_1 + 0x40);
  FUN_003b8988(&local_28,"RegistrationDialog",
               "Please enter the whole Name specified in your license (case-sensitive)",0,0xffffffff
              );
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011fd18;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0011fd18:
  uVar2 = *(undefined8 *)(param_1 + 0x48);
  FUN_003b8988(&local_28,"RegistrationDialog","Organization:",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011fd7c;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0011fd7c:
  uVar2 = *(undefined8 *)(param_1 + 0x50);
  FUN_003b8988(&local_28,"RegistrationDialog",
               "Please enter the Organization name specified in your license (case-sensitive).\nLeave empty if that name is not specified in the license."
               ,0,0xffffffff);
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011fde0;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0011fde0:
  uVar2 = *(undefined8 *)(param_1 + 0x80);
  FUN_003b8988(&local_28,"RegistrationDialog","Where can I find the license data?",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011fe44;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0011fe44:
  uVar2 = *(undefined8 *)(param_1 + 0x90);
  FUN_003b8988(&local_28,"RegistrationDialog","1",0,0xffffffff);
  FUN_0081046c(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) {
        return;
      }
    }
    FUN_00287df8(local_28,2,8);
  }
  return;
}


