// func entry: 000e0b18 name: FUN_000e0b18 size: 10264

long FUN_000e0b18(long param_1)

{
  bool bVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  undefined8 *puVar5;
  undefined8 uVar6;
  undefined8 uVar7;
  int *piVar8;
  long *plVar9;
  uint *puVar10;
  uint uVar11;
  long lVar12;
  uint uVar13;
  uint uVar14;
  uint uVar15;
  long lVar16;
  uint uVar17;
  uint uVar18;
  uint uVar19;
  uint *puVar20;
  undefined8 uVar21;
  undefined8 in_stack_fffffffffffffe20;
  undefined4 uVar22;
  undefined8 in_stack_fffffffffffffe28;
  undefined4 uVar24;
  undefined8 uVar23;
  undefined8 in_stack_fffffffffffffe50;
  uint local_178;
  uint local_174;
  undefined1 auStack_158 [8];
  int *local_150;
  int *local_148;
  int *local_140;
  int *local_138;
  int *local_130;
  uint *local_128;
  undefined1 auStack_120 [8];
  undefined1 auStack_118 [8];
  undefined1 auStack_110 [8];
  undefined1 auStack_108 [8];
  uint *local_100 [2];
  undefined4 local_f0;
  undefined2 uStack_ec;
  undefined2 uStack_ea;
  undefined6 uStack_e8;
  undefined4 local_e0;
  undefined2 local_dc;
  undefined8 uStack_da;
  undefined4 local_d0;
  undefined2 local_cc;
  undefined6 uStack_ca;
  undefined2 uStack_c4;
  undefined4 local_c0;
  undefined2 local_bc;
  undefined6 uStack_ba;
  undefined2 uStack_b4;
  undefined2 local_b0;
  undefined1 auStack_a8 [16];
  undefined1 auStack_98 [16];
  undefined1 auStack_88 [16];
  undefined1 auStack_78 [8];
  undefined1 auStack_70 [16];
  
  puVar5 = (undefined8 *)FUN_000d8ec8();
  *puVar5 = &DAT_013c59c8;
  puVar5[2] = &DAT_013c5bd8;
  puVar5[6] = &DAT_013c5c28;
  puVar5 = puVar5 + 0x1ae2;
  FUN_003d93d8(puVar5,0);
  uVar6 = __Znwm(0x140);
  FUN_000e5ce4(uVar6,0);
  *(undefined8 *)(param_1 + 0xd750) = uVar6;
  uVar6 = __Znwm(0x170);
  FUN_000e820c(uVar6,0);
  *(undefined8 *)(param_1 + 0xd740) = uVar6;
  lVar12 = *(long *)(param_1 + 0xd750);
  *(undefined8 *)(param_1 + 0xd720) = *(undefined8 *)(lVar12 + 0xf0);
  *(undefined8 *)(param_1 + 0xd728) = *(undefined8 *)(lVar12 + 0xf8);
  *(undefined8 *)(param_1 + 0xd730) = *(undefined8 *)(lVar12 + 0x100);
  *(undefined8 *)(param_1 + 0xd738) = *(undefined8 *)(lVar12 + 0x108);
  uVar6 = __Znwm(0x10);
  FUN_0071e5c4(uVar6,param_1);
  FUN_0071ea44(uVar6,1);
  lVar12 = 0;
  lVar16 = param_1 + 0xd720;
  do {
    uVar7 = __Znwm(0x10);
    FUN_0071bd1c(uVar7,*(undefined8 *)(lVar16 + lVar12 * 8));
    FUN_007d5fc8(&local_f0,*(undefined8 *)(lVar16 + lVar12 * 8));
    FUN_0071c9c8(uVar7,&local_f0);
    FUN_004bbe60(&local_f0);
    FUN_0071cfb0(uVar7,1);
    if (lVar12 == 0) {
      FUN_0071cff8(uVar7,1);
    }
    FUN_00756f4c(&local_f0,*(undefined8 *)(lVar16 + lVar12 * 8));
    FUN_0071cd08(uVar7,&local_f0);
    piVar8 = (int *)CONCAT26(uStack_ea,CONCAT24(uStack_ec,local_f0));
    if (*piVar8 != -1) {
      if (*piVar8 != 0) {
        LOAcquire();
        iVar2 = *piVar8;
        *piVar8 = iVar2 + -1;
        LORelease();
        if (iVar2 != 1) goto LAB_000e0cc4;
        piVar8 = (int *)CONCAT26(uStack_ea,CONCAT24(uStack_ec,local_f0));
      }
      FUN_00287df8(piVar8,2,8);
    }
LAB_000e0cc4:
    FUN_00886284(*(undefined8 *)(lVar16 + lVar12 * 8),uVar7);
    FUN_0071e668(uVar6,uVar7);
    FUN_003d9480(puVar5,uVar7,(int)lVar12 + -1);
    FUN_003d0e8c(auStack_70,uVar7,"2triggered(bool)",puVar5,"1map()",0);
    FUN_003d24d8(auStack_70);
    lVar12 = lVar12 + 1;
  } while (lVar12 != 4);
  FUN_003d0e8c(auStack_78,puVar5,"2mapped(int)",param_1,"1setEqPrimarySeries(int)",0);
  FUN_003d24d8(auStack_78);
  uVar3 = *(uint *)(DAT_0141be88 + 0x18ba770);
  FUN_005a5fa0(auStack_88,0xffc00000);
  FUN_005a5fa0(auStack_98,0xff00c000);
  FUN_005a5fa0(auStack_a8,0xff0000c0);
  uVar6 = *(undefined8 *)(*(long *)(param_1 + 0xd750) + 0x90);
  FUN_000208fc(uVar6);
  FUN_0002169c(0xc059000000000000,uVar6);
  FUN_000218d0(0x4069000000000000,uVar6);
  FUN_00021b04(uVar6,0);
  FUN_00021d24(0x4014000000000000,uVar6);
  FUN_00020b28(uVar6,3);
  FUN_00021234(uVar6,9);
  FUN_000e3848(param_1);
  uVar4 = (uVar3 ^ uVar3 >> 1) & 0x41440144;
  if (uVar3 != (uVar4 | uVar4 << 1)) {
    uVar4 = *(uint *)(DAT_0141be98 + 0xb278d8);
    if (DAT_0141be90 != 0) {
      uVar4 = uVar4 >> 8 | uVar4 << 0x18;
    }
    DAT_0141be90 = (int)(CONCAT44((uVar4 + (uVar4 >> 0x10)) * 0x101 + (uVar4 >> 0x18),uVar4) >> 0x10
                        );
    if (DAT_0141be90 != 1) {
      *(uint *)(DAT_0141bea0 + 0x303a28) = uVar4;
    }
  }
  iVar2 = 0;
  local_f0 = 0;
  uStack_ec = 0xffff;
  uStack_ea = 0;
  uStack_e8 = 0;
  local_e0 = 0;
  local_dc = 0xffff;
  uStack_da = 0;
  local_d0 = 0;
  local_cc = 0xffff;
  puVar5 = (undefined8 *)((ulong)local_100 | 4);
  local_174 = 0xff008000;
  uStack_ca = 0;
  uStack_c4 = 0;
  local_178 = 0xff00a000;
  local_c0 = 0;
  local_bc = 0xffff;
  uStack_ba = 0;
  uStack_b4 = 0;
  local_b0 = 0x101;
  do {
    bVar1 = iVar2 != 1;
    uVar4 = local_174;
    if (bVar1) {
      uVar4 = 0xff000000;
    }
    uVar3 = 0xff00c000;
    if (bVar1) {
      uVar3 = local_178;
    }
    uVar13 = 0xff00ff00;
    if (bVar1) {
      uVar13 = 0xff000000;
    }
    uVar14 = 0xc0;
    if (iVar2 != 2) {
      uVar14 = 0;
    }
    uVar19 = 0xff;
    uVar17 = uVar19;
    if (iVar2 != 2) {
      uVar17 = 0xa0;
      uVar19 = 0;
    }
    uVar18 = 0xc00000;
    if (iVar2 != 0) {
      uVar18 = 0;
    }
    uVar15 = 0xff0000;
    uVar11 = uVar15;
    if (iVar2 != 0) {
      uVar11 = 0xa00000;
      uVar15 = 0;
    }
    FUN_005a5fa0(local_100,uVar4 | uVar18 | uVar14);
    local_f0 = local_100[0]._0_4_;
    uVar7 = *puVar5;
    *(undefined2 *)((undefined8 *)((ulong)&local_f0 | 4) + 1) = *(undefined2 *)(puVar5 + 1);
    *(undefined8 *)((ulong)&local_f0 | 4) = uVar7;
    FUN_005a5fa0(local_100,uVar3 | uVar11 | uVar17);
    local_e0 = local_100[0]._0_4_;
    FUN_005a5fa0(local_100,0xff000000);
    local_d0 = local_100[0]._0_4_;
    uStack_c4 = *(undefined2 *)(puVar5 + 1);
    local_cc = (undefined2)*puVar5;
    uStack_ca = (undefined6)((ulong)*puVar5 >> 0x10);
    FUN_005a5fa0(local_100,uVar13 | uVar15 | uVar19);
    local_c0 = local_100[0]._0_4_;
    uStack_b4 = *(undefined2 *)(puVar5 + 1);
    local_bc = (undefined2)*puVar5;
    uStack_ba = (undefined6)((ulong)*puVar5 >> 0x10);
    FUN_00021158(uVar6,iVar2,&local_f0);
    uVar24 = (undefined4)((ulong)in_stack_fffffffffffffe28 >> 0x20);
    uVar4 = (uint)((ulong)in_stack_fffffffffffffe50 >> 0x20);
    uVar22 = (undefined4)((ulong)in_stack_fffffffffffffe20 >> 0x20);
    iVar2 = iVar2 + 1;
  } while (iVar2 != 3);
  FUN_0002090c(uVar6);
  FUN_003d0e8c(auStack_108,uVar6,"2valueChangeBegin(int, int)",param_1,"1onEqValueChangeBegin()",0);
  FUN_003d24d8(auStack_108);
  FUN_003d0e8c(auStack_110,uVar6,"2valueChanged(int, int, double, double)",param_1,
               "1onEqValueChanged(int, int, double, double)",0);
  FUN_003d24d8(auStack_110);
  FUN_003d0e8c(auStack_118,uVar6,"2valueChangeEnd(int, int)",param_1,"1onEqValueChangeEnd()",0);
  FUN_003d24d8(auStack_118);
  FUN_003d0e8c(auStack_120,uVar6,"2seriesGraphClicked(int)",param_1,"1setEqPrimarySeries(int)",0);
  FUN_003d24d8(auStack_120);
  lVar16 = DAT_0155b2d0;
  lVar12 = DAT_0155b2d0 + 0x388;
  FUN_005a9938(*(long *)(*(long *)(param_1 + 0xd740) + 0xa8) + 0x30,lVar12);
  lVar16 = lVar16 + 0x398;
  FUN_005a9938(*(long *)(*(long *)(param_1 + 0xd740) + 0xa8) + 0x40,lVar16);
  FUN_005a9938(*(long *)(*(long *)(param_1 + 0xd740) + 0x90) + 0x30,lVar12);
  FUN_005a9938(*(long *)(*(long *)(param_1 + 0xd740) + 0x90) + 0x40,lVar16);
  FUN_005a9938(*(long *)(*(long *)(param_1 + 0xd750) + 0x118) + 0x30,lVar12);
  FUN_005a9938(*(long *)(*(long *)(param_1 + 0xd750) + 0x118) + 0x40,lVar16);
  uVar6 = *(undefined8 *)(*(long *)(param_1 + 0xd740) + 0xa0);
  local_100[0] = (uint *)FUN_002efa1c(" ",1);
  FUN_00809dc0(uVar6,local_100);
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar3 = *local_100[0];
      *local_100[0] = uVar3 - 1;
      LORelease();
      if (uVar3 != 1) goto LAB_000e11b0;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e11b0:
  plVar9 = *(long **)(*(long *)(param_1 + 0xd740) + 0xa8);
  (**(code **)(*plVar9 + 0x68))(plVar9,0);
  lVar12 = *(long *)(param_1 + 0xd740);
  *(undefined8 *)(param_1 + 0xd698) = *(undefined8 *)(lVar12 + 0xa8);
  *(undefined8 *)(param_1 + 0xd6a0) = *(undefined8 *)(lVar12 + 0x90);
  uVar6 = *(undefined8 *)(lVar12 + 0xd0);
  local_100[0] = (uint *)FUN_002efa1c("-",1);
  FUN_00809dc0(uVar6,local_100);
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar3 = *local_100[0];
      *local_100[0] = uVar3 - 1;
      LORelease();
      if (uVar3 != 1) goto LAB_000e1234;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e1234:
  uVar6 = *(undefined8 *)(*(long *)(param_1 + 0x130) + 0x50);
  FUN_003bbff0(local_100,&DAT_013c5988,"Profile Tuning",0,0xffffffff);
  FUN_00750130(uVar6,local_100);
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar3 = *local_100[0];
      *local_100[0] = uVar3 - 1;
      LORelease();
      if (uVar3 != 1) goto LAB_000e129c;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e129c:
  uVar7 = FUN_00850130(uVar6);
  lVar12 = *(long *)(*(long *)(param_1 + 0xd6c0) + 0x28);
  FUN_0074bf38(*(undefined8 *)(param_1 + 0xd740),
               (*(int *)(lVar12 + 0x1c) - *(int *)(lVar12 + 0x14)) + 1,0x67);
  FUN_0003c9b0(uVar6,*(undefined8 *)(param_1 + 0xd6c0),*(undefined8 *)(param_1 + 0xd740));
  local_100[0] = &DAT_0142f030;
  FUN_00039a7c(uVar6,uVar7,param_1 + 0xd748,local_100,0,0,0);
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar3 = *local_100[0];
      *local_100[0] = uVar3 - 1;
      LORelease();
      if (uVar3 != 1) goto LAB_000e133c;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e133c:
  lVar12 = *(long *)(*(long *)(param_1 + 0xd748) + 0x28);
  FUN_0074bf38(*(undefined8 *)(param_1 + 0xd750),
               (*(int *)(lVar12 + 0x1c) - *(int *)(lVar12 + 0x14)) + 1,0xe6);
  FUN_0003c9b0(uVar6,*(undefined8 *)(param_1 + 0xd748),*(undefined8 *)(param_1 + 0xd750));
  FUN_003bbff0(local_100,&DAT_013c5988,"Profile Check and Tuning",0,0xffffffff);
  local_130 = (int *)&DAT_0142f030;
  local_128 = &DAT_0142f030;
  FUN_00039518(uVar6,uVar7,param_1 + 0xd6c8,local_100,&local_128,&local_130,0x19,&DAT_00c947f0,
               CONCAT44(uVar22,2));
  if (*local_130 != -1) {
    if (*local_130 != 0) {
      LOAcquire();
      iVar2 = *local_130;
      *local_130 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e13f0;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000e13f0:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar3 = *local_128;
      *local_128 = uVar3 - 1;
      LORelease();
      if (uVar3 != 1) goto LAB_000e1424;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000e1424:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar3 = *local_100[0];
      *local_100[0] = uVar3 - 1;
      LORelease();
      if (uVar3 != 1) goto LAB_000e1458;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e1458:
  uVar21 = *(undefined8 *)(param_1 + 0xd6c8);
  FUN_003bbff0(local_100,&DAT_013c5988,"Profile Check",0,0xffffffff);
  local_128 = (uint *)FUN_002efa1c("Profile Check helps to find the optimal value of Noise Level using a visually enhanced preview (Assist Image).\n\nClick Begin to start the checking and tuning process.\nAdjust Noise Level while watching the image in preview to find the Noise Level value that completely removes noise yet preserves details."
                                   ,0x12f);
  FUN_00038ca4(uVar6,uVar21,(undefined8 *)(param_1 + 0xd6d0),local_100,&local_128,0x30,&DAT_00c947f0
               ,3);
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar3 = *local_128;
      *local_128 = uVar3 - 1;
      LORelease();
      if (uVar3 != 1) goto LAB_000e14f0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000e14f0:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar3 = *local_100[0];
      *local_100[0] = uVar3 - 1;
      LORelease();
      if (uVar3 != 1) goto LAB_000e1524;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e1524:
  FUN_00017970(*(undefined8 *)(param_1 + 0xd6d0),0);
  uVar21 = *(undefined8 *)(param_1 + 0xd6c8);
  FUN_003bbff0(local_100,&DAT_013c5988,"Generic Noise",0,0xffffffff);
  local_128 = (uint *)FUN_002efa1c("Strength of noise in this generic profile",0x29);
  FUN_00038ca4(uVar6,uVar21,(undefined8 *)(param_1 + 55000),local_100,&local_128,0x58,&DAT_00c947f0,
               0xfffffffd);
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar3 = *local_128;
      *local_128 = uVar3 - 1;
      LORelease();
      if (uVar3 != 1) goto LAB_000e15c8;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000e15c8:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar3 = *local_100[0];
      *local_100[0] = uVar3 - 1;
      LORelease();
      if (uVar3 != 1) goto LAB_000e15fc;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e15fc:
  uVar21 = *(undefined8 *)(param_1 + 55000);
  local_100[0] = &DAT_0142f030;
  local_128 = &DAT_0142f030;
  local_130 = (int *)FUN_002efa1c("Strength of noise in this generic profile",0x29);
  FUN_003bbff0(&local_138,&DAT_013c5988,"Generic Noise",0,0xffffffff);
  local_140 = (int *)FUN_002efa1c("Strength of noise in this generic profile",0x29);
  lVar12 = (ulong)uVar4 << 0x20;
  uVar23 = CONCAT44(uVar24,5);
  FUN_0003abb8(0x3f800000,0x42480000,uVar6,uVar21,0,local_100,&local_128,param_1 + 0xd6e0,&local_130
               ,1,0x100000032,uVar23,&DAT_012453b8,&local_138,0,&local_140,lVar12);
  uVar22 = (undefined4)((ulong)uVar23 >> 0x20);
  uVar24 = (undefined4)((ulong)lVar12 >> 0x20);
  if (*local_140 != -1) {
    if (*local_140 != 0) {
      LOAcquire();
      iVar2 = *local_140;
      *local_140 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e16e8;
    }
    FUN_00287df8(local_140,2,8);
  }
LAB_000e16e8:
  if (*local_138 != -1) {
    if (*local_138 != 0) {
      LOAcquire();
      iVar2 = *local_138;
      *local_138 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e171c;
    }
    FUN_00287df8(local_138,2,8);
  }
LAB_000e171c:
  if (*local_130 != -1) {
    if (*local_130 != 0) {
      LOAcquire();
      iVar2 = *local_130;
      *local_130 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e1750;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000e1750:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar4 = *local_128;
      *local_128 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e1784;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000e1784:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar4 = *local_100[0];
      *local_100[0] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e17b8;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e17b8:
  uVar21 = *(undefined8 *)(param_1 + 0xd6c8);
  FUN_003bbff0(local_100,&DAT_013c5988,"Noise Level",0,0xffffffff);
  local_128 = (uint *)FUN_002efa1c("Adjust estimation of noise level in video (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                   ,0xc3);
  puVar5 = (undefined8 *)(param_1 + 0xd6f8);
  local_130 = (int *)((ulong)local_130 & 0xffffffffffffff00);
  FUN_00038ca4(uVar6,uVar21,puVar5,local_100,&local_128,0x5b,&local_130,0);
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar4 = *local_128;
      *local_128 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e1850;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000e1850:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar4 = *local_100[0];
      *local_100[0] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e1884;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e1884:
  uVar21 = *puVar5;
  local_100[0] = &DAT_0142f030;
  local_128 = &DAT_0142f030;
  local_130 = (int *)FUN_002efa1c("Adjust overall estimation of noise level (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xc2);
  FUN_003bbff0(&local_138,&DAT_013c5988,"Overall Noise Level",0,0xffffffff);
  local_140 = (int *)FUN_002efa1c("Estimation of overall noise level (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xbb);
  FUN_0003abb8(0,0x40200000,uVar6,uVar21,0,local_100,&local_128,param_1 + 0xd700,&local_130,
               0xffffff9c,0x500000096,CONCAT44(uVar22,0x14),&DAT_012453bc,&local_138,
               param_1 + 0xd708,&local_140,CONCAT44(uVar24,0x12));
  if (*local_140 != -1) {
    if (*local_140 != 0) {
      LOAcquire();
      iVar2 = *local_140;
      *local_140 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e1974;
    }
    FUN_00287df8(local_140,2,8);
  }
LAB_000e1974:
  if (*local_138 != -1) {
    if (*local_138 != 0) {
      LOAcquire();
      iVar2 = *local_138;
      *local_138 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e19a8;
    }
    FUN_00287df8(local_138,2,8);
  }
LAB_000e19a8:
  if (*local_130 != -1) {
    if (*local_130 != 0) {
      LOAcquire();
      iVar2 = *local_130;
      *local_130 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e19dc;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000e19dc:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar4 = *local_128;
      *local_128 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e1a10;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000e1a10:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar4 = *local_100[0];
      *local_100[0] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e1a44;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e1a44:
  uVar21 = *puVar5;
  FUN_003bbff0(local_100,&DAT_013c5988,"Luminance",0,0xffffffff);
  local_128 = (uint *)FUN_002efa1c("ChannelY.png",0xc);
  local_130 = (int *)FUN_002efa1c("Estimation of noise level in Luminance channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd2);
  local_138 = (int *)FUN_002efa1c("Adjust estimation of noise level in Luminance channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd9);
  FUN_003bbff0(&local_140,&DAT_013c5988,"Luminance Noise Level",0,0xffffffff);
  local_148 = (int *)FUN_002efa1c("Estimation of noise level in Luminance channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd2);
  FUN_0003aa60(0,0x40200000,uVar6,uVar21,0,local_100,(undefined8 *)(param_1 + 0xd770),&local_128,
               &local_130,param_1 + 0xd758,&local_138,0x96ffffff9c,0x1400000005,&DAT_012453d4,
               &local_140,0,&local_148,0x5a,0);
  if (*local_148 != -1) {
    if (*local_148 != 0) {
      LOAcquire();
      iVar2 = *local_148;
      *local_148 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e1b7c;
    }
    FUN_00287df8(local_148,2,8);
  }
LAB_000e1b7c:
  if (*local_140 != -1) {
    if (*local_140 != 0) {
      LOAcquire();
      iVar2 = *local_140;
      *local_140 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e1bb0;
    }
    FUN_00287df8(local_140,2,8);
  }
LAB_000e1bb0:
  if (*local_138 != -1) {
    if (*local_138 != 0) {
      LOAcquire();
      iVar2 = *local_138;
      *local_138 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e1be4;
    }
    FUN_00287df8(local_138,2,8);
  }
LAB_000e1be4:
  if (*local_130 != -1) {
    if (*local_130 != 0) {
      LOAcquire();
      iVar2 = *local_130;
      *local_130 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e1c18;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000e1c18:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar4 = *local_128;
      *local_128 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e1c4c;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000e1c4c:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar4 = *local_100[0];
      *local_100[0] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e1c80;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e1c80:
  uVar21 = *puVar5;
  FUN_003bbff0(local_100,&DAT_013c5988,"Cr",0,0xffffffff);
  local_128 = (uint *)FUN_002efa1c("ChannelCr.png",0xd);
  local_130 = (int *)FUN_002efa1c("Estimation of noise level in Cr channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xcb);
  local_138 = (int *)FUN_002efa1c("Adjust estimation of noise level in Cr channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd2);
  FUN_003bbff0(&local_140,&DAT_013c5988,"Cr Noise Level",0,0xffffffff);
  local_148 = (int *)FUN_002efa1c("Estimation of noise level in Cr channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xcb);
  FUN_0003aa60(0,0x40200000,uVar6,uVar21,param_1 + 0xd788,local_100,(undefined8 *)(param_1 + 0xd778)
               ,&local_128,&local_130,param_1 + 0xd760,&local_138,0x96ffffff9c,0x1400000005,
               &DAT_012453d8,&local_140,param_1 + 0xd798,&local_148,0x5a,0);
  if (*local_148 != -1) {
    if (*local_148 != 0) {
      LOAcquire();
      iVar2 = *local_148;
      *local_148 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e1dc4;
    }
    FUN_00287df8(local_148,2,8);
  }
LAB_000e1dc4:
  if (*local_140 != -1) {
    if (*local_140 != 0) {
      LOAcquire();
      iVar2 = *local_140;
      *local_140 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e1df8;
    }
    FUN_00287df8(local_140,2,8);
  }
LAB_000e1df8:
  if (*local_138 != -1) {
    if (*local_138 != 0) {
      LOAcquire();
      iVar2 = *local_138;
      *local_138 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e1e2c;
    }
    FUN_00287df8(local_138,2,8);
  }
LAB_000e1e2c:
  if (*local_130 != -1) {
    if (*local_130 != 0) {
      LOAcquire();
      iVar2 = *local_130;
      *local_130 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e1e60;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000e1e60:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar4 = *local_128;
      *local_128 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e1e94;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000e1e94:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar4 = *local_100[0];
      *local_100[0] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e1ec8;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e1ec8:
  uVar21 = *puVar5;
  FUN_003bbff0(local_100,&DAT_013c5988,"Cb",0,0xffffffff);
  local_128 = (uint *)FUN_002efa1c("ChannelCb.png",0xd);
  local_130 = (int *)FUN_002efa1c("Estimation of noise level in Cb channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xcb);
  local_138 = (int *)FUN_002efa1c("Adjust estimation of noise level in Cb channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd2);
  FUN_003bbff0(&local_140,&DAT_013c5988,"Cb Noise Level",0,0xffffffff);
  local_148 = (int *)FUN_002efa1c("Estimation of noise level in Cb channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xcb);
  FUN_0003aa60(0,0x40200000,uVar6,uVar21,param_1 + 0xd790,local_100,(undefined8 *)(param_1 + 0xd780)
               ,&local_128,&local_130,param_1 + 0xd768,&local_138,0x96ffffff9c,0x1400000005,
               0x12453dc,&local_140,param_1 + 0xd7a0,&local_148,0x5a,0);
  if (*local_148 != -1) {
    if (*local_148 != 0) {
      LOAcquire();
      iVar2 = *local_148;
      *local_148 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e200c;
    }
    FUN_00287df8(local_148,2,8);
  }
LAB_000e200c:
  if (*local_140 != -1) {
    if (*local_140 != 0) {
      LOAcquire();
      iVar2 = *local_140;
      *local_140 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2040;
    }
    FUN_00287df8(local_140,2,8);
  }
LAB_000e2040:
  if (*local_138 != -1) {
    if (*local_138 != 0) {
      LOAcquire();
      iVar2 = *local_138;
      *local_138 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2074;
    }
    FUN_00287df8(local_138,2,8);
  }
LAB_000e2074:
  if (*local_130 != -1) {
    if (*local_130 != 0) {
      LOAcquire();
      iVar2 = *local_130;
      *local_130 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e20a8;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000e20a8:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar4 = *local_128;
      *local_128 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e20dc;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000e20dc:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar4 = *local_100[0];
      *local_100[0] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e2110;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e2110:
  uVar21 = *(undefined8 *)(param_1 + 0xd6f8);
  FUN_003bbff0(local_100,&DAT_013c5988,"Frequencies",0,0xffffffff);
  local_128 = (uint *)FUN_002efa1c("Adjust estimations of noise levels in frequency components (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                   ,0xd4);
  puVar5 = (undefined8 *)(param_1 + 0xd7a8);
  local_130 = (int *)((ulong)local_130 & 0xffffffffffffff00);
  FUN_00038ca4(uVar6,uVar21,puVar5,local_100,&local_128,0x5b,&local_130,0);
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar4 = *local_128;
      *local_128 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e21b0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000e21b0:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar4 = *local_100[0];
      *local_100[0] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e21e4;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e21e4:
  uVar21 = *puVar5;
  FUN_003bbff0(local_100,&DAT_013c5988,"High",0,0xffffffff);
  local_128 = (uint *)FUN_002efa1c("HFImage.png",0xb);
  local_130 = (int *)FUN_002efa1c("Example of high-frequency noise / fine grain",0x2c);
  local_138 = (int *)FUN_002efa1c("Adjust estimation of noise level in high-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd6);
  FUN_003bbff0(&local_140,&DAT_013c5988,"High Freq Noise Level",0,0xffffffff);
  local_148 = (int *)FUN_002efa1c("Estimation of noise level in high-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xcf);
  FUN_0003aa60(0,0x40200000,uVar6,uVar21,0,local_100,param_1 + 0xd7d8,&local_128,&local_130,
               param_1 + 0xd7b0,&local_138,0x96ffffff9c,0x1400000005,&DAT_012453c0,&local_140,0,
               &local_148,0x5a,0);
  if (*local_148 != -1) {
    if (*local_148 != 0) {
      LOAcquire();
      iVar2 = *local_148;
      *local_148 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2318;
    }
    FUN_00287df8(local_148,2,8);
  }
LAB_000e2318:
  if (*local_140 != -1) {
    if (*local_140 != 0) {
      LOAcquire();
      iVar2 = *local_140;
      *local_140 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e234c;
    }
    FUN_00287df8(local_140,2,8);
  }
LAB_000e234c:
  if (*local_138 != -1) {
    if (*local_138 != 0) {
      LOAcquire();
      iVar2 = *local_138;
      *local_138 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2380;
    }
    FUN_00287df8(local_138,2,8);
  }
LAB_000e2380:
  if (*local_130 != -1) {
    if (*local_130 != 0) {
      LOAcquire();
      iVar2 = *local_130;
      *local_130 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e23b4;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000e23b4:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar4 = *local_128;
      *local_128 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e23e8;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000e23e8:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar4 = *local_100[0];
      *local_100[0] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e241c;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e241c:
  uVar21 = *puVar5;
  FUN_003bbff0(local_100,&DAT_013c5988,"Mid",0,0xffffffff);
  local_128 = (uint *)FUN_002efa1c("MFImage.png",0xb);
  local_130 = (int *)FUN_002efa1c("Example of medium-frequency noise / medium-size grain",0x35);
  local_138 = (int *)FUN_002efa1c("Adjust estimation of noise level in medium-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd8);
  FUN_003bbff0(&local_140,&DAT_013c5988,"Mid Freq Noise Level",0,0xffffffff);
  local_148 = (int *)FUN_002efa1c("Estimation of noise level in medium-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd1);
  FUN_0003aa60(0,0x40200000,uVar6,uVar21,0,local_100,(undefined8 *)(param_1 + 0xd7e0),&local_128,
               &local_130,param_1 + 0xd7b8,&local_138,0x96ffffff9c,0x1400000005,&DAT_012453c4,
               &local_140,0,&local_148,0x5a,0);
  if (*local_148 != -1) {
    if (*local_148 != 0) {
      LOAcquire();
      iVar2 = *local_148;
      *local_148 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e255c;
    }
    FUN_00287df8(local_148,2,8);
  }
LAB_000e255c:
  if (*local_140 != -1) {
    if (*local_140 != 0) {
      LOAcquire();
      iVar2 = *local_140;
      *local_140 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2590;
    }
    FUN_00287df8(local_140,2,8);
  }
LAB_000e2590:
  if (*local_138 != -1) {
    if (*local_138 != 0) {
      LOAcquire();
      iVar2 = *local_138;
      *local_138 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e25c4;
    }
    FUN_00287df8(local_138,2,8);
  }
LAB_000e25c4:
  if (*local_130 != -1) {
    if (*local_130 != 0) {
      LOAcquire();
      iVar2 = *local_130;
      *local_130 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e25f8;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000e25f8:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar4 = *local_128;
      *local_128 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e262c;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000e262c:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar4 = *local_100[0];
      *local_100[0] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e2660;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e2660:
  uVar21 = *puVar5;
  FUN_003bbff0(local_100,&DAT_013c5988,"Low",0,0xffffffff);
  local_128 = (uint *)FUN_002efa1c("LFImage.png",0xb);
  local_130 = (int *)FUN_002efa1c("Example of low-frequency noise / large-size grain",0x31);
  local_138 = (int *)FUN_002efa1c("Adjust estimation of noise level in low-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd5);
  FUN_003bbff0(&local_140,&DAT_013c5988,"Low Freq Noise Level",0,0xffffffff);
  local_148 = (int *)FUN_002efa1c("Estimation of noise level in low-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xce);
  FUN_0003aa60(0,0x40200000,uVar6,uVar21,0,local_100,(undefined8 *)(param_1 + 0xd7e8),&local_128,
               &local_130,param_1 + 0xd7c0,&local_138,0x96ffffff9c,0x1400000005,&DAT_012453c8,
               &local_140,0,&local_148,0x5a,0);
  if (*local_148 != -1) {
    if (*local_148 != 0) {
      LOAcquire();
      iVar2 = *local_148;
      *local_148 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2798;
    }
    FUN_00287df8(local_148,2,8);
  }
LAB_000e2798:
  if (*local_140 != -1) {
    if (*local_140 != 0) {
      LOAcquire();
      iVar2 = *local_140;
      *local_140 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e27cc;
    }
    FUN_00287df8(local_140,2,8);
  }
LAB_000e27cc:
  if (*local_138 != -1) {
    if (*local_138 != 0) {
      LOAcquire();
      iVar2 = *local_138;
      *local_138 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2800;
    }
    FUN_00287df8(local_138,2,8);
  }
LAB_000e2800:
  if (*local_130 != -1) {
    if (*local_130 != 0) {
      LOAcquire();
      iVar2 = *local_130;
      *local_130 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2834;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000e2834:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar4 = *local_128;
      *local_128 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e2868;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000e2868:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar4 = *local_100[0];
      *local_100[0] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e289c;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e289c:
  uVar21 = *puVar5;
  FUN_003bbff0(local_100,&DAT_013c5988,"Very Low",0,0xffffffff);
  local_128 = (uint *)FUN_002efa1c("VLFImage.png",0xc);
  local_130 = (int *)FUN_002efa1c("Example of very low-frequency noise / very large-size grain",0x3b
                                 );
  local_138 = (int *)FUN_002efa1c("Adjust estimation of noise level in very low-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xda);
  FUN_003bbff0(&local_140,&DAT_013c5988,"Very Low Freq Noise Level",0,0xffffffff);
  local_148 = (int *)FUN_002efa1c("Estimation of noise level in very low-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd3);
  FUN_0003aa60(0,0x40200000,uVar6,uVar21,0,local_100,0,&local_128,&local_130,param_1 + 0xd7c8,
               &local_138,0x96ffffff9c,0x1400000005,&DAT_012453cc,&local_140,0,&local_148,0x5a,0);
  if (*local_148 != -1) {
    if (*local_148 != 0) {
      LOAcquire();
      iVar2 = *local_148;
      *local_148 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e29d0;
    }
    FUN_00287df8(local_148,2,8);
  }
LAB_000e29d0:
  if (*local_140 != -1) {
    if (*local_140 != 0) {
      LOAcquire();
      iVar2 = *local_140;
      *local_140 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2a04;
    }
    FUN_00287df8(local_140,2,8);
  }
LAB_000e2a04:
  if (*local_138 != -1) {
    if (*local_138 != 0) {
      LOAcquire();
      iVar2 = *local_138;
      *local_138 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2a38;
    }
    FUN_00287df8(local_138,2,8);
  }
LAB_000e2a38:
  if (*local_130 != -1) {
    if (*local_130 != 0) {
      LOAcquire();
      iVar2 = *local_130;
      *local_130 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2a6c;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000e2a6c:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar4 = *local_128;
      *local_128 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e2aa0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000e2aa0:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar4 = *local_100[0];
      *local_100[0] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e2ad4;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e2ad4:
  uVar21 = *puVar5;
  FUN_003bbff0(local_100,&DAT_013c5988,"Ultra Low",0,0xffffffff);
  local_128 = (uint *)FUN_002efa1c("ULFImage.png",0xc);
  local_130 = (int *)FUN_002efa1c("Example of ultra low-frequency noise / ultra large-size grain",
                                  0x3d);
  local_138 = (int *)FUN_002efa1c("Adjust estimation of noise level in ultra low-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xdb);
  FUN_003bbff0(&local_140,&DAT_013c5988,"Ultra Low Freq Noise Level",0,0xffffffff);
  local_148 = (int *)FUN_002efa1c("Estimation of noise level in ultra low-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd4);
  FUN_0003aa60(0,0x40200000,uVar6,uVar21,0,local_100,0,&local_128,&local_130,param_1 + 0xd7d0,
               &local_138,0x96ffffff9c,0x1400000005,&DAT_012453d0,&local_140,0,&local_148,0x5a,0);
  if (*local_148 != -1) {
    if (*local_148 != 0) {
      LOAcquire();
      iVar2 = *local_148;
      *local_148 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2c08;
    }
    FUN_00287df8(local_148,2,8);
  }
LAB_000e2c08:
  if (*local_140 != -1) {
    if (*local_140 != 0) {
      LOAcquire();
      iVar2 = *local_140;
      *local_140 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2c3c;
    }
    FUN_00287df8(local_140,2,8);
  }
LAB_000e2c3c:
  if (*local_138 != -1) {
    if (*local_138 != 0) {
      LOAcquire();
      iVar2 = *local_138;
      *local_138 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2c70;
    }
    FUN_00287df8(local_138,2,8);
  }
LAB_000e2c70:
  if (*local_130 != -1) {
    if (*local_130 != 0) {
      LOAcquire();
      iVar2 = *local_130;
      *local_130 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2ca4;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000e2ca4:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar4 = *local_128;
      *local_128 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e2cd8;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000e2cd8:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar4 = *local_100[0];
      *local_100[0] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e2d0c;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e2d0c:
  local_100[0] = &DAT_0142f030;
  FUN_00039a7c(uVar6,uVar7,(undefined8 *)(param_1 + 0xd6e8),local_100,0,0,0);
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar4 = *local_100[0];
      *local_100[0] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e2d6c;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e2d6c:
  uVar7 = *(undefined8 *)(param_1 + 0xd6e8);
  local_100[0] = (uint *)FUN_002efa1c("Generic noise profile is now in its default state.\n\nPlease manually tune Generic Noise setting\n(and optionally Noise Level settings)\nuntil noise is fully gone in Assist Image\nwhile details are mostly preserved."
                                      ,0xd2);
  local_150 = (int *)&DAT_0142f030;
  FUN_0003c454(uVar6,uVar7,param_1 + 0xd6f0,local_100,&local_150,0,0xc);
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar2 = *local_150;
      *local_150 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000e2de4;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000e2de4:
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar4 = *local_100[0];
      *local_100[0] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e2e18;
    }
    FUN_00287df8(local_100[0],2,8);
  }
LAB_000e2e18:
  FUN_003d0e8c(auStack_158,uVar6,"2scrollAvailabilityMayBeChanged()",param_1,
               "1onScrollAvailabilityMayBeChanged()",0);
  FUN_003d24d8(auStack_158);
  FUN_00038128(uVar6);
  local_100[0] = &DAT_0142f0a8;
  local_128 = *(uint **)(*(long *)(param_1 + 0xd750) + 0x90);
  FUN_0072b508(local_100,&local_128,&local_130);
  local_130 = *(int **)(param_1 + 0xd6b8);
  FUN_0072b508(local_100,&local_130,&local_138);
  FUN_0001cacc(uVar6,local_100);
  if (local_100[0][4] != 0xffffffff) {
    if (local_100[0][4] != 0) {
      LOAcquire();
      uVar4 = local_100[0][4];
      local_100[0][4] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e2ed0;
    }
    FUN_002a9a28(local_100[0],FUN_0072a91c);
  }
LAB_000e2ed0:
  FUN_00093be0(param_1,uVar6);
  FUN_0002c744(*(undefined8 *)(param_1 + 0xd770),*(undefined8 *)(param_1 + 0x70));
  FUN_0002c744(*(undefined8 *)(param_1 + 0xd778),*(undefined8 *)(param_1 + 0x78));
  FUN_0002c744(*(undefined8 *)(param_1 + 0xd780),*(undefined8 *)(param_1 + 0x80));
  uVar3 = DAT_0142f034;
  lVar12 = *(long *)(param_1 + 0xd740);
  local_128 = &DAT_0142f030;
  uVar4 = DAT_0142f034 + 1;
  uVar13 = DAT_0142f038 & 0x7fffffff;
  uVar6 = *(undefined8 *)(param_1 + 0xd7d8);
  if ((DAT_0142f030 < 2) && (uVar4 <= uVar13)) {
    *(undefined8 *)((long)&DAT_0142f030 + (long)(int)DAT_0142f034 * 8 + DAT_0142f040) = uVar6;
  }
  else {
    uVar3 = uVar4;
    if (uVar4 <= uVar13) {
      uVar3 = uVar13;
    }
    FUN_000bf60c(&local_128,uVar3,(ulong)(uVar13 < uVar4) << 3);
    *(undefined8 *)((long)local_128 + (long)(int)local_128[1] * 8 + *(long *)(local_128 + 4)) =
         uVar6;
    uVar3 = local_128[1];
    uVar4 = uVar3 + 1;
  }
  local_128[1] = uVar4;
  uVar3 = uVar3 + 2;
  uVar13 = local_128[2] & 0x7fffffff;
  uVar6 = *(undefined8 *)(param_1 + 0xd7e0);
  if ((*local_128 < 2) && (uVar3 <= uVar13)) {
    lVar16 = (long)local_128 + *(long *)(local_128 + 4);
  }
  else {
    uVar4 = uVar3;
    if (uVar3 <= uVar13) {
      uVar4 = uVar13;
    }
    FUN_000bf60c(&local_128,uVar4,(ulong)(uVar13 < uVar3) << 3);
    lVar16 = (long)local_128 + *(long *)(local_128 + 4);
    uVar4 = local_128[1];
  }
  *(undefined8 *)(lVar16 + (long)(int)uVar4 * 8) = uVar6;
  uVar3 = local_128[1];
  uVar4 = uVar3 + 1;
  local_128[1] = uVar4;
  uVar3 = uVar3 + 2;
  uVar13 = local_128[2] & 0x7fffffff;
  uVar6 = *(undefined8 *)(param_1 + 0xd7e8);
  if ((*local_128 < 2) && (uVar3 <= uVar13)) {
    lVar16 = (long)local_128 + *(long *)(local_128 + 4);
  }
  else {
    uVar4 = uVar3;
    if (uVar3 <= uVar13) {
      uVar4 = uVar13;
    }
    FUN_000bf60c(&local_128,uVar4,(ulong)(uVar13 < uVar3) << 3);
    lVar16 = (long)local_128 + *(long *)(local_128 + 4);
    uVar4 = local_128[1];
  }
  *(undefined8 *)(lVar16 + (long)(int)uVar4 * 8) = uVar6;
  uVar3 = local_128[1];
  uVar4 = uVar3 + 1;
  local_128[1] = uVar4;
  uVar3 = uVar3 + 2;
  uVar13 = local_128[2] & 0x7fffffff;
  uVar6 = *(undefined8 *)(lVar12 + 0xf8);
  if ((*local_128 < 2) && (uVar3 <= uVar13)) {
    lVar16 = (long)local_128 + *(long *)(local_128 + 4);
  }
  else {
    uVar4 = uVar3;
    if (uVar3 <= uVar13) {
      uVar4 = uVar13;
    }
    FUN_000bf60c(&local_128,uVar4,(ulong)(uVar13 < uVar3) << 3);
    lVar16 = (long)local_128 + *(long *)(local_128 + 4);
    uVar4 = local_128[1];
  }
  *(undefined8 *)(lVar16 + (long)(int)uVar4 * 8) = uVar6;
  uVar3 = local_128[1];
  uVar4 = uVar3 + 1;
  local_128[1] = uVar4;
  uVar3 = uVar3 + 2;
  uVar13 = local_128[2] & 0x7fffffff;
  uVar6 = *(undefined8 *)(lVar12 + 0x108);
  if ((*local_128 < 2) && (uVar3 <= uVar13)) {
    lVar16 = (long)local_128 + *(long *)(local_128 + 4);
  }
  else {
    uVar4 = uVar3;
    if (uVar3 <= uVar13) {
      uVar4 = uVar13;
    }
    FUN_000bf60c(&local_128,uVar4,(ulong)(uVar13 < uVar3) << 3);
    lVar16 = (long)local_128 + *(long *)(local_128 + 4);
    uVar4 = local_128[1];
  }
  *(undefined8 *)(lVar16 + (long)(int)uVar4 * 8) = uVar6;
  uVar3 = local_128[1];
  uVar4 = uVar3 + 1;
  local_128[1] = uVar4;
  uVar3 = uVar3 + 2;
  uVar13 = local_128[2] & 0x7fffffff;
  uVar6 = *(undefined8 *)(lVar12 + 0x118);
  if ((*local_128 < 2) && (uVar3 <= uVar13)) {
    lVar12 = (long)local_128 + *(long *)(local_128 + 4);
  }
  else {
    uVar4 = uVar3;
    if (uVar3 <= uVar13) {
      uVar4 = uVar13;
    }
    FUN_000bf60c(&local_128,uVar4,(ulong)(uVar13 < uVar3) << 3);
    lVar12 = (long)local_128 + *(long *)(local_128 + 4);
    uVar4 = local_128[1];
  }
  *(undefined8 *)(lVar12 + (long)(int)uVar4 * 8) = uVar6;
  uVar4 = local_128[1];
  local_128[1] = (uint)((long)(int)uVar4 + 1);
  if (*local_128 == 0) {
    if ((int)local_128[2] < 0) {
      puVar10 = (uint *)FUN_00287b7c(8,8,local_128[2] & 0x7fffffff,0);
      local_100[0] = puVar10;
      if (puVar10 == (uint *)0x0) {
        FUN_00c78400();
      }
      puVar10[2] = puVar10[2] | 0x80000000;
      puVar20 = puVar10;
    }
    else {
      puVar10 = (uint *)FUN_00287b7c(8,8,(long)(int)uVar4 + 1,0);
      puVar20 = puVar10;
      local_100[0] = puVar10;
      if (puVar10 == (uint *)0x0) {
        FUN_00c78400();
        puVar20 = (uint *)0x0;
      }
    }
    if ((puVar20[2] & 0x7fffffff) != 0) {
      _memcpy((void *)((long)puVar20 + *(long *)(puVar20 + 4)),
              (void *)((long)local_128 + *(long *)(local_128 + 4)),(long)(int)local_128[1] << 3);
      puVar20[1] = local_128[1];
    }
  }
  else {
    if (*local_128 != 0xffffffff) {
      LOAcquire();
      *local_128 = *local_128 + 1;
      LORelease();
    }
    local_100[0] = local_128;
    puVar10 = local_128;
  }
  puVar5 = (undefined8 *)((long)puVar10 + *(long *)(puVar10 + 4));
  uVar4 = puVar10[1];
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar3 = *local_128;
      *local_128 = uVar3 - 1;
      LORelease();
      if (uVar3 != 1) goto LAB_000e32ac;
    }
    FUN_00287df8(local_128,8,8);
  }
LAB_000e32ac:
  if (puVar5 + (int)uVar4 != puVar5) {
    lVar12 = (long)(int)uVar4 << 3;
    do {
      FUN_0002c744(*puVar5,*(undefined8 *)(param_1 + 0x68));
      puVar5 = puVar5 + 1;
      lVar12 = lVar12 + -8;
    } while (lVar12 != 0);
  }
  if (*local_100[0] != 0xffffffff) {
    if (*local_100[0] != 0) {
      LOAcquire();
      uVar4 = *local_100[0];
      *local_100[0] = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_000e3304;
    }
    FUN_00287df8(local_100[0],8,8);
  }
LAB_000e3304:
  FUN_00753798(*(undefined8 *)(param_1 + 0x138));
  return param_1;
}


