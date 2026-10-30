// func entry: 000b0934 name: FUN_000b0934 size: 43472

long FUN_000b0934(long param_1)

{
  char *pcVar1;
  undefined8 *puVar2;
  undefined8 *puVar3;
  uint uVar4;
  int iVar5;
  uint uVar6;
  uint uVar7;
  undefined8 *puVar8;
  undefined8 uVar9;
  undefined8 uVar10;
  undefined8 *puVar11;
  undefined8 *puVar12;
  undefined8 uVar13;
  uint *puVar14;
  long lVar15;
  uint *puVar16;
  undefined8 uVar17;
  undefined8 uVar18;
  undefined8 in_stack_fffffffffffffd80;
  undefined4 uVar21;
  ulong uVar19;
  int **ppiVar20;
  undefined4 uVar22;
  undefined1 auStack_180 [8];
  undefined1 auStack_178 [8];
  int *local_170;
  int *local_168;
  int *local_160;
  int *local_158;
  int *local_150;
  int *local_148;
  int *local_140;
  int *local_138;
  uint *local_130;
  uint *local_128;
  int *local_120;
  int *local_118;
  int *local_110;
  int *local_108;
  int *local_100;
  int *local_f8;
  int *local_f0;
  int *local_e8;
  int *local_e0;
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
  long local_70;
  
  uVar6 = (uint)((ulong)in_stack_fffffffffffffd80 >> 0x20);
  local_70 = *(long *)PTR____stack_chk_guard_013b4320;
  puVar8 = (undefined8 *)FUN_000bf708();
  *puVar8 = &DAT_013c4238;
  puVar8[2] = &DAT_013c4448;
  puVar8[6] = &DAT_013c4498;
  uVar9 = __Znwm(0xe390);
  FUN_001b0088(uVar9,*(undefined4 *)(DAT_0155b458 + 400),1);
  *(undefined8 *)(param_1 + 0xda0) = uVar9;
  *(undefined8 *)(*(long *)(param_1 + 0xda8) + 0x18) = uVar9;
  uVar9 = *(undefined8 *)(*(long *)(param_1 + 0x148) + 0x50);
  FUN_003bbff0(&local_b8,&DAT_013c41f8,"Temporal Settings",0,0xffffffff);
  FUN_00750130(uVar9,&local_b8);
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar5 = *local_b8;
      *local_b8 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b0a24;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000b0a24:
  uVar10 = FUN_00850130(uVar9);
  FUN_003bbff0(&local_b8,&DAT_013c41f8,"Temporal Filter",0,0xffffffff);
  local_e0 = (int *)&DAT_0142f030;
  local_108 = (int *)FUN_002efa1c("Enable Temporal Filter",0x16);
  puVar8 = (undefined8 *)(param_1 + 0xce0);
  lVar15 = (ulong)uVar6 << 0x20;
  FUN_00039518(uVar9,uVar10,puVar8,&local_b8,&local_e0,&local_108,0x5d,&DAT_0155b945,lVar15);
  uVar21 = (undefined4)((ulong)lVar15 >> 0x20);
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b0ad8;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b0ad8:
  if (*local_e0 != -1) {
    if (*local_e0 != 0) {
      LOAcquire();
      iVar5 = *local_e0;
      *local_e0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b0b0c;
    }
    FUN_00287df8(local_e0,2,8);
  }
LAB_000b0b0c:
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar5 = *local_b8;
      *local_b8 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b0b40;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000b0b40:
  local_b8 = (int *)&DAT_0142f030;
  FUN_00039a7c(uVar9,*(undefined8 *)(param_1 + 0xce0),
               "/System/Library/Frameworks/Cocoa.framework/Versions/A/Cocoa" + param_1 + 0x30,
               &local_b8,0,0,0);
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar5 = *local_b8;
      *local_b8 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b0ba0;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000b0ba0:
  local_80 = (int *)FUN_002efa1c("Normal",6);
  local_78 = (int *)FUN_002efa1c("High",4);
  local_90 = (int *)FUN_002efa1c("Normal (Faster)",0xf);
  local_88 = (int *)FUN_002efa1c("High (Slower)",0xd);
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/Cocoa.framework/Versions/A/Cocoa" + param_1 + 0x30);
  FUN_003bbff0(&local_b8,&DAT_013c41f8,"Quality Mode",0,0xffffffff);
  local_e0 = (int *)FUN_002efa1c("Select Quality Mode of Temporal Filter\nHigher quality mode delivers better filtration results but reduces processing speed"
                                 ,0x7a);
  puVar11 = (undefined8 *)__Znwm(0x10);
  *puVar11 = &DAT_013b6388;
  puVar11[1] = &DAT_0155b9a8;
  *(undefined8 **)((long)&dylib_command_000011f8.cmd + param_1) = puVar11;
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Temporal Quality Mode",0,0xffffffff);
  FUN_0003bb84(uVar9,uVar17,&local_b8,&local_e0,
               "/System/Library/Frameworks/Cocoa.framework/Versions/A/Cocoa" + param_1 + 0x38,0,
               &local_80,&local_90,CONCAT44(uVar21,2),puVar11,&local_108,0xc00000002);
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b0ce4;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b0ce4:
  if (*local_e0 != -1) {
    if (*local_e0 != 0) {
      LOAcquire();
      iVar5 = *local_e0;
      *local_e0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b0d18;
    }
    FUN_00287df8(local_e0,2,8);
  }
LAB_000b0d18:
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar5 = *local_b8;
      *local_b8 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b0d4c;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000b0d4c:
  puVar11 = (undefined8 *)((long)&dylib_command_000011f8.dylib.name.offset + param_1);
  local_b8 = (int *)&DAT_0142f030;
  FUN_00039a7c(uVar9,*(undefined8 *)(param_1 + 0xce0),puVar11,&local_b8,0,0,0);
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar5 = *local_b8;
      *local_b8 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b0dac;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000b0dac:
  local_b8 = (int *)FUN_002efa1c("1",1);
  local_b0 = (int *)FUN_002efa1c("2",1);
  local_a8 = (int *)FUN_002efa1c("3",1);
  local_a0 = (int *)FUN_002efa1c("4",1);
  local_98 = (int *)FUN_002efa1c("5",1);
  FUN_003bbff0(&local_e0,&DAT_013c41f8,"1 frame",0,0xffffffff);
  FUN_003bbff0(&local_d8,&DAT_013c41f8,"2 frames",0,0xffffffff);
  FUN_003bbff0(&local_d0,&DAT_013c41f8,"3 frames",0,0xffffffff);
  FUN_003bbff0(&local_c8,&DAT_013c41f8,"4 frames",0,0xffffffff);
  FUN_003bbff0(&local_c0,&DAT_013c41f8,"5 frames",0,0xffffffff);
  uVar17 = *puVar11;
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Radius",0,0xffffffff);
  local_f0 = (int *)FUN_002efa1c("Radius determines the number of consecutive frames (before and after the current one) used for temporal filtration"
                                 ,0x72);
  local_120 = (int *)&DAT_0142f030;
  local_118 = (int *)&DAT_0142f030;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Temporal Radius",0,0xffffffff);
  FUN_0003b2d4(uVar9,uVar17,&local_108,&local_f0,param_1 + 0xce8,&local_b8,0,&local_e0,0x500000001,
               &DAT_0155b948,&local_118,&local_120,&local_128,0xc00000005,0x100);
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b0fcc;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b0fcc:
  if (*local_120 != -1) {
    if (*local_120 != 0) {
      LOAcquire();
      iVar5 = *local_120;
      *local_120 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1000;
    }
    FUN_00287df8(local_120,2,8);
  }
LAB_000b1000:
  if (*local_118 != -1) {
    if (*local_118 != 0) {
      LOAcquire();
      iVar5 = *local_118;
      *local_118 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1034;
    }
    FUN_00287df8(local_118,2,8);
  }
LAB_000b1034:
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1068;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b1068:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b109c;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b109c:
  FUN_0003c6d4(uVar9,*puVar8,0);
  uVar17 = *(undefined8 *)(param_1 + 0xce0);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Temporal Noise",0,0xffffffff);
  local_f0 = (int *)FUN_002efa1c("Temporal Noise filter reduces the noise present in video using data from several consecutive frames"
                                 ,99);
  local_118 = (int *)((ulong)local_118 & 0xffffffffffffff00);
  FUN_00038ca4(uVar9,uVar17,param_1 + 0x1208,&local_108,&local_f0,0x5b,&local_118,0);
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1144;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b1144:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1178;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b1178:
  local_108 = (int *)&DAT_0142f030;
  FUN_00039a7c(uVar9,*(undefined8 *)((long)&dylib_command_000011f8.dylib.current_version + param_1),
               "/System/Library/Frameworks/Metal.framework/Versions/A/Metal" + param_1,&local_108,0,
               1,0);
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b11d8;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b11d8:
  uVar17 = *(undefined8 *)("/System/Library/Frameworks/Metal.framework/Versions/A/Metal" + param_1);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Noise Level Check",0,0xffffffff);
  local_f0 = (int *)FUN_002efa1c("Check the Temporal Filter mask to find the optimal value of Noise Level.\n\nClick Begin to show the Temporal Filter mask.\nAdjust Noise Level while watching the mask to make it sufficiently cover the noisy areas of the frame."
                                 ,0xde);
  FUN_00038ca4(uVar9,uVar17,
               "/System/Library/Frameworks/Metal.framework/Versions/A/Metal" + param_1 + 8,
               &local_108,&local_f0,0x30,&DAT_00c944f8,3);
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1270;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b1270:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b12a4;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b12a4:
  uVar17 = *(undefined8 *)("/System/Library/Frameworks/Metal.framework/Versions/A/Metal" + param_1);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Noise Level",0,0xffffffff);
  local_f0 = (int *)&DAT_0142f030;
  local_118 = (int *)FUN_002efa1c("Noise Level determines the balance between sufficient reduction of all noise elements and preservation of moving details by temporal filter\n- higher values -> more temporal noise reduction, less attention to moving details\n- lower values  -> less temporal noise reduction, more attention to moving details"
                                  ,0x131);
  local_120 = (int *)FUN_002efa1c("Noise Level determines the balance between sufficient reduction of all noise elements and preservation of moving details by temporal filter\n- higher values -> more temporal noise reduction, less attention to moving details\n- lower values  -> less temporal noise reduction, more attention to moving details"
                                  ,0x131);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Temporal Noise Level",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Noise Level determines the balance between sufficient reduction of all noise elements and preservation of moving details by temporal filter\n- higher values -> more temporal noise reduction, less attention to moving details\n- lower values  -> less temporal noise reduction, more attention to moving details"
                                   ,0x131);
  FUN_0003aa60(0,0x40200000,uVar9,uVar17,0,&local_108,0,&local_f0,&local_118,
               "/System/Library/Frameworks/Metal.framework/Versions/A/Metal" + param_1 + 0x10,
               &local_120,0x96ffffff9c,0x1400000005,&DAT_0155b94c,&local_128,0,&local_130,0x58,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b13c8;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b13c8:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b13fc;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b13fc:
  if (*local_120 != -1) {
    if (*local_120 != 0) {
      LOAcquire();
      iVar5 = *local_120;
      *local_120 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1430;
    }
    FUN_00287df8(local_120,2,8);
  }
LAB_000b1430:
  if (*local_118 != -1) {
    if (*local_118 != 0) {
      LOAcquire();
      iVar5 = *local_118;
      *local_118 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1464;
    }
    FUN_00287df8(local_118,2,8);
  }
LAB_000b1464:
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1498;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b1498:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b14cc;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b14cc:
  local_108 = (int *)&DAT_0142f030;
  FUN_00039a7c(uVar9,*(undefined8 *)((long)&dylib_command_000011f8.dylib.current_version + param_1),
               "/System/Library/Frameworks/Metal.framework/Versions/A/Metal" + param_1 + 0x18,
               &local_108,0,1,0);
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b152c;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b152c:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/Metal.framework/Versions/A/Metal" + param_1 + 0x18);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Local Flicker Check",0,0xffffffff);
  local_f0 = (int *)FUN_002efa1c("Check the Local Flicker mask to find the optimal value of Local Flicker setting.\n\nClick Begin to show the Local Flicker mask.\nAdjust Local Flicker setting while watching the mask to make it sufficiently cover the areas where local flicker is present."
                                 ,0xfa);
  FUN_00038ca4(uVar9,uVar17,
               "/System/Library/Frameworks/Metal.framework/Versions/A/Metal" + param_1 + 0x20,
               &local_108,&local_f0,0x30,&DAT_00c944f8,6);
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b15c4;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b15c4:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b15f8;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b15f8:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/Metal.framework/Versions/A/Metal" + param_1 + 0x18);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Local Flicker",0,0xffffffff);
  local_f0 = (int *)&DAT_0142f030;
  local_118 = (int *)FUN_002efa1c("The strength of local (part of frame) flicker to be reduced\n- higher values -> stronger brightness variations are considered flicker, stronger reduction of local flicker\n- lower values  -> weaker brightness variations are considered flicker, less reduction of local flicker"
                                  ,0x111);
  local_120 = (int *)FUN_002efa1c("The strength of local (part of frame) flicker to be reduced\n- higher values -> stronger brightness variations are considered flicker, stronger reduction of local flicker\n- lower values  -> weaker brightness variations are considered flicker, less reduction of local flicker"
                                  ,0x111);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Local Flicker Threshold",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("The strength of local (part of frame) flicker to be reduced\n- higher values -> stronger brightness variations are considered flicker, stronger reduction of local flicker\n- lower values  -> weaker brightness variations are considered flicker, less reduction of local flicker"
                                   ,0x111);
  FUN_0003aa60(0,0x42480000,uVar9,uVar17,0,&local_108,0,&local_f0,&local_118,
               "/System/Library/Frameworks/Metal.framework/Versions/A/Metal" + param_1 + 0x28,
               &local_120,0x3200000000,0x500000001,&DAT_0155b9d8,&local_128,0,&local_130,0x48,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b1718;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b1718:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b174c;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b174c:
  if (*local_120 != -1) {
    if (*local_120 != 0) {
      LOAcquire();
      iVar5 = *local_120;
      *local_120 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1780;
    }
    FUN_00287df8(local_120,2,8);
  }
LAB_000b1780:
  if (*local_118 != -1) {
    if (*local_118 != 0) {
      LOAcquire();
      iVar5 = *local_118;
      *local_118 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b17b4;
    }
    FUN_00287df8(local_118,2,8);
  }
LAB_000b17b4:
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b17e8;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b17e8:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b181c;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b181c:
  local_108 = (int *)&DAT_0142f030;
  FUN_00039a7c(uVar9,*(undefined8 *)((long)&dylib_command_000011f8.dylib.current_version + param_1),
               "/System/Library/Frameworks/Metal.framework/Versions/A/Metal" + param_1 + 0x30,
               &local_108,6,1,0);
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b187c;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b187c:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/Metal.framework/Versions/A/Metal" + param_1 + 0x30);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Frame Flicker",0,0xffffffff);
  local_f0 = (int *)&DAT_0142f030;
  local_118 = (int *)FUN_002efa1c("The strength of whole-frame flicker to be reduced\n- higher values -> stronger brightness variations are considered flicker, stronger reduction of whole-frame flicker\n- lower values  -> weaker brightness variations are considered flicker, less reduction of whole-frame flicker"
                                  ,0x113);
  local_120 = (int *)FUN_002efa1c("The strength of whole-frame flicker to be reduced\n- higher values -> stronger brightness variations are considered flicker, stronger reduction of whole-frame flicker\n- lower values  -> weaker brightness variations are considered flicker, less reduction of whole-frame flicker"
                                  ,0x113);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Full Frame Flicker Threshold",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("The strength of whole-frame flicker to be reduced\n- higher values -> stronger brightness variations are considered flicker, stronger reduction of whole-frame flicker\n- lower values  -> weaker brightness variations are considered flicker, less reduction of whole-frame flicker"
                                   ,0x113);
  FUN_0003aa60(0,0x42480000,uVar9,uVar17,0,&local_108,0,&local_f0,&local_118,
               "/System/Library/Frameworks/Metal.framework/Versions/A/Metal" + param_1 + 0x38,
               &local_120,0x3200000000,0x500000001,&DAT_0155b9dc,&local_128,0,&local_130,0x48,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b199c;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b199c:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b19d0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b19d0:
  if (*local_120 != -1) {
    if (*local_120 != 0) {
      LOAcquire();
      iVar5 = *local_120;
      *local_120 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1a04;
    }
    FUN_00287df8(local_120,2,8);
  }
LAB_000b1a04:
  if (*local_118 != -1) {
    if (*local_118 != 0) {
      LOAcquire();
      iVar5 = *local_118;
      *local_118 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1a38;
    }
    FUN_00287df8(local_118,2,8);
  }
LAB_000b1a38:
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1a6c;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b1a6c:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1aa0;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b1aa0:
  uVar17 = *(undefined8 *)((long)&dylib_command_000011f8.dylib.current_version + param_1);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Noise Reduction Amount",0,0xffffffff);
  local_f0 = (int *)FUN_002efa1c("Amount of temporal noise reduction\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                 ,0x86);
  FUN_00038ca4(uVar9,uVar17,(long)&dylib_command_00001250.cmd + param_1,&local_108,&local_f0,0x5b,
               &DAT_00c944f8,4);
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1b38;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b1b38:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1b6c;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b1b6c:
  uVar17 = *(undefined8 *)((long)&dylib_command_00001250.cmd + param_1);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Amount",0,0xffffffff);
  local_f0 = (int *)&DAT_0142f030;
  local_118 = (int *)FUN_002efa1c("Amount of temporal noise reduction\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0x86);
  local_120 = (int *)FUN_002efa1c("Amount of temporal noise reduction\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0x86);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Temporal Noise Reduction Amount",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Amount of temporal noise reduction\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                   ,0x86);
  FUN_0003aa60(0,0x3f800000,uVar9,uVar17,param_1 + 0x1258,&local_108,0,&local_f0,&local_118,
               param_1 + 0x1260,&local_120,0x6400000000,0x1400000005,&DAT_0155b9a0,&local_128,
               "/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" + param_1
               ,&local_130,0x5a,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b1c94;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b1c94:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b1cc8;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b1cc8:
  if (*local_120 != -1) {
    if (*local_120 != 0) {
      LOAcquire();
      iVar5 = *local_120;
      *local_120 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1cfc;
    }
    FUN_00287df8(local_120,2,8);
  }
LAB_000b1cfc:
  if (*local_118 != -1) {
    if (*local_118 != 0) {
      LOAcquire();
      iVar5 = *local_118;
      *local_118 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1d30;
    }
    FUN_00287df8(local_118,2,8);
  }
LAB_000b1d30:
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1d64;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b1d64:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1d98;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b1d98:
  FUN_0003c6d4(uVar9,*puVar8,0);
  uVar17 = *(undefined8 *)(param_1 + 0xce0);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Repeated Frames",0,0xffffffff);
  local_f0 = (int *)FUN_002efa1c("Special treatment for clips with repeated frames:\n- clips shot in \"slow shutter\" or \"night\" mode\n- clips where frame rate has been artificially increased by duplication of some frames\n- clips where in-camera noise reduction created frames with repeated noise"
                                 ,0x102);
  local_118 = (int *)((ulong)local_118 & 0xffffffffffffff00);
  FUN_00038ca4(uVar9,uVar17,
               "/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
               param_1 + 8,&local_108,&local_f0,0x5b,&local_118,0);
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1e40;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b1e40:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1e74;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b1e74:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
            param_1 + 8);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Repeated Frames Check",0,0xffffffff);
  local_f0 = (int *)FUN_002efa1c("Repeated Frames Check helps to find the optimal Repeat Rate using a visually enhanced preview (Assist Image).\nThe preview emphasizes the areas of the frame where noise reduction is insufficient.\n\nClick Begin to start the tuning process and then adjust Repeat Rate while watching the image in preview \nto find the Repeat Rate value that completely removes noise yet preserves details."
                                 ,0x17f);
  FUN_00038ca4(uVar9,uVar17,
               "/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
               param_1 + 0x10,&local_108,&local_f0,0x30,&DAT_00c944f8,3);
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1f0c;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b1f0c:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b1f40;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b1f40:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
            param_1 + 8);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Repeat Rate",0,0xffffffff);
  local_f0 = (int *)&DAT_0142f030;
  local_118 = (int *)FUN_002efa1c("Amount of repeated frames (or frames with repeated noise) in the current clip\n\nFor example: \n0% -> all frames are unique, no repeated frames \n20% -> each 5th frame repeats the preceding 4th frame \n50% -> every second frame repeats the preceding frame \n67% -> every second and third frames repeat the preceding frame \netc."
                                  ,0x141);
  local_120 = (int *)FUN_002efa1c("Amount of repeated frames (or frames with repeated noise) in the current clip\n\nFor example: \n0% -> all frames are unique, no repeated frames \n20% -> each 5th frame repeats the preceding 4th frame \n50% -> every second frame repeats the preceding frame \n67% -> every second and third frames repeat the preceding frame \netc."
                                  ,0x141);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Repeat Rate",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Amount of repeated frames (or frames with repeated noise) in the current clip\n\nFor example: \n0% -> all frames are unique, no repeated frames \n20% -> each 5th frame repeats the preceding 4th frame \n50% -> every second frame repeats the preceding frame \n67% -> every second and third frames repeat the preceding frame \netc."
                                   ,0x141);
  FUN_0003aa60(0x3f800000,0,uVar9,uVar17,
               "/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
               param_1 + 0x28,&local_108,0,&local_f0,&local_118,
               "/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
               param_1 + 0x18,&local_120,0x6400000000,0x1400000001,&DAT_0155b964,&local_128,
               "/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
               param_1 + 0x20,&local_130,0x200000058,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b2070;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b2070:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b20a4;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b20a4:
  if (*local_120 != -1) {
    if (*local_120 != 0) {
      LOAcquire();
      iVar5 = *local_120;
      *local_120 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b20d8;
    }
    FUN_00287df8(local_120,2,8);
  }
LAB_000b20d8:
  if (*local_118 != -1) {
    if (*local_118 != 0) {
      LOAcquire();
      iVar5 = *local_118;
      *local_118 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b210c;
    }
    FUN_00287df8(local_118,2,8);
  }
LAB_000b210c:
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2140;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b2140:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2174;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b2174:
  local_120 = &DAT_0142f0a8;
  local_108 = (int *)((ulong)local_108 & 0xffffffff00000000);
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  local_f0 = (int *)CONCAT44(local_f0._4_4_,10);
  FUN_000bf3ec(&local_120,&local_f0,&local_118);
  local_118 = (int *)CONCAT44(local_118._4_4_,0x1e);
  FUN_000bf3ec(&local_120,&local_118,&local_128);
  local_128 = (uint *)CONCAT44(local_128._4_4_,0x46);
  FUN_000bf3ec(&local_120,&local_128,&local_130);
  local_130 = (uint *)CONCAT44(local_130._4_4_,0x5a);
  FUN_000bf3ec(&local_120,&local_130,&local_150);
  local_108._0_4_ = 100;
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  local_108._0_4_ = 0x32;
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  local_108._0_4_ = 0x21;
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  local_108._0_4_ = 0x19;
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  local_108._0_4_ = 0x14;
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  local_108._0_4_ = 100;
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  local_108._0_4_ = 0x43;
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  local_108._0_4_ = 0x32;
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  local_108._0_4_ = 0x28;
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  local_108._0_4_ = 100;
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  local_108._0_4_ = 0x4b;
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  local_108._0_4_ = 0x3c;
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  local_108._0_4_ = 100;
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  local_108._0_4_ = 0x50;
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  local_108 = (int *)CONCAT44(local_108._4_4_,100);
  FUN_000bf3ec(&local_120,&local_108,&local_f0);
  uVar17 = FUN_003bbf58(&DAT_013bf1c0,
                        *(undefined8 *)
                         (
                         "/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices"
                         + param_1 + 0x18));
  FUN_000bca98(&local_138,&local_120);
  FUN_0002fb04(uVar17,&local_138);
  if (*local_138 != -1) {
    if (*local_138 != 0) {
      LOAcquire();
      iVar5 = *local_138;
      *local_138 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b23b4;
    }
    _free(local_138);
  }
LAB_000b23b4:
  local_108 = (int *)&DAT_0142f030;
  local_140 = (int *)&DAT_0142f030;
  FUN_0003c454(uVar9,*(undefined8 *)
                      ("/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
                      param_1 + 8),
               "/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
               param_1 + 0x30,&local_108,&local_140,0xffffffff,0x14);
  if (*local_140 != -1) {
    if (*local_140 != 0) {
      LOAcquire();
      iVar5 = *local_140;
      *local_140 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2414;
    }
    FUN_00287df8(local_140,2,8);
  }
LAB_000b2414:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2448;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b2448:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
            param_1 + 8);
  local_108 = (int *)FUN_002efa1c("Slow Shutter mode requires more input frames to work correctly.\nPlease increase Radius in Temporal Filter section."
                                  ,0x72);
  FUN_00039a7c(uVar9,uVar17,
               "/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
               param_1 + 0x40,&local_108,0xfffffffe,1,0);
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b24b8;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b24b8:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
            param_1 + 0x40);
  local_108 = (int *)FUN_002efa1c("Radius is too small for current settings",0x28);
  local_148 = (int *)FUN_002efa1c("Warning.png",0xb);
  FUN_0003c454(uVar9,uVar17,
               "/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
               param_1 + 0x38,&local_108,&local_148,0,0x14);
  if (*local_148 != -1) {
    if (*local_148 != 0) {
      LOAcquire();
      iVar5 = *local_148;
      *local_148 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b253c;
    }
    FUN_00287df8(local_148,2,8);
  }
