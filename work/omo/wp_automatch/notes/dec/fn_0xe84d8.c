// FUN_000e84d8 size=6868

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_000e84d8(undefined8 *param_1,undefined8 param_2)

{
  int iVar1;
  int iVar2;
  undefined8 uVar3;
  undefined8 *puVar4;
  ulong uVar5;
  undefined8 *puVar6;
  int *piStack_78;
  int *apiStack_70 [2];
  int *apiStack_60 [2];
  int *apiStack_50 [2];
  
  func_0x003cec40(apiStack_50,param_2);
  iVar2 = apiStack_50[0][1];
  if (*apiStack_50[0] != -1) {
    if (*apiStack_50[0] != 0) {
      LOAcquire();
      iVar1 = *apiStack_50[0];
      *apiStack_50[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000e853c;
    }
    func_0x00287df8(apiStack_50[0],2,8);
  }
LAB_000e853c:
  if (iVar2 == 0) {
    func_0x002efbd0(apiStack_50,"profileStatsPanelAdvanced",0x19);
    func_0x003cec84(param_2,apiStack_50);
    if (*apiStack_50[0] != -1) {
      if (*apiStack_50[0] != 0) {
        LOAcquire();
        iVar2 = *apiStack_50[0];
        *apiStack_50[0] = iVar2 + -1;
        LORelease();
        if (iVar2 != 1) goto LAB_000e8594;
      }
      func_0x00287df8(apiStack_50[0],2,8);
    }
  }
LAB_000e8594:
  apiStack_50[0] = _UNK_00c947e8;
  func_0x00751eb8(param_2,apiStack_50);
  uVar3 = __Znwm(0x20);
  func_0x00735284(uVar3,param_2);
  *param_1 = uVar3;
  func_0x00733ed8(uVar3,0);
  uVar3 = *param_1;
  func_0x002efbd0(apiStack_50,"verticalLayout",0xe);
  func_0x003cec84(uVar3,apiStack_50);
  if (*apiStack_50[0] != -1) {
    if (*apiStack_50[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_50[0];
      *apiStack_50[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e8628;
    }
    func_0x00287df8(apiStack_50[0],2,8);
  }
LAB_000e8628:
  func_0x0073abe0(*param_1,0,0,0,0);
  uVar3 = __Znwm(0x90);
  func_0x00024830(uVar3,param_2);
  puVar6 = param_1 + 1;
  *puVar6 = uVar3;
  func_0x002efbd0(apiStack_50,"statsGroupBox",0xd);
  func_0x003cec84(uVar3,apiStack_50);
  if (*apiStack_50[0] != -1) {
    if (*apiStack_50[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_50[0];
      *apiStack_50[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e86b0;
    }
    func_0x00287df8(apiStack_50[0],2,8);
  }
LAB_000e86b0:
  func_0x004d4bf0(apiStack_50);
  func_0x004d57ac(apiStack_50,0);
  func_0x0074caa8(*puVar6,apiStack_50);
  uVar3 = __Znwm(0x20);
  func_0x007350f0(uVar3,*puVar6);
  param_1[2] = uVar3;
  func_0x00733ed8(uVar3,5);
  uVar3 = param_1[2];
  func_0x002efbd0(apiStack_60,"horizontalLayout_9",0x12);
  func_0x003cec84(uVar3,apiStack_60);
  if (*apiStack_60[0] != -1) {
    if (*apiStack_60[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_60[0];
      *apiStack_60[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e874c;
    }
    func_0x00287df8(apiStack_60[0],2,8);
  }
LAB_000e874c:
  func_0x0073abe0(param_1[2],0,2,0,0);
  uVar3 = __Znwm(0x20);
  func_0x0073533c();
  param_1[3] = uVar3;
  func_0x00733ed8(uVar3,2);
  uVar3 = param_1[3];
  func_0x002efbd0(apiStack_60,"verticalLayout_8",0x10);
  func_0x003cec84(uVar3,apiStack_60);
  if (*apiStack_60[0] != -1) {
    if (*apiStack_60[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_60[0];
      *apiStack_60[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e87dc;
    }
    func_0x00287df8(apiStack_60[0],2,8);
  }
LAB_000e87dc:
  uVar3 = __Znwm(0x20);
  func_0x007387f4();
  param_1[4] = uVar3;
  func_0x002efbd0(apiStack_60,"gridLayout_5",0xc);
  func_0x003cec84(uVar3,apiStack_60);
  if (*apiStack_60[0] != -1) {
    if (*apiStack_60[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_60[0];
      *apiStack_60[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e8844;
    }
    func_0x00287df8(apiStack_60[0],2,8);
  }
LAB_000e8844:
  func_0x007389bc(param_1[4],5);
  func_0x007389d0(param_1[4],4);
  func_0x0073abe0(param_1[4],0xffffffff,0,0xffffffff,0xffffffff);
  uVar3 = __Znwm(0x58);
  func_0x0001699c(uVar3,*puVar6);
  param_1[5] = uVar3;
  func_0x002efbd0(apiStack_60,"advancedQualityLabel",0x14);
  func_0x003cec84(uVar3,apiStack_60);
  if (*apiStack_60[0] != -1) {
    if (*apiStack_60[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_60[0];
      *apiStack_60[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e88e0;
    }
    func_0x00287df8(apiStack_60[0],2,8);
  }
LAB_000e88e0:
  func_0x0074b9bc(param_1[5],0x3c,0x11);
  func_0x0074bd40(param_1[5],0x3c,0x11);
  func_0x004d4bf0(apiStack_60);
  func_0x004d5674(apiStack_60,0x32);
  func_0x004d57ac(apiStack_60,0);
  func_0x004d5674(apiStack_60,0x32);
  func_0x0074caa8(param_1[5],apiStack_60);
  func_0x00742624(param_1[5],1);
  func_0x007d007c(param_1[5],2);
  func_0x007d01a0(param_1[5],0x30);
  func_0x0080a6d8(param_1[5],0x84);
  func_0x00739b6c(param_1[4],param_1[5],0,1,1,1,0);
  uVar3 = __Znwm(0x30);
  func_0x00809ca8(uVar3,*puVar6,0);
  param_1[6] = uVar3;
  func_0x002efbd0(apiStack_70,"advancedQualityCaptionLabel",0x1b);
  func_0x003cec84(uVar3,apiStack_70);
  if (*apiStack_70[0] != -1) {
    if (*apiStack_70[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_70[0];
      *apiStack_70[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e89f4;
    }
    func_0x00287df8(apiStack_70[0],2,8);
  }
LAB_000e89f4:
  func_0x0074b9bc(param_1[6],0,0x11);
  func_0x0074bd40(param_1[6],0xffffff,0x11);
  func_0x0074caa8(param_1[6],apiStack_60);
  func_0x0080a73c(param_1[6],1);
  func_0x00739b6c(param_1[4],param_1[6],0,0,1,1,0);
  uVar3 = __Znwm(0x30);
  func_0x00809ca8(uVar3,*puVar6,0);
  param_1[7] = uVar3;
  func_0x002efbd0(apiStack_70,"advancedMatchCaptionLabel",0x19);
  func_0x003cec84(uVar3,apiStack_70);
  if (*apiStack_70[0] != -1) {
    if (*apiStack_70[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_70[0];
      *apiStack_70[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e8abc;
    }
    func_0x00287df8(apiStack_70[0],2,8);
  }
LAB_000e8abc:
  func_0x0074b9bc(param_1[7],0,0x11);
  func_0x0074bd40(param_1[7],0xffffff,0x11);
  func_0x0074caa8(param_1[7],apiStack_60);
  func_0x0080a73c(param_1[7],1);
  func_0x00739b6c(param_1[4],param_1[7],1,0,1,1,0);
  uVar3 = __Znwm(0x58);
  func_0x0001699c(uVar3,*puVar6);
  param_1[8] = uVar3;
  func_0x002efbd0(apiStack_70,"advancedMatchLabel",0x12);
  func_0x003cec84(uVar3,apiStack_70);
  if (*apiStack_70[0] != -1) {
    if (*apiStack_70[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_70[0];
      *apiStack_70[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e8b80;
    }
    func_0x00287df8(apiStack_70[0],2,8);
  }
LAB_000e8b80:
  func_0x0074b9bc(param_1[8],0x3c,0x11);
  func_0x0074bd40(param_1[8],0x3c,0x11);
  func_0x0074caa8(param_1[8],apiStack_60);
  func_0x00742624(param_1[8],1);
  func_0x007d007c(param_1[8],2);
  func_0x007d01a0(param_1[8],0x30);
  func_0x0080a6d8(param_1[8],0x84);
  func_0x00739b6c(param_1[4],param_1[8],1,1,1,1,0);
  func_0x00734fe0(param_1[3],param_1[4],0);
  uVar3 = __Znwm(0x20);
  func_0x007387f4();
  param_1[9] = uVar3;
  func_0x002efbd0(apiStack_70,"gridLayout_9",0xc);
  func_0x003cec84(uVar3,apiStack_70);
  if (*apiStack_70[0] != -1) {
    if (*apiStack_70[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_70[0];
      *apiStack_70[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e8c70;
    }
    func_0x00287df8(apiStack_70[0],2,8);
  }
LAB_000e8c70:
  func_0x007389bc(param_1[9],5);
  func_0x007389d0(param_1[9],4);
  func_0x0073abe0(param_1[9],0xffffffff,0xffffffff,0xffffffff,0);
  uVar3 = __Znwm(0x30);
  func_0x00809ca8(uVar3,*puVar6,0);
  param_1[10] = uVar3;
  func_0x002efbd0(apiStack_70,"noiseLevelCaptionLabel",0x16);
  func_0x003cec84(uVar3,apiStack_70);
  if (*apiStack_70[0] != -1) {
    if (*apiStack_70[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_70[0];
      *apiStack_70[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e8d10;
    }
    func_0x00287df8(apiStack_70[0],2,8);
  }
LAB_000e8d10:
  func_0x0074b9bc(param_1[10],0,0x11);
  func_0x0074bd40(param_1[10],0xffffff,0x11);
  func_0x0074caa8(param_1[10],apiStack_60);
  func_0x0080a73c(param_1[10],1);
  func_0x00739b6c(param_1[9],param_1[10],1,0,1,1,0);
  uVar3 = __Znwm(0x30);
  func_0x00809ca8(uVar3,*puVar6,0);
  param_1[0xb] = uVar3;
  func_0x002efbd0(apiStack_70,"noiseLevelLabel",0xf);
  func_0x003cec84(uVar3,apiStack_70);
  if (*apiStack_70[0] != -1) {
    if (*apiStack_70[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_70[0];
      *apiStack_70[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e8dd4;
    }
    func_0x00287df8(apiStack_70[0],2,8);
  }
LAB_000e8dd4:
  func_0x0074b9bc(param_1[0xb],0,0x11);
  func_0x0074bd40(param_1[0xb],0xffffff,0x11);
  func_0x0074caa8(param_1[0xb],apiStack_60);
  func_0x0080a6d8(param_1[0xb],0x82);
  func_0x00739b6c(param_1[9],param_1[0xb],1,1,1,1,0);
  uVar3 = __Znwm(0x30);
  func_0x00809ca8(uVar3,*puVar6,0);
  param_1[0xc] = uVar3;
  func_0x002efbd0(apiStack_70,"noiseLevelSelectionCaptionLabel",0x1f);
  func_0x003cec84(uVar3,apiStack_70);
  if (*apiStack_70[0] != -1) {
    if (*apiStack_70[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_70[0];
      *apiStack_70[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e8e9c;
    }
    func_0x00287df8(apiStack_70[0],2,8);
  }
LAB_000e8e9c:
  func_0x0074b9bc(param_1[0xc],0,0x11);
  func_0x0074bd40(param_1[0xc],0xffffff,0x11);
  func_0x0074caa8(param_1[0xc],apiStack_60);
  func_0x0080a73c(param_1[0xc],1);
  func_0x00739b6c(param_1[9],param_1[0xc],2,0,1,1,0);
  uVar3 = __Znwm(0x30);
  func_0x00809ca8(uVar3,*puVar6,0);
  param_1[0xd] = uVar3;
  func_0x002efbd0(apiStack_70,"noiseLevelSelectionLabel",0x18);
  func_0x003cec84(uVar3,apiStack_70);
  if (*apiStack_70[0] != -1) {
    if (*apiStack_70[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_70[0];
      *apiStack_70[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e8f64;
    }
    func_0x00287df8(apiStack_70[0],2,8);
  }
LAB_000e8f64:
  func_0x0074b9bc(param_1[0xd],0,0x11);
  func_0x0074bd40(param_1[0xd],0xffffff,0x11);
  func_0x0074caa8(param_1[0xd],apiStack_60);
  func_0x0080a6d8(param_1[0xd],0x82);
  func_0x00739b6c(param_1[9],param_1[0xd],2,1,1,1,0);
  uVar3 = __Znwm(0x30);
  func_0x00809ca8(uVar3,*puVar6,0);
  param_1[0xe] = uVar3;
  func_0x002efbd0(apiStack_70,"noiseLevelsCaptionLabel",0x17);
  func_0x003cec84(uVar3,apiStack_70);
  if (*apiStack_70[0] != -1) {
    if (*apiStack_70[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_70[0];
      *apiStack_70[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e902c;
    }
    func_0x00287df8(apiStack_70[0],2,8);
  }
LAB_000e902c:
  func_0x0074b9bc(param_1[0xe],0,0x11);
  func_0x0074bd40(param_1[0xe],0xffffff,0x11);
  func_0x004d4bf0(apiStack_70);
  func_0x004d5674(apiStack_70,0x4b);
  func_0x004d57ac(apiStack_70,0);
  func_0x004d5674(apiStack_70,0x4b);
  func_0x0074caa8(param_1[0xe],apiStack_70);
  func_0x0080a73c(param_1[0xe],1);
  func_0x00739b6c(param_1[9],param_1[0xe],0,0,1,1,0);
  puVar4 = (undefined8 *)__Znwm(0x90);
  func_0x0001ae50(puVar4,*puVar6,0);
  *puVar4 = &UNK_013bc550;
  puVar4[2] = &UNK_013bc700;
  puVar4[6] = &UNK_013bc750;
  param_1[0xf] = puVar4;
  func_0x002efbd0(&piStack_78,"hLine",5);
  func_0x003cec84(puVar4,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9140;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9140:
  func_0x0074b9bc(param_1[0xf],0,2);
  func_0x0074bd40(param_1[0xf],0xffffff,2);
  func_0x007d007c(param_1[0xf],4);
  func_0x007d01a0(param_1[0xf],0x30);
  func_0x00739b6c(param_1[9],param_1[0xf],0,1,1,1,0);
  func_0x00734fe0(param_1[3],param_1[9],0);
  func_0x00734fe0(param_1[2],param_1[3],0);
  puVar4 = (undefined8 *)__Znwm(0x90);
  func_0x0001ae50(puVar4,*puVar6,1);
  *puVar4 = &UNK_013bc310;
  puVar4[2] = &UNK_013bc4c0;
  puVar4[6] = &UNK_013bc510;
  param_1[0x10] = puVar4;
  func_0x002efbd0(&piStack_78,"vLine",5);
  func_0x003cec84(puVar4,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9244;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9244:
  uVar5 = func_0x00753f0c(param_1[0x10]);
  func_0x0075597c(param_1[0x10],uVar5 & 0x20000000 | 0x100000);
  func_0x0074b9bc(param_1[0x10],2,0);
  func_0x0074bd40(param_1[0x10],2,0xffffff);
  func_0x007d007c(param_1[0x10],5);
  func_0x007d01a0(param_1[0x10],0x30);
  func_0x00734fc8(param_1[2],param_1[0x10],0,0);
  uVar3 = __Znwm(0x20);
  func_0x007387f4();
  param_1[0x11] = uVar3;
  func_0x002efbd0(&piStack_78,"gridLayout_10",0xd);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9310;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9310:
  func_0x007389bc(param_1[0x11],4);
  func_0x007389d0(param_1[0x11],2);
  uVar3 = __Znwm(0x100);
  piStack_78 = (int *)0x142f030;
  func_0x0002bf20(uVar3,*puVar6,&piStack_78);
  param_1[0x12] = uVar3;
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9384;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9384:
  uVar3 = param_1[0x12];
  func_0x002efbd0(&piStack_78,"hfImageLabel",0xc);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e93dc;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e93dc:
  func_0x0074b9bc(param_1[0x12],0x17,0x11);
  func_0x0074bd40(param_1[0x12],0x17,0x11);
  func_0x00739b6c(param_1[0x11],param_1[0x12],0,0,1,1,0);
  uVar3 = __Znwm(0x100);
  piStack_78 = (int *)0x142f030;
  func_0x0002bf20(uVar3,*puVar6,&piStack_78);
  param_1[0x13] = uVar3;
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e946c;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e946c:
  uVar3 = param_1[0x13];
  func_0x002efbd0(&piStack_78,"ulfImageLabel",0xd);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e94c4;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e94c4:
  func_0x0074b9bc(param_1[0x13],0x17,0x11);
  func_0x0074bd40(param_1[0x13],0x17,0x11);
  func_0x00739b6c(param_1[0x11],param_1[0x13],4,0,1,1,0);
  uVar3 = __Znwm(0x100);
  piStack_78 = (int *)0x142f030;
  func_0x0002bf20(uVar3,*puVar6,&piStack_78);
  param_1[0x14] = uVar3;
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9558;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9558:
  uVar3 = param_1[0x14];
  func_0x002efbd0(&piStack_78,"mfImageLabel",0xc);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e95b0;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e95b0:
  func_0x0074b9bc(param_1[0x14],0x17,0x11);
  func_0x0074bd40(param_1[0x14],0x17,0x11);
  func_0x00739b6c(param_1[0x11],param_1[0x14],1,0,1,1,0);
  uVar3 = __Znwm(0x100);
  piStack_78 = (int *)0x142f030;
  func_0x0002bf20(uVar3,*puVar6,&piStack_78);
  param_1[0x15] = uVar3;
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9644;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9644:
  uVar3 = param_1[0x15];
  func_0x002efbd0(&piStack_78,"vlfImageLabel",0xd);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e969c;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e969c:
  func_0x0074b9bc(param_1[0x15],0x17,0x11);
  func_0x0074bd40(param_1[0x15],0x17,0x11);
  func_0x00739b6c(param_1[0x11],param_1[0x15],3,0,1,1,0);
  uVar3 = __Znwm(0x100);
  piStack_78 = (int *)0x142f030;
  func_0x0002bf20(uVar3,*puVar6,&piStack_78);
  param_1[0x16] = uVar3;
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9730;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9730:
  uVar3 = param_1[0x16];
  func_0x002efbd0(&piStack_78,"lfImageLabel",0xc);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9788;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9788:
  func_0x0074b9bc(param_1[0x16],0x17,0x11);
  func_0x0074bd40(param_1[0x16],0x17,0x11);
  func_0x00739b6c(param_1[0x11],param_1[0x16],2,0,1,1,0);
  uVar3 = __Znwm(0xa0);
  func_0x00028abc(uVar3,*puVar6);
  param_1[0x17] = uVar3;
  func_0x002efbd0(&piStack_78,"mfHist",6);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9834;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9834:
  func_0x0074b9bc(param_1[0x17],0x2e,0xe);
  func_0x0074bd40(param_1[0x17],0xffffff,0xe);
  func_0x00739b6c(param_1[0x11],param_1[0x17],1,2,1,1,0);
  uVar3 = __Znwm(0xa0);
  func_0x00028abc(uVar3,*puVar6);
  param_1[0x18] = uVar3;
  func_0x002efbd0(&piStack_78,"lfHist",6);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e98e0;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e98e0:
  func_0x0074b9bc(param_1[0x18],0x2e,0xe);
  func_0x0074bd40(param_1[0x18],0xffffff,0xe);
  func_0x00739b6c(param_1[0x11],param_1[0x18],2,2,1,1,0);
  uVar3 = __Znwm(0xa0);
  func_0x00028abc(uVar3,*puVar6);
  param_1[0x19] = uVar3;
  func_0x002efbd0(&piStack_78,"vlfHist",7);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e998c;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e998c:
  func_0x0074b9bc(param_1[0x19],0x2e,0xe);
  func_0x0074bd40(param_1[0x19],0xffffff,0xe);
  func_0x00739b6c(param_1[0x11],param_1[0x19],3,2,1,1,0);
  uVar3 = __Znwm(0xa0);
  func_0x00028abc(uVar3,*puVar6);
  param_1[0x1a] = uVar3;
  func_0x002efbd0(&piStack_78,"ulfHist",7);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9a38;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9a38:
  func_0x0074b9bc(param_1[0x1a],0x2e,0xe);
  func_0x0074bd40(param_1[0x1a],0xffffff,0xe);
  func_0x00739b6c(param_1[0x11],param_1[0x1a],4,2,1,1,0);
  uVar3 = __Znwm(0xa0);
  func_0x00028abc(uVar3,*puVar6);
  param_1[0x1b] = uVar3;
  func_0x002efbd0(&piStack_78,"hfHist",6);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9ae4;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9ae4:
  func_0x0074b9bc(param_1[0x1b],0x2e,0xe);
  func_0x0074bd40(param_1[0x1b],0xffffff,0xe);
  func_0x00739b6c(param_1[0x11],param_1[0x1b],0,2,1,1,0);
  uVar3 = __Znwm(0x30);
  func_0x00809ca8(uVar3,*puVar6,0);
  param_1[0x1c] = uVar3;
  func_0x002efbd0(&piStack_78,"vlfHistLabel",0xc);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9b94;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9b94:
  func_0x0074b9bc(param_1[0x1c],0x3c,0x11);
  func_0x0074bd40(param_1[0x1c],0x3c,0x11);
  func_0x0074caa8(param_1[0x1c],apiStack_60);
  func_0x0080a6d8(param_1[0x1c],0x81);
  func_0x0080a73c(param_1[0x1c],1);
  func_0x00739b6c(param_1[0x11],param_1[0x1c],3,1,1,1,0);
  uVar3 = __Znwm(0x30);
  func_0x00809ca8(uVar3,*puVar6,0);
  param_1[0x1d] = uVar3;
  func_0x002efbd0(&piStack_78,"lfHistLabel",0xb);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9c68;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9c68:
  func_0x0074b9bc(param_1[0x1d],0x3c,0x11);
  func_0x0074bd40(param_1[0x1d],0x3c,0x11);
  func_0x0074caa8(param_1[0x1d],apiStack_60);
  func_0x0080a6d8(param_1[0x1d],0x81);
  func_0x0080a73c(param_1[0x1d],1);
  func_0x00739b6c(param_1[0x11],param_1[0x1d],2,1,1,1,0);
  uVar3 = __Znwm(0x30);
  func_0x00809ca8(uVar3,*puVar6,0);
  param_1[0x1e] = uVar3;
  func_0x002efbd0(&piStack_78,"mfHistLabel",0xb);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9d3c;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9d3c:
  func_0x0074b9bc(param_1[0x1e],0x3c,0x11);
  func_0x0074bd40(param_1[0x1e],0x3c,0x11);
  func_0x0074caa8(param_1[0x1e],apiStack_60);
  func_0x0080a6d8(param_1[0x1e],0x81);
  func_0x0080a73c(param_1[0x1e],1);
  func_0x00739b6c(param_1[0x11],param_1[0x1e],1,1,1,1,0);
  uVar3 = __Znwm(0x30);
  func_0x00809ca8(uVar3,*puVar6,0);
  param_1[0x1f] = uVar3;
  func_0x002efbd0(&piStack_78,"ulfHistLabel",0xc);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9e10;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9e10:
  func_0x0074b9bc(param_1[0x1f],0x3c,0x11);
  func_0x0074bd40(param_1[0x1f],0x3c,0x11);
  func_0x0074caa8(param_1[0x1f],apiStack_60);
  func_0x0080a6d8(param_1[0x1f],0x81);
  func_0x0080a73c(param_1[0x1f],1);
  func_0x00739b6c(param_1[0x11],param_1[0x1f],4,1,1,1,0);
  uVar3 = __Znwm(0x30);
  func_0x00809ca8(uVar3,*puVar6,0);
  param_1[0x20] = uVar3;
  func_0x002efbd0(&piStack_78,"hfHistLabel",0xb);
  func_0x003cec84(uVar3,&piStack_78);
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar2 = *piStack_78;
      *piStack_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e9ee4;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_000e9ee4:
  func_0x0074b9bc(param_1[0x20],0x3c,0x11);
  func_0x0074bd40(param_1[0x20],0x3c,0x11);
  func_0x0074caa8(param_1[0x20],apiStack_60);
  func_0x0080a6d8(param_1[0x20],0x81);
  func_0x0080a73c(param_1[0x20],1);
  func_0x00739b6c(param_1[0x11],param_1[0x20],0,1,1,1,0);
  func_0x00734fe0(param_1[2],param_1[0x11],0);
  func_0x00734fc8(*param_1,param_1[1],0,0);
  func_0x000ea530(param_1,param_2);
  func_0x003d39dc(param_2);
  func_0x004d42c0(apiStack_70);
  func_0x004d42c0(apiStack_60);
  func_0x004d42c0(apiStack_50);
  return;
}


