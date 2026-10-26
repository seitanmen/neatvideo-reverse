// FUN_00042c44 size=688

void FUN_00042c44(undefined8 *param_1,ulong param_2)

{
  int iVar1;
  int *piVar2;
  
  if (((bRam0000000001538b80 & 1) == 0) && (iVar1 = ___cxa_guard_acquire(0x1538b80), iVar1 != 0)) {
    uRam0000000001538ae8 = func_0x002efa1c("Normal",6);
    uRam0000000001538af0 = func_0x002efa1c("R",1);
    uRam0000000001538af8 = func_0x002efa1c("G",1);
    uRam0000000001538b00 = func_0x002efa1c("B",1);
    uRam0000000001538b08 = func_0x002efa1c("Y",1);
    uRam0000000001538b10 = func_0x002efa1c("Cr",2);
    uRam0000000001538b18 = func_0x002efa1c("Cb",2);
    uRam0000000001538b20 = func_0x002efa1c("Y High",6);
    uRam0000000001538b28 = func_0x002efa1c("Y Mid",5);
    uRam0000000001538b30 = func_0x002efa1c("Y Low",5);
    uRam0000000001538b38 = func_0x002efa1c("Cr High",7);
    uRam0000000001538b40 = func_0x002efa1c("Cr Mid",6);
    uRam0000000001538b48 = func_0x002efa1c("Cr Low",6);
    uRam0000000001538b50 = func_0x002efa1c("Cb High",7);
    uRam0000000001538b58 = func_0x002efa1c("Cb Mid",6);
    uRam0000000001538b60 = func_0x002efa1c("Cb Low",6);
    uRam0000000001538b68 = func_0x002efa1c("Y Enhanced",10);
    uRam0000000001538b70 = func_0x002efa1c("Cr Enhanced",0xb);
    uRam0000000001538b78 = func_0x002efa1c("Cb Enhanced",0xb);
    ___cxa_atexit(&DAT_00042f20,0,0);
    ___cxa_guard_release(0x1538b80);
  }
  piVar2 = *(int **)((param_2 & 0xffffffff) * 8 + 0x1538ae8);
  *param_1 = piVar2;
  if (1 < *piVar2 + 1U) {
    LOAcquire();
    *piVar2 = *piVar2 + 1;
    LORelease();
  }
  return;
}