LAB_000b253c:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2570;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b2570:
  FUN_0003c6d4(uVar9,*puVar8,0);
  uVar17 = *(undefined8 *)(param_1 + 0xce0);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Jitter of Details",0,0xffffffff);
  local_f0 = (int *)FUN_002efa1c("This filter reduces jitter of fine details (\"heat haze\" effect) caused by video compression or inaccurate profiling"
                                 ,0x73);
  local_118 = (int *)((ulong)local_118 & 0xffffffffffffff00);
  FUN_00038ca4(uVar9,uVar17,
               "/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
               param_1 + 0x48,&local_108,&local_f0,0x5b,&local_118,0);
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b261c;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b261c:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2650;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b2650:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
            param_1 + 0x48);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Threshold",0,0xffffffff);
  local_f0 = (int *)&DAT_0142f030;
  local_118 = (int *)FUN_002efa1c("Jitter filter\'s threshold determines where this filter is applied\n- too low values  -> jitter of fine details not sufficiently reduced, lower sharpness\n- too high values -> loss of fine details or unnatural residual noise"
                                  ,0xdd);
  local_128 = (uint *)FUN_002efa1c("Jitter filter\'s threshold determines where this filter is applied\n- too low values  -> jitter of fine details not sufficiently reduced, lower sharpness\n- too high values -> loss of fine details or unnatural residual noise"
                                   ,0xdd);
  FUN_003bbff0(&local_130,&DAT_013c41f8,"Jitter Threshold",0,0xffffffff);
  local_150 = (int *)FUN_002efa1c("Jitter filter\'s threshold determines where this filter is applied\n- too low values  -> jitter of fine details not sufficiently reduced, lower sharpness\n- too high values -> loss of fine details or unnatural residual noise"
                                  ,0xdd);
  FUN_0003aa60(0,0x40800000,uVar9,uVar17,0,&local_108,param_1 + 0x12c0,&local_f0,&local_118,
               (long)&dylib_command_000012b8.cmd + param_1,&local_128,0x12cffffff9c,0x1400000005,
               &DAT_0155b9b8,&local_130,0,&local_150,0x5c,0);
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2778;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b2778:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b27ac;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b27ac:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b27e0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b27e0:
  if (*local_118 != -1) {
    if (*local_118 != 0) {
      LOAcquire();
      iVar5 = *local_118;
      *local_118 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2814;
    }
    FUN_00287df8(local_118,2,8);
  }
