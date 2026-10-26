// FUN_009987ac size=1428

void FUN_009987ac(undefined8 param_1,undefined8 param_2,long param_3,uint param_4,int param_5,
                 long param_6,long param_7,undefined8 param_8)

{
  bool bVar1;
  uint uVar2;
  float fVar3;
  long extraout_x1;
  ulong uVar4;
  ulong uVar5;
  long lVar6;
  ulong uVar7;
  undefined8 *puVar8;
  long lVar9;
  long lVar10;
  long lVar11;
  long lVar12;
  long lVar13;
  uint uVar14;
  float fVar15;
  long alStack_7218 [2];
  uint uStack_7204;
  undefined2 *puStack_7200;
  undefined2 *puStack_71f8;
  uint uStack_71f0;
  int iStack_71ec;
  long lStack_71e8;
  long alStack_71e0 [5];
  undefined1 auStack_71b1 [9];
  ulong auStack_71a8 [5];
  undefined1 auStack_7179 [57];
  undefined8 auStack_7140 [6];
  undefined1 auStack_7110 [14352];
  long lStack_3900;
  ulong uStack_38f8;
  undefined4 uStack_38f0;
  undefined2 auStack_38ec [6];
  undefined1 auStack_38e0 [14352];
  long lStack_d0;
  ulong uStack_c8;
  undefined4 uStack_c0;
  undefined2 auStack_bc [6];
  long lStack_b0;
  long lStack_a8;
  long lStack_a0;
  undefined8 uStack_98;
  long lStack_90;
  
  uStack_71f0 = param_4;
  iStack_71ec = param_5;
  (*(code *)PTR____chkstk_darwin_013b4308)();
  lStack_90 = 0;
  uVar2 = *(uint *)(&UNK_00b0a5b0 + lRam0000000001487ef8);
  uVar14 = (uVar2 ^ uVar2 >> 1) & 0x45514050;
  uVar2 = (uVar14 | uVar14 << 1) ^ uVar2;
  iRam0000000001487b14 =
       (int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10);
  if (lRam00000000015647d8 != 0) {
    iRam0000000001487b14 = iRam0000000001487b14 + (uint)(byte)(&UNK_00005eb0)[lRam00000000015647d8];
  }
  alStack_7218[0] = param_7 * param_6;
  alStack_7218[1] = param_3;
  uStack_98 = param_8;
  func_0x009c80d4(&uStack_98,0x3fc5276e);
  uStack_c0 = 0;
  uStack_c8 = 0;
  lStack_d0 = 0;
  puStack_71f8 = auStack_bc;
  puStack_7200 = auStack_38ec;
  uStack_38f0 = 0;
  uStack_38f8 = 0;
  lStack_3900 = 0;
  func_0x0099c3a4(alStack_71e0,param_6,param_7);
  uVar14 = *(uint *)(&UNK_010cd9a0 + lRam0000000001487f00);
  uVar2 = (uVar14 ^ uVar14 >> 1) & 0x45404400;
  uVar14 = (uVar2 | uVar2 << 1) ^ uVar14;
  if (uVar14 != 0) {
    *(uint *)(&UNK_0003ba90 + lRam0000000001487f08) = uVar14;
  }
  lVar13 = 0x38;
  fVar15 = 1.0;
  uStack_7204 = uStack_71f0;
  uVar5 = 0;
  lVar9 = extraout_x1;
  uRam0000000001487b18 = uVar14;
  do {
    lStack_71e8 = lVar13;
    lStack_3900 = alStack_71e0[uVar5];
    uStack_38f8 = auStack_71a8[uVar5];
    uStack_c0 = 0;
    *puStack_71f8 = 1;
    uStack_38f0 = 0x3fc90fdb;
    *puStack_7200 = 0x100;
    lStack_d0 = lStack_3900;
    uStack_c8 = uStack_38f8;
    func_0x009944c0(auStack_38e0,lVar9,lStack_a8,1,0);
    func_0x009944c0(auStack_7110,lStack_a8,lStack_a0,1,0);
    lVar10 = alStack_71e0[uVar5];
    uVar4 = auStack_71a8[uVar5];
    lVar13 = lVar10;
    if (lVar10 < 0) {
      lVar13 = lVar10 + 1;
    }
    if (1 < (long)uVar4) {
      lVar6 = (uVar4 >> 1) + 1;
      lVar11 = lStack_a8;
      lVar12 = lStack_a0;
      do {
        func_0x009f15b4(lVar12,lVar11,lVar10);
        lVar6 = lVar6 + -1;
        lVar11 = lVar11 + (lVar13 >> 1) * 4;
        lVar12 = lVar12 + lVar10 * 8;
      } while (1 < lVar6);
    }
    uVar4 = uVar5 + 1;
    func_0x00990da8(lStack_a8,lVar9,lStack_a0,alStack_71e0[uVar4],auStack_71a8[uVar4],
                    auStack_71b1[uVar5 + 1],auStack_7179[uVar5 + 1]);
    lVar10 = lStack_a8;
    lVar13 = lStack_71e8;
    if (uVar14 != 0) {
      uVar14 = *(uint *)(lRam0000000001487f10 + 0x33ab08);
      if (uRam0000000001487b1c != 0) {
        uVar14 = uVar14 >> 0xe | uVar14 << 0x12;
      }
      if (uVar14 != 1) {
        *(uint *)(&UNK_005ff2d8 + lRam0000000001487f18) = uVar14;
      }
      uRam0000000001487b1c = uVar14;
      if (lRam0000000001567b30 != 0) {
        uVar14 = uVar14 + (byte)(&UNK_00007001)[lRam0000000001567b30];
        uRam0000000001487b1c = uVar14;
      }
    }
    lStack_a8 = lVar9;
    if (uVar5 < 3 && (uStack_7204 & 1) == 0) {
      lVar6 = lStack_a0;
      uVar7 = uVar5;
      lVar11 = lStack_71e8;
      if (uVar5 != 0) {
        do {
          func_0x00993df0(lVar6,lStack_a8,*(undefined8 *)((long)alStack_7218 + lVar11),
                          *(undefined8 *)((long)alStack_71e0 + lVar11),auStack_71b1[uVar7],
                          auStack_7179[uVar7]);
          lVar6 = lStack_a8;
          lVar9 = lStack_b0;
          lStack_b0 = lStack_a8;
          lStack_a8 = lVar9;
          bVar1 = 1 < (long)uVar7;
          uVar7 = uVar7 - 1;
          lVar11 = lVar11 + -8;
        } while (bVar1);
      }
      func_0x009ee304(0x3f800000,lVar6,alStack_7218[1] + uVar5 * alStack_7218[0] * 4);
    }
    if (uStack_71f0 != 0) {
      if (iStack_71ec != 0) {
        puVar8 = auStack_7140 + uVar5;
        func_0x009ed2f0(lStack_a0,lStack_a8,*puVar8);
        func_0x009944c0(auStack_38e0,lStack_a8,lStack_b0,1,0);
        func_0x009944c0(auStack_7110,lStack_b0,lStack_a8,1,0);
        func_0x009944c0(auStack_38e0,lStack_a8,lStack_b0,1,0);
        func_0x009944c0(auStack_7110,lStack_b0,lStack_a8,1,0);
        func_0x009ed3e0(lStack_a8,lStack_a8,*puVar8);
        func_0x009ed3e0(lStack_a8,lStack_a8,*puVar8);
        func_0x009ee04c(0x3c23d70a,lStack_a0,lStack_a8,lStack_a0,*puVar8);
      }
      lVar9 = lVar13;
      if (uVar5 == 0) {
        func_0x009ee304(0x3f800000,lStack_a0,alStack_7218[1],alStack_7218[0]);
      }
      else {
        do {
          func_0x00993df0(lStack_a0,lStack_a8,*(undefined8 *)((long)alStack_7218 + lVar9),
                          *(undefined8 *)((long)alStack_71e0 + lVar9),auStack_71b1[uVar5],
                          auStack_7179[uVar5]);
          lVar6 = lStack_a8;
          lStack_a8 = lStack_a0;
          lStack_a0 = lVar6;
          bVar1 = 1 < (long)uVar5;
          uVar5 = uVar5 - 1;
          lVar9 = lVar9 + -8;
        } while (bVar1);
        func_0x009ed978(fVar15,0,alStack_7218[1],lVar6,alStack_7218[1],alStack_7218[0]);
      }
    }
    fVar3 = fVar15 * 1.25;
    if (iStack_71ec == 0) {
      fVar3 = fVar15;
    }
    fVar15 = fVar3;
    lVar13 = lVar13 + 8;
    uVar5 = uVar4;
    lVar9 = lVar10;
  } while (uVar4 != 5);
  if (uVar14 != 0) {
    uVar14 = *(uint *)(&UNK_00dd81c8 + lRam0000000001487f20);
    if (iRam0000000001487b20 != 0) {
      uVar14 = uVar14 >> 5 | uVar14 << 0x1b;
    }
    if ((int)(CONCAT44((uVar14 + (uVar14 >> 0x10)) * 0x101 + (uVar14 >> 0x18),uVar14) >> 0x10) != 1)
    {
      *(uint *)(lRam0000000001487f28 + 0x1827398) = uVar14;
    }
  }
  if (0 < lStack_90) {
    lVar13 = 0;
    do {
      func_0x009d8290(uStack_98);
      lVar13 = lVar13 + 1;
    } while (lVar13 < lStack_90);
  }
  return;
}


