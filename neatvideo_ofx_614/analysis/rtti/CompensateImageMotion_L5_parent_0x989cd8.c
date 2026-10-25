// FUN_00989cd8 @ 00989cd8 size=1276 tag=CompensateImageMotion_L5_parent

void FUN_00989cd8(long param_1,long param_2,long *param_3,undefined8 param_4,undefined8 param_5,
                 undefined8 param_6,undefined8 *param_7,undefined8 param_8)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined8 uVar6;
  long lVar7;
  undefined8 *puVar8;
  long lVar9;
  undefined1 *puVar10;
  long lVar11;
  undefined1 *puVar12;
  long lVar13;
  undefined8 *puVar14;
  long lVar15;
  undefined1 *puVar16;
  long lVar17;
  undefined1 *puVar18;
  long lVar19;
  undefined *local_280;
  long lStack_278;
  long *local_270;
  long **pplStack_268;
  undefined8 *local_260;
  undefined8 *puStack_258;
  undefined8 *local_250;
  undefined1 *puStack_248;
  undefined1 *local_240;
  long *plStack_238;
  undefined1 *local_230;
  undefined8 **ppuStack_228;
  undefined *local_220;
  long lStack_218;
  long *local_210;
  long **pplStack_208;
  undefined8 *local_200;
  undefined8 *puStack_1f8;
  undefined8 *local_1f0;
  undefined1 *puStack_1e8;
  undefined1 *local_1e0;
  long *plStack_1d8;
  undefined1 *local_1d0;
  undefined8 **ppuStack_1c8;
  undefined *local_1c0;
  long lStack_1b8;
  long *local_1b0;
  long **pplStack_1a8;
  undefined8 *local_1a0;
  undefined8 *puStack_198;
  undefined8 *local_190;
  undefined8 *puStack_188;
  undefined8 *local_180;
  long *plStack_178;
  undefined8 *local_170;
  undefined8 **ppuStack_168;
  long local_160;
  undefined1 auStack_158 [8];
  undefined1 auStack_150 [8];
  undefined8 local_148;
  undefined1 auStack_140 [8];
  undefined1 auStack_138 [8];
  undefined8 local_130;
  undefined1 auStack_128 [8];
  undefined1 auStack_120 [8];
  undefined8 local_118;
  undefined1 auStack_110 [48];
  long local_e0;
  long local_c8;
  undefined8 local_c0;
  long local_b8;
  undefined8 *local_b0;
  undefined8 local_a8;
  undefined8 local_a0;
  undefined8 local_98;
  long *local_90;
  long local_88;
  undefined **local_80;
  undefined **ppuStack_78;
  undefined **local_70;
  long local_68;
  
  iVar4 = DAT_01487808;
  local_68 = *(long *)PTR____stack_chk_guard_013b4320;
  uVar3 = *(uint *)(&UNK_01287e28 + DAT_014879e0);
  uVar1 = (uVar3 ^ uVar3 >> 1) & 0x44401145;
  uVar3 = (uVar1 | uVar1 << 1) ^ uVar3;
  local_b8 = 0;
  iVar2 = *(int *)(param_1 + 0x5b0);
  lVar7 = *(long *)(param_1 + 0x540);
  DAT_01487850 = uVar3;
  local_c0 = param_8;
  local_b0 = param_7;
  local_a8 = param_6;
  local_a0 = param_5;
  local_98 = param_4;
  local_90 = param_3;
  local_88 = param_2;
  if (DAT_0156a4b0 != 1) {
    FUN_00af4a8c(&DAT_0156a4a8);
  }
  uVar6 = FUN_009f8054(DAT_0156a4c0);
  FUN_009c3dcc(param_3,uVar6);
  (**(code **)(*local_90 + 0x10))
            (local_90,*(undefined8 *)(local_88 + 0xd0),*(undefined8 *)(local_88 + 0xd8),
             *(undefined4 *)(local_88 + 0xf0),*(undefined1 *)(local_88 + 0xf4),0,0);
  uVar6 = 0x10;
  if (*(int *)(local_88 + 0xf0) != 0) {
    uVar6 = 0x20;
  }
  FUN_0097eebc(auStack_110,*(undefined8 *)(local_88 + 0xd0),*(undefined8 *)(local_88 + 0xd8),8,
               (ulong)(*(int *)(local_88 + 0xf0) - 1U < 2) << 2,uVar6);
  if (uVar3 != 0) {
    uVar3 = *(uint *)(&PNG_01211a4d.field_0x1b4b + DAT_014879e8);
    uVar1 = (uVar3 ^ uVar3 >> 1) & 0x44441045;
    uVar3 = (uVar1 | uVar1 << 1) ^ uVar3;
    DAT_01487858 = (int)(CONCAT44((uVar3 + (uVar3 >> 0x10)) * 0x101 + (uVar3 >> 0x18),uVar3) >> 0x10
                        );
    if (DAT_01487858 != 1) {
      *(uint *)(DAT_014879f0 + 0x1d8ed28) = uVar3;
    }
    if (DAT_01563d00 != 0) {
      DAT_01487858 = DAT_01487858 + (uint)(byte)(&DAT_0000df8f)[DAT_01563d00];
    }
  }
  lVar9 = local_c8 + local_e0 * 0x20;
  lVar15 = *(long *)(local_88 + 0xd8) * 8 + 0x80;
  uVar6 = 0;
  puVar18 = auStack_140;
  puVar16 = auStack_138;
  puVar14 = &local_130;
  puVar12 = auStack_128;
  puVar10 = auStack_120;
  puVar8 = &local_118;
  lVar11 = lVar9;
  lVar13 = lVar9;
  lVar17 = lVar15;
  lVar19 = lVar15;
  FUN_009c80d4(&local_c0,0x925b0fca);
  iVar5 = FUN_009c610c(local_88);
  uVar1 = DAT_01487854 + iVar5;
  DAT_01487854 = (int)(CONCAT44((uVar1 + (uVar1 >> 0x10)) * 0x101 + (uVar1 >> 0x18),uVar1) >> 0x10);
  FUN_00986540(&local_160,*(undefined8 *)(param_1 + 0x48),*(undefined8 *)(param_1 + 0x58),0,
               *(undefined8 *)(param_1 + 8),param_8);
  local_1c0 = &DAT_013fcc50;
  local_270 = &local_88;
  pplStack_268 = &local_90;
  local_260 = &local_98;
  puStack_258 = &local_a0;
  local_250 = &local_a8;
  puStack_188 = &local_118;
  local_180 = &local_130;
  local_170 = &local_148;
  ppuStack_228 = &local_b0;
  local_220 = &DAT_013fcc80;
  puStack_1e8 = auStack_120;
  local_1e0 = auStack_138;
  local_1d0 = auStack_150;
  local_280 = &DAT_013fccb0;
  puStack_248 = auStack_128;
  local_240 = auStack_140;
  local_230 = auStack_158;
  lStack_278 = param_1;
  plStack_238 = &local_160;
  lStack_218 = param_1;
  local_210 = local_270;
  pplStack_208 = pplStack_268;
  local_200 = local_260;
  puStack_1f8 = puStack_258;
  local_1f0 = local_250;
  plStack_1d8 = &local_160;
  ppuStack_1c8 = ppuStack_228;
  lStack_1b8 = param_1;
  local_1b0 = local_270;
  pplStack_1a8 = pplStack_268;
  local_1a0 = local_260;
  puStack_198 = puStack_258;
  local_190 = local_250;
  plStack_178 = &local_160;
  ppuStack_168 = ppuStack_228;
  if (*(char *)(local_88 + 0xf4) == '\0') {
    if (((DAT_01544cd8 & 1) == 0) && (iVar5 = ___cxa_guard_acquire(&DAT_01544cd8), iVar5 != 0)) {
      thunk_FUN_009fb9f0(&DAT_015442e8,"CompensateImageMotion");
      ___cxa_guard_release(&DAT_01544cd8);
    }
    local_80 = &local_1c0;
    ppuStack_78 = &local_220;
    local_70 = &local_280;
    uVar6 = 3;
    if (lVar7 < 2) {
      uVar6 = 1;
    }
    FUN_009fc3a8(&local_80,3,uVar6,&DAT_015442e8,
                 *(long *)(local_88 + 0xd8) * *(long *)(local_88 + 0xd0) *
                 *(long *)(local_88 + 0xe8));
  }
  else {
    FUN_0098c90c(param_1,*(undefined8 *)(local_88 + 0xb0),local_90[0x16],
                 *(undefined8 *)(local_88 + 0xd0),*(undefined8 *)(local_88 + 0xd8),
                 *(undefined4 *)(local_88 + 0xf0),local_98,local_a0,local_a8,local_118,local_130,
                 *(undefined8 *)(local_160 + 0x510),local_148,local_b0,puVar8,lVar9,puVar10,lVar11,
                 puVar12,lVar13,puVar14,lVar15,puVar16,lVar17,puVar18,lVar19,uVar6);
    local_b0[6] = local_b0[1];
    local_b0[5] = *local_b0;
    *(undefined4 *)(local_b0 + 7) = *(undefined4 *)(local_b0 + 2);
    *(undefined8 *)((long)local_b0 + 0x1c) = local_b0[1];
    *(undefined8 *)((long)local_b0 + 0x14) = *local_b0;
    *(undefined4 *)((long)local_b0 + 0x24) = *(undefined4 *)(local_b0 + 2);
  }
  if (iVar2 + iVar4 == 0) {
    uVar1 = *(uint *)(FUN_00871884 + DAT_014879f8 + 4);
    if (DAT_0148785c != 0) {
      uVar1 = uVar1 >> 0xb | uVar1 << 0x15;
    }
    DAT_0148785c = *(uint *)(DAT_01487a08 + 0x156fe30);
    if (DAT_0148785c != 0) {
      uVar1 = DAT_0148785c;
    }
    *(uint *)(&UNK_013bf638 + DAT_01487a00) = uVar1;
    if (DAT_01563298 != 0) {
      DAT_0148785c = DAT_0148785c + *(byte *)(DAT_01563298 + 0x738);
    }
  }
  if (local_160 != 0) {
    FUN_0097f894(local_160 + 0x718,0xffffffffffffffff);
    local_160 = 0;
  }
  if (0 < local_b8) {
    lVar7 = 0;
    do {
      FUN_009d8290(local_c0);
      lVar7 = lVar7 + 1;
    } while (lVar7 < local_b8);
  }
  if (*(long *)PTR____stack_chk_guard_013b4320 != local_68) {
                    /* WARNING: Subroutine does not return */
    ___stack_chk_fail();
  }
  return;
}


