// FUN_001576c0 size=1176

void FUN_001576c0(undefined8 *param_1,undefined8 *param_2,long param_3)

{
  char *pcVar1;
  char *pcVar2;
  float *pfVar3;
  float *pfVar4;
  uint uVar5;
  char cVar6;
  bool bVar7;
  int iVar8;
  size_t sVar9;
  undefined8 *puVar10;
  ulong uVar11;
  float fVar12;
  undefined8 uVar13;
  undefined4 *local_60;
  int *local_58;
  
  uVar13 = *param_2;
  param_1[1] = param_2[1];
  *param_1 = uVar13;
  *(undefined1 *)(param_1 + 8) = 0;
  (**(code **)(*(long *)param_1[2] + 0x18))();
  FUN_009c7318();
  pcVar1 = (char *)((long)param_1 + 0x19);
  pcVar2 = (char *)((long)param_1 + 0x1a);
  FUN_009b01d4(*(undefined8 *)(param_3 + 0x538),param_1[2],param_1 + 3,pcVar1,pcVar2,
               (long)param_1 + 0x1c,0);
  local_60 = &DAT_0142f0e0;
  puVar10 = param_1 + 6;
  local_58 = (int *)*puVar10;
  *puVar10 = &DAT_0142f0e0;
  FUN_001c4af0(&local_58);
  FUN_001c4af0(&local_60);
  if (*(char *)(param_1 + 3) == '\0') {
    uVar11 = (ulong)*(uint *)(*(long *)(*(long *)(param_3 + 0x350) + 0x28) + 0x48);
    sVar9 = _strlen(&DAT_0147dffc + uVar11 * 0x58);
    local_58 = (int *)FUN_002efa1c(&DAT_0147dffc + uVar11 * 0x58,sVar9);
    FUN_001c25a8(puVar10,&local_58);
    if (*local_58 != -1) {
      if (*local_58 != 0) {
        LOAcquire();
        iVar8 = *local_58;
        *local_58 = iVar8 + -1;
        LORelease();
        if (iVar8 != 1) goto LAB_00157834;
      }
      FUN_00287df8(local_58,2,8);
    }
LAB_00157834:
    cVar6 = *pcVar1;
joined_r0x00157838:
    if (cVar6 == '\0') {
      sVar9 = _strlen(&DAT_0147e00b + uVar11 * 0x58);
      local_58 = (int *)FUN_002efa1c(&DAT_0147e00b + uVar11 * 0x58,sVar9);
      FUN_001c25a8(puVar10,&local_58);
      if (*local_58 != -1) {
        if (*local_58 != 0) {
          LOAcquire();
          iVar8 = *local_58;
          *local_58 = iVar8 + -1;
          LORelease();
          if (iVar8 != 1) goto LAB_001578a8;
        }
        FUN_00287df8(local_58,2,8);
      }
    }
LAB_001578a8:
    if (*pcVar2 == '\0') {
      sVar9 = _strlen(&DAT_0147e01a + uVar11 * 0x58);
      local_58 = (int *)FUN_002efa1c(&DAT_0147e01a + uVar11 * 0x58,sVar9);
      FUN_001c25a8(puVar10,&local_58);
      if (*local_58 != -1) {
        if (*local_58 != 0) {
          LOAcquire();
          iVar8 = *local_58;
          *local_58 = iVar8 + -1;
          LORelease();
          if (iVar8 != 1) goto LAB_0015791c;
        }
        FUN_00287df8(local_58,2,8);
      }
    }
  }
  else {
    cVar6 = *pcVar1;
    if ((cVar6 == '\0') || (*pcVar2 == '\0')) {
      uVar11 = (ulong)*(uint *)(*(long *)(*(long *)(param_3 + 0x350) + 0x28) + 0x48);
      goto joined_r0x00157838;
    }
  }
LAB_0015791c:
  uVar5 = *(uint *)(DAT_0141c528 + 0x9d47a0);
  bVar7 = DAT_0141c520 != 0;
  DAT_0141c520 = uVar5;
  if (bVar7) {
    DAT_0141c520 = uVar5 >> 0x10 | uVar5 << 0x10;
  }
  pfVar3 = (float *)((long)param_1 + 0x24);
  pfVar4 = (float *)(param_1 + 5);
  FUN_009c6648(param_1[2],param_1 + 4,pfVar3,pfVar4,1);
  local_60 = &DAT_0142f0e0;
  puVar10 = param_1 + 7;
  local_58 = (int *)*puVar10;
  *puVar10 = &DAT_0142f0e0;
  FUN_001c4af0(&local_58);
  FUN_001c4af0(&local_60);
  if (*(float *)(param_1 + 4) <= 0.33) {
    fVar12 = *pfVar3;
    if ((fVar12 <= 0.33) && (*pfVar4 <= 0.33)) goto LAB_00157b34;
  }
  else {
    sVar9 = _strlen(&DAT_0147dffc);
    local_58 = (int *)FUN_002efa1c(&DAT_0147dffc,sVar9);
    FUN_001c25a8(puVar10,&local_58);
    if (*local_58 != -1) {
      if (*local_58 != 0) {
        LOAcquire();
        iVar8 = *local_58;
        *local_58 = iVar8 + -1;
        LORelease();
        if (iVar8 != 1) goto LAB_00157a08;
      }
      FUN_00287df8(local_58,2,8);
    }
LAB_00157a08:
    fVar12 = *pfVar3;
  }
  if (0.33 < fVar12) {
    sVar9 = _strlen(&DAT_0147e00b);
    local_58 = (int *)FUN_002efa1c(&DAT_0147e00b,sVar9);
    FUN_001c25a8(puVar10,&local_58);
    if (*local_58 != -1) {
      if (*local_58 != 0) {
        LOAcquire();
        iVar8 = *local_58;
        *local_58 = iVar8 + -1;
        LORelease();
        if (iVar8 != 1) goto LAB_00157ab8;
      }
      FUN_00287df8(local_58,2,8);
    }
  }
LAB_00157ab8:
  if (0.33 < *pfVar4) {
    sVar9 = _strlen(&DAT_0147e01a);
    local_58 = (int *)FUN_002efa1c(&DAT_0147e01a,sVar9);
    FUN_001c25a8(puVar10,&local_58);
    if (*local_58 != -1) {
      if (*local_58 != 0) {
        LOAcquire();
        iVar8 = *local_58;
        *local_58 = iVar8 + -1;
        LORelease();
        if (iVar8 != 1) goto LAB_00157b34;
      }
      FUN_00287df8(local_58,2,8);
    }
  }
LAB_00157b34:
  *(undefined1 *)(param_1 + 8) = 1;
  return;
}


