// FUN_009a775c @ 009a775c size=2416 tag=astfqg12AutoFineTune_L32_parent

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8
FUN_009a775c(long param_1,long param_2,int param_3,uint param_4,undefined8 param_5,
            undefined4 *param_6,undefined4 *param_7,undefined4 *param_8,long param_9,
            undefined1 *param_10,char param_11)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  long lVar4;
  int iVar5;
  long lVar6;
  ulong uVar7;
  long lVar8;
  undefined8 uVar9;
  undefined8 uVar10;
  long *plVar11;
  long lVar12;
  long lVar13;
  long lVar14;
  long lVar15;
  long lVar16;
  long lVar17;
  long lVar18;
  undefined1 *puVar19;
  ulong uVar20;
  long lVar21;
  ulong uVar22;
  undefined4 extraout_s0;
  undefined4 extraout_s0_00;
  undefined4 uVar23;
  undefined *local_100;
  long lStack_f8;
  long *local_f0;
  long *plStack_e8;
  undefined8 *local_e0;
  uint *puStack_d8;
  undefined8 *local_d0;
  long local_c8;
  long local_c0;
  long local_b8;
  undefined1 *local_b0;
  long local_a8;
  long local_a0;
  void *local_98;
  undefined1 *local_90;
  undefined1 auStack_88 [8];
  undefined8 local_80;
  uint local_74;
  long local_70 [2];
  
  lVar18 = DAT_014a67c0;
  if (0 < param_9) {
    lVar18 = param_9;
  }
  local_a0 = lVar18;
  local_80 = param_5;
  local_74 = param_4;
  local_70[0] = param_2;
  if (lVar18 < 1) {
    puVar19 = auStack_88;
    local_98 = (void *)0x0;
    if (lVar18 == 0) goto LAB_009a7814;
  }
  else {
    local_98 = _malloc(lVar18 * 0x50 | 8);
    puVar19 = (undefined1 *)((long)local_98 + 7U & 0xfffffffffffffff8);
  }
  lVar21 = 0;
  local_90 = puVar19;
  do {
    thunk_FUN_009d7bc4(puVar19 + lVar21);
    lVar21 = lVar21 + 0x50;
  } while (lVar18 * 0x50 - lVar21 != 0);