LAB_000b2814:
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2848;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b2848:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b287c;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b287c:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices" +
            param_1 + 0x48);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Jitter Extended Filtration",0,0xffffffff);
  local_f0 = (int *)FUN_002efa1c("More aggressive reduction of jitter of fine details",0x33);
  uVar21 = 0;
  FUN_0003ad40(uVar9,uVar17,param_1 + 0x12c8,0,0,&DAT_0155b9b6,&local_108,&local_f0,0);
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2914;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b2914:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2948;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b2948:
  FUN_0003c6d4(uVar9,*puVar8,0);
  uVar17 = *(undefined8 *)(param_1 + 0xce0);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Dust and Scratches",0,0xffffffff);
  local_f0 = (int *)FUN_002efa1c("This filter removes dust, scratches, spots, etc.",0x30);
  puVar11 = (undefined8 *)(param_1 + 0xcf0);
  FUN_00038ca4(uVar9,uVar17,puVar11,&local_108,&local_f0,0x5f,&DAT_0155b951,0);
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b29ec;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000b29ec:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2a20;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b2a20:
  FUN_003bbff0(&local_f0,&DAT_013c41f8,"Normal",0,0xffffffff);
  FUN_003bbff0(&local_e8,&DAT_013c41f8,"Aggressive",0,0xffffffff);
  uVar17 = *puVar11;
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Mode",0,0xffffffff);
  local_118 = (int *)FUN_002efa1c("Mode determines general thoroughness of Dust and Scratches filter\n(in Aggressive mode, more image elements are considered dust and scratches)"
                                  ,0x8d);
  puVar12 = (undefined8 *)__Znwm(0x10);
  *puVar12 = &DAT_013b63f0;
  puVar12[1] = &DAT_0155b954;
  *(undefined8 **)
   ("/System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices" +
   param_1 + 0x10) = puVar12;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Dust and Scratches Mode",0,0xffffffff);
  FUN_0003bb84(uVar9,uVar17,&local_108,&local_118,
               "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices"
               + param_1 + 0x18,0,&local_f0,0,CONCAT44(uVar21,2),puVar12,&local_128,2);
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b2b50;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b2b50:
  if (*local_118 != -1) {
    if (*local_118 != 0) {
      LOAcquire();
      iVar5 = *local_118;
      *local_118 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2b84;
    }
    FUN_00287df8(local_118,2,8);
  }
LAB_000b2b84:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2bb8;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b2bb8:
  uVar17 = *(undefined8 *)(param_1 + 0xcf0);
  local_108 = (int *)FUN_002efa1c("Dust and Scratches filter requires more input frames to work correctly.\nPlease increase Radius in Temporal Filter section."
                                  ,0x7a);
  FUN_00039a7c(uVar9,uVar17,
               "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices"
               + param_1 + 0x20,&local_108,0xfffffffe,1,0);
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2c28;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b2c28:
  uVar17 = *(undefined8 *)
            (
            "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices"
            + param_1 + 0x20);
  local_108 = (int *)FUN_002efa1c("Radius is too small for current settings",0x28);
  local_158 = (int *)FUN_002efa1c("Warning.png",0xb);
  FUN_0003c454(uVar9,uVar17,
               "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices"
               + param_1 + 0x28,&local_108,&local_158,0,0x14);
  if (*local_158 != -1) {
    if (*local_158 != 0) {
      LOAcquire();
      iVar5 = *local_158;
      *local_158 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2cac;
    }
    FUN_00287df8(local_158,2,8);
  }
LAB_000b2cac:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2ce0;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b2ce0:
  uVar17 = *puVar11;
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Threshold",0,0xffffffff);
  local_118 = (int *)&DAT_0142f030;
  local_128 = (uint *)FUN_002efa1c("Threshold controls the aggressiveness of Dust and Scratches filter\n- higher values  -> more aggressive filtration, less attention to details\n- lower values  -> less aggressive filtration, better preserving details"
                                   ,0xd5);
  local_130 = (uint *)FUN_002efa1c("Threshold controls the aggressiveness of Dust and Scratches filter\n- higher values  -> more aggressive filtration, less attention to details\n- lower values  -> less aggressive filtration, better preserving details"
                                   ,0xd5);
  FUN_003bbff0(&local_150,&DAT_013c41f8,"Dust and Scratches Threshold",0,0xffffffff);
  local_160 = (int *)FUN_002efa1c("Threshold controls the aggressiveness of Dust and Scratches filter\n- higher values  -> more aggressive filtration, less attention to details\n- lower values  -> less aggressive filtration, better preserving details"
                                  ,0xd5);
  FUN_0003aa60(0,0x40a00000,uVar9,uVar17,0,&local_108,0,&local_118,&local_128,
               "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices"
               + param_1,&local_130,0x1f400000000,0x1400000005,&DAT_0155b958,&local_150,0,&local_160
               ,0x100000058,0);
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2e00;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b2e00:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2e34;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b2e34:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b2e68;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b2e68:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b2e9c;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b2e9c:
  if (*local_118 != -1) {
    if (*local_118 != 0) {
      LOAcquire();
      iVar5 = *local_118;
      *local_118 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2ed0;
    }
    FUN_00287df8(local_118,2,8);
  }
LAB_000b2ed0:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b2f04;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b2f04:
  uVar17 = *puVar11;
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Amount",0,0xffffffff);
  local_118 = (int *)&DAT_0142f030;
  local_128 = (uint *)FUN_002efa1c("Amount of reduction applied by this filter\n- high values -> more reduction of detected dust and scratches\n- low values  -> less reduction of detected dust and scratches"
                                   ,0xa8);
  local_130 = (uint *)FUN_002efa1c("Adjust amount of reduction applied by this filter\n- high values -> more reduction of detected dust and scratches\n- low values  -> less reduction of detected dust and scratches"
                                   ,0xaf);
  FUN_003bbff0(&local_150,&DAT_013c41f8,"Dust and Scratches Amount",0,0xffffffff);
  local_160 = (int *)FUN_002efa1c("Amount of reduction applied by this filter\n- high values -> more reduction of detected dust and scratches\n- low values  -> less reduction of detected dust and scratches"
                                  ,0xa8);
  FUN_0003aa60(0,0x3f800000,uVar9,uVar17,0,&local_108,0,&local_118,&local_128,
               "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices"
               + param_1 + 8,&local_130,0x6400000000,0x1400000005,&DAT_0155b9a4,&local_150,0,
               &local_160,0x10000005a,0);
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3024;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b3024:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3058;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b3058:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b308c;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b308c:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b30c0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b30c0:
  if (*local_118 != -1) {
    if (*local_118 != 0) {
      LOAcquire();
      iVar5 = *local_118;
      *local_118 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b30f4;
    }
    FUN_00287df8(local_118,2,8);
  }
LAB_000b30f4:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3128;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b3128:
  FUN_0003c6d4(uVar9,*puVar8,0);
  local_108 = (int *)&DAT_0142f030;
  FUN_00039a7c(uVar9,uVar10,
               "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices"
               + param_1 + 0x30,&local_108,0,0,1);
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3194;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b3194:
  uVar10 = *(undefined8 *)
            (
            "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices"
            + param_1 + 0x30);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Adapt to Changing Noise",0,0xffffffff);
  local_118 = (int *)FUN_002efa1c("Automatically adjust processing when clip\'s noise changes over time"
                                  ,0x43);
  uVar19 = 0x200000000;
  FUN_0003ad40(uVar9,uVar10,
               "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices"
               + param_1 + 0x40,
               "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices"
               + param_1 + 0x38,0,&DAT_0155b950,&local_108,&local_118,0x200000000);
  if (*local_118 != -1) {
    if (*local_118 != 0) {
      LOAcquire();
      iVar5 = *local_118;
      *local_118 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3238;
    }
    FUN_00287df8(local_118,2,8);
  }
LAB_000b3238:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b326c;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b326c:
  FUN_00014420(*(undefined8 *)
                (
                "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices"
                + param_1 + 0x38),1);
  uVar10 = *(undefined8 *)(*(long *)(param_1 + 0x170) + 0x50);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Spatial Settings",0,0xffffffff);
  FUN_00750130(uVar10,&local_108);
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b32e0;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b32e0:
  uVar17 = FUN_00850130(uVar10);
  FUN_003bbff0(&local_108,&DAT_013c41f8,"Spatial Filter",0,0xffffffff);
  local_118 = (int *)&DAT_0142f030;
  local_128 = (uint *)FUN_002efa1c("Enable Spatial Filter",0x15);
  puVar8 = (undefined8 *)(param_1 + 0xcf8);
  uVar19 = uVar19 & 0xffffffff00000000;
  FUN_00039518(uVar10,uVar17,puVar8,&local_108,&local_118,&local_128,0x5d,&DAT_0155b878,uVar19);
  uVar21 = (undefined4)(uVar19 >> 0x20);
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b338c;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b338c:
  if (*local_118 != -1) {
    if (*local_118 != 0) {
      LOAcquire();
      iVar5 = *local_118;
      *local_118 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b33c0;
    }
    FUN_00287df8(local_118,2,8);
  }
