// FUN_0004e250 size=800

void FUN_0004e250(undefined1 param_1 [16],double param_2,double param_3,undefined8 param_4,
                 long *param_5,long param_6,char *param_7)

{
  int iVar1;
  int iVar2;
  bool bVar3;
  int iVar4;
  double dVar5;
  double local_70;
  double dStack_68;
  undefined8 local_60;
  double dStack_58;
  double local_50;
  undefined8 uStack_48;
  
  FUN_00053b10();
  if (*param_7 != '\0') {
    return;
  }
  iVar1 = (**(code **)(*param_5 + 0x208))(param_5);
  if (iVar1 != 0) {
    if (*(char *)((long)param_5 + 0x1a9) != '\0') {
      dVar5 = *(double *)(param_6 + 0x20);
      if (0.0 <= dVar5) {
        iVar1 = (int)(dVar5 + 0.5);
      }
      else {
        param_2 = (double)(int)(dVar5 + -1.0);
        iVar1 = (int)((dVar5 - param_2) + 0.5) + (int)(dVar5 + -1.0);
      }
      dVar5 = *(double *)(param_6 + 0x28);
      if (0.0 <= dVar5) {
        iVar4 = (int)(dVar5 + 0.5);
      }
      else {
        param_2 = (double)(int)(dVar5 + -1.0);
        iVar4 = (int)((dVar5 - param_2) + 0.5) + (int)(dVar5 + -1.0);
      }
      local_60 = CONCAT44(iVar4,iVar1);
      dVar5 = (double)FUN_0095d6ec(param_5,&local_60);
      if (0.0 <= dVar5) {
        iVar1 = (int)(dVar5 + 0.5);
      }
      else {
        param_3 = (double)(int)(dVar5 + -1.0);
        iVar1 = (int)((dVar5 - param_3) + 0.5) + (int)(dVar5 + -1.0);
      }
      if (0.0 <= param_2) {
        iVar4 = (int)(param_2 + 0.5);
      }
      else {
        iVar4 = (int)((param_2 - (double)(int)(param_2 + -1.0)) + 0.5) + (int)(param_2 + -1.0);
      }
      if (*(int *)(param_6 + 0x50) == 1) {
        iVar2 = (int)param_5[0x26];
        if ((iVar2 == -1) || (iVar2 == 1)) {
          if (iVar2 == 1) {
            local_60 = FUN_00939c10(param_5 + 0x27);
            local_70 = (double)iVar1;
            dStack_68 = (double)iVar4;
            dStack_58 = param_2;
            local_50 = param_3;
            uStack_48 = param_4;
            iVar2 = FUN_002acfa4(&local_60,&local_70);
            if (iVar2 != 0) {
              if ((char)param_5[0x3c] != '\0') {
                FUN_001d0fe4(param_5);
              }
              *(undefined1 *)((long)param_5 + 0x1e4) = 1;
              *(undefined1 *)((long)param_5 + 0x1e1) = 1;
              if ((int)param_5[0x3b] < (int)param_5[0x3a]) {
                bVar3 = false;
              }
              else {
                bVar3 = *(int *)((long)param_5 + 0x1d4) <= *(int *)((long)param_5 + 0x1dc);
              }
              *(bool *)((long)param_5 + 0x1e3) = bVar3;
              param_5[0x34] = CONCAT44(iVar4,iVar1);
              *(undefined1 *)(param_5 + 0x3c) = 0;
              *(int *)(param_5 + 0x3a) = iVar1;
              *(int *)((long)param_5 + 0x1d4) = iVar4;
              *(int *)(param_5 + 0x3b) = iVar1 + -1;
              *(int *)((long)param_5 + 0x1dc) = iVar4 + -1;
              FUN_001d1d68(param_5,1);
              if (*(char *)((long)param_5 + 0x1e3) != '\0') {
                FUN_001d0ff8(param_5);
              }
              FUN_00052788((double)iVar1,(double)iVar4,param_5);
            }
          }
        }
        else if (iVar2 == 2) {
          *(undefined2 *)((long)param_5 + 0x1e1) = 0x100;
          param_5[0x34] =
               CONCAT44(iVar4 - *(int *)((long)param_5 + 0x1d4),iVar1 - (int)param_5[0x3a]);
          FUN_00051f04((double)iVar1,(double)iVar4,param_5,0);
        }
        else {
          *(undefined2 *)((long)param_5 + 0x1e1) = 0x101;
        }
        iVar1 = (**(code **)(*param_5 + 0x2a8))(param_5);
        if (((iVar1 == 0) || ((char)param_5[0x3c] == '\0')) ||
           (((*(char *)((long)param_5 + 0x69) == '\0' && ((int)param_5[0x19] < 1)) &&
            ((*(char *)((long)param_5 + 0x1a9) != '\0' || ((int)param_5[0x26] != 1)))))) {
          (**(code **)(*param_5 + 0x250))(param_5,0);
          FUN_00053344(param_5);
        }
        (**(code **)(*param_5 + 0x1e8))(param_5);
        FUN_009ec2d8(param_5 + 0x36);
      }
      FUN_000506f8(param_5);
      *(ushort *)(param_6 + 0x12) = *(ushort *)(param_6 + 0x12) | 4;
      return;
    }
    return;
  }
  return;
}


