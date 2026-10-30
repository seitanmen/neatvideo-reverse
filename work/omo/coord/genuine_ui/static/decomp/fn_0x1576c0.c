// FUN_001576c0 size=1176

void FUN_001576c0(undefined8 *param_1,undefined8 *param_2,long param_3)

{
  char *pcVar1;
  char *pcVar2;
  long lVar3;
  float *pfVar4;
  float *pfVar5;
  uint uVar6;
  char cVar7;
  bool bVar8;
  int iVar9;
  undefined8 uVar10;
  undefined8 *puVar11;
  ulong uVar12;
  float fVar13;
  undefined8 uStack_60;
  int *piStack_58;
  
  uVar10 = *param_2;
  param_1[1] = param_2[1];
  *param_1 = uVar10;
  *(undefined1 *)(param_1 + 8) = 0;
  (**(code **)(*(long *)param_1[2] + 0x18))();
  func_0x009c7318();
  pcVar1 = (char *)((long)param_1 + 0x19);
  pcVar2 = (char *)((long)param_1 + 0x1a);
  func_0x009b01d4(*(undefined8 *)(param_3 + 0x538),param_1[2],param_1 + 3,pcVar1,pcVar2,
                  (long)param_1 + 0x1c,0);
  uStack_60 = 0x142f0e0;
  puVar11 = param_1 + 6;
  piStack_58 = (int *)*puVar11;
  *puVar11 = 0x142f0e0;
  func_0x001c4af0(&piStack_58);
  func_0x001c4af0(&uStack_60);
  if (*(char *)(param_1 + 3) == '\0') {
    uVar12 = (ulong)*(uint *)(*(long *)(*(long *)(param_3 + 0x350) + 0x28) + 0x48);
    lVar3 = uVar12 * 0x58 + 0x147dffc;
    uVar10 = _strlen(lVar3);
    piStack_58 = (int *)func_0x002efa1c(lVar3,uVar10);
    func_0x001c25a8(puVar11,&piStack_58);
    if (*piStack_58 != -1) {
      if (*piStack_58 != 0) {
        LOAcquire();
        iVar9 = *piStack_58;
        *piStack_58 = iVar9 + -1;
        LORelease();
        if (iVar9 != 1) goto LAB_00157834;
      }
      func_0x00287df8(piStack_58,2,8);
    }
LAB_00157834:
    cVar7 = *pcVar1;
joined_r0x00157838:
    if (cVar7 == '\0') {
      lVar3 = uVar12 * 0x58 + 0x147e00b;
      uVar10 = _strlen(lVar3);
      piStack_58 = (int *)func_0x002efa1c(lVar3,uVar10);
      func_0x001c25a8(puVar11,&piStack_58);
      if (*piStack_58 != -1) {
        if (*piStack_58 != 0) {
          LOAcquire();
          iVar9 = *piStack_58;
          *piStack_58 = iVar9 + -1;
          LORelease();
          if (iVar9 != 1) goto LAB_001578a8;
        }
        func_0x00287df8(piStack_58,2,8);
      }
    }
LAB_001578a8:
    if (*pcVar2 == '\0') {
      lVar3 = uVar12 * 0x58 + 0x147e01a;
      uVar10 = _strlen(lVar3);
      piStack_58 = (int *)func_0x002efa1c(lVar3,uVar10);
      func_0x001c25a8(puVar11,&piStack_58);
      if (*piStack_58 != -1) {
        if (*piStack_58 != 0) {
          LOAcquire();
          iVar9 = *piStack_58;
          *piStack_58 = iVar9 + -1;
          LORelease();
          if (iVar9 != 1) goto LAB_0015791c;
        }
        func_0x00287df8(piStack_58,2,8);
      }
    }
  }
  else {
    cVar7 = *pcVar1;
    if ((cVar7 == '\0') || (*pcVar2 == '\0')) {
      uVar12 = (ulong)*(uint *)(*(long *)(*(long *)(param_3 + 0x350) + 0x28) + 0x48);
      goto joined_r0x00157838;
    }
  }
LAB_0015791c:
  uVar6 = *(uint *)(lRam000000000141c528 + 0x9d47a0);
  bVar8 = uRam000000000141c520 != 0;
  uRam000000000141c520 = uVar6;
  if (bVar8) {
    uRam000000000141c520 = uVar6 >> 0x10 | uVar6 << 0x10;
  }
  pfVar4 = (float *)((long)param_1 + 0x24);
  pfVar5 = (float *)(param_1 + 5);
  func_0x009c6648(param_1[2],param_1 + 4,pfVar4,pfVar5,1);
  uStack_60 = 0x142f0e0;
  puVar11 = param_1 + 7;
  piStack_58 = (int *)*puVar11;
  *puVar11 = 0x142f0e0;
  func_0x001c4af0(&piStack_58);
  func_0x001c4af0(&uStack_60);
  if (*(float *)(param_1 + 4) <= 0.33) {
    fVar13 = *pfVar4;
    if ((fVar13 <= 0.33) && (*pfVar5 <= 0.33)) goto LAB_00157b34;
  }
  else {
    uVar10 = _strlen(0x147dffc);
    piStack_58 = (int *)func_0x002efa1c(0x147dffc,uVar10);
    func_0x001c25a8(puVar11,&piStack_58);
    if (*piStack_58 != -1) {
      if (*piStack_58 != 0) {
        LOAcquire();
        iVar9 = *piStack_58;
        *piStack_58 = iVar9 + -1;
        LORelease();
        if (iVar9 != 1) goto LAB_00157a08;
      }
      func_0x00287df8(piStack_58,2,8);
    }
LAB_00157a08:
    fVar13 = *pfVar4;
  }
  if (0.33 < fVar13) {
    uVar10 = _strlen(0x147e00b);
    piStack_58 = (int *)func_0x002efa1c(0x147e00b,uVar10);
    func_0x001c25a8(puVar11,&piStack_58);
    if (*piStack_58 != -1) {
      if (*piStack_58 != 0) {
        LOAcquire();
        iVar9 = *piStack_58;
        *piStack_58 = iVar9 + -1;
        LORelease();
        if (iVar9 != 1) goto LAB_00157ab8;
      }
      func_0x00287df8(piStack_58,2,8);
    }
  }
LAB_00157ab8:
  if (0.33 < *pfVar5) {
    uVar10 = _strlen(0x147e01a);
    piStack_58 = (int *)func_0x002efa1c(0x147e01a,uVar10);
    func_0x001c25a8(puVar11,&piStack_58);
    if (*piStack_58 != -1) {
      if (*piStack_58 != 0) {
        LOAcquire();
        iVar9 = *piStack_58;
        *piStack_58 = iVar9 + -1;
        LORelease();
        if (iVar9 != 1) goto LAB_00157b34;
      }
      func_0x00287df8(piStack_58,2,8);
    }
  }
LAB_00157b34:
  *(undefined1 *)(param_1 + 8) = 1;
  return;
}