LAB_009a7814:
  uVar3 = DAT_01488496;
  if (param_10 == (undefined1 *)0x0) {
    param_10 = puVar19;
  }
  local_a8 = 0;
  uVar22 = (ulong)DAT_01488496;
  *(undefined **)(DAT_01488cf0 + 0x16eff928) = &DAT_01488494;
  if (*(long *)(DAT_01488ce8 + 0x1a07eb80) == 0) {
    *(undefined1 *)(param_1 + 0x81) = 0;
    iVar5 = DAT_014881dc;
  }
  else {
    uVar22 = -(ulong)*(byte *)(*(long *)(DAT_01488ce8 + 0x1a07eb80) + (uVar22 & 0xffff));
    *(undefined1 *)(param_1 + 0x81) = 0;
    iVar5 = DAT_014881dc;
  }
  DAT_014881dc = iVar5;
  local_b0 = param_10;
  local_90 = puVar19;
  if (param_11 != '\0') {
    uVar1 = *(uint *)(DAT_014885c0 + 0x256f28);
    DAT_014881dc = *(int *)(DAT_014885d0 + 0x161f9f8);
    if (DAT_014881dc != 1) {
      if (iVar5 != 0) {
        uVar1 = uVar1 >> 0x1d | uVar1 << 3;
      }
      *(uint *)(DAT_014885c8 + 0x8fa5f8) = uVar1;
    }
    FUN_00a22470(param_1 + 8);
    *(undefined8 *)(param_1 + 0x70) = 0;
    *(undefined8 *)(param_1 + 0x78) = 0;
    uVar9 = FUN_00a224e4(0x3f800000,param_1 + 8,0);
    *(undefined8 *)(param_1 + 0x78) = uVar9;
  }
  if (param_3 == 0) {
    local_b8 = *(long *)(param_1 + 0x3f90);
  }
  else {
    local_b8 = param_1 + 0x114e8;
  }
  uVar2 = *(uint *)(DAT_01488cf8 + 0x18d7688);
  uVar1 = (uVar2 ^ uVar2 >> 1) & 0x40155445;
  uVar2 = (uVar1 | uVar1 << 1) ^ uVar2;
  if ((int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10) != 1) {
    *(uint *)(&PNG_00ce5d0a.field_0x1ac54e + DAT_01488d00) = uVar2;
  }
  *(undefined8 *)(&DAT_00025bc0 + param_1) = 0x20;
  lVar13 = *(long *)(local_70[0] + 0xd0);
  lVar12 = *(long *)(local_70[0] + 0xd8);
  lVar21 = lVar13 + 0x1f;
  if (-1 < lVar13) {
    lVar21 = lVar13;
  }
  uVar20 = lVar21 >> 5;
  lVar21 = lVar12 + 0x1f;
  if (-1 < lVar12) {
    lVar21 = lVar12;
  }
  lVar21 = lVar21 >> 5;
  *(ulong *)(&DAT_00025bb0 + param_1) = lVar21 * uVar20;
  FUN_00a2279c(*(undefined8 *)(param_1 + 0x78));
  if ((uVar22 + (uVar3 >> 0x10 & 0xff)) * 2 + (ulong)(uVar3 >> 0x18) == 0) {
    lVar12 = *(long *)(&DAT_00025bb0 + param_1);
    FUN_009c80d4(&local_b0,0x57b01d86);
    *(undefined8 *)(param_1 + 0x259f8) = 0;
    *(long *)(param_1 + 0x25a00) = local_c0;
    lVar12 = (lVar12 + 3U & 0xfffffffffffffffc) * 4;
    *(undefined8 *)(param_1 + 0x25a88) = 0;
    *(long *)(param_1 + 0x25a90) = local_c0 + lVar12;
    local_c0 = local_c0 + lVar12 + lVar12;
    *(undefined8 *)(&DAT_00025b18 + param_1) = 0;
    *(long *)(&DAT_00025b20 + param_1) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(param_1 + 0x25a08) = 0;
    *(long *)(param_1 + 0x25a10) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(param_1 + 0x25a98) = 0;
    *(long *)(param_1 + 0x25aa0) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(&LAB_00025b28 + param_1) = 0;
    *(long *)(&DAT_00025b30 + param_1) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(param_1 + 0x25a18) = 0;
    *(long *)(param_1 + 0x25a20) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(param_1 + 0x25aa8) = 0;
    *(long *)(param_1 + 0x25ab0) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(&DAT_00025b38 + param_1) = 0;
    *(long *)(&DAT_00025b40 + param_1) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(param_1 + 0x25a28) = 0;
    *(long *)(param_1 + 0x25a30) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(param_1 + 0x25ab8) = 0;
    *(long *)(param_1 + 0x25ac0) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(&DAT_00025b48 + param_1) = 0;
    *(long *)(&DAT_00025b50 + param_1) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(param_1 + 0x25a38) = 0;
    *(long *)(param_1 + 0x25a40) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(param_1 + 0x25ac8) = 0;
    *(long *)(param_1 + 0x25ad0) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(&DAT_00025b58 + param_1) = 0;
    *(long *)(&DAT_00025b60 + param_1) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(param_1 + 0x25a48) = 0;
    *(long *)(param_1 + 0x25a50) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(param_1 + 0x25ad8) = 0;
    *(long *)(param_1 + 0x25ae0) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(&DAT_00025b68 + param_1) = 0;
    *(long *)(&DAT_00025b70 + param_1) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(param_1 + 0x25a58) = 0;
    *(long *)(param_1 + 0x25a60) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(param_1 + 0x25ae8) = 0;
    *(long *)(param_1 + 0x25af0) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(&DAT_00025b78 + param_1) = 0;
    *(long *)(&DAT_00025b80 + param_1) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(param_1 + 0x25a68) = 0;
    *(long *)(param_1 + 0x25a70) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(param_1 + 0x25af8) = 0;
    *(long *)(&DAT_00025b00 + param_1) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(&DAT_00025b88 + param_1) = 0;
    *(long *)(&DAT_00025b90 + param_1) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(param_1 + 0x25a78) = 0;
    *(long *)(param_1 + 0x25a80) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(&DAT_00025b08 + param_1) = 0;
    *(long *)(&DAT_00025b10 + param_1) = local_c0;
    local_c0 = local_c0 + lVar12;
    *(undefined8 *)(&DAT_00025b98 + param_1) = 0;
    *(long *)(&LAB_00025ba0 + param_1) = local_c0;
    uVar3 = DAT_0148849c;
    local_c0 = local_c0 + lVar12;
    if (DAT_01488318 >> 0x1e == 0) {
      uVar22 = (ulong)DAT_0148849c;
      *(undefined **)(DAT_01488d10 + 0x16f3d478) = &DAT_0148849a;
      if (*(long *)(DAT_01488d08 + 0x1a346f40) != 0) {
        uVar22 = -(ulong)*(byte *)(*(long *)(DAT_01488d08 + 0x1a346f40) + (uVar22 & 0xffff));
      }
      lVar21 = lVar21 + ((ulong)(uVar3 >> 0x18) + (uVar22 + (uVar3 >> 0x10 & 0xff)) * 2) * 0x40;
    }
    DAT_01488318 = (int)(CONCAT44((DAT_01488318 + (DAT_01488318 >> 0x10)) * 0x101 +
                                  (DAT_01488318 >> 0x18),DAT_01488318) >> 0x10) + (int)local_70[0];
    if (local_74 == 2) {
      FUN_009d39ec(local_c8,*(undefined8 *)(&DAT_00025bb0 + param_1));
    }
    lVar4 = _UNK_00c95958;
    lVar12 = _DAT_00c95950;
    if ((0 < lVar21) && (0x1f < lVar13)) {
      lVar13 = 0;
      lVar14 = *(long *)(&DAT_00025bc0 + param_1);
      lVar15 = *(long *)(&DAT_00025ba8 + param_1);
      uVar22 = uVar20 & 0xfffffffffffffffc;
      lVar6 = 0;
      do {
        lVar16 = lVar14 * lVar13;
        lVar17 = lVar6;
        if (local_74 == 2) {
          lVar6 = 0;
          uVar7 = uVar20;
          do {
            plVar11 = (long *)(lVar15 + *(long *)(local_c8 + lVar17 * 8) * 0x20);
            *plVar11 = lVar16;
            plVar11[1] = lVar6;
            plVar11[2] = lVar14;
            plVar11[3] = lVar14;
            lVar17 = lVar17 + 1;
            lVar6 = lVar6 + lVar14;
            uVar7 = uVar7 - 1;
          } while (uVar7 != 0);
        }
        else {
          if (uVar20 < 4) {
            uVar7 = 0;
          }
          else {
            lVar17 = lVar6 + uVar22;
            plVar11 = (long *)(lVar15 + 0x40 + lVar6 * 0x20);
            uVar7 = uVar22;
            lVar6 = lVar12;
            lVar8 = lVar4;
            do {
              plVar11[-8] = lVar16;
              plVar11[-7] = lVar14 * lVar6;
              plVar11[-6] = lVar14;
              plVar11[-5] = lVar14;
              plVar11[-4] = lVar16;
              plVar11[-3] = lVar14 * lVar8;
              plVar11[-2] = lVar14;
              plVar11[-1] = lVar14;
              *plVar11 = lVar16;
              plVar11[1] = lVar14 * (lVar6 + 2);
              plVar11[2] = lVar14;
              plVar11[3] = lVar14;
              plVar11[4] = lVar16;
              plVar11[5] = lVar14 * (lVar8 + 2);
              plVar11[6] = lVar14;
              plVar11[7] = lVar14;
              lVar6 = lVar6 + 4;
              lVar8 = lVar8 + 4;
              plVar11 = plVar11 + 0x10;
              uVar7 = uVar7 - 4;
            } while (uVar7 != 0);
            uVar7 = uVar22;
            if (uVar20 == uVar22) goto LAB_009a7c2c;
          }
          lVar6 = lVar14 * uVar7;
          lVar8 = uVar20 - uVar7;
          plVar11 = (long *)(lVar15 + 0x10 + lVar17 * 0x20);
          do {
            plVar11[-2] = lVar16;
            plVar11[-1] = lVar6;
            lVar17 = lVar17 + 1;
            lVar6 = lVar6 + lVar14;
            *plVar11 = lVar14;
            plVar11[1] = lVar14;
            lVar8 = lVar8 + -1;
            plVar11 = plVar11 + 4;
          } while (lVar8 != 0);
        }
LAB_009a7c2c:
        lVar13 = lVar13 + 1;
        lVar6 = lVar17;
      } while (lVar13 != lVar21);
    }
    uVar3 = *(uint *)(DAT_01488d18 + 0x1d77500);
    if (DAT_0148831c != 0) {
      uVar3 = uVar3 >> 0x10 | uVar3 << 0x10;
    }
    DAT_0148831c = (int)(CONCAT44((uVar3 + (uVar3 >> 0x10)) * 0x101 + (uVar3 >> 0x18),uVar3) >> 0x10
                        );
    if (DAT_0148831c != 0) {
      *(uint *)(DAT_01488d20 + 0x352af0) = uVar3;
    }
    if (DAT_015666c8 != 0) {
      DAT_0148831c = DAT_0148831c + (uint)(byte)(&DAT_000027ea)[DAT_015666c8];
    }
    *(undefined8 *)(&DAT_00025bb8 + param_1) = 0;
    local_100 = &DAT_013fd300;
    local_f0 = local_70;
    plStack_e8 = &local_b8;
    local_e0 = &local_80;
    puStack_d8 = &local_74;
    local_d0 = &param_10;
    lStack_f8 = param_1;
    if (((DAT_015487e8 & 1) == 0) && (iVar5 = ___cxa_guard_acquire(&DAT_015487e8), iVar5 != 0)) {
      thunk_FUN_009fb9f0(&DAT_01547428,"AutoFineTune");
      ___cxa_guard_release(&DAT_015487e8);
    }
    uVar9 = FUN_009fc6e8(&local_100,lVar18,&DAT_01547428,
                         *(long *)(local_70[0] + 0xd8) * *(long *)(local_70[0] + 0xd0) *
                         *(long *)(local_70[0] + 0xe8));
    uVar1 = *(uint *)(DAT_01488d28 + 0x1d02c70);
    uVar3 = (uVar1 ^ uVar1 >> 1) & 0x40104440;
    DAT_01488320 = (uVar3 | uVar3 << 1) ^ uVar1;
    if (DAT_01488320 != 0) {
      *(uint *)(BYTE_ARRAY_ARRAY_015b2719[0x9a0] + DAT_01488d30 + 7) = DAT_01488320;
    }
    if ((int)uVar9 != 0) {
      if (local_74 < 2) {
        uVar10 = FUN_009b4e08(uVar9,param_1 + 0x259f8,local_b8 + 0xbf44);
        uVar10 = FUN_009b4e08(uVar10,param_1 + 0x25a88,local_b8 + 0xc064,local_74);
        uVar10 = FUN_009b4e08(uVar10,&DAT_00025b18 + param_1,local_b8 + 0xc184,local_74);
        lVar18 = local_b8;
        uVar3 = *(uint *)(DAT_014891f8 + 0x50f3f8);
        if (DAT_014883fc != 0) {
          uVar3 = uVar3 >> 0x12 | uVar3 << 0xe;
        }
        if (uVar3 != 1) {
          *(uint *)("QColor::fromHsl: HSL parameters out of range" + DAT_01489200 + 3) = uVar3;
        }
        uVar10 = FUN_009bdf30(0x3e4ccccd,0x40400000,uVar10,local_b8 + 0xbf44,0);
        uVar10 = FUN_009bdf30(0x3e4ccccd,0x40400000,uVar10,lVar18 + 0xc064,0);
        FUN_009bdf30(0x3e4ccccd,0x40400000,uVar10,lVar18 + 0xc184,0);
      }
      else if ((((local_74 == 2) && (param_6 != (undefined4 *)0x0)) &&
               (param_7 != (undefined4 *)0x0)) && (param_8 != (undefined4 *)0x0)) {
        uVar10 = FUN_009b5408(uVar9,param_1 + 0x259f8,local_b8 + 0xbf44);
        *param_6 = extraout_s0;
        uVar10 = FUN_009b5408(uVar10,param_1 + 0x25a88,local_b8 + 0xc064);
        *param_7 = extraout_s0_00;
        uVar23 = FUN_009b5408(uVar10,&DAT_00025b18 + param_1,local_b8 + 0xc184);
        *param_8 = uVar23;
      }
    }
    *(undefined1 *)(local_b8 + 0x2d) = 0;
  }
  else {
    uVar9 = 0;
  }
  if (0 < local_a8) {
    lVar18 = 0;
    do {
      FUN_009d8290(local_b0);
      lVar18 = lVar18 + 1;
    } while (lVar18 < local_a8);
  }
  if (0 < local_a0) {
    lVar21 = 0;
    lVar18 = 0;
    do {
      thunk_FUN_009d841c(local_90 + lVar21);
      lVar18 = lVar18 + 1;
      lVar21 = lVar21 + 0x50;
    } while (lVar18 < local_a0);
  }
  if (local_98 != (void *)0x0) {
    _free(local_98);
  }
  return uVar9;
}


