// FUN_0002342c size=1508

void FUN_0002342c(long param_1)

{
  int iVar1;
  uint uVar2;
  int *piVar3;
  uint *puVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  int *piVar9;
  long lVar10;
  undefined8 uVar11;
  int *piStack_178;
  int *apiStack_170 [5];
  undefined8 uStack_148;
  undefined8 uStack_140;
  int *piStack_138;
  undefined1 auStack_130 [8];
  int *apiStack_128 [20];
  undefined *apuStack_88 [2];
  int *piStack_78;
  uint *puStack_70;
  int *piStack_68;
  uint *puStack_60;
  undefined8 uStack_58;
  
  lVar10 = *(long *)(*(long *)(param_1 + 0x78) + 0x28);
  apiStack_128[0] =
       (int *)((ulong)(uint)(*(int *)(lVar10 + 0x20) - *(int *)(lVar10 + 0x18)) << 0x20);
  uStack_58 = func_0x00755030(*(long *)(param_1 + 0x78),apiStack_128);
  if (param_1 == 0) {
    piVar9 = (int *)0x0;
  }
  else {
    piVar9 = (int *)func_0x002af1fc(param_1);
  }
  uVar2 = *(uint *)(&UNK_00a3e7a8 + lRam000000000141a3e0);
  uVar6 = (uVar2 ^ uVar2 >> 1) & 0x41541105;
  uVar2 = (uVar6 | uVar6 << 1) ^ uVar2;
  iVar8 = (int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10);
  iRam000000000141a3d8 = iVar8;
  puStack_60 = (uint *)func_0x002efa1c(".",1);
  func_0x001cb46c(param_1,&puStack_60);
  piStack_68 = (int *)func_0x002efa1c("*",1);
  if (*(int *)(*(long *)(param_1 + 0x90) + 4) != 0) {
    func_0x002efbd0(apiStack_128,".",1);
    func_0x002e97cc(apiStack_128,(long *)(param_1 + 0x90));
    func_0x002e97cc(&piStack_68,apiStack_128);
    if (*apiStack_128[0] != -1) {
      if (*apiStack_128[0] != 0) {
        LOAcquire();
        iVar5 = *apiStack_128[0];
        *apiStack_128[0] = iVar5 + -1;
        LORelease();
        if (iVar5 != 1) goto LAB_0002357c;
      }
      func_0x00287df8(apiStack_128[0],2,8);
    }
  }
LAB_0002357c:
  puVar4 = puStack_60;
  piVar3 = piStack_68;
  if (1 < *piStack_68 + 1U) {
    LOAcquire();
    *piStack_68 = *piStack_68 + 1;
    LORelease();
  }
  uVar6 = *puStack_60 + 1;
  if (1 < uVar6) {
    LOAcquire();
    uVar6 = *puStack_60;
    *puStack_60 = uVar6 + 1;
    LORelease();
  }
  func_0x003cda5c(uVar6,apuStack_88,0);
  apuStack_88[0] = &UNK_013bdc38;
  if (1 < *piVar3 + 1U) {
    LOAcquire();
    *piVar3 = *piVar3 + 1;
    LORelease();
  }
  puStack_70 = puVar4;
  if (1 < *puVar4 + 1) {
    LOAcquire();
    *puVar4 = *puVar4 + 1;
    LORelease();
  }
  piStack_78 = piVar3;
  if (*puVar4 != 0xffffffff) {
    if (*puVar4 != 0) {
      LOAcquire();
      uVar6 = *puVar4;
      *puVar4 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_00023634;
    }
    func_0x00287df8(puVar4,2,8);
  }
LAB_00023634:
  if (*piVar3 != -1) {
    if (*piVar3 != 0) {
      LOAcquire();
      iVar5 = *piVar3;
      *piVar3 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_00023664;
    }
    func_0x00287df8(piVar3,2,8);
  }
LAB_00023664:
  func_0x0000e1b4(apiStack_128,param_1);
  func_0x000279a0(apuStack_88,apiStack_128,*(undefined1 *)(param_1 + 0xca),
                  *(undefined8 *)(param_1 + 0xd0),param_1 + 0xe0);
  func_0x0074ab38(apiStack_170,apiStack_128);
  iVar5 = apiStack_170[0][2];
  iVar1 = apiStack_170[0][3];
  if (*apiStack_170[0] != -1) {
    if (*apiStack_170[0] != 0) {
      LOAcquire();
      iVar7 = *apiStack_170[0];
      *apiStack_170[0] = iVar7 + -1;
      LORelease();
      if (iVar7 != 1) goto LAB_000236c4;
    }
    func_0x002aa848(apiStack_170[0]);
  }
LAB_000236c4:
  if (iVar1 == iVar5) {
    if (*(int *)(*(long *)(param_1 + 0xd8) + 4) != 0) {
      if (iVar8 != 0) {
        uVar2 = *(uint *)(&UNK_00955678 + lRam000000000141a3e8);
        uVar6 = (uVar2 ^ uVar2 >> 1) & 0x41040011;
        uVar2 = (uVar6 | uVar6 << 1) ^ uVar2;
        if ((int)(CONCAT44((uVar2 + (uVar2 >> 0x10)) * 0x101 + (uVar2 >> 0x18),uVar2) >> 0x10) != 1)
        {
          *(uint *)(&UNK_00ccfac8 + lRam000000000141a3f0) = uVar2;
        }
      }
      func_0x0033db48(apiStack_170,&puStack_60);
      func_0x002f4ef8(&piStack_138,(long *)(param_1 + 0xd8),apiStack_170,0,0x20);
      if (*apiStack_170[0] != -1) {
        if (*apiStack_170[0] != 0) {
          LOAcquire();
          iVar8 = *apiStack_170[0];
          *apiStack_170[0] = iVar8 + -1;
          LORelease();
          if (iVar8 != 1) goto LAB_00023790;
        }
        func_0x00287df8(apiStack_170[0],2,8);
      }
LAB_00023790:
      func_0x00756efc(*(undefined8 *)(param_1 + 0x78),&piStack_138);
      uStack_140 = func_0x00470e94();
      uStack_148 = func_0x00757ebc(*(undefined8 *)(param_1 + 0x78),&uStack_140);
      func_0x004743f8(apiStack_170,0x6e,&uStack_148,&uStack_140);
      apiStack_170[0] = (int *)&UNK_013b5268;
      func_0x003b72a4(*(undefined8 *)(param_1 + 0x78),apiStack_170);
      uVar11 = *(undefined8 *)(param_1 + 0x78);
      piStack_178 = (int *)func_0x002efa1c("",0);
      func_0x00756efc(uVar11,&piStack_178);
      if (*piStack_178 != -1) {
        if (*piStack_178 != 0) {
          LOAcquire();
          iVar8 = *piStack_178;
          *piStack_178 = iVar8 + -1;
          LORelease();
          if (iVar8 != 1) goto LAB_00023838;
        }
        func_0x00287df8(piStack_178,2,8);
      }
LAB_00023838:
      func_0x00474478(apiStack_170);
      if (*piStack_138 != -1) {
        if (*piStack_138 != 0) {
          LOAcquire();
          iVar8 = *piStack_138;
          *piStack_138 = iVar8 + -1;
          LORelease();
          if (iVar8 != 1) goto LAB_000238f0;
        }
        func_0x00287df8(piStack_138,2,8);
      }
    }
  }
  else {
    func_0x0002acb8(apiStack_170,*(undefined8 *)(param_1 + 0x78),apiStack_128);
    func_0x003d0e8c(auStack_130,apuStack_88,"2fileSelected(const QString&)",param_1,
                    "2fileSelected(const QString&)",0);
    func_0x003d24d8(auStack_130);
    func_0x0083fe84(apiStack_128,*(undefined8 *)(param_1 + 0x78));
    func_0x0083b9b8(apiStack_128,&uStack_58,0);
    if (((piVar9 != (int *)0x0) && (param_1 != 0)) && (piVar9[1] != 0)) {
      func_0x007d6020(*(undefined8 *)(param_1 + 0x78),0);
    }
    func_0x003cded4(apiStack_170);
  }
LAB_000238f0:
  func_0x001c63c0(apiStack_128);
  apuStack_88[0] = &UNK_013bdc38;
  if (*puStack_70 != 0xffffffff) {
    if (*puStack_70 != 0) {
      LOAcquire();
      uVar6 = *puStack_70;
      *puStack_70 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_00023934;
    }
    func_0x00287df8(puStack_70,2,8);
  }
LAB_00023934:
  if (*piStack_78 != -1) {
    if (*piStack_78 != 0) {
      LOAcquire();
      iVar8 = *piStack_78;
      *piStack_78 = iVar8 + -1;
      LORelease();
      if (iVar8 != 1) goto LAB_00023968;
    }
    func_0x00287df8(piStack_78,2,8);
  }
LAB_00023968:
  func_0x003cded4(apuStack_88);
  if (*piStack_68 != -1) {
    if (*piStack_68 != 0) {
      LOAcquire();
      iVar8 = *piStack_68;
      *piStack_68 = iVar8 + -1;
      LORelease();
      if (iVar8 != 1) goto LAB_000239a4;
    }
    func_0x00287df8(piStack_68,2,8);
  }
LAB_000239a4:
  if (*puStack_60 != 0xffffffff) {
    if (*puStack_60 != 0) {
      LOAcquire();
      uVar6 = *puStack_60;
      *puStack_60 = uVar6 - 1;
      LORelease();
      if (uVar6 != 1) goto LAB_000239d8;
    }
    func_0x00287df8(puStack_60,2,8);
  }
LAB_000239d8:
  if (piVar9 != (int *)0x0) {
    LOAcquire();
    iVar8 = *piVar9;
    *piVar9 = iVar8 + -1;
    LORelease();
    if (iVar8 == 1) {
      __ZdlPv(piVar9);
    }
  }
  return;
}


