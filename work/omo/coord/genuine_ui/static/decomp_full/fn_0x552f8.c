// FUN_000552f8 size=112

void FUN_000552f8(long param_1,uint param_2)

{
  undefined8 local_30;
  undefined8 local_28;
  
  if ((*(byte *)(param_1 + 0x168) != param_2) &&
     (((param_2 & 1) != 0 || (*(char *)(param_1 + 0x69) == '\0')))) {
    *(char *)(param_1 + 0x168) = (char)param_2;
    local_30 = FUN_00470e94();
    local_28 = FUN_00757ebc(param_1,&local_30);
    FUN_0095d6ec(param_1,&local_28);
    FUN_00051f04(param_1,1);
  }
  return;
}


