// FUN_0004e878 size=236

void FUN_0004e878(long *param_1,long param_2)

{
  int iVar1;
  int iVar2;
  double dVar3;
  undefined8 local_28;
  
  if ((*(char *)((long)param_1 + 0x1a9) != '\0') && (*(int *)(param_2 + 0x50) == 1)) {
    dVar3 = *(double *)(param_2 + 0x20);
    if (0.0 <= dVar3) {
      iVar1 = (int)(dVar3 + 0.5);
    }
    else {
      iVar1 = (int)((dVar3 - (double)(int)(dVar3 + -1.0)) + 0.5) + (int)(dVar3 + -1.0);
    }
    dVar3 = *(double *)(param_2 + 0x28);
    if (0.0 <= dVar3) {
      iVar2 = (int)(dVar3 + 0.5);
    }
    else {
      iVar2 = (int)((dVar3 - (double)(int)(dVar3 + -1.0)) + 0.5) + (int)(dVar3 + -1.0);
    }
    local_28 = CONCAT44(iVar2,iVar1);
    (**(code **)(*param_1 + 0x2e0))(param_1,&local_28);
    *(ushort *)(param_2 + 0x12) = *(ushort *)(param_2 + 0x12) | 4;
    *(undefined1 *)((long)param_1 + 0x22a) = 0;
    (**(code **)(*param_1 + 0x1e8))(param_1);
  }
  return;
}


