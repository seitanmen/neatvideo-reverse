// FUN_00051b10 size=680

void FUN_00051b10(undefined1 param_1 [16],double param_2,double param_3,double param_4,long param_5)

{
  bool bVar1;
  uint uVar2;
  int iVar3;
  long lVar4;
  uint *puVar5;
  undefined8 uVar6;
  double dVar7;
  double dVar8;
  uint *local_98;
  uint *local_90;
  int *local_88 [11];
  
  if (*(long *)(param_5 + 0x148) == 0) {
    FUN_009299d4(*(undefined8 *)(param_5 + 0xd8),0);
    return;
  }
  uVar2 = *(uint *)(DAT_0141a8b0 + 0x1a15690);
  bVar1 = DAT_0141a8a8 != 0;
  DAT_0141a8a8 = uVar2;
  if (bVar1) {
    DAT_0141a8a8 = uVar2 >> 0x15 | uVar2 << 0xb;
  }
  if (DAT_0141a8a8 != 0) {
    *(uint *)(DAT_0141a8b8 + 0x16dc800) = DAT_0141a8a8;
  }
  if (DAT_01562f68 != 0) {
    DAT_0141a8a8 = DAT_0141a8a8 + *(byte *)(DAT_01562f68 + 0xcee6);
  }
  dVar7 = (double)FUN_0005102c(param_5,0);
  lVar4 = *(long *)(param_5 + 0x148);
  if (lVar4 == 0) {
    dVar8 = 0.0;
    iVar3 = 0;
  }
  else {
    iVar3 = 0;
    if (*(long *)(lVar4 + 8) != 0) {
      iVar3 = *(int *)(*(long *)(lVar4 + 8) + 0xd0);
    }
    dVar8 = (double)iVar3;
    if (dVar7 + param_3 <= (double)iVar3) {
      dVar8 = dVar7 + param_3;
    }
    if (dVar8 <= 0.0) {
      dVar8 = 0.0;
    }
    iVar3 = 0;
    if (*(long *)(lVar4 + 8) != 0) {
      iVar3 = *(int *)(*(long *)(lVar4 + 8) + 0xd8);
    }
  }
  dVar7 = (double)iVar3;
  if (param_2 + param_4 <= (double)iVar3) {
    dVar7 = param_2 + param_4;
  }
  if (dVar7 <= 0.0) {
    dVar7 = 0.0;
  }
  uVar6 = *(undefined8 *)(param_5 + 0xd8);
  FUN_00965aa0(local_88,param_5);
  FUN_00056b04(local_88[0],uVar6);
  FUN_000568b4(dVar8,dVar7,*(undefined8 *)(param_5 + 0xd8),3);
  if (*(int *)(param_5 + 0xcc) == 0) {
    FUN_009299d4(*(undefined8 *)(param_5 + 0xd8),0);
    return;
  }
  FUN_00042c44(local_88);
  if (*(char *)(param_5 + 0x6b) == '\0') {
    uVar6 = *(undefined8 *)(param_5 + 0xd8);
  }
  else {
    uVar6 = *(undefined8 *)(param_5 + 0xd8);
    if (*(long *)(param_5 + 0x148) != 0) {
      FUN_003bbff0(&local_98,&DAT_013c1668,"Updating - ",0,0xffffffff);
      local_90 = local_98;
      puVar5 = local_98;
      if (1 < *local_98 + 1) {
        LOAcquire();
        uVar2 = *local_98;
        *local_98 = uVar2 + 1;
        puVar5 = (uint *)(ulong)uVar2;
        LORelease();
      }
      FUN_002e97cc(puVar5,&local_90,local_88);
      FUN_000567e4(uVar6,&local_90);
      if (*local_90 != 0xffffffff) {
        if (*local_90 != 0) {
          LOAcquire();
          uVar2 = *local_90;
          *local_90 = uVar2 - 1;
          LORelease();
          if (uVar2 != 1) goto LAB_00051cf4;
        }
        FUN_00287df8(local_90,2,8);
      }
LAB_00051cf4:
      if (*local_98 != 0xffffffff) {
        if (*local_98 != 0) {
          LOAcquire();
          uVar2 = *local_98;
          *local_98 = uVar2 - 1;
          LORelease();
          if (uVar2 != 1) goto LAB_00051d4c;
        }
        FUN_00287df8(local_98,2,8);
      }
      goto LAB_00051d4c;
    }
  }
  FUN_000567e4(uVar6,local_88);
LAB_00051d4c:
  if (*(char *)(param_5 + 0x69) == '\0') {
    bVar1 = *(int *)(param_5 + 200) < 1;
  }
  else {
    bVar1 = false;
  }
  FUN_009299d4(*(undefined8 *)(param_5 + 0xd8),bVar1);
  if (*local_88[0] != -1) {
    if (*local_88[0] != 0) {
      LOAcquire();
      iVar3 = *local_88[0];
      *local_88[0] = iVar3 + -1;
      LORelease();
      if (iVar3 != 1) {
        return;
      }
    }
    FUN_00287df8(local_88[0],2,8);
  }
  return;
}


