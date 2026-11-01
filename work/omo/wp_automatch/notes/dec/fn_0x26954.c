// FUN_00026954 size=2356

undefined8
FUN_00026954(long param_1,undefined8 param_2,undefined8 param_3,uint param_4,ulong param_5,
            undefined8 param_6,int *param_7,long *param_8)

{
  char *pcVar1;
  bool bVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  undefined4 *puVar6;
  ulong uVar7;
  undefined8 uVar8;
  long lVar9;
  uint *puVar10;
  int *piVar11;
  undefined8 uVar12;
  long lVar13;
  int *piStack_c8;
  undefined1 auStack_c0 [8];
  int *piStack_b8;
  int iStack_ac;
  uint *puStack_a8;
  undefined8 uStack_a0;
  undefined1 auStack_98 [8];
  undefined1 auStack_90 [8];
  uint *puStack_88;
  uint *puStack_80;
  undefined8 uStack_78;
  int *piStack_70;
  undefined8 uStack_68;
  
  if ((param_5 & 1) == 0) {
    func_0x00011c6c(param_3);
    func_0x0000e1b8(param_3,0,0);
    piStack_70 = (int *)func_0x002efa1c("",0);
    func_0x0000e4f4(param_3,0,0,&piStack_70,0);
    if (*piStack_70 != -1) {
      if (*piStack_70 != 0) {
        LOAcquire();
        iVar3 = *piStack_70;
        *piStack_70 = iVar3 + -1;
        LORelease();
        if (iVar3 != 1) goto LAB_00026a0c;
      }
      func_0x00287df8(piStack_70,2,8);
    }
LAB_00026a0c:
    piStack_70 = (int *)&LAB_008397c8;
    uStack_68 = 0;
    puStack_80 = (uint *)&DAT_000275d4;
    uStack_78 = 0;
    puVar6 = (undefined4 *)__Znwm(0x20);
    *puVar6 = 1;
    *(undefined **)(puVar6 + 2) = &DAT_00027abc;
    *(undefined **)(puVar6 + 4) = &DAT_000275d4;
    *(undefined8 *)(puVar6 + 6) = 0;
    func_0x003d6598(auStack_90,param_3,&piStack_70,param_1,&puStack_80,puVar6,0,0,&UNK_013ee670);
    func_0x003d24d8(auStack_90);
  }
  func_0x0033cec8(auStack_98,param_2);
  uVar7 = func_0x00340220(auStack_98);
  if (((uVar7 & 1) == 0) || (iVar3 = func_0x00159f20(param_2,1,1,1), iVar3 != 0)) {
    uVar12 = 0;
    goto LAB_00026aac;
  }
  func_0x0033e5dc(auStack_98,&UNK_00006402);
  uStack_a0 = 0x142f0e0;
  func_0x001c25a8(&uStack_a0,param_1 + 0x10);
  func_0x0033e298(auStack_98,&uStack_a0);
  uVar4 = param_7 != (int *)0x0 & param_4;
  if (uVar4 == 1) {
    func_0x0033e6b8(auStack_98,0xffffffff);
  }
  else {
    func_0x0033e6b8(auStack_98,0x14);
  }
  func_0x0033ee8c(&puStack_a8,auStack_98,0xffffffff,0xffffffff);
  if (uVar4 != 0) {
    if (1 < *puStack_a8) {
      func_0x00027b38(&puStack_a8,puStack_a8[1]);
    }
    puVar10 = puStack_a8;
    uVar4 = puStack_a8[2];
    if (1 < *puStack_a8) {
      func_0x00027b38(&puStack_a8,puStack_a8[1]);
    }
    puStack_80 = puVar10 + (long)(int)uVar4 * 2 + 4;
    puStack_88 = puStack_a8 + (long)(int)puStack_a8[3] * 2 + 4;
    piStack_70 = param_7;
    func_0x00027ca4(&puStack_80,&puStack_88,&piStack_70);
  }
  puStack_88 = (uint *)0x142f0e0;
  if ((*(int *)(*param_8 + 4) == 0) || (puStack_a8[3] == puStack_a8[2])) {
    iVar3 = 1;
  }
  else {
    param_6 = 1;
    func_0x0000e4f4(param_3,0,1,param_8,0);
    iVar3 = 2;
  }
  iStack_ac = 0;
  uVar4 = puStack_a8[2];
  if ((int)uVar4 < (int)puStack_a8[3]) {
    do {
      func_0x00349548(&piStack_b8,puStack_a8 + (long)(int)uVar4 * 2 + (long)iStack_ac * 2 + 4);
      iVar5 = func_0x0034ac60(&piStack_b8);
      if (iVar5 == 0) {
        uVar12 = __Znwm(0x10);
        func_0x0034a53c(&piStack_70,&piStack_b8);
        func_0x0071bf08(uVar12,&piStack_70,param_3);
        if (*piStack_70 != -1) {
          if (*piStack_70 != 0) {
            LOAcquire();
            iVar5 = *piStack_70;
            *piStack_70 = iVar5 + -1;
            LORelease();
            if (iVar5 != 1) goto LAB_00026e54;
          }
          func_0x00287df8(piStack_70,2,8);
        }
LAB_00026e54:
        func_0x00349a94(&puStack_80,&piStack_b8);
        func_0x003df0c8(&piStack_70,&puStack_80);
        func_0x0071d560(uVar12,&piStack_70);
        func_0x003dec3c(&piStack_70);
        if (*puStack_80 != 0xffffffff) {
          if (*puStack_80 != 0) {
            LOAcquire();
            uVar4 = *puStack_80;
            *puStack_80 = uVar4 - 1;
            LORelease();
            if (uVar4 != 1) goto LAB_00026eb0;
          }
          func_0x00287df8(puStack_80,2,8);
        }
LAB_00026eb0:
        func_0x0000f43c(param_3,0,uVar12,0,0,0xffffffff,param_6);
        piStack_70 = (int *)&DAT_0071d674;
        uStack_68 = 0;
        puStack_80 = (uint *)&DAT_00027858;
        uStack_78 = 0;
        puVar6 = (undefined4 *)__Znwm(0x20);
        *puVar6 = 1;
        *(undefined **)(puVar6 + 2) = &DAT_00028a48;
        *(undefined **)(puVar6 + 4) = &DAT_00027858;
        *(undefined8 *)(puVar6 + 6) = 0;
        func_0x003d6598(auStack_c0,uVar12,&piStack_70,param_1,&puStack_80,puVar6,0,0,&UNK_013e3320);
        func_0x003d24d8(auStack_c0);
      }
      else if (param_4 == 0) {
        uVar12 = __Znwm(0x10);
        func_0x0034a44c(&piStack_70,&piStack_b8);
        func_0x0071bf08(uVar12,&piStack_70,param_3);
        if (*piStack_70 != -1) {
          if (*piStack_70 != 0) {
            LOAcquire();
            iVar5 = *piStack_70;
            *piStack_70 = iVar5 + -1;
            LORelease();
            if (iVar5 != 1) goto LAB_00026ce8;
          }
          func_0x00287df8(piStack_70,2,8);
        }
LAB_00026ce8:
        func_0x00349a94(&puStack_80,&piStack_b8);
        func_0x003df0c8(&piStack_70,&puStack_80);
        func_0x0071d560(uVar12,&piStack_70);
        func_0x003dec3c(&piStack_70);
        if (*puStack_80 != 0xffffffff) {
          if (*puStack_80 != 0) {
            LOAcquire();
            uVar4 = *puStack_80;
            *puStack_80 = uVar4 - 1;
            LORelease();
            if (uVar4 != 1) goto LAB_00026d44;
          }
          func_0x00287df8(puStack_80,2,8);
        }
LAB_00026d44:
        func_0x00349a94(&puStack_80,&piStack_b8);
        func_0x0033cec8(&piStack_70,&puStack_80);
        if (*puStack_80 != 0xffffffff) {
          if (*puStack_80 != 0) {
            LOAcquire();
            uVar4 = *puStack_80;
            *puStack_80 = uVar4 - 1;
            LORelease();
            if (uVar4 != 1) goto LAB_00026d8c;
          }
          func_0x00287df8(puStack_80,2,8);
        }
LAB_00026d8c:
        func_0x0033e5dc(&piStack_70,&UNK_00006402);
        func_0x0033e298(&piStack_70,&uStack_a0);
        iVar5 = func_0x0033e794(&piStack_70);
        if (iVar5 != 0) {
          uVar8 = __Znwm(0xa0);
          func_0x0000e1b4(uVar8,param_3);
          func_0x0071ca24(uVar12,uVar8);
        }
        func_0x0000f43c(param_3,0,uVar12,0,0,0xffffffff,param_6);
        func_0x0033cee4(&piStack_70);
      }
      else {
        func_0x001820e0(&puStack_88,&iStack_ac);
      }
      func_0x00349560(&piStack_b8);
      iStack_ac = iStack_ac + 1;
      uVar4 = puStack_a8[2];
    } while (iStack_ac < (int)(puStack_a8[3] - uVar4));
  }
  uVar4 = puStack_88[2];
  if (puStack_88[3] != uVar4) {
    do {
      puVar10 = puStack_88;
      if (1 < *puStack_88) {
        piVar11 = (int *)func_0x002aa738(&puStack_88,puStack_88[1]);
        if ((puVar10 + (long)(int)uVar4 * 2 + 4 != puStack_88 + (long)(int)puStack_88[2] * 2 + 4) &&
           (0 < (long)puStack_88 +
                ((long)(int)puStack_88[3] * 8 -
                (long)(puStack_88 + (long)(int)puStack_88[2] * 2 + 4)) + 0x10)) {
          _memcpy();
        }
        if (*piVar11 != -1) {
          if (*piVar11 != 0) {
            LOAcquire();
            iVar5 = *piVar11;
            *piVar11 = iVar5 + -1;
            LORelease();
            if (iVar5 != 1) goto LAB_000270a0;
          }
          func_0x002aa848(piVar11);
        }
      }
LAB_000270a0:
      func_0x00349548(&piStack_70,
                      puStack_a8 +
                      (long)(int)puStack_a8[2] * 2 +
                      (long)(int)puStack_88[(long)(int)puStack_88[2] * 2 + 4] * 2 + 4);
      func_0x008eb498(&puStack_88);
      iVar5 = *(int *)(*param_8 + 4);
      func_0x0034a44c(&piStack_b8,&piStack_70);
      bVar2 = iVar5 != 0;
      pcVar1 = "";
      if (bVar2) {
        pcVar1 = "  ";
      }
      func_0x002efbd0(&puStack_80,pcVar1,(ulong)bVar2 << 1);
      func_0x002e97cc(&puStack_80,&piStack_b8);
      func_0x0000e4f4(param_3,0,iVar3,&puStack_80,0);
      if (*puStack_80 != 0xffffffff) {
        if (*puStack_80 != 0) {
          LOAcquire();
          uVar4 = *puStack_80;
          *puStack_80 = uVar4 - 1;
          LORelease();
          if (uVar4 != 1) goto LAB_00027154;
        }
        func_0x00287df8(puStack_80,2,8);
      }
LAB_00027154:
      if (*piStack_b8 != -1) {
        if (*piStack_b8 != 0) {
          LOAcquire();
          iVar5 = *piStack_b8;
          *piStack_b8 = iVar5 + -1;
          LORelease();
          if (iVar5 != 1) goto LAB_00027184;
        }
        func_0x00287df8(piStack_b8,2,8);
      }
LAB_00027184:
      func_0x00349a94(&piStack_c8,&piStack_70);
      puStack_80 = (uint *)func_0x002efa1c("",0);
      uVar7 = FUN_00026954(param_1,&piStack_c8,param_3,0,1,iVar3,0,&puStack_80);
      if (*puStack_80 != 0xffffffff) {
        if (*puStack_80 != 0) {
          LOAcquire();
          uVar4 = *puStack_80;
          *puStack_80 = uVar4 - 1;
          LORelease();
          if (uVar4 != 1) goto LAB_000271f8;
        }
        func_0x00287df8(puStack_80,2,8);
      }
LAB_000271f8:
      if (*piStack_c8 != -1) {
        if (*piStack_c8 != 0) {
          LOAcquire();
          iVar5 = *piStack_c8;
          *piStack_c8 = iVar5 + -1;
          LORelease();
          if (iVar5 != 1) goto LAB_00027228;
        }
        func_0x00287df8(piStack_c8,2,8);
      }
LAB_00027228:
      if ((uVar7 & 1) == 0) {
        func_0x00349a94(&puStack_80,&piStack_70);
        uVar12 = func_0x00026858(&puStack_80);
        func_0x0000f43c(param_3,0,uVar12,0,0,0xffffffff,iVar3);
        if (*puStack_80 != 0xffffffff) {
          if (*puStack_80 != 0) {
            LOAcquire();
            uVar4 = *puStack_80;
            *puStack_80 = uVar4 - 1;
            LORelease();
            if (uVar4 != 1) goto LAB_00027008;
          }
          func_0x00287df8(puStack_80,2,8);
        }
      }
LAB_00027008:
      iVar3 = iVar3 + 1;
      func_0x00349560(&piStack_70);
      uVar4 = puStack_88[2];
    } while (puStack_88[3] != uVar4);
  }
  if (*puStack_88 != 0xffffffff) {
    if (*puStack_88 != 0) {
      LOAcquire();
      uVar4 = *puStack_88;
      *puStack_88 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_00026f74;
    }
    func_0x002aa848(puStack_88);
  }
LAB_00026f74:
  if (*puStack_a8 != 0xffffffff) {
    if (*puStack_a8 != 0) {
      LOAcquire();
      uVar4 = *puStack_a8;
      *puStack_a8 = uVar4 - 1;
      LORelease();
      if (uVar4 != 1) goto LAB_00026fd4;
    }
    uVar4 = puStack_a8[3];
    if (uVar4 != puStack_a8[2]) {
      lVar13 = (long)(int)puStack_a8[2] * 8 + (long)(int)uVar4 * -8;
      puVar10 = puStack_a8 + (long)(int)uVar4 * 2 + 2;
      do {
        lVar9 = func_0x00349560(puVar10);
        puVar10 = (uint *)(lVar9 + -8);
        lVar13 = lVar13 + 8;
      } while (lVar13 != 0);
    }
    func_0x002aa848(puStack_a8);
  }
LAB_00026fd4:
  func_0x001c4af0(&uStack_a0);
  uVar12 = 1;
LAB_00026aac:
  func_0x0033cee4(auStack_98);
  return uVar12;
}


