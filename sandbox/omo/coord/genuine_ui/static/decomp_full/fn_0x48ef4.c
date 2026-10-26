// FUN_00048ef4 size=844

void FUN_00048ef4(long param_1)

{
  int iVar1;
  undefined1 auStack_48 [8];
  undefined1 auStack_40 [8];
  undefined1 auStack_38 [8];
  int *local_30;
  int *local_28;
  
  if (*(char *)(param_1 + 0x118) == '\0') {
    return;
  }
  if (*(long *)(param_1 + 0x2c0) != 0) {
    FUN_003d2778(*(long *)(param_1 + 0x2c0),"2pressed(QGraphicsSceneMouseEvent*)",param_1,
                 "1onSelectionMenuIconClicked(QGraphicsSceneMouseEvent*)");
    FUN_0004ebdc(param_1,*(undefined8 *)(param_1 + 0x2c0));
  }
  if (*(long *)(param_1 + 0x2c8) != 0) {
    FUN_003d2778(*(long *)(param_1 + 0x2c8),"2pressed(QGraphicsSceneMouseEvent*)",param_1,
                 "1onNonUniformInfoIconClicked()");
    FUN_0004ebdc(param_1,*(undefined8 *)(param_1 + 0x2c8));
  }
  if (*(long *)(param_1 + 0x2d0) != 0) {
    FUN_003d2778(*(long *)(param_1 + 0x2d0),"2pressed(QGraphicsSceneMouseEvent*)",param_1,
                 "1onClippingInfoIconClicked()");
    FUN_0004ebdc(param_1,*(undefined8 *)(param_1 + 0x2d0));
  }
  local_28 = (int *)FUN_002efa1c("Expand.png",10);
  FUN_003bbff0(&local_30,&DAT_013c0ef8,"Show additional actions",0,0xffffffff);
  FUN_0004eae4(0x4022000000000000,param_1,&local_28,&local_30,param_1 + 0x2c0);
  if (*local_30 != -1) {
    if (*local_30 != 0) {
      LOAcquire();
      iVar1 = *local_30;
      *local_30 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_00049014;
    }
    FUN_00287df8(local_30,2,8);
  }
LAB_00049014:
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_00049048;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_00049048:
  local_28 = (int *)FUN_002efa1c("Info.png",8);
  FUN_003bbff0(&local_30,&DAT_013c0ef8,"Show additional information",0,0xffffffff);
  FUN_0004eae4(0x4022000000000000,param_1,&local_28,&local_30,param_1 + 0x2c8);
  if (*local_30 != -1) {
    if (*local_30 != 0) {
      LOAcquire();
      iVar1 = *local_30;
      *local_30 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000490c8;
    }
    FUN_00287df8(local_30,2,8);
  }
LAB_000490c8:
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000490fc;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000490fc:
  local_28 = (int *)FUN_002efa1c("Info.png",8);
  FUN_003bbff0(&local_30,&DAT_013c0ef8,"Show additional information",0,0xffffffff);
  FUN_0004eae4(0x4022000000000000,param_1,&local_28,&local_30,param_1 + 0x2d0);
  if (*local_30 != -1) {
    if (*local_30 != 0) {
      LOAcquire();
      iVar1 = *local_30;
      *local_30 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0004917c;
    }
    FUN_00287df8(local_30,2,8);
  }
LAB_0004917c:
  if (*local_28 != -1) {
    if (*local_28 != 0) {
      LOAcquire();
      iVar1 = *local_28;
      *local_28 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_000491b0;
    }
    FUN_00287df8(local_28,2,8);
  }
LAB_000491b0:
  FUN_003d0e8c(auStack_38,*(undefined8 *)(param_1 + 0x2c0),"2pressed(QGraphicsSceneMouseEvent*)",
               param_1,"1onSelectionMenuIconClicked(QGraphicsSceneMouseEvent*)",0);
  FUN_003d24d8(auStack_38);
  FUN_003d0e8c(auStack_40,*(undefined8 *)(param_1 + 0x2c8),"2pressed(QGraphicsSceneMouseEvent*)",
               param_1,"1onNonUniformInfoIconClicked()",0);
  FUN_003d24d8(auStack_40);
  FUN_003d0e8c(auStack_48,*(undefined8 *)(param_1 + 0x2d0),"2pressed(QGraphicsSceneMouseEvent*)",
               param_1,"1onClippingInfoIconClicked()",0);
  FUN_003d24d8(auStack_48);
  return;
}


