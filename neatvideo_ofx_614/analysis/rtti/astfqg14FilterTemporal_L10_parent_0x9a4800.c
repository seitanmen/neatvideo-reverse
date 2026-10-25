// FUN_009a4800 @ 009a4800 size=1040 tag=astfqg14FilterTemporal_L10_parent

undefined8
FUN_009a4800(undefined8 *param_1,long param_2,long *param_3,int param_4,undefined8 param_5,
            undefined8 param_6,undefined8 param_7,undefined8 param_8,undefined8 param_9,
            long param_10)

{
  int iVar1;
  ulong uVar2;
  ulong uVar3;
  undefined4 uVar4;
  uint uVar5;
  bool bVar6;
  int iVar7;
  undefined8 uVar8;
  undefined8 *puVar9;
  long lVar10;
  undefined8 uVar11;
  undefined8 uVar12;
  undefined8 uVar13;
  undefined8 uVar14;
  undefined8 uVar15;
  undefined8 uVar16;
  undefined *local_4f8;
  undefined8 *puStack_4f0;
  long *local_4e8;
  long **pplStack_4e0;
  undefined8 *local_4d8;
  undefined8 *puStack_4d0;
  undefined8 *local_4c8;
  undefined8 *puStack_4c0;
  undefined8 *local_4b8;
  undefined1 *puStack_4b0;
  undefined8 local_4a8;
  undefined8 uStack_4a0;
  undefined8 local_498;
  undefined8 uStack_490;
  long *local_488;
  long local_480;
  undefined1 auStack_471 [1025];
  long local_70;
  
  local_70 = *(long *)PTR____stack_chk_guard_013b4320;
  local_4a8 = param_8;
  uStack_4a0 = param_7;
  local_498 = param_6;
  uStack_490 = param_5;
  local_488 = param_3;
  local_480 = param_2;
  if (*(int *)(param_2 + 0xf0) == 0) {
    do {
      FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0x7083c7a1,0);
    } while( true );
  }
  uVar5 = *(uint *)(&PNG_00f0ab00.field_0x752c8 + DAT_014886c8);
  if (DAT_01488220 != 0) {
    uVar5 = uVar5 >> 0xb | uVar5 << 0x15;
  }
  iVar7 = (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10);
  if (iVar7 != 1) {
    *(uint *)(BYTE_ARRAY_ARRAY_015b2719[0xb25] + DAT_014886d0 + 0x1f) = uVar5;
  }
  uVar4 = *(undefined4 *)(*(long *)(param_10 + 0x10) + 0x28);
  DAT_01488220 = iVar7;
  if (DAT_0156a4b0 != 1) {
    FUN_00af4a8c(&DAT_0156a4a8);
  }
  uVar8 = FUN_009f8054(DAT_0156a4c0);
  FUN_009c3dcc(param_3,uVar8);
  if (param_4 == 0) {
    bVar6 = false;
  }
  else {
    bVar6 = *(long *)(param_2 + 200) != 0;
  }
  (**(code **)(*param_3 + 0x10))
            (param_3,*(undefined8 *)(param_2 + 0xd0),*(undefined8 *)(param_2 + 0xd8),uVar4,
             *(char *)(param_2 + 0xf4) != '\0',bVar6,0);
  iVar1 = **(int **)(param_2 + 0xb8) + (int)param_2 + DAT_01488224;
  DAT_01488224 = DAT_01488228;
  DAT_01488228 = (DAT_01488228 + iVar1) * 0x10001 + (DAT_01488228 >> 0x10);
  FUN_00ac0d94(param_7);
  FUN_00a2279c(param_1[0xe],*(long *)(param_2 + 0xd8) * *(long *)(param_2 + 0xd0));
  FUN_00ac2b20(param_8);
  local_4f8 = &DAT_013fcf10;
  local_4e8 = &local_480;
  pplStack_4e0 = &local_488;
  local_4d8 = &uStack_490;
  puStack_4d0 = &local_498;
  local_4c8 = &uStack_4a0;
  puStack_4c0 = &local_4a8;
  local_4b8 = &param_10;
  puStack_4b0 = &stack0x00000010;
  puStack_4f0 = param_1;
  uVar8 = FUN_009fc6e8(&local_4f8,param_9,0,0);
  if (iVar7 != 0) {
    uVar5 = *(uint *)(&PNG_00ce5d0a.field_0x1b0f8e + DAT_014886d8);
    if (DAT_0148822c != 0) {
      uVar5 = uVar5 >> 0xf | uVar5 << 0x11;
    }
    DAT_0148822c = (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10
                        );
    if (DAT_0148822c != 1) {
      *(uint *)(DAT_014886e0 + 0x1aaec28) = uVar5;
    }
  }
  iVar7 = (**(code **)*param_1)(param_1);
  uVar2 = 0x80;
  if (iVar7 == 0) {
    uVar2 = 0;
  }
  iVar7 = FUN_00ac17a4(param_1 + 0x4b3a);
  uVar3 = 2;
  if (iVar7 == 0) {
    uVar3 = 0;
  }
  puVar9 = (undefined8 *)FUN_00ac2c0c(local_4a8,uStack_4a0,uVar3 | uVar2,param_10);
  uVar12 = puVar9[1];
  uVar11 = *puVar9;
  uVar13 = puVar9[2];
  uVar15 = puVar9[5];
  uVar14 = puVar9[4];
  param_1[0x4b0b] = puVar9[3];
  param_1[0x4b0a] = uVar13;
  param_1[0x4b0d] = uVar15;
  param_1[0x4b0c] = uVar14;
  param_1[0x4b09] = uVar12;
  param_1[0x4b08] = uVar11;
  uVar12 = puVar9[7];
  uVar11 = puVar9[6];
  uVar14 = puVar9[9];
  uVar13 = puVar9[8];
  uVar16 = puVar9[0xb];
  uVar15 = puVar9[10];
  *(undefined8 *)((long)param_1 + 0x2589d) = *(undefined8 *)((long)puVar9 + 0x5d);
  param_1[0x4b11] = uVar14;
  param_1[0x4b10] = uVar13;
  param_1[0x4b13] = uVar16;
  param_1[0x4b12] = uVar15;
  param_1[0x4b0f] = uVar12;
  param_1[0x4b0e] = uVar11;
  if ((DAT_01488224 & 0x1fff80) == 0) {
    FUN_0015e370(auStack_471);
    lVar10 = FUN_009d3924(auStack_471);
    if (lVar10 != DAT_01569f68) {
      uVar5 = *(uint *)(DAT_014886e8 + 0x1a91d20);
      if (DAT_01488230 != 0) {
        uVar5 = uVar5 >> 0x1f | uVar5 << 1;
      }
      DAT_01488230 = *(uint *)(DAT_014886f8 + 0x867878);
      if (DAT_01488230 != 0) {
        uVar5 = DAT_01488230;
      }
      *(uint *)(DAT_014886f0 + 0x157dc70) = uVar5;
      if (DAT_015670d0 != 0) {
        DAT_01488230 = DAT_01488230 + *(byte *)(DAT_015670d0 + 0xc5dd);
      }
    }
  }
  if ((param_4 != 0) && (*(long *)(local_480 + 200) != 0)) {
    FUN_009c06cc(*(long *)(local_480 + 200),*(undefined4 *)(local_480 + 0xf0),
                 *(long *)(local_480 + 0xe8) * *(long *)(local_480 + 0xd0),local_488[0x19],
                 (int)local_488[0x1e],local_488[0x1d] * local_488[0x1a],*(long *)(local_480 + 0xd0),
                 *(undefined8 *)(local_480 + 0xd8),0);
  }
  if (*(long *)PTR____stack_chk_guard_013b4320 != local_70) {
                    /* WARNING: Subroutine does not return */
    ___stack_chk_fail();
  }
  return uVar8;
}


