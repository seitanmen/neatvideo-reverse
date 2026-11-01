// FUN_000eb7fc size=2988

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_000eb7fc(undefined8 *param_1,undefined8 param_2)

{
  int iVar1;
  int iVar2;
  undefined8 uVar3;
  undefined8 *puVar4;
  undefined8 uStack_78;
  int *piStack_70;
  int *piStack_68;
  undefined8 uStack_60;
  undefined8 uStack_58;
  undefined8 uStack_50;
  undefined1 auStack_48 [8];
  int *piStack_40;
  undefined8 uStack_38;
  
  func_0x003cec40(&piStack_40,param_2);
  iVar2 = piStack_40[1];
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar1 = *piStack_40;
      *piStack_40 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000eb85c;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000eb85c:
  if (iVar2 == 0) {
    func_0x002efbd0(&piStack_40,"profilePageToolbar",0x12);
    func_0x003cec84(param_2,&piStack_40);
    if (*piStack_40 != -1) {
      if (*piStack_40 != 0) {
        LOAcquire();
        iVar2 = *piStack_40;
        *piStack_40 = iVar2 + -1;
        LORelease();
        if (iVar2 != 1) goto LAB_000eb8b4;
      }
      func_0x00287df8(piStack_40,2,8);
    }
  }
LAB_000eb8b4:
  piStack_40 = _UNK_00c948e0;
  func_0x00751eb8(param_2,&piStack_40);
  uVar3 = __Znwm(0x20);
  func_0x007350f0(uVar3,param_2);
  *param_1 = uVar3;
  func_0x00733ed8(uVar3,0);
  uVar3 = *param_1;
  func_0x002efbd0(&piStack_40,"horizontalLayout",0x10);
  func_0x003cec84(uVar3,&piStack_40);
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar2 = *piStack_40;
      *piStack_40 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000eb948;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000eb948:
  func_0x0073abe0(*param_1,0,0,0,0);
  uVar3 = __Znwm(0x30);
  func_0x00742ef4(uVar3,param_2,0);
  param_1[1] = uVar3;
  func_0x002efbd0(&piStack_40,"buttonsWidget",0xd);
  func_0x003cec84(uVar3,&piStack_40);
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar2 = *piStack_40;
      *piStack_40 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000eb9d0;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000eb9d0:
  func_0x0074b9bc(param_1[1],0x1ef,0x20);
  func_0x0074bd40(param_1[1],0xffffff,0x20);
  uVar3 = __Znwm(0xd0);
  func_0x000294d0(uVar3,param_1[1]);
  param_1[2] = uVar3;
  func_0x002efbd0(&piStack_40,"autoProfileButton",0x11);
  func_0x003cec84(uVar3,&piStack_40);
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar2 = *piStack_40;
      *piStack_40 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000eba5c;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000eba5c:
  uStack_38 = _UNK_00c948f8;
  piStack_40 = _UNK_00c948f0;
  func_0x00751fb4(param_1[2],&piStack_40);
  func_0x0074b9bc(param_1[2],0,0x18);
  func_0x0074bd40(param_1[2],0xffffff,0x18);
  uVar3 = param_1[2];
  func_0x00471f60(&piStack_40,0);
  func_0x0074d1a0(uVar3,&piStack_40);
  func_0x004715ec(&piStack_40);
  func_0x004bb978(auStack_48);
  func_0x002efbd0(&piStack_40,":/icons/small/AutoProfile.png",0x1d);
  uStack_50 = 0xffffffffffffffff;
  func_0x004bbb30(auStack_48,&piStack_40,&uStack_50,0,1);
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar2 = *piStack_40;
      *piStack_40 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000ebb28;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000ebb28:
  func_0x007d5f88(param_1[2],auStack_48);
  func_0x00885fc8(param_1[2],2);
  uVar3 = __Znwm(0x88);
  func_0x00040054(uVar3,param_1[1]);
  param_1[3] = uVar3;
  func_0x002efbd0(&piStack_40,"loadProfileButton",0x11);
  func_0x003cec84(uVar3,&piStack_40);
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar2 = *piStack_40;
      *piStack_40 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000ebbac;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000ebbac:
  uStack_38 = _UNK_00c94908;
  piStack_40 = _UNK_00c94900;
  func_0x00751fb4(param_1[3],&piStack_40);
  func_0x0074b9bc(param_1[3],0,0x18);
  func_0x0074bd40(param_1[3],0xffffff,0x18);
  func_0x004bb978(&uStack_50);
  func_0x002efbd0(&piStack_40,":/icons/small/ProfileOpenLong.png",0x21);
  uStack_58 = 0xffffffffffffffff;
  func_0x004bbb30(&uStack_50,&piStack_40,&uStack_58,0,1);
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar2 = *piStack_40;
      *piStack_40 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000ebc54;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000ebc54:
  func_0x007d5f88(param_1[3],&uStack_50);
  uVar3 = __Znwm(0x88);
  func_0x00040054(uVar3,param_1[1]);
  param_1[4] = uVar3;
  func_0x002efbd0(&piStack_40,"autoMatchButton",0xf);
  func_0x003cec84(uVar3,&piStack_40);
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar2 = *piStack_40;
      *piStack_40 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000ebccc;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000ebccc:
  uStack_38 = _UNK_00c94918;
  piStack_40 = _UNK_00c94910;
  func_0x00751fb4(param_1[4],&piStack_40);
  func_0x0074b9bc(param_1[4],0,0x18);
  func_0x0074bd40(param_1[4],0xffffff,0x18);
  func_0x004bb978(&uStack_58);
  func_0x002efbd0(&piStack_40,":/icons/small/AutoMatch.png",0x1b);
  uStack_60 = 0xffffffffffffffff;
  func_0x004bbb30(&uStack_58,&piStack_40,&uStack_60,0,1);
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar2 = *piStack_40;
      *piStack_40 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000ebd74;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000ebd74:
  func_0x007d5f88(param_1[4],&uStack_58);
  uVar3 = __Znwm(0x88);
  func_0x00040054(uVar3,param_1[1]);
  param_1[5] = uVar3;
  func_0x002efbd0(&piStack_40,"genericProfileButton",0x14);
  func_0x003cec84(uVar3,&piStack_40);
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar2 = *piStack_40;
      *piStack_40 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000ebdec;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000ebdec:
  uStack_38 = _UNK_00c94928;
  piStack_40 = _UNK_00c94920;
  func_0x00751fb4(param_1[5],&piStack_40);
  func_0x0074b9bc(param_1[5],0,0x18);
  func_0x0074bd40(param_1[5],0xffffff,0x18);
  func_0x004bb978(&uStack_60);
  func_0x002efbd0(&piStack_40,":/icons/small/GenericProfile.png",0x20);
  piStack_68 = (int *)0xffffffffffffffff;
  func_0x004bbb30(&uStack_60,&piStack_40,&piStack_68,0,1);
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar2 = *piStack_40;
      *piStack_40 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000ebe94;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000ebe94:
  func_0x007d5f88(param_1[5],&uStack_60);
  func_0x007d5fe8(param_1[5],1);
  func_0x00734fc8(*param_1,param_1[1],0,0);
  puVar4 = (undefined8 *)__Znwm(0x28);
  *puVar4 = &UNK_013e41b0;
  uVar3 = _UNK_00c95d00;
  puVar4[2] = _UNK_00c95d08;
  puVar4[1] = uVar3;
  uVar3 = _UNK_010b4460;
  puVar4[4] = _UNK_010b4468;
  puVar4[3] = uVar3;
  param_1[6] = puVar4;
  (**(code **)(*(long *)*param_1 + 0x70))();
  uVar3 = __Znwm(0x30);
  func_0x00742ef4(uVar3,param_2,0);
  param_1[7] = uVar3;
  func_0x002efbd0(&piStack_40,"rightButtonsWidget",0x12);
  func_0x003cec84(uVar3,&piStack_40);
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar2 = *piStack_40;
      *piStack_40 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000ebf74;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000ebf74:
  uVar3 = __Znwm(0x20);
  func_0x007350f0(uVar3,param_1[7]);
  param_1[8] = uVar3;
  func_0x00733ed8(uVar3,4);
  uVar3 = param_1[8];
  func_0x002efbd0(&piStack_40,"horizontalLayout_2",0x12);
  func_0x003cec84(uVar3,&piStack_40);
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar2 = *piStack_40;
      *piStack_40 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000ebff0;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000ebff0:
  func_0x0073abe0(param_1[8],0,0,0,0);
  uVar3 = __Znwm(0x30);
  func_0x00742ef4(uVar3,param_1[7],0);
  param_1[9] = uVar3;
  func_0x002efbd0(&piStack_40,"profileCheckWidget",0x12);
  func_0x003cec84(uVar3,&piStack_40);
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar2 = *piStack_40;
      *piStack_40 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000ec078;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000ec078:
  func_0x0074b9bc(param_1[9],0x6e,0);
  func_0x0074bd40(param_1[9],0x6e,0xffffff);
  uVar3 = __Znwm(0xd0);
  func_0x000294d0(uVar3,param_1[9]);
  param_1[10] = uVar3;
  func_0x002efbd0(&piStack_40,"profileCheckButton",0x12);
  func_0x003cec84(uVar3,&piStack_40);
  if (*piStack_40 != -1) {
    if (*piStack_40 != 0) {
      LOAcquire();
      iVar2 = *piStack_40;
      *piStack_40 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000ec104;
    }
    func_0x00287df8(piStack_40,2,8);
  }
LAB_000ec104:
  uStack_38 = _UNK_00c94938;
  piStack_40 = _UNK_00c94930;
  func_0x00751fb4(param_1[10],&piStack_40);
  func_0x0074b9bc(param_1[10],0x6e,0x18);
  func_0x0074bd40(param_1[10],0x6e,0x18);
  func_0x004bb978(&piStack_40);
  func_0x002efbd0(&piStack_68,":/icons/small/TuningAssist.png",0x1e);
  piStack_70 = (int *)0xffffffffffffffff;
  func_0x004bbb30(&piStack_40,&piStack_68,&piStack_70,0,1);
  if (*piStack_68 != -1) {
    if (*piStack_68 != 0) {
      LOAcquire();
      iVar2 = *piStack_68;
      *piStack_68 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000ec1ac;
    }
    func_0x00287df8(piStack_68,2,8);
  }
LAB_000ec1ac:
  func_0x007d5f88(param_1[10],&piStack_40);
  func_0x007d5fe8(param_1[10],1);
  func_0x00885fc8(param_1[10],2);
  func_0x00734fc8(param_1[8],param_1[9],0,0);
  uVar3 = __Znwm(0x88);
  func_0x00040054(uVar3,param_1[7]);
  param_1[0xb] = uVar3;
  func_0x002efbd0(&piStack_68,"compareButton",0xd);
  func_0x003cec84(uVar3,&piStack_68);
  if (*piStack_68 != -1) {
    if (*piStack_68 != 0) {
      LOAcquire();
      iVar2 = *piStack_68;
      *piStack_68 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000ec24c;
    }
    func_0x00287df8(piStack_68,2,8);
  }
LAB_000ec24c:
  func_0x0074b9bc(param_1[0xb],0x55,0x18);
  func_0x0074bd40(param_1[0xb],0x55,0x18);
  func_0x004bb978(&piStack_68);
  func_0x002efbd0(&piStack_70,":/icons/small/CompareVariants.png",0x21);
  uStack_78 = 0xffffffffffffffff;
  func_0x004bbb30(&piStack_68,&piStack_70,&uStack_78,0,1);
  if (*piStack_70 != -1) {
    if (*piStack_70 != 0) {
      LOAcquire();
      iVar2 = *piStack_70;
      *piStack_70 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000ec2dc;
    }
    func_0x00287df8(piStack_70,2,8);
  }
LAB_000ec2dc:
  func_0x007d5f88(param_1[0xb],&piStack_68);
  func_0x007d5fe8(param_1[0xb],1);
  func_0x00734fc8(param_1[8],param_1[0xb],0,0);
  func_0x00734fc8(*param_1,param_1[7],0,0);
  func_0x00750f08(param_1[2],param_1[5]);
  func_0x00750f08(param_1[5],param_1[3]);
  func_0x00750f08(param_1[3],param_1[4]);
  func_0x00750f08(param_1[4],param_1[10]);
  func_0x00750f08(param_1[10],param_1[0xb]);
  FUN_000ed358(param_1,param_2);
  func_0x003d39dc(param_2);
  func_0x004bbe60(&piStack_68);
  func_0x004bbe60(&piStack_40);
  func_0x004bbe60(&uStack_60);
  func_0x004bbe60(&uStack_58);
  func_0x004bbe60(&uStack_50);
  func_0x004bbe60(auStack_48);
  return;
}


