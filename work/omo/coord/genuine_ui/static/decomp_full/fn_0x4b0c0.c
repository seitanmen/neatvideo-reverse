// FUN_0004b0c0 size=1120

void FUN_0004b0c0(undefined1 param_1 [16],undefined1 param_2 [16],double param_3,long *param_4,
                 long *param_5,long *param_6)

{
  int iVar1;
  bool bVar2;
  bool bVar3;
  uint uVar4;
  int iVar5;
  int *piVar6;
  uint *puVar7;
  int *unaff_x19;
  long lVar8;
  int *local_88;
  int *local_80;
  int *local_78;
  int *local_70;
  uint *local_68;
  uint *local_60;
  uint *local_58;
  
  iVar5 = *(int *)(*param_5 + 8);
  iVar1 = *(int *)(*param_5 + 0xc);
  if ((iVar1 == iVar5) && (*(int *)(*param_6 + 0xc) == *(int *)(*param_6 + 8))) {
    FUN_00046d2c(param_4[0x43],0);
    FUN_009299d4(param_4[0x59] + 0x10,0);
    FUN_009299d4(param_4[0x5a] + 0x10,0);
    return;
  }
  lVar8 = param_4[0x43];
  piVar6 = &DAT_0142f030;
  if (iVar1 == iVar5) {
    bVar2 = false;
    local_68 = &DAT_0142f030;
    local_78 = &DAT_0142f030;
  }
  else {
    unaff_x19 = (int *)FUN_002efa1c(", ",2);
    FUN_002faa10(&local_70,param_5,(long)unaff_x19 + *(long *)(unaff_x19 + 4),unaff_x19[1]);
    FUN_002efbd0(&local_68,"Not Uniform: ",0xd);
    FUN_002e97cc(&local_68,&local_70);
    if ((*(int *)(*param_5 + 0xc) == *(int *)(*param_5 + 8)) ||
       (*(int *)(*param_6 + 0xc) == *(int *)(*param_6 + 8))) {
      bVar2 = true;
      local_78 = &DAT_0142f030;
    }
    else {
      local_78 = (int *)FUN_002efa1c("\n",1);
      bVar2 = true;
    }
  }
  local_60 = local_68;
  puVar7 = local_68;
  if (1 < *local_68 + 1) {
    LOAcquire();
    uVar4 = *local_68;
    *local_68 = uVar4 + 1;
    puVar7 = (uint *)(ulong)uVar4;
    LORelease();
  }
  FUN_002e97cc(puVar7,&local_60,&local_78);
  bVar3 = *(int *)(*param_6 + 0xc) != *(int *)(*param_6 + 8);
  if (bVar3) {
    piVar6 = (int *)FUN_002efa1c(", ",2);
    FUN_002faa10(&local_88,param_6,(long)piVar6 + *(long *)(piVar6 + 4),piVar6[1]);
    FUN_002efbd0(&local_80,"Clipping: ",10);
    FUN_002e97cc(&local_80,&local_88);
  }
  else {
    local_80 = &DAT_0142f030;
  }
  local_58 = local_60;
  puVar7 = local_60;
  if (1 < *local_60 + 1) {
    LOAcquire();
    uVar4 = *local_60;
    *local_60 = uVar4 + 1;
    puVar7 = (uint *)(ulong)uVar4;
    LORelease();
  }
  FUN_002e97cc(puVar7,&local_58,&local_80);
  (**(code **)(*(long *)(param_4[0x59] + 0x10) + 0x18))();
  FUN_00046d70(lVar8,&local_58,(int)(param_3 + 10.0));
  if (*local_58 != 0xffffffff) {
    if (*local_58 != 0) {
      LOAcquire();
      uVar4 = *local_58;
      *local_58 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_0004b320;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_0004b320:
  if (*local_80 != -1) {
    if (*local_80 != 0) {
      LOAcquire();
      iVar5 = *local_80;
      *local_80 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_0004b354;
    }
    FUN_00287df8(local_80,2,8);
  }
LAB_0004b354:
  if (bVar3) {
    if (*local_88 != -1) {
      if (*local_88 != 0) {
        LOAcquire();
        iVar5 = *local_88;
        *local_88 = iVar5 + -1;
        LORelease();
        if (iVar5 != 1) goto LAB_0004b38c;
      }
      FUN_00287df8(local_88,2,8);
    }
LAB_0004b38c:
    if (*piVar6 != -1) {
      if (*piVar6 != 0) {
        LOAcquire();
        iVar5 = *piVar6;
        *piVar6 = iVar5 + -1;
        LORelease();
        if (iVar5 != 1) goto LAB_0004b3bc;
      }
      FUN_00287df8(piVar6,2,8);
    }
  }
LAB_0004b3bc:
  if (*local_60 != 0xffffffff) {
    if (*local_60 != 0) {
      LOAcquire();
      uVar4 = *local_60;
      *local_60 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_0004b3f0;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_0004b3f0:
  if (*local_78 != -1) {
    if (*local_78 != 0) {
      LOAcquire();
      iVar5 = *local_78;
      *local_78 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_0004b424;
    }
    FUN_00287df8(local_78,2,8);
  }
LAB_0004b424:
  if (*local_68 != 0xffffffff) {
    if (*local_68 != 0) {
      LOAcquire();
      uVar4 = *local_68;
      *local_68 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_0004b458;
    }
    FUN_00287df8(local_68,2,8);
  }
LAB_0004b458:
  if (!bVar2) goto LAB_0004b4c0;
  if (*local_70 != -1) {
    if (*local_70 != 0) {
      LOAcquire();
      iVar5 = *local_70;
      *local_70 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_0004b490;
    }
    FUN_00287df8(local_70,2,8);
  }
LAB_0004b490:
  if (*unaff_x19 != -1) {
    if (*unaff_x19 != 0) {
      LOAcquire();
      iVar5 = *unaff_x19;
      *unaff_x19 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_0004b4c0;
    }
    FUN_00287df8(unaff_x19,2,8);
  }
LAB_0004b4c0:
  FUN_00046d2c(param_4[0x43],1);
  *(bool *)(param_4 + 0x5b) = *(int *)(*param_5 + 0xc) != *(int *)(*param_5 + 8);
  *(bool *)((long)param_4 + 0x2d9) = *(int *)(*param_6 + 0xc) != *(int *)(*param_6 + 8);
  (**(code **)(*param_4 + 0x1e8))(param_4);
  return;
}


