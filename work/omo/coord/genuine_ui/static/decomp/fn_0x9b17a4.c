// FUN_009b17a4 size=628

void FUN_009b17a4(long param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4,long param_5
                 ,long param_6,long param_7,undefined4 param_8,undefined1 param_9,long param_10,
                 long param_11,undefined8 param_12,undefined8 param_13)

{
  uint uVar1;
  long lVar2;
  long lVar3;
  undefined8 *puVar4;
  undefined8 *puVar5;
  undefined8 *puVar6;
  long lVar7;
  undefined8 uVar8;
  undefined8 uStack_88;
  undefined8 uStack_80;
  undefined8 uStack_78;
  undefined8 uStack_70;
  long lStack_68;
  
  uStack_70 = param_12;
  lStack_68 = 0;
  lVar2 = param_11 * param_10 * 4;
  uVar8 = 0;
  puVar6 = &uStack_88;
  puVar5 = &uStack_80;
  puVar4 = &uStack_78;
  lVar3 = lVar2;
  lVar7 = lVar2;
  func_0x009c80d4(&uStack_70,0xe848b07e);
  uVar1 = *(uint *)(&UNK_00eeb750 + lRam0000000001488bd8);
  if (iRam00000000014882fc != 0) {
    uVar1 = uVar1 >> 0x1d | uVar1 << 3;
  }
  if (uVar1 != 0) {
    *(uint *)(&UNK_00fc82c0 + lRam0000000001488be0) = uVar1;
  }
  (**(code **)((ulong)*(uint *)(*(long *)(&UNK_00003f90 + param_1) + 0x48) * 8 + 0x147df50))
            (param_2,param_3,param_4,uStack_78,uStack_80,uStack_88,param_11 * param_10);
  uVar1 = *(uint *)(&UNK_002075a0 + lRam0000000001488be8);
  iRam0000000001488300 = *(int *)(lRam0000000001488bf8 + 0x1bdcc88);
  if (iRam0000000001488300 != 0) {
    *(uint *)(lRam0000000001488bf0 + 0x19df2d0) = ((uVar1 ^ uVar1 >> 1) & 0x41555514) * 3 ^ uVar1;
  }
  if (param_5 != 0) {
    func_0x009987ac(param_13,uStack_78,param_5,param_8,param_9,param_10,param_11,param_12,puVar4,
                    lVar2,puVar5,lVar3,puVar6,lVar7,uVar8);
  }
  if (param_6 != 0) {
    func_0x009987ac(param_13,uStack_80,param_6,param_8,param_9,param_10,param_11,param_12,puVar4,
                    lVar2,puVar5,lVar3,puVar6,lVar7,uVar8);
  }
  if (param_7 != 0) {
    func_0x009987ac(param_13,uStack_88,param_7,param_8,param_9,param_10,param_11,param_12,puVar4,
                    lVar2,puVar5,lVar3,puVar6,lVar7,uVar8);
  }
  uVar1 = *(uint *)(&UNK_0083f230 + lRam0000000001488c00);
  if (*(int *)(&UNK_002995c8 + lRam0000000001488c10) != 0) {
    *(uint *)(&UNK_00e0d2e0 + lRam0000000001488c08) =
         ((uVar1 ^ uVar1 >> 1) & 0x41555150) * 3 ^ uVar1;
  }
  if (0 < lStack_68) {
    lVar3 = 0;
    do {
      func_0x009d8290(uStack_70);
      lVar3 = lVar3 + 1;
    } while (lVar3 < lStack_68);
  }
  return;
}


