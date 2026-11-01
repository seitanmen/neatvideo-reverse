// FUN_00984b5c size=76

bool FUN_00984b5c(void)

{
  undefined8 uVar1;
  long lVar2;
  
  uVar1 = _objc_msgSend(uRam0000000001419220,PTR_s_processInfo_01417720);
  uVar1 = _objc_msgSend(uVar1,PTR_s_environment_01418d20);
  lVar2 = _objc_msgSend(uVar1,PTR_s_objectForKey__014177d0,0x140f818);
  return lVar2 != 0;
}


