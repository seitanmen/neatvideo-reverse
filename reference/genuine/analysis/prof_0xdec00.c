// func entry: 000dec00 name: FUN_000dec00 size: 2644

long FUN_000dec00(long param_1)

{
  int iVar1;
  long lVar2;
  int iVar3;
  undefined8 *puVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  ulong uVar7;
  long lVar8;
  long *plVar9;
  undefined8 uVar10;
  undefined8 in_stack_ffffffffffffff40;
  undefined4 uVar11;
  undefined1 auStack_88 [8];
  int *local_80;
  int *local_78;
  int *local_70;
  int *local_68;
  int *local_60;
  int *local_58;
  
  uVar11 = (undefined4)((ulong)in_stack_ffffffffffffff40 >> 0x20);
  puVar4 = (undefined8 *)FUN_000d8ec8();
  *puVar4 = &DAT_013c5700;
  puVar4[2] = &DAT_013c5908;
  puVar4[6] = &DAT_013c5958;
  uVar5 = __Znwm(0xa8);
  FUN_000dfe1c(uVar5,0);
  *(undefined8 *)(param_1 + 0xd710) = uVar5;
  uVar5 = *(undefined8 *)(*(long *)(param_1 + 0x130) + 0x50);
  FUN_003bbff0(&local_58,&DAT_013c56c0,"Profile Tuning",0,0xffffffff);
  FUN_00750130(uVar5,&local_58);
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar3 = *local_58;
      *local_58 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000deccc;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000deccc:
  uVar6 = FUN_00850130(uVar5);
  plVar9 = *(long **)(param_1 + 0xd710);
  lVar8 = *(long *)(*(long *)(param_1 + 0xd6c0) + 0x28);
  iVar3 = *(int *)(lVar8 + 0x1c);
  iVar1 = *(int *)(lVar8 + 0x14);
  uVar7 = (**(code **)(*plVar9 + 0x70))(plVar9);
  FUN_0074bf38(plVar9,(iVar3 - iVar1) + 1,uVar7 >> 0x20);
  FUN_0003c9b0(uVar5,*(undefined8 *)(param_1 + 0xd6c0),*(undefined8 *)(param_1 + 0xd710));
  FUN_003bbff0(&local_58,&DAT_013c56c0,"Profile Check and Tuning",0,0xffffffff);
  local_60 = &DAT_0142f030;
  local_68 = &DAT_0142f030;
  FUN_00039518(uVar5,uVar6,param_1 + 0xd6c8,&local_58,&local_60,&local_68,0x19,&DAT_00c94740,
               CONCAT44(uVar11,5));
  if (*local_68 != -1) {
    if (*local_68 != 0) {
      LOAcquire();
      iVar3 = *local_68;
      *local_68 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000dedb4;
    }
    FUN_00287df8(local_68,2,8);
  }
LAB_000dedb4:
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar3 = *local_60;
      *local_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000dede8;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000dede8:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar3 = *local_58;
      *local_58 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000dee1c;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000dee1c:
  uVar10 = *(undefined8 *)(param_1 + 0xd6c8);
  FUN_003bbff0(&local_58,&DAT_013c56c0,"Profile Check",0,0xffffffff);
  local_60 = (int *)FUN_002efa1c("Profile Check helps to find the optimal value of Noise Level using a visually enhanced preview (Assist Image).\n\nClick Begin to start the checking and tuning process.\nAdjust Noise Level while watching the image in preview to find the Noise Level value that completely removes noise yet preserves details."
                                 ,0x12f);
  FUN_00038ca4(uVar5,uVar10,(undefined8 *)(param_1 + 0xd6d0),&local_58,&local_60,0x30,&DAT_00c94740,
               3);
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar3 = *local_60;
      *local_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000deeb4;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000deeb4:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar3 = *local_58;
      *local_58 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000deee8;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000deee8:
  FUN_00017970(*(undefined8 *)(param_1 + 0xd6d0),0);
  uVar10 = *(undefined8 *)(param_1 + 0xd6c8);
  FUN_003bbff0(&local_58,&DAT_013c56c0,"Generic Noise",0,0xffffffff);
  local_60 = (int *)FUN_002efa1c("Strength of noise in this generic profile",0x29);
  FUN_00038ca4(uVar5,uVar10,(undefined8 *)(param_1 + 55000),&local_58,&local_60,0x58,&DAT_00c94740,
               0xfffffffd);
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar3 = *local_60;
      *local_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000def8c;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000def8c:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar3 = *local_58;
      *local_58 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000defc0;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000defc0:
  uVar10 = *(undefined8 *)(param_1 + 55000);
  local_60 = &DAT_0142f030;
  local_58 = &DAT_0142f030;
  local_68 = (int *)FUN_002efa1c("Strength of noise in this generic profile",0x29);
  FUN_003bbff0(&local_70,&DAT_013c56c0,"Generic Noise",0,0xffffffff);
  local_78 = (int *)FUN_002efa1c("Strength of noise in this generic profile",0x29);
  FUN_0003abb8(0x3f800000,0x42480000,uVar5,uVar10,0,&local_58,&local_60,param_1 + 0xd6e0,&local_68,1
               ,0x100000032,5,&DAT_012453b8,&local_70,0,&local_78,0);
  if (*local_78 != -1) {
    if (*local_78 != 0) {
      LOAcquire();
      iVar3 = *local_78;
      *local_78 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df0a8;
    }
    FUN_00287df8(local_78,2,8);
  }
LAB_000df0a8:
  if (*local_70 != -1) {
    if (*local_70 != 0) {
      LOAcquire();
      iVar3 = *local_70;
      *local_70 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df0dc;
    }
    FUN_00287df8(local_70,2,8);
  }
LAB_000df0dc:
  if (*local_68 != -1) {
    if (*local_68 != 0) {
      LOAcquire();
      iVar3 = *local_68;
      *local_68 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df110;
    }
    FUN_00287df8(local_68,2,8);
  }
LAB_000df110:
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar3 = *local_60;
      *local_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df144;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000df144:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar3 = *local_58;
      *local_58 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df178;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000df178:
  uVar10 = *(undefined8 *)(param_1 + 0xd6c8);
  FUN_003bbff0(&local_58,&DAT_013c56c0,"Noise Level",0,0xffffffff);
  local_60 = (int *)FUN_002efa1c("Adjust estimation of noise level in video (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                 ,0xc3);
  local_68 = (int *)((ulong)local_68 & 0xffffffffffffff00);
  FUN_00038ca4(uVar5,uVar10,(undefined8 *)(param_1 + 0xd6f8),&local_58,&local_60,0x59,&local_68,0);
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar3 = *local_60;
      *local_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df210;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000df210:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar3 = *local_58;
      *local_58 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df244;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000df244:
  uVar10 = *(undefined8 *)(param_1 + 0xd6f8);
  local_60 = &DAT_0142f030;
  local_58 = &DAT_0142f030;
  local_68 = (int *)FUN_002efa1c("Adjust overall estimation of noise level (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                 ,0xc2);
  FUN_003bbff0(&local_70,&DAT_013c56c0,"Overall Noise Level",0,0xffffffff);
  local_78 = (int *)FUN_002efa1c("Estimation of overall noise level (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                 ,0xbb);
  FUN_0003abb8(0,0x40200000,uVar5,uVar10,0,&local_58,&local_60,param_1 + 0xd700,&local_68,0xffffff9c
               ,0x500000096,0x14,&DAT_012453bc,&local_70,0,&local_78,0x12);
  if (*local_78 != -1) {
    if (*local_78 != 0) {
      LOAcquire();
      iVar3 = *local_78;
      *local_78 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df328;
    }
    FUN_00287df8(local_78,2,8);
  }
LAB_000df328:
  if (*local_70 != -1) {
    if (*local_70 != 0) {
      LOAcquire();
      iVar3 = *local_70;
      *local_70 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df35c;
    }
    FUN_00287df8(local_70,2,8);
  }
LAB_000df35c:
  if (*local_68 != -1) {
    if (*local_68 != 0) {
      LOAcquire();
      iVar3 = *local_68;
      *local_68 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df390;
    }
    FUN_00287df8(local_68,2,8);
  }
LAB_000df390:
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar3 = *local_60;
      *local_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df3c4;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000df3c4:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar3 = *local_58;
      *local_58 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df3f8;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000df3f8:
  local_58 = &DAT_0142f030;
  FUN_00039a7c(uVar5,uVar6,(undefined8 *)(param_1 + 0xd6e8),&local_58,0,0,0);
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar3 = *local_58;
      *local_58 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df454;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000df454:
  uVar6 = *(undefined8 *)(param_1 + 0xd6e8);
  local_58 = (int *)FUN_002efa1c("Generic noise profile is now in its default state.\n\nPlease manually tune Generic Noise setting\nuntil noise is fully gone in Assist Image\nwhile details are mostly preserved."
                                 ,0xac);
  local_80 = &DAT_0142f030;
  FUN_0003c454(uVar5,uVar6,param_1 + 0xd6f0,&local_58,&local_80,0,0xc);
  if (*local_80 != -1) {
    if (*local_80 != 0) {
      LOAcquire();
      iVar3 = *local_80;
      *local_80 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df4c8;
    }
    FUN_00287df8(local_80,2,8);
  }
LAB_000df4c8:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar3 = *local_58;
      *local_58 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df4fc;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000df4fc:
  FUN_003d0e8c(auStack_88,uVar5,"2scrollAvailabilityMayBeChanged()",param_1,
               "1onScrollAvailabilityMayBeChanged()",0);
  FUN_003d24d8(auStack_88);
  FUN_00038128(uVar5);
  local_58 = &DAT_0142f0a8;
  FUN_0001cacc(uVar5,&local_58);
  if (local_58[4] != -1) {
    if (local_58[4] != 0) {
      LOAcquire();
      iVar3 = local_58[4];
      local_58[4] = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000df580;
    }
    FUN_002a9a28(local_58,FUN_0072a91c);
  }
LAB_000df580:
  FUN_00093be0(param_1,uVar5);
  lVar2 = DAT_0155b2d0;
  lVar8 = DAT_0155b2d0 + 0x388;
  FUN_005a9938(*(long *)(*(long *)(param_1 + 0xd710) + 0xa0) + 0x30,lVar8);
  FUN_005a9938(*(long *)(*(long *)(param_1 + 0xd710) + 0xa0) + 0x40,lVar2 + 0x398);
  FUN_005a9938(*(long *)(*(long *)(param_1 + 0xd710) + 0x88) + 0x30,lVar8);
  FUN_005a9938(*(long *)(*(long *)(param_1 + 0xd710) + 0x88) + 0x40,lVar2 + 0x398);
  plVar9 = *(long **)(*(long *)(param_1 + 0xd710) + 0x98);
  (**(code **)(*plVar9 + 0x68))(plVar9,0);
  plVar9 = *(long **)(*(long *)(param_1 + 0xd710) + 0xa0);
  (**(code **)(*plVar9 + 0x68))(plVar9,0);
  *(undefined8 *)(param_1 + 0xd698) = *(undefined8 *)(*(long *)(param_1 + 0xd710) + 0xa0);
  *(undefined8 *)(param_1 + 0xd6a0) = *(undefined8 *)(*(long *)(param_1 + 0xd710) + 0x88);
  return param_1;
}


