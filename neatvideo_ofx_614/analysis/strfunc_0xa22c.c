// func entry: 0000a22c name: FUN_0000a22c size: 2364

undefined8 FUN_0000a22c(undefined8 param_1)

{
  char *pcVar1;
  int iVar2;
  undefined8 local_78;
  undefined8 local_70;
  undefined8 local_68;
  
  (**(code **)(DAT_0155b1e8 + 0x10))(param_1,"Output",&local_68);
  (**(code **)(DAT_0155b1f0 + 8))
            (local_68,"OfxImageEffectPropSupportedComponents",0,"OfxImageComponentRGBA");
  (**(code **)(DAT_0155b1f0 + 8))
            (local_68,"OfxImageEffectPropSupportedPixelDepths",0,"OfxBitDepthFloat");
  (**(code **)(DAT_0155b1f0 + 8))
            (local_68,"OfxImageEffectPropSupportedPixelDepths",1,"OfxBitDepthByte");
  (**(code **)(DAT_0155b1f0 + 8))
            (local_68,"OfxImageEffectPropSupportedPixelDepths",2,"OfxBitDepthShort");
  (**(code **)(DAT_0155b1f0 + 8))
            (local_68,"OfxImageEffectPropSupportedPixelDepths",3,"OfxBitDepthHalf");
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxImageEffectPropTemporalClipAccess",0,0);
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxImageEffectPropSupportsTiles",0,0);
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxImageClipPropIsMask",0,0);
  pcVar1 = "OfxFieldSingle";
  if (DAT_0141a1c8 != 2) {
    pcVar1 = "OfxFieldBoth";
  }
  (**(code **)(DAT_0155b1f0 + 8))(local_68,"OfxImageClipPropFieldExtraction",0,pcVar1);
  (**(code **)(DAT_0155b1e8 + 0x10))(param_1,"Source",&local_68);
  (**(code **)(DAT_0155b1f0 + 8))
            (local_68,"OfxImageEffectPropSupportedComponents",0,"OfxImageComponentRGBA");
  (**(code **)(DAT_0155b1f0 + 8))
            (local_68,"OfxImageEffectPropSupportedPixelDepths",0,"OfxBitDepthFloat");
  (**(code **)(DAT_0155b1f0 + 8))
            (local_68,"OfxImageEffectPropSupportedPixelDepths",1,"OfxBitDepthByte");
  (**(code **)(DAT_0155b1f0 + 8))
            (local_68,"OfxImageEffectPropSupportedPixelDepths",2,"OfxBitDepthShort");
  (**(code **)(DAT_0155b1f0 + 8))
            (local_68,"OfxImageEffectPropSupportedPixelDepths",3,"OfxBitDepthHalf");
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxImageEffectPropTemporalClipAccess",0,1);
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxImageEffectPropSupportsTiles",0,0);
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxImageClipPropIsMask",0,0);
  pcVar1 = "OfxFieldSingle";
  if (DAT_0141a1c8 != 2) {
    pcVar1 = "OfxFieldBoth";
  }
  (**(code **)(DAT_0155b1f0 + 8))(local_68,"OfxImageClipPropFieldExtraction",0,pcVar1);
  (**(code **)(DAT_0155b1e8 + 8))(param_1,&local_70);
  (*(code *)*DAT_0155b1f8)(local_70,"OfxParamTypePage","Controls",&local_78);
  (*(code *)*DAT_0155b1f8)(local_70,"OfxParamTypePushButton","Prepare Profile...",&local_68);
  (**(code **)(DAT_0155b1f0 + 8))(local_68,"OfxPropLabel",0,"Prepare Noise Profile");
  (**(code **)(DAT_0155b1f0 + 8))
            (local_68,"OfxParamPropHint",0,
             "Prepare a noise profile to enable the filters in this instance of the effect");
  (**(code **)(DAT_0155b1f0 + 8))(local_68,"OfxParamPropScriptName",0,"Prepare Profile...");
  (**(code **)(DAT_0155b1f0 + 8))(local_78,"OfxParamPropPageChild",0,"Prepare Profile...");
  if (DAT_0141a1c8 == 10) {
    (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"uk.ltd.filmlight:OfxParamPropMainThreadAction",0,1)
    ;
  }
  if (DAT_0141a1d0 == '\0') {
    iVar2 = 1;
  }
  else {
    (*(code *)*DAT_0155b1f8)(local_70,"OfxParamTypePushButton","Adjust Filters...",&local_68);
    (**(code **)(DAT_0155b1f0 + 8))(local_68,"OfxPropLabel",0,"Adjust Filter Settings");
    (**(code **)(DAT_0155b1f0 + 8))
              (local_68,"OfxParamPropHint",0,
               "Adjust filter settings to customize the processing applied by this instance of the effect"
              );
    (**(code **)(DAT_0155b1f0 + 8))(local_68,"OfxParamPropScriptName",0,"Adjust Filters...");
    (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxParamPropEnabled",0,0);
    (**(code **)(DAT_0155b1f0 + 8))(local_78,"OfxParamPropPageChild",1,"Adjust Filters...");
    if (DAT_0141a1c8 == 10) {
      (**(code **)(DAT_0155b1f0 + 0x18))
                (local_68,"uk.ltd.filmlight:OfxParamPropMainThreadAction",0,1);
    }
    iVar2 = 2;
  }
  (*(code *)*DAT_0155b1f8)(local_70,"OfxParamTypeCustom","ClipParams",&local_68);
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxParamPropSecret",0,1);
  (**(code **)(DAT_0155b1f0 + 8))(local_68,"OfxParamPropScriptName",0,"ClipParams");
  (**(code **)(DAT_0155b1f0 + 8))(local_68,"OfxParamPropDefault",0,"");
  (**(code **)(DAT_0155b1f0 + 8))
            (local_68,"OfxParamPropCacheInvalidation",0,"OfxParamInvalidateAll");
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxParamPropAnimates",0,0);
  (**(code **)(DAT_0155b1f0 + 8))(local_78,"OfxParamPropPageChild",iVar2,"ClipParams");
  if (DAT_0141a1c8 == 7) {
    (*(code *)*DAT_0155b1f8)(local_70,"OfxParamTypeString","ParamsHash1",&local_68);
    (**(code **)(DAT_0155b1f0 + 8))(local_68,"OfxParamPropDefault",0,"");
  }
  else {
    (*(code *)*DAT_0155b1f8)(local_70,"OfxParamTypeInteger","ParamsHash1",&local_68);
    (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxParamPropDefault",0,0);
  }
  (**(code **)(DAT_0155b1f0 + 8))(local_68,"OfxParamPropScriptName",0,"ParamsHash1");
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxParamPropSecret",0,1);
  (**(code **)(DAT_0155b1f0 + 8))
            (local_68,"OfxParamPropCacheInvalidation",0,"OfxParamInvalidateAll");
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxParamPropAnimates",0,0);
  (**(code **)(DAT_0155b1f0 + 8))(local_78,"OfxParamPropPageChild",iVar2 + 1,"ParamsHash1");
  (*(code *)*DAT_0155b1f8)(local_70,"OfxParamTypeInteger","ParamsHash2",&local_68);
  (**(code **)(DAT_0155b1f0 + 8))(local_68,"OfxParamPropScriptName",0,"ParamsHash2");
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxParamPropSecret",0,1);
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxParamPropDefault",0,0);
  (**(code **)(DAT_0155b1f0 + 8))
            (local_68,"OfxParamPropCacheInvalidation",0,"OfxParamInvalidateAll");
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxParamPropAnimates",0,0);
  (**(code **)(DAT_0155b1f0 + 8))(local_78,"OfxParamPropPageChild",iVar2 + 2,"ParamsHash2");
  (*(code *)*DAT_0155b1f8)(local_70,"OfxParamTypeInteger","ParamsHash3",&local_68);
  (**(code **)(DAT_0155b1f0 + 8))(local_68,"OfxParamPropScriptName",0,"ParamsHash3");
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxParamPropSecret",0,1);
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxParamPropDefault",0,0);
  (**(code **)(DAT_0155b1f0 + 8))
            (local_68,"OfxParamPropCacheInvalidation",0,"OfxParamInvalidateAll");
  (**(code **)(DAT_0155b1f0 + 0x18))(local_68,"OfxParamPropAnimates",0,0);
  (**(code **)(DAT_0155b1f0 + 8))(local_78,"OfxParamPropPageChild",iVar2 + 3,"ParamsHash3");
  return 0;
}


