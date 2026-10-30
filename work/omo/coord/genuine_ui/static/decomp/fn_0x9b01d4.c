// FUN_009b01d4 size=1632

void FUN_009b01d4(undefined8 param_1,undefined8 param_2,undefined1 *param_3,undefined1 *param_4,
                 undefined1 *param_5,float *param_6,float *param_7)

{
  long lVar1;
  long lVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  undefined8 uVar6;
  undefined8 uVar7;
  undefined8 uVar8;
  long lVar9;
  long lVar10;
  long lVar11;
  ulong uVar12;
  long lVar13;
  float fVar14;
  float fVar15;
  undefined1 auVar16 [16];
  undefined1 auStack_5ed8 [80];
  float fStack_5e88;
  float fStack_5e84;
  float fStack_5e80;
  float fStack_5e7c;
  float fStack_5e78;
  float fStack_3f30;
  float fStack_3f2c;
  float fStack_3f28;
  float fStack_3f24;
  float fStack_3f20;
  float fStack_1fd8;
  float fStack_1fd4;
  float fStack_1fd0;
  float fStack_1fcc;
  float fStack_1fc8;
  undefined8 auStack_80 [2];
  
  auVar16 = (*(code *)PTR____chkstk_darwin_013b4308)();
  lVar9 = auVar16._8_8_;
  lVar1 = *(long *)(lVar9 + 0xd0);
  lVar2 = *(long *)(lVar9 + 0xd8);
  if (param_3 != (undefined1 *)0x0) {
    *param_3 = 1;
  }
  if (param_4 != (undefined1 *)0x0) {
    *param_4 = 1;
  }
  if (param_5 != (undefined1 *)0x0) {
    *param_5 = 1;
  }
  if (param_6 != (float *)0x0) {
    *param_6 = 0.0;
  }
  if (param_7 != (float *)0x0) {
    param_7[0] = 0.0;
    param_7[1] = 0.0;
    param_7[2] = 0.0;
    param_7[3] = 0.0;
    param_7[4] = 0.0;
  }
  auStack_80[0] = 0;
  lVar13 = lVar2 * lVar1 * 4;
  uVar6 = func_0x009d7200(lVar13,0x3f914fb1);
  uVar7 = func_0x009d7200(lVar13,0x93f8a91c);
  uVar8 = func_0x009d7200(lVar13,0x54de9d0);
  func_0x009c8c24(lVar9,uVar6,uVar7,uVar8,1);
  uVar3 = *(uint *)(&UNK_001039e0 + lRam0000000001489458);
  uVar4 = *(uint *)(&UNK_00031988 + lRam0000000001489468);
  iVar5 = (int)(CONCAT44((uVar4 + (uVar4 >> 0x10)) * 0x101 + (uVar4 >> 0x18),uVar4) >> 0x10);
  if (iVar5 != 0) {
    if (iRam0000000001488474 != 0) {
      uVar3 = uVar3 >> 0x1c | uVar3 << 4;
    }
    *(uint *)(&UNK_0085b510 + lRam0000000001489460) = uVar3;
  }
  lVar9 = lVar2;
  if (lVar1 <= lVar2) {
    lVar9 = lVar1;
  }
  iRam0000000001488474 = iVar5;
  if (lVar9 < lRam0000000001488128) goto LAB_009b07f4;
  if (lVar9 < lRam0000000001488130) {
    lVar10 = 1;
    lVar13 = lRam0000000001488130;
LAB_009b03e0:
    lVar11 = *(long *)(lVar10 * 8 + 0x1488120);
    if ((float)(lVar9 - lVar11) / (float)(lVar13 - lVar11) + (float)lVar10 < 1.0) goto LAB_009b07f4;
  }
  else {
    if (lVar9 < lRam0000000001488138) {
      lVar10 = 2;
      lVar13 = lRam0000000001488138;
      goto LAB_009b03e0;
    }
    if (lVar9 < lRam0000000001488140) {
      lVar10 = 3;
      lVar13 = lRam0000000001488140;
      goto LAB_009b03e0;
    }
    if (lVar9 < lRam0000000001488148) {
      lVar10 = 4;
      lVar13 = lRam0000000001488148;
      goto LAB_009b03e0;
    }
  }
  func_0x0099ce14(&fStack_1fd8);
  func_0x0099ce14(&fStack_3f30);
  func_0x0099ce14(&fStack_5e88);
  func_0x009d7d58(auStack_5ed8);
  if (param_6 != (float *)0x0 || param_7 != (float *)0x0) {
    func_0x00995250(auStack_80,uVar6,lVar1,lVar2,&fStack_1fd8,5,0,auStack_5ed8);
    func_0x00995250(auStack_80,uVar7,lVar1,lVar2,&fStack_3f30,5,0,auStack_5ed8);
    func_0x00995250(auStack_80,uVar8,lVar1,lVar2,&fStack_5e88,5,0,auStack_5ed8);
    fVar15 = fStack_1fd8 * fStack_1fd8 + fStack_3f30 * fStack_3f30 + fStack_5e88 * fStack_5e88;
    fVar14 = *param_6 + fVar15;
    if (param_7 == (float *)0x0) {
      *param_6 = fVar14 + fStack_1fd4 * fStack_1fd4 + fStack_3f2c * fStack_3f2c +
                          fStack_5e84 * fStack_5e84 +
                 fStack_1fd0 * fStack_1fd0 + fStack_3f28 * fStack_3f28 + fStack_5e80 * fStack_5e80 +
                 fStack_1fcc * fStack_1fcc + fStack_3f24 * fStack_3f24 + fStack_5e7c * fStack_5e7c +
                 fStack_1fc8 * fStack_1fc8 + fStack_3f20 * fStack_3f20 + fStack_5e78 * fStack_5e78;
    }
    else {
      *param_6 = fVar14;
      *param_7 = SQRT(fVar15);
      fVar14 = fStack_1fd4 * fStack_1fd4 + fStack_3f2c * fStack_3f2c + fStack_5e84 * fStack_5e84;
      *param_6 = *param_6 + fVar14;
      param_7[1] = SQRT(fVar14);
      fVar14 = fStack_1fd0 * fStack_1fd0 + fStack_3f28 * fStack_3f28 + fStack_5e80 * fStack_5e80;
      *param_6 = *param_6 + fVar14;
      param_7[2] = SQRT(fVar14);
      fVar14 = fStack_1fcc * fStack_1fcc + fStack_3f24 * fStack_3f24 + fStack_5e7c * fStack_5e7c;
      *param_6 = *param_6 + fVar14;
      param_7[3] = SQRT(fVar14);
      fVar14 = fStack_1fc8 * fStack_1fc8 + fStack_3f20 * fStack_3f20 + fStack_5e78 * fStack_5e78;
      *param_6 = *param_6 + fVar14;
      param_7[4] = SQRT(fVar14);
    }
    *param_6 = SQRT(*param_6);
  }
  if (((param_3 != (undefined1 *)0x0) || (param_4 != (undefined1 *)0x0)) ||
     (param_5 != (undefined1 *)0x0)) {
    (**(code **)((ulong)*(uint *)(*(long *)(&UNK_00003f90 + auVar16._0_8_) + 0x48) * 8 + 0x147df50))
              (uVar6,uVar7,uVar8,uVar6,uVar7,uVar8,lVar2 * lVar1);
    uVar12 = (ulong)*(uint *)(*(long *)(&UNK_00003f90 + auVar16._0_8_) + 0x48);
    func_0x009980e8(auStack_80,uVar6,lVar1,lVar2,&fStack_1fd8);
    func_0x009980e8(auStack_80,uVar7,lVar1,lVar2,&fStack_3f30);
    func_0x009980e8(auStack_80,uVar8,lVar1,lVar2,&fStack_5e88);
    if (param_3 != (undefined1 *)0x0) {
      fVar14 = (float)func_0x009b096c(&fStack_1fd8,*(undefined1 *)(uVar12 * 3 + 0x147dfc8));
      *param_3 = fVar14 < 2.0;
    }
    if (param_4 != (undefined1 *)0x0) {
      fVar14 = (float)func_0x009b096c(&fStack_3f30,*(undefined1 *)(uVar12 * 3 + 0x147dfc9));
      *param_4 = fVar14 < 2.0;
    }
    if (param_5 != (undefined1 *)0x0) {
      fVar14 = (float)func_0x009b096c(&fStack_5e88,*(undefined1 *)(uVar12 * 3 + 0x147dfca));
      *param_5 = fVar14 < 2.0;
    }
  }
  uVar3 = *(uint *)(&UNK_00ffc440 + lRam0000000001488b50);
  iRam00000000014882e8 = *(int *)(lRam0000000001488b60 + 0x144fca8);
  if (iRam00000000014882e8 != 0) {
    uVar4 = (uVar3 ^ uVar3 >> 1) & 0x41140450;
    *(uint *)(lRam0000000001488b58 + 0x146a030) = (uVar4 | uVar4 << 1) ^ uVar3;
  }
  func_0x009d8564(auStack_5ed8);
LAB_009b07f4:
  func_0x009d7910(uVar6);
  func_0x009d7910(uVar7);
  func_0x009d7910(uVar8);
  return;
}


