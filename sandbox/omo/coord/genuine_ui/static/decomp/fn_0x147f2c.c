// FUN_00147f2c size=388

undefined8 FUN_00147f2c(long param_1,long param_2,undefined *param_3,undefined4 param_4,int param_5)

{
  long *plVar1;
  uint uVar2;
  uint uVar3;
  undefined4 uVar4;
  long lVar5;
  undefined *puVar6;
  undefined *puVar7;
  undefined *puVar8;
  undefined8 uVar9;
  undefined1 auStack_58 [8];
  
  plVar1 = (long *)(&UNK_0000d850 + param_1);
  if (*plVar1 != 0) {
    return 0;
  }
  func_0x009c45a0(param_3,*(undefined8 *)(param_2 + 0xd0),*(undefined8 *)(param_2 + 0xd8),1,0,0,0);
  lVar5 = __Znwm(400);
  func_0x0014e3cc(lVar5,param_1 + 0x10,0x155b878,0,0);
  *plVar1 = lVar5;
  uVar4 = (**(code **)(*plRam000000000155b458 + 0x40))();
  lVar5 = *plVar1;
  *(undefined4 *)(lVar5 + 0x180) = uVar4;
  uVar3 = *(uint *)(&UNK_01125120 + lRam000000000141c190);
  uVar2 = (uVar3 ^ uVar3 >> 1) & 0x41505414;
  uVar3 = (uVar2 | uVar2 << 1) ^ uVar3;
  iRam000000000141c188 =
       (int)(CONCAT44((uVar3 + (uVar3 >> 0x10)) * 0x101 + (uVar3 >> 0x18),uVar3) >> 0x10);
  if (iRam000000000141c188 != 0) {
    *(uint *)(&UNK_0006b250 + lRam000000000141c198) = uVar3;
  }
  func_0x003d0e8c(auStack_58,lVar5,"2finished()",param_1,"1threadFinished()",0);
  func_0x003d24d8(auStack_58);
  switch(param_4) {
  case 1:
    lVar5 = *plVar1;
    puVar7 = (undefined *)0x0;
    puVar6 = param_3;
    break;
  case 2:
    lVar5 = *plVar1;
    puVar6 = (undefined *)0x0;
    puVar7 = param_3;
    break;
  case 3:
    lVar5 = *plVar1;
    puVar6 = (undefined *)0x0;
    puVar7 = (undefined *)0x0;
    uVar9 = 0;
    puVar8 = param_3;
    goto code_r0x001481e0;
  case 4:
    puVar6 = &UNK_0000d538 + param_1;
    FUN_009c3de4(puVar6,*(undefined8 *)(param_3 + 0xb0),0,0,0,*(undefined8 *)(param_3 + 0xd0),
                 *(undefined8 *)(param_3 + 0xd8),*(undefined4 *)(param_3 + 0xf0),1);
    puVar7 = &UNK_0000d640 + param_1;
    FUN_009c3de4(puVar7,*(undefined8 *)(param_3 + 0xb8),0,0,0,*(undefined8 *)(param_3 + 0xd0),
                 *(undefined8 *)(param_3 + 0xd8),*(undefined4 *)(param_3 + 0xf0),1);
    FUN_009c3de4(&UNK_0000d748 + param_1,*(undefined8 *)(param_3 + 0xc0),0,0,0,
                 *(undefined8 *)(param_3 + 0xd0),*(undefined8 *)(param_3 + 0xd8),
                 *(undefined4 *)(param_3 + 0xf0),1);
    lVar5 = *plVar1;
    uVar9 = 1;
    puVar8 = &UNK_0000d748 + param_1;
    goto code_r0x001481e0;
  default:
    return 1;
  }
  puVar8 = (undefined *)0x0;
  uVar9 = 0;
code_r0x001481e0:
  func_0x001501ac(lVar5,param_2,puVar6,puVar7,puVar8,uVar9,param_5 == 1,param_3 + 0x110,
                  param_3 + 0x11c,1);
  return 1;
}


