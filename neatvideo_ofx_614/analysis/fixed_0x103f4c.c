// func entry: 00103e10 name: FUN_00103e10 size: 688

void FUN_00103e10(undefined8 param_1,long param_2)

{
  undefined8 *puVar1;
  uint uVar2;
  int iVar3;
  long *plVar4;
  uint *puVar5;
  int *local_60;
  int *local_58;
  int *local_50;
  int *local_48;
  int *local_40;
  uint *local_38;
  
  plVar4 = (long *)(*(long *)(param_2 + 0x430) + 0x10);
  iVar3 = (**(code **)(*plVar4 + 0x20))(plVar4,0xc);
  puVar1 = &DAT_0155b2f0;
  if ((double)iVar3 * 1.52587890625e-05 <= 1.0) {
    puVar1 = &DAT_0155b2e8;
  }
  puVar5 = (uint *)*puVar1;
  if (1 < *puVar5 + 1) {
    LOAcquire();
    *puVar5 = *puVar5 + 1;
    LORelease();
  }
  uVar2 = *puVar5 + 1;
  if (1 < uVar2) {
    LOAcquire();
    uVar2 = *puVar5;
    *puVar5 = uVar2 + 1;
    LORelease();
  }
  local_38 = puVar5;
  FUN_002e97cc(uVar2,&local_38,&DAT_0155b450);
  if (*puVar5 != 0xffffffff) {
    if (*puVar5 != 0) {
      LOAcquire();
      uVar2 = *puVar5;
      *puVar5 = uVar2 - 1;
      LORelease();
      if (uVar2 != 1) goto LAB_00103ee4;
    }
    FUN_00287df8(puVar5,2,8);
  }
LAB_00103ee4:
  local_58 = (int *)FUN_002efa1c("<table><tr><td><img src=\"%1\" width=%2 height=%3/></td><td>&nbsp;%4</td></tr></table>"
                                 ,0x54);
  FUN_002f4ef8(&local_50,&local_58,&local_38,0,0x20);
  FUN_002f58f8(&local_48,&local_50,(long)*(int *)(param_2 + 0x448),0,10,0x20);
  FUN_002f58f8(&local_40,&local_48,(long)*(int *)(param_2 + 0x44c),0,10,0x20);
  local_60 = (int *)FUN_002efa1c("Selected combination is not available (limitation of the Demo edition)"
                                 ,0x46);
  FUN_002f4ef8(param_1,&local_40,&local_60,0,0x20);
  if (*local_60 != -1) {
    if (*local_60 != 0) {
      LOAcquire();
      iVar3 = *local_60;
      *local_60 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_00103fa8;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_00103fa8:
  if (*local_40 != -1) {
    if (*local_40 != 0) {
      LOAcquire();
      iVar3 = *local_40;
      *local_40 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_00103fdc;
    }
    FUN_00287df8(local_40,2,8);
  }
LAB_00103fdc:
  if (*local_48 != -1) {
    if (*local_48 != 0) {
      LOAcquire();
      iVar3 = *local_48;
      *local_48 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_00104010;
    }
    FUN_00287df8(local_48,2,8);
  }
LAB_00104010:
  if (*local_50 != -1) {
    if (*local_50 != 0) {
      LOAcquire();
      iVar3 = *local_50;
      *local_50 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_00104044;
    }
    FUN_00287df8(local_50,2,8);
  }
LAB_00104044:
  if (*local_58 != -1) {
    if (*local_58 != 0) {
      LOAcquire();
      iVar3 = *local_58;
      *local_58 = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) goto LAB_00104078;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_00104078:
  if (*local_38 != 0xffffffff) {
    if (*local_38 != 0) {
      LOAcquire();
      uVar2 = *local_38;
      *local_38 = uVar2 - 1;
      LORelease();
      if (uVar2 != 1) {
        return;
      }
    }
    FUN_00287df8(local_38,2,8);
  }
  return;
}


