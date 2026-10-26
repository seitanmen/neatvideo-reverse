// FUN_00052788 size=648

void FUN_00052788(double param_1,double param_2,long *param_3)

{
  uint uVar1;
  undefined8 uVar2;
  int iVar3;
  undefined8 uVar4;
  undefined8 uVar5;
  long lVar6;
  ulong uVar7;
  ulong uVar8;
  ulong local_58;
  undefined8 local_50;
  undefined8 local_48;
  undefined4 local_40;
  undefined4 local_3c;
  float local_38;
  float local_34;
  
  local_48 = CONCAT44((int)param_2,(int)param_1);
  if ((param_3[0x29] == 0) || (lVar6 = *(long *)(param_3[0x29] + 8), lVar6 == 0)) {
    local_50 = 0xffffffffffffffff;
  }
  else {
    local_50 = CONCAT44((int)*(undefined8 *)(lVar6 + 0xd8) + -1,
                        (int)*(undefined8 *)(lVar6 + 0xd0) + -1);
  }
  local_58 = 0;
  iVar3 = FUN_002ac814(&local_58,&local_48,0);
  if ((iVar3 == 0) || ((char)param_3[0xd] == '\0')) {
    local_48 = 0xffffffffffffffff;
  }
  (**(code **)(*param_3 + 0x230))
            (param_3,&local_48,&local_58,&local_34,&local_38,&local_3c,&local_40);
  uVar1 = DAT_0141a936;
  if (DAT_0141a930 >> 0x10 < 199) {
    uVar8 = (ulong)DAT_0141a936;
    *(undefined **)(DAT_0141a948 + 0x5ed22f8) = &DAT_0141a934;
    if (*(long *)(DAT_0141a940 + 0x32c53e0) != 0) {
      uVar8 = -(ulong)*(byte *)(*(long *)(DAT_0141a940 + 0x32c53e0) + (uVar8 & 0xffff));
    }
    local_58 = CONCAT44(local_58._4_4_,
                        (float)local_58 +
                        (float)(long)(((ulong)(uVar1 >> 0x18) + (uVar8 + (uVar1 >> 0x10 & 0xff)) * 2
                                      ) * 0x3e));
  }
  uVar1 = DAT_0141a956;
  DAT_0141a930 = (uint)(local_38 +
                       (float)(CONCAT44(((DAT_0141a930 >> 0x10) + DAT_0141a930) * 0x101 +
                                        (DAT_0141a930 >> 0x18),DAT_0141a930) >> 0x10 & 0xffffffff));
  if (DAT_0141a950 >> 0x13 < 0x1b) {
    uVar8 = (ulong)DAT_0141a956;
    *(undefined **)(DAT_0141a968 + 0x5ec0728) = &DAT_0141a954;
    if (*(long *)(DAT_0141a960 + 0x3255e00) != 0) {
      uVar8 = -(ulong)*(byte *)(*(long *)(DAT_0141a960 + 0x3255e00) + (uVar8 & 0xffff));
    }
    local_34 = local_34 +
               (float)(long)(((ulong)(uVar1 >> 0x18) + (uVar8 + (uVar1 >> 0x10 & 0xff)) * 2) * 0x2a)
    ;
  }
  DAT_0141a950 = local_48._4_4_ +
                 (int)(CONCAT44((DAT_0141a950 + (DAT_0141a950 >> 0x10)) * 0x101 +
                                (DAT_0141a950 >> 0x18),DAT_0141a950) >> 0x10);
  uVar4 = (**(code **)(*param_3 + 0x210))(param_3);
  uVar2 = local_48;
  uVar5 = (**(code **)(*param_3 + 0x218))(param_3);
  if (param_3[0x29] == 0) {
    uVar7 = 0;
    uVar8 = 0;
  }
  else {
    lVar6 = *(long *)(param_3[0x29] + 8);
    if (lVar6 == 0) {
      uVar8 = 0;
      uVar7 = 0;
    }
    else {
      uVar8 = (ulong)*(uint *)(lVar6 + 0xd0);
      uVar7 = *(long *)(lVar6 + 0xd8) << 0x20;
    }
  }
  FUN_001d1aec(local_58 & 0xffffffff,local_34,local_38,local_3c,param_3,uVar4,uVar2,uVar5,
               uVar7 | uVar8,local_40);
  return;
}