LAB_000b33c0:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b33f4;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000b33f4:
  local_108 = (int *)FUN_002efa1c("Normal",6);
  local_100 = (int *)FUN_002efa1c("High",4);
  local_f8 = (int *)FUN_002efa1c("Highest",7);
  local_118 = (int *)FUN_002efa1c("Normal (Faster)",0xf);
  local_110 = (int *)FUN_002efa1c("High (Slower)",0xd);
  if (1 < DAT_0155b97c) {
    DAT_0155b97c = 1;
  }
  uVar17 = *(undefined8 *)(param_1 + 0xcf8);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Filter Tuning Assist",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Tuning Assist helps to find the optimal values of filter settings using a visually enhanced preview (Assist Image).\nThe preview emphasizes the unfiltered parts of the frame to make them easier to see.\n\nClick Begin to start the tuning process and then adjust the noise levels and other filter settings while watching \nthe image in preview to find the settings that sufficiently remove noise yet preserve details."
                                   ,0x19b);
  FUN_00038ca4(uVar10,uVar17,
               "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices"
               + param_1 + 0x48,&local_128,&local_130,0x31,&DAT_00c944f8,4);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b350c;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b350c:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b3540;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b3540:
  FUN_0003c6d4(uVar10,*puVar8,0);
  local_128 = &DAT_0142f030;
  FUN_00039a7c(uVar10,*(undefined8 *)(param_1 + 0xcf8),
               "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices"
               + param_1 + 0x50,&local_128,0,0,0);
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b35b0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b35b0:
  uVar17 = *(undefined8 *)
            (
            "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices"
            + param_1 + 0x50);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Quality Mode",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Select Quality Mode of Spatial Filter\nHigh quality mode can deliver better visual results at the expense of lower processing speed"
                                   ,0x82);
  puVar11 = (undefined8 *)__Znwm(0x10);
  *puVar11 = &DAT_013b6430;
  puVar11[1] = &DAT_0155b97c;
  *(undefined8 **)((long)&dylib_command_00001328.dylib.name.offset + param_1) = puVar11;
  FUN_003bbff0(&local_150,&DAT_013c41f8,"Spatial Quality Mode",0,0xffffffff);
  FUN_0003bb84(uVar10,uVar17,&local_128,&local_130,(long)&dylib_command_00001328.cmd + param_1,0,
               &local_108,&local_118,CONCAT44(uVar21,2),puVar11,&local_150,0xc00000002);
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b36a4;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b36a4:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b36d8;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b36d8:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b370c;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b370c:
  FUN_0003c6d4(uVar10,*puVar8,0);
  uVar17 = *(undefined8 *)(param_1 + 0xcf8);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Noise Reduction Amount",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Adjust amount of noise reduction applied to frequency and channel components\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                   ,0xb0);
  local_150 = (int *)((ulong)local_150 & 0xffffffffffffff00);
  FUN_00038ca4(uVar10,uVar17,param_1 + 0xd00,&local_128,&local_130,0x5b,&local_150,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b37b0;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b37b0:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b37e4;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b37e4:
  uVar17 = *(undefined8 *)(param_1 + 0xd00);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Luminance",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("ChannelY.png",0xc);
  local_150 = (int *)FUN_002efa1c("Amount of noise reduction applied to Luminance channel component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0xa4);
  local_160 = (int *)FUN_002efa1c("Adjust amount of noise reduction applied to Luminance channel component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0xab);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Luminance Reduction Amount",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Amount of noise reduction applied to Luminance channel component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0xa4);
  FUN_0003aa60(0,0x3f800000,uVar10,uVar17,
               "/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation" +
               param_1 + 0x18,&local_128,
               "/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation" +
               param_1 + 0x30,&local_130,&local_150,param_1 + 0xd18,&local_160,0x6400000000,
               0xa00000005,&DAT_0155b8b4,&local_168,(long)&dylib_command_00001388.cmd + param_1,
               &local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3920;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b3920:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3954;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b3954:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3988;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b3988:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b39bc;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b39bc:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b39f0;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b39f0:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b3a24;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b3a24:
  uVar17 = *(undefined8 *)(param_1 + 0xd00);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Chrominance",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Adjust amount of noise reduction applied to chrominance channel components\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                   ,0xae);
  local_150 = (int *)((ulong)local_150 & 0xffffffffffffff00);
  FUN_00038ca4(uVar10,uVar17,param_1 + 0x1338,&local_128,&local_130,0x5b,&local_150,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b3abc;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b3abc:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b3af0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b3af0:
  uVar17 = *(undefined8 *)((long)&dylib_command_00001328.dylib.current_version + param_1);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Cr",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("ChannelCr.png",0xd);
  local_150 = (int *)FUN_002efa1c("Amount of noise reduction applied to Cr channel component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0x9d);
  local_160 = (int *)FUN_002efa1c("Adjust amount of noise reduction applied to Cr channel component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0xa4);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Cr Reduction Amount",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Amount of noise reduction applied to Cr channel component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0x9d);
  FUN_0003aa60(0,0x3f800000,uVar10,uVar17,
               "/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation" +
               param_1 + 0x20,&local_128,
               "/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation" +
               param_1 + 0x38,&local_130,&local_150,
               "/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation" + param_1 + 8
               ,&local_160,0x6400000000,0xa00000005,&DAT_0155b8b8,&local_168,param_1 + 0x1390,
               &local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3c30;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b3c30:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3c64;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b3c64:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3c98;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b3c98:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3ccc;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b3ccc:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b3d00;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b3d00:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b3d34;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b3d34:
  uVar17 = *(undefined8 *)((long)&dylib_command_00001328.dylib.current_version + param_1);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Cb",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("ChannelCb.png",0xd);
  local_150 = (int *)FUN_002efa1c("Amount of noise reduction applied to Cb channel component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0x9d);
  local_160 = (int *)FUN_002efa1c("Adjust amount of noise reduction applied to Cb channel component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0xa4);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Cb Reduction Amount",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Amount of noise reduction applied to Cb channel component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0x9d);
  FUN_0003aa60(0,0x3f800000,uVar10,uVar17,
               "/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation" +
               param_1 + 0x28,&local_128,
               "/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation" +
               param_1 + 0x40,&local_130,&local_150,
               "/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation" +
               param_1 + 0x10,&local_160,0x6400000000,0xa00000005,&DAT_0155b8bc,&local_168,
               param_1 + 0x1398,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3e74;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b3e74:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3ea8;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b3ea8:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3edc;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b3edc:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b3f10;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b3f10:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b3f44;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b3f44:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b3f78;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b3f78:
  uVar17 = *(undefined8 *)(param_1 + 0xd00);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Frequencies",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Adjust amount of noise reduction applied to frequency components\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                   ,0xa4);
  pcVar1 = "/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation" + param_1;
  local_150 = (int *)((ulong)local_150 & 0xffffffffffffff00);
  FUN_00038ca4(uVar10,uVar17,pcVar1,&local_128,&local_130,0x5b,&local_150,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b4014;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b4014:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b4048;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b4048:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"High",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("HFImage.png",0xb);
  local_150 = (int *)FUN_002efa1c("Example of high-frequency noise / fine grain",0x2c);
  local_160 = (int *)FUN_002efa1c("Adjust amount of noise reduction applied to high-frequency component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0xa8);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"High Freq Reduction Amount",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Amount of noise reduction applied to high-frequency component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0xa1);
  FUN_0003aa60(0,0x3f800000,uVar10,uVar17,0,&local_128,
               "/System/Library/Frameworks/IOSurface.framework/Versions/A/IOSurface" +
               param_1 + 0x28,&local_130,&local_150,
               "/System/Library/Frameworks/IOSurface.framework/Versions/A/IOSurface" + param_1,
               &local_160,0x6400000000,0xa00000005,&DAT_0155b894,&local_168,0,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b417c;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b417c:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b41b0;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b41b0:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b41e4;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b41e4:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4218;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b4218:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b424c;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b424c:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b4280;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b4280:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Mid",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("MFImage.png",0xb);
  local_150 = (int *)FUN_002efa1c("Example of medium-frequency noise / medium-size grain",0x35);
  local_160 = (int *)FUN_002efa1c("Adjust amount of noise reduction applied to medium-frequency component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0xaa);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Mid Freq Reduction Amount",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Amount of noise reduction applied to medium-frequency component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0xa3);
  FUN_0003aa60(0,0x3f800000,uVar10,uVar17,0,&local_128,
               "/System/Library/Frameworks/IOSurface.framework/Versions/A/IOSurface" +
               param_1 + 0x30,&local_130,&local_150,
               "/System/Library/Frameworks/IOSurface.framework/Versions/A/IOSurface" + param_1 + 8,
               &local_160,0x6400000000,0xa00000005,&DAT_0155b898,&local_168,0,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b43b4;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b43b4:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b43e8;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b43e8:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b441c;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b441c:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4450;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b4450:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b4484;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b4484:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b44b8;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b44b8:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Low",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("LFImage.png",0xb);
  local_150 = (int *)FUN_002efa1c("Example of low-frequency noise / large-size grain",0x31);
  local_160 = (int *)FUN_002efa1c("Adjust amount of noise reduction applied to low-frequency component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0xa7);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Low Freq Reduction Amount",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Amount of noise reduction applied to low-frequency component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0xa0);
  FUN_0003aa60(0,0x3f800000,uVar10,uVar17,0,&local_128,
               "/System/Library/Frameworks/IOSurface.framework/Versions/A/IOSurface" +
               param_1 + 0x38,&local_130,&local_150,
               "/System/Library/Frameworks/IOSurface.framework/Versions/A/IOSurface" +
               param_1 + 0x10,&local_160,0x6400000000,0xa00000005,&DAT_0155b89c,&local_168,0,
               &local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b45ec;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b45ec:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4620;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b4620:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4654;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b4654:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4688;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b4688:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b46bc;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b46bc:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b46f0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b46f0:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Very Low",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("VLFImage.png",0xc);
  local_150 = (int *)FUN_002efa1c("Example of very low-frequency noise / very large-size grain",0x3b
                                 );
  local_160 = (int *)FUN_002efa1c("Adjust amount of noise reduction applied to very low-frequency component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0xac);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Very Low Freq Reduction Amount",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Amount of noise reduction applied to very low-frequency component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0xa5);
  FUN_0003aa60(0,0x3f800000,uVar10,uVar17,0,&local_128,0,&local_130,&local_150,
               "/System/Library/Frameworks/IOSurface.framework/Versions/A/IOSurface" +
               param_1 + 0x18,&local_160,0x6400000000,0xa00000005,&DAT_0155b8a0,&local_168,0,
               &local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b481c;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b481c:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4850;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b4850:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4884;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b4884:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b48b8;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b48b8:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b48ec;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b48ec:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b4920;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b4920:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Ultra Low",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("ULFImage.png",0xc);
  local_150 = (int *)FUN_002efa1c("Example of ultra low-frequency noise / ultra large-size grain",
                                  0x3d);
  local_160 = (int *)FUN_002efa1c("Adjust amount of noise reduction applied to ultra low-frequency component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0xad);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Ultra Low Freq Reduction Amount",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Amount of noise reduction applied to ultra low-frequency component\n- high values -> more reduction of detected noise\n- low values  -> less reduction of detected noise"
                                  ,0xa6);
  ppiVar20 = &local_170;
  uVar21 = 100;
  FUN_0003aa60(0,0x3f800000,uVar10,uVar17,0,&local_128,0,&local_130,&local_150,
               "/System/Library/Frameworks/IOSurface.framework/Versions/A/IOSurface" +
               param_1 + 0x20,&local_160,0x6400000000,0xa00000005,&DAT_0155b8a4,&local_168,0,
               ppiVar20,0x5a,0);
  uVar22 = (undefined4)((ulong)ppiVar20 >> 0x20);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4a4c;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b4a4c:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4a80;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b4a80:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4ab4;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b4ab4:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4ae8;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b4ae8:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b4b1c;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b4b1c:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b4b50;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b4b50:
  FUN_0003c6d4(uVar10,*puVar8,0);
  uVar17 = *(undefined8 *)(param_1 + 0xcf8);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Noise Level",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Adjust estimations of noise levels in frequency and channel components (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                   ,0xe0);
  local_150 = (int *)((ulong)local_150 & 0xffffffffffffff00);
  FUN_00038ca4(uVar10,uVar17,(undefined8 *)(param_1 + 0xd08),&local_128,&local_130,0x5b,&local_150,0
              );
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b4bf8;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b4bf8:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b4c2c;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b4c2c:
  uVar17 = *(undefined8 *)(param_1 + 0xd08);
  local_130 = &DAT_0142f030;
  local_128 = &DAT_0142f030;
  local_150 = (int *)FUN_002efa1c("Adjust overall estimation of noise level (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xc2);
  FUN_003bbff0(&local_160,&DAT_013c41f8,"Overall Noise Level",0,0xffffffff);
  local_168 = (int *)FUN_002efa1c("Estimation of overall noise level (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xbb);
  FUN_0003abb8(0,0x40200000,uVar10,uVar17,0,&local_128,&local_130,param_1 + 0xd20,&local_150,
               0xffffff9c,0x500000096,CONCAT44(uVar21,0x14),&DAT_0155b87c,&local_160,0,&local_168,
               CONCAT44(uVar22,0x12));
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4d0c;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b4d0c:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4d40;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b4d40:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4d74;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b4d74:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b4da8;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b4da8:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b4ddc;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b4ddc:
  uVar17 = *(undefined8 *)(param_1 + 0xd08);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Luminance",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("ChannelY.png",0xc);
  local_150 = (int *)FUN_002efa1c("Estimation of noise level in Luminance channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd2);
  local_160 = (int *)FUN_002efa1c("Adjust estimation of noise level in Luminance channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd9);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Luminance Noise Level",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Estimation of noise level in Luminance channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd2);
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,0,&local_128,
               "/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo" +
               param_1 + 0x10,&local_130,&local_150,(long)&dylib_command_000013e8.cmd + param_1,
               &local_160,0x96ffffff9c,0x1400000005,&DAT_0155b8a8,&local_168,0,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4f18;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b4f18:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4f4c;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b4f4c:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4f80;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b4f80:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b4fb4;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b4fb4:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b4fe8;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b4fe8:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b501c;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b501c:
  uVar17 = *(undefined8 *)(param_1 + 0xd08);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Cr",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("ChannelCr.png",0xd);
  local_150 = (int *)FUN_002efa1c("Estimation of noise level in Cr channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xcb);
  local_160 = (int *)FUN_002efa1c("Adjust estimation of noise level in Cr channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd2);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Cr Noise Level",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Estimation of noise level in Cr channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xcb);
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,
               "/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo" + param_1,
               &local_128,
               "/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo" +
               param_1 + 0x18,&local_130,&local_150,param_1 + 0x13f0,&local_160,0x96ffffff9c,
               0x1400000005,&DAT_0155b8ac,&local_168,
               "/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo" +
               param_1 + 0x28,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5164;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b5164:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5198;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b5198:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b51cc;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b51cc:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5200;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b5200:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b5234;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b5234:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b5268;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b5268:
  uVar17 = *(undefined8 *)(param_1 + 0xd08);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Cb",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("ChannelCb.png",0xd);
  local_150 = (int *)FUN_002efa1c("Estimation of noise level in Cb channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xcb);
  local_160 = (int *)FUN_002efa1c("Adjust estimation of noise level in Cb channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd2);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Cb Noise Level",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Estimation of noise level in Cb channel component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xcb);
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,
               "/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo" + param_1 + 8,
               &local_128,
               "/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo" +
               param_1 + 0x20,&local_130,&local_150,param_1 + 0x13f8,&local_160,0x96ffffff9c,
               0x1400000005,&DAT_0155b8b0,&local_168,
               "/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo" +
               param_1 + 0x30,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b53b0;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b53b0:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b53e4;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b53e4:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5418;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b5418:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b544c;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b544c:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b5480;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b5480:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b54b4;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b54b4:
  uVar17 = *(undefined8 *)(param_1 + 0xd08);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Frequencies",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Adjust estimations of noise levels in frequency components (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                   ,0xd4);
  pcVar1 = "/System/Library/Frameworks/IOSurface.framework/Versions/A/IOSurface" + param_1 + 0x40;
  local_150 = (int *)((ulong)local_150 & 0xffffffffffffff00);
  FUN_00038ca4(uVar10,uVar17,pcVar1,&local_128,&local_130,0x5b,&local_150,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b5550;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b5550:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b5584;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b5584:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"High",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("HFImage.png",0xb);
  local_150 = (int *)FUN_002efa1c("Example of high-frequency noise / fine grain",0x2c);
  local_160 = (int *)FUN_002efa1c("Adjust estimation of noise level in high-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd6);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"High Freq Noise Level",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Estimation of noise level in high-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xcf);
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,0,&local_128,
               "/System/Library/Frameworks/ImageIO.framework/Versions/A/ImageIO" + param_1,
               &local_130,&local_150,
               "/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo" +
               param_1 + 0x38,&local_160,0x96ffffff9c,0x1400000005,&DAT_0155b880,&local_168,0,
               &local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b56c0;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b56c0:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b56f4;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b56f4:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5728;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b5728:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b575c;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b575c:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b5790;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b5790:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b57c4;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b57c4:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Mid",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("MFImage.png",0xb);
  local_150 = (int *)FUN_002efa1c("Example of medium-frequency noise / medium-size grain",0x35);
  local_160 = (int *)FUN_002efa1c("Adjust estimation of noise level in medium-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd8);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Mid Freq Noise Level",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Estimation of noise level in medium-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd1);
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,0,&local_128,
               "/System/Library/Frameworks/ImageIO.framework/Versions/A/ImageIO" + param_1 + 8,
               &local_130,&local_150,
               "/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo" +
               param_1 + 0x40,&local_160,0x96ffffff9c,0x1400000005,&DAT_0155b884,&local_168,0,
               &local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5900;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b5900:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5934;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b5934:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5968;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b5968:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b599c;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b599c:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b59d0;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b59d0:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b5a04;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b5a04:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Low",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("LFImage.png",0xb);
  local_150 = (int *)FUN_002efa1c("Example of low-frequency noise / large-size grain",0x31);
  local_160 = (int *)FUN_002efa1c("Adjust estimation of noise level in low-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd5);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Low Freq Noise Level",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Estimation of noise level in low-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xce);
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,0,&local_128,
               "/System/Library/Frameworks/ImageIO.framework/Versions/A/ImageIO" + param_1 + 0x10,
               &local_130,&local_150,(long)&dylib_command_00001448.cmd + param_1,&local_160,
               0x96ffffff9c,0x1400000005,&DAT_0155b888,&local_168,0,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5b40;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b5b40:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5b74;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b5b74:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5ba8;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b5ba8:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5bdc;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b5bdc:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b5c10;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b5c10:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b5c44;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b5c44:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Very Low",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("VLFImage.png",0xc);
  local_150 = (int *)FUN_002efa1c("Example of very low-frequency noise / very large-size grain",0x3b
                                 );
  local_160 = (int *)FUN_002efa1c("Adjust estimation of noise level in very low-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xda);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Very Low Freq Noise Level",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Estimation of noise level in very low-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd3);
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,0,&local_128,0,&local_130,&local_150,param_1 + 0x1450,
               &local_160,0x96ffffff9c,0x1400000005,&DAT_0155b88c,&local_168,0,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5d78;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b5d78:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5dac;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b5dac:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5de0;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b5de0:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5e14;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b5e14:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b5e48;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b5e48:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b5e7c;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b5e7c:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Ultra Low",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("ULFImage.png",0xc);
  local_150 = (int *)FUN_002efa1c("Example of ultra low-frequency noise / ultra large-size grain",
                                  0x3d);
  local_160 = (int *)FUN_002efa1c("Adjust estimation of noise level in ultra low-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xdb);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Ultra Low Freq Noise Level",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Estimation of noise level in ultra low-frequency component (relative to noise profile)\n- positive values  -> more image elements are considered noise\n- negative values -> fewer image elements are considered noise"
                                  ,0xd4);
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,0,&local_128,0,&local_130,&local_150,param_1 + 0x1458,
               &local_160,0x96ffffff9c,0x1400000005,&DAT_0155b890,&local_168,0,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5fb0;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b5fb0:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b5fe4;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b5fe4:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6018;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b6018:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b604c;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b604c:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b6080;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b6080:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b60b4;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b60b4:
  FUN_0003c6d4(uVar10,*puVar8,0);
  uVar17 = *(undefined8 *)(param_1 + 0xcf8);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Artifact Removal",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Remove unwanted digital artifacts (dots and lines) introduced by compression and similar post-processing"
                                   ,0x68);
  pcVar1 = "/System/Library/Frameworks/ImageIO.framework/Versions/A/ImageIO" + param_1 + 0x18;
  FUN_00038ca4(uVar10,uVar17,pcVar1,&local_128,&local_130,0x5f,&DAT_0155b8c0,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b615c;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b615c:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b6190;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b6190:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Dots",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("SpeckDot.png",0xc);
  local_150 = (int *)FUN_002efa1c("Example of dots removed by artifact filter",0x2a);
  local_160 = (int *)FUN_002efa1c("Adjust dot removal threshold (relative to noise profile)\n- positive values  -> more dots are considered noise\n- negative values -> fewer dots are considered noise"
                                  ,0xa2);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Dots Removal Threshold",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Dot removal threshold (relative to noise profile)\n- positive values  -> more dots are considered noise\n- negative values -> fewer dots are considered noise"
                                  ,0x9b);
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,0,&local_128,0,&local_130,&local_150,
               "/System/Library/Frameworks/ImageIO.framework/Versions/A/ImageIO" + param_1 + 0x28,
               &local_160,0x96ffffff9c,0x1400000005,&DAT_0155b8c4,&local_168,0,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b62c4;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b62c4:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b62f8;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b62f8:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b632c;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b632c:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6360;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b6360:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b6394;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b6394:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b63c8;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b63c8:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Lines",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("SpeckLine.png",0xd);
  local_150 = (int *)FUN_002efa1c("Example of lines removed by artifact filter",0x2b);
  local_160 = (int *)FUN_002efa1c("Adjust line removal threshold (relative to noise profile)\n- positive values  -> more lines are considered noise\n- negative values -> fewer lines are considered noise"
                                  ,0xa5);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Lines Removal Threshold",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Line removal threshold (relative to noise profile)\n- positive values  -> more lines are considered noise\n- negative values -> fewer lines are considered noise"
                                  ,0x9e);
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,0,&local_128,0,&local_130,&local_150,
               "/System/Library/Frameworks/ImageIO.framework/Versions/A/ImageIO" + param_1 + 0x30,
               &local_160,0x96ffffff9c,0x1400000005,&DAT_0155b8c8,&local_168,0,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b64fc;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b64fc:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6530;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b6530:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6564;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b6564:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6598;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b6598:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b65cc;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b65cc:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b6600;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b6600:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/ImageIO.framework/Versions/A/ImageIO" + param_1 + 0x18);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Frequencies",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Adjust settings for individual frequency components",0x33);
  pcVar1 = "/System/Library/Frameworks/ImageIO.framework/Versions/A/ImageIO" + param_1 + 0x20;
  local_150 = (int *)((ulong)local_150 & 0xffffffffffffff00);
  FUN_00038ca4(uVar10,uVar17,pcVar1,&local_128,&local_130,0x5b,&local_150,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b669c;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b669c:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b66d0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b66d0:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"High",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("SpeckHF.png",0xb);
  local_150 = (int *)FUN_002efa1c("Example of high-frequency lines and dots removed by artifact filter"
                                  ,0x43);
  local_160 = (int *)FUN_002efa1c("Adjust artifact removal threshold in high-frequency component (relative to noise profile)\n- positive values  -> more dots and lines are considered noise\n- negative values -> fewer dots and lines are considered noise"
                                  ,0xd7);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"High Freq Artifact Removal Threshold",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Artifact removal threshold in high-frequency component (relative to noise profile)\n- positive values  -> more dots and lines are considered noise\n- negative values -> fewer dots and lines are considered noise"
                                  ,0xd0);
  puVar11 = (undefined8 *)((long)&dylib_command_000014a0.dylib.current_version + param_1);
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,0,&local_128,puVar11,&local_130,&local_150,
               "/System/Library/Frameworks/ImageIO.framework/Versions/A/ImageIO" + param_1 + 0x38,
               &local_160,0x96ffffff9c,0x1400000005,&DAT_0155b8cc,&local_168,0,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b680c;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b680c:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6840;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b6840:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6874;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b6874:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b68a8;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b68a8:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b68dc;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b68dc:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b6910;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b6910:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Mid",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("SpeckMF.png",0xb);
  local_150 = (int *)FUN_002efa1c("Example of medium-frequency lines and dots removed by artifact filter"
                                  ,0x45);
  local_160 = (int *)FUN_002efa1c("Adjust artifact removal threshold in medium-frequency component (relative to noise profile)\n- positive values  -> more dots and lines are considered noise\n- negative values -> fewer dots and lines are considered noise"
                                  ,0xd9);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Mid Freq Artifact Removal Threshold",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Artifact removal threshold in medium-frequency component (relative to noise profile)\n- positive values  -> more dots and lines are considered noise\n- negative values -> fewer dots and lines are considered noise"
                                  ,0xd2);
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,0,&local_128,
               "/System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics" + param_1
               ,&local_130,&local_150,(long)&dylib_command_000014a0.cmd + param_1,&local_160,
               0x96ffffff9c,0x1400000005,&DAT_0155b8d0,&local_168,0,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6a4c;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b6a4c:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6a80;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b6a80:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6ab4;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b6ab4:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6ae8;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b6ae8:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b6b1c;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b6b1c:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b6b50;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b6b50:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Low",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("SpeckLF.png",0xb);
  local_150 = (int *)FUN_002efa1c("Example of low-frequency lines and dots removed by artifact filter"
                                  ,0x42);
  local_160 = (int *)FUN_002efa1c("Adjust artifact removal threshold in low-frequency component (relative to noise profile)\n- positive values  -> more dots and lines are considered noise\n- negative values -> fewer dots and lines are considered noise"
                                  ,0xd6);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Low Freq Artifact Removal Threshold",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Artifact removal threshold in low-frequency component (relative to noise profile)\n- positive values  -> more dots and lines are considered noise\n- negative values -> fewer dots and lines are considered noise"
                                  ,0xcf);
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,0,&local_128,
               "/System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics" +
               param_1 + 8,&local_130,&local_150,param_1 + 0x14a8,&local_160,0x96ffffff9c,
               0x1400000005,&DAT_0155b8d4,&local_168,0,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6c8c;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b6c8c:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6cc0;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b6cc0:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6cf4;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b6cf4:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6d28;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b6d28:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b6d5c;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b6d5c:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b6d90;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b6d90:
  FUN_0003c6d4(uVar10,*puVar8,0);
  uVar17 = *(undefined8 *)(param_1 + 0xcf8);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Edge Smoothing",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Make edges look smoother (may also smooth some fine details).\nExpand to adjust strength of edge smoothing applied to individual frequency components"
                                   ,0x94);
  pcVar1 = "/System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics" +
           param_1 + 0x10;
  FUN_00038ca4(uVar10,uVar17,pcVar1,&local_128,&local_130,0x5f,&DAT_0155b8d8,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b6e38;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b6e38:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b6e6c;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b6e6c:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"High",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("SmEdgesHF.png",0xd);
  local_150 = (int *)FUN_002efa1c("Example of smoothed high-frequency edges",0x28);
  local_160 = (int *)FUN_002efa1c("Adjust strength of edge smoothing in high-frequency component\n- high values -> more smoothing of edges\n- low values  -> less smoothing, keep edges rough"
                                  ,0x98);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"High Freq Edge Smoothing Threshold",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Strength of edge smoothing in high-frequency component\n- high values -> more smoothing of edges\n- low values  -> less smoothing, keep edges rough"
                                  ,0x91);
  FUN_0003aa60(0,0x3f800000,uVar10,uVar17,0,&local_128,
               "/System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics" +
               param_1 + 0x30,&local_130,&local_150,
               "/System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics" +
               param_1 + 0x18,&local_160,0x6400000000,0xa00000005,&DAT_0155b8dc,&local_168,0,
               &local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6fa0;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b6fa0:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b6fd4;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b6fd4:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7008;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b7008:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b703c;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b703c:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b7070;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b7070:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b70a4;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b70a4:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Mid",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("SmEdgesMF.png",0xd);
  local_150 = (int *)FUN_002efa1c("Example of smoothed medium-frequency edges",0x2a);
  local_160 = (int *)FUN_002efa1c("Adjust strength of edge smoothing in medium-frequency component\n- high values -> more smoothing of edges\n- low values  -> less smoothing, keep edges rough"
                                  ,0x9a);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Mid Freq Edge Smoothing Threshold",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Strength of edge smoothing in medium-frequency component\n- high values -> more smoothing of edges\n- low values  -> less smoothing, keep edges rough"
                                  ,0x93);
  FUN_0003aa60(0,0x3f800000,uVar10,uVar17,0,&local_128,
               "/System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics" +
               param_1 + 0x38,&local_130,&local_150,
               "/System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics" +
               param_1 + 0x20,&local_160,0x6400000000,0xa00000005,&DAT_0155b8e0,&local_168,0,
               &local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b71d8;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b71d8:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b720c;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b720c:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7240;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b7240:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7274;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b7274:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b72a8;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b72a8:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b72dc;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b72dc:
  uVar17 = *(undefined8 *)pcVar1;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Low",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("SmEdgesLF.png",0xd);
  local_150 = (int *)FUN_002efa1c("Example of smoothed low-frequency edges",0x27);
  local_160 = (int *)FUN_002efa1c("Adjust strength of edge smoothing in low-frequency component\n- high values -> more smoothing of edges\n- low values  -> less smoothing, keep edges rough"
                                  ,0x97);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Low Freq Edge Smoothing Threshold",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Strength of edge smoothing in low-frequency component\n- high values -> more smoothing of edges\n- low values  -> less smoothing, keep edges rough"
                                  ,0x90);
  ppiVar20 = &local_170;
  uVar21 = 100;
  FUN_0003aa60(0,0x3f800000,uVar10,uVar17,0,&local_128,
               "/System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics" +
               param_1 + 0x40,&local_130,&local_150,
               "/System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics" +
               param_1 + 0x28,&local_160,0x6400000000,0xa00000005,&DAT_0155b8e4,&local_168,0,
               ppiVar20,0x5a,0);
  uVar22 = (undefined4)((ulong)ppiVar20 >> 0x20);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7410;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b7410:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7444;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b7444:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7478;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b7478:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b74ac;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b74ac:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b74e0;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b74e0:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b7514;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b7514:
  FUN_0003c6d4(uVar10,*puVar8,0);
  FUN_00a81628(*(undefined8 *)(DAT_0141bb80 + 0x4b1520),&DAT_01538c10);
  uVar17 = *(undefined8 *)(param_1 + 0xcf8);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Sharpening",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Sharpen image after noise reduction. Expand to adjust sharpening parameters"
                                   ,0x4b);
  FUN_00038ca4(uVar10,uVar17,param_1 + 0xd10,&local_128,&local_130,0x5f,&DAT_0155b908,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b75d4;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b75d4:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b7608;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b7608:
  uVar17 = *(undefined8 *)(param_1 + 0xd10);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"High",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Amount of sharpening applied to high-frequency component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                   ,0x7a);
  FUN_00038ca4(uVar10,uVar17,(long)&dylib_command_00001568.cmd + param_1,&local_128,&local_130,0x5b,
               &DAT_0155b908,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b769c;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b769c:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b76d0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b76d0:
  uVar17 = *(undefined8 *)((long)&dylib_command_00001568.cmd + param_1);
  local_128 = (uint *)FUN_002efa1c("HFSSImage.png",0xd);
  local_130 = (uint *)FUN_002efa1c("Example of sharpened high-frequency details",0x2b);
  local_150 = (int *)FUN_002efa1c("Adjust amount of sharpening applied to high-frequency component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                  ,0x81);
  FUN_003bbff0(&local_160,&DAT_013c41f8,"High Freq Sharpening Amount",0,0xffffffff);
  local_168 = (int *)FUN_002efa1c("Amount of sharpening applied to high-frequency component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                  ,0x7a);
  puVar8 = (undefined8 *)((long)&dylib_command_000015d0.dylib.current_version + param_1);
  FUN_0003abb8(0,0x40200000,uVar10,uVar17,puVar8,&local_128,&local_130,param_1 + 0xd28,&local_150,0,
               0x5000000fa,CONCAT44(uVar21,0x14),&DAT_0155b918,&local_160,0,&local_168,
               CONCAT44(uVar22,0x12));
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b77dc;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b77dc:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7810;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b7810:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7844;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b7844:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b7878;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b7878:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b78ac;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b78ac:
  uVar17 = *(undefined8 *)((long)&dylib_command_00001568.cmd + param_1);
  local_128 = (uint *)FUN_002efa1c("Example of sharpened high-frequency details",0x2b);
  FUN_00039a7c(uVar10,uVar17,
               "/System/Library/Frameworks/CFNetwork.framework/Versions/A/CFNetwork" +
               param_1 + 0x10,&local_128,0,1,0);
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b7920;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b7920:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/CFNetwork.framework/Versions/A/CFNetwork" + param_1 + 0x10)
  ;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Range",0,0xffffffff);
  local_130 = &DAT_0142f030;
  local_150 = (int *)&DAT_0142f030;
  local_160 = (int *)FUN_002efa1c("Adjustment of the frequency range to be sharpened:\n-1 - sharpening is applied to narrower frequency range\n 0 - sharpening is applied to normal frequency range of high-frequency component (default)\n+1 - sharpening is applied to wider frequency range"
                                  ,0xf8);
  puVar12 = (undefined8 *)__Znwm(0x10);
  *puVar12 = &DAT_013b6470;
  puVar12[1] = &DAT_0155b9cc;
  *(undefined8 **)
   ("/System/Library/Frameworks/CFNetwork.framework/Versions/A/CFNetwork" + param_1 + 0x40) =
       puVar12;
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Sharpening High Freq Range",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Adjustment of the frequency range to be sharpened:\n-1 - sharpening is applied to narrower frequency range\n 0 - sharpening is applied to normal frequency range of high-frequency component (default)\n+1 - sharpening is applied to wider frequency range"
                                  ,0xf8);
  ppiVar20 = &local_170;
  uVar21 = 1;
  FUN_0003ab5c(0,0x40000000,uVar10,uVar17,0,&local_128,0,&local_130,&local_150,
               "/System/Library/Frameworks/CFNetwork.framework/Versions/A/CFNetwork" +
               param_1 + 0x28,&local_160,0x1ffffffff,0x100000001,puVar12,&local_168,0,ppiVar20,0x48,
               0);
  uVar22 = (undefined4)((ulong)ppiVar20 >> 0x20);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7a48;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b7a48:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7a7c;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b7a7c:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7ab0;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b7ab0:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7ae4;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b7ae4:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b7b18;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b7b18:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b7b4c;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b7b4c:
  uVar17 = *(undefined8 *)(param_1 + 0xd10);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Mid",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Amount of sharpening applied to medium-frequency component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                   ,0x7c);
  FUN_00038ca4(uVar10,uVar17,param_1 + 0x1570,&local_128,&local_130,0x5b,&DAT_0155b908,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b7be0;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b7be0:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b7c14;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b7c14:
  uVar17 = *(undefined8 *)((long)&dylib_command_00001568.dylib.name.offset + param_1);
  local_128 = (uint *)FUN_002efa1c("MFSSImage.png",0xd);
  local_130 = (uint *)FUN_002efa1c("Example of sharpened medium-frequency details",0x2d);
  local_150 = (int *)FUN_002efa1c("Adjust amount of sharpening applied to medium-frequency component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                  ,0x83);
  FUN_003bbff0(&local_160,&DAT_013c41f8,"Mid Freq Sharpening Amount",0,0xffffffff);
  local_168 = (int *)FUN_002efa1c("Amount of sharpening applied to medium-frequency component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                  ,0x7c);
  FUN_0003abb8(0,0x40200000,uVar10,uVar17,
               "/System/Library/Frameworks/CFNetwork.framework/Versions/A/CFNetwork" + param_1,
               &local_128,&local_130,param_1 + 0xd30,&local_150,0,0x5000000fa,CONCAT44(uVar21,0x14),
               &DAT_0155b91c,&local_160,0,&local_168,CONCAT44(uVar22,0x12));
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7d20;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b7d20:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7d54;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b7d54:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7d88;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b7d88:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b7dbc;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b7dbc:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b7df0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b7df0:
  uVar17 = *(undefined8 *)((long)&dylib_command_00001568.dylib.name.offset + param_1);
  local_128 = (uint *)FUN_002efa1c("Example of sharpened medium-frequency details",0x2d);
  FUN_00039a7c(uVar10,uVar17,
               "/System/Library/Frameworks/CFNetwork.framework/Versions/A/CFNetwork" +
               param_1 + 0x18,&local_128,0,1,0);
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b7e64;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b7e64:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/CFNetwork.framework/Versions/A/CFNetwork" + param_1 + 0x18)
  ;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Range",0,0xffffffff);
  local_130 = &DAT_0142f030;
  local_150 = (int *)&DAT_0142f030;
  local_160 = (int *)FUN_002efa1c("Adjustment of the frequency range to be sharpened:\n-1 - sharpening is applied to narrower frequency range\n 0 - sharpening is applied to normal frequency range of medium-frequency component (default)\n+1 - sharpening is applied to wider frequency range"
                                  ,0xfa);
  puVar12 = (undefined8 *)__Znwm(0x10);
  *puVar12 = &DAT_013b6470;
  puVar12[1] = &DAT_0155b9d0;
  *(undefined8 **)((long)&dylib_command_00001630.cmd + param_1) = puVar12;
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Sharpening Mid Freq Range",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Adjustment of the frequency range to be sharpened:\n-1 - sharpening is applied to narrower frequency range\n 0 - sharpening is applied to normal frequency range of medium-frequency component (default)\n+1 - sharpening is applied to wider frequency range"
                                  ,0xfa);
  ppiVar20 = &local_170;
  uVar21 = 1;
  FUN_0003ab5c(0,0x40000000,uVar10,uVar17,0,&local_128,0,&local_130,&local_150,
               "/System/Library/Frameworks/CFNetwork.framework/Versions/A/CFNetwork" +
               param_1 + 0x30,&local_160,0x1ffffffff,0x100000001,puVar12,&local_168,0,ppiVar20,0x48,
               0);
  uVar22 = (undefined4)((ulong)ppiVar20 >> 0x20);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7f84;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b7f84:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7fb8;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b7fb8:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b7fec;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b7fec:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b8020;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b8020:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8054;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b8054:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8088;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b8088:
  uVar17 = *(undefined8 *)(param_1 + 0xd10);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Low",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Amount of sharpening applied to low-frequency component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                   ,0x79);
  FUN_00038ca4(uVar10,uVar17,param_1 + 0x1578,&local_128,&local_130,0x5b,&DAT_0155b908,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b811c;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b811c:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8150;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b8150:
  uVar17 = *(undefined8 *)((long)&dylib_command_00001568.dylib.current_version + param_1);
  local_128 = (uint *)FUN_002efa1c("LFSSImage.png",0xd);
  local_130 = (uint *)FUN_002efa1c("Example of sharpened low-frequency details",0x2a);
  local_150 = (int *)FUN_002efa1c("Adjust amount of sharpening applied to low-frequency component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                  ,0x80);
  FUN_003bbff0(&local_160,&DAT_013c41f8,"Low Freq Sharpening Amount",0,0xffffffff);
  local_168 = (int *)FUN_002efa1c("Amount of sharpening applied to low-frequency component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                  ,0x79);
  FUN_0003abb8(0,0x40200000,uVar10,uVar17,
               "/System/Library/Frameworks/CFNetwork.framework/Versions/A/CFNetwork" + param_1 + 8,
               &local_128,&local_130,param_1 + 0xd38,&local_150,0,0x5000000fa,CONCAT44(uVar21,0x14),
               &DAT_0155b920,&local_160,0,&local_168,CONCAT44(uVar22,0x12));
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b825c;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b825c:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b8290;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b8290:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b82c4;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b82c4:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b82f8;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b82f8:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b832c;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b832c:
  uVar17 = *(undefined8 *)((long)&dylib_command_00001568.dylib.current_version + param_1);
  local_128 = (uint *)FUN_002efa1c("Example of sharpened low-frequency details",0x2a);
  FUN_00039a7c(uVar10,uVar17,
               "/System/Library/Frameworks/CFNetwork.framework/Versions/A/CFNetwork" +
               param_1 + 0x20,&local_128,0,1,0);
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b83a0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b83a0:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/CFNetwork.framework/Versions/A/CFNetwork" + param_1 + 0x20)
  ;
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Range",0,0xffffffff);
  local_130 = &DAT_0142f030;
  local_150 = (int *)&DAT_0142f030;
  local_160 = (int *)FUN_002efa1c("Adjustment of the frequency range to be sharpened:\n-1 - sharpening is applied to narrower frequency range\n 0 - sharpening is applied to normal frequency range of low-frequency component (default)\n+1 - sharpening is applied to wider frequency range"
                                  ,0xf7);
  puVar12 = (undefined8 *)__Znwm(0x10);
  *puVar12 = &DAT_013b6470;
  puVar12[1] = &DAT_0155b9d4;
  *(undefined8 **)((long)&dylib_command_00001630.dylib.name.offset + param_1) = puVar12;
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Sharpening Low Freq Range",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Adjustment of the frequency range to be sharpened:\n-1 - sharpening is applied to narrower frequency range\n 0 - sharpening is applied to normal frequency range of low-frequency component (default)\n+1 - sharpening is applied to wider frequency range"
                                  ,0xf7);
  FUN_0003ab5c(0,0x40000000,uVar10,uVar17,0,&local_128,0,&local_130,&local_150,
               "/System/Library/Frameworks/CFNetwork.framework/Versions/A/CFNetwork" +
               param_1 + 0x38,&local_160,0x1ffffffff,0x100000001,puVar12,&local_168,0,&local_170,
               0x48,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b84c0;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b84c0:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b84f4;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b84f4:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b8528;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b8528:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b855c;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b855c:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8590;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b8590:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b85c4;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b85c4:
  uVar17 = *(undefined8 *)(param_1 + 0xd10);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Channels",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Adjust amount of sharpening applied to individual channel components"
                                   ,0x44);
  local_150 = (int *)((ulong)local_150 & 0xffffffffffffff00);
  FUN_00038ca4(uVar10,uVar17,
               "/System/Library/Frameworks/QuartzCore.framework/Versions/A/QuartzCore" +
               param_1 + 0x40,&local_128,&local_130,0x5b,&local_150,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8660;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b8660:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8694;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b8694:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/QuartzCore.framework/Versions/A/QuartzCore" +
            param_1 + 0x40);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Luminance",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("ChannelY.png",0xc);
  local_150 = (int *)FUN_002efa1c("Amount of sharpening applied to Luminance channel component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                  ,0x7d);
  local_160 = (int *)FUN_002efa1c("Adjust amount of sharpening applied to Luminance channel component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                  ,0x84);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Luminance Sharpening Amount",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Amount of sharpening applied to Luminance channel component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                  ,0x7d);
  puVar12 = (undefined8 *)((long)&dylib_command_00001598.cmd + param_1);
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,0,&local_128,puVar12,&local_130,&local_150,
               "/usr/lib/libSystem.B.dylib" + param_1 + 0x18,&local_160,0xfa00000000,0x1400000005,
               &DAT_0155b90c,&local_168,0,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b87c8;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b87c8:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b87fc;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b87fc:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b8830;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b8830:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b8864;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b8864:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8898;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b8898:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b88cc;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b88cc:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/QuartzCore.framework/Versions/A/QuartzCore" +
            param_1 + 0x40);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Cr",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("ChannelCr.png",0xd);
  local_150 = (int *)FUN_002efa1c("Amount of sharpening applied to Cr channel component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                  ,0x76);
  local_160 = (int *)FUN_002efa1c("Adjust amount of sharpening applied to Cr channel component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                  ,0x7d);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Cr Sharpening Amount",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Amount of sharpening applied to Cr channel component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                  ,0x76);
  puVar2 = (undefined8 *)((long)&dylib_command_00001598.dylib.name.offset + param_1);
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,"/usr/lib/libc++.1.dylib" + param_1 + 8,&local_128,puVar2,
               &local_130,&local_150,(long)&dylib_command_000015d0.cmd + param_1,&local_160,
               0xfa00000000,0x1400000005,&DAT_0155b910,&local_168,
               "/usr/lib/libSystem.B.dylib" + param_1,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b8a0c;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b8a0c:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b8a40;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b8a40:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b8a74;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b8a74:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b8aa8;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b8aa8:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8adc;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b8adc:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8b10;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b8b10:
  uVar17 = *(undefined8 *)
            ("/System/Library/Frameworks/QuartzCore.framework/Versions/A/QuartzCore" +
            param_1 + 0x40);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Cb",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("ChannelCb.png",0xd);
  local_150 = (int *)FUN_002efa1c("Amount of sharpening applied to Cb channel component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                  ,0x76);
  local_160 = (int *)FUN_002efa1c("Adjust amount of sharpening applied to Cb channel component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                  ,0x7d);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Cb Sharpening Amount",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Amount of sharpening applied to Cb channel component\n- high values -> more sharpening\n- low values  -> less sharpening"
                                  ,0x76);
  puVar3 = (undefined8 *)((long)&dylib_command_00001598.dylib.current_version + param_1);
  ppiVar20 = &local_170;
  uVar21 = 0xfa;
  FUN_0003aa60(0,0x40200000,uVar10,uVar17,"/usr/lib/libc++.1.dylib" + param_1 + 0x10,&local_128,
               puVar3,&local_130,&local_150,param_1 + 0x15d8,&local_160,0xfa00000000,0x1400000005,
               &DAT_0155b914,&local_168,"/usr/lib/libSystem.B.dylib" + param_1 + 8,ppiVar20,0x5a,0);
  uVar22 = (undefined4)((ulong)ppiVar20 >> 0x20);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b8c50;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b8c50:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b8c84;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b8c84:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b8cb8;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b8cb8:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b8cec;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b8cec:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8d20;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b8d20:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8d54;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b8d54:
  uVar17 = *(undefined8 *)(param_1 + 0xd10);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Prevent Over-Sharpening",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Use higher-quality halo-free sharpening method",0x2e);
  uVar19 = 0;
  FUN_0003ad40(uVar10,uVar17,"/usr/lib/libc++.1.dylib" + param_1,0,0,&DAT_0155b909,&local_128,
               &local_130,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8dec;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b8dec:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8e20;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b8e20:
  uVar17 = *(undefined8 *)(*(long *)(param_1 + 0x198) + 0x50);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"General Settings",0,0xffffffff);
  FUN_00750130(uVar17,&local_128);
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8e88;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b8e88:
  uVar13 = FUN_00850130(uVar17);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Mix with Original",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Adjust mixing of filter effect and original frame\n- high values -> more of filter effect, less of original frame\n- low values  -> less of filter effect, more of original frame"
                                   ,0xaf);
  local_150 = (int *)FUN_002efa1c("Enable mixing of filtered and original frame",0x2c);
  FUN_00039518(uVar17,uVar13,param_1 + 0x1640,&local_128,&local_130,&local_150,0x5d,&DAT_0155b984,
               uVar19 & 0xffffffff00000000);
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b8f3c;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b8f3c:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8f70;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b8f70:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b8fa4;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b8fa4:
  uVar18 = *(undefined8 *)((long)&dylib_command_00001630.dylib.current_version + param_1);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Mix",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Adjust mixing of filter effect and original frame\n- high values -> more of filter effect, less of original frame\n- low values  -> less of filter effect, more of original frame"
                                   ,0xaf);
  local_150 = (int *)((ulong)local_150 & 0xffffffffffffff00);
  FUN_00038ca4(uVar17,uVar18,
               "/System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync" + param_1,
               &local_128,&local_130,0x5b,&local_150,0);
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b9044;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b9044:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b9078;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b9078:
  uVar18 = *(undefined8 *)
            ("/System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync" + param_1);
  local_130 = &DAT_0142f030;
  local_128 = &DAT_0142f030;
  local_150 = (int *)FUN_002efa1c("Adjust mixing of filter effect and original frame\n- high values -> more of filter effect, less of original frame\n- low values  -> less of filter effect, more of original frame"
                                  ,0xaf);
  FUN_003bbff0(&local_160,&DAT_013c41f8,"Mix with original amount",0,0xffffffff);
  local_168 = (int *)FUN_002efa1c("Mixing with original frame\n- high values -> more of filter effect, less of original frame\n- low values  -> less of filter effect, more of original frame"
                                  ,0x98);
  FUN_0003abb8(0,0x3f800000,uVar17,uVar18,0,&local_128,&local_130,
               "/System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync" + param_1 + 8,
               &local_150,0,0x500000064,CONCAT44(uVar21,10),&DAT_0155b988,&local_160,0,&local_168,
               CONCAT44(uVar22,0x12));
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b915c;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b915c:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b9190;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b9190:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b91c4;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b91c4:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b91f8;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b91f8:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b922c;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b922c:
  uVar18 = *(undefined8 *)
            ("/System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync" + param_1);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Luminance",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("ChannelY.png",0xc);
  local_150 = (int *)FUN_002efa1c("Mixing with original applied to Luminance channel component\n- high values -> more of filter effect, less of original frame\n- low values  -> less of filter effect, more of original frame"
                                  ,0xb9);
  local_160 = (int *)FUN_002efa1c("Adjust mixing of filter effect and original frame in Luminance channel component\n- high values -> more of filter effect, less of original frame\n- low values  -> less of filter effect, more of original frame"
                                  ,0xce);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Luminance mix with original amount",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Mixing with original applied to Luminance channel component\n- high values -> more of filter effect, less of original frame\n- low values  -> less of filter effect, more of original frame"
                                  ,0xb9);
  FUN_0003aa60(0,0x3f800000,uVar17,uVar18,0,&local_128,
               "/System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync" +
               param_1 + 0x38,&local_130,&local_150,
               "/System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync" +
               param_1 + 0x10,&local_160,0x6400000000,0xa00000005,&DAT_0155b98c,&local_168,0,
               &local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b935c;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b935c:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b9390;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b9390:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b93c4;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b93c4:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b93f8;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b93f8:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b942c;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b942c:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b9460;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b9460:
  uVar18 = *(undefined8 *)
            ("/System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync" + param_1);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Cr",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("ChannelCr.png",0xd);
  local_150 = (int *)FUN_002efa1c("Mixing with original applied to Cr channel component\n- high values -> more of filter effect, less of original frame\n- low values  -> less of filter effect, more of original frame"
                                  ,0xb2);
  local_160 = (int *)FUN_002efa1c("Adjust mixing of filter effect and original frame in Cr channel component\n- high values -> more of filter effect, less of original frame\n- low values  -> less of filter effect, more of original frame"
                                  ,199);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Cr mix with original amount",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Mixing with original applied to Cr channel component\n- high values -> more of filter effect, less of original frame\n- low values  -> less of filter effect, more of original frame"
                                  ,0xb2);
  FUN_0003aa60(0,0x3f800000,uVar17,uVar18,
               "/System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync" +
               param_1 + 0x28,&local_128,
               "/System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync" +
               param_1 + 0x40,&local_130,&local_150,
               "/System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync" +
               param_1 + 0x18,&local_160,0x6400000000,0xa00000005,&DAT_0155b990,&local_168,
               param_1 + 0x1698,&local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b959c;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b959c:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b95d0;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b95d0:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b9604;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b9604:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b9638;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b9638:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b966c;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b966c:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b96a0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b96a0:
  uVar18 = *(undefined8 *)
            ("/System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync" + param_1);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Cb",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("ChannelCb.png",0xd);
  local_150 = (int *)FUN_002efa1c("Mixing with original applied to Cb channel component\n- high values -> more of filter effect, less of original frame\n- low values  -> less of filter effect, more of original frame"
                                  ,0xb2);
  local_160 = (int *)FUN_002efa1c("Adjust mixing of filter effect and original frame in Cb channel component\n- high values -> more of filter effect, less of original frame\n- low values  -> less of filter effect, more of original frame"
                                  ,199);
  FUN_003bbff0(&local_168,&DAT_013c41f8,"Cb mix with original amount",0,0xffffffff);
  local_170 = (int *)FUN_002efa1c("Mixing with original applied to Cb channel component\n- high values -> more of filter effect, less of original frame\n- low values  -> less of filter effect, more of original frame"
                                  ,0xb2);
  uVar19 = param_1 + 0x16a0;
  ppiVar20 = &local_160;
  FUN_0003aa60(0,0x3f800000,uVar17,uVar18,
               "/System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync" +
               param_1 + 0x30,&local_128,(long)&dylib_command_00001690.cmd + param_1,&local_130,
               &local_150,
               "/System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync" +
               param_1 + 0x20,ppiVar20,0x6400000000,0xa00000005,&DAT_0155b994,&local_168,uVar19,
               &local_170,0x5a,0);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b97dc;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b97dc:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b9810;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b9810:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b9844;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b9844:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b9878;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b9878:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b98ac;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b98ac:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b98e0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b98e0:
  FUN_0003c864(uVar17,uVar13,2);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Performance",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Adjust performance settings of Neat Video filters",0x31);
  local_150 = (int *)&DAT_0142f030;
  local_160 = (int *)((ulong)local_160 & 0xffffffffffffff00);
  FUN_00039518(uVar17,uVar13,param_1 + 0xd40,&local_128,&local_130,&local_150,0x59,&local_160,
               (ulong)ppiVar20 & 0xffffffff00000000);
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b9988;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b9988:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b99bc;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b99bc:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b99f0;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b99f0:
  uVar13 = *(undefined8 *)(param_1 + 0xd40);
  local_128 = (uint *)FUN_002efa1c("Adjust performance settings of Neat Video filters",0x31);
  FUN_00039a7c(uVar17,uVar13,(undefined8 *)(param_1 + 0xd48),&local_128,0,0,1);
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b9a60;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b9a60:
  uVar13 = *(undefined8 *)(param_1 + 0xd48);
  FUN_003bbff0(&local_128,&DAT_013c41f8,"Check Speed",0,0xffffffff);
  local_130 = (uint *)FUN_002efa1c("Measure processing speed achieved with current filter settings",
                                   0x3e);
  FUN_003bbff0(&local_150,&DAT_013c41f8,"Preferences...",0,0xffffffff);
  local_160 = (int *)FUN_002efa1c("Open Preferences to adjust and optimize CPU and GPU settings",
                                  0x3c);
  local_168 = (int *)FUN_002efa1c("Current filter settings that affect processing speed the most",
                                  0x3d);
  local_170 = (int *)FUN_002efa1c("Processing speed achieved with current filter settings",0x36);
  FUN_0003c0d4(uVar17,uVar13,&local_128,&local_130,param_1 + 0xd50,&local_150,&local_160,
               param_1 + 0xd58,param_1 + 0xd68,param_1 + 0xd70,&local_168,param_1 + 0xd78,&local_170
               ,uVar19 & 0xffffffff00000000);
  if (*local_170 != -1) {
    if (*local_170 != 0) {
      LOAcquire();
      iVar5 = *local_170;
      *local_170 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b9b70;
    }
    FUN_00287df8(local_170,2,8);
  }
LAB_000b9b70:
  if (*local_168 != -1) {
    if (*local_168 != 0) {
      LOAcquire();
      iVar5 = *local_168;
      *local_168 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b9ba4;
    }
    FUN_00287df8(local_168,2,8);
  }
LAB_000b9ba4:
  if (*local_160 != -1) {
    if (*local_160 != 0) {
      LOAcquire();
      iVar5 = *local_160;
      *local_160 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b9bd8;
    }
    FUN_00287df8(local_160,2,8);
  }
LAB_000b9bd8:
  if (*local_150 != -1) {
    if (*local_150 != 0) {
      LOAcquire();
      iVar5 = *local_150;
      *local_150 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000b9c0c;
    }
    FUN_00287df8(local_150,2,8);
  }
LAB_000b9c0c:
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar6 = *local_130;
      *local_130 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b9c40;
    }
    FUN_00287df8(local_130,2,8);
  }
LAB_000b9c40:
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b9c74;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_000b9c74:
  FUN_003d0e8c(auStack_178,*(undefined8 *)(param_1 + 0xd50),"2clicked()",param_1,
               "1checkSpeedButtonClick()",0);
  FUN_003d24d8(auStack_178);
  FUN_003d0e8c(auStack_180,*(undefined8 *)(param_1 + 0xd58),"2clicked()",param_1,
               "1preferencesButtonClick()",0);
  FUN_003d24d8(auStack_180);
  FUN_00753798(*(undefined8 *)(param_1 + 0x158));
  FUN_00753798(*(undefined8 *)(param_1 + 0x150));
  FUN_00753798(*(undefined8 *)(param_1 + 0x180));
  FUN_00753798(*(undefined8 *)(param_1 + 0x178));
  FUN_00753798(*(undefined8 *)(param_1 + 0x1a8));
  FUN_00753798(*(undefined8 *)(param_1 + 0x1a0));
  local_128 = &DAT_0142f0a8;
  FUN_0001cacc(uVar9,&local_128);
  if (local_128[4] != 0xffffffff) {
    if (local_128[4] != 0) {
      LOAcquire();
      uVar6 = local_128[4];
      local_128[4] = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b9d4c;
    }
    FUN_002a9a28(local_128,FUN_0072a91c);
  }
LAB_000b9d4c:
  FUN_00093be0(param_1,uVar9);
  local_128 = &DAT_0142f0a8;
  FUN_0001cacc(uVar10,&local_128);
  if (local_128[4] != 0xffffffff) {
    if (local_128[4] != 0) {
      LOAcquire();
      uVar6 = local_128[4];
      local_128[4] = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b9da8;
    }
    FUN_002a9a28(local_128,FUN_0072a91c);
  }
LAB_000b9da8:
  FUN_00093be0(param_1,uVar10);
  local_128 = &DAT_0142f0a8;
  FUN_0001cacc(uVar17,&local_128);
  if (local_128[4] != 0xffffffff) {
    if (local_128[4] != 0) {
      LOAcquire();
      uVar6 = local_128[4];
      local_128[4] = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000b9e04;
    }
    FUN_002a9a28(local_128,FUN_0072a91c);
  }
LAB_000b9e04:
  FUN_00093be0(param_1,uVar17);
  local_130 = &DAT_0142f030;
  uVar6 = DAT_0142f034 + 1;
  uVar7 = DAT_0142f038 & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation" + param_1 + 0x30
           );
  if ((DAT_0142f030 < 2) && (uVar6 <= uVar7)) {
    *(undefined8 *)((long)&DAT_0142f030 + (long)DAT_0142f034 * 8 + DAT_0142f040) = uVar9;
  }
  else {
    uVar4 = uVar6;
    if (uVar6 <= uVar7) {
      uVar4 = uVar7;
    }
    FUN_000bf60c(&local_130,uVar4,(ulong)(uVar7 < uVar6) << 3);
    *(undefined8 *)((long)local_130 + (long)(int)local_130[1] * 8 + *(long *)(local_130 + 4)) =
         uVar9;
    uVar6 = local_130[1] + 1;
  }
  local_130[1] = uVar6;
  uVar7 = uVar6 + 1;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo" + param_1 + 0x10);
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *puVar12;
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar6 = local_130[1];
  local_130[1] = (uint)((long)(int)uVar6 + 1);
  if (*local_130 == 0) {
    if ((int)local_130[2] < 0) {
      puVar14 = (uint *)FUN_00287b7c(8,8,local_130[2] & 0x7fffffff,0);
      local_128 = puVar14;
      if (puVar14 == (uint *)0x0) {
        FUN_00c78400();
      }
      puVar14[2] = puVar14[2] | 0x80000000;
      puVar16 = puVar14;
    }
    else {
      puVar14 = (uint *)FUN_00287b7c(8,8,(long)(int)uVar6 + 1,0);
      puVar16 = puVar14;
      local_128 = puVar14;
      if (puVar14 == (uint *)0x0) {
        FUN_00c78400();
        puVar16 = (uint *)0x0;
      }
    }
    if ((puVar16[2] & 0x7fffffff) != 0) {
      _memcpy((void *)((long)puVar16 + *(long *)(puVar16 + 4)),
              (void *)((long)local_130 + *(long *)(local_130 + 4)),(long)(int)local_130[1] << 3);
      puVar16[1] = local_130[1];
    }
  }
  else {
    if (*local_130 != 0xffffffff) {
      LOAcquire();
      *local_130 = *local_130 + 1;
      LORelease();
    }
    local_128 = local_130;
    puVar14 = local_130;
  }
  puVar12 = (undefined8 *)((long)puVar14 + *(long *)(puVar14 + 4));
  uVar6 = puVar14[1];
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar7 = *local_130;
      *local_130 = uVar7 - 1;
      LORelease();
      if (uVar7 != 1) goto LAB_000ba07c;
    }
    FUN_00287df8(local_130,8,8);
  }
LAB_000ba07c:
  if (puVar12 + (int)uVar6 != puVar12) {
    lVar15 = (long)(int)uVar6 << 3;
    do {
      FUN_0002c744(*puVar12,*(undefined8 *)(param_1 + 0x70));
      puVar12 = puVar12 + 1;
      lVar15 = lVar15 + -8;
    } while (lVar15 != 0);
  }
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000ba0d4;
    }
    FUN_00287df8(local_128,8,8);
  }
LAB_000ba0d4:
  local_130 = &DAT_0142f030;
  uVar6 = DAT_0142f034 + 1;
  uVar7 = DAT_0142f038 & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation" + param_1 + 0x38
           );
  if (DAT_0142f030 < 2 && uVar6 <= uVar7) {
    *(undefined8 *)((long)&DAT_0142f030 + (long)DAT_0142f034 * 8 + DAT_0142f040) = uVar9;
  }
  else {
    uVar4 = uVar6;
    if (uVar6 <= uVar7) {
      uVar4 = uVar7;
    }
    FUN_000bf60c(&local_130,uVar4,(ulong)(uVar7 < uVar6) << 3);
    *(undefined8 *)((long)local_130 + (long)(int)local_130[1] * 8 + *(long *)(local_130 + 4)) =
         uVar9;
    uVar6 = local_130[1] + 1;
  }
  local_130[1] = uVar6;
  uVar7 = uVar6 + 1;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo" + param_1 + 0x18);
  if (*local_130 < 2 && uVar7 <= uVar4) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *puVar2;
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar6 = local_130[1];
  local_130[1] = (uint)((long)(int)uVar6 + 1);
  if (*local_130 == 0) {
    if ((int)local_130[2] < 0) {
      puVar14 = (uint *)FUN_00287b7c(8,8,local_130[2] & 0x7fffffff,0);
      local_128 = puVar14;
      if (puVar14 == (uint *)0x0) {
        FUN_00c78400();
      }
      puVar14[2] = puVar14[2] | 0x80000000;
      puVar16 = puVar14;
    }
    else {
      puVar14 = (uint *)FUN_00287b7c(8,8,(long)(int)uVar6 + 1,0);
      puVar16 = puVar14;
      local_128 = puVar14;
      if (puVar14 == (uint *)0x0) {
        FUN_00c78400();
        puVar16 = (uint *)0x0;
      }
    }
    if ((puVar16[2] & 0x7fffffff) != 0) {
      _memcpy((void *)((long)puVar16 + *(long *)(puVar16 + 4)),
              (void *)((long)local_130 + *(long *)(local_130 + 4)),(long)(int)local_130[1] << 3);
      puVar16[1] = local_130[1];
    }
  }
  else {
    if (*local_130 != 0xffffffff) {
      LOAcquire();
      *local_130 = *local_130 + 1;
      LORelease();
    }
    local_128 = local_130;
    puVar14 = local_130;
  }
  puVar12 = (undefined8 *)((long)puVar14 + *(long *)(puVar14 + 4));
  uVar6 = puVar14[1];
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar7 = *local_130;
      *local_130 = uVar7 - 1;
      LORelease();
      if (uVar7 != 1) goto LAB_000ba33c;
    }
    FUN_00287df8(local_130,8,8);
  }
LAB_000ba33c:
  if (puVar12 + (int)uVar6 != puVar12) {
    lVar15 = (long)(int)uVar6 << 3;
    do {
      FUN_0002c744(*puVar12,*(undefined8 *)(param_1 + 0x78));
      puVar12 = puVar12 + 1;
      lVar15 = lVar15 + -8;
    } while (lVar15 != 0);
  }
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000ba394;
    }
    FUN_00287df8(local_128,8,8);
  }
LAB_000ba394:
  local_130 = &DAT_0142f030;
  uVar6 = DAT_0142f034 + 1;
  uVar7 = DAT_0142f038 & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation" + param_1 + 0x40
           );
  if (DAT_0142f030 < 2 && uVar6 <= uVar7) {
    *(undefined8 *)((long)&DAT_0142f030 + (long)DAT_0142f034 * 8 + DAT_0142f040) = uVar9;
  }
  else {
    uVar4 = uVar6;
    if (uVar6 <= uVar7) {
      uVar4 = uVar7;
    }
    FUN_000bf60c(&local_130,uVar4,(ulong)(uVar7 < uVar6) << 3);
    *(undefined8 *)((long)local_130 + (long)(int)local_130[1] * 8 + *(long *)(local_130 + 4)) =
         uVar9;
    uVar6 = local_130[1] + 1;
  }
  local_130[1] = uVar6;
  uVar7 = uVar6 + 1;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo" + param_1 + 0x20);
  if (*local_130 < 2 && uVar7 <= uVar4) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *puVar3;
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar6 = local_130[1];
  local_130[1] = (uint)((long)(int)uVar6 + 1);
  if (*local_130 == 0) {
    if ((int)local_130[2] < 0) {
      puVar14 = (uint *)FUN_00287b7c(8,8,local_130[2] & 0x7fffffff,0);
      local_128 = puVar14;
      if (puVar14 == (uint *)0x0) {
        FUN_00c78400();
      }
      puVar14[2] = puVar14[2] | 0x80000000;
      puVar16 = puVar14;
    }
    else {
      puVar14 = (uint *)FUN_00287b7c(8,8,(long)(int)uVar6 + 1,0);
      puVar16 = puVar14;
      local_128 = puVar14;
      if (puVar14 == (uint *)0x0) {
        FUN_00c78400();
        puVar16 = (uint *)0x0;
      }
    }
    if ((puVar16[2] & 0x7fffffff) != 0) {
      _memcpy((void *)((long)puVar16 + *(long *)(puVar16 + 4)),
              (void *)((long)local_130 + *(long *)(local_130 + 4)),(long)(int)local_130[1] << 3);
      puVar16[1] = local_130[1];
    }
  }
  else {
    if (*local_130 != 0xffffffff) {
      LOAcquire();
      *local_130 = *local_130 + 1;
      LORelease();
    }
    local_128 = local_130;
    puVar14 = local_130;
  }
  puVar12 = (undefined8 *)((long)puVar14 + *(long *)(puVar14 + 4));
  uVar6 = puVar14[1];
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar7 = *local_130;
      *local_130 = uVar7 - 1;
      LORelease();
      if (uVar7 != 1) goto LAB_000ba5fc;
    }
    FUN_00287df8(local_130,8,8);
  }
LAB_000ba5fc:
  if (puVar12 + (int)uVar6 != puVar12) {
    lVar15 = (long)(int)uVar6 << 3;
    do {
      FUN_0002c744(*puVar12,*(undefined8 *)(param_1 + 0x80));
      puVar12 = puVar12 + 1;
      lVar15 = lVar15 + -8;
    } while (lVar15 != 0);
  }
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000ba654;
    }
    FUN_00287df8(local_128,8,8);
  }
LAB_000ba654:
  FUN_0002c744(*(undefined8 *)
                ("/System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync" +
                param_1 + 0x38),*(undefined8 *)(param_1 + 0x70));
  FUN_0002c744(*(undefined8 *)
                ("/System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync" +
                param_1 + 0x40),*(undefined8 *)(param_1 + 0x78));
  FUN_0002c744(*(undefined8 *)((long)&dylib_command_00001690.cmd + param_1),
               *(undefined8 *)(param_1 + 0x80));
  local_130 = &DAT_0142f030;
  uVar6 = DAT_0142f034 + 1;
  uVar7 = DAT_0142f038 & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/IOSurface.framework/Versions/A/IOSurface" + param_1 + 0x28);
  if (DAT_0142f030 < 2 && uVar6 <= uVar7) {
    *(undefined8 *)((long)&DAT_0142f030 + (long)DAT_0142f034 * 8 + DAT_0142f040) = uVar9;
  }
  else {
    uVar4 = uVar6;
    if (uVar6 <= uVar7) {
      uVar4 = uVar7;
    }
    FUN_000bf60c(&local_130,uVar4,(ulong)(uVar7 < uVar6) << 3);
    *(undefined8 *)((long)local_130 + (long)(int)local_130[1] * 8 + *(long *)(local_130 + 4)) =
         uVar9;
    uVar6 = local_130[1] + 1;
  }
  local_130[1] = uVar6;
  uVar7 = uVar6 + 1;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/IOSurface.framework/Versions/A/IOSurface" + param_1 + 0x30);
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/IOSurface.framework/Versions/A/IOSurface" + param_1 + 0x38);
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/ImageIO.framework/Versions/A/ImageIO" + param_1);
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/ImageIO.framework/Versions/A/ImageIO" + param_1 + 8);
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/ImageIO.framework/Versions/A/ImageIO" + param_1 + 0x10);
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *puVar11;
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics" + param_1);
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics" +
           param_1 + 8);
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *puVar8;
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/CFNetwork.framework/Versions/A/CFNetwork" + param_1);
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/CFNetwork.framework/Versions/A/CFNetwork" + param_1 + 8);
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics" +
           param_1 + 0x30);
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics" +
           param_1 + 0x38);
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar7 = local_130[1];
  uVar6 = uVar7 + 1;
  local_130[1] = uVar6;
  uVar7 = uVar7 + 2;
  uVar4 = local_130[2] & 0x7fffffff;
  uVar9 = *(undefined8 *)
           ("/System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics" +
           param_1 + 0x40);
  if ((*local_130 < 2) && (uVar7 <= uVar4)) {
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
  }
  else {
    uVar6 = uVar7;
    if (uVar7 <= uVar4) {
      uVar6 = uVar4;
    }
    FUN_000bf60c(&local_130,uVar6,(ulong)(uVar4 < uVar7) << 3);
    lVar15 = (long)local_130 + *(long *)(local_130 + 4);
    uVar6 = local_130[1];
  }
  *(undefined8 *)(lVar15 + (long)(int)uVar6 * 8) = uVar9;
  uVar6 = local_130[1];
  local_130[1] = (uint)((long)(int)uVar6 + 1);
  if (*local_130 == 0) {
    if ((int)local_130[2] < 0) {
      puVar14 = (uint *)FUN_00287b7c(8,8,local_130[2] & 0x7fffffff,0);
      local_128 = puVar14;
      if (puVar14 == (uint *)0x0) {
        FUN_00c78400();
      }
      puVar14[2] = puVar14[2] | 0x80000000;
      puVar16 = puVar14;
    }
    else {
      puVar14 = (uint *)FUN_00287b7c(8,8,(long)(int)uVar6 + 1,0);
      puVar16 = puVar14;
      local_128 = puVar14;
      if (puVar14 == (uint *)0x0) {
        FUN_00c78400();
        puVar16 = (uint *)0x0;
      }
    }
    if ((puVar16[2] & 0x7fffffff) != 0) {
      _memcpy((void *)((long)puVar16 + *(long *)(puVar16 + 4)),
              (void *)((long)local_130 + *(long *)(local_130 + 4)),(long)(int)local_130[1] << 3);
      puVar16[1] = local_130[1];
    }
  }
  else {
    if (*local_130 != 0xffffffff) {
      LOAcquire();
      *local_130 = *local_130 + 1;
      LORelease();
    }
    local_128 = local_130;
    puVar14 = local_130;
  }
  puVar8 = (undefined8 *)((long)puVar14 + *(long *)(puVar14 + 4));
  uVar6 = puVar14[1];
  if (*local_130 != 0xffffffff) {
    if (*local_130 != 0) {
      LOAcquire();
      uVar7 = *local_130;
      *local_130 = uVar7 - 1;
      LORelease();
      if (uVar7 != 1) goto LAB_000badf0;
    }
    FUN_00287df8(local_130,8,8);
  }
