// FUN_000ea530 size=3012

void FUN_000ea530(long param_1,undefined8 param_2)

{
  int iVar1;
  undefined8 uVar2;
  int *piStack_28;
  
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced","Form",0,0xffffffff);
  func_0x00750130(param_2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea5a8;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000ea5a8:
  piStack_28 = (int *)0x142f030;
  func_0x00024d78(*(undefined8 *)(param_1 + 8),&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea5f4;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000ea5f4:
  uVar2 = *(undefined8 *)(param_1 + 0x28);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced",
                  "Profile quality estimates accuracy and completeness of noise analysis",0,
                  0xffffffff);
  func_0x00756efc(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea658;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000ea658:
  uVar2 = *(undefined8 *)(param_1 + 0x28);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced","100%",0,0xffffffff);
  func_0x00809dc0(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea6bc;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000ea6bc:
  uVar2 = *(undefined8 *)(param_1 + 0x30);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced","Quality",0,0xffffffff);
  func_0x00809dc0(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea720;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000ea720:
  uVar2 = *(undefined8 *)(param_1 + 0x38);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced","Match",0,0xffffffff);
  func_0x00809dc0(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea784;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000ea784:
  uVar2 = *(undefined8 *)(param_1 + 0x40);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced",
                  "Profile-image match shows how close are device modes of current noise profile and input image"
                  ,0,0xffffffff);
  func_0x00756efc(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea7e8;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000ea7e8:
  uVar2 = *(undefined8 *)(param_1 + 0x40);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced","100%",0,0xffffffff);
  func_0x00809dc0(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea84c;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000ea84c:
  uVar2 = *(undefined8 *)(param_1 + 0x50);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced","Main Sample",0,0xffffffff);
  func_0x00809dc0(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea8b0;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000ea8b0:
  uVar2 = *(undefined8 *)(param_1 + 0x58);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced",
                  "Estimation of the noise level in the main sample\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
                  ,0,0xffffffff);
  func_0x00756efc(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea914;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000ea914:
  uVar2 = *(undefined8 *)(param_1 + 0x58);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced","55.5",0,0xffffffff);
  func_0x00809dc0(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea978;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000ea978:
  uVar2 = *(undefined8 *)(param_1 + 0x60);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced","Selection",0,0xffffffff);
  func_0x00809dc0(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ea9dc;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000ea9dc:
  uVar2 = *(undefined8 *)(param_1 + 0x68);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced",
                  "Estimation of the noise level in current selection\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
                  ,0,0xffffffff);
  func_0x00756efc(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eaa40;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eaa40:
  uVar2 = *(undefined8 *)(param_1 + 0x68);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced","55.5",0,0xffffffff);
  func_0x00809dc0(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eaaa4;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eaaa4:
  uVar2 = *(undefined8 *)(param_1 + 0x70);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced","Noise Levels",0,0xffffffff);
  func_0x00809dc0(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eab08;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eab08:
  uVar2 = *(undefined8 *)(param_1 + 0xb8);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced",
                  "Estimation of noise level in medium frequency component",0,0xffffffff);
  func_0x00756efc(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eab6c;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eab6c:
  uVar2 = *(undefined8 *)(param_1 + 0xc0);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced",
                  "Estimation of noise level in low frequency component",0,0xffffffff);
  func_0x00756efc(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eabd0;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eabd0:
  uVar2 = *(undefined8 *)(param_1 + 200);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced",
                  "Estimation of noise level in very low frequency component",0,0xffffffff);
  func_0x00756efc(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eac34;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eac34:
  uVar2 = *(undefined8 *)(param_1 + 0xd0);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced",
                  "Estimation of noise level in ultra low frequency component",0,0xffffffff);
  func_0x00756efc(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eac98;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eac98:
  uVar2 = *(undefined8 *)(param_1 + 0xd8);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced",
                  "Estimation of noise level in high frequency component",0,0xffffffff);
  func_0x00756efc(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eacfc;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eacfc:
  uVar2 = *(undefined8 *)(param_1 + 0xe0);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced",
                  "Estimation of noise level in very low frequency component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
                  ,0,0xffffffff);
  func_0x00756efc(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000ead60;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000ead60:
  uVar2 = *(undefined8 *)(param_1 + 0xe0);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced","Very Low",0,0xffffffff);
  func_0x00809dc0(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eadc4;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eadc4:
  uVar2 = *(undefined8 *)(param_1 + 0xe8);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced",
                  "Estimation of noise level in low frequency component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
                  ,0,0xffffffff);
  func_0x00756efc(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eae28;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eae28:
  uVar2 = *(undefined8 *)(param_1 + 0xe8);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced","Low",0,0xffffffff);
  func_0x00809dc0(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eae8c;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eae8c:
  uVar2 = *(undefined8 *)(param_1 + 0xf0);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced",
                  "Estimation of noise level in medium frequency component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
                  ,0,0xffffffff);
  func_0x00756efc(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eaef0;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eaef0:
  uVar2 = *(undefined8 *)(param_1 + 0xf0);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced","Medium",0,0xffffffff);
  func_0x00809dc0(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eaf54;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eaf54:
  uVar2 = *(undefined8 *)(param_1 + 0xf8);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced",
                  "Estimation of noise level in ultra low frequency component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
                  ,0,0xffffffff);
  func_0x00756efc(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eafb8;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eafb8:
  uVar2 = *(undefined8 *)(param_1 + 0xf8);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced","Ultra Low",0,0xffffffff);
  func_0x00809dc0(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eb01c;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eb01c:
  uVar2 = *(undefined8 *)(param_1 + 0x100);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced",
                  "Estimation of noise level in high frequency component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
                  ,0,0xffffffff);
  func_0x00756efc(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eb080;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000eb080:
  uVar2 = *(undefined8 *)(param_1 + 0x100);
  func_0x003b8988(&piStack_28,"profileStatsPanelAdvanced","High",0,0xffffffff);
  func_0x00809dc0(uVar2,&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar1 = *piStack_28;
      *piStack_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) {
        return;
      }
    }
    func_0x00287df8(piStack_28,2,8);
  }
  return;
}


