// FUN_00052a94 size=548

void FUN_00052a94(long *param_1,long *param_2,long param_3)

{
  uint uVar1;
  uint uVar2;
  char cVar3;
  int iVar4;
  ulong uVar5;
  byte bVar6;
  undefined8 local_40;
  undefined8 local_38;
  
  (**(code **)(*param_1 + 0x240))();
  uVar1 = *(uint *)(DAT_0141a978 + 0x6aa0d0);
  if (DAT_0141a970 != 0) {
    uVar1 = uVar1 >> 10 | uVar1 << 0x16;
  }
  if (uVar1 != 0) {
    *(uint *)(DAT_0141a980 + 0x19b5c80) = uVar1;
  }
  DAT_0141a970 = uVar1;
  *(undefined1 *)((long)param_1 + 0x109) = *(undefined1 *)((long)param_2 + 0x109);
  *(char *)(param_1 + 0x21) = (char)param_2[0x21];
  *(undefined1 *)((long)param_1 + 0x69) = *(undefined1 *)((long)param_2 + 0x69);
  *(undefined1 *)((long)param_1 + 0x6a) = *(undefined1 *)((long)param_2 + 0x6a);
  *(int *)(param_1 + 0x19) = (int)param_2[0x19];
  FUN_002e79b0(param_1 + 0x22,param_2 + 0x22);
  if ((param_2 != (long *)0x0) &&
     (*(int *)((long)param_1 + 0x134) != *(int *)((long)param_2 + 0x134))) {
    *(int *)((long)param_1 + 0x134) = *(int *)((long)param_2 + 0x134);
    FUN_001d1a88(param_1);
    FUN_00050360(param_1);
  }
  if ((*(char *)(param_3 + 0xb) == '\0') ||
     (uVar5 = (**(code **)(*param_2 + 0x220))(param_2), (uVar5 & 1) != 0)) {
    if (((char)param_2[0x1e] != '\0') || (*(char *)(param_3 + 10) == '\0')) goto LAB_00052bc8;
    bVar6 = *(byte *)(param_3 + 9) ^ 1;
  }
  else {
    iVar4 = (**(code **)(*param_2 + 0x228))(param_2);
    if (iVar4 == 0) goto LAB_00052bc8;
    bVar6 = 1;
    if ((char)param_2[0xd] != '\0') {
      bVar6 = 2;
    }
  }
  FUN_000508fc(param_1,param_2,bVar6);
LAB_00052bc8:
  if ((((param_1[0x29] != 0) && (param_2[0x29] != 0)) &&
      (cVar3 = *(char *)(param_2[0x29] + 0x78),
      (*(char *)(param_1[0x29] + 0x78) != '\0') != (cVar3 != '\0'))) &&
     ((**(code **)(*param_1 + 0x250))(param_1,cVar3 != '\0'), param_1[0x2a] != 0)) {
    FUN_00051b10(param_1);
  }
  if (uVar1 != 0) {
    uVar2 = *(uint *)(DAT_0141a988 + 0x586b58);
    uVar1 = (uVar2 ^ uVar2 >> 1) & 0x44044155;
    uVar2 = (uVar1 | uVar1 << 1) ^ uVar2;
    if (uVar2 != 1) {
      *(uint *)(DAT_0141a990 + 0x7d54e8) = uVar2;
    }
  }
  local_40 = FUN_00470e94();
  local_38 = FUN_00757ebc(param_1,&local_40);
  FUN_0095d6ec(param_1,&local_38);
  FUN_00051f04(param_1,1);
  FUN_00050548(param_1);
  return;
}


