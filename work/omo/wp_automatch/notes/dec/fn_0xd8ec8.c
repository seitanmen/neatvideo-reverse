// FUN_000d8ec8 size=2496

long FUN_000d8ec8(long param_1)

{
  uint uVar1;
  bool bVar2;
  int iVar3;
  undefined8 *puVar4;
  undefined8 uVar5;
  undefined4 *puVar6;
  long lVar7;
  undefined8 uVar8;
  undefined1 auStack_a8 [8];
  undefined1 auStack_a0 [8];
  undefined1 auStack_98 [8];
  undefined1 auStack_90 [8];
  undefined1 auStack_88 [8];
  undefined1 auStack_80 [8];
  undefined1 auStack_78 [8];
  undefined *puStack_70;
  undefined8 uStack_68;
  int *piStack_60;
  undefined8 uStack_58;
  
  puVar4 = (undefined8 *)func_0x000929d8();
  *puVar4 = &UNK_013c53f0;
  puVar4[2] = &UNK_013c55f8;
  puVar4[6] = &UNK_013c5648;
  func_0x0099df20(puVar4 + 0x2a);
  *(undefined8 *)(&UNK_0000d678 + param_1) = 0x142f030;
  *(undefined8 *)(&DAT_0000d680 + param_1) = 0x142f030;
  func_0x000d99d4(param_1 + 0xa0,param_1);
  uVar5 = __Znwm(0xa0);
  func_0x000d8598(uVar5,0);
  *(undefined8 *)(&UNK_0000d6b8 + param_1) = uVar5;
  *(undefined1 *)(param_1 + 0x148) = 0;
  *(undefined8 *)(&UNK_0000d688 + param_1) = 0xffffffff00000000;
  (**(code **)(**(long **)(param_1 + 0xf8) + 0x1b8))
            (*(long **)(param_1 + 0xf8),*(undefined8 *)(param_1 + 0x88));
  uVar5 = *(undefined8 *)(param_1 + 0xf8);
  piStack_60 = (int *)func_0x002efa1c("Undo.png",8);
  func_0x000696a0(uVar5,&piStack_60,1);
  if (*piStack_60 != -1) {
    if (*piStack_60 != 0) {
      LOAcquire();
      iVar3 = *piStack_60;
      *piStack_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000d8fcc;
    }
    func_0x00287df8(piStack_60,2,8);
  }
LAB_000d8fcc:
  (**(code **)(**(long **)(param_1 + 0x100) + 0x1b8))
            (*(long **)(param_1 + 0x100),*(undefined8 *)(param_1 + 0x90));
  uVar5 = *(undefined8 *)(param_1 + 0x100);
  piStack_60 = (int *)func_0x002efa1c("Redo.png",8);
  func_0x000696a0(uVar5,&piStack_60,1);
  if (*piStack_60 != -1) {
    if (*piStack_60 != 0) {
      LOAcquire();
      iVar3 = *piStack_60;
      *piStack_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000d903c;
    }
    func_0x00287df8(piStack_60,2,8);
  }
LAB_000d903c:
  func_0x0074c210(*(undefined8 *)(param_1 + 0xf8),0x25);
  func_0x0074c210(*(undefined8 *)(param_1 + 0x100),0x25);
  lVar7 = *(long *)(param_1 + 0x128);
  piStack_60 = (int *)func_0x002efa1c("dnp",3);
  func_0x002e79b0(lVar7 + 0x90,&piStack_60);
  if (*piStack_60 != -1) {
    if (*piStack_60 != 0) {
      LOAcquire();
      iVar3 = *piStack_60;
      *piStack_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000d90ac;
    }
    func_0x00287df8(piStack_60,2,8);
  }
LAB_000d90ac:
  uVar5 = *(undefined8 *)(param_1 + 0x128);
  piStack_60 = (int *)func_0x002efa1c("ProfilePopupButton%1.png",0x18);
  func_0x000244c8(uVar5,&piStack_60);
  if (*piStack_60 != -1) {
    if (*piStack_60 != 0) {
      LOAcquire();
      iVar3 = *piStack_60;
      *piStack_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000d9104;
    }
    func_0x00287df8(piStack_60,2,8);
  }
LAB_000d9104:
  uVar5 = *(undefined8 *)(param_1 + 0x128);
  piStack_60 = (int *)func_0x002efa1c("profile not ready",0x11);
  func_0x00024340(uVar5,&piStack_60);
  if (*piStack_60 != -1) {
    if (*piStack_60 != 0) {
      LOAcquire();
      iVar3 = *piStack_60;
      *piStack_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000d915c;
    }
    func_0x00287df8(piStack_60,2,8);
  }
LAB_000d915c:
  uVar5 = *(undefined8 *)(param_1 + 0x128);
  piStack_60 = (int *)func_0x002efa1c("Build a noise profile by analysing a uniform area or load a ready-made profile."
                                      ,0x4f);
  func_0x000243e4(uVar5,&piStack_60);
  if (*piStack_60 != -1) {
    if (*piStack_60 != 0) {
      LOAcquire();
      iVar3 = *piStack_60;
      *piStack_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000d91b4;
    }
    func_0x00287df8(piStack_60,2,8);
  }
LAB_000d91b4:
  lVar7 = *(long *)(param_1 + 0x128);
  piStack_60 = (int *)func_0x002efa1c("Neat Video could not find any profiles in the assigned Profile folder:\n %1\n\nPlease either put some profiles to that folder or adjust its location using the Preferences dialog:\n  menu Tools > Preferences > Folders > Profile folder"
                                      ,0xe5);
  func_0x002e79b0(lVar7 + 0xd8,&piStack_60);
  if (*piStack_60 != -1) {
    if (*piStack_60 != 0) {
      LOAcquire();
      iVar3 = *piStack_60;
      *piStack_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000d920c;
    }
    func_0x00287df8(piStack_60,2,8);
  }
LAB_000d920c:
  uVar5 = *(undefined8 *)(param_1 + 0x128);
  piStack_60 = (int *)func_0x002efa1c("Show saved noise profiles",0x19);
  func_0x00024488(uVar5,&piStack_60);
  if (*piStack_60 != -1) {
    if (*piStack_60 != 0) {
      LOAcquire();
      iVar3 = *piStack_60;
      *piStack_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000d9264;
    }
    func_0x00287df8(piStack_60,2,8);
  }
LAB_000d9264:
  lVar7 = *(long *)(param_1 + 0x128);
  func_0x003bbff0(&piStack_60,&UNK_013c5128,"Profile Folder",0,0xffffffff);
  func_0x002e79b0(lVar7 + 0xe0,&piStack_60);
  if (*piStack_60 != -1) {
    if (*piStack_60 != 0) {
      LOAcquire();
      iVar3 = *piStack_60;
      *piStack_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000d92c8;
    }
    func_0x00287df8(piStack_60,2,8);
  }
LAB_000d92c8:
  func_0x007d1d58(*(undefined8 *)(*(long *)(&UNK_0000d6b8 + param_1) + 0x98),1);
  uVar8 = *(undefined8 *)(*(long *)(param_1 + 0x130) + 0x50);
  uVar5 = func_0x00850130(uVar8);
  piStack_60 = (int *)0x142f030;
  func_0x00039a7c(uVar8,uVar5,&UNK_0000d6b0 + param_1,&piStack_60,0,0,0);
  if (*piStack_60 != -1) {
    if (*piStack_60 != 0) {
      LOAcquire();
      iVar3 = *piStack_60;
      *piStack_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000d9344;
    }
    func_0x00287df8(piStack_60,2,8);
  }
LAB_000d9344:
  func_0x0074bf38(*(undefined8 *)(&UNK_0000d6b8 + param_1),
                  (*(int *)(*(long *)(*(long *)(&UNK_0000d6b0 + param_1) + 0x28) + 0x1c) -
                  *(int *)(*(long *)(*(long *)(&UNK_0000d6b0 + param_1) + 0x28) + 0x14)) + 1,0xa2);
  func_0x0003c9b0(uVar8,*(undefined8 *)(&UNK_0000d6b0 + param_1),
                  *(undefined8 *)(&UNK_0000d6b8 + param_1));
  piStack_60 = (int *)0x142f030;
  func_0x00039a7c(uVar8,uVar5,&UNK_0000d6c0 + param_1,&piStack_60,0,0,0);
  if (*piStack_60 != -1) {
    if (*piStack_60 != 0) {
      LOAcquire();
      iVar3 = *piStack_60;
      *piStack_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000d93cc;
    }
    func_0x00287df8(piStack_60,2,8);
  }
LAB_000d93cc:
  uVar5 = *(undefined8 *)(param_1 + 0x128);
  piStack_60 = (int *)&DAT_001cb46c;
  uStack_58 = 0;
  puStack_70 = &DAT_000db188;
  uStack_68 = 0;
  puVar6 = (undefined4 *)__Znwm(0x20);
  *puVar6 = 1;
  *(undefined **)(puVar6 + 2) = &DAT_000de690;
  *(undefined **)(puVar6 + 4) = &DAT_000db188;
  *(undefined8 *)(puVar6 + 6) = 0;
  func_0x003d6598(auStack_78,uVar5,&piStack_60,param_1,&puStack_70,puVar6,0,0,&UNK_013bd198);
  func_0x003d24d8(auStack_78);
  uVar1 = *(uint *)(&UNK_01076438 + lRam000000000141be30);
  bVar2 = uRam000000000141be28 != 0;
  uRam000000000141be28 = uVar1;
  if (bVar2) {
    uRam000000000141be28 = uVar1 >> 0x13 | uVar1 << 0xd;
  }
  *(undefined4 *)(&UNK_0000d6a8 + param_1) = 0;
  uVar5 = *(undefined8 *)(*(long *)(&UNK_0000d6b8 + param_1) + 0x90);
  piStack_60 = (int *)&DAT_001c6b8c;
  uStack_58 = 0;
  puStack_70 = &DAT_000db1a0;
  uStack_68 = 0;
  puVar6 = (undefined4 *)__Znwm(0x20);
  *puVar6 = 1;
  *(undefined **)(puVar6 + 2) = &DAT_000de708;
  *(undefined **)(puVar6 + 4) = &DAT_000db1a0;
  *(undefined8 *)(puVar6 + 6) = 0;
  func_0x003d6598(auStack_80,uVar5,&piStack_60,param_1,&puStack_70,puVar6,0,0,&UNK_013ba120);
  func_0x003d24d8(auStack_80);
  uVar5 = *(undefined8 *)(*(long *)(&UNK_0000d6b8 + param_1) + 0x98);
  piStack_60 = (int *)&DAT_001c6d54;
  uStack_58 = 0;
  puStack_70 = &DAT_000db328;
  uStack_68 = 0;
  puVar6 = (undefined4 *)__Znwm(0x20);
  *puVar6 = 1;
  *(undefined **)(puVar6 + 2) = &DAT_000de708;
  *(undefined **)(puVar6 + 4) = &DAT_000db328;
  *(undefined8 *)(puVar6 + 6) = 0;
  func_0x003d6598(auStack_88,uVar5,&piStack_60,param_1,&puStack_70,puVar6,0,0,&UNK_013ba150);
  func_0x003d24d8(auStack_88);
  uVar5 = *(undefined8 *)(*(long *)(&UNK_0000d6b8 + param_1) + 0x98);
  piStack_60 = (int *)&DAT_00872c64;
  uStack_58 = 0;
  puStack_70 = &DAT_000db4e4;
  uStack_68 = 0;
  puVar6 = (undefined4 *)__Znwm(0x20);
  *puVar6 = 1;
  *(undefined **)(puVar6 + 2) = &DAT_000de708;
  *(undefined **)(puVar6 + 4) = &DAT_000db4e4;
  *(undefined8 *)(puVar6 + 6) = 0;
  func_0x003d6598(auStack_90,uVar5,&piStack_60,param_1,&puStack_70,puVar6,0,0,&UNK_013f2658);
  func_0x003d24d8(auStack_90);
  uVar5 = *(undefined8 *)(*(long *)(&UNK_0000d6b8 + param_1) + 0x80);
  piStack_60 = (int *)&DAT_001c7848;
  uStack_58 = 0;
  puStack_70 = &DAT_000db5d8;
  uStack_68 = 0;
  puVar6 = (undefined4 *)__Znwm(0x20);
  *puVar6 = 1;
  *(undefined **)(puVar6 + 2) = &DAT_000de77c;
  *(undefined **)(puVar6 + 4) = &DAT_000db5d8;
  *(undefined8 *)(puVar6 + 6) = 0;
  func_0x003d6598(auStack_98,uVar5,&piStack_60,param_1,&puStack_70,puVar6,0,0,&UNK_013ba6e0);
  func_0x003d24d8(auStack_98);
  uVar5 = *(undefined8 *)(param_1 + 0x140);
  piStack_60 = (int *)func_0x002efa1c("<div style=\"margin-bottom: 4px;\">Currently working in <b><nobr>Beginner Mode</nobr></b>.</div><div>For more control switch to <b><nobr><a href=dummy style=\"color: %1; text-decoration: none\">Advanced Mode</a></nobr></b>.</div>"
                                      ,0xe1);
  func_0x0002bbbc(uVar5,&piStack_60);
  if (*piStack_60 != -1) {
    if (*piStack_60 != 0) {
      LOAcquire();
      iVar3 = *piStack_60;
      *piStack_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000d9684;
    }
    func_0x00287df8(piStack_60,2,8);
  }
LAB_000d9684:
  uVar5 = *(undefined8 *)(param_1 + 0x140);
  piStack_60 = (int *)func_0x002efa1c("<b>Advanced Mode</b> offers more control over noise filter and allows to give preference to higher quality vs higher speed of processing.<br><br>Switch to Advanced Mode to be able to utilize Neat Video to its full potential."
                                      ,0xe0);
  func_0x0002bbc4(uVar5,&piStack_60);
  if (*piStack_60 != -1) {
    if (*piStack_60 != 0) {
      LOAcquire();
      iVar3 = *piStack_60;
      *piStack_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000d96dc;
    }
    func_0x00287df8(piStack_60,2,8);
  }
LAB_000d96dc:
  func_0x00753798(*(undefined8 *)(param_1 + 0x140));
  uVar5 = *(undefined8 *)(param_1 + 0x138);
  piStack_60 = (int *)func_0x002efa1c("<div>How to use: <b><nobr><a href=dummy style=\"color: %1; text-decoration: none\">Quick Start Guide</a></nobr></b>.</div>"
                                      ,0x78);
  func_0x0002bbbc(uVar5,&piStack_60);
  if (*piStack_60 != -1) {
    if (*piStack_60 != 0) {
      LOAcquire();
      iVar3 = *piStack_60;
      *piStack_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000d973c;
    }
    func_0x00287df8(piStack_60,2,8);
  }
LAB_000d973c:
  uVar5 = *(undefined8 *)(param_1 + 0x138);
  piStack_60 = (int *)func_0x002efa1c("<b>Quick Start Guide</b> demonstrates the basics of applying Neat Video to a noisy clip."
                                      ,0x58);
  func_0x0002bbc4(uVar5,&piStack_60);
  if (*piStack_60 != -1) {
    if (*piStack_60 != 0) {
      LOAcquire();
      iVar3 = *piStack_60;
      *piStack_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_000d9794;
    }
    func_0x00287df8(piStack_60,2,8);
  }
LAB_000d9794:
  uVar5 = *(undefined8 *)(*(long *)(&UNK_0000d6b8 + param_1) + 0x88);
  piStack_60 = (int *)&DAT_007d58c8;
  uStack_58 = 0;
  puVar6 = (undefined4 *)__Znwm(0x18);
  *puVar6 = 1;
  *(undefined **)(puVar6 + 2) = &DAT_000de7f8;
  *(long *)(puVar6 + 4) = param_1;
  func_0x003d6598(auStack_a0,uVar5,&piStack_60,param_1,0,puVar6,0,0,&UNK_013e8a80);
  func_0x003d24d8(auStack_a0);
  uVar5 = *(undefined8 *)(param_1 + 0xa8);
  piStack_60 = (int *)&DAT_00755080;
  uStack_58 = 0;
  puStack_70 = &DAT_00031cf0;
  uStack_68 = 0;
  puVar6 = (undefined4 *)__Znwm(0x20);
  *puVar6 = 1;
  *(undefined **)(puVar6 + 2) = &DAT_0003d5dc;
  *(undefined **)(puVar6 + 4) = &DAT_00031cf0;
  *(undefined8 *)(puVar6 + 6) = 0;
  func_0x003d6598(auStack_a8,uVar5,&piStack_60,uVar8,&puStack_70,puVar6,0,0,&UNK_013e4888);
  func_0x003d24d8(auStack_a8);
  return param_1;
}


