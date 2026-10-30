// FUN_009ac2b0 @ 009ac2b0 size=2372 tag=astfqg6Filter_L11_parent

uint FUN_009ac2b0(undefined8 param_1,undefined8 param_2,undefined8 param_3,undefined4 param_4)

{
  undefined8 *puVar1;
  byte *pbVar2;
  undefined8 *puVar3;
  undefined8 *puVar4;
  ulong uVar5;
  byte bVar6;
  bool bVar7;
  bool bVar8;
  int iVar9;
  int iVar10;
  uint uVar11;
  undefined8 *puVar12;
  undefined8 uVar13;
  undefined8 uVar14;
  undefined4 *puVar15;
  undefined8 *puVar16;
  undefined8 uVar17;
  long lVar18;
  ulong uVar19;
  undefined1 *puVar20;
  int iVar21;
  undefined8 *puVar22;
  long lVar23;
  undefined8 uVar24;
  undefined2 uVar25;
  undefined8 uVar26;
  undefined8 uVar27;
  undefined1 auVar28 [16];
  undefined8 in_stack_ffffffffffffa770;
  undefined4 uVar29;
  uint local_5844;
  undefined *local_57f8;
  undefined8 *puStack_57f0;
  long *local_57e8;
  undefined8 *local_57e0;
  undefined4 *local_57d8;
  undefined4 *local_57d0;
  long *local_57c8;
  undefined8 local_57c0;
  undefined8 uStack_57b8;
  undefined8 local_57b0;
  undefined4 local_57a4;
  uint uStack_57a0;
  undefined1 local_579c;
  long local_5798;
  undefined1 auStack_5790 [248];
  undefined4 local_5698 [10];
  long local_5670;
  long local_5668;
  long local_168;
  void *local_160;
  undefined1 *local_158;
  undefined1 auStack_150 [8];
  undefined8 local_148;
  long local_140;
  undefined4 local_138 [30];
  undefined8 local_c0 [7];
  long local_88;
  
  auVar28 = (*(code *)PTR____chkstk_darwin_013b4308)();
  local_140 = auVar28._8_8_;
  puVar12 = auVar28._0_8_;
  local_88 = *(long *)PTR____stack_chk_guard_013b4320;
  pbVar2 = (byte *)((long)puVar12 + 0x25bca);
  uVar11 = *(uint *)(&UNK_0136ba08 + DAT_01488818);
  if (DAT_0148825c != 0) {
    uVar11 = uVar11 >> 0x11 | uVar11 << 0xf;
  }
  local_5844 = (uint)(CONCAT44((uVar11 + (uVar11 >> 0x10)) * 0x101 + (uVar11 >> 0x18),uVar11) >>
                     0x10);
  if (local_5844 != 1) {
    *(uint *)(DAT_01488820 + 0x8209d8) = uVar11;
  }
  *(undefined1 *)((long)puVar12 + 0x81) = 0;
  uVar11 = *(uint *)(DAT_014885c0 + 0x256f28);
  iVar21 = *(int *)(DAT_014885d0 + 0x161f9f8);
  if (iVar21 != 1) {
    if (DAT_014881dc != 0) {
      uVar11 = uVar11 >> 0x1d | uVar11 << 3;
    }
    *(uint *)(DAT_014885c8 + 0x8fa5f8) = uVar11;
  }
  puVar1 = puVar12 + 1;
  DAT_014881dc = iVar21;
  local_148 = param_3;
  FUN_00a22470(puVar1);
  puVar22 = puVar12 + 0xe;
  *puVar22 = 0;
  puVar12[0xf] = 0;
  local_5698[0] = 0;
  FUN_009f13bc(puVar12 + 0x4afa,local_5698,0x70);
  local_5698[0] = 0;
  FUN_009f13bc(puVar12 + 0x4b08,local_5698,0x68);
  puVar3 = puVar12 + 0x4b15;
  local_5698[0] = 0;
  FUN_009f13bc(puVar3,local_5698,0x68);
  uVar29 = (undefined4)((ulong)in_stack_ffffffffffffa770 >> 0x20);
  *pbVar2 = 0;
  lVar18 = puVar12[0x3d4d];
  if (lVar18 != 0) {
    *(undefined1 *)(lVar18 + 0x5b4) = 0;
  }
  lVar18 = DAT_014a67b8;
  if (*(char *)(puVar12[0x7f2] + 0xbf34) == '\0') {
    FUN_009d9bdc("Warning: calling filter without initialized profile.",0,0,0xffffffffffffffff);
    uVar11 = 0;
    goto LAB_009acb68;
  }
  local_168 = DAT_014a67b8;
  if (DAT_014a67b8 < 1) {
    puVar20 = auStack_150;
    local_160 = (void *)0x0;
    if (DAT_014a67b8 != 0) goto LAB_009ac4f4;
  }
  else {
    local_160 = _malloc(DAT_014a67b8 * 0x50 | 8);
    puVar20 = (undefined1 *)((long)local_160 + 7U & 0xfffffffffffffff8);
LAB_009ac4f4:
    lVar23 = 0;
    local_158 = puVar20;
    do {
      thunk_FUN_009d7bc4(puVar20 + lVar23);
      uVar29 = (undefined4)((ulong)in_stack_ffffffffffffa770 >> 0x20);
      lVar23 = lVar23 + 0x50;
    } while (lVar18 * 0x50 - lVar23 != 0);
  }
  local_158 = puVar20;
  FUN_0099f79c(puVar12);
  FUN_0099e264(puVar12,0);
  FUN_0099ead0(puVar12,0);
  uVar11 = DAT_0148848a;
  uVar19 = (ulong)DAT_0148848a;
  *(undefined **)(DAT_01488830 + 0xc9eb6f8) = &DAT_01488488;
  if (*(long *)(DAT_01488828 + 0x14e42ce0) != 0) {
    uVar19 = -(ulong)*(byte *)(*(long *)(DAT_01488828 + 0x14e42ce0) + (uVar19 & 0xffff));
  }
  lVar18 = *(long *)(local_140 + 0xd0);
  lVar23 = *(long *)(local_140 + 0xd8);
  if ((lVar18 < 0x30) || (lVar23 < 0x30)) {
    uVar11 = 1;
    FUN_009c7318(local_140,0,0,lVar18,lVar23,local_148,param_4,1);
  }
  else {
    FUN_009c7318(local_140,0,0,lVar18,lVar23,local_148,param_4,1);
    if ((uVar19 + (uVar11 >> 0x10 & 0xff)) * 2 + (ulong)(uVar11 >> 0x18) == 0) {
      puVar15 = local_5698;
      FUN_00ac1368(local_5698);
      lVar18 = puVar12[0x7f0] + puVar12[0x7f1] * 2;
      FUN_00addccc(auStack_5790,local_140,param_4,(long)*(int *)(puVar12[0x7a0] + 0x104),
                   (long)((float)(lVar18 * lVar18) * 4.0),0,0,0,CONCAT44(uVar29,2),puVar15);
      bVar7 = false;
      puVar4 = puVar12 + 0x4889;
      do {
        local_5798 = 0;
        FUN_00ac1660(puVar12 + 0x4b3a,&local_5798);
        uVar11 = *(uint *)(DAT_014885c0 + 0x256f28);
        iVar21 = *(int *)(DAT_014885d0 + 0x161f9f8);
        if (iVar21 != 1) {
          if (DAT_014881dc != 0) {
            uVar11 = uVar11 >> 0x1d | uVar11 << 3;
          }
          *(uint *)(DAT_014885c8 + 0x8fa5f8) = uVar11;
        }
        DAT_014881dc = iVar21;
        FUN_00a22470(puVar1);
        *puVar22 = 0;
        puVar12[0xf] = 0;
        local_138[0] = 0;
        FUN_009f13bc(puVar12 + 0x4afa,local_138,0x70);
        local_138[0] = 0;
        FUN_009f13bc(puVar12 + 0x4b08,local_138,0x68);
        local_138[0] = 0;
        FUN_009f13bc(puVar3,local_138,0x68);
        bVar6 = *pbVar2 | bVar7;
        bVar7 = bVar6 != 0;
        *pbVar2 = 0;
        lVar18 = puVar12[0x3d4d];
        if (lVar18 != 0) {
          *(undefined1 *)(lVar18 + 0x5b4) = 0;
        }
        uStack_57a0 = *(uint *)((long)puVar12 + 0x25bcc) & 0x1ffff;
        local_57a4 = 0;
        uVar25 = NEON_uaddlv((ulong)CONCAT12(POPCOUNT((char)(uStack_57a0 >> 0x10)),
                                             CONCAT11(POPCOUNT((char)(uStack_57a0 >> 8)),
                                                      POPCOUNT((char)uStack_57a0))),1);
        local_579c = (undefined1)uVar25;
        uStack_57b8 = puVar12[0x4b7c];
        local_57c0 = puVar12[0x4b7b];
        local_57b0 = puVar12[0x4b7d];
        FUN_00a06540(local_138,puVar12,&uStack_57a0,&local_57a4,&local_57c0,auStack_5790,0,
                     *(undefined1 *)((long)puVar12 + 0x25bcb),pbVar2,"");
        FUN_009f7c54(puVar12 + 0x4b7f,local_c0);
        if (*pbVar2 == 0) {
          uVar19 = (**(code **)*puVar12)(puVar12);
          uVar17 = local_c0[0];
          if ((uVar19 & 1) == 0) {
            uVar24 = puVar12[0x49c1];
            lVar18 = puVar12[0x7f0];
            lVar23 = puVar12[0x7f1];
            uVar14 = *(undefined8 *)(local_140 + 0xd0);
            uVar26 = *(undefined8 *)(local_140 + 0xd8);
            uVar13 = FUN_009a3988(puVar12[0x7a0]);
            lVar18 = lVar18 + lVar23 * 2;
            FUN_00ac2088(puVar4,uVar24,lVar18 * lVar18,uVar14,uVar26,param_4,uVar13,local_138);
            FUN_00ac14d4(local_5698,0,0);
            FUN_00ac0d94(local_5698);
            uVar14 = FUN_00a22934(0x3f800000,puVar1,local_5668 * local_5670);
            *puVar22 = uVar14;
            FUN_00ac2b20(puVar4);
            if (local_5844 == 0) {
              iVar21 = 0;
            }
            else {
              uVar11 = *(uint *)(DAT_01488838 + 0x16e8308);
              if (DAT_01488260 != 0) {
                uVar11 = uVar11 >> 5 | uVar11 << 0x1b;
              }
              iVar21 = (int)(CONCAT44((uVar11 + (uVar11 >> 0x10)) * 0x101 + (uVar11 >> 0x18),uVar11)
                            >> 0x10);
              if (iVar21 != 1) {
                *(uint *)(DAT_01488840 + 0xb00ad8) = uVar11;
              }
              DAT_01488260 = iVar21;
              if (DAT_01566b30 != 0) {
                iVar21 = iVar21 + (uint)*(byte *)(DAT_01566b30 + 0xca01);
                DAT_01488260 = iVar21;
              }
            }
            local_57f8 = &DAT_013fcf40;
            local_57e8 = &local_140;
            local_57e0 = &local_148;
            local_57d8 = local_5698;
            local_57d0 = local_138;
            local_57c8 = &local_168;
            puStack_57f0 = puVar12;
            puVar15 = (undefined4 *)FUN_009fc6e8(&local_57f8,uVar17,0,0);
            iVar9 = (**(code **)*puVar12)(puVar12);
            iVar10 = FUN_00ac17a4(puVar12 + 0x4b3a);
            uVar19 = 0x80;
            if (iVar9 == 0) {
              uVar19 = 0;
            }
            uVar5 = 2;
            if (iVar10 == 0) {
              uVar5 = 0;
            }
            puVar16 = (undefined8 *)FUN_00ac2c0c(puVar4,local_5698,uVar5 | uVar19,local_138);
            uVar14 = puVar16[1];
            uVar17 = *puVar16;
            uVar26 = puVar16[2];
            uVar24 = puVar16[5];
            uVar13 = puVar16[4];
            puVar12[0x4b18] = puVar16[3];
            puVar12[0x4b17] = uVar26;
            puVar12[0x4b1a] = uVar24;
            puVar12[0x4b19] = uVar13;
            puVar12[0x4b16] = uVar14;
            *puVar3 = uVar17;
            uVar14 = puVar16[7];
            uVar17 = puVar16[6];
            uVar13 = puVar16[9];
            uVar26 = puVar16[8];
            uVar27 = puVar16[0xb];
            uVar24 = puVar16[10];
            *(undefined8 *)((long)puVar12 + 0x25905) = *(undefined8 *)((long)puVar16 + 0x5d);
            puVar12[0x4b1e] = uVar13;
            puVar12[0x4b1d] = uVar26;
            puVar12[0x4b20] = uVar27;
            puVar12[0x4b1f] = uVar24;
            puVar12[0x4b1c] = uVar14;
            puVar12[0x4b1b] = uVar17;
            if (iVar21 == 0) {
              local_5844 = 0;
            }
            else {
              uVar11 = *(uint *)(DAT_01488848 + 0x1673a78);
              bVar8 = DAT_01488264 != 0;
              DAT_01488264 = uVar11;
              if (bVar8) {
                DAT_01488264 = uVar11 >> 7 | uVar11 << 0x19;
              }
              local_5844 = DAT_01488264;
              if (DAT_01488264 != 1) {
                *(uint *)(DAT_01488850 + 0x1d73b08) = DAT_01488264;
              }
            }
            bVar8 = *pbVar2 != 0;
          }
          else {
            bVar8 = false;
          }
        }
        else {
          bVar8 = true;
        }
        FUN_00a067d8(local_138);
        uVar17 = 0;
        if (local_5798 != 0) {
          uVar17 = FUN_00ac17dc();
        }
      } while (bVar8);
      FUN_009a4c10(uVar17,local_148,0);
      *pbVar2 = bVar6;
      uVar11 = (**(code **)*puVar12)(puVar12);
      uVar11 = (uint)puVar15 & (uVar11 ^ 1);
      thunk_FUN_00ac0ea0(local_5698);
    }
    else {
      uVar11 = 0;
    }
  }
  if (0 < local_168) {
    lVar23 = 0;
    lVar18 = 0;
    do {
      thunk_FUN_009d841c(local_158 + lVar23);
      lVar18 = lVar18 + 1;
      lVar23 = lVar23 + 0x50;
    } while (lVar18 < local_168);
  }
  if (local_160 != (void *)0x0) {
    _free(local_160);
  }
LAB_009acb68:
  if (*(long *)PTR____stack_chk_guard_013b4320 != local_88) {
                    /* WARNING: Subroutine does not return */
    ___stack_chk_fail();
  }
  return uVar11;
}


