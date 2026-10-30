// FUN_009b1088 size=1792

undefined8
FUN_009b1088(undefined8 *param_1,long param_2,long param_3,long param_4,long param_5,int param_6,
            undefined1 param_7,long *param_8,long param_9)

{
  uint uVar1;
  long lVar2;
  long lVar3;
  long lVar4;
  undefined8 *puVar5;
  long lVar6;
  code *pcVar7;
  long *plVar8;
  ulong uVar9;
  long lVar10;
  long lVar11;
  long lVar12;
  undefined8 *puVar13;
  long lVar14;
  undefined8 *puVar15;
  long lVar16;
  long lVar17;
  long lVar18;
  long lVar19;
  long lVar20;
  long lVar21;
  uint uVar22;
  long lVar23;
  long lVar24;
  long lVar25;
  undefined8 *puVar26;
  long lVar27;
  undefined8 *puVar28;
  undefined8 *puVar29;
  undefined8 *puVar30;
  long lVar31;
  long *plVar32;
  long *plVar33;
  long *plVar34;
  undefined8 uVar35;
  long lStack_d8;
  long lStack_d0;
  long lStack_c8;
  undefined8 uStack_c0;
  undefined8 uStack_b8;
  undefined8 uStack_b0;
  undefined8 uStack_a8;
  long lStack_a0;
  long lStack_98;
  undefined8 auStack_90 [4];
  long lStack_70;
  
  lStack_70 = *(long *)PTR____stack_chk_guard_013b4320;
  lStack_a0 = param_9;
  lStack_98 = 0;
  lVar11 = *(long *)(param_2 + 0xd0);
  uVar22 = *(uint *)(&UNK_00be36f0 + lRam0000000001488b98);
  uVar1 = (uVar22 ^ uVar22 >> 1) & 0x41151540;
  uVar22 = (uVar1 | uVar1 << 1) ^ uVar22;
  if (lRam0000000001568858 != 0) {
    uVar22 = uVar22 + (byte)(&UNK_0000d9e0)[lRam0000000001568858];
  }
  uStack_a8 = 0;
  lVar2 = 3;
  if (param_6 != 0) {
    lVar2 = 1;
  }
  lVar12 = param_8[0xd] << 2;
  lVar18 = 0;
  if (param_3 != 0) {
    lVar18 = lVar12;
  }
  lVar18 = lVar18 * lVar2;
  lVar20 = 0;
  if (param_4 != 0) {
    lVar20 = lVar12;
  }
  lVar20 = lVar20 * lVar2;
  lVar21 = 0;
  if (param_5 != 0) {
    lVar21 = lVar12;
  }
  lVar21 = lVar21 * lVar2;
  uVar35 = 0;
  plVar34 = &lStack_d8;
  plVar33 = &lStack_d0;
  plVar32 = &lStack_c8;
  puVar30 = &uStack_c0;
  puVar29 = &uStack_b8;
  puVar28 = &uStack_b0;
  lVar17 = lVar12;
  lVar31 = lVar12;
  func_0x009c80d4(&lStack_a0,0xd6320003);
  do {
    plVar8 = (long *)(**(code **)(*param_8 + 0x18))(param_8,0);
    if (plVar8 == (long *)0x0) {
      uVar35 = 1;
      if (lStack_98 < 1) goto LAB_009b170c;
      goto LAB_009b16f0;
    }
    if (uVar22 != 0) {
      uVar1 = *(uint *)(&UNK_0116c698 + lRam0000000001488ba0);
      uVar22 = (uVar1 ^ uVar1 >> 1) & 0x41454151;
      uVar1 = (uVar22 | uVar22 << 1) ^ uVar1;
      if (uVar1 != 1) {
        *(uint *)(&UNK_00a4f9e8 + lRam0000000001488ba8) = uVar1;
      }
    }
    lVar24 = *plVar8;
    lVar27 = plVar8[1];
    lVar3 = plVar8[2];
    puVar5 = (undefined8 *)plVar8[3];
    puVar13 = (undefined8 *)plVar8[4];
    lVar14 = plVar8[5];
    puVar15 = (undefined8 *)plVar8[6];
    lVar4 = plVar8[7];
    lVar6 = plVar8[8];
    lVar25 = plVar8[9];
    lVar16 = *(long *)(param_2 + 0xb0);
    uVar1 = *(uint *)(param_2 + 0xf0);
    lVar19 = *(long *)(&UNK_01247820 + (ulong)uVar1 * 8);
    lVar10 = lVar19 * lVar11;
    uVar22 = *(uint *)(&UNK_012e7188 + lRam0000000001488e00);
    uVar22 = ((uVar22 ^ uVar22 >> 1) & 0x44155511) * 3 ^ uVar22;
    if ((int)(CONCAT44((uVar22 + (uVar22 >> 0x10)) * 0x101 + (uVar22 >> 0x18),uVar22) >> 0x10) != 1)
    {
      *(uint *)(&UNK_00054198 + lRam0000000001488e08) = uVar22;
    }
    lVar23 = lVar3 << 2;
    uVar9 = (ulong)puVar28 & 0xffffffff00000000;
    func_0x009c06cc(lVar16 + lVar10 * lVar27 + lVar19 * lVar24,(ulong)uVar1,lVar10,uStack_b0,2,
                    lVar23,lVar3,puVar5,uVar9,lVar12,puVar29,lVar17,puVar30,lVar31,plVar32,lVar18,
                    plVar33,lVar20,plVar34,lVar21,uVar35);
    lVar16 = *(long *)(param_2 + 0xb8);
    uVar1 = *(uint *)(param_2 + 0xf0);
    lVar19 = *(long *)(&UNK_01247820 + (ulong)uVar1 * 8);
    lVar10 = lVar19 * lVar11;
    uVar22 = *(uint *)(&UNK_012e7188 + lRam0000000001488e00);
    uVar22 = ((uVar22 ^ uVar22 >> 1) & 0x44155511) * 3 ^ uVar22;
    if ((int)(CONCAT44((uVar22 + (uVar22 >> 0x10)) * 0x101 + (uVar22 >> 0x18),uVar22) >> 0x10) != 1)
    {
      *(uint *)(&UNK_00054198 + lRam0000000001488e08) = uVar22;
    }
    uVar9 = uVar9 & 0xffffffff00000000;
    func_0x009c06cc(lVar16 + lVar10 * lVar27 + lVar19 * lVar24,(ulong)uVar1,lVar10,uStack_b8,2,
                    lVar23,lVar3,puVar5,uVar9,lVar12,puVar29,lVar17,puVar30,lVar31,plVar32,lVar18,
                    plVar33,lVar20,plVar34,lVar21,uVar35);
    lVar17 = *(long *)(param_2 + 0xc0);
    uVar1 = *(uint *)(param_2 + 0xf0);
    lVar10 = *(long *)(&UNK_01247820 + (ulong)uVar1 * 8);
    lVar12 = lVar10 * lVar11;
    uVar22 = *(uint *)(&UNK_012e7188 + lRam0000000001488e00);
    uVar22 = ((uVar22 ^ uVar22 >> 1) & 0x44155511) * 3 ^ uVar22;
    if ((int)(CONCAT44((uVar22 + (uVar22 >> 0x10)) * 0x101 + (uVar22 >> 0x18),uVar22) >> 0x10) != 1)
    {
      *(uint *)(&UNK_00054198 + lRam0000000001488e08) = uVar22;
    }
    uVar9 = uVar9 & 0xffffffff00000000;
    func_0x009c06cc(lVar17 + lVar12 * lVar27 + lVar10 * lVar24,(ulong)uVar1,lVar12,uStack_c0,2,
                    lVar23,lVar3,puVar5,uVar9);
    uVar22 = *(uint *)(&UNK_00a85d20 + lRam0000000001488bb0);
    uVar1 = *(uint *)(lRam0000000001488bc0 + 0x1e66228);
    if ((int)(CONCAT44((uVar1 + (uVar1 >> 0x10)) * 0x101 + (uVar1 >> 0x18),uVar1) >> 0x10) != 0) {
      if (iRam00000000014882f4 != 0) {
        uVar22 = uVar22 >> 0xe | uVar22 << 0x12;
      }
      *(uint *)(&UNK_011ce210 + lRam0000000001488bb8) = uVar22;
    }
    puVar30 = &uStack_a8;
    puVar28 = (undefined8 *)CONCAT71((int7)(uVar9 >> 8),param_7);
    lVar12 = lVar3;
    puVar29 = puVar5;
    lVar17 = param_9;
    func_0x009b17a4(param_1,uStack_b0,uStack_b8,uStack_c0,lStack_c8,lStack_d0,lStack_d8,param_6,
                    puVar28);
    lVar24 = lStack_d0;
    lVar27 = lStack_d8;
    if (param_3 != 0) {
      auStack_90[1] = *(undefined8 *)(param_3 + 0xb8);
      auStack_90[0] = *(undefined8 *)(param_3 + 0xb0);
      auStack_90[2] = *(undefined8 *)(param_3 + 0xc0);
      puVar26 = auStack_90;
      lVar10 = lStack_c8;
      lVar16 = lVar2;
      do {
        puVar28 = puVar13;
        lVar12 = lVar14;
        puVar29 = puVar15;
        lVar17 = lVar4;
        func_0x009af2f8(param_1,lVar10,*puVar26,*(undefined4 *)(param_3 + 0xf0),lVar3,lVar11,lVar6,
                        lVar25,puVar13);
        puVar26 = puVar26 + 1;
        lVar10 = lVar10 + (long)puVar5 * lVar3 * 4;
        lVar16 = lVar16 + -1;
      } while (lVar16 != 0);
    }
    if (param_4 != 0) {
      auStack_90[1] = *(undefined8 *)(param_4 + 0xb8);
      auStack_90[0] = *(undefined8 *)(param_4 + 0xb0);
      auStack_90[2] = *(undefined8 *)(param_4 + 0xc0);
      puVar26 = auStack_90;
      lVar10 = lVar2;
      do {
        puVar28 = puVar13;
        lVar12 = lVar14;
        puVar29 = puVar15;
        lVar17 = lVar4;
        func_0x009af2f8(param_1,lVar24,*puVar26,*(undefined4 *)(param_4 + 0xf0),lVar3,lVar11,lVar6,
                        lVar25,puVar13);
        puVar26 = puVar26 + 1;
        lVar24 = lVar24 + (long)puVar5 * lVar3 * 4;
        lVar10 = lVar10 + -1;
      } while (lVar10 != 0);
    }
    if (param_5 != 0) {
      auStack_90[1] = *(undefined8 *)(param_5 + 0xb8);
      auStack_90[0] = *(undefined8 *)(param_5 + 0xb0);
      auStack_90[2] = *(undefined8 *)(param_5 + 0xc0);
      puVar26 = auStack_90;
      lVar24 = lVar2;
      do {
        puVar28 = puVar13;
        lVar12 = lVar14;
        puVar29 = puVar15;
        lVar17 = lVar4;
        func_0x009af2f8(param_1,lVar27,*puVar26,*(undefined4 *)(param_5 + 0xf0),lVar3,lVar11,lVar6,
                        lVar25,puVar13);
        puVar26 = puVar26 + 1;
        lVar27 = lVar27 + (long)puVar5 * lVar3 * 4;
        lVar24 = lVar24 + -1;
      } while (lVar24 != 0);
    }
    uVar1 = *(uint *)(lRam0000000001488bc8 + 0x1cb8a40);
    if (iRam00000000014882f8 != 0) {
      uVar1 = uVar1 >> 6 | uVar1 << 0x1a;
    }
    uVar22 = (uint)(CONCAT44((uVar1 + (uVar1 >> 0x10)) * 0x101 + (uVar1 >> 0x18),uVar1) >> 0x10);
    if (uVar22 != 0) {
      *(uint *)(&UNK_000cb270 + lRam0000000001488bd0) = uVar1;
    }
    func_0x00a22868(param_1[0xe],lVar4 * (long)puVar15);
    uVar9 = (**(code **)*param_1)(param_1);
  } while ((uVar9 & 1) == 0);
  uVar35 = 0;
  if (0 < lStack_98) {
LAB_009b16f0:
    lVar11 = 0;
    do {
      func_0x009d8290(lStack_a0);
      lVar11 = lVar11 + 1;
    } while (lVar11 < lStack_98);
  }
LAB_009b170c:
  if (*(long *)PTR____stack_chk_guard_013b4320 != lStack_70) {
    uVar35 = ___stack_chk_fail();
    if (0 < lStack_98) {
      lVar11 = 0;
      do {
        func_0x009d8290(lStack_a0);
        lVar11 = lVar11 + 1;
      } while (lVar11 < lStack_98);
    }
    __Unwind_Resume(uVar35);
                    /* WARNING: Does not return */
    pcVar7 = (code *)SoftwareBreakpoint(1,0x9b17a0);
    (*pcVar7)();
  }
  return uVar35;
}


