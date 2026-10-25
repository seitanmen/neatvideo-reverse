// FUN_0098a368 @ 0098a368 size=1012 tag=EstimateScaleMotion_L4_parent

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0098a368(undefined8 param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4,
                 ulong param_5,long param_6,undefined4 *param_7)

{
  long lVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  long lVar5;
  undefined4 *puVar6;
  ulong uVar7;
  long lVar8;
  ulong *puVar9;
  undefined4 uVar10;
  long lVar11;
  long *plVar12;
  long lVar13;
  ulong uVar14;
  ulong uVar15;
  long lVar16;
  undefined4 *puVar17;
  long lVar18;
  undefined1 auVar19 [16];
  undefined *local_2108;
  long lStack_2100;
  long *local_20f8;
  undefined8 *puStack_20f0;
  undefined8 *local_20e8;
  long *plStack_20e0;
  undefined4 **local_20d8;
  ulong *puStack_20d0;
  undefined1 auStack_20c8 [24];
  undefined1 auStack_20b0 [8];
  ulong local_20a8;
  char local_2088;
  undefined8 local_2080;
  long local_2078;
  undefined4 *local_2070;
  undefined8 uStack_2068;
  undefined8 local_2060;
  long lStack_2058;
  ulong local_2050;
  void *local_2048;
  undefined1 *puStack_2040;
  undefined1 local_2038 [8192];
  long local_38;
  
  auVar19 = (*(code *)PTR____chkstk_darwin_013b4308)();
  lStack_2058 = auVar19._8_8_;
  lVar5 = auVar19._0_8_;
  local_38 = *(long *)PTR____stack_chk_guard_013b4320;
  uVar2 = *(uint *)(DAT_01487990 + 0xb9bc0);
  iVar4 = *(int *)(DAT_014879a0 + 0x16652a8);
  if (iVar4 != 0) {
    if (DAT_01487844 != 0) {
      uVar2 = uVar2 >> 0x19 | uVar2 << 7;
    }
    *(uint *)(DAT_01487998 + 0xb77bb0) = uVar2;
  }
  if (DAT_01565648 != 0) {
    iVar4 = iVar4 + (uint)*(byte *)(DAT_01565648 + 0x8eba);
  }
  if ((0 < param_6) && (0 < (long)param_5)) {
    lVar8 = 0;
    lVar11 = *(long *)(lStack_2058 + 0x20) + -0x10;
    lVar13 = *(long *)(lStack_2058 + 0x18) + -0x10;
    uVar15 = param_5 & 0xfffffffffffffffe;
    puVar6 = param_7;
    do {
      lVar16 = lVar8 * 0x10;
      uVar10 = (undefined4)lVar11;
      if (lVar16 - lVar11 == 0 || lVar16 < lVar11) {
        uVar10 = (undefined4)lVar16;
      }
      if (param_5 < 2) {
        puVar17 = puVar6;
        uVar7 = 0;
LAB_0098a4c4:
        lVar18 = param_5 - uVar7;
        lVar16 = uVar7 << 4;
        do {
          lVar1 = lVar13;
          if (lVar16 <= lVar13) {
            lVar1 = lVar16;
          }
          puVar6 = puVar17 + 0x11;
          *puVar17 = (int)lVar1;
          puVar17[1] = uVar10;
          lVar16 = lVar16 + 0x10;
          lVar18 = lVar18 + -1;
          puVar17 = puVar6;
        } while (lVar18 != 0);
      }
      else {
        puVar17 = puVar6 + uVar15 * 0x11;
        puVar6 = puVar6 + 0x12;
        lVar16 = 0x10;
        uVar7 = uVar15;
        do {
          lVar18 = lVar13;
          if (lVar16 + -0x10 <= lVar13) {
            lVar18 = lVar16 + -0x10;
          }
          lVar1 = lVar13;
          if (lVar16 <= lVar13) {
            lVar1 = lVar16;
          }
          puVar6[-1] = (int)lVar1;
          *puVar6 = uVar10;
          puVar6[-0x12] = (int)lVar18;
          puVar6[-0x11] = uVar10;
          puVar6 = puVar6 + 0x22;
          lVar16 = lVar16 + 0x20;
          uVar7 = uVar7 - 2;
        } while (uVar7 != 0);
        puVar6 = puVar17;
        uVar7 = uVar15;
        if (uVar15 != param_5) goto LAB_0098a4c4;
      }
      lVar8 = lVar8 + 1;
    } while (lVar8 != param_6);
  }
  local_2078 = param_6 * param_5;
  if (iVar4 != 0) {
    uVar3 = *(uint *)(DAT_014879a8 + 0x642b78);
    uVar2 = (uVar3 ^ uVar3 >> 1) & 0x44044505;
    uVar3 = (uVar2 | uVar2 << 1) ^ uVar3;
    if ((int)(CONCAT44((uVar3 + (uVar3 >> 0x10)) * 0x101 + (uVar3 >> 0x18),uVar3) >> 0x10) != 1) {
      *(uint *)(DAT_014879b0 + 0x1b52408) = uVar3;
    }
  }
  local_2080 = 0;
  if (*(long *)(lVar5 + 0x548) < 2) {
    lVar8 = 1;
  }
  else {
    lVar8 = 0;
    if (*(long *)(lVar5 + 0x550) != 0) {
      lVar8 = *(long *)(lVar5 + 0x548) / *(long *)(lVar5 + 0x550);
    }
    lVar8 = lVar8 + 1;
  }
  local_2070 = param_7;
  uStack_2068 = param_4;
  local_2060 = param_3;
  if (((DAT_01544cd0 & 1) == 0) && (iVar4 = ___cxa_guard_acquire(&DAT_01544cd0), iVar4 != 0)) {
    thunk_FUN_009fb9f0(&DAT_01543918,"EstimateScaleMotion");
    ___cxa_guard_release(&DAT_01544cd0);
  }
  FUN_009fbd24(auStack_20b0,&DAT_01543918,local_2078,lVar8);
  uVar15 = local_20a8;
  if (local_2088 == '\x01') {
    uVar15 = 1;
  }
  FUN_0097f8d0(auStack_20c8,uVar15);
  local_2050 = uVar15;
  if ((long)uVar15 < 0x101) {
    puStack_2040 = local_2038;
    local_2048 = (void *)0x0;
    if ((long)uVar15 < 1) goto LAB_0098a69c;
    if (3 < uVar15) goto LAB_0098a604;
LAB_0098a5dc:
    uVar7 = 0;
  }
  else {
    local_2048 = _malloc(uVar15 << 5 | 8);
    puStack_2040 = (undefined1 *)((long)local_2048 + 7U & 0xfffffffffffffff8);
    if (uVar15 < 4) goto LAB_0098a5dc;
LAB_0098a604:
    uVar7 = uVar15 & 0xfffffffffffffffc;
    plVar12 = (long *)(puStack_2040 + 0x40);
    uVar14 = uVar7;
    lVar8 = _DAT_00c95950;
    lVar11 = _UNK_00c95958;
    do {
      plVar12[-8] = (long)auStack_20c8;
      plVar12[-7] = uVar15;
      plVar12[-6] = lVar8;
      plVar12[-5] = (long)&local_2080;
      plVar12[-4] = (long)auStack_20c8;
      plVar12[-3] = uVar15;
      plVar12[-2] = lVar11;
      plVar12[-1] = (long)&local_2080;
      *plVar12 = (long)auStack_20c8;
      plVar12[1] = uVar15;
      plVar12[2] = lVar8 + 2;
      plVar12[3] = (long)&local_2080;
      plVar12[4] = (long)auStack_20c8;
      plVar12[5] = uVar15;
      plVar12[6] = lVar11 + 2;
      plVar12[7] = (long)&local_2080;
      plVar12 = plVar12 + 0x10;
      lVar8 = lVar8 + 4;
      lVar11 = lVar11 + 4;
      uVar14 = uVar14 - 4;
    } while (uVar14 != 0);
    if (uVar15 == uVar7) goto LAB_0098a69c;
  }
  puVar9 = (ulong *)(puStack_2040 + uVar7 * 0x20 + 0x10);
  do {
    *puVar9 = uVar7;
    puVar9[1] = (ulong)&local_2080;
    puVar9[-2] = (ulong)auStack_20c8;
    puVar9[-1] = uVar15;
    uVar7 = uVar7 + 1;
    puVar9 = puVar9 + 4;
  } while (uVar15 != uVar7);
LAB_0098a69c:
  local_2108 = &DAT_013fcc20;
  local_20f8 = &lStack_2058;
  puStack_20f0 = &local_2060;
  local_20e8 = &uStack_2068;
  plStack_20e0 = &local_2078;
  local_20d8 = &local_2070;
  puStack_20d0 = &local_2050;
  lStack_2100 = lVar5;
  FUN_009fc6e8(&local_2108,uVar15,0,0);
  if (local_2048 != (void *)0x0) {
    _free(local_2048);
  }
  FUN_009fb92c(auStack_20b0);
  if (*(long *)PTR____stack_chk_guard_013b4320 != local_38) {
                    /* WARNING: Subroutine does not return */
    ___stack_chk_fail();
  }
  return;
}


