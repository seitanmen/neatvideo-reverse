// FUN_000492d4 size=788

void FUN_000492d4(long param_1)

{
  long lVar1;
  long lVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  bool bVar7;
  int iVar8;
  undefined4 *puVar9;
  ulong uVar10;
  uint uVar11;
  undefined8 uVar12;
  long lVar13;
  undefined4 local_60;
  undefined8 local_5c;
  undefined2 local_54;
  undefined1 auStack_50 [16];
  undefined4 local_40;
  undefined8 local_3c;
  undefined2 local_34;
  
  uVar6 = DAT_0141a61a;
  iVar8 = *(int *)(param_1 + 0x1d8);
  iVar3 = *(int *)(param_1 + 0x1d0);
  iVar4 = *(int *)(param_1 + 0x1dc);
  iVar5 = *(int *)(param_1 + 0x1d4);
  uVar10 = (ulong)DAT_0141a61a;
  *(undefined **)(DAT_0141a628 + 0x73b42f8) = &DAT_0141a618;
  uVar11 = uVar6;
  if (*(long *)(DAT_0141a620 + 0x78233e0) != 0) {
    uVar11 = -(uint)*(byte *)(*(long *)(DAT_0141a620 + 0x78233e0) + (uVar10 & 0xffff));
  }
  if ((((*(int *)(param_1 + 0x2a8) == iVar3) && (*(int *)(param_1 + 0x2b0) == iVar8)) &&
      (*(int *)(param_1 + 0x2ac) == iVar5)) && (*(int *)(param_1 + 0x2b4) == iVar4)) {
    bVar7 = *(char *)(param_1 + 0x2b8) != '\0';
  }
  else {
    bVar7 = false;
  }
  lVar13 = *(long *)(param_1 + 0x1c0);
  if (param_1 + 0x1b8 != lVar13) {
    iVar4 = (iVar4 - iVar5) + 1;
    do {
      if (((int)((iVar8 - iVar3) + 1 +
                ((uVar11 + (uVar6 >> 0x10 & 0xff)) * 2 + (uVar6 >> 0x18)) * iVar4) <=
           *(int *)(lVar13 + 0x10)) || (iVar4 <= *(int *)(lVar13 + 0x14))) {
        lVar1 = param_1 + 0x220;
        iVar8 = FUN_0063906c(lVar1);
        if ((iVar8 == *(int *)(lVar13 + 0x18)) &&
           (iVar8 = FUN_00639cb0(lVar1), iVar8 == *(int *)(lVar13 + 0x1c))) {
          if (bVar7) {
            FUN_00639e84(&local_40,lVar1);
            uVar10 = FUN_005a9d1c(&local_40,param_1 + 0x278);
          }
          else {
            FUN_00639e84(auStack_50,lVar1);
            uVar10 = FUN_005a9d1c(auStack_50,lVar13 + 0x20);
          }
          if ((uVar10 & 1) == 0) {
            return;
          }
        }
        FUN_00639078(lVar1,*(undefined4 *)(lVar13 + 0x18));
        FUN_00639d04(lVar1,*(undefined4 *)(lVar13 + 0x1c));
        lVar2 = param_1 + 0x278;
        if (!bVar7) {
          lVar2 = lVar13 + 0x20;
        }
        FUN_00639ea8(lVar1,lVar2);
        if (*(char *)(lVar13 + 0x30) == '\0') goto LAB_00049574;
        FUN_00639078(param_1 + 0x288,1);
        if (bVar7) {
          lVar13 = param_1 + 0x290;
        }
        else {
          lVar13 = lVar13 + 0x34;
        }
        FUN_00639ea8(param_1 + 0x288,lVar13);
        goto LAB_0004958c;
      }
      lVar13 = *(long *)(lVar13 + 8);
    } while (param_1 + 0x1b8 != lVar13);
  }
  lVar13 = param_1 + 0x220;
  iVar8 = FUN_0063906c(lVar13);
  if ((iVar8 == 1) && (iVar8 = FUN_00639cb0(lVar13), iVar8 == 1)) {
    if (bVar7) {
      FUN_00639e84(&local_40,lVar13);
      uVar10 = FUN_005a9d1c(&local_40,param_1 + 0x278);
    }
    else {
      FUN_00639e84(auStack_50,lVar13);
      local_60 = 1;
      local_5c = DAT_00c93fc0;
      local_54 = 0;
      uVar10 = FUN_005a9d1c(auStack_50,&local_60);
    }
    if ((uVar10 & 1) == 0) {
      return;
    }
  }
  FUN_00639078(lVar13,1);
  FUN_00639d04(lVar13,1);
  if (bVar7) {
    puVar9 = (undefined4 *)(param_1 + 0x278);
  }
  else {
    local_40 = 1;
    local_3c = DAT_00c93fc0;
    local_34 = 0;
    puVar9 = &local_40;
  }
  FUN_00639ea8(lVar13,puVar9);
LAB_00049574:
  FUN_00639078(param_1 + 0x288,0);
LAB_0004958c:
  FUN_0092e77c(*(undefined8 *)(param_1 + 0x218),param_1 + 0x220);
  uVar12 = *(undefined8 *)(param_1 + 0x218);
  FUN_00639e84(&local_40,param_1 + 0x288);
  FUN_000472e4(uVar12,&local_40);
  uVar12 = *(undefined8 *)(param_1 + 0x218);
  iVar8 = FUN_0063906c(param_1 + 0x288);
  FUN_00047324(uVar12,iVar8 != 0);
  return;
}