LAB_000badf0:
  if (puVar8 + (int)uVar6 != puVar8) {
    lVar15 = (long)(int)uVar6 << 3;
    do {
      FUN_0002c744(*puVar8,*(undefined8 *)(param_1 + 0x68));
      puVar8 = puVar8 + 1;
      lVar15 = lVar15 + -8;
    } while (lVar15 != 0);
  }
  if (*local_128 != 0xffffffff) {
    if (*local_128 != 0) {
      LOAcquire();
      uVar6 = *local_128;
      *local_128 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000bae48;
    }
    FUN_00287df8(local_128,8,8);
  }
LAB_000bae48:
  if (*local_110 != -1) {
    if (*local_110 != 0) {
      LOAcquire();
      iVar5 = *local_110;
      *local_110 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bae7c;
    }
    FUN_00287df8(local_110,2,8);
  }
LAB_000bae7c:
  if (*local_118 != -1) {
    if (*local_118 != 0) {
      LOAcquire();
      iVar5 = *local_118;
      *local_118 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000baeb0;
    }
    FUN_00287df8(local_118,2,8);
  }
LAB_000baeb0:
  if (*local_f8 != -1) {
    if (*local_f8 != 0) {
      LOAcquire();
      iVar5 = *local_f8;
      *local_f8 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000baee4;
    }
    FUN_00287df8(local_f8,2,8);
  }
