// FUN_0008d0e8 size=7428

void FUN_0008d0e8(long *param_1,ulong param_2,ulong param_3,int param_4)

{
  long *plVar1;
  long *plVar2;
  uint uVar3;
  bool bVar4;
  int iVar5;
  int iVar6;
  uint *puVar7;
  uint *puVar8;
  undefined8 uVar9;
  undefined8 uVar10;
  undefined1 uVar11;
  undefined8 *puVar12;
  long lVar13;
  undefined8 uVar14;
  uint uVar15;
  long lVar16;
  long lVar17;
  uint uVar18;
  int *local_c0;
  undefined1 auStack_b8 [8];
  undefined1 auStack_b0 [8];
  undefined1 auStack_a8 [8];
  undefined1 auStack_a0 [8];
  undefined1 auStack_98 [8];
  undefined1 auStack_90 [8];
  undefined1 auStack_88 [8];
  undefined1 auStack_80 [8];
  undefined1 auStack_78 [8];
  undefined1 auStack_70 [8];
  int *local_68;
  
  if ((int)param_3 == -1) {
    param_3 = (ulong)*(uint *)((long)param_1 + 0x1dc);
  }
  uVar18 = *(uint *)(DAT_0141b370 + 0x1ce0808);
  uVar15 = (uVar18 ^ uVar18 >> 1) & 0x44504005;
  uVar18 = (uVar15 | uVar15 << 1) ^ uVar18;
  if (DAT_01566d30 != 0) {
    uVar18 = uVar18 + *(byte *)(DAT_01566d30 + 0xe6c1);
  }
  iVar5 = (**(code **)(*param_1 + 0x210))(param_1);
  if (iVar5 < (int)param_2) {
    param_2 = (ulong)*(uint *)(param_1 + 0x3b);
  }
  iVar5 = (int)param_2;
  if (iVar5 != 0) {
    uVar3 = *(uint *)(DAT_0141b1d8 + 0x450d48);
    uVar15 = (uVar3 ^ uVar3 >> 1) & 0x44551115;
    DAT_0141b1d0 = (uVar15 | uVar15 << 1) ^ uVar3;
    if (DAT_0141b1d0 != 1) {
      *(uint *)(DAT_0141b1e0 + 0x8b6318) = DAT_0141b1d0;
    }
    iVar6 = (**(code **)(*param_1 + 0x210))(param_1);
    if ((iVar6 < iVar5) ||
       ((lVar17 = *(long *)(*(long *)(param_1[0x3a] + 0x108) +
                            *(long *)(*(long *)(param_1[0x3a] + 0x108) + 0xd0) * 8 + 0x58),
        *(char *)(lVar17 + 0xe0) != '\0' &&
        ((*(char *)(lVar17 + 0xf4) != '\0' && iVar5 != 1) && iVar5 != 4)))) {
      param_2 = (**(code **)(*param_1 + 0x218))(param_1);
    }
  }
  uVar15 = (uint)param_2;
  FUN_000f63dc(param_1[0x36],uVar15 != 0);
  iVar5 = (**(code **)(*(long *)param_1[0x3a] + 0x1d0))((long *)param_1[0x3a],param_1);
  if (iVar5 != 0) {
    FUN_0001006c(*(undefined8 *)(*(long *)(param_1[0x3a] + 0x118) + 0x118),1,uVar15 != 0);
    FUN_0001006c(*(undefined8 *)(*(long *)(param_1[0x3a] + 0x118) + 0x118),2,uVar15 != 0);
  }
  if (((uVar15 == *(uint *)(param_1 + 0x3b)) && ((int)param_3 == *(int *)((long)param_1 + 0x1dc)))
     && (param_4 == 0)) {
    return;
  }
  plVar2 = param_1 + 0x3b;
  if (uVar18 == 0) {
    iVar5 = 0;
  }
  else {
    uVar18 = *(uint *)(BYTE_ARRAY_ARRAY_015b2719[0x699] + DAT_0141b380 + 0xf);
    if (DAT_0141b378 != 0) {
      uVar18 = uVar18 >> 0x13 | uVar18 << 0xd;
    }
    iVar5 = (int)(CONCAT44((uVar18 + (uVar18 >> 0x10)) * 0x101 + (uVar18 >> 0x18),uVar18) >> 0x10);
    if (iVar5 != 1) {
      *(uint *)(DAT_0141b388 + 0x1716818) = uVar18;
    }
    if (DAT_015687b0 != 0) {
      iVar5 = iVar5 + (uint)*(byte *)(DAT_015687b0 + 0xefb1);
    }
  }
  uVar18 = *(uint *)(param_1 + 0x3b);
  param_1[0x3b] = param_2 & 0xffffffff | param_3 << 0x20;
  if (((uVar18 != 0) && (uVar18 != uVar15)) &&
     ((uVar15 != 0 && (*(int *)((long)param_1 + 0x1dc) == (int)param_3)))) {
    if ((uVar15 & 0xfffffffe) == 2) {
      plVar1 = param_1 + 0x35;
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      uVar14 = *(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 8);
      uVar9 = FUN_00089bf0(puVar7,plVar2,1);
      uVar10 = (**(code **)(*param_1 + 0x220))(param_1);
      FUN_00055288(uVar14,uVar9,uVar10);
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      uVar14 = *(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 0x10);
      uVar9 = FUN_00089bf0(puVar7,plVar2,2);
      uVar10 = (**(code **)(*param_1 + 0x220))(param_1);
      FUN_00055288(uVar14,uVar9,uVar10);
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      uVar14 = *(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 0x18);
      uVar9 = FUN_00089bf0(puVar7,plVar2,3);
      uVar10 = (**(code **)(*param_1 + 0x220))(param_1);
      FUN_00055288(uVar14,uVar9,uVar10);
      param_2 = (ulong)*(uint *)(param_1 + 0x3b);
      param_3 = (ulong)*(uint *)((long)param_1 + 0x1dc);
    }
    FUN_001d5188(param_1,param_2,param_3);
    return;
  }
  FUN_00752f98(param_1[0x12],0);
  plVar1 = param_1 + 0x35;
  puVar8 = (uint *)param_1[0x35];
  puVar7 = puVar8 + 1;
  uVar18 = *puVar7;
  if (0 < (int)uVar18) {
    lVar17 = 0;
    do {
      if (1 < *puVar8) {
        if ((puVar8[2] & 0x7fffffff) == 0) {
          puVar8 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar8;
        }
        else {
          FUN_000918e8(plVar1,puVar8[2] & 0x7fffffff,0);
          puVar8 = (uint *)*plVar1;
        }
      }
      FUN_0008f0e4(*(undefined8 *)((long)puVar8 + lVar17 * 8 + *(long *)(puVar8 + 4)));
      if (lVar17 != 0) {
        puVar7 = (uint *)*plVar1;
        if (1 < *puVar7) {
          if ((puVar7[2] & 0x7fffffff) == 0) {
            puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
            *plVar1 = (long)puVar7;
          }
          else {
            FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
            puVar7 = (uint *)*plVar1;
          }
        }
        FUN_00753798(*(undefined8 *)((long)puVar7 + lVar17 * 8 + *(long *)(puVar7 + 4)));
        lVar16 = param_1[0x1d];
        if (lVar16 != 0) {
          puVar7 = (uint *)*plVar1;
          if (1 < *puVar7) {
            if ((puVar7[2] & 0x7fffffff) == 0) {
              puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
              *plVar1 = (long)puVar7;
            }
            else {
              FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
              puVar7 = (uint *)*plVar1;
            }
          }
          FUN_0073c4cc(lVar16,*(undefined8 *)((long)puVar7 + lVar17 * 8 + *(long *)(puVar7 + 4)));
        }
        puVar7 = (uint *)*plVar1;
        if (1 < *puVar7) {
          if ((puVar7[2] & 0x7fffffff) == 0) {
            puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
            *plVar1 = (long)puVar7;
          }
          else {
            FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
            puVar7 = (uint *)*plVar1;
          }
        }
        (**(code **)(**(long **)((long)puVar7 + lVar17 * 8 + *(long *)(puVar7 + 4)) + 0x290))();
        puVar7 = (uint *)*plVar1;
        if (1 < *puVar7) {
          if ((puVar7[2] & 0x7fffffff) == 0) {
            puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
            *plVar1 = (long)puVar7;
          }
          else {
            FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
            puVar7 = (uint *)*plVar1;
          }
        }
        FUN_003d0924(*(undefined8 *)((long)puVar7 + lVar17 * 8 + *(long *)(puVar7 + 4)));
      }
      lVar17 = lVar17 + 1;
      puVar8 = (uint *)*plVar1;
      uVar18 = puVar8[1];
    } while (lVar17 < (int)uVar18);
    puVar7 = puVar8 + 1;
  }
  if (iVar5 != 0) {
    uVar15 = *(uint *)(&PNG_00ce5d0a.field_0x1446ee + DAT_0141b398);
    if (DAT_0141b390 != 0) {
      uVar15 = uVar15 >> 0x12 | uVar15 << 0xe;
    }
    DAT_0141b390 = (int)(CONCAT44((uVar15 + (uVar15 >> 0x10)) * 0x101 + (uVar15 >> 0x18),uVar15) >>
                        0x10);
    if (DAT_0141b390 != 1) {
      *(uint *)(DAT_0141b3a0 + 0x1a41888) = uVar15;
      uVar18 = *puVar7;
    }
  }
  if (uVar18 != 0) {
    if (1 < *puVar8) {
      if ((puVar8[2] & 0x7fffffff) == 0) {
        puVar8 = (uint *)FUN_00287b7c(8,8,0,2);
        *plVar1 = (long)puVar8;
      }
      else {
        FUN_000918e8(plVar1,puVar8[2] & 0x7fffffff,0);
        puVar8 = (uint *)*plVar1;
      }
    }
    if (1 < *puVar8) {
      if ((puVar8[2] & 0x7fffffff) == 0) {
        puVar8 = (uint *)FUN_00287b7c(8,8,0,2);
        *plVar1 = (long)puVar8;
      }
      else {
        FUN_000918e8(plVar1,puVar8[2] & 0x7fffffff,0);
        puVar8 = (uint *)*plVar1;
      }
    }
    puVar8[1] = 0;
  }
  if (param_1[0x1d] != 0) {
    FUN_0073c4cc(param_1[0x1d],param_1[0x34]);
    if ((long *)param_1[0x1d] != (long *)0x0) {
      (**(code **)(*(long *)param_1[0x1d] + 0x20))();
    }
  }
  if (param_1[0x34] != 0) {
    FUN_003d2778(param_1[0x34],
                 "2mouseCoordsChanged(Bool, QPoint, QSize, float, float, float, float, QSize, int)",
                 param_1[0x36],
                 "1updateCoordsOrSize(Bool, QPoint, QSize, float, float, float, float, QSize, int)")
    ;
    FUN_003d2778(param_1[0x34],"2customContextMenuRequested(const QPoint&)",param_1,
                 "1onZoomViewContextMenuRequested(const QPoint&)");
  }
  lVar17 = __Znwm(0x20);
  FUN_007387f4();
  param_1[0x1d] = lVar17;
  FUN_007389e4(lVar17,3);
  FUN_0073ab94(param_1[0x1c],1);
  FUN_00733ed8(param_1[0x1c],0);
  FUN_00734fe0(param_1[0x1c],param_1[0x1d],0);
  FUN_00734fc8(param_1[0x1c],param_1[0x36],0,0);
  lVar17 = param_1[0x34];
  *(int *)(lVar17 + 300) = *(int *)(lVar17 + 300) + 1;
  *(undefined4 *)(lVar17 + 0x128) = *(undefined4 *)((long)param_1 + 0x1dc);
  if ((int)param_1[0x3b] == 0) {
    FUN_00090a08(plVar1,1);
    lVar17 = param_1[0x34];
    puVar7 = (uint *)param_1[0x35];
    if (1 < *puVar7) {
      if ((puVar7[2] & 0x7fffffff) == 0) {
        puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
        *plVar1 = (long)puVar7;
      }
      else {
        FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
        puVar7 = (uint *)*plVar1;
      }
    }
    *(long *)((long)puVar7 + *(long *)(puVar7 + 4)) = lVar17;
    uVar11 = 0;
    if (*(long *)(param_1[0x3a] + 0x100) != 0) {
      uVar11 = *(undefined1 *)(*(long *)(param_1[0x3a] + 0x100) + 0x88);
    }
    *(undefined1 *)(param_1[0x34] + 0xd4) = uVar11;
    FUN_0004fe44();
    FUN_00739e08(param_1[0x1d],0,1);
    FUN_00739d3c(param_1[0x1d],0,1);
    FUN_00739868(param_1[0x1d],param_1[0x34],0,0,0);
  }
  else {
    FUN_00090a08(plVar1,4);
    lVar17 = param_1[0x34];
    puVar7 = (uint *)param_1[0x35];
    if (1 < *puVar7) {
      if ((puVar7[2] & 0x7fffffff) == 0) {
        puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
        *plVar1 = (long)puVar7;
      }
      else {
        FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
        puVar7 = (uint *)*plVar1;
      }
    }
    *(long *)((long)puVar7 + *(long *)(puVar7 + 4)) = lVar17;
    iVar5 = *(int *)((long)param_1 + 0x1dc);
    lVar17 = 1;
    do {
      uVar9 = (**(code **)(*param_1 + 0x1a0))(param_1,0,iVar5);
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      *(undefined8 *)((long)puVar7 + lVar17 * 8 + *(long *)(puVar7 + 4)) = uVar9;
      puVar7 = (uint *)*plVar1;
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      lVar16 = *(long *)((long)puVar7 + lVar17 * 8 + *(long *)(puVar7 + 4));
      *(int *)(lVar16 + 300) = *(int *)(lVar16 + 300) + 1;
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      uVar9 = *(undefined8 *)((long)puVar7 + lVar17 * 8 + *(long *)(puVar7 + 4));
      FUN_002f35a0(&local_68,lVar17,10);
      FUN_003cec84(uVar9,&local_68);
      if (*local_68 != -1) {
        if (*local_68 != 0) {
          LOAcquire();
          iVar5 = *local_68;
          *local_68 = iVar5 + -1;
          LORelease();
          if (iVar5 != 1) goto LAB_0008da58;
        }
        FUN_00287df8(local_68,2,8);
      }
LAB_0008da58:
      puVar7 = (uint *)*plVar1;
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      uVar14 = *(undefined8 *)((long)puVar7 + lVar17 * 8 + *(long *)(puVar7 + 4));
      uVar9 = FUN_00089bf0(puVar7,plVar2,lVar17);
      uVar10 = (**(code **)(*param_1 + 0x220))(param_1);
      FUN_00055288(uVar14,uVar9,uVar10);
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      uVar10 = *(undefined8 *)((long)puVar7 + lVar17 * 8 + *(long *)(puVar7 + 4));
      uVar9 = (**(code **)(*(long *)param_1[0x3a] + 0x220))();
      FUN_003d0e8c(auStack_70,uVar10,"2mousePressed(QWidget*, QMouseEvent*, bool&)",uVar9,
                   "1blockAnimation()",0);
      FUN_003d24d8(auStack_70);
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      uVar10 = *(undefined8 *)((long)puVar7 + lVar17 * 8 + *(long *)(puVar7 + 4));
      uVar9 = (**(code **)(*(long *)param_1[0x3a] + 0x220))();
      FUN_003d0e8c(auStack_78,uVar10,"2mouseReleased()",uVar9,"1unBlockAnimation()",0);
      FUN_003d24d8(auStack_78);
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      uVar10 = *(undefined8 *)((long)puVar7 + lVar17 * 8 + *(long *)(puVar7 + 4));
      uVar9 = (**(code **)(*(long *)param_1[0x3a] + 0x220))();
      FUN_003d0e8c(auStack_80,uVar10,"2panningStarted()",uVar9,"1blockAnimation()",0);
      FUN_003d24d8(auStack_80);
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      uVar10 = *(undefined8 *)((long)puVar7 + lVar17 * 8 + *(long *)(puVar7 + 4));
      uVar9 = (**(code **)(*(long *)param_1[0x3a] + 0x220))();
      FUN_003d0e8c(auStack_88,uVar10,"2panningFinished()",uVar9,"1unBlockAnimation()",0);
      FUN_003d24d8(auStack_88);
      lVar17 = lVar17 + 1;
      iVar5 = *(int *)((long)param_1 + 0x1dc);
    } while (lVar17 != 4);
    if (iVar5 == 0) {
      FUN_00739e08(param_1[0x1d],0,1);
      FUN_00739e08(param_1[0x1d],1,1);
      FUN_00739d3c(param_1[0x1d],0,1);
      FUN_00739d3c(param_1[0x1d],1,1);
      FUN_00739868(param_1[0x1d],param_1[0x34],0,0,0);
      lVar17 = param_1[0x1d];
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      FUN_00739868(lVar17,*(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 8),0,1,0);
      lVar17 = param_1[0x1d];
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      FUN_00739868(lVar17,*(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 0x10),1,0,0);
      lVar17 = param_1[0x1d];
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      FUN_00739868(lVar17,*(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 0x18),1,1,0);
      puVar7 = (uint *)*plVar1;
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      *(undefined1 *)(*(long *)((long)puVar7 + *(long *)(puVar7 + 4)) + 0xd4) = 1;
      FUN_0004fe44();
      puVar7 = (uint *)*plVar1;
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      *(undefined1 *)(*(long *)((long)puVar7 + *(long *)(puVar7 + 4) + 8) + 0xd4) = 1;
      FUN_0004fe44();
      puVar7 = (uint *)*plVar1;
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      *(undefined1 *)(*(long *)((long)puVar7 + *(long *)(puVar7 + 4) + 0x10) + 0xd4) = 1;
      FUN_0004fe44();
      puVar7 = (uint *)*plVar1;
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      *(undefined1 *)(*(long *)((long)puVar7 + *(long *)(puVar7 + 4) + 0x18) + 0xd4) = 1;
      FUN_0004fe44();
    }
    else if (iVar5 == 1) {
      FUN_00739e08(param_1[0x1d],0);
      FUN_00739e08(param_1[0x1d],1,1);
      FUN_00739e08(param_1[0x1d],2,1);
      FUN_00739d3c(param_1[0x1d],0,2);
      FUN_00739d3c(param_1[0x1d],1,1);
      FUN_00739b6c(param_1[0x1d],param_1[0x34],0,0,1,3,0);
      lVar17 = param_1[0x1d];
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      FUN_00739868(lVar17,*(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 8),1,0,0);
      lVar17 = param_1[0x1d];
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      FUN_00739868(lVar17,*(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 0x10),1,1,0);
      lVar17 = param_1[0x1d];
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      FUN_00739868(lVar17,*(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 0x18),1,2,0);
      lVar17 = 0;
      do {
        puVar7 = (uint *)*plVar1;
        if (1 < *puVar7) {
          if ((puVar7[2] & 0x7fffffff) == 0) {
            puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
            *plVar1 = (long)puVar7;
          }
          else {
            FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
            puVar7 = (uint *)*plVar1;
          }
        }
        *(undefined1 *)(*(long *)((long)puVar7 + lVar17 + *(long *)(puVar7 + 4)) + 0xd4) = 1;
        FUN_0004fe44();
        puVar7 = (uint *)*plVar1;
        if (1 < *puVar7) {
          if ((puVar7[2] & 0x7fffffff) == 0) {
            puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
            *plVar1 = (long)puVar7;
          }
          else {
            FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
            puVar7 = (uint *)*plVar1;
          }
        }
        FUN_000552f8(*(undefined8 *)((long)puVar7 + lVar17 + *(long *)(puVar7 + 4)),lVar17 == 0);
        lVar17 = lVar17 + 8;
      } while (lVar17 != 0x20);
    }
    else if (iVar5 == 2) {
      FUN_0008ae60(param_1);
      FUN_00739d3c(param_1[0x1d],0,1);
      FUN_00739d3c(param_1[0x1d],1,1);
      FUN_00739d3c(param_1[0x1d],2,1);
      FUN_00739b6c(param_1[0x1d],param_1[0x34],0,0,3,1,0);
      lVar17 = param_1[0x1d];
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      FUN_00739868(lVar17,*(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 8),0,1,0);
      lVar17 = param_1[0x1d];
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      FUN_00739868(lVar17,*(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 0x10),1,1,0);
      lVar17 = param_1[0x1d];
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      FUN_00739868(lVar17,*(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 0x18),2,1,0);
      lVar17 = 0;
      do {
        puVar7 = (uint *)*plVar1;
        if (1 < *puVar7) {
          if ((puVar7[2] & 0x7fffffff) == 0) {
            puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
            *plVar1 = (long)puVar7;
          }
          else {
            FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
            puVar7 = (uint *)*plVar1;
          }
        }
        *(undefined1 *)(*(long *)((long)puVar7 + lVar17 + *(long *)(puVar7 + 4)) + 0xd4) = 1;
        FUN_0004fe44();
        puVar7 = (uint *)*plVar1;
        if (1 < *puVar7) {
          if ((puVar7[2] & 0x7fffffff) == 0) {
            puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
            *plVar1 = (long)puVar7;
          }
          else {
            FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
            puVar7 = (uint *)*plVar1;
          }
        }
        FUN_000552f8(*(undefined8 *)((long)puVar7 + lVar17 + *(long *)(puVar7 + 4)),lVar17 == 0);
        lVar17 = lVar17 + 8;
      } while (lVar17 != 0x20);
    }
    uVar18 = *(uint *)(&PNG_01081549.field_0x2227 + DAT_0141b3b8);
    bVar4 = DAT_0141b3b0 != 0;
    DAT_0141b3b0 = uVar18;
    if (bVar4) {
      DAT_0141b3b0 = uVar18 >> 0xb | uVar18 << 0x15;
    }
    if (DAT_0141b3b0 != 0) {
      *(uint *)(DAT_0141b3c0 + 0xb411e0) = DAT_0141b3b0;
    }
    iVar5 = *(int *)((long)param_1 + 0x1dc);
    if (iVar5 == 0) {
      puVar7 = (uint *)*plVar1;
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      FUN_003d0e8c(auStack_90,*(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 0x10),
                   "2painted(QWidget*)",param_1,"1drawSideButtons(QWidget*)",0);
      FUN_003d24d8(auStack_90);
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      FUN_003d0e8c(auStack_98,*(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 0x10),
                   "2mousePressed(QWidget*, QMouseEvent*, bool&)",param_1,
                   "1onSideButtonPressed(QWidget*, QMouseEvent*, bool&)",0);
      FUN_003d24d8(auStack_98);
      iVar5 = *(int *)((long)param_1 + 0x1dc);
    }
    if (iVar5 == 1) {
      puVar7 = (uint *)*plVar1;
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      FUN_003d0e8c(auStack_a0,*(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 8),
                   "2painted(QWidget*)",param_1,"1drawSideButtons(QWidget*)",0);
      FUN_003d24d8(auStack_a0);
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      FUN_003d0e8c(auStack_a8,*(undefined8 *)((long)puVar7 + *(long *)(puVar7 + 4) + 8),
                   "2mousePressed(QWidget*, QMouseEvent*, bool&)",param_1,
                   "1onSideButtonPressed(QWidget*, QMouseEvent*, bool&)",0);
      FUN_003d24d8(auStack_a8);
    }
  }
  puVar7 = (uint *)*plVar1;
  if (0 < (int)puVar7[1]) {
    lVar17 = 0;
    do {
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      FUN_003d0e8c(auStack_b0,*(undefined8 *)((long)puVar7 + lVar17 * 8 + *(long *)(puVar7 + 4)),
                   "2mouseCoordsChanged(Bool, QPoint, QSize, float, float, float, float, QSize, int)"
                   ,param_1[0x36],
                   "1updateCoordsOrSize(Bool, QPoint, QSize, float, float, float, float, QSize, int)"
                   ,0);
      FUN_003d24d8(auStack_b0);
      puVar7 = (uint *)param_1[0x35];
      if (1 < *puVar7) {
        if ((puVar7[2] & 0x7fffffff) == 0) {
          puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
          *plVar1 = (long)puVar7;
        }
        else {
          FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
          puVar7 = (uint *)*plVar1;
        }
      }
      FUN_003d0e8c(auStack_b8,*(undefined8 *)((long)puVar7 + lVar17 * 8 + *(long *)(puVar7 + 4)),
                   "2customContextMenuRequested(const QPoint&)",param_1,
                   "1onZoomViewContextMenuRequested(const QPoint&)",0);
      FUN_003d24d8(auStack_b8);
      puVar7 = (uint *)param_1[0x35];
      uVar18 = puVar7[1];
      if (0 < (int)uVar18) {
        lVar16 = 0;
        do {
          if (lVar17 != lVar16) {
            if (1 < *puVar7) {
              if ((puVar7[2] & 0x7fffffff) == 0) {
                puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
                *plVar1 = (long)puVar7;
              }
              else {
                FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
                puVar7 = (uint *)*plVar1;
              }
            }
            uVar9 = *(undefined8 *)((long)puVar7 + lVar16 * 8 + *(long *)(puVar7 + 4));
            iVar5 = *(int *)((long)param_1 + 0x1dc);
            if (1 < *puVar7) {
              if ((puVar7[2] & 0x7fffffff) == 0) {
                puVar7 = (uint *)FUN_00287b7c(8,8,0,2);
                *plVar1 = (long)puVar7;
              }
              else {
                FUN_000918e8(plVar1,puVar7[2] & 0x7fffffff,0);
                puVar7 = (uint *)*plVar1;
              }
            }
            lVar13 = *(long *)((long)puVar7 + lVar17 * 8 + *(long *)(puVar7 + 4));
            plVar2 = (long *)(lVar13 + 0x160);
            puVar7 = *(uint **)(lVar13 + 0x160);
            uVar18 = puVar7[1] + 1;
            uVar15 = puVar7[2] & 0x7fffffff;
            if (*puVar7 < 2 && uVar18 <= uVar15) {
              puVar12 = (undefined8 *)
                        ((long)puVar7 + (long)(int)puVar7[1] * 0x10 + *(long *)(puVar7 + 4));
            }
            else {
              uVar3 = uVar18;
              if (uVar18 <= uVar15) {
                uVar3 = uVar15;
              }
              FUN_00091120(plVar2,uVar3,(ulong)(uVar15 < uVar18) << 3);
              lVar13 = *plVar2;
              puVar12 = (undefined8 *)
                        (lVar13 + *(long *)(lVar13 + 0x10) + (long)*(int *)(lVar13 + 4) * 0x10);
            }
            *puVar12 = uVar9;
            *(bool *)(puVar12 + 1) = lVar17 == 0;
            *(bool *)((long)puVar12 + 9) = iVar5 == 0 || (lVar17 == 0) != (lVar16 != 0);
            *(bool *)((long)puVar12 + 10) = lVar16 != 0 || iVar5 == 0;
            *(bool *)((long)puVar12 + 0xb) = lVar17 == 0 && iVar5 != 0;
            *(int *)(*plVar2 + 4) = *(int *)(*plVar2 + 4) + 1;
            puVar7 = (uint *)*plVar1;
          }
          lVar16 = lVar16 + 1;
          uVar18 = puVar7[1];
        } while (lVar16 < (int)uVar18);
      }
      lVar17 = lVar17 + 1;
    } while (lVar17 < (int)uVar18);
  }
  FUN_00752f98(param_1[0x12],1);
  lVar17 = param_1[0x34];
  local_c0 = (int *)param_1[0x35];
  if (*local_c0 == 0) {
    if (local_c0[2] < 0) {
      local_c0 = (int *)FUN_00287b7c(8,8,local_c0[2] & 0x7fffffff,0);
      if (local_c0 == (int *)0x0) {
        FUN_00c78400();
      }
      local_c0[2] = local_c0[2] | 0x80000000;
    }
    else {
      local_c0 = (int *)FUN_00287b7c(8,8,(long)local_c0[1],0);
      if (local_c0 == (int *)0x0) {
        FUN_00c78400();
      }
    }
    if ((local_c0[2] & 0x7fffffffU) != 0) {
      lVar16 = *plVar1;
      _memcpy((void *)((long)local_c0 + *(long *)(local_c0 + 4)),
              (void *)(lVar16 + *(long *)(lVar16 + 0x10)),(long)*(int *)(lVar16 + 4) << 3);
      local_c0[1] = *(int *)(*plVar1 + 4);
    }
  }
  else if (*local_c0 != -1) {
    LOAcquire();
    *local_c0 = *local_c0 + 1;
    LORelease();
    local_c0 = (int *)*plVar1;
  }
  FUN_00054e00(lVar17,0,&local_c0);
  if (*local_c0 != -1) {
    if (*local_c0 != 0) {
      LOAcquire();
      iVar5 = *local_c0;
      *local_c0 = iVar5 + -1;
      LORelease();
      if (iVar5 != 1) goto LAB_0008ed6c;
    }
    FUN_00287df8(local_c0,8,8);
  }
LAB_0008ed6c:
  uVar15 = *(uint *)(DAT_0141b3d8 + 0x804ff0);
  uVar18 = (uVar15 ^ uVar15 >> 1) & 0x45145500;
  uVar15 = (uVar18 | uVar18 << 1) ^ uVar15;
  if (uVar15 != 0) {
    *(uint *)(&UNK_013522a0 + DAT_0141b3e0) = uVar15;
  }
  FUN_001d5188(param_1,(int)param_1[0x3b],*(undefined4 *)((long)param_1 + 0x1dc));
  return;
}


