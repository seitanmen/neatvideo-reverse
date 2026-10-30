// func entry: 0012e94c name: FUN_0012e94c size: 1648

void FUN_0012e94c(long param_1)

{
  int iVar1;
  undefined8 uVar2;
  int *local_28;
  
  local_28 = &DAT_0142f030;
  FUN_00809dc0(*(undefined8 *)(param_1 + 0x10),&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012e9ac;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012e9ac:
  local_28 = &DAT_0142f030;
  FUN_00809dc0(*(undefined8 *)(param_1 + 0x20),&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012e9f0;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012e9f0:
  local_28 = &DAT_0142f030;
  FUN_00809dc0(*(undefined8 *)(param_1 + 0x30),&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012ea34;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012ea34:
  uVar2 = *(undefined8 *)(param_1 + 0x40);
  FUN_003b8988(&local_28,"SplashScreen","Pro plug-in",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012ea98;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012ea98:
  uVar2 = *(undefined8 *)(param_1 + 0x50);
  FUN_003b8988(&local_28,"SplashScreen","Current version: 6.0.0",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012eafc;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012eafc:
  uVar2 = *(undefined8 *)(param_1 + 0x58);
  FUN_003b8988(&local_28,"SplashScreen","What\'s new in this version",0,0xffffffff);
  FUN_007d5e4c(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012eb60;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012eb60:
  uVar2 = *(undefined8 *)(param_1 + 0x78);
  FUN_003b8988(&local_28,"SplashScreen",
               "This free Demo edition of\nNeat Video may only be used for\ntesting and evaluation purposes"
               ,0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012ebc4;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012ebc4:
  uVar2 = *(undefined8 *)(param_1 + 0x80);
  FUN_003b8988(&local_28,"SplashScreen","Purchase full version",0,0xffffffff);
  FUN_007d5e4c(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012ec28;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012ec28:
  uVar2 = *(undefined8 *)(param_1 + 0x88);
  FUN_003b8988(&local_28,"SplashScreen","Download full version",0,0xffffffff);
  FUN_007d5e4c(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012ec8c;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012ec8c:
  uVar2 = *(undefined8 *)(param_1 + 0xa0);
  FUN_003b8988(&local_28,"SplashScreen","This copy is licensed to:",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012ecf0;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012ecf0:
  uVar2 = *(undefined8 *)(param_1 + 0xa8);
  FUN_003b8988(&local_28,"SplashScreen","User Name",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012ed54;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012ed54:
  uVar2 = *(undefined8 *)(param_1 + 0xb0);
  FUN_003b8988(&local_28,"SplashScreen","Company Name Pty Ltd",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012edb8;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012edb8:
  uVar2 = *(undefined8 *)(param_1 + 0xb8);
  FUN_003b8988(&local_28,"SplashScreen","for use on up to 5 computers",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012ee1c;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012ee1c:
  uVar2 = *(undefined8 *)(param_1 + 200);
  FUN_003b8988(&local_28,"SplashScreen","CONTINUE",0,0xffffffff);
  FUN_007d5e4c(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012ee80;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012ee80:
  uVar2 = *(undefined8 *)(param_1 + 0xe0);
  FUN_003b8988(&local_28,"SplashScreen","Quick Start Guide",0,0xffffffff);
  FUN_007d5e4c(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012eee4;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012eee4:
  uVar2 = *(undefined8 *)(param_1 + 0xf0);
  FUN_003b8988(&local_28,"SplashScreen","Video Tutorials",0,0xffffffff);
  FUN_007d5e4c(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0012ef48;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_0012ef48:
  uVar2 = *(undefined8 *)(param_1 + 0x100);
  FUN_003b8988(&local_28,"SplashScreen",s_1999_2026_NeatLab_Pty_Ltd__01310886,0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
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


