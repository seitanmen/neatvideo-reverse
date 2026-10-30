// FUN_0004ce6c size=160

void FUN_0004ce6c(long param_1,int *param_2)

{
  int iVar1;
  
  FUN_00051ec4();
  if (*param_2 != 0) {
    return;
  }
  if (*(char *)(param_1 + 0x1a9) == '\0') {
    return;
  }
  if (*(char *)(param_1 + 0x1e4) == '\0') {
    switch(*(undefined4 *)(param_1 + 0x130)) {
    case 1:
      goto switchD_0004ced4_caseD_1;
    case 2:
      iVar1 = 0x11;
      if (*(char *)(param_1 + 0x1e2) != '\0') {
        iVar1 = 0x12;
      }
      break;
    case 3:
    case 4:
      iVar1 = 6;
      break;
    case 5:
    case 6:
      iVar1 = 5;
      break;
    case 7:
    case 10:
      iVar1 = 8;
      break;
    case 8:
    case 9:
      iVar1 = 7;
      break;
    default:
      goto switchD_0004ced4_default;
    }
  }
  else {
switchD_0004ced4_caseD_1:
    iVar1 = 2;
  }
  *param_2 = iVar1;
switchD_0004ced4_default:
  return;
}


