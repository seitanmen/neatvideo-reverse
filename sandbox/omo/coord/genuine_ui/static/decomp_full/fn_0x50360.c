// FUN_00050360 size=464

void FUN_00050360(long *param_1)

{
  long *plVar1;
  uint uVar2;
  undefined8 uVar3;
  uint *puVar4;
  undefined8 local_a0;
  double local_98;
  double dStack_90;
  undefined1 auStack_88 [104];
  
  if (param_1[0x2a] != 0) {
    uVar2 = *(uint *)(DAT_0141a7f0 + 0x1c0028);
    if (DAT_0141a7e8 != 0) {
      uVar2 = uVar2 >> 0xf | uVar2 << 0x11;
    }
    DAT_0141a7e8 = (int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10
                        );
    if (DAT_0141a7e8 != 1) {
      *(uint *)(&PNG_00ff6f28.field_0x3c10 + DAT_0141a7f8) = uVar2;
    }
    if (DAT_01562fa0 != 0) {
      DAT_0141a7e8 = DAT_0141a7e8 + (uint)(byte)(&DAT_0000224b)[DAT_01562fa0];
    }
    *(int *)(param_1 + 0x13) = (int)param_1[0x13] + 1;
    (**(code **)(*param_1 + 0x1f8))(param_1);
    local_a0 = FUN_00470e94();
    uVar3 = FUN_00757ebc(param_1,&local_a0);
    local_98 = (double)(int)uVar3;
    dStack_90 = (double)(int)((ulong)uVar3 >> 0x20);
    FUN_0047249c(auStack_88,5,&local_98,0,0,0);
    (**(code **)(*param_1 + 0xb0))(param_1,auStack_88);
    (**(code **)(*param_1 + 0x200))(param_1);
    (**(code **)(*param_1 + 0x1e8))(param_1);
    *(int *)(param_1 + 0x13) = (int)param_1[0x13] + -1;
    FUN_00050548(param_1);
    FUN_000506f8(param_1);
    if ((((char)param_1[0x23] == '\0') && ((int)param_1[0x25] != 0)) &&
       (puVar4 = (uint *)param_1[0x2c], puVar4[1] != 0)) {
      if (1 < *puVar4) {
        plVar1 = param_1 + 0x2c;
        if ((puVar4[2] & 0x7fffffff) == 0) {
          puVar4 = (uint *)FUN_00287b7c(0x10,8,0,2);
          *plVar1 = (long)puVar4;
        }
        else {
          FUN_00091120(plVar1,puVar4[2] & 0x7fffffff,0);
          puVar4 = (uint *)*plVar1;
        }
      }
      FUN_000508fc(param_1,*(undefined8 *)((long)puVar4 + *(long *)(puVar4 + 4)),1);
    }
    thunk_FUN_003bb5cc(auStack_88);
  }
  return;
}


