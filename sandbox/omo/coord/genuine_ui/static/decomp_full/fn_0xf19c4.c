// FUN_000f19c4 size=7812

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_000f19c4(undefined8 *param_1,undefined8 param_2)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  undefined8 uVar4;
  ulong uVar5;
  undefined8 uVar6;
  undefined8 *puVar7;
  undefined8 local_68;
  int *local_60;
  undefined8 local_58;
  undefined8 local_50;
  undefined1 auStack_48 [8];
  int *local_40;
  undefined8 uStack_38;
  
  FUN_003cec40(&local_40,param_2);
  iVar3 = local_40[1];
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar2 = *local_40;
      *local_40 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000f1a24;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f1a24:
  if (iVar3 == 0) {
    FUN_002efbd0(&local_40,"ViewerToolbar",0xd);
    FUN_003cec84(param_2,&local_40);
    if (*local_40 != -1) {
      if (*local_40 != 0) {
        LOAcquire();
        iVar3 = *local_40;
        *local_40 = iVar3 + -1;
        LORelease();
        if (iVar3 != 1) goto LAB_000f1a7c;
      }
      FUN_00287df8(local_40,2,8);
    }
  }
LAB_000f1a7c:
  local_40 = DAT_00c94960;
  FUN_00751eb8(param_2,&local_40);
  FUN_00742624(param_2,1);
  uVar4 = __Znwm(0x20);
  FUN_00735284(uVar4,param_2);
  *param_1 = uVar4;
  FUN_00733ed8(uVar4,0);
  uVar4 = *param_1;
  FUN_002efbd0(&local_40,"vboxLayout",10);
  FUN_003cec84(uVar4,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f1b1c;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f1b1c:
  FUN_0073abe0(*param_1,0,0,0,0);
  uVar4 = __Znwm(0x30);
  FUN_007cfdcc(uVar4,param_2,0);
  param_1[1] = uVar4;
  FUN_002efbd0(&local_40,"viewerToolbarWidget",0x13);
  FUN_003cec84(uVar4,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f1ba4;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f1ba4:
  FUN_0074b9bc(param_1[1],0,0x18);
  FUN_0074bd40(param_1[1],0xffffff,0x18);
  uVar4 = __Znwm(0x20);
  FUN_00735284(uVar4,param_1[1]);
  param_1[2] = uVar4;
  FUN_00733ed8(uVar4,0);
  uVar4 = param_1[2];
  FUN_002efbd0(&local_40,"viewerToolbarWidgetLayout",0x19);
  FUN_003cec84(uVar4,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f1c40;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f1c40:
  FUN_0073abe0(param_1[2],0,0,0,0);
  uVar4 = __Znwm(0x30);
  FUN_00742ef4(uVar4,param_1[1],0);
  param_1[3] = uVar4;
  FUN_002efbd0(&local_40,"viewerControlsWidget",0x14);
  FUN_003cec84(uVar4,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f1cc8;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f1cc8:
  FUN_0074b9bc(param_1[3],0,0x17);
  FUN_0074bd40(param_1[3],0xffffff,0x17);
  uVar4 = __Znwm(0x20);
  FUN_007350f0(uVar4,param_1[3]);
  param_1[4] = uVar4;
  FUN_00733ed8(uVar4,0);
  uVar4 = param_1[4];
  FUN_002efbd0(&local_40,"hboxLayout",10);
  FUN_003cec84(uVar4,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f1d64;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f1d64:
  FUN_0073abe0(param_1[4],1,0,0,0);
  uVar4 = __Znwm(0x30);
  FUN_00742ef4(uVar4,param_1[3],0);
  param_1[5] = uVar4;
  FUN_002efbd0(&local_40,"zoomButtonWidget",0x10);
  FUN_003cec84(uVar4,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f1dec;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f1dec:
  FUN_0074b9bc(param_1[5],0x36,0x17);
  FUN_0074bd40(param_1[5],0x36,0x17);
  uVar4 = __Znwm(0xd0);
  thunk_FUN_000291dc(uVar4,param_1[5]);
  param_1[6] = uVar4;
  FUN_002efbd0(&local_40,"zoomButton",10);
  FUN_003cec84(uVar4,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f1e78;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f1e78:
  uStack_38 = _UNK_00c94978;
  local_40 = _DAT_00c94970;
  FUN_00751fb4(param_1[6],&local_40);
  uVar5 = FUN_00753f0c(param_1[6]);
  FUN_0075597c(param_1[6],uVar5 & 0x20000000 | 0x110000);
  FUN_0074b9bc(param_1[6],0x35,0x14);
  FUN_0074bd40(param_1[6],0x35,0x14);
  FUN_00887500(param_1[6],2);
  FUN_00885fc8(param_1[6],2);
  FUN_00734fc8(param_1[4],param_1[5],0,0);
  uVar4 = __Znwm(0x30);
  FUN_00742ef4(uVar4,param_1[3],0);
  param_1[7] = uVar4;
  FUN_002efbd0(&local_40,"viewerModeWidget",0x10);
  FUN_003cec84(uVar4,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f1f64;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f1f64:
  FUN_0074b9bc(param_1[7],0x54,0x17);
  FUN_0074bd40(param_1[7],0x54,0x17);
  uVar4 = __Znwm(0xd0);
  thunk_FUN_000291dc(uVar4,param_1[7]);
  param_1[8] = uVar4;
  FUN_002efbd0(&local_40,"viewerModeButton",0x10);
  FUN_003cec84(uVar4,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f1ff0;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f1ff0:
  uStack_38 = _UNK_00c94988;
  local_40 = _DAT_00c94980;
  FUN_00751fb4(param_1[8],&local_40);
  uVar5 = FUN_00753f0c(param_1[8]);
  FUN_0075597c(param_1[8],uVar5 & 0x20000000 | 0x110000);
  FUN_0074b9bc(param_1[8],0x51,0x14);
  FUN_0074bd40(param_1[8],0x51,0x14);
  FUN_00887500(param_1[8],2);
  FUN_00885fc8(param_1[8],2);
  FUN_00734fc8(param_1[4],param_1[7],0,0);
  uVar4 = __Znwm(0x30);
  FUN_00742ef4(uVar4,param_1[3],0);
  param_1[9] = uVar4;
  FUN_002efbd0(&local_40,"dragButtonWidget",0x10);
  FUN_003cec84(uVar4,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f20e0;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f20e0:
  FUN_0074b9bc(param_1[9],0x18,0);
  FUN_0074bd40(param_1[9],0x18,0xffffff);
  uVar4 = __Znwm(0x78);
  thunk_FUN_0002d7b8(uVar4,param_1[9]);
  param_1[10] = uVar4;
  FUN_002efbd0(&local_40,"dragButton",10);
  FUN_003cec84(uVar4,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f216c;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f216c:
  uVar4 = _UNK_00c94998;
  piVar1 = _DAT_00c94990;
  uStack_38 = _UNK_00c94998;
  local_40 = _DAT_00c94990;
  FUN_00751fb4(param_1[10],&local_40);
  FUN_0074b9bc(param_1[10],0x14,0x14);
  FUN_0074bd40(param_1[10],0x14,0x14);
  FUN_004bb978(auStack_48);
  FUN_002efbd0(&local_40,":/icons/small/Pan.png",0x15);
  local_50 = 0xffffffffffffffff;
  FUN_004bbb30(auStack_48,&local_40,&local_50,0,1);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f2218;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f2218:
  FUN_007d5f88(param_1[10],auStack_48);
  FUN_007d5fe8(param_1[10],1);
  FUN_00734fc8(param_1[4],param_1[9],0,0);
  uVar6 = __Znwm(0x30);
  FUN_00742ef4(uVar6,param_1[3],0);
  param_1[0xb] = uVar6;
  FUN_002efbd0(&local_40,"alphaButtonWidget",0x11);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f22b4;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f22b4:
  FUN_0074b9bc(param_1[0xb],0x18,0);
  FUN_0074bd40(param_1[0xb],0x18,0xffffff);
  uVar6 = __Znwm(0x78);
  thunk_FUN_0002d7b8(uVar6,param_1[0xb]);
  param_1[0xc] = uVar6;
  FUN_002efbd0(&local_40,"alphaButton",0xb);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f2340;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f2340:
  uStack_38 = uVar4;
  local_40 = piVar1;
  FUN_00751fb4(param_1[0xc],&local_40);
  FUN_0074b9bc(param_1[0xc],0x14,0x14);
  FUN_0074bd40(param_1[0xc],0x14,0x14);
  FUN_004bb978(&local_50);
  FUN_002efbd0(&local_40,":/icons/small/Alpha.png",0x17);
  local_58 = 0xffffffffffffffff;
  FUN_004bbb30(&local_50,&local_40,&local_58,0,1);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f23e4;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f23e4:
  FUN_007d5f88(param_1[0xc],&local_50);
  FUN_007d5fe8(param_1[0xc],1);
  FUN_00734fc8(param_1[4],param_1[0xb],0,0);
  puVar7 = (undefined8 *)__Znwm(0x90);
  FUN_0001ae50(puVar7,param_1[3],1);
  *puVar7 = &DAT_013bc310;
  puVar7[2] = &DAT_013bc4c0;
  puVar7[6] = &DAT_013bc510;
  param_1[0xd] = puVar7;
  FUN_002efbd0(&local_40,"buttonsLine",0xb);
  FUN_003cec84(puVar7,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f24a0;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f24a0:
  FUN_0074b9bc(param_1[0xd],2,0x16);
  FUN_0074bd40(param_1[0xd],2,0x16);
  FUN_007d007c(param_1[0xd],5);
  FUN_007d01a0(param_1[0xd],0x20);
  uVar6 = param_1[0xd];
  FUN_003df018(&local_40,5);
  FUN_003d5aa0(uVar6,"orientation",&local_40);
  FUN_003dec3c(&local_40);
  FUN_00734fc8(param_1[4],param_1[0xd],0,0);
  uVar6 = __Znwm(0x30);
  FUN_00742ef4(uVar6,param_1[3],0);
  param_1[0xe] = uVar6;
  FUN_002efbd0(&local_40,"levelAdjustmentWidget",0x15);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f2588;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f2588:
  FUN_0074b9bc(param_1[0xe],0x60,0);
  FUN_0074bd40(param_1[0xe],0x60,0xffffff);
  uVar6 = __Znwm(0x130);
  thunk_FUN_0002db30(uVar6,param_1[0xe]);
  param_1[0xf] = uVar6;
  FUN_002efbd0(&local_40,"levelAdjustmentSlider",0x15);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f2614;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f2614:
  uStack_38 = _UNK_00c949a8;
  local_40 = _DAT_00c949a0;
  FUN_00751fb4(param_1[0xf],&local_40);
  FUN_0074b9bc(param_1[0xf],0x46,0x12);
  FUN_0074bd40(param_1[0xf],0x46,0x12);
  FUN_0002ec80(param_1[0xf],0);
  FUN_0002edd0(param_1[0xf],100);
  FUN_007d7d04(param_1[0xf],1);
  FUN_007d7d6c(param_1[0xf],1);
  FUN_0002e320(param_1[0xf],0x32);
  FUN_007d7abc(param_1[0xf],1);
  uVar6 = __Znwm(0x30);
  FUN_008859b4(uVar6,param_1[0xe]);
  param_1[0x10] = uVar6;
  FUN_002efbd0(&local_40,"resetLevelAdjustmentButton",0x1a);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f2700;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f2700:
  uStack_38 = uVar4;
  local_40 = piVar1;
  FUN_00751fb4(param_1[0x10],&local_40);
  FUN_0074b9bc(param_1[0x10],0x14,0x14);
  FUN_0074bd40(param_1[0x10],0x14,0x14);
  FUN_004bb978(&local_58);
  FUN_002efbd0(&local_40,":/icons/small/Brightness.png",0x1c);
  local_60 = (int *)0xffffffffffffffff;
  FUN_004bbb30(&local_58,&local_40,&local_60,0,1);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f27a4;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f27a4:
  FUN_007d5f88(param_1[0x10],&local_58);
  FUN_00734fc8(param_1[4],param_1[0xe],0,0);
  puVar7 = (undefined8 *)__Znwm(0x90);
  FUN_0001ae50(puVar7,param_1[3],1);
  *puVar7 = &DAT_013bc310;
  puVar7[2] = &DAT_013bc4c0;
  puVar7[6] = &DAT_013bc510;
  param_1[0x11] = puVar7;
  FUN_002efbd0(&local_40,"levelsPresetLine",0x10);
  FUN_003cec84(puVar7,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f284c;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f284c:
  FUN_0074b9bc(param_1[0x11],2,0x16);
  FUN_0074bd40(param_1[0x11],2,0x16);
  FUN_007d007c(param_1[0x11],5);
  FUN_007d01a0(param_1[0x11],0x20);
  uVar6 = param_1[0x11];
  FUN_003df018(&local_40,5);
  FUN_003d5aa0(uVar6,"orientation",&local_40);
  FUN_003dec3c(&local_40);
  FUN_00734fc8(param_1[4],param_1[0x11],0,0);
  uVar6 = __Znwm(0x30);
  FUN_00742ef4(uVar6,param_1[3],0);
  param_1[0x12] = uVar6;
  FUN_002efbd0(&local_40,"whWidget",8);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f2934;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f2934:
  uVar6 = __Znwm(0x20);
  FUN_007350f0(uVar6,param_1[0x12]);
  param_1[0x13] = uVar6;
  FUN_00733ed8(uVar6,0);
  uVar6 = param_1[0x13];
  FUN_002efbd0(&local_40,"hboxLayout1",0xb);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f29b0;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f29b0:
  FUN_0073abe0(param_1[0x13],3,1,3,2);
  uVar6 = __Znwm(0x30);
  FUN_00809ca8(uVar6,param_1[0x12],0);
  param_1[0x14] = uVar6;
  FUN_002efbd0(&local_40,"wLabel",6);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f2a38;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f2a38:
  FUN_0074b9bc(param_1[0x14],0x28,0x14);
  FUN_0074bd40(param_1[0x14],0x28,0x14);
  FUN_00734fc8(param_1[0x13],param_1[0x14],0,0);
  uVar6 = __Znwm(0x30);
  FUN_00809ca8(uVar6,param_1[0x12],0);
  param_1[0x15] = uVar6;
  FUN_002efbd0(&local_40,"hLabel",6);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f2ad8;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f2ad8:
  FUN_0074b9bc(param_1[0x15],0x28,0x14);
  FUN_0074bd40(param_1[0x15],0x28,0x14);
  FUN_00734fc8(param_1[0x13],param_1[0x15],0,0);
  FUN_00734fc8(param_1[4],param_1[0x12],0,0);
  puVar7 = (undefined8 *)__Znwm(0x90);
  FUN_0001ae50(puVar7,param_1[3],1);
  *puVar7 = &DAT_013bc310;
  puVar7[2] = &DAT_013bc4c0;
  puVar7[6] = &DAT_013bc510;
  param_1[0x16] = puVar7;
  FUN_002efbd0(&local_40,"whLine",6);
  FUN_003cec84(puVar7,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f2ba8;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f2ba8:
  FUN_0074b9bc(param_1[0x16],2,0x16);
  FUN_0074bd40(param_1[0x16],2,0x16);
  FUN_007d007c(param_1[0x16],5);
  FUN_007d01a0(param_1[0x16],0x20);
  uVar6 = param_1[0x16];
  FUN_003df018(&local_40,5);
  FUN_003d5aa0(uVar6,"orientation",&local_40);
  FUN_003dec3c(&local_40);
  FUN_00734fc8(param_1[4],param_1[0x16],0,0);
  uVar6 = __Znwm(0x30);
  FUN_00742ef4(uVar6,param_1[3],0);
  param_1[0x17] = uVar6;
  FUN_002efbd0(&local_40,"rgbWidget",9);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f2c90;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f2c90:
  uVar6 = __Znwm(0x20);
  FUN_007350f0(uVar6,param_1[0x17]);
  param_1[0x18] = uVar6;
  FUN_00733ed8(uVar6,0);
  uVar6 = param_1[0x18];
  FUN_002efbd0(&local_40,"hboxLayout2",0xb);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f2d0c;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f2d0c:
  FUN_0073abe0(param_1[0x18],3,1,3,2);
  uVar6 = __Znwm(0x30);
  FUN_00809ca8(uVar6,param_1[0x17],0);
  param_1[0x19] = uVar6;
  FUN_002efbd0(&local_40,"rLabel",6);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f2d94;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f2d94:
  uVar5 = FUN_00753f0c(param_1[0x19]);
  FUN_0075597c(param_1[0x19],uVar5 & 0x20000000 | 0x150000);
  FUN_0074b9bc(param_1[0x19],0x28,0x14);
  FUN_0074bd40(param_1[0x19],0x28,0x14);
  FUN_00734fc8(param_1[0x18],param_1[0x19],0,0);
  uVar6 = __Znwm(0x30);
  FUN_00809ca8(uVar6,param_1[0x17],0);
  param_1[0x1a] = uVar6;
  FUN_002efbd0(&local_40,"gLabel",6);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f2e50;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f2e50:
  FUN_0074b9bc(param_1[0x1a],0x28,0x14);
  FUN_0074bd40(param_1[0x1a],0x28,0x14);
  FUN_00734fc8(param_1[0x18],param_1[0x1a],0,0);
  uVar6 = __Znwm(0x30);
  FUN_00809ca8(uVar6,param_1[0x17],0);
  param_1[0x1b] = uVar6;
  FUN_002efbd0(&local_40,"bLabel",6);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f2ef4;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f2ef4:
  FUN_0074b9bc(param_1[0x1b],0x28,0x14);
  FUN_0074bd40(param_1[0x1b],0x28,0x14);
  FUN_00734fc8(param_1[0x18],param_1[0x1b],0,0);
  FUN_00734fc8(param_1[4],param_1[0x17],0,0);
  puVar7 = (undefined8 *)__Znwm(0x28);
  *puVar7 = &DAT_013e41b0;
  uVar6 = _DAT_00c949b0;
  puVar7[2] = _UNK_00c949b8;
  puVar7[1] = uVar6;
  uVar6 = _DAT_010b4460;
  puVar7[4] = _UNK_010b4468;
  puVar7[3] = uVar6;
  param_1[0x1c] = puVar7;
  (**(code **)(*(long *)param_1[4] + 0x70))();
  puVar7 = (undefined8 *)__Znwm(0x90);
  FUN_0001ae50(puVar7,param_1[3],1);
  *puVar7 = &DAT_013bc310;
  puVar7[2] = &DAT_013bc4c0;
  puVar7[6] = &DAT_013bc510;
  param_1[0x1d] = puVar7;
  FUN_002efbd0(&local_40,"sizeLeftLine",0xc);
  FUN_003cec84(puVar7,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f300c;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f300c:
  FUN_0074b9bc(param_1[0x1d],2,0x16);
  FUN_0074bd40(param_1[0x1d],2,0x16);
  FUN_007d007c(param_1[0x1d],5);
  FUN_007d01a0(param_1[0x1d],0x20);
  uVar6 = param_1[0x1d];
  FUN_003df018(&local_40,5);
  FUN_003d5aa0(uVar6,"orientation",&local_40);
  FUN_003dec3c(&local_40);
  FUN_00734fc8(param_1[4],param_1[0x1d],0,0);
  uVar6 = __Znwm(0x30);
  FUN_00742ef4(uVar6,param_1[3],0);
  param_1[0x1e] = uVar6;
  FUN_002efbd0(&local_40,"sizeWidget",10);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f30f4;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f30f4:
  uVar6 = __Znwm(0x20);
  FUN_007350f0(uVar6,param_1[0x1e]);
  param_1[0x1f] = uVar6;
  FUN_00733ed8(uVar6,0);
  uVar6 = param_1[0x1f];
  FUN_002efbd0(&local_40,"hboxLayout3",0xb);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f3170;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f3170:
  FUN_0073abe0(param_1[0x1f],3,1,3,2);
  uVar6 = __Znwm(0x30);
  FUN_00809ca8(uVar6,param_1[0x1e],0);
  param_1[0x20] = uVar6;
  FUN_002efbd0(&local_40,"sizeLabel",9);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f31f8;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f31f8:
  FUN_0074b9bc(param_1[0x20],0,0x14);
  FUN_0074bd40(param_1[0x20],0xffffff,0x14);
  FUN_00734fc8(param_1[0x1f],param_1[0x20],0,0);
  FUN_00734fc8(param_1[4],param_1[0x1e],0,0);
  puVar7 = (undefined8 *)__Znwm(0x90);
  FUN_0001ae50(puVar7,param_1[3],1);
  *puVar7 = &DAT_013bc310;
  puVar7[2] = &DAT_013bc4c0;
  puVar7[6] = &DAT_013bc510;
  param_1[0x21] = puVar7;
  FUN_002efbd0(&local_40,"sizeLine",8);
  FUN_003cec84(puVar7,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f32c4;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f32c4:
  FUN_0074b9bc(param_1[0x21],2,0x17);
  FUN_0074bd40(param_1[0x21],2,0x17);
  FUN_007d007c(param_1[0x21],5);
  FUN_007d01a0(param_1[0x21],0x20);
  uVar6 = param_1[0x21];
  FUN_003df018(&local_40,5);
  FUN_003d5aa0(uVar6,"orientation",&local_40);
  FUN_003dec3c(&local_40);
  FUN_00734fc8(param_1[4],param_1[0x21],0,0);
  uVar6 = __Znwm(0x30);
  FUN_00742ef4(uVar6,param_1[3],0);
  param_1[0x22] = uVar6;
  FUN_002efbd0(&local_40,"isoWidget",9);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f33ac;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f33ac:
  uVar6 = __Znwm(0x20);
  FUN_007350f0(uVar6,param_1[0x22]);
  param_1[0x23] = uVar6;
  FUN_00733ed8(uVar6,0);
  uVar6 = param_1[0x23];
  FUN_002efbd0(&local_40,"hboxLayout4",0xb);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f3428;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f3428:
  FUN_0073abe0(param_1[0x23],3,1,3,2);
  uVar6 = __Znwm(0x30);
  FUN_00809ca8(uVar6,param_1[0x22],0);
  param_1[0x24] = uVar6;
  FUN_002efbd0(&local_40,"isoLabel",8);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f34b0;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f34b0:
  FUN_0074b9bc(param_1[0x24],0,0x14);
  FUN_0074bd40(param_1[0x24],0xffffff,0x14);
  FUN_00734fc8(param_1[0x23],param_1[0x24],0,0);
  FUN_00734fc8(param_1[4],param_1[0x22],0,0);
  puVar7 = (undefined8 *)__Znwm(0x90);
  FUN_0001ae50(puVar7,param_1[3],1);
  *puVar7 = &DAT_013bc310;
  puVar7[2] = &DAT_013bc4c0;
  puVar7[6] = &DAT_013bc510;
  param_1[0x25] = puVar7;
  FUN_002efbd0(&local_40,"isoLine",7);
  FUN_003cec84(puVar7,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f357c;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f357c:
  FUN_0074b9bc(param_1[0x25],2,0x16);
  FUN_0074bd40(param_1[0x25],2,0x16);
  FUN_007d007c(param_1[0x25],5);
  FUN_007d01a0(param_1[0x25],0x20);
  uVar6 = param_1[0x25];
  FUN_003df018(&local_40,5);
  FUN_003d5aa0(uVar6,"orientation",&local_40);
  FUN_003dec3c(&local_40);
  FUN_00734fc8(param_1[4],param_1[0x25],0,0);
  uVar6 = __Znwm(0x30);
  FUN_00742ef4(uVar6,param_1[3],0);
  param_1[0x26] = uVar6;
  FUN_002efbd0(&local_40,"infoButtonWidget",0x10);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f3664;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f3664:
  FUN_0074b9bc(param_1[0x26],0x18,0);
  FUN_0074bd40(param_1[0x26],0x18,0xffffff);
  uVar6 = __Znwm(0x30);
  FUN_008859b4(uVar6,param_1[0x26]);
  param_1[0x27] = uVar6;
  FUN_002efbd0(&local_40,"imageInfoButton",0xf);
  FUN_003cec84(uVar6,&local_40);
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f36f0;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_000f36f0:
  uStack_38 = uVar4;
  local_40 = piVar1;
  FUN_00751fb4(param_1[0x27],&local_40);
  FUN_0074b9bc(param_1[0x27],0x14,0x14);
  FUN_0074bd40(param_1[0x27],0x14,0x14);
  FUN_004bb978(&local_40);
  FUN_002efbd0(&local_60,":/icons/small/InfoWhite.png",0x1b);
  local_68 = 0xffffffffffffffff;
  FUN_004bbb30(&local_40,&local_60,&local_68,0,1);
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar3 = *local_60;
      *local_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f3794;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000f3794:
  FUN_007d5f88(param_1[0x27],&local_40);
  FUN_00734fc8(param_1[4],param_1[0x26],0,0);
  FUN_00734fc8(param_1[2],param_1[3],0,0);
  FUN_00734fc8(*param_1,param_1[1],0,0);
  FUN_00750f08(param_1[6],param_1[8]);
  FUN_00750f08(param_1[8],param_1[0x10]);
  FUN_00750f08(param_1[0x10],param_1[0xf]);
  FUN_00750f08(param_1[0xf],param_1[0x27]);
  FUN_000f6e90(param_1,param_2);
  FUN_003d39dc(param_2);
  FUN_004bbe60(&local_40);
  FUN_004bbe60(&local_58);
  FUN_004bbe60(&local_50);
  FUN_004bbe60(auStack_48);
  return;
}


