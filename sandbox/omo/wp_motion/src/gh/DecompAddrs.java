import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.io.*;
public class DecompAddrs extends GhidraScript {
  public void run() throws Exception {
    String[] a = getScriptArgs();
    DecompInterface d = new DecompInterface(); d.openProgram(currentProgram);
    for (int i = 1; i < a.length; i++) {
      Function f = getFunctionContaining(toAddr(Long.parseLong(a[i].replace("0x",""),16)));
      if (f == null) { println("nofunc " + a[i]); continue; }
      DecompileResults r = d.decompileFunction(f, 180, monitor);
      try (PrintWriter w = new PrintWriter(a[0] + "/fn_0x" + Long.toHexString(f.getEntryPoint().getOffset()) + ".c")) {
        w.println("// " + f.getName() + " size=" + f.getBody().getNumAddresses());
        w.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// FAILED");
      }
    }
  }
}
