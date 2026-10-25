// func entry: 000ea530 name: FUN_000ea530 size: 3012

void FUN_000ea530(long param_1,undefined8 param_2)

{
  int iVar1;
  undefined8 uVar2;
  int *local_28;
  
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced","Form",0,0xffffffff);
  FUN_00750130(param_2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea5a8;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000ea5a8:
  local_28 = &DAT_0142f030;
  FUN_00024d78(*(undefined8 *)(param_1 + 8),&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea5f4;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000ea5f4:
  uVar2 = *(undefined8 *)(param_1 + 0x28);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced",
               "Profile quality estimates accuracy and completeness of noise analysis",0,0xffffffff)
  ;
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea658;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000ea658:
  uVar2 = *(undefined8 *)(param_1 + 0x28);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced","100%",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea6bc;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000ea6bc:
  uVar2 = *(undefined8 *)(param_1 + 0x30);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced","Quality",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea720;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000ea720:
  uVar2 = *(undefined8 *)(param_1 + 0x38);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced","Match",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea784;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000ea784:
  uVar2 = *(undefined8 *)(param_1 + 0x40);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced",
               "Profile-image match shows how close are device modes of current noise profile and input image"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea7e8;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000ea7e8:
  uVar2 = *(undefined8 *)(param_1 + 0x40);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced","100%",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea84c;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000ea84c:
  uVar2 = *(undefined8 *)(param_1 + 0x50);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced","Main Sample",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea8b0;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000ea8b0:
  uVar2 = *(undefined8 *)(param_1 + 0x58);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced",
               "Estimation of the noise level in the main sample\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea914;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000ea914:
  uVar2 = *(undefined8 *)(param_1 + 0x58);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced","55.5",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea978;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000ea978:
  uVar2 = *(undefined8 *)(param_1 + 0x60);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced","Selection",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea9dc;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000ea9dc:
  uVar2 = *(undefined8 *)(param_1 + 0x68);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced",
               "Estimation of the noise level in current selection\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eaa40;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eaa40:
  uVar2 = *(undefined8 *)(param_1 + 0x68);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced","55.5",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eaaa4;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eaaa4:
  uVar2 = *(undefined8 *)(param_1 + 0x70);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced","Noise Levels",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eab08;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eab08:
  uVar2 = *(undefined8 *)(param_1 + 0xb8);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced",
               "Estimation of noise level in medium frequency component",0,0xffffffff);
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eab6c;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eab6c:
  uVar2 = *(undefined8 *)(param_1 + 0xc0);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced",
               "Estimation of noise level in low frequency component",0,0xffffffff);
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eabd0;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eabd0:
  uVar2 = *(undefined8 *)(param_1 + 200);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced",
               "Estimation of noise level in very low frequency component",0,0xffffffff);
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eac34;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eac34:
  uVar2 = *(undefined8 *)(param_1 + 0xd0);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced",
               "Estimation of noise level in ultra low frequency component",0,0xffffffff);
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eac98;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eac98:
  uVar2 = *(undefined8 *)(param_1 + 0xd8);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced",
               "Estimation of noise level in high frequency component",0,0xffffffff);
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eacfc;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eacfc:
  uVar2 = *(undefined8 *)(param_1 + 0xe0);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced",
               "Estimation of noise level in very low frequency component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ead60;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000ead60:
  uVar2 = *(undefined8 *)(param_1 + 0xe0);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced","Very Low",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eadc4;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eadc4:
  uVar2 = *(undefined8 *)(param_1 + 0xe8);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced",
               "Estimation of noise level in low frequency component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eae28;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eae28:
  uVar2 = *(undefined8 *)(param_1 + 0xe8);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced","Low",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eae8c;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eae8c:
  uVar2 = *(undefined8 *)(param_1 + 0xf0);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced",
               "Estimation of noise level in medium frequency component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eaef0;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eaef0:
  uVar2 = *(undefined8 *)(param_1 + 0xf0);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced","Medium",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eaf54;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eaf54:
  uVar2 = *(undefined8 *)(param_1 + 0xf8);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced",
               "Estimation of noise level in ultra low frequency component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eafb8;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eafb8:
  uVar2 = *(undefined8 *)(param_1 + 0xf8);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced","Ultra Low",0,0xffffffff);
  FUN_00809dc0(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eb01c;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eb01c:
  uVar2 = *(undefined8 *)(param_1 + 0x100);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced",
               "Estimation of noise level in high frequency component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,&local_28);
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eb080;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000eb080:
  uVar2 = *(undefined8 *)(param_1 + 0x100);
  FUN_003b8988(&local_28,"profileStatsPanelAdvanced","High",0,0xffffffff);
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


