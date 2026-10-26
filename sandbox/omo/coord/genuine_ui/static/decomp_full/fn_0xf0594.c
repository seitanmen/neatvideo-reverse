// FUN_000f0594 size=4668

undefined8 * FUN_000f0594(undefined8 *param_1,undefined8 param_2)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  undefined8 *puVar5;
  long lVar6;
  undefined8 uVar7;
  undefined8 uVar8;
  undefined8 in_stack_ffffffffffffff20;
  uint uVar9;
  undefined1 auStack_c0 [8];
  undefined1 auStack_b8 [8];
  undefined1 auStack_b0 [8];
  undefined1 auStack_a8 [8];
  undefined1 auStack_a0 [8];
  undefined1 auStack_98 [8];
  undefined1 auStack_90 [8];
  undefined1 auStack_88 [8];
  undefined1 auStack_80 [8];
  undefined1 auStack_78 [8];
  undefined1 auStack_70 [8];
  int *local_68;
  int *local_60;
  int *local_58;
  
  uVar9 = (uint)((ulong)in_stack_ffffffffffffff20 >> 0x20);
  puVar5 = (undefined8 *)FUN_007429b0(param_1,param_2,0);
  *puVar5 = &DAT_013c6200;
  puVar5[2] = &DAT_013c63b8;
  puVar5[0x2e] = &DAT_013c6408;
  FUN_000673dc(puVar5 + 0x2e,param_1);
  *param_1 = &DAT_013c6200;
  param_1[2] = &DAT_013c63b8;
  param_1[0x2e] = &DAT_013c6408;
  FUN_000f19c4(param_1 + 6,param_1);
  *(undefined1 *)(param_1 + 0x37) = 1;
  uVar1 = *(uint *)(DAT_0141bf60 + 0x59e930);
  uVar2 = *(uint *)(DAT_0141bf70 + 0x4fe1c8);
  iVar4 = (int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10);
  if (iVar4 != 0) {
    if (DAT_0141bf58 != 0) {
      uVar1 = uVar1 >> 0x1d | uVar1 << 3;
    }
    *(uint *)("Backend is waiting for QNetworkSession to connect, but there is none!" +
             DAT_0141bf68 + 5) = uVar1;
  }
  DAT_0141bf58 = iVar4;
  FUN_0000e1b8(*(undefined8 *)(param_1[0xc] + 0x78),0,1);
  uVar7 = *(undefined8 *)(param_1[0xc] + 0x78);
  local_58 = (int *)FUN_002efa1c("",0);
  FUN_0000e4f4(uVar7,0,0,&local_58,0);
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar3 = *local_58;
      *local_58 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f06f0;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f06f0:
  *(undefined4 *)(param_1[0xc] + 0xb4) = 0;
  uVar7 = param_1[0xb];
  iVar3 = FUN_0074b660(uVar7);
  FUN_0074c210(uVar7,iVar3 + 4);
  uVar7 = param_1[0xc];
  iVar3 = FUN_0074b660(uVar7);
  FUN_0074c210(uVar7,iVar3 + 4);
  uVar7 = param_1[0xc];
  local_58 = (int *)FUN_002efa1c(" ",1);
  FUN_0002a540(uVar7,&local_58);
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar3 = *local_58;
      *local_58 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f0780;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f0780:
  FUN_0000e1b8(*(undefined8 *)(param_1[0xc] + 0x78),1,0);
  uVar7 = *(undefined8 *)(param_1[0xc] + 0x78);
  local_58 = (int *)FUN_002efa1c("",0);
  FUN_0000e4f4(uVar7,1,0,&local_58,0);
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar3 = *local_58;
      *local_58 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f07fc;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f07fc:
  uVar7 = *(undefined8 *)(param_1[0xc] + 0x78);
  FUN_003bbff0(&local_58,&DAT_013c61c0,"Zoom to Fit",0,0xffffffff);
  local_60 = (int *)FUN_002efa1c("",0);
  FUN_0047d068(&local_68);
  uVar7 = FUN_0000eb18(uVar7,0,0,&local_58,1,0,&local_60,1,(ulong)uVar9 << 0x20,&local_68);
  param_1[0x35] = uVar7;
  FUN_0047d3f4(&local_68);
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar3 = *local_60;
      *local_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f08b0;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000f08b0:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar3 = *local_58;
      *local_58 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000f08e4;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f08e4:
  lVar6 = param_1[0xe];
  *(undefined1 *)(lVar6 + 0x98) = 1;
  if (iVar4 == 0) {
    param_1[0x36] = 0;
  }
  else {
    uVar1 = *(uint *)(DAT_0141bf80 + 0x4efc48);
    if (DAT_0141bf78 != 0) {
      uVar1 = uVar1 >> 1 | uVar1 << 0x1f;
    }
    param_1[0x36] = 0;
    if ((int)(CONCAT44((uVar1 + (uVar1 >> 0x10)) * 0x101 + (uVar1 >> 0x18),uVar1) >> 0x10) != 0) {
      uVar1 = *(uint *)(DAT_0141bf88 + 0x180ba98);
      uVar2 = *(uint *)(DAT_0141bf98 + 0x99f118);
      if ((int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10) != 1) {
        uVar2 = (uVar1 ^ uVar1 >> 1) & 0x41515505;
        *(uint *)(DAT_0141bf90 + 0x18eca28) = (uVar2 | uVar2 << 1) ^ uVar1;
      }
    }
  }
  uVar7 = *(undefined8 *)(lVar6 + 0x78);
  *(undefined4 *)(lVar6 + 0xb4) = 0;
  FUN_0000e1b8(uVar7,0,1);
  FUN_003bbff0(&local_58,&DAT_013c61c0,"Primary Modes",0,0xffffffff);
  FUN_0000e4f4(uVar7,0,0,&local_58,0);
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar4 = *local_58;
      *local_58 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f0a2c;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f0a2c:
  uVar8 = param_1[0xe];
  local_58 = (int *)FUN_002efa1c("",0);
  if (*(int *)(DAT_0155b3b8 + 4) == 0) {
    local_60 = (int *)FUN_002efa1c("",0);
  }
  else {
    FUN_001c0fe4(&local_60,&DAT_0155b3b8,".png");
  }
  FUN_0047d068(&local_68);
  FUN_0002a49c(uVar8,0,0,&DAT_0155b380,&local_58,0,0,&local_60,0,&DAT_0155b348,&local_68);
  FUN_0047d3f4(&local_68);
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar4 = *local_60;
      *local_60 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f0b0c;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000f0b0c:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar4 = *local_58;
      *local_58 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f0b40;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f0b40:
  uVar8 = param_1[0xe];
  local_58 = (int *)FUN_002efa1c("",0);
  if (*(int *)(DAT_0155b3c0 + 4) == 0) {
    local_60 = (int *)FUN_002efa1c("",0);
  }
  else {
    FUN_001c0fe4(&local_60,&DAT_0155b3c0,".png");
  }
  FUN_0047d068(&local_68);
  FUN_0002a49c(uVar8,0,1,&DAT_0155b388,&local_58,0,0,&local_60,0,&DAT_0155b350,&local_68);
  FUN_0047d3f4(&local_68);
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar4 = *local_60;
      *local_60 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f0c08;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000f0c08:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar4 = *local_58;
      *local_58 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f0c3c;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f0c3c:
  FUN_003bbff0(&local_58,&DAT_013c61c0,"Channel Components",0,0xffffffff);
  FUN_0000e4f4(uVar7,0,1,&local_58,0);
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar4 = *local_58;
      *local_58 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f0ca8;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f0ca8:
  uVar8 = param_1[0xe];
  local_58 = (int *)FUN_002efa1c("",0);
  if (*(int *)(DAT_0155b3c8 + 4) == 0) {
    local_60 = (int *)FUN_002efa1c("",0);
  }
  else {
    FUN_001c0fe4(&local_60,&DAT_0155b3c8,".png");
  }
  FUN_0047d068(&local_68);
  FUN_0002a49c(uVar8,0,2,&DAT_0155b390,&local_58,0,0,&local_60,0x100000000,&DAT_0155b358,&local_68);
  FUN_0047d3f4(&local_68);
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar4 = *local_60;
      *local_60 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f0d74;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000f0d74:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar4 = *local_58;
      *local_58 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f0da8;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f0da8:
  uVar8 = param_1[0xe];
  local_58 = (int *)FUN_002efa1c("",0);
  if (*(int *)(DAT_0155b3d0 + 4) == 0) {
    local_60 = (int *)FUN_002efa1c("",0);
  }
  else {
    FUN_001c0fe4(&local_60,&DAT_0155b3d0,".png");
  }
  FUN_0047d068(&local_68);
  FUN_0002a49c(uVar8,0,3,&DAT_0155b398,&local_58,0,0,&local_60,0x100000000,&DAT_0155b360,&local_68);
  FUN_0047d3f4(&local_68);
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar4 = *local_60;
      *local_60 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f0e74;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000f0e74:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar4 = *local_58;
      *local_58 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f0ea8;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f0ea8:
  FUN_003bbff0(&local_58,&DAT_013c61c0,"Frequency Components",0,0xffffffff);
  FUN_0000e4f4(uVar7,0,2,&local_58,0);
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar4 = *local_58;
      *local_58 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f0f14;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f0f14:
  uVar8 = param_1[0xe];
  local_58 = (int *)FUN_002efa1c("",0);
  if (*(int *)(DAT_0155b3d8 + 4) == 0) {
    local_60 = (int *)FUN_002efa1c("",0);
  }
  else {
    FUN_001c0fe4(&local_60,&DAT_0155b3d8,".png");
  }
  FUN_0047d068(&local_68);
  FUN_0002a49c(uVar8,0,4,&DAT_0155b3a0,&local_58,0,0,&local_60,0x200000000,&DAT_0155b368,&local_68);
  FUN_0047d3f4(&local_68);
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar4 = *local_60;
      *local_60 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f0fe0;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000f0fe0:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar4 = *local_58;
      *local_58 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f1014;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f1014:
  uVar8 = param_1[0xe];
  local_58 = (int *)FUN_002efa1c("",0);
  if (*(int *)(DAT_0155b3e0 + 4) == 0) {
    local_60 = (int *)FUN_002efa1c("",0);
  }
  else {
    FUN_001c0fe4(&local_60,&DAT_0155b3e0,".png");
  }
  FUN_0047d068(&local_68);
  FUN_0002a49c(uVar8,0,5,&DAT_0155b3a8,&local_58,0,0,&local_60,0x200000000,&DAT_0155b370,&local_68);
  FUN_0047d3f4(&local_68);
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar4 = *local_60;
      *local_60 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f10e0;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000f10e0:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar4 = *local_58;
      *local_58 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f1114;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f1114:
  uVar8 = param_1[0xe];
  local_58 = (int *)FUN_002efa1c("",0);
  if (*(int *)(DAT_0155b3e8 + 4) == 0) {
    local_60 = (int *)FUN_002efa1c("",0);
  }
  else {
    FUN_001c0fe4(&local_60,&DAT_0155b3e8,".png");
  }
  FUN_0047d068(&local_68);
  FUN_0002a49c(uVar8,0,6,&DAT_0155b3b0,&local_58,0,0,&local_60,0x200000000,&DAT_0155b378,&local_68);
  FUN_0047d3f4(&local_68);
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar4 = *local_60;
      *local_60 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f11e0;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000f11e0:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar4 = *local_58;
      *local_58 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f1214;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f1214:
  FUN_0000e1b8(uVar7,1,1);
  FUN_003bbff0(&local_58,&DAT_013c61c0,"Viewer Split",0,0xffffffff);
  FUN_0000e4f4(uVar7,1,0,&local_58,0);
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar4 = *local_58;
      *local_58 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f1290;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f1290:
  uVar7 = param_1[0xe];
  local_58 = (int *)FUN_002efa1c("",0);
  FUN_0007e7a0(&local_60,&DAT_0155b408);
  local_68 = &DAT_0142f030;
  FUN_0047d068(auStack_70);
  FUN_0002a49c(uVar7,0,0,&DAT_0155b3f0,&local_58,0,0,&local_60,1,&local_68,auStack_70);
  FUN_0047d3f4(auStack_70);
  if (*local_68 != -1) {
    if (*local_68 != 0) {
      LOAcquire();
      iVar4 = *local_68;
      *local_68 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f134c;
    }
    FUN_00287df8(local_68,2,8);
  }