LAB_000baee4:
  if (*local_100 != -1) {
    if (*local_100 != 0) {
      LOAcquire();
      iVar5 = *local_100;
      *local_100 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000baf18;
    }
    FUN_00287df8(local_100,2,8);
  }
LAB_000baf18:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar5 = *local_108;
      *local_108 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000baf4c;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_000baf4c:
  if (*local_e8 != -1) {
    if (*local_e8 != 0) {
      LOAcquire();
      iVar5 = *local_e8;
      *local_e8 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000baf80;
    }
    FUN_00287df8(local_e8,2,8);
  }
LAB_000baf80:
  if (*local_f0 != -1) {
    if (*local_f0 != 0) {
      LOAcquire();
      iVar5 = *local_f0;
      *local_f0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bafb4;
    }
    FUN_00287df8(local_f0,2,8);
  }
LAB_000bafb4:
  if (local_120[4] != -1) {
    if (local_120[4] != 0) {
      LOAcquire();
      iVar5 = local_120[4];
      local_120[4] = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bafec;
    }
    FUN_002a9a28(local_120,FUN_00559b60);
  }
LAB_000bafec:
  if (*local_c0 != -1) {
    if (*local_c0 != 0) {
      LOAcquire();
      iVar5 = *local_c0;
      *local_c0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bb020;
    }
    FUN_00287df8(local_c0,2,8);
  }
