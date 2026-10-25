// func entry: 00007104 name: FUN_00007104 size: 420

undefined8 FUN_00007104(long param_1,undefined8 param_2,undefined8 param_3)

{
  char cVar1;
  int iVar2;
  uint uVar3;
  undefined8 uVar4;
  long lVar5;
  uint uVar6;
  undefined8 local_60;
  int local_54;
  
  if (DAT_0141a1c8 != 0 && DAT_0141a1c8 != 3) {
    if (DAT_0141a1c8 == 10) {
      local_54 = 0;
      iVar2 = (**(code **)(DAT_0155b1f0 + 0x58))
                        (param_2,"OfxImageEffectPropThumbnailRender",0,&local_54);
      uVar6 = (uint)(iVar2 == 0 && local_54 == 1);
    }
    else {
      uVar6 = 0;
    }
    if (DAT_0155c408 != 1) {
      FUN_00af4a8c(&DAT_0155c400);
    }
    uVar4 = FUN_001c05cc(DAT_0155c418,*(undefined8 *)(param_1 + 0x18));
    lVar5 = FUN_001bdcf8();
    cVar1 = *(char *)(lVar5 + 0xbf34);
    FUN_001bdd20(uVar4);
    if (DAT_0155c408 != 1) {
      FUN_00af4a8c(&DAT_0155c400);
    }
    FUN_001c087c(DAT_0155c418,uVar4);
    if ((cVar1 == '\0') || (uVar3 = FUN_001b3dcc(*DAT_013b94b0), (uVar6 | uVar3) == 1)) {
      (**(code **)(DAT_0155b1f0 + 8))(param_3,"OfxPropName",0,"Source");
      (**(code **)(DAT_0155b1f0 + 0x50))(param_2,"OfxPropTime",0,&local_60);
      (**(code **)(DAT_0155b1f0 + 0x10))(local_60,param_3,"OfxPropTime",0);
      return 0;
    }
  }
  return 0xe;
}


