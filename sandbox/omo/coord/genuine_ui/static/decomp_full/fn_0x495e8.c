// FUN_000495e8 size=780

void FUN_000495e8(long param_1)

{
  int iVar1;
  int iVar2;
  undefined8 uVar3;
  int *piVar4;
  uint *puVar5;
  long lVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  undefined8 uVar10;
  double dVar11;
  undefined4 local_70;
  undefined8 local_6c;
  undefined2 local_64;
  undefined4 local_60;
  undefined2 local_5c;
  undefined8 local_5a;
  undefined4 local_50;
  undefined2 uStack_4c;
  undefined2 uStack_4a;
  undefined6 uStack_48;
  
  uVar10 = *(undefined8 *)(param_1 + 0x260);
  if (*(char *)(param_1 + 0x2b8) == '\0') {
    uVar3 = FUN_002efa1c("Main Sample\n(other frame)",0x19);
    local_50 = (undefined4)uVar3;
    uStack_4c = (undefined2)((ulong)uVar3 >> 0x20);
    uStack_4a = (undefined2)((ulong)uVar3 >> 0x30);
    FUN_00046d70(uVar10,&local_50,0);
    piVar4 = (int *)CONCAT26(uStack_4a,CONCAT24(uStack_4c,local_50));
    if (*piVar4 != -1) {
      if (*piVar4 != 0) {
        LOAcquire();
        iVar1 = *piVar4;
        *piVar4 = iVar1 + -1;
        LORelease();
        if (iVar1 != 1) goto LAB_000496d0;
        piVar4 = (int *)CONCAT26(uStack_4a,CONCAT24(uStack_4c,local_50));
      }
      FUN_00287df8(piVar4,2,8);
    }
LAB_000496d0:
    puVar5 = &DAT_0142f030;
    local_50 = 0x142f030;
    uStack_4c = 0;
    uStack_4a = 0;
    uVar7 = DAT_0142f034 + 1;
    uVar9 = DAT_0142f038 & 0x7fffffff;
    uVar8 = DAT_0142f034;
    if (1 < DAT_0142f030 || uVar9 < uVar7) {
      uVar8 = uVar7;
      if (uVar7 <= uVar9) {
        uVar8 = uVar9;
      }
      FUN_00b09570(&local_50,uVar8,(ulong)(uVar9 < uVar7) << 3);
      puVar5 = (uint *)CONCAT26(uStack_4a,CONCAT24(uStack_4c,local_50));
      uVar8 = puVar5[1];
      uVar7 = uVar8 + 1;
    }
    *(undefined8 *)((long)puVar5 + (long)(int)uVar8 * 8 + *(long *)(puVar5 + 4)) =
         0x4014000000000000;
    puVar5[1] = uVar7;
    uVar9 = uVar7 + 1;
    uVar8 = puVar5[2] & 0x7fffffff;
    if (1 < *puVar5 || uVar8 < uVar9) {
      uVar7 = uVar9;
      if (uVar9 <= uVar8) {
        uVar7 = uVar8;
      }
      FUN_00b09570(&local_50,uVar7,(ulong)(uVar8 < uVar9) << 3);
      puVar5 = (uint *)CONCAT26(uStack_4a,CONCAT24(uStack_4c,local_50));
      uVar7 = puVar5[1];
      uVar9 = uVar7 + 1;
    }
    *(undefined8 *)((long)puVar5 + (long)(int)uVar7 * 8 + *(long *)(puVar5 + 4)) =
         0x4014000000000000;
    puVar5[1] = uVar9;
    FUN_00639ac8(param_1 + 0x268,&local_50);
    piVar4 = (int *)CONCAT26(uStack_4a,CONCAT24(uStack_4c,local_50));
    if (*piVar4 != -1) {
      if (*piVar4 != 0) {
        LOAcquire();
        iVar1 = *piVar4;
        *piVar4 = iVar1 + -1;
        LORelease();
        if (iVar1 != 1) goto LAB_000497c4;
        piVar4 = (int *)CONCAT26(uStack_4a,CONCAT24(uStack_4c,local_50));
      }
      FUN_00287df8(piVar4,8,8);
    }
    goto LAB_000497c4;
  }
  uVar3 = FUN_002efa1c("Main Sample",0xb);
  local_50 = (undefined4)uVar3;
  uStack_4c = (undefined2)((ulong)uVar3 >> 0x20);
  uStack_4a = (undefined2)((ulong)uVar3 >> 0x30);
  FUN_00046d70(uVar10,&local_50,0);
  piVar4 = (int *)CONCAT26(uStack_4a,CONCAT24(uStack_4c,local_50));
  if (*piVar4 != -1) {
    if (*piVar4 != 0) {
      LOAcquire();
      iVar1 = *piVar4;
      *piVar4 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_00049668;
      piVar4 = (int *)CONCAT26(uStack_4a,CONCAT24(uStack_4c,local_50));
    }
    FUN_00287df8(piVar4,2,8);
  }
LAB_00049668:
  FUN_00639078(param_1 + 0x268,1);
LAB_000497c4:
  local_50 = 0;
  uStack_4c = 0xffff;
  uStack_4a = 0;
  uStack_48 = 0;
  lVar6 = param_1 + 0x278;
  uVar10 = FUN_005a8a1c(lVar6);
  iVar1 = FUN_005a8a90(lVar6);
  dVar11 = (double)iVar1 * DAT_00c93fc8;
  uVar3 = FUN_005a8ae4(lVar6);
  FUN_005a6010(&local_50,uVar10,(int)dVar11,uVar3,0xff);
  FUN_00639ea8(param_1 + 0x268,&local_50);
  FUN_0092e77c(*(undefined8 *)(param_1 + 0x260),param_1 + 0x268);
  local_60 = 0;
  local_5c = 0xffff;
  lVar6 = param_1 + 0x290;
  local_5a = 0;
  uVar10 = FUN_005a8a1c(lVar6);
  iVar1 = FUN_005a8a90(lVar6);
  iVar2 = FUN_005a8ae4(lVar6);
  FUN_005a6010(&local_60,uVar10,(int)((double)iVar1 * 0.5),(int)((double)iVar2 * 1.2),0xff);
  lVar6 = *(long *)(param_1 + 0x260);
  local_70 = *(undefined4 *)(lVar6 + 200);
  local_6c = *(undefined8 *)(lVar6 + 0xcc);
  local_64 = *(undefined2 *)(lVar6 + 0xd4);
  iVar1 = FUN_005a9d1c(&local_70,&local_60);
  if (iVar1 != 0) {
    FUN_000472e4(*(undefined8 *)(param_1 + 0x260),&local_60);
  }
  return;
}


