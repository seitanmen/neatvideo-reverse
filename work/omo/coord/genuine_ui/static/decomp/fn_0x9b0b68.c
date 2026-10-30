// FUN_009b0b68 size=1156

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
  undefined *puStack_5db8;
  long lStack_5db0;
  long *plStack_5da8;
  long **pplStack_5da0;
  long **pplStack_5d98;
  long **pplStack_5d90;
  undefined1 *puStack_5d88;
  undefined1 *puStack_5d80;
  undefined1 *puStack_5d78;
  long *plStack_5d70;
  undefined1 auStack_5d68 [23760];
  long lStack_98;
  long lStack_90;
  undefined1 *puStack_88;
  undefined1 auStack_80 [14];
  undefined1 uStack_72;
  undefined1 uStack_71;
  long *plStack_70;
  long *plStack_68;
  long *plStack_60;
  long lStack_58;
  
  uStack_72 = param_7;
  auVar11 = (*(code *)PTR____chkstk_darwin_013b4308)();
  lVar6 = auVar11._8_8_;
  lVar7 = auVar11._0_8_;
  uStack_71 = (undefined1)param_6;
  plStack_70 = param_5;
  plStack_68 = param_4;
  plStack_60 = param_3;
  lStack_58 = lVar6;
  func_0x0099ead0(lVar7,param_6);
  if ((*(long *)(lVar6 + 0xd0) < 0x30) || (*(long *)(lVar6 + 0xd8) < 0x30)) {
    return 0;
  }
  *(undefined1 *)(lVar7 + 0x81) = 0;
  lVar6 = lRam00000000014a67c0;
  lStack_98 = lRam00000000014a67c0;
  if (lRam00000000014a67c0 < 1) {
    puVar8 = auStack_80;
    lStack_90 = 0;
    if (lRam00000000014a67c0 == 0) goto LAB_009b0c6c;
  }
  else {
    lStack_90 = _malloc(lRam00000000014a67c0 * 0x50 | 8);
    puVar8 = (undefined1 *)(lStack_90 + 7U & 0xfffffffffffffff8);
  }
  lVar10 = 0;
  puStack_88 = puVar8;
  do {
    func_0x009d7d58(puVar8 + lVar10);
    lVar10 = lVar10 + 0x50;
  } while (lVar6 * 0x50 - lVar10 != 0);
LAB_009b0c6c:
  uRam00000000014882ec = *(uint *)(&UNK_00db5950 + lRam0000000001488b68);
  uVar1 = (uRam00000000014882ec ^ uRam00000000014882ec >> 1) & 0x41055150;
  uRam00000000014882ec = (uVar1 | uVar1 << 1) ^ uRam00000000014882ec;
  if (uRam00000000014882ec != 0) {
    *(uint *)(&UNK_010a9100 + lRam0000000001488b70) = uRam00000000014882ec;
  }
  puStack_88 = puVar8;
  func_0x00ac10bc(auStack_5d68);
  lVar10 = lStack_58;
  func_0x00ac1194(auStack_5d68);
  func_0x00ac0d94(auStack_5d68);
  bVar2 = plStack_60 != (long *)0x0;
  if (bVar2) {
    (**(code **)(*plStack_60 + 0x10))
              (plStack_60,*(undefined8 *)(lVar10 + 0xd0),*(undefined8 *)(lVar10 + 0xd8),param_8,
               uStack_71,0,0);
  }
  uVar9 = (ulong)bVar2;
  if (plStack_68 != (long *)0x0) {
    (**(code **)(*plStack_68 + 0x10))
              (plStack_68,*(undefined8 *)(lStack_58 + 0xd0),*(undefined8 *)(lStack_58 + 0xd8),
               param_8,uStack_71,0,0);
    uVar9 = uVar9 + 1;
  }
  if (plStack_70 != (long *)0x0) {
    (**(code **)(*plStack_70 + 0x10))
              (plStack_70,*(undefined8 *)(lStack_58 + 0xd0),*(undefined8 *)(lStack_58 + 0xd8),
               param_8,uStack_71,0,0);
    uVar9 = uVar9 + 1;
  }
  uVar1 = *(uint *)(&UNK_00256f28 + lRam00000000014885c0);
  iVar4 = *(int *)(lRam00000000014885d0 + 0x161f9f8);
  if (iVar4 != 1) {
    if (iRam00000000014881dc != 0) {
      uVar1 = uVar1 >> 0x1d | uVar1 << 3;
    }
    *(uint *)(&UNK_008fa5f8 + lRam00000000014885c8) = uVar1;
  }
  iRam00000000014881dc = iVar4;
  func_0x00a22470(lVar7 + 8);
  *(undefined8 *)(lVar7 + 0x70) = 0;
  *(undefined8 *)(lVar7 + 0x78) = 0;
  uVar5 = func_0x00a22934(0x3f800000,lVar7 + 8,
                          *(long *)(lStack_58 + 0xd8) * *(long *)(lStack_58 + 0xd0));
  *(undefined8 *)(lVar7 + 0x70) = uVar5;
  uVar1 = *(uint *)(&UNK_01378d40 + lRam0000000001488b78);
  if (iRam00000000014882f0 != 0) {
    uVar1 = uVar1 >> 9 | uVar1 << 0x17;
  }
  if ((int)(CONCAT44((uVar1 + (uVar1 >> 0x10)) * 0x101 + (uVar1 >> 0x18),uVar1) >> 0x10) != 0) {
    *(uint *)(lRam0000000001488b80 + 0x174a130) = uVar1;
  }
  puStack_5db8 = &UNK_013fcfa0;
  plStack_5da8 = &lStack_58;
  pplStack_5da0 = &plStack_60;
  pplStack_5d98 = &plStack_68;
  pplStack_5d90 = &plStack_70;
  puStack_5d88 = &uStack_71;
  puStack_5d80 = &uStack_72;
  puStack_5d78 = auStack_5d68;
  plStack_5d70 = &lStack_98;
  lStack_5db0 = lVar7;
  if (((bRam00000000015487c8 & 1) == 0) && (iVar4 = ___cxa_guard_acquire(0x15487c8), iVar4 != 0)) {
    func_0x009fbba8(0x15487f8,"BuildFreqImage");
    ___cxa_guard_release(0x15487c8);
  }
  uVar5 = func_0x009fc6e8(&puStack_5db8,lVar6,0x15487f8,
                          *(long *)(lStack_58 + 0xd0) * uVar9 * *(long *)(lStack_58 + 0xd8) *
                          *(long *)(lStack_58 + 0xe8) *
                          *(long *)(&UNK_01247820 + (param_8 & 0xffffffff) * 8));
  uVar3 = *(uint *)(&UNK_006202f0 + lRam0000000001488b88);
  uVar1 = (uVar3 ^ uVar3 >> 1) & 0x41100004;
  uVar3 = (uVar1 | uVar1 << 1) ^ uVar3;
  if ((int)(CONCAT44((uVar3 + (uVar3 >> 0x10)) * 0x101 + (uVar3 >> 0x18),uVar3) >> 0x10) != 0) {
    *(uint *)(&UNK_013d4160 + lRam0000000001488b90) = uVar3;
  }
  func_0x00ac12c4(auStack_5d68);
  if (0 < lStack_98) {
    lVar6 = 0;
    lVar7 = 0;
    do {
      func_0x009d8564(puStack_88 + lVar6);
      lVar7 = lVar7 + 1;
      lVar6 = lVar6 + 0x50;
    } while (lVar7 < lStack_98);
  }
  if (lStack_90 != 0) {
    _free();
  }
  return uVar5;
}


