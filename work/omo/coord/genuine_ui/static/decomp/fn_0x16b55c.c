// FUN_0016b55c size=152

void FUN_0016b55c(undefined8 param_1,long *param_2)

{
  long lVar1;
  
  if (*param_2 != 0) {
    lVar1 = func_0x0016b704();
    if (1 < lVar1) {
      do {
        func_0x009d95a4("Assertion failed",0,0,0,0xffffffffffffffff,0xbb44edaf,lVar1);
      } while( true );
    }
    if (lVar1 != 0) {
      func_0x0016ad84(param_1,*param_2);
    }
    if (*param_2 != 0) {
      func_0x00168d44(param_1,*param_2 + 0x108);
    }
    *param_2 = 0;
  }
  return;
}


