// FUN_000ed0f4 size=612

void FUN_000ed0f4(long param_1)

{
  undefined8 uVar1;
  long lVar2;
  undefined1 auStack_90 [8];
  undefined1 auStack_88 [8];
  undefined1 auStack_80 [8];
  undefined1 auStack_78 [8];
  undefined1 auStack_70 [8];
  undefined1 auStack_68 [8];
  undefined1 auStack_60 [8];
  undefined1 auStack_58 [8];
  undefined1 auStack_50 [8];
  undefined1 auStack_48 [8];
  undefined1 auStack_40 [8];
  undefined1 auStack_38 [8];
  
  func_0x0009588c();
  *(undefined8 *)(param_1 + 0xe0) = *(undefined8 *)(param_1 + 0x78);
  uVar1 = func_0x00095284(param_1);
  func_0x003d0e8c(auStack_38,uVar1,"2taskChanged(NITask*, NITask*)",param_1,
                  "1onTaskChanged(NITask*, NITask*)",0);
  func_0x003d24d8(auStack_38);
  func_0x003d0e8c(auStack_40,*(undefined8 *)(param_1 + 0x78),
                  "2previewModeChanged(PreviewMode, PreviewMode)",param_1,
                  "1onPreviewModeChanged(PreviewMode, PreviewMode)",0);
  func_0x003d24d8(auStack_40);
  func_0x003d0e8c(auStack_48,*(undefined8 *)(param_1 + 0x90),"2clicked(bool)",param_1,
                  "1onAutoProfileButtonClicked()",0);
  func_0x003d24d8(auStack_48);
  func_0x003d0e8c(auStack_50,*(undefined8 *)(param_1 + 0xe8),"2triggered()",
                  *(undefined8 *)(param_1 + 0xe0),"1autoBuildProfileWithMultiFrames()",0);
  func_0x003d24d8(auStack_50);
  func_0x003d0e8c(auStack_58,*(undefined8 *)(param_1 + 0xf0),"2triggered()",
                  *(undefined8 *)(param_1 + 0xe0),"1autoBuildProfileWithCurrentFrame()",0);
  func_0x003d24d8(auStack_58);
  func_0x003d0e8c(auStack_60,uRam000000000155b2d0,"2styleApplied()",param_1,
                  "1onStylistStyleChanged()",0);
  func_0x003d24d8(auStack_60);
  func_0x000ecab0(param_1);
  func_0x003d0e8c(auStack_68,*(undefined8 *)(param_1 + 0x98),"2clicked(bool)",
                  *(undefined8 *)(param_1 + 0xe0),"1loadProfile()",0);
  func_0x003d24d8(auStack_68);
  func_0x003d0e8c(auStack_70,*(undefined8 *)(param_1 + 0xa8),"2clicked(bool)",
                  *(undefined8 *)(param_1 + 0xe0),"1toggleGenericProfile()",0);
  func_0x003d24d8(auStack_70);
  func_0x003d0e8c(auStack_78,*(undefined8 *)(param_1 + 0xa0),"2clicked(bool)",
                  *(undefined8 *)(param_1 + 0xe0),"1autoMatchProfile()",0);
  func_0x003d24d8(auStack_78);
  func_0x003d0e8c(auStack_80,*(undefined8 *)(param_1 + 0xd0),"2toggled(bool)",
                  *(undefined8 *)(param_1 + 0xe0),"1toggleProfileCheck()",0);
  func_0x003d24d8(auStack_80);
  uVar1 = *(undefined8 *)(param_1 + 0xd8);
  lVar2 = func_0x00095284(param_1);
  func_0x003d0e8c(auStack_88,uVar1,"2toggled(bool)",*(undefined8 *)(lVar2 + 0x120),
                  "1toggleComparison(bool)",0);
  func_0x003d24d8(auStack_88);
  lVar2 = func_0x00095284(param_1);
  func_0x003d0e8c(auStack_90,*(undefined8 *)(lVar2 + 0x120),"2comparisonToggled(bool)",param_1,
                  "1onComparisonToggled(bool)",0);
  func_0x003d24d8(auStack_90);
  return;
}


