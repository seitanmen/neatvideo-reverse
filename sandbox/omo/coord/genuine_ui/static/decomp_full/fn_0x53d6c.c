// FUN_00053d6c size=556

void FUN_00053d6c(long param_1,long param_2)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  ulong uVar4;
  int iVar5;
  undefined8 *puVar6;
  int iVar7;
  undefined8 *puVar8;
  double dVar9;
  undefined8 local_38;
  
  dVar9 = *(double *)(param_2 + 0x20);
  if (0.0 <= dVar9) {
    iVar5 = (int)(dVar9 + 0.5);
  }
  else {
    iVar5 = (int)((dVar9 - (double)(int)(dVar9 + -1.0)) + 0.5) + (int)(dVar9 + -1.0);
  }
  dVar9 = *(double *)(param_2 + 0x28);
  if (0.0 <= dVar9) {
    iVar7 = (int)(dVar9 + 0.5);
  }
  else {
    iVar7 = (int)((dVar9 - (double)(int)(dVar9 + -1.0)) + 0.5) + (int)(dVar9 + -1.0);
  }
  local_38 = CONCAT44(iVar7,iVar5);
  uVar4 = FUN_00961ed8(param_1,&local_38);
  puVar6 = *(undefined8 **)(param_1 + 0x198);
  uVar2 = *(uint *)(puVar6 + 4);
  if (uVar2 != 0) {
    uVar1 = *(uint *)((long)puVar6 + 0x24) ^ (uint)(uVar4 >> 0x1f) ^ (uint)uVar4;
    uVar3 = 0;
    if (uVar2 != 0) {
      uVar3 = uVar1 / uVar2;
    }
    for (puVar8 = *(undefined8 **)(puVar6[1] + (ulong)(uVar1 - uVar3 * uVar2) * 8); puVar8 != puVar6
        ; puVar8 = (undefined8 *)*puVar8) {
      if ((*(uint *)(puVar8 + 1) == uVar1) && (puVar8[2] == uVar4)) {
        if (puVar8 != puVar6) goto LAB_00053ec4;
        break;
      }
    }
  }
  FUN_001d1cf0(param_1);
  if ((*(long *)(param_1 + 0x150) == 0) || (*(char *)(param_1 + 0x168) == '\0')) {
LAB_00053ec4:
    FUN_00964100(param_1,param_2);
  }
  else {
    *(ushort *)(param_2 + 0x12) = *(ushort *)(param_2 + 0x12) | 4;
    dVar9 = *(double *)(param_2 + 0x20);
    if (0.0 <= dVar9) {
      iVar5 = (int)(dVar9 + 0.5);
    }
    else {
      iVar5 = (int)((dVar9 - (double)(int)(dVar9 + -1.0)) + 0.5) + (int)(dVar9 + -1.0);
    }
    dVar9 = *(double *)(param_2 + 0x28);
    if (0.0 <= dVar9) {
      iVar7 = (int)(dVar9 + 0.5);
    }
    else {
      iVar7 = (int)((dVar9 - (double)(int)(dVar9 + -1.0)) + 0.5) + (int)(dVar9 + -1.0);
    }
    local_38 = CONCAT44(iVar7,iVar5);
    FUN_0095d6ec(param_1,&local_38);
    if ((*(char *)(param_1 + 0x69) != '\0') && (iVar5 = *(int *)(param_1 + 0x88), iVar5 != 2)) {
      if (*(int *)(param_2 + 0x50) == 4) {
        if (iVar5 != 1) {
          return;
        }
      }
      else {
        if (*(int *)(param_2 + 0x50) != 1) {
          return;
        }
        if (iVar5 != 0) {
          return;
        }
      }
      *(undefined1 *)(param_1 + 0x69) = 0;
      FUN_00051f04(param_1,1);
      FUN_00050548(param_1);
      FUN_001d1c0c(param_1);
    }
  }
  return;
}


