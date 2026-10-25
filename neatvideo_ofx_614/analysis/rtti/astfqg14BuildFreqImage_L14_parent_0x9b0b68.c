// FUN_009b0b68 @ 009b0b68 size=1156 tag=astfqg14BuildFreqImage_L14_parent

undefined8
FUN_009b0b68(undefined8 param_1,undefined8 param_2,long *param_3,long *param_4,long *param_5,
            undefined8 param_6,undefined1 param_7,ulong param_8)

{
  uint uVar1;
  bool bVar2;
  uint uVar3;
  int iVar4;
  undefined8 uVar5;
  long lVar6;
  long lVar7;
  undefined1 *puVar8;
  ulong uVar9;
  long lVar10;
  undefined1 auVar11 [16];
  undefined *local_5db8;
  long lStack_5db0;
  long *local_5da8;
  long **pplStack_5da0;
  long **local_5d98;
  long **pplStack_5d90;
  undefined1 *local_5d88;
  undefined1 *puStack_5d80;
  undefined1 *local_5d78;
  long *plStack_5d70;
  undefined1 auStack_5d68 [23760];
  long local_98;
  void *local_90;
  undefined1 *local_88;
  undefined1 auStack_80 [14];
  undefined1 local_72;
  undefined1 local_71;
  long *local_70;
  long *local_68;
  long *local_60;
  long local_58;
  
  local_72 = param_7;
  auVar11 = (*(code *)PTR____chkstk_darwin_013b4308)();
  lVar6 = auVar11._8_8_;
  lVar7 = auVar11._0_8_;
  local_71 = (undefined1)param_6;
  local_70 = param_5;
  local_68 = param_4;
  local_60 = param_3;
  local_58 = lVar6;
  FUN_0099ead0(lVar7,param_6);
  if ((*(long *)(lVar6 + 0xd0) < 0x30) || (*(long *)(lVar6 + 0xd8) < 0x30)) {
    return 0;
  }
  *(undefined1 *)(lVar7 + 0x81) = 0;
  lVar6 = DAT_014a67c0;
  local_98 = DAT_014a67c0;
  if (DAT_014a67c0 < 1) {
    puVar8 = auStack_80;
    local_90 = (void *)0x0;
    if (DAT_014a67c0 == 0) goto LAB_009b0c6c;
  }
  else {
    local_90 = _malloc(DAT_014a67c0 * 0x50 | 8);
    puVar8 = (undefined1 *)((long)local_90 + 7U & 0xfffffffffffffff8);
  }
  lVar10 = 0;
  local_88 = puVar8;
  do {
    thunk_FUN_009d7bc4(puVar8 + lVar10);
    lVar10 = lVar10 + 0x50;
  } while (lVar6 * 0x50 - lVar10 != 0);
LAB_009b0c6c:
  uVar3 = *(uint *)(&PNG_00ce5d0a.field_0xcfc46 + DAT_01488b68);
  uVar1 = (uVar3 ^ uVar3 >> 1) & 0x41055150;
  DAT_014882ec = (uVar1 | uVar1 << 1) ^ uVar3;
  if (DAT_014882ec != 0) {
    *(uint *)(&UNK_010a9100 + DAT_01488b70) = DAT_014882ec;
  }
  local_88 = puVar8;
  thunk_FUN_00ac0ff0(auStack_5d68);
  lVar10 = local_58;
  FUN_00ac1194(auStack_5d68);
  FUN_00ac0d94(auStack_5d68);
  bVar2 = local_60 != (long *)0x0;
  if (bVar2) {
    (**(code **)(*local_60 + 0x10))
              (local_60,*(undefined8 *)(lVar10 + 0xd0),*(undefined8 *)(lVar10 + 0xd8),param_8,
               local_71,0,0);
  }
  uVar9 = (ulong)bVar2;
  if (local_68 != (long *)0x0) {
    (**(code **)(*local_68 + 0x10))
              (local_68,*(undefined8 *)(local_58 + 0xd0),*(undefined8 *)(local_58 + 0xd8),param_8,
               local_71,0,0);
    uVar9 = uVar9 + 1;
  }
  if (local_70 != (long *)0x0) {
    (**(code **)(*local_70 + 0x10))
              (local_70,*(undefined8 *)(local_58 + 0xd0),*(undefined8 *)(local_58 + 0xd8),param_8,
               local_71,0,0);
    uVar9 = uVar9 + 1;
  }
  uVar1 = *(uint *)(DAT_014885c0 + 0x256f28);
  iVar4 = *(int *)(DAT_014885d0 + 0x161f9f8);
  if (iVar4 != 1) {
    if (DAT_014881dc != 0) {
      uVar1 = uVar1 >> 0x1d | uVar1 << 3;
    }
    *(uint *)(DAT_014885c8 + 0x8fa5f8) = uVar1;
  }
  DAT_014881dc = iVar4;
  FUN_00a22470(lVar7 + 8);
  *(undefined8 *)(lVar7 + 0x70) = 0;
  *(undefined8 *)(lVar7 + 0x78) = 0;
  uVar5 = FUN_00a22934(0x3f800000,lVar7 + 8,*(long *)(local_58 + 0xd8) * *(long *)(local_58 + 0xd0))
  ;
  *(undefined8 *)(lVar7 + 0x70) = uVar5;
  uVar1 = *(uint *)(&UNK_01378d40 + DAT_01488b78);
  if (DAT_014882f0 != 0) {
    uVar1 = uVar1 >> 9 | uVar1 << 0x17;
  }
  if ((int)(CONCAT44((uVar1 + (uVar1 >> 0x10)) * 0x101 + (uVar1 >> 0x18),uVar1) >> 0x10) != 0) {
    *(uint *)(DAT_01488b80 + 0x174a130) = uVar1;
  }
  local_5db8 = &DAT_013fcfa0;
  local_5da8 = &local_58;
  pplStack_5da0 = &local_60;
  local_5d98 = &local_68;
  pplStack_5d90 = &local_70;
  local_5d88 = &local_71;
  puStack_5d80 = &local_72;
  local_5d78 = auStack_5d68;
  plStack_5d70 = &local_98;
  lStack_5db0 = lVar7;
  if (((DAT_015487c8 & 1) == 0) && (iVar4 = ___cxa_guard_acquire(&DAT_015487c8), iVar4 != 0)) {
    thunk_FUN_009fb9f0(&DAT_015487f8,"BuildFreqImage");
    ___cxa_guard_release(&DAT_015487c8);
  }
  uVar5 = FUN_009fc6e8(&local_5db8,lVar6,&DAT_015487f8,
                       *(long *)(local_58 + 0xd0) * uVar9 * *(long *)(local_58 + 0xd8) *
                       *(long *)(local_58 + 0xe8) *
                       *(long *)(&DAT_01247820 + (param_8 & 0xffffffff) * 8));
  uVar3 = *(uint *)(DAT_01488b88 + 0x6202f0);
  uVar1 = (uVar3 ^ uVar3 >> 1) & 0x41100004;
  uVar3 = (uVar1 | uVar1 << 1) ^ uVar3;
  if ((int)(CONCAT44((uVar3 + (uVar3 >> 0x10)) * 0x101 + (uVar3 >> 0x18),uVar3) >> 0x10) != 0) {
    *(uint *)(&UNK_013d4160 + DAT_01488b90) = uVar3;
  }
  FUN_00ac12c4(auStack_5d68);
  if (0 < local_98) {
    lVar6 = 0;
    lVar7 = 0;
    do {
      thunk_FUN_009d841c(local_88 + lVar6);
      lVar7 = lVar7 + 1;
      lVar6 = lVar6 + 0x50;
    } while (lVar7 < local_98);
  }
  if (local_90 != (void *)0x0) {
    _free(local_90);
  }
  return uVar5;
}