LAB_000f134c:
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar4 = *local_60;
      *local_60 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f1380;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000f1380:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar4 = *local_58;
      *local_58 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f13b4;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f13b4:
  uVar7 = param_1[0xe];
  local_58 = (int *)FUN_002efa1c("",0);
  FUN_0007e7a0(&local_60,&DAT_0155b410);
  local_68 = &DAT_0142f030;
  FUN_0047d068(auStack_70);
  FUN_0002a49c(uVar7,0,1,&DAT_0155b3f8,&local_58,0,0,&local_60,1,&local_68,auStack_70);
  FUN_0047d3f4(auStack_70);
  if (*local_68 != -1) {
    if (*local_68 != 0) {
      LOAcquire();
      iVar4 = *local_68;
      *local_68 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f1458;
    }
    FUN_00287df8(local_68,2,8);
  }
LAB_000f1458:
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar4 = *local_60;
      *local_60 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f148c;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000f148c:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar4 = *local_58;
      *local_58 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f14c0;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f14c0:
  uVar7 = param_1[0xe];
  local_58 = (int *)FUN_002efa1c("",0);
  FUN_0007e7a0(&local_60,&DAT_0155b418);
  local_68 = &DAT_0142f030;
  FUN_0047d068(auStack_70);
  FUN_0002a49c(uVar7,0,2,&DAT_0155b400,&local_58,0,0,&local_60,1,&local_68,auStack_70);
  FUN_0047d3f4(auStack_70);
  if (*local_68 != -1) {
    if (*local_68 != 0) {
      LOAcquire();
      iVar4 = *local_68;
      *local_68 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f1564;
    }
    FUN_00287df8(local_68,2,8);
  }
