// func entry: 000072a8 name: FUN_000072a8 size: 624

void FUN_000072a8(long param_1,undefined8 param_2,undefined8 param_3)

{
  ulong uVar1;
  ulong uVar2;
  undefined8 uVar3;
  long lVar4;
  ulong uVar5;
  double dVar6;
  double dVar7;
  undefined8 local_80;
  double local_78;
  double dStack_70;
  undefined8 local_68;
  double local_60;
  double local_58;
  double local_50;
  long local_48;
  
  local_48 = *(long *)PTR____stack_chk_guard_013b4320;
  if (DAT_0141a1d2 == '\0') {
    (**(code **)(DAT_0155b1f0 + 0x50))(param_2,"OfxPropTime",0,&local_60);
    (**(code **)(DAT_0155b1e8 + 0x18))(*(undefined8 *)(param_1 + 0x10),"Source",&local_68,0);
    (**(code **)(DAT_0155b1e8 + 0x20))(local_68,&local_80);
    (**(code **)(DAT_0155b1f0 + 0x70))(local_80,"OfxImageEffectPropFrameRange",2,&local_78);
    if (DAT_0155c408 != 1) {
      FUN_00af4a8c(&DAT_0155c400);
    }
    uVar3 = FUN_001c05cc(DAT_0155c418,*(undefined8 *)(param_1 + 0x18));
    lVar4 = FUN_001bdcf8();
    if (*(char *)(lVar4 + 0xd5f5) == '\0') {
      uVar5 = 0;
    }
    else {
      uVar5 = (ulong)*(int *)(lVar4 + 0xd5f8);
    }
    FUN_001bdd20(uVar3);
    if (DAT_0155c408 != 1) {
      FUN_00af4a8c(&DAT_0155c400);
    }
    FUN_001c087c(DAT_0155c418,uVar3);
    uVar1 = 7;
    if (DAT_0141a1d1 != '\0') {
      uVar1 = 0;
    }
    uVar2 = uVar1;
    if ((long)uVar1 <= (long)uVar5) {
      uVar2 = uVar5;
    }
    lVar4 = FUN_001bf474(uVar5);
    if ((long)uVar1 <= (long)(lVar4 + ~uVar5)) {
      uVar1 = lVar4 + ~uVar5;
    }
    dVar6 = local_60 - (double)(long)uVar2;
    dVar7 = dStack_70;
    if (dVar6 <= dStack_70) {
      dVar7 = dVar6;
    }
    local_58 = local_78;
    if (local_78 <= dVar6) {
      local_58 = dVar7;
    }
    local_60 = local_60 + (double)(long)uVar1;
    dVar7 = dStack_70;
    if (local_60 <= dStack_70) {
      dVar7 = local_60;
    }
    local_50 = local_78;
    if (local_78 <= local_60) {
      local_50 = dVar7;
    }
    if ((DAT_0141a1c8 == 4) && (lVar4 = (long)(15.0 - ((local_50 - local_58) + 1.0)), 0 < lVar4)) {
      dVar7 = (double)lVar4;
      if ((local_58 == local_78) &&
         (dVar6 = local_50 + dVar7, local_50 = dStack_70, dVar6 <= dStack_70)) {
        local_50 = dVar6;
      }
      if ((local_50 == dStack_70) && (local_58 = local_58 - dVar7, local_58 <= local_78)) {
        local_58 = local_78;
      }
    }
    (**(code **)(DAT_0155b1f0 + 0x30))(param_3,"OfxImageClipPropFrameRange_Source",2,&local_58);
    uVar3 = 0;
  }
  else {
    uVar3 = 0xe;
  }
  if (*(long *)PTR____stack_chk_guard_013b4320 == local_48) {
    return;
  }
                    /* WARNING: Subroutine does not return */
  ___stack_chk_fail(uVar3);
}


