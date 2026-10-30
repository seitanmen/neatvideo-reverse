// FUN_000d5a4c size=736

void FUN_000d5a4c(long param_1)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  int *piVar4;
  int iVar5;
  long *plVar6;
  long lVar7;
  float fVar8;
  undefined1 auVar9 [16];
  long lStack_60;
  int *piStack_58;
  undefined4 uStack_50;
  undefined4 uStack_4c;
  int *piStack_48;
  
  func_0x009ec2d8(param_1 + 0x200);
  auVar9 = (**(code **)(**(long **)(param_1 + 0x1f0) + 0x280))();
  iVar5 = (auVar9._8_4_ - auVar9._0_4_) + 1;
  iVar1 = (auVar9._12_4_ - auVar9._4_4_) + 1;
  fVar8 = (float)func_0x009b0850((long)iVar5,(long)iVar1);
  if (0 < (int)fVar8) {
    plVar6 = *(long **)(*(long *)(param_1 + 0x1d0) + 0x108);
    *(undefined4 *)(&UNK_0000bf40 + *(long *)(plVar6[0x6a] + 0x28)) = 0;
    (**(code **)(*plVar6 + 0x70))();
    func_0x003d0e8c(&piStack_48,plVar6[0xa3],"2finished()",plVar6,"1FinishFineTune()",0);
    func_0x003d24d8(&piStack_48);
    uVar3 = *(uint *)(lRam000000000141c510 + 0x18da340);
    uVar2 = (uVar3 ^ uVar3 >> 1) & 0x45011544;
    uVar3 = (uVar2 | uVar2 << 1) ^ uVar3;
    if (uVar3 != 0) {
      *(uint *)(lRam000000000141c518 + 0x19a8370) = uVar3;
    }
    lVar7 = __Znwm(0x108);
    func_0x001c1114();
    plVar6[3] = lVar7;
    func_0x009c7318(plVar6[plVar6[0x1a] + 0xb],(long)auVar9._0_4_,(long)auVar9._4_4_,(long)iVar5,
                    (long)iVar1,lVar7,*(undefined4 *)(plVar6[plVar6[0x1a] + 0xb] + 0xf0),1);
    func_0x0014fe84(plVar6[0xa3],plVar6[3]);
    lStack_60 = plVar6[0xa3];
    piStack_58 = (int *)0x0;
    uStack_50 = 0;
    func_0x001de780(plVar6,plVar6,&lStack_60);
    return;
  }
  piStack_58 = (int *)func_0x002efa1c("Too small area for fine-tuning.\n\nThe selected area should be at least %1x%2 pixels large."
                                      ,0x59);
  func_0x002f58f8(&uStack_50,&piStack_58,uRam0000000001488128,0,10,0x20);
  func_0x002f58f8(&piStack_48,&uStack_50,uRam0000000001488128,0,10,0x20);
  func_0x0008434c(&piStack_48);
  if (*piStack_48 != -1) {
    if (*piStack_48 != 0) {
      LOAcquire();
      iVar5 = *piStack_48;
      *piStack_48 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000d5b74;
    }
    func_0x00287df8(piStack_48,2,8);
  }
LAB_000d5b74:
  piVar4 = (int *)CONCAT44(uStack_4c,uStack_50);
  if (*piVar4 != -1) {
    if (*piVar4 != 0) {
      LOAcquire();
      iVar5 = *piVar4;
      *piVar4 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_000d5ba8;
    }
    func_0x00287df8(CONCAT44(uStack_4c,uStack_50),2,8);
  }
LAB_000d5ba8:
  if (*piStack_58 != -1) {
    if (*piStack_58 != 0) {
      LOAcquire();
      iVar5 = *piStack_58;
      *piStack_58 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) {
        return;
      }
    }
    func_0x00287df8(piStack_58,2,8);
  }
  return;
}


