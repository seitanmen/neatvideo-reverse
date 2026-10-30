// func entry: 000c74e0 name: FUN_000c74e0 size: 8520

long FUN_000c74e0(long param_1)

{
  int iVar1;
  undefined8 *puVar2;
  undefined8 uVar3;
  long lVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  undefined8 uVar7;
  undefined8 in_stack_fffffffffffffed0;
  uint uVar10;
  ulong uVar8;
  int **ppiVar9;
  undefined4 uVar11;
  undefined4 uVar12;
  undefined1 auStack_e8 [8];
  undefined1 auStack_e0 [8];
  int *local_d8;
  int *local_d0;
  int *local_c8;
  int *local_c0;
  int *local_b8;
  int *local_b0;
  int *local_a8;
  int *local_a0;
  int *local_98;
  int *local_90;
  int *local_88;
  int *local_80;
  int *local_78;
  int *local_70;
  int *local_68;
  int *local_60;
  long local_58;
  
  uVar10 = (uint)((ulong)in_stack_fffffffffffffed0 >> 0x20);
  local_58 = *(long *)PTR____stack_chk_guard_013b4320;
  puVar2 = (undefined8 *)FUN_000bf708();
  *puVar2 = &DAT_013c4820;
  puVar2[2] = &DAT_013c4a30;
  puVar2[6] = &DAT_013c4a80;
  uVar3 = __Znwm(0xe390);
  FUN_001b0088(uVar3,*(undefined4 *)(DAT_0155b458 + 400),1);
  *(undefined8 *)(param_1 + 0xda0) = uVar3;
  *(undefined8 *)(*(long *)(param_1 + 0xda8) + 0x18) = uVar3;
  lVar4 = FUN_000c48c8(param_1);
  uVar3 = *(undefined8 *)(*(long *)(param_1 + 0x148) + 0x50);
  FUN_003bbff0(&local_80,&DAT_013c47e0,"Temporal settings",0,0xffffffff);
  FUN_00750130(uVar3,&local_80);
  if (*local_80 != -1) {
    if (*local_80 != 0) {
      LOAcquire();
      iVar1 = *local_80;
      *local_80 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c75d8;
    }
    FUN_00287df8(local_80,2,8);
  }
LAB_000c75d8:
  uVar5 = FUN_00850130(uVar3);
  FUN_003bbff0(&local_80,&DAT_013c47e0,"Temporal Filter",0,0xffffffff);
  local_a8 = (int *)FUN_002efa1c("",0);
  local_b0 = (int *)FUN_002efa1c("Enable Temporal Filter",0x16);
  puVar2 = (undefined8 *)(param_1 + 0xce0);
  FUN_00039518(uVar3,uVar5,puVar2,&local_80,&local_a8,&local_b0,0x5d,lVar4 + 0xcd,
               (ulong)uVar10 << 0x20);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c768c;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c768c:
  if (*local_a8 != -1) {
    if (*local_a8 != 0) {
      LOAcquire();
      iVar1 = *local_a8;
      *local_a8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c76c0;
    }
    FUN_00287df8(local_a8,2,8);
  }
LAB_000c76c0:
  if (*local_80 != -1) {
    if (*local_80 != 0) {
      LOAcquire();
      iVar1 = *local_80;
      *local_80 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c76f4;
    }
    FUN_00287df8(local_80,2,8);
  }
LAB_000c76f4:
  local_80 = (int *)FUN_002efa1c("1",1);
  local_78 = (int *)FUN_002efa1c("2",1);
  local_70 = (int *)FUN_002efa1c("3",1);
  local_68 = (int *)FUN_002efa1c("4",1);
  local_60 = (int *)FUN_002efa1c("5",1);
  FUN_003bbff0(&local_a8,&DAT_013c47e0,"1 frame",0,0xffffffff);
  FUN_003bbff0(&local_a0,&DAT_013c47e0,"2 frames",0,0xffffffff);
  FUN_003bbff0(&local_98,&DAT_013c47e0,"3 frames",0,0xffffffff);
  FUN_003bbff0(&local_90,&DAT_013c47e0,"4 frames",0,0xffffffff);
  FUN_003bbff0(&local_88,&DAT_013c47e0,"5 frames",0,0xffffffff);
  uVar5 = *puVar2;
  FUN_003bbff0(&local_b0,&DAT_013c47e0,"Radius",0,0xffffffff);
  local_b8 = (int *)FUN_002efa1c("Radius determines the number of consecutive frames (before and after the current one) used for temporal filtration"
                                 ,0x72);
  local_c0 = (int *)FUN_002efa1c("",0);
  local_c8 = (int *)FUN_002efa1c("",0);
  FUN_003bbff0(&local_d0,&DAT_013c47e0,"Radius",0,0xffffffff);
  uVar8 = 0x500000001;
  FUN_0003b2d4(uVar3,uVar5,&local_b0,&local_b8,param_1 + 0xce8,&local_80,0,&local_a8,0x500000001,
               lVar4 + 0xd0,&local_c0,&local_c8,&local_d0,0xc00000002,0x100);
  if (*local_d0 != -1) {
    if (*local_d0 != 0) {
      LOAcquire();
      iVar1 = *local_d0;
      *local_d0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7938;
    }
    FUN_00287df8(local_d0,2,8);
  }
LAB_000c7938:
  if (*local_c8 != -1) {
    if (*local_c8 != 0) {
      LOAcquire();
      iVar1 = *local_c8;
      *local_c8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c796c;
    }
    FUN_00287df8(local_c8,2,8);
  }
LAB_000c796c:
  if (*local_c0 != -1) {
    if (*local_c0 != 0) {
      LOAcquire();
      iVar1 = *local_c0;
      *local_c0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c79a0;
    }
    FUN_00287df8(local_c0,2,8);
  }
LAB_000c79a0:
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar1 = *local_b8;
      *local_b8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c79d4;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000c79d4:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7a08;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c7a08:
  FUN_0003c6d4(uVar3,*puVar2,0);
  if (((DAT_01538c28 & 1) == 0) && (iVar1 = ___cxa_guard_acquire(&DAT_01538c28), iVar1 != 0)) {
    DAT_01538c20 = *(float *)(lVar4 + 0x160) != 0.0 || *(float *)(lVar4 + 0x164) != 0.0;
    ___cxa_guard_release(&DAT_01538c28);
  }
  uVar5 = *(undefined8 *)(param_1 + 0xce0);
  FUN_003bbff0(&local_b0,&DAT_013c47e0,"Flicker",0,0xffffffff);
  local_b8 = (int *)FUN_002efa1c("This filter reduces flickering (sudden changes of brightness)",
                                 0x3d);
  FUN_00038ca4(uVar3,uVar5,
               "/System/Library/Frameworks/Cocoa.framework/Versions/A/Cocoa" + param_1 + 0x30,
               &local_b0,&local_b8,0x5d,&DAT_01538c20,0);
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar1 = *local_b8;
      *local_b8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7ac0;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000c7ac0:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7af4;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c7af4:
  FUN_0003c6d4(uVar3,*puVar2,0);
  uVar5 = *(undefined8 *)(param_1 + 0xce0);
  FUN_003bbff0(&local_b0,&DAT_013c47e0,"Dust and Scratches",0,0xffffffff);
  local_b8 = (int *)FUN_002efa1c("This filter removes dust, scratches, spots, etc.",0x30);
  FUN_00038ca4(uVar3,uVar5,param_1 + 0xcf0,&local_b0,&local_b8,0x5d,lVar4 + 0xd9,0);
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar1 = *local_b8;
      *local_b8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7b94;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000c7b94:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7bc8;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c7bc8:
  uVar5 = *(undefined8 *)(*(long *)(param_1 + 0x170) + 0x50);
  FUN_003bbff0(&local_b0,&DAT_013c47e0,"Spatial settings",0,0xffffffff);
  FUN_00750130(uVar5,&local_b0);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7c30;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c7c30:
  uVar6 = FUN_00850130(uVar5);
  FUN_003bbff0(&local_b0,&DAT_013c47e0,"Spatial Filter",0,0xffffffff);
  local_b8 = (int *)FUN_002efa1c("",0);
  local_c0 = (int *)FUN_002efa1c("Enable Spatial Filter",0x15);
  puVar2 = (undefined8 *)(param_1 + 0xcf8);
  FUN_00039518(uVar5,uVar6,puVar2,&local_b0,&local_b8,&local_c0,0x5d,lVar4,
               uVar8 & 0xffffffff00000000);
  if (*local_c0 != -1) {
    if (*local_c0 != 0) {
      LOAcquire();
      iVar1 = *local_c0;
      *local_c0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7ce4;
    }
    FUN_00287df8(local_c0,2,8);
  }
LAB_000c7ce4:
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar1 = *local_b8;
      *local_b8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7d18;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000c7d18:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7d4c;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c7d4c:
  uVar6 = *(undefined8 *)(param_1 + 0xcf8);
  FUN_003bbff0(&local_b0,&DAT_013c47e0,"Noise Reduction Amount",0,0xffffffff);
  local_b8 = (int *)FUN_002efa1c("Adjust amount of noise reduction applied to video\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                 ,0x95);
  local_c0 = (int *)((ulong)local_c0 & 0xffffffffffffff00);
  FUN_00038ca4(uVar5,uVar6,(undefined8 *)(param_1 + 0xd00),&local_b0,&local_b8,0x59,&local_c0,0);
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar1 = *local_b8;
      *local_b8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7de4;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000c7de4:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7e18;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c7e18:
  uVar6 = *(undefined8 *)(param_1 + 0xd00);
  FUN_003bbff0(&local_b0,&DAT_013c47e0,"Luminance",0,0xffffffff);
  local_b8 = (int *)FUN_002efa1c("ChannelY.png",0xc);
  local_c0 = (int *)FUN_002efa1c("Amount of noise reduction applied to Luminance channel component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                 ,0xa4);
  local_c8 = (int *)FUN_002efa1c("Adjust amount of noise reduction applied to Luminance channel component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                 ,0xab);
  FUN_003bbff0(&local_d0,&DAT_013c47e0,"Luminance Reduction Amount",0,0xffffffff);
  local_d8 = (int *)FUN_002efa1c("Amount of noise reduction applied to Luminance channel component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                 ,0xa4);
  ppiVar9 = &local_d8;
  uVar11 = 100;
  FUN_0003aa60(0,0x3f800000,uVar5,uVar6,0,&local_b0,0,&local_b8,&local_c0,param_1 + 0xd18,&local_c8,
               0x6400000000,0xa00000005,lVar4 + 0x3c,&local_d0,0,ppiVar9,0x5a,0);
  uVar12 = (undefined4)((ulong)ppiVar9 >> 0x20);
  if (*local_d8 != -1) {
    if (*local_d8 != 0) {
      LOAcquire();
      iVar1 = *local_d8;
      *local_d8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7f40;
    }
    FUN_00287df8(local_d8,2,8);
  }
LAB_000c7f40:
  if (*local_d0 != -1) {
    if (*local_d0 != 0) {
      LOAcquire();
      iVar1 = *local_d0;
      *local_d0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7f74;
    }
    FUN_00287df8(local_d0,2,8);
  }
LAB_000c7f74:
  if (*local_c8 != -1) {
    if (*local_c8 != 0) {
      LOAcquire();
      iVar1 = *local_c8;
      *local_c8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7fa8;
    }
    FUN_00287df8(local_c8,2,8);
  }
LAB_000c7fa8:
  if (*local_c0 != -1) {
    if (*local_c0 != 0) {
      LOAcquire();
      iVar1 = *local_c0;
      *local_c0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c7fdc;
    }
    FUN_00287df8(local_c0,2,8);
  }
LAB_000c7fdc:
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar1 = *local_b8;
      *local_b8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8010;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000c8010:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8044;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c8044:
  FUN_0003c6d4(uVar5,*puVar2,0);
  uVar6 = *(undefined8 *)(param_1 + 0xcf8);
  FUN_003bbff0(&local_b0,&DAT_013c47e0,"Noise Level",0,0xffffffff);
  local_b8 = (int *)FUN_002efa1c("Adjust estimation of noise level in video (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                 ,0xc3);
  local_c0 = (int *)((ulong)local_c0 & 0xffffffffffffff00);
  FUN_00038ca4(uVar5,uVar6,param_1 + 0xd08,&local_b0,&local_b8,0x59,&local_c0,0);
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar1 = *local_b8;
      *local_b8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c80e8;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000c80e8:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c811c;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c811c:
  uVar6 = *(undefined8 *)(param_1 + 0xd08);
  local_b8 = &DAT_0142f030;
  local_b0 = &DAT_0142f030;
  local_c0 = (int *)FUN_002efa1c("Adjust overall estimation of noise level (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                 ,0xc2);
  FUN_003bbff0(&local_c8,&DAT_013c47e0,"Overall Noise Level",0,0xffffffff);
  local_d0 = (int *)FUN_002efa1c("Estimation of overall noise level (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                 ,0xbb);
  FUN_0003abb8(0,0x40200000,uVar5,uVar6,0,&local_b0,&local_b8,param_1 + 0xd20,&local_c0,0xffffff9c,
               0x500000096,CONCAT44(uVar11,0x14),lVar4 + 4,&local_c8,0,&local_d0,
               CONCAT44(uVar12,0x12));
  if (*local_d0 != -1) {
    if (*local_d0 != 0) {
      LOAcquire();
      iVar1 = *local_d0;
      *local_d0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8204;
    }
    FUN_00287df8(local_d0,2,8);
  }
LAB_000c8204:
  if (*local_c8 != -1) {
    if (*local_c8 != 0) {
      LOAcquire();
      iVar1 = *local_c8;
      *local_c8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8238;
    }
    FUN_00287df8(local_c8,2,8);
  }
LAB_000c8238:
  if (*local_c0 != -1) {
    if (*local_c0 != 0) {
      LOAcquire();
      iVar1 = *local_c0;
      *local_c0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c826c;
    }
    FUN_00287df8(local_c0,2,8);
  }
LAB_000c826c:
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar1 = *local_b8;
      *local_b8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c82a0;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000c82a0:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c82d4;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c82d4:
  FUN_0003c6d4(uVar5,*puVar2,0);
  uVar6 = *(undefined8 *)(param_1 + 0xcf8);
  FUN_003bbff0(&local_b0,&DAT_013c47e0,"Sharpening",0,0xffffffff);
  local_b8 = (int *)FUN_002efa1c("Sharpen image after noise reduction. Expand to adjust sharpening parameters"
                                 ,0x4b);
  puVar2 = (undefined8 *)(param_1 + 0xd10);
  FUN_00038ca4(uVar5,uVar6,puVar2,&local_b0,&local_b8,0x5f,lVar4 + 0x90,0);
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar1 = *local_b8;
      *local_b8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8378;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000c8378:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c83ac;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c83ac:
  uVar6 = *puVar2;
  FUN_003bbff0(&local_b0,&DAT_013c47e0,"Fine",0,0xffffffff);
  local_b8 = (int *)FUN_002efa1c("HFSSImage.png",0xd);
  local_c0 = (int *)FUN_002efa1c("Example of sharpened fine details",0x21);
  local_c8 = (int *)FUN_002efa1c("Adjust amount of sharpening applied to fine details\n- high values -> more sharpening\n- low values  -> less sharpening"
                                 ,0x75);
  FUN_003bbff0(&local_d0,&DAT_013c47e0,"Fine details Sharpening Amount",0,0xffffffff);
  local_d8 = (int *)FUN_002efa1c("Amount of sharpening applied to fine details\n- high values -> more sharpening\n- low values  -> less sharpening"
                                 ,0x6e);
  FUN_0003aa60(0,0x40200000,uVar5,uVar6,0,&local_b0,0,&local_b8,&local_c0,param_1 + 0xd28,&local_c8,
               0xfa00000000,0x1400000005,lVar4 + 0xa0,&local_d0,0,&local_d8,0x5a,0);
  if (*local_d8 != -1) {
    if (*local_d8 != 0) {
      LOAcquire();
      iVar1 = *local_d8;
      *local_d8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c84d4;
    }
    FUN_00287df8(local_d8,2,8);
  }
LAB_000c84d4:
  if (*local_d0 != -1) {
    if (*local_d0 != 0) {
      LOAcquire();
      iVar1 = *local_d0;
      *local_d0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8508;
    }
    FUN_00287df8(local_d0,2,8);
  }
LAB_000c8508:
  if (*local_c8 != -1) {
    if (*local_c8 != 0) {
      LOAcquire();
      iVar1 = *local_c8;
      *local_c8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c853c;
    }
    FUN_00287df8(local_c8,2,8);
  }
LAB_000c853c:
  if (*local_c0 != -1) {
    if (*local_c0 != 0) {
      LOAcquire();
      iVar1 = *local_c0;
      *local_c0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8570;
    }
    FUN_00287df8(local_c0,2,8);
  }
LAB_000c8570:
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar1 = *local_b8;
      *local_b8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c85a4;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000c85a4:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c85d8;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c85d8:
  uVar6 = *puVar2;
  FUN_003bbff0(&local_b0,&DAT_013c47e0,"Medium",0,0xffffffff);
  local_b8 = (int *)FUN_002efa1c("MFSSImage.png",0xd);
  local_c0 = (int *)FUN_002efa1c("Example of sharpened medium-sized details",0x29);
  local_c8 = (int *)FUN_002efa1c("Adjust amount of sharpening applied to medium-sized details\n- high values -> more sharpening\n- low values  -> less sharpening"
                                 ,0x7d);
  FUN_003bbff0(&local_d0,&DAT_013c47e0,"Medium details Sharpening Amount",0,0xffffffff);
  local_d8 = (int *)FUN_002efa1c("Amount of sharpening applied to medium-sized details\n- high values -> more sharpening\n- low values  -> less sharpening"
                                 ,0x76);
  FUN_0003aa60(0,0x40200000,uVar5,uVar6,0,&local_b0,0,&local_b8,&local_c0,param_1 + 0xd30,&local_c8,
               0xfa00000000,0x1400000005,lVar4 + 0xa4,&local_d0,0,&local_d8,0x5a,0);
  if (*local_d8 != -1) {
    if (*local_d8 != 0) {
      LOAcquire();
      iVar1 = *local_d8;
      *local_d8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8700;
    }
    FUN_00287df8(local_d8,2,8);
  }
LAB_000c8700:
  if (*local_d0 != -1) {
    if (*local_d0 != 0) {
      LOAcquire();
      iVar1 = *local_d0;
      *local_d0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8734;
    }
    FUN_00287df8(local_d0,2,8);
  }
LAB_000c8734:
  if (*local_c8 != -1) {
    if (*local_c8 != 0) {
      LOAcquire();
      iVar1 = *local_c8;
      *local_c8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8768;
    }
    FUN_00287df8(local_c8,2,8);
  }
LAB_000c8768:
  if (*local_c0 != -1) {
    if (*local_c0 != 0) {
      LOAcquire();
      iVar1 = *local_c0;
      *local_c0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c879c;
    }
    FUN_00287df8(local_c0,2,8);
  }
LAB_000c879c:
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar1 = *local_b8;
      *local_b8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c87d0;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000c87d0:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8804;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c8804:
  uVar6 = *puVar2;
  FUN_003bbff0(&local_b0,&DAT_013c47e0,"Large",0,0xffffffff);
  local_b8 = (int *)FUN_002efa1c("LFSSImage.png",0xd);
  local_c0 = (int *)FUN_002efa1c("Example of sharpened large details",0x22);
  local_c8 = (int *)FUN_002efa1c("Adjust amount of sharpening applied to large details\n- high values -> more sharpening\n- low values  -> less sharpening"
                                 ,0x76);
  FUN_003bbff0(&local_d0,&DAT_013c47e0,"Large details Sharpening Amount",0,0xffffffff);
  local_d8 = (int *)FUN_002efa1c("Amount of sharpening applied to large details\n- high values -> more sharpening\n- low values  -> less sharpening"
                                 ,0x6f);
  uVar8 = 0;
  ppiVar9 = &local_c8;
  FUN_0003aa60(0,0x40200000,uVar5,uVar6,0,&local_b0,0,&local_b8,&local_c0,param_1 + 0xd38,ppiVar9,
               0xfa00000000,0x1400000005,lVar4 + 0xa8,&local_d0,0,&local_d8,0x5a,0);
  if (*local_d8 != -1) {
    if (*local_d8 != 0) {
      LOAcquire();
      iVar1 = *local_d8;
      *local_d8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c892c;
    }
    FUN_00287df8(local_d8,2,8);
  }
LAB_000c892c:
  if (*local_d0 != -1) {
    if (*local_d0 != 0) {
      LOAcquire();
      iVar1 = *local_d0;
      *local_d0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8960;
    }
    FUN_00287df8(local_d0,2,8);
  }
LAB_000c8960:
  if (*local_c8 != -1) {
    if (*local_c8 != 0) {
      LOAcquire();
      iVar1 = *local_c8;
      *local_c8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8994;
    }
    FUN_00287df8(local_c8,2,8);
  }
LAB_000c8994:
  if (*local_c0 != -1) {
    if (*local_c0 != 0) {
      LOAcquire();
      iVar1 = *local_c0;
      *local_c0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c89c8;
    }
    FUN_00287df8(local_c0,2,8);
  }
LAB_000c89c8:
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar1 = *local_b8;
      *local_b8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c89fc;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000c89fc:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8a30;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c8a30:
  uVar6 = *(undefined8 *)(*(long *)(param_1 + 0x198) + 0x50);
  FUN_003bbff0(&local_b0,&DAT_013c47e0,"General settings",0,0xffffffff);
  FUN_00750130(uVar6,&local_b0);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8a98;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c8a98:
  uVar7 = FUN_00850130(uVar6);
  FUN_003bbff0(&local_b0,&DAT_013c47e0,"Performance",0,0xffffffff);
  local_b8 = (int *)FUN_002efa1c("Adjust performance settings of Neat Video filters",0x31);
  local_c0 = &DAT_0142f030;
  local_c8 = (int *)((ulong)local_c8 & 0xffffffffffffff00);
  FUN_00039518(uVar6,uVar7,param_1 + 0xd40,&local_b0,&local_b8,&local_c0,0x59,&local_c8,
               (ulong)ppiVar9 & 0xffffffff00000000);
  if (*local_c0 != -1) {
    if (*local_c0 != 0) {
      LOAcquire();
      iVar1 = *local_c0;
      *local_c0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8b38;
    }
    FUN_00287df8(local_c0,2,8);
  }
LAB_000c8b38:
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar1 = *local_b8;
      *local_b8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8b6c;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000c8b6c:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8ba0;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c8ba0:
  uVar7 = *(undefined8 *)(param_1 + 0xd40);
  local_b0 = (int *)FUN_002efa1c("Current filter settings that affect processing speed the most",
                                 0x3d);
  FUN_00039a7c(uVar6,uVar7,(undefined8 *)(param_1 + 0xd48),&local_b0,0,0,1);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8c10;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c8c10:
  uVar7 = *(undefined8 *)(param_1 + 0xd48);
  FUN_003bbff0(&local_b0,&DAT_013c47e0,"Check Speed",0,0xffffffff);
  local_b8 = (int *)FUN_002efa1c("Measure processing speed achieved with current filter settings",
                                 0x3e);
  FUN_003bbff0(&local_c0,&DAT_013c47e0,"Preferences...",0,0xffffffff);
  local_c8 = (int *)FUN_002efa1c("Open Preferences to adjust and optimize CPU and GPU settings",0x3c
                                );
  local_d0 = (int *)FUN_002efa1c("Current filter settings that affect processing speed the most",
                                 0x3d);
  local_d8 = (int *)FUN_002efa1c("Processing speed achieved with current filter settings",0x36);
  FUN_0003c0d4(uVar6,uVar7,&local_b0,&local_b8,param_1 + 0xd50,&local_c0,&local_c8,param_1 + 0xd58,
               param_1 + 0xd68,param_1 + 0xd70,&local_d0,param_1 + 0xd78,&local_d8,
               uVar8 & 0xffffffff00000000);
  if (*local_d8 != -1) {
    if (*local_d8 != 0) {
      LOAcquire();
      iVar1 = *local_d8;
      *local_d8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8d20;
    }
    FUN_00287df8(local_d8,2,8);
  }
LAB_000c8d20:
  if (*local_d0 != -1) {
    if (*local_d0 != 0) {
      LOAcquire();
      iVar1 = *local_d0;
      *local_d0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8d54;
    }
    FUN_00287df8(local_d0,2,8);
  }
LAB_000c8d54:
  if (*local_c8 != -1) {
    if (*local_c8 != 0) {
      LOAcquire();
      iVar1 = *local_c8;
      *local_c8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8d88;
    }
    FUN_00287df8(local_c8,2,8);
  }
LAB_000c8d88:
  if (*local_c0 != -1) {
    if (*local_c0 != 0) {
      LOAcquire();
      iVar1 = *local_c0;
      *local_c0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8dbc;
    }
    FUN_00287df8(local_c0,2,8);
  }
LAB_000c8dbc:
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar1 = *local_b8;
      *local_b8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8df0;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000c8df0:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8e24;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c8e24:
  FUN_003d0e8c(auStack_e0,*(undefined8 *)(param_1 + 0xd50),"2clicked()",param_1,
               "1checkSpeedButtonClick()",0);
  FUN_003d24d8(auStack_e0);
  FUN_003d0e8c(auStack_e8,*(undefined8 *)(param_1 + 0xd58),"2clicked()",param_1,
               "1preferencesButtonClick()",0);
  FUN_003d24d8(auStack_e8);
  uVar7 = *(undefined8 *)(param_1 + 0x158);
  local_b0 = (int *)FUN_002efa1c("<div style=\"margin-bottom: 4px;\">Currently working in <b><nobr>Beginner Mode</nobr></b>.</div><div>For more control switch to <b><nobr><a href=dummy style=\"color: %1; text-decoration: none\">Advanced Mode</a></nobr></b>.</div>"
                                 ,0xe1);
  FUN_0002bbbc(uVar7,&local_b0);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8ed4;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c8ed4:
  uVar7 = *(undefined8 *)(param_1 + 0x158);
  local_b0 = (int *)FUN_002efa1c("<b>Advanced Mode</b> offers more control over noise filter and allows to give preference to higher quality vs higher speed of processing.<br><br>Switch to Advanced Mode to be able to utilize Neat Video to its full potential."
                                 ,0xe0);
  FUN_0002bbc4(uVar7,&local_b0);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8f2c;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c8f2c:
  uVar7 = *(undefined8 *)(param_1 + 0x150);
  local_b0 = (int *)FUN_002efa1c("<div>How to use: <b><nobr><a href=dummy style=\"color: %1; text-decoration: none\">Quick Start Guide</a></nobr></b>.</div>"
                                 ,0x78);
  FUN_0002bbbc(uVar7,&local_b0);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8f84;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c8f84:
  uVar7 = *(undefined8 *)(param_1 + 0x150);
  local_b0 = (int *)FUN_002efa1c("<b>Quick Start Guide</b> demonstrates the basics of applying Neat Video to a noisy clip."
                                 ,0x58);
  FUN_0002bbc4(uVar7,&local_b0);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c8fdc;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c8fdc:
  uVar7 = *(undefined8 *)(param_1 + 0x180);
  local_b0 = (int *)FUN_002efa1c("<div style=\"margin-bottom: 4px;\">Currently working in <b><nobr>Beginner Mode</nobr></b>.</div><div>For more control switch to <b><nobr><a href=dummy style=\"color: %1; text-decoration: none\">Advanced Mode</a></nobr></b>.</div>"
                                 ,0xe1);
  FUN_0002bbbc(uVar7,&local_b0);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c9034;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c9034:
  uVar7 = *(undefined8 *)(param_1 + 0x180);
  local_b0 = (int *)FUN_002efa1c("<b>Advanced Mode</b> offers more control over noise filter and allows to give preference to higher quality vs higher speed of processing.<br><br>Switch to Advanced Mode to be able to utilize Neat Video to its full potential."
                                 ,0xe0);
  FUN_0002bbc4(uVar7,&local_b0);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c908c;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c908c:
  uVar7 = *(undefined8 *)(param_1 + 0x178);
  local_b0 = (int *)FUN_002efa1c("<div>How to use: <b><nobr><a href=dummy style=\"color: %1; text-decoration: none\">Quick Start Guide</a></nobr></b>.</div>"
                                 ,0x78);
  FUN_0002bbbc(uVar7,&local_b0);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c90e4;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c90e4:
  uVar7 = *(undefined8 *)(param_1 + 0x178);
  local_b0 = (int *)FUN_002efa1c("<b>Quick Start Guide</b> demonstrates the basics of applying Neat Video to a noisy clip."
                                 ,0x58);
  FUN_0002bbc4(uVar7,&local_b0);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c913c;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c913c:
  uVar7 = *(undefined8 *)(param_1 + 0x1a8);
  local_b0 = (int *)FUN_002efa1c("<div style=\"margin-bottom: 4px;\">Currently working in <b><nobr>Beginner Mode</nobr></b>.</div><div>For more control switch to <b><nobr><a href=dummy style=\"color: %1; text-decoration: none\">Advanced Mode</a></nobr></b>.</div>"
                                 ,0xe1);
  FUN_0002bbbc(uVar7,&local_b0);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c9194;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c9194:
  uVar7 = *(undefined8 *)(param_1 + 0x1a8);
  local_b0 = (int *)FUN_002efa1c("<b>Advanced Mode</b> offers more control over noise filter and allows to give preference to higher quality vs higher speed of processing.<br><br>Switch to Advanced Mode to be able to utilize Neat Video to its full potential."
                                 ,0xe0);
  FUN_0002bbc4(uVar7,&local_b0);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c91ec;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c91ec:
  uVar7 = *(undefined8 *)(param_1 + 0x1a0);
  local_b0 = (int *)FUN_002efa1c("<div>How to use: <b><nobr><a href=dummy style=\"color: %1; text-decoration: none\">Quick Start Guide</a></nobr></b>.</div>"
                                 ,0x78);
  FUN_0002bbbc(uVar7,&local_b0);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c9244;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c9244:
  uVar7 = *(undefined8 *)(param_1 + 0x1a0);
  local_b0 = (int *)FUN_002efa1c("<b>Quick Start Guide</b> demonstrates the basics of applying Neat Video to a noisy clip."
                                 ,0x58);
  FUN_0002bbc4(uVar7,&local_b0);
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar1 = *local_b0;
      *local_b0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c929c;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000c929c:
  local_b0 = &DAT_0142f0a8;
  FUN_0001cacc(uVar3,&local_b0);
  if (local_b0[4] != -1) {
    if (local_b0[4] != 0) {
      LOAcquire();
      iVar1 = local_b0[4];
      local_b0[4] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c92ec;
    }
    FUN_002a9a28(local_b0,FUN_0072a91c);
  }
LAB_000c92ec:
  FUN_00093be0(param_1,uVar3);
  local_b0 = &DAT_0142f0a8;
  FUN_0001cacc(uVar5,&local_b0);
  if (local_b0[4] != -1) {
    if (local_b0[4] != 0) {
      LOAcquire();
      iVar1 = local_b0[4];
      local_b0[4] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c9340;
    }
    FUN_002a9a28(local_b0,FUN_0072a91c);
  }
LAB_000c9340:
  FUN_00093be0(param_1,uVar5);
  local_b0 = &DAT_0142f0a8;
  FUN_0001cacc(uVar6,&local_b0);
  if (local_b0[4] != -1) {
    if (local_b0[4] != 0) {
      LOAcquire();
      iVar1 = local_b0[4];
      local_b0[4] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c9394;
    }
    FUN_002a9a28(local_b0,FUN_0072a91c);
  }
LAB_000c9394:
  FUN_00093be0(param_1,uVar6);
  if (*local_88 != -1) {
    if (*local_88 != 0) {
      LOAcquire();
      iVar1 = *local_88;
      *local_88 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c93d4;
    }
    FUN_00287df8(local_88,2,8);
  }
LAB_000c93d4:
  if (*local_90 != -1) {
    if (*local_90 != 0) {
      LOAcquire();
      iVar1 = *local_90;
      *local_90 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c9408;
    }
    FUN_00287df8(local_90,2,8);
  }
LAB_000c9408:
  if (*local_98 != -1) {
    if (*local_98 != 0) {
      LOAcquire();
      iVar1 = *local_98;
      *local_98 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c943c;
    }
    FUN_00287df8(local_98,2,8);
  }
LAB_000c943c:
  if (*local_a0 != -1) {
    if (*local_a0 != 0) {
      LOAcquire();
      iVar1 = *local_a0;
      *local_a0 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c9470;
    }
    FUN_00287df8(local_a0,2,8);
  }
LAB_000c9470:
  if (*local_a8 != -1) {
    if (*local_a8 != 0) {
      LOAcquire();
      iVar1 = *local_a8;
      *local_a8 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c94a4;
    }
    FUN_00287df8(local_a8,2,8);
  }
LAB_000c94a4:
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar1 = *local_60;
      *local_60 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c94d8;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000c94d8:
  if (*local_68 != -1) {
    if (*local_68 != 0) {
      LOAcquire();
      iVar1 = *local_68;
      *local_68 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c950c;
    }
    FUN_00287df8(local_68,2,8);
  }
LAB_000c950c:
  if (*local_70 != -1) {
    if (*local_70 != 0) {
      LOAcquire();
      iVar1 = *local_70;
      *local_70 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c9540;
    }
    FUN_00287df8(local_70,2,8);
  }
LAB_000c9540:
  if (*local_78 != -1) {
    if (*local_78 != 0) {
      LOAcquire();
      iVar1 = *local_78;
      *local_78 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c9574;
    }
    FUN_00287df8(local_78,2,8);
  }
LAB_000c9574:
  if (*local_80 != -1) {
    if (*local_80 != 0) {
      LOAcquire();
      iVar1 = *local_80;
      *local_80 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000c95a8;
    }
    FUN_00287df8(local_80,2,8);
  }
LAB_000c95a8:
  if (*(long *)PTR____stack_chk_guard_013b4320 == local_58) {
    return param_1;
  }
                    /* WARNING: Subroutine does not return */
  ___stack_chk_fail();
}


