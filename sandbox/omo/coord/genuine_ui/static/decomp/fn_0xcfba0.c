// FUN_000cfba0 size=1032

undefined8 FUN_000cfba0(long param_1,undefined8 param_2,undefined8 param_3)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  long lVar4;
  ulong uVar5;
  float fVar6;
  int *piStack_48;
  int *piStack_40;
  int *piStack_38;
  int *piStack_30;
  int *piStack_28;
  
  fVar6 = (float)func_0x009b0850(param_2,param_3);
  uVar1 = uRam000000000141bd6e;
  lVar4 = (long)fVar6;
  if (uRam000000000141bd68 >> 0x1d == 0) {
    uVar5 = (ulong)uRam000000000141bd6e;
    *(undefined8 *)(lRam000000000141bd80 + 0x3253d78) = 0x141bd6c;
    if (*(long *)(&UNK_00167580 + lRam000000000141bd78) != 0) {
      uVar5 = -(ulong)*(byte *)(*(long *)(&UNK_00167580 + lRam000000000141bd78) + (uVar5 & 0xffff));
    }
    lVar4 = lVar4 + ((ulong)(uVar1 >> 0x18) + (uVar5 + (uVar1 >> 0x10 & 0xff)) * 2) * -4;
  }
  uRam000000000141bd68 =
       (int)(CONCAT44((uRam000000000141bd68 + (uRam000000000141bd68 >> 0x10)) * 0x101 +
                      (uRam000000000141bd68 >> 0x18),uRam000000000141bd68) >> 0x10) + (int)param_2;
  if (1 < lVar4) {
    if (3 < lVar4) {
      return 1;
    }
    if ((*(byte *)(lRam000000000155b458 + 0x180) & 1) == 0) {
      return 1;
    }
    piStack_38 = (int *)func_0x002efa1c("The selected area is small.\n\nUsing a larger area may produce a more accurate noise profile.\nThe recommended size is %1x%2 (or more) pixels.\n\nYou can:\n1) Manually select a larger area, if possible\n2) Continue building profile using the currently-selected area\n3) Try to create a Generic profile"
                                        ,0x125);
    func_0x002f58f8(&piStack_30,&piStack_38,uRam0000000001488140,0,10,0x20);
    func_0x002f58f8(&piStack_28,&piStack_30,uRam0000000001488140,0,10,0x20);
    piStack_40 = (int *)func_0x002efa1c("Use the small area",0x12);
    piStack_48 = (int *)func_0x002efa1c("Cancel",6);
    iVar2 = func_0x00084568(&piStack_28,&piStack_40,&piStack_48);
    if (*piStack_48 != -1) {
      if (*piStack_48 != 0) {
        LOAcquire();
        iVar3 = *piStack_48;
        *piStack_48 = iVar3 + -1;
        LORelease();
        if (iVar3 != 1) goto LAB_000cfe94;
      }
      func_0x00287df8(piStack_48,2,8);
    }
LAB_000cfe94:
    if (*piStack_40 != -1) {
      if (*piStack_40 != 0) {
        LOAcquire();
        iVar3 = *piStack_40;
        *piStack_40 = iVar3 + -1;
        LORelease();
        if (iVar3 != 1) goto LAB_000cfec8;
      }
      func_0x00287df8(piStack_40,2,8);
    }
LAB_000cfec8:
    if (*piStack_28 != -1) {
      if (*piStack_28 != 0) {
        LOAcquire();
        iVar3 = *piStack_28;
        *piStack_28 = iVar3 + -1;
        LORelease();
        if (iVar3 != 1) goto LAB_000cfefc;
      }
      func_0x00287df8(piStack_28,2,8);
    }
LAB_000cfefc:
    if (*piStack_30 != -1) {
      if (*piStack_30 != 0) {
        LOAcquire();
        iVar3 = *piStack_30;
        *piStack_30 = iVar3 + -1;
        LORelease();
        if (iVar3 != 1) goto LAB_000cff30;
      }
      func_0x00287df8(piStack_30,2,8);
    }
LAB_000cff30:
    if (*piStack_38 != -1) {
      if (*piStack_38 != 0) {
        LOAcquire();
        iVar3 = *piStack_38;
        *piStack_38 = iVar3 + -1;
        LORelease();
        if (iVar3 != 1) goto LAB_000cff64;
      }
      func_0x00287df8(piStack_38,2,8);
    }
LAB_000cff64:
    if (iVar2 != 0x10000) {
      return 1;
    }
    piStack_28 = (int *)0x142f030;
    (**(code **)(**(long **)(param_1 + 0x1d0) + 0x1b8))
              (*(long **)(param_1 + 0x1d0),&piStack_28,0xffffffffffffffff);
    goto LAB_000cfd78;
  }
  piStack_38 = (int *)func_0x002efa1c("Too small area to build a noise profile.\n\nThe selected area should be at least %1x%2 pixels large."
                                      ,0x62);
  func_0x002f58f8(&piStack_30,&piStack_38,uRam0000000001488130,0,10,0x20);
  func_0x002f58f8(&piStack_28,&piStack_30,uRam0000000001488130,0,10,0x20);
  func_0x0008434c(&piStack_28);
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar2 = *piStack_28;
      *piStack_28 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000cfcec;
    }
    func_0x00287df8(piStack_28,2,8);
  }
LAB_000cfcec:
  if (*piStack_30 != -1) {
    if (*piStack_30 != 0) {
      LOAcquire();
      iVar2 = *piStack_30;
      *piStack_30 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000cfd20;
    }
    func_0x00287df8(piStack_30,2,8);
  }
LAB_000cfd20:
  if (*piStack_38 != -1) {
    if (*piStack_38 != 0) {
      LOAcquire();
      iVar2 = *piStack_38;
      *piStack_38 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_000cfd54;
    }
    func_0x00287df8(piStack_38,2,8);
  }
LAB_000cfd54:
  piStack_28 = (int *)0x142f030;
  (**(code **)(**(long **)(param_1 + 0x1d0) + 0x1b8))
            (*(long **)(param_1 + 0x1d0),&piStack_28,0xffffffffffffffff);
LAB_000cfd78:
  if (*piStack_28 != -1) {
    if (*piStack_28 != 0) {
      LOAcquire();
      iVar2 = *piStack_28;
      *piStack_28 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) {
        return 0;
      }
    }
    func_0x00287df8(piStack_28,2,8);
  }
  return 0;
}


