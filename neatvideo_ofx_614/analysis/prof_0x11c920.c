// func entry: 0011c920 name: FUN_0011c920 size: 5052

undefined8 * FUN_0011c920(undefined8 *param_1,undefined8 param_2)

{
  uint uVar1;
  int iVar2;
  undefined8 uVar3;
  uint *puVar4;
  undefined1 auStack_148 [8];
  undefined1 auStack_140 [8];
  undefined1 auStack_138 [8];
  undefined1 auStack_130 [8];
  int *local_128;
  int *local_120;
  int *local_118;
  int *local_110;
  int *local_108;
  int *local_100;
  int *local_f8;
  uint *local_f0 [2];
  int *local_e0;
  int *local_d8;
  int *local_d0;
  int *local_c8;
  int *local_c0;
  int *local_b8;
  int *local_b0;
  int *local_a8;
  int *local_a0;
  int *local_98;
  int *local_90;
  int *local_88;
  uint *local_80;
  int *local_78;
  int *local_70;
  uint *local_68;
  uint *local_60;
  uint *local_58;
  int *local_50;
  int *local_48;
  
  uVar3 = FUN_0006dc8c(param_2,0);
  FUN_0089c0a4(param_1,uVar3,0);
  *param_1 = &DAT_013c78a0;
  param_1[2] = &DAT_013c7a78;
  param_1[0x19] = &DAT_013c7ac8;
  FUN_000673dc(param_1 + 0x19,param_1);
  *param_1 = &DAT_013c78a0;
  param_1[2] = &DAT_013c7a78;
  param_1[0x19] = &DAT_013c7ac8;
  FUN_0011eb64(param_1 + 6,param_1);
  uVar3 = FUN_00752dd0(param_1);
  FUN_0073c2a4(uVar3,3);
  FUN_000141e0(param_1[0x16],2);
  local_48 = (int *)FUN_002efa1c("Mac",3);
  local_50 = (int *)FUN_002efa1c("MacOS",5);
  local_68 = (uint *)FUN_002efa1c("<p>     Registration code (%1): <span class=\"gray_text\">Your registration code for %2</span></p>"
                                  ,0x60);
  local_70 = (int *)FUN_002efa1c("Win",3);
  FUN_002f4ef8(&local_60,&local_68,&local_70,0,0x20);
  local_78 = (int *)FUN_002efa1c("Windows",7);
  FUN_002f4ef8(local_f0,&local_60,&local_78,0,0x20);
  local_90 = (int *)FUN_002efa1c("<p>     Registration code <span class=\"bold_text_hl\">(%1)</span>: <span class=\"green_text_hl\">Your registration code for %2</span></p>"
                                 ,0x86);
  local_98 = (int *)FUN_002efa1c("Mac",3);
  FUN_002f4ef8(&local_88,&local_90,&local_98,0,0x20);
  local_a0 = (int *)FUN_002efa1c("MacOS",5);
  FUN_002f4ef8(&local_80,&local_88,&local_a0,0,0x20);
  local_58 = local_f0[0];
  puVar4 = local_f0[0];
  if (1 < *local_f0[0] + 1) {
    LOAcquire();
    uVar1 = *local_f0[0];
    *local_f0[0] = uVar1 + 1;
    puVar4 = (uint *)(ulong)uVar1;
    LORelease();
  }
  FUN_002e97cc(puVar4,&local_58,&local_80);
  if (*local_80 != 0xffffffff) {
    if (*local_80 != 0) {
      LOAcquire();
      uVar1 = *local_80;
      *local_80 = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011cb1c;
    }
    FUN_00287df8(local_80,2,8);
  }
LAB_0011cb1c:
  if (*local_a0 != -1) {
    if (*local_a0 != 0) {
      LOAcquire();
      iVar2 = *local_a0;
      *local_a0 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011cb50;
    }
    FUN_00287df8(local_a0,2,8);
  }
LAB_0011cb50:
  if (*local_88 != -1) {
    if (*local_88 != 0) {
      LOAcquire();
      iVar2 = *local_88;
      *local_88 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011cb84;
    }
    FUN_00287df8(local_88,2,8);
  }
LAB_0011cb84:
  if (*local_98 != -1) {
    if (*local_98 != 0) {
      LOAcquire();
      iVar2 = *local_98;
      *local_98 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011cbb8;
    }
    FUN_00287df8(local_98,2,8);
  }
LAB_0011cbb8:
  if (*local_90 != -1) {
    if (*local_90 != 0) {
      LOAcquire();
      iVar2 = *local_90;
      *local_90 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011cbec;
    }
    FUN_00287df8(local_90,2,8);
  }
LAB_0011cbec:
  if (*local_f0[0] != 0xffffffff) {
    if (*local_f0[0] != 0) {
      LOAcquire();
      uVar1 = *local_f0[0];
      *local_f0[0] = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011cc20;
    }
    FUN_00287df8(local_f0[0],2,8);
  }
LAB_0011cc20:
  if (*local_78 != -1) {
    if (*local_78 != 0) {
      LOAcquire();
      iVar2 = *local_78;
      *local_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011cc54;
    }
    FUN_00287df8(local_78,2,8);
  }
LAB_0011cc54:
  if (*local_60 != 0xffffffff) {
    if (*local_60 != 0) {
      LOAcquire();
      uVar1 = *local_60;
      *local_60 = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011cc88;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_0011cc88:
  if (*local_70 != -1) {
    if (*local_70 != 0) {
      LOAcquire();
      iVar2 = *local_70;
      *local_70 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011ccbc;
    }
    FUN_00287df8(local_70,2,8);
  }
LAB_0011ccbc:
  if (*local_68 != 0xffffffff) {
    if (*local_68 != 0) {
      LOAcquire();
      uVar1 = *local_68;
      *local_68 = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011ccf0;
    }
    FUN_00287df8(local_68,2,8);
  }
LAB_0011ccf0:
  local_78 = (int *)FUN_002efa1c("<p>     Registration code (%1): <span class=\"gray_text\">Your registration code for %2</span></p>"
                                 ,0x60);
  local_80 = (uint *)FUN_002efa1c("Win",3);
  FUN_002f4ef8(&local_70,&local_78,&local_80,0,0x20);
  local_88 = (int *)FUN_002efa1c("Windows",7);
  FUN_002f4ef8(&local_68,&local_70,&local_88,0,0x20);
  local_a0 = (int *)FUN_002efa1c("<p>     Registration code <span class=\"bold_text_hl\">(%1)</span>: <span class=\"green_text_hl\">Your registration code for %2</span></p>"
                                 ,0x86);
  local_a8 = (int *)FUN_002efa1c("Mac",3);
  FUN_002f4ef8(&local_98,&local_a0,&local_a8,0,0x20);
  local_b0 = (int *)FUN_002efa1c("MacOS",5);
  FUN_002f4ef8(&local_90,&local_98,&local_b0,0,0x20);
  local_f0[0] = local_68;
  puVar4 = local_68;
  if (1 < *local_68 + 1) {
    LOAcquire();
    uVar1 = *local_68;
    *local_68 = uVar1 + 1;
    puVar4 = (uint *)(ulong)uVar1;
    LORelease();
  }
  FUN_002e97cc(puVar4,local_f0,&local_90);
  local_c8 = (int *)FUN_002efa1c("<p>     Registration code (%1): <span class=\"gray_text\">Your registration code for %2</span></p>"
                                 ,0x60);
  local_d0 = (int *)FUN_002efa1c("Lin",3);
  FUN_002f4ef8(&local_c0,&local_c8,&local_d0,0,0x20);
  local_d8 = (int *)FUN_002efa1c("Linux",5);
  FUN_002f4ef8(&local_b8,&local_c0,&local_d8,0,0x20);
  local_60 = local_f0[0];
  puVar4 = local_f0[0];
  if (1 < *local_f0[0] + 1) {
    LOAcquire();
    uVar1 = *local_f0[0];
    *local_f0[0] = uVar1 + 1;
    puVar4 = (uint *)(ulong)uVar1;
    LORelease();
  }
  FUN_002e97cc(puVar4,&local_60,&local_b8);
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar2 = *local_b8;
      *local_b8 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011cec0;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_0011cec0:
  if (*local_d8 != -1) {
    if (*local_d8 != 0) {
      LOAcquire();
      iVar2 = *local_d8;
      *local_d8 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011cef4;
    }
    FUN_00287df8(local_d8,2,8);
  }
LAB_0011cef4:
  if (*local_c0 != -1) {
    if (*local_c0 != 0) {
      LOAcquire();
      iVar2 = *local_c0;
      *local_c0 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011cf28;
    }
    FUN_00287df8(local_c0,2,8);
  }
LAB_0011cf28:
  if (*local_d0 != -1) {
    if (*local_d0 != 0) {
      LOAcquire();
      iVar2 = *local_d0;
      *local_d0 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011cf5c;
    }
    FUN_00287df8(local_d0,2,8);
  }
LAB_0011cf5c:
  if (*local_c8 != -1) {
    if (*local_c8 != 0) {
      LOAcquire();
      iVar2 = *local_c8;
      *local_c8 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011cf90;
    }
    FUN_00287df8(local_c8,2,8);
  }
LAB_0011cf90:
  if (*local_f0[0] != 0xffffffff) {
    if (*local_f0[0] != 0) {
      LOAcquire();
      uVar1 = *local_f0[0];
      *local_f0[0] = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011cfc4;
    }
    FUN_00287df8(local_f0[0],2,8);
  }
LAB_0011cfc4:
  if (*local_90 != -1) {
    if (*local_90 != 0) {
      LOAcquire();
      iVar2 = *local_90;
      *local_90 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011cff8;
    }
    FUN_00287df8(local_90,2,8);
  }
LAB_0011cff8:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar2 = *local_b0;
      *local_b0 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d02c;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_0011d02c:
  if (*local_98 != -1) {
    if (*local_98 != 0) {
      LOAcquire();
      iVar2 = *local_98;
      *local_98 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d060;
    }
    FUN_00287df8(local_98,2,8);
  }
LAB_0011d060:
  if (*local_a8 != -1) {
    if (*local_a8 != 0) {
      LOAcquire();
      iVar2 = *local_a8;
      *local_a8 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d094;
    }
    FUN_00287df8(local_a8,2,8);
  }
LAB_0011d094:
  if (*local_a0 != -1) {
    if (*local_a0 != 0) {
      LOAcquire();
      iVar2 = *local_a0;
      *local_a0 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d0c8;
    }
    FUN_00287df8(local_a0,2,8);
  }
LAB_0011d0c8:
  if (*local_68 != 0xffffffff) {
    if (*local_68 != 0) {
      LOAcquire();
      uVar1 = *local_68;
      *local_68 = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011d0fc;
    }
    FUN_00287df8(local_68,2,8);
  }
LAB_0011d0fc:
  if (*local_88 != -1) {
    if (*local_88 != 0) {
      LOAcquire();
      iVar2 = *local_88;
      *local_88 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d130;
    }
    FUN_00287df8(local_88,2,8);
  }
LAB_0011d130:
  if (*local_70 != -1) {
    if (*local_70 != 0) {
      LOAcquire();
      iVar2 = *local_70;
      *local_70 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d164;
    }
    FUN_00287df8(local_70,2,8);
  }
LAB_0011d164:
  if (*local_80 != 0xffffffff) {
    if (*local_80 != 0) {
      LOAcquire();
      uVar1 = *local_80;
      *local_80 = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011d198;
    }
    FUN_00287df8(local_80,2,8);
  }
LAB_0011d198:
  if (*local_78 != -1) {
    if (*local_78 != 0) {
      LOAcquire();
      iVar2 = *local_78;
      *local_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d1cc;
    }
    FUN_00287df8(local_78,2,8);
  }
LAB_0011d1cc:
  local_f0[0] = (uint *)FUN_002efa1c(" (%1)",5);
  FUN_002f4ef8(&local_68,local_f0,&local_48,0,0x20);
  if (*local_f0[0] != 0xffffffff) {
    if (*local_f0[0] != 0) {
      LOAcquire();
      uVar1 = *local_f0[0];
      *local_f0[0] = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011d22c;
    }
    FUN_00287df8(local_f0[0],2,8);
  }
LAB_0011d22c:
  local_f0[0] = (uint *)FUN_002efa1c(" (%1)",5);
  FUN_002f4ef8(&local_70,local_f0,&local_50,0,0x20);
  if (*local_f0[0] != 0xffffffff) {
    if (*local_f0[0] != 0) {
      LOAcquire();
      uVar1 = *local_f0[0];
      *local_f0[0] = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011d28c;
    }
    FUN_00287df8(local_f0[0],2,8);
  }
LAB_0011d28c:
  local_78 = (int *)FUN_002efa1c(" (Win+Mac+Lin)",0xe);
  local_80 = local_60;
  puVar4 = local_60;
  if (1 < *local_60 + 1) {
    LOAcquire();
    uVar1 = *local_60;
    *local_60 = uVar1 + 1;
    puVar4 = (uint *)(ulong)uVar1;
    LORelease();
  }
  local_f0[0] = (uint *)FUN_002efa1c(puVar4,"Registering Neat Video 6.1.4 Demo",0x21);
  FUN_00750130(param_1,local_f0);
  if (*local_f0[0] != 0xffffffff) {
    if (*local_f0[0] != 0) {
      LOAcquire();
      uVar1 = *local_f0[0];
      *local_f0[0] = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011d310;
    }
    FUN_00287df8(local_f0[0],2,8);
  }
LAB_0011d310:
  uVar3 = param_1[0xc];
  local_f0[0] = (uint *)FUN_002efa1c("Neat Video v6 Demo plug-in for OpenFX hosts (Mac)",0x31);
  FUN_00809dc0(uVar3,local_f0);
  if (*local_f0[0] != 0xffffffff) {
    if (*local_f0[0] != 0) {
      LOAcquire();
      uVar1 = *local_f0[0];
      *local_f0[0] = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011d368;
    }
    FUN_00287df8(local_f0[0],2,8);
  }
LAB_0011d368:
  uVar3 = param_1[10];
  local_88 = (int *)FUN_002efa1c("Registration code%1:",0x14);
  FUN_002f4ef8(local_f0,&local_88,&local_70,0,0x20);
  FUN_00809dc0(uVar3,local_f0);
  if (*local_f0[0] != 0xffffffff) {
    if (*local_f0[0] != 0) {
      LOAcquire();
      uVar1 = *local_f0[0];
      *local_f0[0] = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011d3d8;
    }
    FUN_00287df8(local_f0[0],2,8);
  }
LAB_0011d3d8:
  if (*local_88 != -1) {
    if (*local_88 != 0) {
      LOAcquire();
      iVar2 = *local_88;
      *local_88 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d40c;
    }
    FUN_00287df8(local_88,2,8);
  }
LAB_0011d40c:
  uVar3 = param_1[0x16];
  thunk_FUN_00471e7c(local_f0,0xf);
  FUN_0074d1a0(uVar3,local_f0);
  FUN_004715ec(local_f0);
  uVar3 = param_1[0x16];
  local_d0 = (int *)FUN_002efa1c("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\"><html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">p {color:         #000000;white-space:   nowrap;font-family:   \"Courier New\",Courier,monospace;font-size:     %1;margin-top:    0px;margin-bottom: 0px;margin-left:   0px;margin-right:  0px;}.header_text p {font-family:   \"%2\",Arial,Helvetica,Sans-serif;}.gray_text p, span {color:         #969696;}.red_text_hl {color:         #cc3300;background-color: rgba(255,0,0,0.1);font-weight:   900;}.green_text_hl {color:         #007000;background-color: rgba(0,255,0,0.25);font-weight:   900;}.blue_text_hl {color:         #3232be;background-color: rgba(0,0,255,0.15);font-weight:   900;}.blue_text {color:         #3232be;font-weight:   900;}.bold_text_hl {color:         #000000;background-color: rgba(0,0,0,0.1);font-weight:   900;}.bold_text {color:         #000000;font-weight:   900;}</style></head><body style=\"background-color: #ffffff; margin: 8px;\"><div class=\"header_text\"><p>Please find the license for <span class=\"blue_text\">%3</span> in the e&#x2011;mail with download instruction that you received after purchase.</p><p> </p><p>That e&#x2011;mail looked like this:</p></div><hr /><div class=\"gray_text\"><p>E-MAIL</p><p>Subject: %4%5: license and download instruction</p><p>...</p><p>Thank you for purchasing our product:</p><p> %6%7</p><p> </p><p>This letter contains your license and download instruction.</p><p>Please save it for future reference.</p><p>...</p><p>     Please enter the following data exactly as specified below</p><p>     (case-sensitive; spaces are significant):</p><p> </p></div><p>     %8</p><p>                        Name: <span class=\"green_text_hl\">Your name</span></p><p>                Organization: <span class=\"green_text_hl\">Your organization</span> <span class=\"gray_text\">(may be blank)</span></p><p>            Number of copies: <span class=\"green_text_hl\">1</span> <span class=\"gray_text\">(may be different)</span></p>%9<p>     ---------..." /* TRUNCATED STRING LITERAL */
                                 ,0x86c);
  FUN_0005eaac(&local_d8,0x3ff0000000000000,DAT_0155b2d0);
  FUN_002f4ef8(&local_c8,&local_d0,&local_d8,0,0x20);
  thunk_FUN_00465d78(local_f0);
  FUN_004d79c8(&local_e0,local_f0);
  FUN_002f4ef8(&local_c0,&local_c8,&local_e0,0,0x20);
  local_f8 = (int *)FUN_002efa1c("Neat Video v6 Demo plug-in for OpenFX hosts",0x2b);
  FUN_002f4ef8(&local_b8,&local_c0,&local_f8,0,0x20);
  local_100 = (int *)FUN_002efa1c("Neat Video Demo plug-in for OpenFX hosts",0x28);
  FUN_002f4ef8(&local_b0,&local_b8,&local_100,0,0x20);
  FUN_002f4ef8(&local_a8,&local_b0,&local_78,0,0x20);
  local_108 = (int *)FUN_002efa1c("Neat Video v6 Demo plug-in for OpenFX hosts",0x2b);
  FUN_002f4ef8(&local_a0,&local_a8,&local_108,0,0x20);
  FUN_002f4ef8(&local_98,&local_a0,&local_78,0,0x20);
  local_120 = (int *)FUN_002efa1c("-- License for <span class=\"blue_text_hl\">%1</span> ----------------------------------------"
                                  ,0x5c);
  local_128 = (int *)FUN_002efa1c("Neat Video v6 Demo plug-in for OpenFX hosts",0x2b);
  FUN_002f4ef8(&local_118,&local_120,&local_128,0,0x20);
  FUN_002eebe8(&local_110,&local_118,0x5e);
  FUN_002f4ef8(&local_90,&local_98,&local_110,0,0x20);
  FUN_002f4ef8(&local_88,&local_90,&local_80,0,0x20);
  FUN_00756efc(uVar3,&local_88);
  if (*local_88 != -1) {
    if (*local_88 != 0) {
      LOAcquire();
      iVar2 = *local_88;
      *local_88 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d618;
    }
    FUN_00287df8(local_88,2,8);
  }
LAB_0011d618:
  if (*local_90 != -1) {
    if (*local_90 != 0) {
      LOAcquire();
      iVar2 = *local_90;
      *local_90 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d64c;
    }
    FUN_00287df8(local_90,2,8);
  }
LAB_0011d64c:
  if (*local_110 != -1) {
    if (*local_110 != 0) {
      LOAcquire();
      iVar2 = *local_110;
      *local_110 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d680;
    }
    FUN_00287df8(local_110,2,8);
  }
LAB_0011d680:
  if (*local_118 != -1) {
    if (*local_118 != 0) {
      LOAcquire();
      iVar2 = *local_118;
      *local_118 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d6b4;
    }
    FUN_00287df8(local_118,2,8);
  }
LAB_0011d6b4:
  if (*local_128 != -1) {
    if (*local_128 != 0) {
      LOAcquire();
      iVar2 = *local_128;
      *local_128 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d6e8;
    }
    FUN_00287df8(local_128,2,8);
  }
LAB_0011d6e8:
  if (*local_120 != -1) {
    if (*local_120 != 0) {
      LOAcquire();
      iVar2 = *local_120;
      *local_120 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d71c;
    }
    FUN_00287df8(local_120,2,8);
  }
LAB_0011d71c:
  if (*local_98 != -1) {
    if (*local_98 != 0) {
      LOAcquire();
      iVar2 = *local_98;
      *local_98 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d750;
    }
    FUN_00287df8(local_98,2,8);
  }
LAB_0011d750:
  if (*local_a0 != -1) {
    if (*local_a0 != 0) {
      LOAcquire();
      iVar2 = *local_a0;
      *local_a0 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d784;
    }
    FUN_00287df8(local_a0,2,8);
  }
LAB_0011d784:
  if (*local_108 != -1) {
    if (*local_108 != 0) {
      LOAcquire();
      iVar2 = *local_108;
      *local_108 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d7b8;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_0011d7b8:
  if (*local_a8 != -1) {
    if (*local_a8 != 0) {
      LOAcquire();
      iVar2 = *local_a8;
      *local_a8 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d7ec;
    }
    FUN_00287df8(local_a8,2,8);
  }
LAB_0011d7ec:
  if (*local_b0 != -1) {
    if (*local_b0 != 0) {
      LOAcquire();
      iVar2 = *local_b0;
      *local_b0 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d820;
    }
    FUN_00287df8(local_b0,2,8);
  }
LAB_0011d820:
  if (*local_100 != -1) {
    if (*local_100 != 0) {
      LOAcquire();
      iVar2 = *local_100;
      *local_100 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d854;
    }
    FUN_00287df8(local_100,2,8);
  }
LAB_0011d854:
  if (*local_b8 != -1) {
    if (*local_b8 != 0) {
      LOAcquire();
      iVar2 = *local_b8;
      *local_b8 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d888;
    }
    FUN_00287df8(local_b8,2,8);
  }
LAB_0011d888:
  if (*local_f8 != -1) {
    if (*local_f8 != 0) {
      LOAcquire();
      iVar2 = *local_f8;
      *local_f8 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d8bc;
    }
    FUN_00287df8(local_f8,2,8);
  }
LAB_0011d8bc:
  if (*local_c0 != -1) {
    if (*local_c0 != 0) {
      LOAcquire();
      iVar2 = *local_c0;
      *local_c0 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d8f0;
    }
    FUN_00287df8(local_c0,2,8);
  }
LAB_0011d8f0:
  if (*local_e0 != -1) {
    if (*local_e0 != 0) {
      LOAcquire();
      iVar2 = *local_e0;
      *local_e0 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d924;
    }
    FUN_00287df8(local_e0,2,8);
  }
LAB_0011d924:
  thunk_FUN_004d4dd4(local_f0);
  if (*local_c8 != -1) {
    if (*local_c8 != 0) {
      LOAcquire();
      iVar2 = *local_c8;
      *local_c8 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d960;
    }
    FUN_00287df8(local_c8,2,8);
  }
LAB_0011d960:
  if (*local_d8 != -1) {
    if (*local_d8 != 0) {
      LOAcquire();
      iVar2 = *local_d8;
      *local_d8 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d994;
    }
    FUN_00287df8(local_d8,2,8);
  }
LAB_0011d994:
  if (*local_d0 != -1) {
    if (*local_d0 != 0) {
      LOAcquire();
      iVar2 = *local_d0;
      *local_d0 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011d9c8;
    }
    FUN_00287df8(local_d0,2,8);
  }
LAB_0011d9c8:
  FUN_003d0e8c(auStack_130,param_1[0x16],"2linkActivated(const QString&)",param_1,
               "1onHelpLinkClicked()",0);
  FUN_003d24d8(auStack_130);
  FUN_0084ea88(param_1[0x11],0);
  FUN_003d0e8c(auStack_138,param_1[0x11],"2accepted()",param_1,"1onOkButtonClick()",0);
  FUN_003d24d8(auStack_138);
  FUN_003d0e8c(auStack_140,param_1[0x11],"2rejected()",param_1,"1reject()",0);
  FUN_003d24d8(auStack_140);
  if (DAT_0155b628 != '\0') {
    uVar3 = __Znwm(0x10);
    local_88 = (int *)FUN_002efa1c("F1",2);
    FUN_0047d328(local_f0,&local_88,0);
    FUN_0073f5cc(uVar3,local_f0,param_1,0,0,1);
    param_1[0x20] = uVar3;
    FUN_0047d3f4(local_f0);
    if (*local_88 != -1) {
      if (*local_88 != 0) {
        LOAcquire();
        iVar2 = *local_88;
        *local_88 = iVar2 + -1;
        LORelease();
        if (iVar2 != 1) goto LAB_0011daf4;
      }
      FUN_00287df8(local_88,2,8);
    }
LAB_0011daf4:
    FUN_003d0e8c(auStack_148,param_1[0x20],"2activated()",param_1,"1onHelpShortcutActivated()",0);
    FUN_003d24d8(auStack_148);
  }
  if (*local_80 != 0xffffffff) {
    if (*local_80 != 0) {
      LOAcquire();
      uVar1 = *local_80;
      *local_80 = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011db54;
    }
    FUN_00287df8(local_80,2,8);
  }
LAB_0011db54:
  if (*local_78 != -1) {
    if (*local_78 != 0) {
      LOAcquire();
      iVar2 = *local_78;
      *local_78 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011db88;
    }
    FUN_00287df8(local_78,2,8);
  }
LAB_0011db88:
  if (*local_70 != -1) {
    if (*local_70 != 0) {
      LOAcquire();
      iVar2 = *local_70;
      *local_70 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011dbbc;
    }
    FUN_00287df8(local_70,2,8);
  }
LAB_0011dbbc:
  if (*local_68 != 0xffffffff) {
    if (*local_68 != 0) {
      LOAcquire();
      uVar1 = *local_68;
      *local_68 = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011dbf0;
    }
    FUN_00287df8(local_68,2,8);
  }
LAB_0011dbf0:
  if (*local_60 != 0xffffffff) {
    if (*local_60 != 0) {
      LOAcquire();
      uVar1 = *local_60;
      *local_60 = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011dc24;
    }
    FUN_00287df8(local_60,2,8);
  }
LAB_0011dc24:
  if (*local_58 != 0xffffffff) {
    if (*local_58 != 0) {
      LOAcquire();
      uVar1 = *local_58;
      *local_58 = uVar1 - 1;
      LORelease();
      if (uVar1 != 1) goto LAB_0011dc58;
    }
    FUN_00287df8(local_58,2,8);
  }
LAB_0011dc58:
  if (*local_50 != -1) {
    if (*local_50 != 0) {
      LOAcquire();
      iVar2 = *local_50;
      *local_50 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) goto LAB_0011dc8c;
    }
    FUN_00287df8(local_50,2,8);
  }
LAB_0011dc8c:
  if (*local_48 != -1) {
    if (*local_48 != 0) {
      LOAcquire();
      iVar2 = *local_48;
      *local_48 = iVar2 + -1;
      LORelease();
      if (iVar2 != 1) {
        return param_1;
      }
    }
    FUN_00287df8(local_48,2,8);
  }
  return param_1;
}


