// FUN_000506f8 size=448

void FUN_000506f8(long param_1)

{
  code *pcVar1;
  int iVar2;
  int *piVar3;
  long lVar4;
  undefined8 *puVar5;
  long *plVar6;
  long *local_60;
  undefined8 uStack_58;
  int *local_48;
  
  if (*(int *)(param_1 + 300) < 1) {
    piVar3 = *(int **)(param_1 + 0x160);
    if (*piVar3 == 0) {
      if (piVar3[2] < 0) {
        piVar3 = (int *)FUN_00287b7c(0x10,8,piVar3[2] & 0x7fffffff,0);
        local_48 = piVar3;
        if (piVar3 == (int *)0x0) {
          FUN_00c78400();
        }
        piVar3[2] = piVar3[2] | 0x80000000;
      }
      else {
        piVar3 = (int *)FUN_00287b7c(0x10,8,(long)piVar3[1],0);
        local_48 = piVar3;
        if (piVar3 == (int *)0x0) {
          FUN_00c78400();
                    /* WARNING: Does not return */
          pcVar1 = (code *)SoftwareBreakpoint(1,0x508b8);
          (*pcVar1)();
        }
      }
      if ((piVar3[2] & 0x7fffffffU) != 0) {
        lVar4 = *(long *)(param_1 + 0x160);
        _memcpy((void *)((long)piVar3 + *(long *)(piVar3 + 4)),
                (void *)(lVar4 + *(long *)(lVar4 + 0x10)),(long)*(int *)(lVar4 + 4) << 4);
        piVar3[1] = *(int *)(*(long *)(param_1 + 0x160) + 4);
      }
    }
    else {
      local_48 = piVar3;
      if (*piVar3 != -1) {
        LOAcquire();
        *piVar3 = *piVar3 + 1;
        LORelease();
        piVar3 = *(int **)(param_1 + 0x160);
        local_48 = piVar3;
      }
    }
    lVar4 = (long)piVar3[1] << 4;
    if (lVar4 != 0) {
      puVar5 = (undefined8 *)((long)piVar3 + *(long *)(piVar3 + 4));
      do {
        uStack_58 = puVar5[1];
        plVar6 = (long *)*puVar5;
        local_60 = plVar6;
        if (*(int *)((long)plVar6 + 300) < 1) {
          *(int *)((long)plVar6 + 300) = *(int *)((long)plVar6 + 300) + 1;
          (**(code **)(*plVar6 + 0x238))(plVar6,param_1,&local_60);
          (**(code **)(*plVar6 + 0x1e8))(plVar6);
          *(int *)((long)plVar6 + 300) = *(int *)((long)plVar6 + 300) + -1;
        }
        puVar5 = puVar5 + 2;
        lVar4 = lVar4 + -0x10;
      } while (lVar4 != 0);
    }
    if (*piVar3 != -1) {
      if (*piVar3 != 0) {
        LOAcquire();
        iVar2 = *piVar3;
        *piVar3 = iVar2 + -1;
        LORelease();
        piVar3 = local_48;
        if (iVar2 != 1) {
          return;
        }
      }
      FUN_00287df8(piVar3,0x10,8);
    }
  }
  return;
}


