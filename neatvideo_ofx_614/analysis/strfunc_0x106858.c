// func entry: 00106858 name: FUN_00106858 size: 4824

void FUN_00106858(long *param_1)

{
  char *pcVar1;
  char cVar2;
  int *piVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  long lVar7;
  undefined8 uVar8;
  long lVar9;
  uint *puVar10;
  ulong uVar11;
  int **ppiVar12;
  uint *puVar13;
  uint *puVar14;
  long *plVar15;
  int *local_1d0;
  uint *local_1c8;
  uint *local_1c0;
  uint *local_1b8;
  uint *local_1b0;
  int *local_1a8 [2];
  uint *local_198;
  uint *local_190;
  int *local_188;
  int *local_180;
  ushort local_178 [4];
  undefined1 auStack_170 [8];
  ushort local_168 [4];
  undefined8 local_160;
  undefined8 uStack_158;
  undefined8 local_150;
  undefined8 uStack_148;
  undefined8 local_140;
  int *local_138;
  long local_130;
  long lStack_128;
  undefined8 local_120;
  long local_118;
  long lStack_110;
  undefined8 local_108;
  int **local_100;
  ulong uStack_f8;
  undefined8 local_f0;
  undefined8 uStack_e8;
  ulong local_e0;
  ulong local_d8;
  long local_d0 [2];
  char local_c0;
  undefined4 local_bc;
  char local_b8;
  char local_b4 [20];
  undefined1 auStack_a0 [20];
  uint local_8c;
  char local_88;
  long local_70;
  
  local_70 = *(long *)PTR____stack_chk_guard_013b4320;
  FUN_00137eb4(auStack_a0,DAT_0155b458,param_1 + 0x97,0);
  FUN_001381a8(local_d0,DAT_0155b458,auStack_a0);
  lVar7 = DAT_0156a538;
  local_140 = 0;
  uStack_158 = 0;
  local_160 = 0;
  uStack_148 = 0;
  local_150 = 0;
  local_108 = (uint *)((ulong)local_108._4_4_ << 0x20);
  FUN_009f7b24(auStack_170,&local_108);
  FUN_00a04f40(local_168,lVar7,auStack_170,0,&local_160,"",1);
  FUN_00a057b4(local_178,DAT_0156a538);
  local_108 = (uint *)FUN_009d3278(auStack_a0,0x30,0);
  local_100 = (int **)FUN_009d3278(local_d0,0x30,0);
  uStack_f8 = (ulong)local_168[0];
  uStack_e8 = local_150;
  local_f0 = uStack_158;
  local_e0 = (ulong)local_178[0];
  local_d8 = FUN_005a6458(DAT_0155b2d0 + 0x4e8);
  local_d8 = local_d8 & 0xffffffff;
  lVar7 = FUN_009d326c(&local_108,7);
  uVar8 = 0;
  if (lVar7 == param_1[0x9d]) goto LAB_00107ac0;
  param_1[0x9d] = lVar7;
  *(int *)(param_1 + 0x198) = (int)param_1[0x198] + 1;
  local_180 = (int *)FUN_002efa1c("%1x%2 frames",0xc);
  FUN_002f58f8(&local_108,&local_180,uStack_158,0,10,0x20);
  FUN_002f58f8(&local_188,&local_108,local_150,0,10,0x20);
  if (*local_108 != 0xffffffff) {
    if (*local_108 != 0) {
      LOAcquire();
      uVar5 = *local_108;
      *local_108 = uVar5 - 1;
      LORelease();
      if (uVar5 != 1) goto LAB_00106a1c;
    }
    FUN_00287df8(local_108,2,8);
  }
LAB_00106a1c:
  if (local_c0 == '\0') {
    FUN_007d4e50(param_1[0xa9],1);
    (**(code **)(*(long *)param_1[0xab] + 0x68))((long *)param_1[0xab],1);
    lVar7 = param_1[0xac];
    local_108 = (uint *)FUN_002efa1c("Devices selected manually:",0x1a);
    FUN_00809dc0(lVar7,&local_108);
    if (*local_108 != -1) {
      if (*local_108 != 0) {
        LOAcquire();
        iVar6 = *local_108;
        *local_108 = iVar6 + -1;
        LORelease();
        if (iVar6 != 1) goto LAB_00106bf0;
      }
      FUN_00287df8(local_108,2,8);
    }
LAB_00106bf0:
    FUN_00740f04(param_1[0xad],param_1[0xd1]);
    FUN_007d4e50(param_1[0xd2],local_bc >> 0x10 & 1);
    if (0 < *(long *)(DAT_0156a538 + 0x18)) {
      lVar7 = 0;
      do {
        uVar5 = 1 << (ulong)((uint)lVar7 & 0x1f);
        FUN_007d4e50(param_1[lVar7 + 0xd4],(local_bc & uVar5) != 0);
        FUN_0002e320(param_1[lVar7 + 0x104],(long)local_b4[lVar7]);
        local_120 = &DAT_0142f030;
        if ((local_bc & uVar5) == 0) {
          plVar15 = param_1 + lVar7 + 0x134;
          lVar9 = param_1[lVar7 + 0x134];
LAB_00106cc4:
          FUN_0080a5dc(lVar9);
        }
        else {
          if ((uVar5 & local_168[0]) == 0) {
            plVar15 = param_1 + lVar7 + 0x134;
            lVar9 = param_1[lVar7 + 0x134];
            if ((uVar5 & local_178[0]) == 0) goto LAB_00106cc4;
          }
          else {
            plVar15 = param_1 + lVar7 + 0x134;
            lVar9 = param_1[lVar7 + 0x134];
          }
          FUN_0080a604(lVar9,param_1 + 0x92);
          local_138 = &DAT_0142f030;
          local_1a8[0] = &DAT_0142f030;
          if ((uVar5 & local_168[0]) != 0) {
            FUN_002efbd0(&local_108,
                         "<p>It is not recommended to let Neat Video use this GPU for processing as it has insufficient amount of memory for OpenFX hosts and Neat Video to safely process %1.</p><p>Enabling this GPU here can make OpenFX hosts and Neat Video unstable, unless you "
                         ,0xfc);
            piVar3 = local_138;
            local_138 = (int *)local_108;
            local_108 = (uint *)piVar3;
            if (*piVar3 != -1) {
              if (*piVar3 != 0) {
                LOAcquire();
                iVar6 = *piVar3;
                *piVar3 = iVar6 + -1;
                LORelease();
                if (iVar6 != 1) goto LAB_00106d50;
              }
              FUN_00287df8(piVar3,2,8);
            }
LAB_00106d50:
            iVar6 = FUN_00a04ca4(DAT_0156a538,(int)(char)lVar7);
            if (iVar6 == 0) {
              FUN_002efbd0(&local_108,"reduce the frame size.</p>",0x1a);
              FUN_002e97cc(&local_138,&local_108);
            }
            else {
              FUN_002efbd0(&local_108,"limit the amount of GPU memory used by each of them.</p>",
                           0x38);
              FUN_002e97cc(&local_138,&local_108);
            }
            if (*local_108 != -1) {
              if (*local_108 != 0) {
                LOAcquire();
                iVar6 = *local_108;
                *local_108 = iVar6 + -1;
                LORelease();
                if (iVar6 != 1) goto LAB_00106dd4;
              }
              FUN_00287df8(local_108,2,8);
            }
LAB_00106dd4:
            FUN_002f4ef8(&local_108,&local_138,&local_188,0,0x20);
            piVar3 = local_138;
            local_138 = (int *)local_108;
            local_108 = (uint *)piVar3;
            if (*piVar3 != -1) {
              if (*piVar3 != 0) {
                LOAcquire();
                iVar6 = *piVar3;
                *piVar3 = iVar6 + -1;
                LORelease();
                if (iVar6 != 1) goto LAB_00106e28;
              }
              FUN_00287df8(piVar3,2,8);
            }
          }
LAB_00106e28:
          if ((uVar5 & local_178[0]) != 0) {
            FUN_002efbd0(&local_108,
                         "<p>Neat Video has tried to use this GPU, but encountered a problem. See GPU log in Advanced Settings for details. The use of this GPU has been suspended Neat Video may resume attempts to use this GPU later.</p><p>If you want to permanently disable the use of this GPU by Neat Video, clear the checkbox next to this GPU.</p>"
                         ,0x143);
            piVar3 = local_1a8[0];
            local_1a8[0] = (int *)local_108;
            local_108 = (uint *)piVar3;
            if (*piVar3 != -1) {
              if (*piVar3 != 0) {
                LOAcquire();
                iVar6 = *piVar3;
                *piVar3 = iVar6 + -1;
                LORelease();
                if (iVar6 != 1) goto LAB_00106e84;
              }
              FUN_00287df8(piVar3,2,8);
            }
          }
LAB_00106e84:
          if (local_138[1] == 0) {
            if (local_1a8[0][1] == 0) goto LAB_00106ff8;
            ppiVar12 = local_1a8;
LAB_00106ff4:
            FUN_002e79b0(&local_120,ppiVar12);
          }
          else {
            if (local_1a8[0][1] == 0) {
              ppiVar12 = &local_138;
              goto LAB_00106ff4;
            }
            FUN_002efbd0(&local_1b0,"<ul><li>",8);
            FUN_002e97cc(&local_1b0,&local_138);
            FUN_001c0fe4(&local_198,&local_1b0,"</li><li>");
            local_190 = local_198;
            puVar13 = local_198;
            if (1 < *local_198 + 1) {
              LOAcquire();
              uVar5 = *local_198;
              *local_198 = uVar5 + 1;
              puVar13 = (uint *)(ulong)uVar5;
              LORelease();
            }
            FUN_002e97cc(puVar13,&local_190,local_1a8);
            FUN_001c0fe4(&local_108,&local_190,"</li></ul>");
            FUN_002e79b0(&local_120,&local_108);
            if (*local_108 != -1) {
              if (*local_108 != 0) {
                LOAcquire();
                iVar6 = *local_108;
                *local_108 = iVar6 + -1;
                LORelease();
                if (iVar6 != 1) goto LAB_00106f48;
              }
              FUN_00287df8(local_108,2,8);
            }
LAB_00106f48:
            if (*local_190 != 0xffffffff) {
              if (*local_190 != 0) {
                LOAcquire();
                uVar5 = *local_190;
                *local_190 = uVar5 - 1;
                LORelease();
                if (uVar5 != 1) goto LAB_00106f78;
              }
              FUN_00287df8(local_190,2,8);
            }
LAB_00106f78:
            if (*local_198 != 0xffffffff) {
              if (*local_198 != 0) {
                LOAcquire();
                uVar5 = *local_198;
                *local_198 = uVar5 - 1;
                LORelease();
                if (uVar5 != 1) goto LAB_00106fa8;
              }
              FUN_00287df8(local_198,2,8);
            }
LAB_00106fa8:
            if (*local_1b0 != 0xffffffff) {
              if (*local_1b0 != 0) {
                LOAcquire();
                uVar5 = *local_1b0;
                *local_1b0 = uVar5 - 1;
                LORelease();
                if (uVar5 != 1) goto LAB_00106ff8;
              }
              FUN_00287df8(local_1b0,2,8);
            }
          }
LAB_00106ff8:
          if (*local_1a8[0] != -1) {
            if (*local_1a8[0] != 0) {
              LOAcquire();
              iVar6 = *local_1a8[0];
              *local_1a8[0] = iVar6 + -1;
              LORelease();
              if (iVar6 != 1) goto LAB_00107028;
            }
            FUN_00287df8(local_1a8[0],2,8);
          }
LAB_00107028:
          if (*local_138 != -1) {
            if (*local_138 != 0) {
              LOAcquire();
              iVar6 = *local_138;
              *local_138 = iVar6 + -1;
              LORelease();
              if (iVar6 != 1) goto LAB_00107058;
            }
            FUN_00287df8(local_138,2,8);
          }
        }
LAB_00107058:
        FUN_00756efc(*plVar15,&local_120);
        if (*local_120 != 0xffffffff) {
          if (*local_120 != 0) {
            LOAcquire();
            uVar5 = *local_120;
            *local_120 = uVar5 - 1;
            LORelease();
            if (uVar5 != 1) goto LAB_00106c48;
          }
          FUN_00287df8(local_120,2,8);
        }
LAB_00106c48:
        lVar7 = (long)(char)((char)lVar7 + '\x01');
      } while (lVar7 < *(long *)(DAT_0156a538 + 0x18));
    }
    local_108 = &DAT_0142f0e0;
    local_120 = (uint *)CONCAT44(0xffffffff,(int)DAT_014a67b8);
    FUN_000a6e38(&local_108,&local_120);
    FUN_0002a568(param_1[0xd3],&local_108,(local_bc & 0x10000) == 0);
    if ((local_bc._2_1_ & 1) == 0) {
      FUN_0002a358(param_1[0xd3],DAT_014a67b8,0xffffffff,0);
      lVar7 = param_1[0xd3];
      local_120 = (uint *)FUN_002efa1c("CPU cores are not used for processing",0x25);
      FUN_00756efc(lVar7,&local_120);
    }
    else {
      if (local_d0[0] < 2) {
        local_d0[0] = 1;
      }
      iVar6 = (int)local_d0[0];
      if (DAT_014a67b8 <= iVar6) {
        iVar6 = (int)DAT_014a67b8;
      }
      FUN_0002a358(param_1[0xd3],iVar6 + -1,0xffffffff,0);
      lVar7 = param_1[0xd3];
      local_120 = (uint *)FUN_002efa1c("Select the number of CPU cores to be used for processing",
                                       0x38);
      FUN_00756efc(lVar7,&local_120);
    }
    if (*local_120 != 0xffffffff) {
      if (*local_120 != 0) {
        LOAcquire();
        uVar5 = *local_120;
        *local_120 = uVar5 - 1;
        LORelease();
        if (uVar5 != 1) goto LAB_001079b0;
      }
      FUN_00287df8(local_120,2,8);
    }
LAB_001079b0:
    puVar13 = local_108;
    if (*local_108 != 0xffffffff) {
      if (*local_108 != 0) {
        LOAcquire();
        uVar5 = *local_108;
        *local_108 = uVar5 - 1;
        LORelease();
        if (uVar5 != 1) goto LAB_001079ec;
      }
      uVar5 = local_108[3];
      if (uVar5 != local_108[2]) {
        lVar7 = (long)(int)local_108[2] * 8 + (long)(int)uVar5 * -8;
        puVar14 = local_108 + (long)(int)uVar5 * 2 + 2;
        do {
          if (*(long *)puVar14 != 0) {
            __ZdlPv();
          }
          puVar14 = puVar14 + -2;
          lVar7 = lVar7 + 8;
        } while (lVar7 != 0);
      }
      _free(puVar13);
    }
  }
  else {
    FUN_007d4e50(param_1[0xa8],1);
    (**(code **)(*(long *)param_1[0xab] + 0x68))((long *)param_1[0xab],0);
    lVar7 = param_1[0xac];
    local_108 = (uint *)FUN_002efa1c("Devices currently selected by Neat Video:",0x29);
    FUN_00809dc0(lVar7,&local_108);
    if (*local_108 != 0xffffffff) {
      if (*local_108 != 0) {
        LOAcquire();
        uVar5 = *local_108;
        *local_108 = uVar5 - 1;
        LORelease();
        if (uVar5 != 1) goto LAB_00106a9c;
      }
      FUN_00287df8(local_108,2,8);
    }
LAB_00106a9c:
    FUN_00740f04(param_1[0xad],param_1[0xae]);
    local_198 = &DAT_0142f030;
    local_190 = &DAT_0142f030;
    FUN_00422ae0(local_1a8,&local_190,3);
    if ((local_bc & 0x10000) == 0) {
      FUN_0080a604(param_1[0xaf],param_1 + 0x8e);
      uVar8 = FUN_00423dec(local_1a8,"Not using CPU (");
      pcVar1 = "GPU alone is";
      if ((long)local_b8 + (long)(int)-(local_bc >> 0x10 & 1) != 1) {
        pcVar1 = "GPUs alone are";
      }
      uVar8 = FUN_00423dec(uVar8,pcVar1);
      FUN_00423dec(uVar8," faster)");
      FUN_002efbd0(&local_108,
                   "<p>Neat Video is not using CPU for processing as enabling it reduces speed compared with GPU-only processing.</p>"
                   ,0x71);
      puVar13 = local_198;
      local_198 = local_108;
      local_108 = puVar13;
      if (*puVar13 != 0xffffffff) {
        if (*puVar13 != 0) {
          LOAcquire();
          uVar5 = *puVar13;
          *puVar13 = uVar5 - 1;
          LORelease();
          if (uVar5 != 1) goto LAB_001071fc;
        }
        FUN_00287df8(puVar13,2,8);
      }
    }
    else {
      FUN_0080a604(param_1[0xaf],param_1 + 0x8a);
      uVar8 = FUN_00423dec(local_1a8,"Using ");
      uVar8 = FUN_00423950(uVar8,local_d0[0]);
      uVar8 = FUN_00423dec(uVar8," core");
      pcVar1 = "s";
      if (local_d0[0] < 2) {
        pcVar1 = "";
      }
      FUN_00423dec(uVar8,pcVar1);
      uVar8 = FUN_00423dec(local_1a8," (total ");
      uVar8 = FUN_00423950(uVar8,DAT_014a67b8);
      uVar8 = FUN_00423dec(uVar8," core");
      pcVar1 = "s";
      if (DAT_014a67b8 < 2) {
        pcVar1 = "";
      }
      FUN_00423dec(uVar8,pcVar1);
      if (DAT_014a67b0 != DAT_014a67b8) {
        uVar8 = FUN_00423dec(local_1a8,": ");
        uVar8 = FUN_00423950(uVar8,DAT_014a67b0);
        uVar8 = FUN_00423dec(uVar8," physical, ");
        uVar8 = FUN_00423950(uVar8,DAT_0156a398);
        FUN_00423dec(uVar8," virtual");
      }
      FUN_00423dec(local_1a8,")");
    }
LAB_001071fc:
    FUN_00422d84(local_1a8);
    FUN_00809dc0(param_1[0xb0],&local_190);
    FUN_00756efc(param_1[0xb0],&local_198);
    if (0 < *(long *)(DAT_0156a538 + 0x18)) {
      local_1b0 = &DAT_0142f030;
      if (((long)local_88 + (long)(int)-(local_8c >> 0x10 & 1) < 2) ||
         ((long)local_b8 + (long)(int)-(local_bc >> 0x10 & 1) != 1)) {
LAB_00107304:
        lVar7 = 0;
        do {
          uVar4 = local_bc;
          uVar5 = 1 << (ulong)((uint)lVar7 & 0x1f);
          plVar15 = param_1 + 0x8a;
          if ((local_bc & uVar5) == 0) {
            plVar15 = param_1 + 0x8e;
          }
          FUN_0080a604(param_1[lVar7 + 0xb1],plVar15);
          local_1c0 = &DAT_0142f030;
          local_1b8 = &DAT_0142f030;
          cVar2 = (char)lVar7;
          if ((uVar4 & uVar5) == 0) {
            if ((uVar5 & local_168[0]) == 0) {
              if ((uVar5 & local_178[0]) != 0) {
                FUN_002efbd0(&local_108,"Not using this GPU for processing due to errors",0x2f);
                puVar13 = local_1b8;
                local_1b8 = local_108;
                local_108 = puVar13;
                if (*puVar13 != 0xffffffff) {
                  if (*puVar13 != 0) {
                    LOAcquire();
                    uVar5 = *puVar13;
                    *puVar13 = uVar5 - 1;
                    LORelease();
                    if (uVar5 != 1) goto LAB_001075ec;
                  }
                  FUN_00287df8(puVar13,2,8);
                }
LAB_001075ec:
                FUN_002efbd0(&local_108,
                             "<p>Neat Video has tried to use this GPU, but encountered a problem. See GPU log in Advanced Settings for details. The use of this GPU has been suspended. Neat Video may resume attempts to use this GPU later.</p><p>If you want to permanently disable the use of this GPU by Neat Video, switch to Manual mode and clear the checkbox next to this GPU.</p>"
                             ,0x15e);
                goto LAB_00107600;
              }
              if (local_1b0[1] != 0) {
                FUN_002e79b0(&local_1b8,&local_1b0);
                goto LAB_0010763c;
              }
              FUN_002efbd0(&local_108,"Not using this GPU (it slows processing down)",0x2d);
              puVar13 = local_1b8;
              local_1b8 = local_108;
              local_108 = puVar13;
              if (*puVar13 != 0xffffffff) {
                if (*puVar13 != 0) {
                  LOAcquire();
                  uVar5 = *puVar13;
                  *puVar13 = uVar5 - 1;
                  LORelease();
                  if (uVar5 != 1) goto LAB_00107844;
                }
                FUN_00287df8(puVar13,2,8);
              }
LAB_00107844:
              FUN_002efbd0(&local_108,
                           "<p>Neat Video is not using this GPU for processing as enabling it reduces speed compared with the optimal combination of devices.</p>"
                           ,0x85);
              puVar13 = local_108;
              puVar14 = local_1c0;
              if (*local_1c0 == 0xffffffff) goto LAB_00107760;
              puVar10 = local_1c0;
              if (*local_1c0 == 0) goto LAB_00107754;
              LOAcquire();
              uVar5 = *local_1c0;
              *local_1c0 = uVar5 - 1;
              LORelease();
              puVar13 = local_108;
              puVar10 = local_1c0;
            }
            else {
              FUN_002efbd0(&local_108,"Not using this GPU for processing as it has low memory",0x36)
              ;
              puVar13 = local_1b8;
              local_1b8 = local_108;
              local_108 = puVar13;
              if (*puVar13 != 0xffffffff) {
                if (*puVar13 != 0) {
                  LOAcquire();
                  uVar5 = *puVar13;
                  *puVar13 = uVar5 - 1;
                  LORelease();
                  if (uVar5 != 1) goto LAB_00107460;
                }
                FUN_00287df8(puVar13,2,8);
              }
LAB_00107460:
              FUN_002efbd0(&local_108,
                           "<p>This GPU is not used by Neat Video for processing as it has insufficient amount of memory for OpenFX hosts and Neat Video to safely process %1.</p><p>Neat Video may still use this GPU if you either:</p><ul><li>reduce the frame size</li>"
                           ,0xef);
              puVar13 = local_1c0;
              local_1c0 = local_108;
              local_108 = puVar13;
              if (*puVar13 != 0xffffffff) {
                if (*puVar13 != 0) {
                  LOAcquire();
                  uVar5 = *puVar13;
                  *puVar13 = uVar5 - 1;
                  LORelease();
                  if (uVar5 != 1) goto LAB_001074b0;
                }
                FUN_00287df8(puVar13,2,8);
              }
LAB_001074b0:
              iVar6 = FUN_00a04ca4(DAT_0156a538,(int)cVar2);
              if ((iVar6 != 0) && (iVar6 = FUN_00a04d44(DAT_0156a538,(int)cVar2), iVar6 != 0)) {
                FUN_002efbd0(&local_108,
                             "<li>limit the amount of GPU memory used by OpenFX hosts, other applications, and operating system </li>"
                             ,0x67);
                FUN_002e97cc(&local_1c0,&local_108);
                if (*local_108 != 0xffffffff) {
                  if (*local_108 != 0) {
                    LOAcquire();
                    uVar5 = *local_108;
                    *local_108 = uVar5 - 1;
                    LORelease();
                    if (uVar5 != 1) goto LAB_00107524;
                  }
                  FUN_00287df8(local_108,2,8);
                }
              }
LAB_00107524:
              FUN_002efbd0(&local_108,
                           "<li>let Neat Video use this GPU in Manual mode (proceed with caution).</li></ul>"
                           ,0x50);
              FUN_002e97cc(&local_1c0,&local_108);
              if (*local_108 != 0xffffffff) {
                if (*local_108 != 0) {
                  LOAcquire();
                  uVar5 = *local_108;
                  *local_108 = uVar5 - 1;
                  LORelease();
                  if (uVar5 != 1) goto LAB_00107574;
                }
                FUN_00287df8(local_108,2,8);
              }
LAB_00107574:
              FUN_002f4ef8(&local_108,&local_1c0,&local_188,0,0x20);
LAB_00107600:
              puVar13 = local_1c0;
              local_1c0 = local_108;
              local_108 = puVar13;
              if (*puVar13 != 0xffffffff) {
                if (*puVar13 != 0) {
                  LOAcquire();
                  uVar5 = *puVar13;
                  *puVar13 = uVar5 - 1;
                  LORelease();
                  if (uVar5 != 1) goto LAB_0010763c;
                }
                FUN_00287df8(puVar13,2,8);
              }
LAB_0010763c:
              puVar10 = (uint *)FUN_002efa1c("<span style=\"color: %1;\">%2</span>",0x22);
              FUN_005a631c(&local_1d0,DAT_0155b2d0 + 0x4e8);
              lStack_110 = (long)local_1d0 + *(long *)(local_1d0 + 4);
              local_118 = (long)local_1d0[1];
              local_120 = (uint *)CONCAT71(local_120._1_7_,2);
              lStack_128 = (long)local_1b8 + *(long *)(local_1b8 + 4);
              local_130 = (long)(int)local_1b8[1];
              local_138 = (int *)CONCAT71(local_138._1_7_,2);
              local_108 = (uint *)&local_120;
              local_100 = &local_138;
              uStack_f8 = 0;
              FUN_002f680c(&local_1c8,(long)(int)puVar10[1],(long)puVar10 + *(long *)(puVar10 + 4),2
                           ,&local_108);
              puVar13 = local_1b8;
              local_1b8 = local_1c8;
              local_1c8 = puVar13;
              if (*puVar13 != 0xffffffff) {
                if (*puVar13 != 0) {
                  LOAcquire();
                  uVar5 = *puVar13;
                  *puVar13 = uVar5 - 1;
                  LORelease();
                  if (uVar5 != 1) goto LAB_00107704;
                }
                FUN_00287df8(puVar13,2,8);
              }
LAB_00107704:
              if (*local_1d0 != -1) {
                if (*local_1d0 != 0) {
                  LOAcquire();
                  iVar6 = *local_1d0;
                  *local_1d0 = iVar6 + -1;
                  LORelease();
                  if (iVar6 != 1) goto LAB_00107734;
                }
                FUN_00287df8(local_1d0,2,8);
              }
LAB_00107734:
              puVar13 = local_1c0;
              puVar14 = local_108;
              if (*puVar10 == 0xffffffff) goto LAB_00107760;
              if (*puVar10 == 0) goto LAB_00107754;
              LOAcquire();
              uVar5 = *puVar10;
              *puVar10 = uVar5 - 1;
              LORelease();
              local_1c0 = local_108;
            }
            puVar14 = local_1c0;
            if (uVar5 == 1) goto LAB_00107754;
          }
          else {
            local_120 = (uint *)FUN_002efa1c("Using up to %1% of GPU memory",0x1d);
            FUN_002f58f8(&local_108,&local_120,(long)local_b4[lVar7],0,10,0x20);
            puVar13 = local_1b8;
            local_1b8 = local_108;
            local_108 = puVar13;
            if (*puVar13 != 0xffffffff) {
              if (*puVar13 != 0) {
                LOAcquire();
                uVar5 = *puVar13;
                *puVar13 = uVar5 - 1;
                LORelease();
                if (uVar5 != 1) goto LAB_001073dc;
              }
              FUN_00287df8(puVar13,2,8);
            }
LAB_001073dc:
            puVar13 = local_1c0;
            puVar14 = local_108;
            if (*local_120 != 0xffffffff) {
              puVar10 = local_120;
              if (*local_120 != 0) {
                LOAcquire();
                uVar5 = *local_120;
                *local_120 = uVar5 - 1;
                LORelease();
                if (uVar5 != 1) goto LAB_00107760;
              }
LAB_00107754:
              local_108 = puVar14;
              local_1c0 = puVar13;
              FUN_00287df8(puVar10,2,8);
              puVar13 = local_1c0;
              puVar14 = local_108;
            }
          }
LAB_00107760:
          local_108 = puVar14;
          local_1c0 = puVar13;
          FUN_00809dc0(param_1[lVar7 + 0xc1],&local_1b8);
          FUN_00756efc(param_1[lVar7 + 0xc1],&local_1c0);
          if (*local_1c0 != 0xffffffff) {
            if (*local_1c0 != 0) {
              LOAcquire();
              uVar5 = *local_1c0;
              *local_1c0 = uVar5 - 1;
              LORelease();
              if (uVar5 != 1) goto LAB_001077b0;
            }
            FUN_00287df8(local_1c0,2,8);
          }
LAB_001077b0:
          if (*local_1b8 != 0xffffffff) {
            if (*local_1b8 != 0) {
              LOAcquire();
              uVar5 = *local_1b8;
              *local_1b8 = uVar5 - 1;
              LORelease();
              if (uVar5 != 1) goto LAB_00107328;
            }
            FUN_00287df8(local_1b8,2,8);
          }
LAB_00107328:
          lVar7 = (long)(char)(cVar2 + '\x01');
        } while (lVar7 < *(long *)(DAT_0156a538 + 0x18));
      }
      else {
        local_120 = (uint *)FUN_002efa1c("Only one GPU will be used at a time (limitation of the Demo edition)"
                                         ,0x44);
        FUN_002f98ec(&local_108,&local_120);
        puVar13 = local_1b0;
        local_1b0 = local_108;
        local_108 = puVar13;
        if (*puVar13 != 0xffffffff) {
          if (*puVar13 != 0) {
            LOAcquire();
            uVar5 = *puVar13;
            *puVar13 = uVar5 - 1;
            LORelease();
            if (uVar5 != 1) goto LAB_001072c0;
          }
          FUN_00287df8(puVar13,2,8);
        }
LAB_001072c0:
        if (*local_120 != 0xffffffff) {
          if (*local_120 != 0) {
            LOAcquire();
            uVar5 = *local_120;
            *local_120 = uVar5 - 1;
            LORelease();
            if (uVar5 != 1) goto LAB_001072f4;
          }
          FUN_00287df8(local_120,2,8);
        }
LAB_001072f4:
        if (0 < *(long *)(DAT_0156a538 + 0x18)) goto LAB_00107304;
      }
      if (*local_1b0 != 0xffffffff) {
        if (*local_1b0 != 0) {
          LOAcquire();
          uVar5 = *local_1b0;
          *local_1b0 = uVar5 - 1;
          LORelease();
          if (uVar5 != 1) goto LAB_001078c0;
        }
        FUN_00287df8(local_1b0,2,8);
      }
    }
LAB_001078c0:
    FUN_00422c8c(local_1a8);
    if (*local_198 != 0xffffffff) {
      if (*local_198 != 0) {
        LOAcquire();
        uVar5 = *local_198;
        *local_198 = uVar5 - 1;
        LORelease();
        if (uVar5 != 1) goto LAB_001078fc;
      }
      FUN_00287df8(local_198,2,8);
    }
LAB_001078fc:
    if (*local_190 != 0xffffffff) {
      if (*local_190 != 0) {
        LOAcquire();
        uVar5 = *local_190;
        *local_190 = uVar5 - 1;
        LORelease();
        if (uVar5 != 1) goto LAB_001079ec;
      }
      FUN_00287df8(local_190,2,8);
    }
  }
LAB_001079ec:
  (**(code **)(*param_1 + 0x1e8))(param_1);
  FUN_00105750(param_1);
  FUN_001052d0(param_1);
  uVar11 = FUN_009f7c9c(local_d0,param_1 + 0xa2);
  if ((uVar11 & 1) == 0) {
    FUN_001080d4(param_1);
    FUN_009f7ba4(&local_108);
    FUN_009f7c54(&local_108,local_d0);
    FUN_009f7c54(param_1 + 0xa2,&local_108);
  }
  if (*local_188 != -1) {
    if (*local_188 != 0) {
      LOAcquire();
      iVar6 = *local_188;
      *local_188 = iVar6 + -1;
      LORelease();
      if (iVar6 != 1) goto LAB_00107a7c;
    }
    FUN_00287df8(local_188,2,8);
  }
LAB_00107a7c:
  if (*local_180 != -1) {
    if (*local_180 != 0) {
      LOAcquire();
      iVar6 = *local_180;
      *local_180 = iVar6 + -1;
      LORelease();
      if (iVar6 != 1) goto LAB_00107ab0;
    }
    FUN_00287df8(local_180,2,8);
  }
LAB_00107ab0:
  *(int *)(param_1 + 0x198) = (int)param_1[0x198] + -1;
  uVar8 = 1;
LAB_00107ac0:
  if (*(long *)PTR____stack_chk_guard_013b4320 == local_70) {
    return;
  }
                    /* WARNING: Subroutine does not return */
  ___stack_chk_fail(uVar8);
}


