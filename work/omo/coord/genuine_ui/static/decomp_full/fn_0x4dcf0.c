// FUN_0004dcf0 size=168

void FUN_0004dcf0(long *param_1,undefined8 param_2)

{
  bool bVar1;
  int iVar2;
  code *pcVar3;
  
  if (param_1[0x29] != 0) {
    iVar2 = (**(code **)(*param_1 + 0x2a8))(param_1);
    if ((iVar2 == 0) || ((char)param_1[0x3c] == '\0')) {
      if ((int)param_2 == 0) {
        FUN_0004d8dc(param_1);
        if (((char)param_1[0x45] == '\0') || (*(char *)((long)param_1 + 0x229) != '\0')) {
          bVar1 = *(char *)((long)param_1 + 0x259) != '\0';
        }
        else {
          bVar1 = false;
        }
        pcVar3 = *(code **)(*param_1 + 0x250);
      }
      else {
        pcVar3 = *(code **)(*param_1 + 0x250);
        bVar1 = false;
      }
      (*pcVar3)(param_1,bVar1);
    }
    FUN_000506f8(param_1);
  }
  FUN_000549b0(param_1,param_2);
  return;
}


