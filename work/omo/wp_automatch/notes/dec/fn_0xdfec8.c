// FUN_000dfec8 size=1580

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_000dfec8(undefined8 *param_1,undefined8 param_2)

{
  int iVar1;
  int iVar2;
  undefined8 uVar3;
  undefined8 *puVar4;
  int *piStack_48;
  int *apiStack_40 [2];
  
  func_0x003cec40(apiStack_40,param_2);
  iVar2 = apiStack_40[0][1];
  if (*apiStack_40[0] != -1) {
    if (*apiStack_40[0] != 0) {
      LOAcquire();
      iVar1 = *apiStack_40[0];
      *apiStack_40[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000dff28;
    }
    func_0x00287df8(apiStack_40[0],2,8);
  }
LAB_000dff28:
  if (iVar2 == 0) {
    func_0x002efbd0(apiStack_40,"profileStatsPanelSimple",0x17);
    func_0x003cec84(param_2,apiStack_40);
    if (*apiStack_40[0] != -1) {
      if (*apiStack_40[0] != 0) {
        LOAcquire();
        iVar2 = *apiStack_40[0];
        *apiStack_40[0] = iVar2 + -1;
        LORelease();
        if (iVar2 != 1) goto LAB_000dff80;
      }
      func_0x00287df8(apiStack_40[0],2,8);
    }
  }
LAB_000dff80:
  apiStack_40[0] = _UNK_00c94720;
  func_0x00751eb8(param_2,apiStack_40);
  uVar3 = __Znwm(0x20);
  func_0x00735284(uVar3,param_2);
  *param_1 = uVar3;
  func_0x00733ed8(uVar3,0);
  uVar3 = *param_1;
  func_0x002efbd0(apiStack_40,"verticalLayout",0xe);
  func_0x003cec84(uVar3,apiStack_40);
  if (*apiStack_40[0] != -1) {
    if (*apiStack_40[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_40[0];
      *apiStack_40[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e0014;
    }
    func_0x00287df8(apiStack_40[0],2,8);
  }
LAB_000e0014:
  func_0x0073abe0(*param_1,0,0,0,0);
  uVar3 = __Znwm(0x90);
  func_0x00024830(uVar3,param_2);
  param_1[1] = uVar3;
  func_0x002efbd0(apiStack_40,"simpleQualityMatchGroupBox",0x1a);
  func_0x003cec84(uVar3,apiStack_40);
  if (*apiStack_40[0] != -1) {
    if (*apiStack_40[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_40[0];
      *apiStack_40[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e0098;
    }
    func_0x00287df8(apiStack_40[0],2,8);
  }
LAB_000e0098:
  uVar3 = __Znwm(0x20);
  func_0x007350f0(uVar3,param_1[1]);
  param_1[2] = uVar3;
  func_0x00733ed8(uVar3,10);
  uVar3 = param_1[2];
  func_0x002efbd0(apiStack_40,"horizontalLayout",0x10);
  func_0x003cec84(uVar3,apiStack_40);
  if (*apiStack_40[0] != -1) {
    if (*apiStack_40[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_40[0];
      *apiStack_40[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e0114;
    }
    func_0x00287df8(apiStack_40[0],2,8);
  }
LAB_000e0114:
  func_0x0073abe0(param_1[2],0,2,0,0);
  uVar3 = __Znwm(0x30);
  func_0x00809ca8(uVar3,param_1[1],0);
  param_1[3] = uVar3;
  func_0x002efbd0(apiStack_40,"simpleQualityCaptionLabel",0x19);
  func_0x003cec84(uVar3,apiStack_40);
  if (*apiStack_40[0] != -1) {
    if (*apiStack_40[0] != 0) {
      LOAcquire();
      iVar2 = *apiStack_40[0];
      *apiStack_40[0] = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e019c;
    }
    func_0x00287df8(apiStack_40[0],2,8);
  }
LAB_000e019c:
  func_0x0074b9bc(param_1[3],0,0x11);
  func_0x0074bd40(param_1[3],0xffffff,0x11);
  func_0x004d4bf0(apiStack_40);
  func_0x004d5674(apiStack_40,0x32);
  func_0x004d5674(apiStack_40,0x32);
  func_0x0074caa8(param_1[3],apiStack_40);
  func_0x0080a73c(param_1[3],1);
  func_0x00734fc8(param_1[2],param_1[3],0,0);
  uVar3 = __Znwm(0x58);
  func_0x0001699c(uVar3,param_1[1]);
  param_1[4] = uVar3;
  func_0x002efbd0(&piStack_48,"simpleQualityLabel",0x12);
  func_0x003cec84(uVar3,&piStack_48);
  if (*piStack_48 != -1) {
    if (*piStack_48 != 0) {
      LOAcquire();
      iVar2 = *piStack_48;
      *piStack_48 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e0270;
    }
    func_0x00287df8(piStack_48,2,8);
  }
LAB_000e0270:
  func_0x0074b9bc(param_1[4],0x3c,0x11);
  func_0x0074bd40(param_1[4],0x3c,0x11);
  func_0x0074caa8(param_1[4],apiStack_40);
  func_0x007d007c(param_1[4],2);
  func_0x007d01a0(param_1[4],0x30);
  func_0x0080a6d8(param_1[4],0x84);
  func_0x00734fc8(param_1[2],param_1[4],0,0);
  puVar4 = (undefined8 *)__Znwm(0x28);
  *puVar4 = &UNK_013e41b0;
  uVar3 = _UNK_00c94730;
  puVar4[2] = _UNK_00c94738;
  puVar4[1] = uVar3;
  uVar3 = _UNK_010b4460;
  puVar4[4] = _UNK_010b4468;
  puVar4[3] = uVar3;
  param_1[5] = puVar4;
  (**(code **)(*(long *)param_1[2] + 0x70))();
  uVar3 = __Znwm(0x30);
  func_0x00809ca8(uVar3,param_1[1],0);
  param_1[6] = uVar3;
  func_0x002efbd0(&piStack_48,"simpleMatchCaptionLabel",0x17);
  func_0x003cec84(uVar3,&piStack_48);
  if (*piStack_48 != -1) {
    if (*piStack_48 != 0) {
      LOAcquire();
      iVar2 = *piStack_48;
      *piStack_48 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e038c;
    }
    func_0x00287df8(piStack_48,2,8);
  }
LAB_000e038c:
  func_0x0074b9bc(param_1[6],0,0x10);
  func_0x0074bd40(param_1[6],0xffffff,0x10);
  func_0x0074caa8(param_1[6],apiStack_40);
  func_0x0080a73c(param_1[6],1);
  func_0x00734fc8(param_1[2],param_1[6],0,0);
  uVar3 = __Znwm(0x58);
  func_0x0001699c(uVar3,param_1[1]);
  param_1[7] = uVar3;
  func_0x002efbd0(&piStack_48,"simpleMatchLabel",0x10);
  func_0x003cec84(uVar3,&piStack_48);
  if (*piStack_48 != -1) {
    if (*piStack_48 != 0) {
      LOAcquire();
      iVar2 = *piStack_48;
      *piStack_48 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e0444;
    }
    func_0x00287df8(piStack_48,2,8);
  }
LAB_000e0444:
  func_0x0074b9bc(param_1[7],0x3c,0x11);
  func_0x0074bd40(param_1[7],0x3c,0x11);
  func_0x0074caa8(param_1[7],apiStack_40);
  func_0x00742624(param_1[7],1);
  func_0x007d007c(param_1[7],2);
  func_0x007d01a0(param_1[7],0x30);
  func_0x0080a6d8(param_1[7],0x84);
  func_0x00734fc8(param_1[2],param_1[7],0,0);
  func_0x00734fc8(*param_1,param_1[1],0,0);
  func_0x000e07dc(param_1,param_2);
  func_0x003d39dc(param_2);
  func_0x004d42c0(apiStack_40);
  return;
}


