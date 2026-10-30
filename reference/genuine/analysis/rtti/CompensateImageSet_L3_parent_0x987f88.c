// FUN_00987f88 @ 00987f88 size=1256 tag=CompensateImageSet_L3_parent

bool FUN_00987f88(long param_1,long *param_2,undefined8 param_3,long param_4,ulong param_5,
                 undefined1 param_6,undefined8 param_7,undefined8 param_8,undefined8 param_9,
                 undefined8 param_10,long param_11)

{
  long lVar1;
  long lVar2;
  ulong uVar3;
  uint uVar4;
  uint uVar5;
  bool bVar6;
  int iVar7;
  undefined8 *puVar8;
  ulong uVar9;
  long *plVar10;
  long lVar11;
  ulong uVar12;
  undefined8 uVar13;
  undefined8 uVar14;
  long lVar15;
  undefined8 uVar16;
  undefined8 uVar17;
  long lVar18;
  undefined8 uVar19;
  undefined8 uVar20;
  long lVar21;
  undefined8 uVar22;
  undefined *local_c0;
  long lStack_b8;
  long **local_b0;
  long *plStack_a8;
  undefined8 *local_a0;
  undefined8 *puStack_98;
  undefined8 *local_90;
  undefined1 *puStack_88;
  undefined8 local_80;
  long lStack_78;
  undefined8 local_70;
  long *plStack_68;
  
  lVar1 = param_11;
  uVar4 = *(uint *)(DAT_014878c0 + 0x35548);
  if (DAT_0148781c != 0) {
    uVar4 = uVar4 >> 0xe | uVar4 << 0x12;
  }
  iVar7 = (int)(CONCAT44((uVar4 + (uVar4 >> 0x10)) * 0x101 + (uVar4 >> 0x18),uVar4) >> 0x10);
  if (iVar7 != 1) {
    *(uint *)(DAT_014878c8 + 0x1fb6b18) = uVar4;
  }
  DAT_0148781c = iVar7;
  local_80 = param_8;
  lStack_78 = param_4;
  local_70 = param_3;
  plStack_68 = param_2;
  if (*(int *)(*param_2 + 0xf0) == 0) {
    do {
      FUN_009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0x74f8c2ea,0);
    } while( true );
  }
  *(undefined8 *)(param_1 + 0x18) = param_10;
  FUN_00a2279c(param_10,param_5);
  *(undefined8 *)(param_1 + 0x50) = param_7;
  FUN_00a00360(param_1 + 0x300,*(undefined8 *)(param_4 + 0xd0),*(undefined8 *)(param_4 + 0xd8),
               *(undefined4 *)(param_4 + 0xf0),*(undefined1 *)(param_4 + 0xf4),param_5,
               *(undefined4 *)(param_4 + 0xf8),*(undefined4 *)(param_4 + 0xfc),1);
  *(undefined1 *)(param_1 + 0x538) = param_6;
  *(long *)(param_1 + 0x58) = param_4;
  *(undefined8 *)(param_1 + 0x540) = param_9;
  lVar2 = param_1 + 0x60;
  puVar8 = (undefined8 *)
           FUN_00ac4058(lVar2,*(undefined8 *)(param_1 + 0x278),*(undefined8 *)(param_1 + 0x308),
                        *(undefined8 *)(param_1 + 0x310),*(undefined4 *)(param_1 + 0x318),param_5,
                        lVar1);
  uVar19 = puVar8[3];
  uVar17 = puVar8[2];
  uVar14 = puVar8[5];
  uVar13 = puVar8[4];
  uVar16 = *puVar8;
  *(undefined8 *)(param_1 + 0x288) = puVar8[1];
  *(undefined8 *)(param_1 + 0x280) = uVar16;
  *(undefined8 *)(param_1 + 0x298) = uVar19;
  *(undefined8 *)(param_1 + 0x290) = uVar17;
  *(undefined8 *)(param_1 + 0x2a8) = uVar14;
  *(undefined8 *)(param_1 + 0x2a0) = uVar13;
  uVar14 = puVar8[7];
  uVar13 = puVar8[6];
  uVar17 = puVar8[9];
  uVar16 = puVar8[8];
  uVar20 = puVar8[0xb];
  uVar19 = puVar8[10];
  uVar22 = *(undefined8 *)((long)puVar8 + 0x59);
  *(undefined8 *)(param_1 + 0x2e1) = *(undefined8 *)((long)puVar8 + 0x61);
  *(undefined8 *)(param_1 + 0x2d9) = uVar22;
  *(undefined8 *)(param_1 + 0x2c8) = uVar17;
  *(undefined8 *)(param_1 + 0x2c0) = uVar16;
  *(undefined8 *)(param_1 + 0x2d8) = uVar20;
  *(undefined8 *)(param_1 + 0x2d0) = uVar19;
  *(undefined8 *)(param_1 + 0x2b8) = uVar14;
  *(undefined8 *)(param_1 + 0x2b0) = uVar13;
  FUN_009f14e4(param_1 + 0x558,(undefined8 *)(param_1 + 0x280),0x50,0);
  if ((long)param_5 < 1) {
    uVar12 = 0;
  }
  else {
    if (param_5 < 4) {
      uVar12 = 0;
      uVar9 = 0;
    }
    else {
      uVar9 = param_5 & 0xfffffffffffffffc;
      plVar10 = (long *)(param_1 + 0x568);
      lVar11 = 0;
      lVar15 = 0;
      lVar18 = 0;
      lVar21 = 0;
      uVar12 = uVar9;
      do {
        lVar11 = lVar11 + (ulong)(plVar10[-2] == 0x10);
        lVar15 = lVar15 + (ulong)(plVar10[-1] == 0x10);
        lVar18 = lVar18 + (ulong)(*plVar10 == 0x10);
        lVar21 = lVar21 + (ulong)(plVar10[1] == 0x10);
        plVar10 = plVar10 + 4;
        uVar12 = uVar12 - 4;
      } while (uVar12 != 0);
      uVar12 = lVar18 + lVar11 + lVar21 + lVar15;
      if (uVar9 == param_5) goto LAB_009881a8;
    }
    lVar11 = param_5 - uVar9;
    plVar10 = (long *)(param_1 + uVar9 * 8 + 0x558);
    do {
      if (*plVar10 == 0x10) {
        uVar12 = uVar12 + 1;
      }
      lVar11 = lVar11 + -1;
      plVar10 = plVar10 + 1;
    } while (lVar11 != 0);
  }
