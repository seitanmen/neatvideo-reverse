// FUN_00048b28 size=832

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

long FUN_00048b28(long param_1)

{
  long lVar1;
  long lVar2;
  long lVar3;
  uint uVar4;
  uint uVar5;
  double dVar6;
  undefined8 *puVar7;
  undefined8 uVar8;
  double dVar9;
  undefined4 local_b0;
  undefined8 local_ac;
  undefined2 local_a4;
  undefined1 auStack_a0 [16];
  undefined4 local_90;
  undefined8 local_8c;
  undefined2 local_84;
  undefined4 local_80;
  undefined8 local_7c;
  undefined2 local_74;
  undefined4 local_70;
  undefined8 local_6c;
  undefined2 local_64;
  
  puVar7 = (undefined8 *)FUN_0004bd08();
  *puVar7 = &DAT_013c0f38;
  puVar7[2] = &DAT_013c1230;
  puVar7[6] = &DAT_013c1280;
  puVar7 = puVar7 + 0x4d;
  thunk_FUN_00638810(puVar7);
  lVar1 = param_1 + 0x270;
  FUN_005a1e30(lVar1);
  *(undefined4 *)(param_1 + 0x278) = 0;
  *(undefined2 *)(param_1 + 0x27c) = 0xffff;
  *(undefined8 *)(param_1 + 0x27e) = 0;
  lVar2 = param_1 + 0x288;
  thunk_FUN_00638810(lVar2);
  *(undefined4 *)(param_1 + 0x290) = 0;
  *(undefined2 *)(param_1 + 0x294) = 0xffff;
  *(undefined8 *)(param_1 + 0x296) = 0;
  uVar8 = _DAT_00c93b10;
  *(undefined8 *)(param_1 + 0x2a8) = _UNK_00c93b18;
  *(undefined8 *)(param_1 + 0x2a0) = uVar8;
  *(undefined8 *)(param_1 + 0x2b0) = 0xffffffffffffffff;
  local_70 = 1;
  local_6c = DAT_00c93fa8;
  local_64 = 0;
  local_80 = 1;
  local_7c = DAT_00c93fb0;
  local_74 = 0;
  local_90 = 1;
  local_8c = DAT_00c93fa8;
  local_84 = 0;
  FUN_005a2834(lVar1,0);
  lVar3 = param_1 + 0x220;
  FUN_00639e48(lVar3,0);
  uVar8 = __Znwm(0x100);
  thunk_FUN_00045334(uVar8,param_1,0,param_1 + 0x138);
  *(undefined8 *)(param_1 + 0x218) = uVar8;
  FUN_009299d4(uVar8,0);
  FUN_0092e77c(*(undefined8 *)(param_1 + 0x218),lVar3);
  FUN_0092e7fc(*(undefined8 *)(param_1 + 0x218),lVar1);
  FUN_00047170(*(undefined8 *)(param_1 + 0x218),&local_70);
  FUN_00046fc0(*(undefined8 *)(param_1 + 0x218),&local_80);
  FUN_004d4db4(auStack_a0,*(long *)(param_1 + 0x218) + 0x88);
  dVar6 = DAT_0127a7a0;
  dVar9 = *(double *)(DAT_0155b2d0 + 0x548) + 0.0;
  if (dVar9 <= DAT_0127a7a0) {
    dVar9 = DAT_0127a7a0;
  }
  FUN_004d5400(auStack_a0,(int)dVar9);
  FUN_00045dbc(*(undefined8 *)(param_1 + 0x218),auStack_a0);
  FUN_00638fcc(puVar7,lVar3);
  FUN_005a5f50(&local_b0,5);
  FUN_00639ea8(puVar7,&local_b0);
  uVar8 = __Znwm(0x100);
  thunk_FUN_00045334(uVar8,param_1,0,param_1 + 0x138);
  *(undefined8 *)(param_1 + 0x260) = uVar8;
  FUN_00046fc0(uVar8,&local_90);
  FUN_009299d4(*(undefined8 *)(param_1 + 0x260),0);
  FUN_0092e7fc(*(undefined8 *)(param_1 + 0x260),lVar1);
  local_b0 = 1;
  local_ac = DAT_00c93fb8;
  local_a4 = 0;
  FUN_000472e4(*(undefined8 *)(param_1 + 0x260),&local_b0);
  FUN_00047324(*(undefined8 *)(param_1 + 0x260),1);
  FUN_004d4ec8(auStack_a0,*(long *)(param_1 + 0x260) + 0x88);
  dVar9 = *(double *)(DAT_0155b2d0 + 0x548) + -2.0;
  if (dVar9 <= dVar6) {
    dVar9 = dVar6;
  }
  FUN_004d5400(auStack_a0,(int)dVar9);
  FUN_00045dbc(*(undefined8 *)(param_1 + 0x260),auStack_a0);
  *(undefined2 *)(param_1 + 0x2b8) = 0;
  *(undefined2 *)(param_1 + 0x2d8) = 0;
  FUN_00639d04(lVar2,1);
  FUN_00639f7c(lVar2,1);
  uVar5 = *(uint *)(DAT_0141a608 + 0x1e7db70);
  uVar4 = (uVar5 ^ uVar5 >> 1) & 0x41511100;
  uVar5 = (uVar4 | uVar4 << 1) ^ uVar5;
  if ((int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10) != 0) {
    *(uint *)(DAT_0141a610 + 0x3e1220) = uVar5;
  }
  FUN_0005755c(*(undefined8 *)(param_1 + 0x188),DAT_0155b2d0 + 0x478,DAT_0155b2d0 + 0x4a8);
  *(undefined8 *)(param_1 + 0x2d0) = 0;
  *(undefined8 *)(param_1 + 0x2c8) = 0;
  *(undefined8 *)(param_1 + 0x2c0) = 0;
  FUN_00048ef4(param_1);
  thunk_FUN_004d4dd4(auStack_a0);
  return param_1;
}


