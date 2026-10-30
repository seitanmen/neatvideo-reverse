import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import java.io.*;
// args: outdir addr1 addr2 ... (function entry addresses). Creates functions if missing, then decompiles.
public class DecompCreate extends GhidraScript {
  public void run() throws Exception {
    String[] a = getScriptArgs();
    DecompInterface d = new DecompInterface(); d.openProgram(currentProgram);
    for (int i = 1; i < a.length; i++) {
      Address ad = toAddr(Long.parseLong(a[i].replace("0x",""),16));
      Function f = getFunctionAt(ad);
      if (f == null) {
        new DisassembleCommand(ad, null, true).applyTo(currentProgram, monitor);
        new CreateFunctionCmd(ad).applyTo(currentProgram, monitor);
        f = getFunctionAt(ad);
      }
      if (f == null) { println("nofunc " + a[i]); continue; }
      DecompileResults r = d.decompileFunction(f, 300, monitor);
      try (PrintWriter w = new PrintWriter(a[0] + "/fn_0x" + Long.toHexString(f.getEntryPoint().getOffset()) + ".c")) {
        w.println("// " + f.getName() + " size=" + f.getBody().getNumAddresses());
        w.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// FAILED " + r.getErrorMessage());
      }
    }
  }
}