LAB_000bb020:
  if (*local_c8 != -1) {
    if (*local_c8 != 0) {
      LOAcquire();
      iVar5 = *local_c8;
      *local_c8 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bb054;
    }
    FUN_00287df8(local_c8,2,8);
  }
LAB_000bb054:
  if (*local_d0 != -1) {
    if (*local_d0 != 0) {
      LOAcquire();
      iVar5 = *local_d0;
      *local_d0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bb088;
    }
    FUN_00287df8(local_d0,2,8);
  }
LAB_000bb088:
  if (*local_d8 != -1) {
    if (*local_d8 != 0) {
      LOAcquire();
      iVar5 = *local_d8;
      *local_d8 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bb0bc;
    }
    FUN_00287df8(local_d8,2,8);
  }
LAB_000bb0bc:
  if (*local_e0 != -1) {
    if (*local_e0 != 0) {
      LOAcquire();
      iVar5 = *local_e0;
      *local_e0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bb0f0;
    }
    FUN_00287df8(local_e0,2,8);
  }
LAB_000bb0f0:
  if (*local_98 != -1) {
    if (*local_98 != 0) {
      LOAcquire();
      iVar5 = *local_98;
      *local_98 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bb124;
    }
    FUN_00287df8(local_98,2,8);
  }
LAB_000bb124:
  if (*local_a0 != -1) {
    if (*local_a0 != 0) {
      LOAcquire();
      iVar5 = *local_a0;
      *local_a0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bb158;
    }
    FUN_00287df8(local_a0,2,8);
  }