LAB_009881a8:
  FUN_009870ec(param_1 + 0x20);
  lVar11 = *(long *)(param_1 + 0x20 + (ulong)(*(int *)(param_4 + 0xf8) == 2) * 0x10 +
                    (ulong)(*(int *)(param_4 + 0xfc) == 2) * 8);
  *(long *)(param_1 + 0x48) = lVar11;
  if (uVar12 == 0) {
    *(undefined4 *)(&DAT_0000d980 + lVar11) = 2;
  }
  else {
    FUN_009859a8(lVar11,param_1 + 0x300,param_2,param_4,param_1 + 0x558,
                 *(undefined8 *)(param_1 + 0x540),*(undefined8 *)(param_1 + 8));
    if (iVar7 != 0) {
      uVar5 = *(uint *)(&PNG_011f0a20.field_0x2fa8 + DAT_014878d0);
      uVar4 = (uVar5 ^ uVar5 >> 1) & 0x45055455;
      uVar5 = (uVar4 | uVar4 << 1) ^ uVar5;
      iVar7 = (int)(CONCAT44((uVar5 + (uVar5 >> 0x10)) * 0x101 + (uVar5 >> 0x18),uVar5) >> 0x10);
      DAT_01487820 = iVar7;
      if (DAT_01567ab0 != 0) {
        iVar7 = iVar7 + (uint)(byte)(&DAT_00006cd1)[DAT_01567ab0];
        DAT_01487820 = iVar7;
      }
    }
  }
  FUN_0097ff78(*(long *)(param_1 + 8) + 0x25c30);
  if ((uVar12 == 0) || (*(char *)(param_1 + 0x2d0) == '\0')) {
    param_5 = uVar12;
  }
  lVar1 = (long)*(char *)(lVar1 + 0x90) + (long)(int)-(*(uint *)(lVar1 + 0x8c) >> 0x10 & 1);
  if (uVar12 == 0) {
    uVar9 = 0;
  }
  else {
    uVar9 = *(long *)(param_1 + 0x540) - lVar1;
  }
  *(ulong *)(param_1 + 0x548) = uVar9;
  if ((long)param_5 <= (long)uVar9) {
    uVar9 = param_5;
  }
  *(ulong *)(param_1 + 0x550) = uVar9;
  *(undefined8 *)(param_1 + 0x2f0) = 0;
  if ((iVar7 != 0) &&
     (uVar4 = *(uint *)(DAT_014878d8 + 0x1742528), *(int *)(DAT_014878e8 + 0x1ccc1f8) != 1)) {
    uVar5 = (uVar4 ^ uVar4 >> 1) & 0x45110005;
    *(uint *)(DAT_014878e0 + 0x1a3abf8) = (uVar5 | uVar5 << 1) ^ uVar4;
  }
  FUN_00ac4940(lVar2);
  local_c0 = &DAT_013fcbf0;
  local_b0 = &plStack_68;
  plStack_a8 = &lStack_78;
  local_a0 = &local_70;
  puStack_98 = &local_80;
  local_90 = &param_11;
  puStack_88 = &stack0x00000018;
  lStack_b8 = param_1;
  FUN_009fc6e8(&local_c0,uVar9 + lVar1,0,0);
  if (*(undefined8 **)(param_1 + 0x10) == (undefined8 *)0x0) {
    uVar9 = 0;
  }
  else {
    iVar7 = (**(code **)**(undefined8 **)(param_1 + 0x10))();
    uVar9 = 0x80;
    if (iVar7 == 0) {
      uVar9 = 0;
    }
  }
  if (uVar12 == 0) {
    uVar12 = 0;
  }
  else {
    uVar12 = (ulong)(1 < *(long *)(&DAT_0000d9f0 + *(long *)(param_1 + 0x48))) << 0xb;
  }
  iVar7 = FUN_00ac17a4(*(long *)(param_1 + 8) + 0x259d0);
  uVar3 = 2;
  if (iVar7 == 0) {
    uVar3 = 0;
  }
  puVar8 = (undefined8 *)FUN_00ac4a2c(lVar2,uVar12 | uVar9 | uVar3,param_11);
  uVar14 = puVar8[1];
  uVar13 = *puVar8;
  uVar16 = puVar8[2];
  uVar19 = puVar8[5];
  uVar17 = puVar8[4];
  *(undefined8 *)(param_1 + 0x298) = puVar8[3];
  *(undefined8 *)(param_1 + 0x290) = uVar16;
  *(undefined8 *)(param_1 + 0x2a8) = uVar19;
  *(undefined8 *)(param_1 + 0x2a0) = uVar17;
  *(undefined8 *)(param_1 + 0x288) = uVar14;
  *(undefined8 *)(param_1 + 0x280) = uVar13;
  uVar14 = puVar8[7];
  uVar13 = puVar8[6];
  uVar17 = puVar8[9];
  uVar16 = puVar8[8];
  uVar20 = puVar8[0xb];
  uVar19 = puVar8[10];
  uVar22 = *(undefined8 *)((long)puVar8 + 0x59);
  *(undefined8 *)(param_1 + 0x2e1) = *(undefined8 *)((long)puVar8 + 0x61);
  *(undefined8 *)(param_1 + 0x2d9) = uVar22;
  *(undefined8 *)(param_1 + 0x2c8) = uVar17;
  *(undefined8 *)(param_1 + 0x2c0) = uVar16;
  *(undefined8 *)(param_1 + 0x2d8) = uVar20;
  *(undefined8 *)(param_1 + 0x2d0) = uVar19;
  *(undefined8 *)(param_1 + 0x2b8) = uVar14;
  *(undefined8 *)(param_1 + 0x2b0) = uVar13;
  if ((*(undefined8 **)(param_1 + 0x10) == (undefined8 *)0x0) ||
     (uVar12 = (**(code **)**(undefined8 **)(param_1 + 0x10))(), (uVar12 & 1) == 0)) {
    bVar6 = *(char *)(param_1 + 0x5b4) == '\0';
  }
  else {
    bVar6 = false;
  }
  return bVar6;
}


