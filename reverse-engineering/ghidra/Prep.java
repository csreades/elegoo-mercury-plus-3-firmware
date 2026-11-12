import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.data.*;
import java.nio.file.*;

// Pre-analysis: SWM190 memory map, peripheral register labels, vector-table entry points
public class Prep extends GhidraScript {
  void blk(String n, long a, long sz, boolean vol) throws Exception {
    MemoryBlock b = currentProgram.getMemory().createUninitializedBlock(n, toAddr(a), sz, false);
    b.setRead(true); b.setWrite(true); b.setVolatile(vol);
  }
  public void run() throws Exception {
    MemoryBlock rom = currentProgram.getMemory().getBlock(toAddr(0));
    rom.setWrite(false); rom.setExecute(true);
    blk("RAM", 0x20000000L, 0x4000, false);
    blk("AHB", 0x40000000L, 0x6000, true);
    blk("APB1", 0x40040000L, 0xC000, true);
    blk("APB2", 0x400A0000L, 0x8000, true);
    blk("SCS", 0xE000E000L, 0x1000, true);
    SymbolTable st = currentProgram.getSymbolTable();
    for (String l : Files.readAllLines(Paths.get(getSourceFile().getParentFile().getParentFile().getAbsolutePath(), "swm190_regs.txt"))) {
      String[] p = l.trim().split(" ");
      Address a = toAddr(Long.parseLong(p[0], 16));
      if (currentProgram.getMemory().getBlock(a) == null) continue;
      st.createLabel(a, p[1], SourceType.USER_DEFINED);
      if (p[2].equals("reg")) {
        int s = Integer.parseInt(p[3]);
        try { createData(a, s == 4 ? new DWordDataType() : s == 2 ? new WordDataType() : new ByteDataType()); } catch (Exception e) {}
      }
    }
    String[] scs = {"e000e010","SysTick_CTRL","e000e014","SysTick_LOAD","e000e018","SysTick_VAL",
                    "e000e100","NVIC_ISER","e000e180","NVIC_ICER","e000e280","NVIC_ICPR",
                    "e000e400","NVIC_IPR0","e000ed0c","SCB_AIRCR","e000ed20","SCB_SHPR3"};
    for (int i = 0; i < scs.length; i += 2)
      st.createLabel(toAddr(Long.parseLong(scs[i], 16)), scs[i+1], SourceType.USER_DEFINED);
    String[] irq = {"GPIOA","GPIOA0","UART0","UART1","SPI0","TIMR0","DMA","PWM0","BTIMR0","BTIMR3","TIMR1","WDT",
      "GPIOA4","I2C0","GPIOA5","BTIMR2","GPIOC_CORDIC","SPI1_BTIMR1","GPIOB_BOD","GPIOC4_ADC0","GPIOD_RTC",
      "XTAL_ACMP","UART2_GPIOB4","TIMR2_GPIOA15","TIMR3_HALL","GPIOC5_I2C1","PWM1_GPIOB6","UART3_GPIOB5",
      "PWM2_FLASH","PWM3_CACHE","ADC1_GPIOA14","GPIOE_PWMHALT"};
    for (int i = 1; i < 48; i++) {
      long v = getInt(toAddr(i * 4)) & 0xffffffffL;
      createDWord(toAddr(i * 4));
      if (v == 0 || v >= 0x3000 || (v & 1) == 0) continue;
      Address a = toAddr(v & ~1L);
      disassemble(a);
      String nm = i == 1 ? "Reset_Handler" : i >= 16 ? irq[i-16] + "_Handler" : "Exc" + i + "_Handler";
      if (getFunctionAt(a) == null) createFunction(a, nm);
    }
  }
}