LAB_000bb158:
  if (*local_a8 != -1) {
    if (*local_a8 != 0) {
      LOAcquire();
      iVar5 = *local_a8;
      *local_a8 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bb18c;
    }
    FUN_00287df8(local_a8,2,8);
  }
LAB_000bb18c:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar5 = *local_b0;
      *local_b0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bb1c0;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_000bb1c0:
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar5 = *local_b8;
      *local_b8 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bb1f4;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_000bb1f4:
  if (*local_88 != -1) {
    if (*local_88 != 0) {
      LOAcquire();
      iVar5 = *local_88;
      *local_88 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bb228;
    }
    FUN_00287df8(local_88,2,8);
  }
LAB_000bb228:
  if (*local_90 != -1) {
    if (*local_90 != 0) {
      LOAcquire();
      iVar5 = *local_90;
      *local_90 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bb25c;
    }
    FUN_00287df8(local_90,2,8);
  }
LAB_000bb25c:
  if (*local_78 != -1) {
    if (*local_78 != 0) {
      LOAcquire();
      iVar5 = *local_78;
      *local_78 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bb290;
    }
    FUN_00287df8(local_78,2,8);
  }
LAB_000bb290:
  if (*local_80 != -1) {
    if (*local_80 != 0) {
      LOAcquire();
      iVar5 = *local_80;
      *local_80 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000bb2c4;
    }
    FUN_00287df8(local_80,2,8);
  }
LAB_000bb2c4:
  if (*(long *)PTR____stack_chk_guard_013b4320 == local_70) {
    return param_1;
  }
                    /* WARNING: Subroutine does not return */
  ___stack_chk_fail();
}


