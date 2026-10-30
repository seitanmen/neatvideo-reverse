// func entry: 0011ae10 name: FUN_0011ae10 size: 6544

void FUN_0011ae10(long param_1,undefined8 param_2)

{
  int iVar1;
  undefined8 uVar2;
  int *local_38;
  int *local_30 [2];
  
  FUN_003b8988(local_30,"profileViewerDialog","Profile Viewer",0,0xffffffff);
  FUN_00750130(param_2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011ae88;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011ae88:
  uVar2 = *(undefined8 *)(param_1 + 0x28);
  FUN_003b8988(local_30,"profileViewerDialog","Image Metadata",0,0xffffffff);
  FUN_00808038(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011aeec;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011aeec:
  uVar2 = *(undefined8 *)(param_1 + 0x38);
  FUN_003b8988(local_30,"profileViewerDialog",
               "Information about the image used to build this noise profile",0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011af50;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011af50:
  uVar2 = *(undefined8 *)(param_1 + 0x38);
  local_38 = &DAT_0142f030;
  FUN_003df0c8(local_30,&local_38);
  FUN_003d5aa0(uVar2,"html",local_30);
  FUN_003dec3c(local_30);
  if (*local_38 != -1) {
    if (*local_38 != 0) {
      LOAcquire();
      iVar1 = *local_38;
      *local_38 = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011afbc;
    }
    FUN_00287df8(local_38,2,8);
  }
LAB_0011afbc:
  uVar2 = *(undefined8 *)(param_1 + 0x50);
  FUN_003b8988(local_30,"profileViewerDialog","Main Noise Sample",0,0xffffffff);
  FUN_00808038(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b020;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b020:
  uVar2 = *(undefined8 *)(param_1 + 0xa0);
  FUN_003b8988(local_30,"profileViewerDialog","Clipping:",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b084;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b084:
  uVar2 = *(undefined8 *)(param_1 + 0xa8);
  FUN_003b8988(local_30,"profileViewerDialog","not detected",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b0e8;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b0e8:
  uVar2 = *(undefined8 *)(param_1 + 0xd8);
  FUN_003b8988(local_30,"profileViewerDialog","-",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b14c;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b14c:
  uVar2 = *(undefined8 *)(param_1 + 0xe0);
  FUN_003b8988(local_30,"profileViewerDialog","5%",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b1b0;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b1b0:
  uVar2 = *(undefined8 *)(param_1 + 0xf0);
  FUN_003b8988(local_30,"profileViewerDialog","Cr",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b214;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b214:
  uVar2 = *(undefined8 *)(param_1 + 0xf8);
  FUN_003b8988(local_30,"profileViewerDialog","100%",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b278;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b278:
  uVar2 = *(undefined8 *)(param_1 + 0x108);
  FUN_003b8988(local_30,"profileViewerDialog","Uniformity:   ",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b2dc;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b2dc:
  uVar2 = *(undefined8 *)(param_1 + 0x110);
  FUN_003b8988(local_30,"profileViewerDialog","-",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b340;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b340:
  uVar2 = *(undefined8 *)(param_1 + 0x118);
  FUN_003b8988(local_30,"profileViewerDialog","-",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b3a4;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b3a4:
  uVar2 = *(undefined8 *)(param_1 + 0x120);
  FUN_003b8988(local_30,"profileViewerDialog","70%",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b408;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b408:
  uVar2 = *(undefined8 *)(param_1 + 0x130);
  FUN_003b8988(local_30,"profileViewerDialog","Luminance",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b46c;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b46c:
  uVar2 = *(undefined8 *)(param_1 + 0x138);
  FUN_003b8988(local_30,"profileViewerDialog","Cb",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b4d0;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b4d0:
  uVar2 = *(undefined8 *)(param_1 + 0x160);
  FUN_003b8988(local_30,"profileViewerDialog","Overall Profile Quality",0,0xffffffff);
  FUN_00808038(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b534;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b534:
  uVar2 = *(undefined8 *)(param_1 + 0x170);
  FUN_003b8988(local_30,"profileViewerDialog",
               "Estimation of the overall quality of this noise profile\nTo achieve higher quality:\n- use a uniform image area without any clipping\n- use Fine-Tuning to make analyzis more complete and accurate\n- use Calibration Target (instead of input image) to build the profile"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b598;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b598:
  uVar2 = *(undefined8 *)(param_1 + 0x170);
  FUN_003b8988(local_30,"profileViewerDialog","50%",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b5fc;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b5fc:
  uVar2 = *(undefined8 *)(param_1 + 0x178);
  FUN_003b8988(local_30,"profileViewerDialog","Profile noise levels",0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b660;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b660:
  uVar2 = *(undefined8 *)(param_1 + 0x178);
  FUN_003b8988(local_30,"profileViewerDialog","Noise Levels",0,0xffffffff);
  FUN_00808038(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b6c4;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b6c4:
  uVar2 = *(undefined8 *)(param_1 + 0x198);
  FUN_003b8988(local_30,"profileViewerDialog","Display values from the base (measured) profile",0,
               0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b728;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b728:
  uVar2 = *(undefined8 *)(param_1 + 0x198);
  FUN_003b8988(local_30,"profileViewerDialog","Base Profile",0,0xffffffff);
  FUN_007d5e4c(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b78c;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b78c:
  uVar2 = *(undefined8 *)(param_1 + 0x1a0);
  FUN_003b8988(local_30,"profileViewerDialog","Display values from the tuned profile",0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b7f0;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b7f0:
  uVar2 = *(undefined8 *)(param_1 + 0x1a0);
  FUN_003b8988(local_30,"profileViewerDialog","Tuned Profile",0,0xffffffff);
  FUN_007d5e4c(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b854;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b854:
  uVar2 = *(undefined8 *)(param_1 + 0x1a8);
  FUN_003b8988(local_30,"profileViewerDialog","Profile noise levels",0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b8b8;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b8b8:
  uVar2 = *(undefined8 *)(param_1 + 0x1a8);
  FUN_003b8988(local_30,"profileViewerDialog","  Profile",0,0xffffffff);
  FUN_00024d78(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b91c;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b91c:
  uVar2 = *(undefined8 *)(param_1 + 0x1c8);
  FUN_003b8988(local_30,"profileViewerDialog","Main Noise Sample",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b980;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b980:
  uVar2 = *(undefined8 *)(param_1 + 0x1d0);
  FUN_003b8988(local_30,"profileViewerDialog",
               "Estimation of the noise level in the main sample\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011b9e4;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011b9e4:
  uVar2 = *(undefined8 *)(param_1 + 0x1d0);
  FUN_003b8988(local_30,"profileViewerDialog","3,95",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011ba48;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011ba48:
  uVar2 = *(undefined8 *)(param_1 + 0x1e0);
  FUN_003b8988(local_30,"profileViewerDialog","Average",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011baac;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011baac:
  uVar2 = *(undefined8 *)(param_1 + 0x1e8);
  FUN_003b8988(local_30,"profileViewerDialog",
               "Estimation of the average noise level\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011bb10;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011bb10:
  uVar2 = *(undefined8 *)(param_1 + 0x1e8);
  FUN_003b8988(local_30,"profileViewerDialog","3,75",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011bb74;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011bb74:
  uVar2 = *(undefined8 *)(param_1 + 0x1f8);
  FUN_003b8988(local_30,"profileViewerDialog","Noise levels in channel components",0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011bbd8;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011bbd8:
  uVar2 = *(undefined8 *)(param_1 + 0x1f8);
  FUN_003b8988(local_30,"profileViewerDialog","  Channel Components",0,0xffffffff);
  FUN_00024d78(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011bc3c;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011bc3c:
  uVar2 = *(undefined8 *)(param_1 + 0x218);
  FUN_003b8988(local_30,"profileViewerDialog","Luminance",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011bca0;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011bca0:
  uVar2 = *(undefined8 *)(param_1 + 0x220);
  FUN_003b8988(local_30,"profileViewerDialog",
               "Estimation of noise level in this channel component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011bd04;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011bd04:
  uVar2 = *(undefined8 *)(param_1 + 0x220);
  FUN_003b8988(local_30,"profileViewerDialog","4,13",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011bd68;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011bd68:
  uVar2 = *(undefined8 *)(param_1 + 0x230);
  FUN_003b8988(local_30,"profileViewerDialog","Cr",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011bdcc;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011bdcc:
  uVar2 = *(undefined8 *)(param_1 + 0x238);
  FUN_003b8988(local_30,"profileViewerDialog",
               "Estimation of noise level in this channel component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011be30;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011be30:
  uVar2 = *(undefined8 *)(param_1 + 0x238);
  FUN_003b8988(local_30,"profileViewerDialog","0,00",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011be94;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011be94:
  uVar2 = *(undefined8 *)(param_1 + 0x248);
  FUN_003b8988(local_30,"profileViewerDialog","Cb",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011bef8;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011bef8:
  uVar2 = *(undefined8 *)(param_1 + 0x250);
  FUN_003b8988(local_30,"profileViewerDialog",
               "Estimation of noise level in this channel component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011bf5c;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011bf5c:
  uVar2 = *(undefined8 *)(param_1 + 0x250);
  FUN_003b8988(local_30,"profileViewerDialog","0,00",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011bfc0;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011bfc0:
  uVar2 = *(undefined8 *)(param_1 + 0x260);
  FUN_003b8988(local_30,"profileViewerDialog","Noise levels in frequency components",0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c024;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c024:
  uVar2 = *(undefined8 *)(param_1 + 0x260);
  FUN_003b8988(local_30,"profileViewerDialog","  Frequency Components",0,0xffffffff);
  FUN_00024d78(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c088;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c088:
  uVar2 = *(undefined8 *)(param_1 + 0x280);
  FUN_003b8988(local_30,"profileViewerDialog","High",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c0ec;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c0ec:
  uVar2 = *(undefined8 *)(param_1 + 0x288);
  FUN_003b8988(local_30,"profileViewerDialog",
               "Estimation of noise level in high frequency component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c150;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c150:
  uVar2 = *(undefined8 *)(param_1 + 0x288);
  FUN_003b8988(local_30,"profileViewerDialog","6,27",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c1b4;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c1b4:
  uVar2 = *(undefined8 *)(param_1 + 0x298);
  FUN_003b8988(local_30,"profileViewerDialog","Mid",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c218;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c218:
  uVar2 = *(undefined8 *)(param_1 + 0x2a0);
  FUN_003b8988(local_30,"profileViewerDialog",
               "Estimation of noise level in medium frequency component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c27c;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c27c:
  uVar2 = *(undefined8 *)(param_1 + 0x2a0);
  FUN_003b8988(local_30,"profileViewerDialog","0,57",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c2e0;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c2e0:
  uVar2 = *(undefined8 *)(param_1 + 0x2b0);
  FUN_003b8988(local_30,"profileViewerDialog","Low",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c344;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c344:
  uVar2 = *(undefined8 *)(param_1 + 0x2b8);
  FUN_003b8988(local_30,"profileViewerDialog",
               "Estimation of noise level in low frequency component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c3a8;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c3a8:
  uVar2 = *(undefined8 *)(param_1 + 0x2b8);
  FUN_003b8988(local_30,"profileViewerDialog","0,22",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c40c;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c40c:
  uVar2 = *(undefined8 *)(param_1 + 0x2c8);
  FUN_003b8988(local_30,"profileViewerDialog","Very Low",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c470;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c470:
  uVar2 = *(undefined8 *)(param_1 + 0x2d0);
  FUN_003b8988(local_30,"profileViewerDialog",
               "Estimation of noise level in very low frequency component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c4d4;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c4d4:
  uVar2 = *(undefined8 *)(param_1 + 0x2d0);
  FUN_003b8988(local_30,"profileViewerDialog","0,09",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c538;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c538:
  uVar2 = *(undefined8 *)(param_1 + 0x2e0);
  FUN_003b8988(local_30,"profileViewerDialog","Ultra Low",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c59c;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c59c:
  uVar2 = *(undefined8 *)(param_1 + 0x2e8);
  FUN_003b8988(local_30,"profileViewerDialog",
               "Estimation of noise level in ultra low frequency component\n- higher values -> more image elements are considered noise\n- lower values  -> fewer image elements are considered noise"
               ,0,0xffffffff);
  FUN_00756efc(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c600;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c600:
  uVar2 = *(undefined8 *)(param_1 + 0x2e8);
  FUN_003b8988(local_30,"profileViewerDialog","0,33",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c664;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c664:
  uVar2 = *(undefined8 *)(param_1 + 0x2f8);
  FUN_003b8988(local_30,"profileViewerDialog","Fine Tuning",0,0xffffffff);
  FUN_00808038(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c6c8;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c6c8:
  uVar2 = *(undefined8 *)(param_1 + 0x318);
  FUN_003b8988(local_30,"profileViewerDialog","Completeness",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) goto LAB_0011c72c;
    }
    FUN_00287df8(local_30[0],2,8);
  }
LAB_0011c72c:
  uVar2 = *(undefined8 *)(param_1 + 800);
  FUN_003b8988(local_30,"profileViewerDialog","55%",0,0xffffffff);
  FUN_00809dc0(uVar2,local_30);
  if (*local_30[0] != -1) {
    if (*local_30[0] != 0) {
      LOAcquire();
      iVar1 = *local_30[0];
      *local_30[0] = iVar1 + -1;
      LORelease();
      if (iVar1 != 1) {
        return;
      }
    }
    FUN_00287df8(local_30[0],2,8);
  }
  return;
}


