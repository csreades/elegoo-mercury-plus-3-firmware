import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.address.*;

// Apply reverse-engineered names to functions and RAM globals (SWM190 wash & cure firmware)
public class Rename extends GhidraScript {
  void fn(long a, String n, String c) throws Exception {
    Address ad = toAddr(a);
    Function f = getFunctionAt(ad);
    if (f == null) { disassemble(ad); f = createFunction(ad, n); }
    if (f != null) { f.setName(n, SourceType.USER_DEFINED); if (c != null) f.setComment(c); }
  }
  void lbl(long a, String n) throws Exception {
    currentProgram.getSymbolTable().createLabel(toAddr(a), n, SourceType.USER_DEFINED);
  }
  public void run() throws Exception {
    fn(0x0c0, "SFC_QSPI_Enable", "Called from Reset if chip ID 0x854012 and SFC not in QSPI mode");
    fn(0x114, "time_to_digits", "secs -> g.digit[0..3] = mm:ss");
    fn(0x138, "delay_ms", "busy-wait, calibrated for 24 MHz");
    fn(0x154, "boot_beep_and_animation", null);
    fn(0x19c, "TIMR2_init_1s", "TIMR2 period = SystemCoreClock (1 s), IRQ on");
    fn(0x1b6, "TIMR0_init_1ms", "TIMR0 period = SystemCoreClock/1000 (1 ms), IRQ on");
    fn(0x1dc, "io_init_buzzer_uv_lid", "PE4 buzzer out, PA8 UV out, PB6 lid in");
    fn(0x212, "main", null);
    fn(0x538, "finish_beep_unused", "not referenced");
    fn(0x584, "TIMR2_IRQ_countdown_1s", "decrements g.time_s while running; finish => 3 beeps, UV off, motor stop");
    fn(0x72c, "TIMR0_IRQ_buttons_1ms", "polls all buttons every 1 ms");
    fn(0x768, "display_refresh_n", "refresh 4-digit display n times (~8 ms each)");
    fn(0x7b4, "leds_init", "PA9 PA10 PA11 PE2 PE3 outputs, all set high");
    fn(0x836, "buttons_init", "PB1..PB5 inputs (no internal pulls)");
    fn(0x88e, "btn_start_read", "PB1");
    fn(0x8cc, "btn_start_process", "short press toggles run if time>0");
    fn(0x970, "btn_plusminus_read", "PB3 = +, PB2 = -");
    fn(0x9f4, "btn_plusminus_process", "+/-30 s, clamp 0..1800 s, auto-repeat after 2 s hold");
    fn(0xbd2, "btn_wash_read", "PB5");
    fn(0xc10, "btn_wash_process", "mode=0 (wash), PA9 low, PA10 high");
    fn(0xcba, "btn_cure_read", "PB4");
    fn(0xcf8, "btn_cure_process", "mode=1 (cure), PA10 low, PA9 high");
    fn(0xdcc, "display_init", "7-seg: segs PB7-PB14, digits PB15 PC2 PC4 PC5");
    fn(0xef6, "motor_io_init", "PD0 PD1 PA12 PA13 PA14 PA15");
    fn(0xf88, "display_select_digit", "bitmask -> PB15,PC2,PC4,PC5 (active low)");
    fn(0xfea, "display_write_segments", "bit7..1 = a,b,c,d,e,f,g -> PB14,PB12,PB10,PB8,PB7,PB13,PB11");
    fn(0x108c, "display_show_pattern", "(digitmask, idx in chase table @0x20000060)");
    fn(0x10a0, "display_show_digit_all", null);
    fn(0x10b8, "display_refresh", "multiplex 4 digits, 2 ms each, blocking");
    fn(0x1190, "boot_animation", "segment chase");
    fn(0x12a4, "motor_pwm_start_wash", "PWM1A on PC3, clkdiv 8, period 30000");
    fn(0x1314, "motor_pwm_start_cure", "PWM1A on PC3, clkdiv 8, period 35156 (slow turntable)");
    fn(0x13d4, "motor_ramp_up_wash", "DIR=PD1 from g.dir; period 1000->700, 600->54");
    fn(0x1464, "motor_ramp_down_stop", "period -> 300 then PWM off");
    fn(0x14ac, "motor_ramp_down_reverse", "period 54 -> 500 then off");
    fn(0x1514, "GPIO_Init", "(GPIOx, pin, dir, pullup, pulldown, opendrain) - Synwit SDK");
    fn(0x1786, "GPIO_SetBit", null);
    fn(0x1792, "GPIO_ClrBit", null);
    fn(0x179e, "GPIO_InvBit", null);
    fn(0x17aa, "GPIO_GetBit", null);
    fn(0x1884, "PORT_Init", "(port idx, pin, func, digital_in_en)");
    fn(0x1a64, "PWM_Init", null);
    fn(0x1dfa, "PWM_Start", null);
    fn(0x1e7a, "PWM_SetPeriod", null);
    fn(0x1e9e, "PWM_SetHDuty", null);
    fn(0x2478, "TIMR_Stop", null);
    fn(0x24f0, "TIMR_Init", null);
    fn(0x2632, "TIMR_Start", null);
    fn(0x2744, "TIMR_INTClr", null);
    fn(0x28ec, "NVIC_EnableIRQ", null);
    fn(0x2900, "SystemCoreClockUpdate", null);
    fn(0x2994, "switchToHRC", null);
    fn(0x2a82, "switchToXTAL_PLL", "PC0/PC1 = XTAL_OUT/XTAL_IN");
    fn(0x2b9c, "SystemInit", null);
    fn(0x2c5c, "__aeabi_uidivmod", null);
    fn(0x2c88, "__aeabi_idivmod", null);
    fn(0x2cb0, "__scatterload", null);
    lbl(0x20000000, "g_mode");          // 0=wash 1=cure 3=finished
    lbl(0x20000001, "g_wash_btn_ack");
    lbl(0x20000002, "g_cure_btn_ack");
    lbl(0x20000003, "g_plusminus_lock");
    lbl(0x20000004, "g_running");
    lbl(0x20000005, "g_dir");
    lbl(0x20000006, "g_wash_rev_done");
    lbl(0x20000007, "g_wash_fwd_done");
    lbl(0x20000008, "g_cure_motor_on");
    lbl(0x2000000c, "g_time_s");
    lbl(0x20000010, "g_time_total_s");
    lbl(0x20000014, "g_digit");         // 4 x int
    lbl(0x20000024, "g_loop_i");
    lbl(0x20000028, "g_motor_period");
    lbl(0x2000002c, "g_btn");           // debounce state
    lbl(0x20000038, "g_segfont");       // 10 x int, active-low
    lbl(0x20000060, "g_segchase");
    lbl(0x2000007c, "PWMX_table");
    lbl(0x2000009c, "SystemCoreClock");
    lbl(0x200000a0, "CyclesPerUs");
  }
}
