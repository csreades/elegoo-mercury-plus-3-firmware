import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.io.*;

// Post-analysis: write decompiled C for every function to the file given as script arg
public class Dump extends GhidraScript {
  public void run() throws Exception {
    DecompInterface d = new DecompInterface();
    d.openProgram(currentProgram);
    PrintWriter w = new PrintWriter(new FileWriter(getScriptArgs()[0]));
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
      DecompileResults r = d.decompileFunction(f, 60, monitor);
      w.println("// ===== " + f.getName() + " @ " + f.getEntryPoint());
      w.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// decompile failed");
    }
    w.close();
  }
}