LAB_000f1564:
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar4 = *local_60;
      *local_60 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f1598;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_000f1598:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar4 = *local_58;
      *local_58 = iVar4 + -1;
      LORelease();
      if (iVar4 != 1) goto LAB_000f15cc;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_000f15cc:
  FUN_0000ff64(*(undefined8 *)(param_1[0xe] + 0x78),1);
  FUN_0002eb9c(param_1[0x15],5);
  FUN_007430f8(param_1[0x15],0x5a,1);
  FUN_003d0e8c(auStack_78,param_1[0xc],"2currentChanged(int, int)",param_1,"1setZoomIndex(int)",0);
  FUN_003d24d8(auStack_78);
  FUN_003d0e8c(auStack_80,param_1[0xc],"2released()",param_1,"2needToSetFocusOnViewerArea()",0);
  FUN_003d24d8(auStack_80);
  FUN_003d0e8c(auStack_88,param_1[0x35],"2triggered(bool)",param_1,"2zoomToFitTriggered()",0);
  FUN_003d24d8(auStack_88);
  FUN_003d0e8c(auStack_90,param_1[0xe],"2currentChanged(int, int)",param_1,
               "1onViewerModeCurrentChanged()",0);
  FUN_003d24d8(auStack_90);
  FUN_003d0e8c(auStack_98,param_1[0xe],"2released()",param_1,"2needToSetFocusOnViewerArea()",0);
  FUN_003d24d8(auStack_98);
  FUN_003d0e8c(auStack_a0,param_1[0x10],"2toggled(bool)",param_1,"1onDragButtonToggled(bool)",0);
  FUN_003d24d8(auStack_a0);
  FUN_003d0e8c(auStack_a8,param_1[0x12],"2toggled(bool)",param_1,"2alphaModeChanged(bool)",0);
  FUN_003d24d8(auStack_a8);
  FUN_003d0e8c(auStack_b0,param_1[0x15],"2preciseValueChanged(int, int)",param_1,
               "1setLevelAdjustment(int)",0);
  FUN_003d24d8(auStack_b0);
  FUN_003d0e8c(auStack_b8,param_1[0x16],"2clicked(bool)",param_1,
               "1onResetLevelAdjustmentButtonClicked()",0);
  FUN_003d24d8(auStack_b8);
  FUN_003d0e8c(auStack_c0,param_1[0x2d],"2clicked(bool)",param_1,"1onImageInfoButtonClicked()",0);
  FUN_003d24d8(auStack_c0);
  return param_1;
}


