/*
 * Wash & Cure replacement firmware for the Synwit SWM190CBT7-50.
 *
 * Normal operation reproduces the original firmware (reverse-engineered, see docs/hardware.md):
 * same motor PWM settings and ramp shapes, wash direction reversal at half time, cure lid
 * interlock + alarm, button behaviour, LEDs and beeps. Changes requested:
 *   - power-on in CURE mode, default 10:00; selecting WASH loads 20:00, CURE loads 10:00
 *   - after a cycle finishes, the mode stays selected and its default time is reloaded
 * Unlike the original everything is non-blocking (1 ms SysTick), so the UART console on
 * UART1 (PE7 TX / PE5 RX = PCB "RX"/"TX" pads, 115200 8N1) is always responsive.
 *
 * Console: line based, "HELP" lists commands. Replies "OK ..." / "ERR ...",
 * async events "EVT ...", periodic status with STREAM.
 *
 * Timing note: the original used busy-wait loops calibrated for 24 MHz but ran at 60 MHz,
 * so its "1 ms" was ~0.415 ms. Durations below are the real-time equivalents.
 */
#include "SWM190.h"
#include <string.h>
#include <stdlib.h>

#define FW_VERSION "WNC-FW 1.1"

/* ======================================================================= pins */
typedef struct { GPIO_TypeDef *port; uint8_t pin; } pin_t;
#define PIN(p, n) { p, n }
static const pin_t SEG[7]   = { PIN(GPIOB,14), PIN(GPIOB,12), PIN(GPIOB,10), PIN(GPIOB,8),
                                PIN(GPIOB,7),  PIN(GPIOB,13), PIN(GPIOB,11) };   /* a..g, low = lit */
static const pin_t COLON    = PIN(GPIOB,9);                                      /* low = lit */
static const pin_t DIGIT[4] = { PIN(GPIOB,15), PIN(GPIOC,2), PIN(GPIOC,4), PIN(GPIOC,5) }; /* low = on */
/* LEDs (low = lit): 0 WASH(PA9) 1 CURE(PA10) 2 PLUS(PA11) 3 MINUS(PE2) 4 START(PE3) */
enum { L_WASH, L_CURE, L_PLUS, L_MINUS, L_START, NLED };
static const pin_t LED[NLED] = { PIN(GPIOA,9), PIN(GPIOA,10), PIN(GPIOA,11), PIN(GPIOE,2), PIN(GPIOE,3) };
static const char *const LED_NAME[NLED] = { "WASH", "CURE", "PLUS", "MINUS", "START" };
/* inputs: buttons low = pressed, lid 1 = open */
enum { B_START, B_MINUS, B_PLUS, B_CURE, B_WASH, B_LID, NBTN };
static const pin_t BTN[NBTN] = { PIN(GPIOB,1), PIN(GPIOB,2), PIN(GPIOB,3), PIN(GPIOB,4), PIN(GPIOB,5), PIN(GPIOB,6) };
static const char *const BTN_NAME[NBTN] = { "START", "MINUS", "PLUS", "CURE", "WASH", "LID" };
static const pin_t BUZZER = PIN(GPIOE,4);     /* high = on */
static const pin_t UV     = PIN(GPIOA,8);     /* high = on */
static const pin_t M_DIR  = PIN(GPIOD,1);
static const pin_t M_NEN  = PIN(GPIOA,15);    /* low = driver enabled */
static const pin_t M_A12  = PIN(GPIOA,12);    /* original: always high */
static const pin_t M_A13  = PIN(GPIOA,13);    /* original: always low */
static const pin_t M_A14  = PIN(GPIOA,14);    /* original: high in wash, low in cure */
static const pin_t M_D0   = PIN(GPIOD,0);     /* original: always high */

static inline void pin_set(pin_t p, uint32_t v) { (&p.port->DATAPIN0)[p.pin] = v ? 1 : 0; }
static inline uint32_t pin_out(pin_t p)         { return (p.port->ODR >> p.pin) & 1; }
static inline uint32_t pin_in(pin_t p)          { return (p.port->IDR >> p.pin) & 1; }
static void out_init(pin_t p, uint32_t level) { pin_set(p, level); GPIO_Init(p.port, p.pin, 1, 0, 0, 0); pin_set(p, level); }

/* ======================================================================= settings (flash) */
#define SETTINGS_OFF   0x1C000u           /* 4 KB sector, well clear of the image */
#define SETTINGS_MAGIC 0x57434E32u        /* "WCN2" (v1.1: reversal settings added) */
typedef struct {
    uint32_t magic;
    uint32_t wash_default_s, cure_default_s;
    uint32_t wash_period;                 /* PWM period held at wash speed (original 55) */
    uint32_t cure_period;                 /* PWM period for cure turntable (original 35156) */
    uint32_t cure_dir;                    /* original 0 */
    uint32_t steps_per_rev;               /* for rpm display; calibrate with CFG SPR */
    uint32_t key_beeps;                   /* 1 = beep on button press */
    uint32_t rev_down_ms;                 /* wash reversal: slow-down time */
    uint32_t rev_dwell_ms;                /* wash reversal: pause at standstill (liquid settles) */
    uint32_t rev_up_ms;                   /* wash reversal: speed-up time (even acceleration) */
    uint32_t crc;
} settings_t;
static settings_t cfg;
static const settings_t CFG_DEFAULT = { SETTINGS_MAGIC, 1200, 600, 55, 35156, 0, 3200, 1, 3000, 2500, 8000, 0 };

/* ======================================================================= shared state */
enum { MODE_WASH = 0, MODE_CURE = 1 };
static volatile uint32_t ms;
static volatile uint8_t  disp[4];         /* segment masks (bit0=a) */
static volatile uint8_t  colon_on = 1;
static volatile uint8_t  blank;           /* blank display (alarm/finish flashes) */
static volatile uint16_t beep_ms;         /* timed beep */
static volatile uint8_t  buz_manual;      /* manual buzzer level */
static volatile uint8_t  btn_level[NBTN]; /* debounced raw level */
static volatile uint8_t  btn_edge[NBTN];  /* set by ISR on debounced change */
static volatile uint32_t btn_down_ms[NBTN];

static volatile uint8_t  mode = MODE_CURE, running;
static volatile int32_t  time_s, total_s;
static volatile uint8_t  timer_paused;    /* countdown paused (cure lid open) */
static volatile uint8_t  finished_flag;
static uint8_t manual, interlock = 1, buttons_locked;
static uint32_t disp_override_until;      /* text overlay on display */
static uint32_t stream_ms, stream_t;

/* ======================================================================= console UART (UART1) */
#define CON UART1
static volatile uint8_t rxb[256]; static volatile uint16_t rx_h, rx_t;
static volatile uint8_t txb[512]; static volatile uint16_t tx_h, tx_t;

void UART1_Handler(void) {
    while (!UART_IsRXFIFOEmpty(CON)) {
        uint32_t c; UART_ReadByte(CON, &c);
        uint16_t n = (rx_h + 1) & 255;
        if (n != rx_t) { rxb[rx_h] = c; rx_h = n; }
    }
    UART_INTClr(CON, UART_IT_RX_TOUT);
    while (tx_t != tx_h && !UART_IsTXFIFOFull(CON)) { UART_WriteByte(CON, txb[tx_t]); tx_t = (tx_t + 1) & 511; }
    if (tx_t == tx_h) UART_INTDis(CON, UART_IT_TX_THR);
}
static void con_putc(char c) {
    uint16_t n = (tx_h + 1) & 511;
    while (n == tx_t) { /* buffer full: wait for ISR to drain */ }
    txb[tx_h] = c; tx_h = n;
    UART_INTEn(CON, UART_IT_TX_THR);
}
static void con_puts(const char *s) { while (*s) con_putc(*s++); }
static void con_putu(uint32_t v) { char b[11]; int i = 10; b[i] = 0; do { b[--i] = '0' + v % 10; v /= 10; } while (v); con_puts(&b[i]); }
static void con_puti(int32_t v) { if (v < 0) { con_putc('-'); v = -v; } con_putu(v); }
static void con_puthex(uint32_t v, int digits) { for (int i = digits - 1; i >= 0; i--) con_putc("0123456789ABCDEF"[(v >> (4 * i)) & 15]); }
static void con_nl(void) { con_puts("\r\n"); }
static void con_init(void) {
    UART_InitStructure u; memset(&u, 0, sizeof u);
    PORT_Init(PORTE, PIN5, PORTE_PIN5_UART1_RX, 1);
    PORT_Init(PORTE, PIN7, PORTE_PIN7_UART1_TX, 0);
    u.Baudrate = 115200; u.DataBits = UART_DATA_8BIT; u.Parity = UART_PARITY_NONE; u.StopBits = UART_STOP_1BIT;
    u.RXThreshold = 3; u.RXThresholdIEn = 1; u.TXThreshold = 3; u.TXThresholdIEn = 0;
    u.TimeoutTime = 10; u.TimeoutIEn = 1;
    UART_Init(CON, &u);
    NVIC_SetPriority(UART1_IRQn, 0);       /* above SysTick: never lose RX bytes */
    NVIC_EnableIRQ(UART1_IRQn);
    UART_Open(CON);
}

/* ======================================================================= 7-segment */
static uint8_t glyph(char c) {
    switch (c) {
    case '0': case 'O': case 'o': return 0x3F; case '1': case 'I': case 'i': return 0x06;
    case '2': case 'Z': case 'z': return 0x5B; case '3': return 0x4F; case '4': return 0x66;
    case '5': case 'S': case 's': return 0x6D; case '6': return 0x7D; case '7': return 0x07;
    case '8': case 'B': return 0x7F; case '9': case 'g': return 0x6F;
    case 'A': case 'a': return 0x77; case 'b': return 0x7C; case 'C': return 0x39; case 'c': return 0x58;
    case 'D': case 'd': return 0x5E; case 'E': case 'e': return 0x79; case 'F': case 'f': return 0x71;
    case 'G': return 0x3D; case 'H': return 0x76; case 'h': return 0x74; case 'J': case 'j': return 0x1E;
    case 'L': case 'l': return 0x38; case 'n': case 'N': return 0x54; case 'P': case 'p': return 0x73;
    case 'q': case 'Q': return 0x67; case 'r': case 'R': return 0x50; case 't': case 'T': return 0x78;
    case 'U': case 'V': return 0x3E; case 'u': case 'v': return 0x1C; case 'y': case 'Y': return 0x6E;
    case '-': return 0x40; case '_': return 0x08; default: return 0x00;
    }
}
static void disp_text(const char *t) { int i = 0; for (; i < 4 && t[i]; i++) disp[i] = glyph(t[i]); for (; i < 4; i++) disp[i] = 0; }
static void disp_time(int32_t s) {
    if (s < 0) s = 0;
    if (s > 5999) s = 5999;
    int m = s / 60, ss = s % 60;
    disp[0] = glyph('0' + m / 10); disp[1] = glyph('0' + m % 10);
    disp[2] = glyph('0' + ss / 10); disp[3] = glyph('0' + ss % 10);
}

/* ======================================================================= tone generator (TIMR1) */
static volatile uint32_t tone_until;
static volatile uint8_t  tone_on;
void TIMR1_Handler(void) {
    TIMR_INTClr(TIMR1);
    if (tone_on) pin_set(BUZZER, !pin_out(BUZZER));
}
static void tone_start(uint32_t hz, uint32_t dur) {
    if (hz < 20) hz = 20;
    if (hz > 20000) hz = 20000;
    TIMR_Stop(TIMR1);
    TIMR_Init(TIMR1, TIMR_MODE_TIMER, 1, SystemCoreClock / (2 * hz), 1);
    tone_until = ms + dur; tone_on = 1;
    TIMR_Start(TIMR1);
}
static void tone_stop(void) { tone_on = 0; TIMR_Stop(TIMR1); }

/* ======================================================================= motor engine
 * All PWM register writes happen in the SysTick ISR. Main requests work via mot_req.
 * Ramps step every RAMP_STEP_MS, matching the original's ramp-per-display-refresh rate
 * (2 refreshes x 4 digits x delay_ms(2) at 0.415 ms -> ~7 ms).
 */
#define RAMP_STEP_MS 7
#define WASH_INIT_CYCLE 30000u
enum { MP_IDLE, MP_HOLD, MP_WASH_UP, MP_STOP, MP_REV_DOWN, MP_TO_TARGET, MP_REV_DWELL, MP_REV_UP };
enum { RQ_NONE, RQ_WASH_START, RQ_WASH_REVERSE, RQ_STOP_RAMP, RQ_OFF_NOW, RQ_CONST, RQ_TO_TARGET };
static volatile struct { uint8_t type, dir; uint32_t period; } mot_req;
static volatile uint8_t  mp = MP_IDLE;     /* current programme */
static volatile uint32_t m_per;            /* original g_motor_period semantics */
static volatile uint32_t m_applied;        /* PWM period currently applied (0 = off) */
static volatile uint32_t m_hold;           /* wash hold period / generic target */
static volatile uint8_t  m_next_dir, m_chain;

static void pwm_apply(uint32_t p) { PWM_SetCycle(PWM1A, p); PWM_SetHDuty(PWM1A, p >> 1); m_applied = p; }
static void pwm_hw_init(uint32_t cycle) {   /* = original motor_pwm_start_{wash,cure} */
    PWM_InitStructure s; memset(&s, 0, sizeof s);
    s.clkdiv = PWM_CLKDIV_8; s.cycle = cycle; s.hduty = cycle >> 1; s.initLevel = 1;
    PWM_Init(PWM1A, &s);
    PORT_Init(PORTC, PIN3, PORTC_PIN3_PWM1A, 0);
    PWM_Start(PWM1A);
    m_applied = cycle;
}
static void wash_up_begin(uint8_t dir) {
    pwm_hw_init(WASH_INIT_CYCLE);
    pwm_apply(0);
    pin_set(M_DIR, dir);                   /* original ramp_up sets DIR first */
    m_per = 1000; mp = MP_WASH_UP;
}
/* v1.1 wash reversal. The stock sequence (period 54 -> 499, cut, immediately ramp up reversed
 * with a period-linear ramp whose speed doubles in the last 0.4 s) stalls the motor against
 * liquid still swirling the old way. Now: slow down evenly to a crawl, stop, wait for the
 * liquid, then speed up with constant acceleration. Speed is tracked as SPD_K/period in Q8. */
#define SPD_K        60000u
#define REV_STOP_PER 1500u                 /* slow down to this period before cutting the steps */
#define REV_START_PER 1000u                /* reversed ramp starts here (same as stock) */
static volatile uint32_t m_spd, m_spd_end, m_spd_step;   /* Q8 */
static volatile uint32_t m_dwell;          /* ramp ticks left at standstill */
static uint32_t spd_q8(uint32_t per) { return per ? (SPD_K << 8) / per : 0; }
static uint32_t per_of(uint32_t q8)  { return q8 ? ((SPD_K << 8) + q8 / 2) / q8 : REV_STOP_PER; }
static uint32_t spd_step(uint32_t span, uint32_t dur_ms) {
    uint32_t n = dur_ms / RAMP_STEP_MS; if (n < 1) n = 1;
    uint32_t st = span / n; return st ? st : 1;
}
static void rev_up_begin(uint8_t dir) {
    uint32_t hold = m_hold ? m_hold : 55;
    pwm_hw_init(WASH_INIT_CYCLE);
    pwm_apply(0);
    pin_set(M_DIR, dir);
    if (hold >= REV_START_PER) { pwm_apply(hold); m_per = hold; mp = MP_HOLD; return; }
    m_spd = spd_q8(REV_START_PER); m_spd_end = spd_q8(hold);
    m_spd_step = spd_step(m_spd_end - m_spd, cfg.rev_up_ms);
    m_per = REV_START_PER; mp = MP_REV_UP;
}
static void motor_service_request(void) {
    uint8_t t = mot_req.type;
    if (t == RQ_NONE) return;
    mot_req.type = RQ_NONE;
    switch (t) {
    case RQ_WASH_START:   wash_up_begin(mot_req.dir); break;
    case RQ_WASH_REVERSE:
        m_next_dir = mot_req.dir;
        if (m_applied && m_applied < REV_STOP_PER) {             /* turning: slow down first */
            m_spd = spd_q8(m_applied); m_spd_end = spd_q8(REV_STOP_PER);
            m_spd_step = spd_step(m_spd - m_spd_end, cfg.rev_down_ms);
            m_per = m_applied; mp = MP_REV_DOWN; m_chain = 1;
        } else { pwm_apply(0); m_per = 0; rev_up_begin(m_next_dir); }
        break;
    case RQ_STOP_RAMP:    m_chain = 0; if (m_per) mp = MP_STOP; else { pwm_apply(0); mp = MP_IDLE; } break;
    case RQ_OFF_NOW:      m_chain = 0; pwm_apply(0); m_per = 0; mp = MP_IDLE; break;
    case RQ_CONST:        m_chain = 0; pin_set(M_DIR, mot_req.dir); pwm_hw_init(mot_req.period); m_per = 0; mp = MP_HOLD; break;
    case RQ_TO_TARGET:
        m_chain = 0; pin_set(M_DIR, mot_req.dir); m_hold = mot_req.period;
        if (m_applied == 0) { uint32_t start = m_hold < 1000 ? 1000 : m_hold; pwm_hw_init(start); m_per = start; }
        else m_per = m_applied;
        mp = MP_TO_TARGET;
        break;
    }
}
static void motor_tick(void) {             /* every RAMP_STEP_MS */
    switch (mp) {
    case MP_WASH_UP:                       /* 1000 -> 701, then 600 -> 55 (original) */
        if (m_per > 700 && m_hold < 700) { pwm_apply(m_per); m_per--; }
        else if (m_per == 700) m_per = 600;
        else if (m_per >= m_hold) { pwm_apply(m_per); m_per--; }   /* last applied = m_hold (55) */
        else { if (m_applied != m_hold) pwm_apply(m_hold); mp = MP_HOLD; }
        break;
    case MP_STOP:                          /* -> 299 then off */
        if (m_per < 300) { pwm_apply(m_per); m_per++; }
        else { pwm_apply(0); m_per = 0; mp = MP_IDLE; }
        break;
    case MP_REV_DOWN:                      /* even slow-down to a crawl, off, then dwell */
        if (m_spd > m_spd_end + m_spd_step) { m_spd -= m_spd_step; m_per = per_of(m_spd); pwm_apply(m_per); }
        else {
            pwm_apply(0); m_per = 0;
            if (m_chain) { m_chain = 0; m_dwell = cfg.rev_dwell_ms / RAMP_STEP_MS; mp = MP_REV_DWELL; }
            else mp = MP_IDLE;
        }
        break;
    case MP_REV_DWELL:                     /* standstill (driver still holding) while the liquid slows */
        if (m_dwell) m_dwell--; else rev_up_begin(m_next_dir);
        break;
    case MP_REV_UP: {                      /* constant acceleration up to the wash speed */
        uint32_t hold = m_hold ? m_hold : 55;
        if (m_spd + m_spd_step < m_spd_end) {
            m_spd += m_spd_step; m_per = per_of(m_spd);
            if (m_per < hold) m_per = hold;
            pwm_apply(m_per);
        } else { pwm_apply(hold); m_per = hold; mp = MP_HOLD; }
        break;
    }
    case MP_TO_TARGET:
        if (m_per == m_hold || (m_per >= 1000 && m_hold >= 1000)) { pwm_apply(m_hold); m_per = m_hold; mp = MP_HOLD; }
        else { m_per += (m_per < m_hold) ? 1 : -1; pwm_apply(m_per); }
        break;
    default: break;
    }
}
static int motor_busy(void) { return mp == MP_WASH_UP || mp == MP_STOP || mp == MP_REV_DOWN || mp == MP_REV_DWELL || mp == MP_REV_UP || mp == MP_TO_TARGET || mot_req.type != RQ_NONE; }
static void motor_request(uint8_t type, uint8_t dir, uint32_t period) {
    __disable_irq(); mot_req.dir = dir; mot_req.period = period; mot_req.type = type; __enable_irq();
}
/* estimated step rate: PWM clock 60 MHz/8; HIGH register = 2*hduty suggests a 2x counter */
static uint32_t step_hz_x10(uint32_t per) { return per ? (75000000u / (2 * per)) : 0; }   /* tenths of Hz */
static uint32_t rpm_x100(uint32_t per) { return per && cfg.steps_per_rev ? (uint32_t)(((uint64_t)step_hz_x10(per) * 600) / cfg.steps_per_rev) : 0; }

/* ======================================================================= SysTick 1 ms */
static volatile uint16_t key_beep;
void SysTick_Handler(void) {
    static uint8_t digit, cnt[NBTN], ramp_div;
    static uint16_t sec_ms;
    ms++;

    /* display multiplex */
    for (int i = 0; i < 4; i++) pin_set(DIGIT[i], 1);
    digit = (digit + 1) & 3;
    uint8_t m = blank ? 0 : disp[digit];
    for (int s = 0; s < 7; s++) pin_set(SEG[s], !((m >> s) & 1));
    pin_set(COLON, blank ? 1 : !colon_on);
    pin_set(DIGIT[digit], 0);

    /* inputs: 20 ms debounce */
    for (int b = 0; b < NBTN; b++) {
        uint8_t raw = pin_in(BTN[b]);
        if (raw != btn_level[b]) {
            if (++cnt[b] >= 20) { btn_level[b] = raw; btn_edge[b] = 1; cnt[b] = 0; if (b != B_LID && !raw) btn_down_ms[b] = ms; }
        } else cnt[b] = 0;
    }

    /* countdown */
    if (running && !timer_paused && time_s > 0) {
        if (++sec_ms >= 1000) { sec_ms = 0; time_s--; if (time_s <= 0) finished_flag = 1; }
    } else sec_ms = 0;

    /* motor engine */
    motor_service_request();
    if (++ramp_div >= RAMP_STEP_MS) { ramp_div = 0; motor_tick(); }

    /* buzzer: tone generator owns the pin while active */
    if (tone_on) { if ((int32_t)(ms - tone_until) >= 0) tone_stop(); }
    else {
        if (key_beep) key_beep--;
        if (beep_ms) beep_ms--;
        pin_set(BUZZER, buz_manual || beep_ms || key_beep);
    }
}

/* ======================================================================= flash (SFC) */
#define SFC_REG(o) (*(volatile uint32_t *)(0x4004A000u + (o)))
#define SFC_CFGR SFC_REG(0x00)
#define SFC_TIMR SFC_REG(0x04)
#define SFC_SRR  SFC_REG(0x08)
#define SFC_IFR  SFC_REG(0x0C)
#define SFC_GOR  SFC_REG(0x14)
#define SFC_ADRR SFC_REG(0x18)
#define SFC_CMDR SFC_REG(0x24)
#define FLASH_WIN 0x41000000u
#define FLASH_SIZE 0x1E000u
static int fl_wait(void) { uint32_t t = ms; while (SFC_SRR & 1) if (ms - t > 1000) return -1; return 0; }
static uint32_t fl_if(void) { uint32_t v = SFC_IFR; if (v) SFC_IFR = v; return v; }
static int fl_erase(uint32_t off) {
    if ((off & 0xFFF) || off >= FLASH_SIZE) return -1;
    if ((SFC_TIMR & 0xFFFF) != 0xFF3C) SFC_TIMR = (SFC_TIMR & ~0xFFFFu) | 0xFF3C;
    if (fl_wait()) return -2;
    fl_if();
    SFC_CFGR = (SFC_CFGR & ~0xFu) | 0x1027;          /* WREN | CMDWREN | CMDTYPE=7 */
    SFC_CMDR = 0x20; SFC_ADRR = off;
    if (SFC_CMDR != 0x20 || SFC_ADRR != off) { SFC_CFGR &= ~0x1027u; return -3; }
    SFC_GOR = 1;
    uint32_t t = ms; while (SFC_GOR) if (ms - t > 1000) break;
    int r = fl_wait();
    SFC_CFGR &= ~0x1027u;
    uint32_t f = fl_if();
    return (r || (f & 6)) ? -4 : 0;
}
static int fl_write(uint32_t off, const uint8_t *d, uint32_t n) {   /* n % 4 == 0, inside one 256 B page */
    if ((off & 3) || (n & 3) || off + n > FLASH_SIZE || ((off & 0xFF) + n) > 0x100) return -1;
    if (off <= 0x1C && off + n > 0x1C) {                        /* never arm read protection */
        uint32_t w; memcpy(&w, d + (0x1C - off), 4);
        if (w != 0) return -5;
    }
    if (fl_wait()) return -2;
    fl_if();
    SFC_CFGR |= 0x1000;
    for (uint32_t i = 0; i < n; i += 4) {
        uint32_t w; memcpy(&w, d + i, 4);
        if (w != 0xFFFFFFFFu) *(volatile uint32_t *)(FLASH_WIN + off + i) = w;
        fl_wait();
    }
    SFC_CFGR &= ~0x1000u;
    return (fl_if() & 6) ? -4 : 0;
}
/* The flash window (0x41000000) only returns correct data for aligned 32-bit reads
 * (byte reads come back lane-shuffled: seen on hardware). Always read through this. */
static uint8_t fl_byte(uint32_t off) {
    uint32_t w = *(volatile uint32_t *)(FLASH_WIN + (off & ~3u));
    return w >> (8 * (off & 3));
}
static void fl_read(uint32_t off, void *dst, uint32_t n) { uint8_t *d = dst; for (uint32_t i = 0; i < n; i++) d[i] = fl_byte(off + i); }
static uint32_t crc32(uint32_t crc, const uint8_t *p, uint32_t n) {
    crc = ~crc;
    while (n--) { crc ^= *p++; for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1)); }
    return ~crc;
}

static uint32_t crc32_flash(uint32_t off, uint32_t n) {
    uint32_t crc = ~0u;
    for (uint32_t i = 0; i < n; i++) { crc ^= fl_byte(off + i); for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1)); }
    return ~crc;
}
static void settings_load(void) {
    fl_read(SETTINGS_OFF, &cfg, sizeof cfg);
    if (cfg.magic != SETTINGS_MAGIC || cfg.crc != crc32(0, (uint8_t *)&cfg, sizeof cfg - 4)) cfg = CFG_DEFAULT;
}
static int settings_save(void) {
    cfg.magic = SETTINGS_MAGIC;
    cfg.crc = crc32(0, (uint8_t *)&cfg, sizeof cfg - 4);
    if (fl_erase(SETTINGS_OFF)) return -1;
    return fl_write(SETTINGS_OFF, (uint8_t *)&cfg, sizeof cfg);
}

/* Reload the image from flash into code SRAM and reset, i.e. what the boot ROM does at
 * power-up (a plain SYSRESETREQ does NOT reload: verified). Runs from data RAM (.data)
 * because it overwrites the code it would otherwise be executing. Never returns. */
__attribute__((section(".data.ramfunc"), noinline, long_call))
static void ram_reload_and_reset(uint32_t len) {
    volatile uint32_t *src = (volatile uint32_t *)FLASH_WIN;
    uint32_t *dst = (uint32_t *)0;
    __asm__ volatile("" : "+r"(dst));               /* address 0 is real code SRAM here */
    for (uint32_t i = 0; i < len / 4; i++) dst[i] = src[i];
    __asm__ volatile("dsb");
    *(volatile uint32_t *)0xE000ED0C = 0x05FA0004;  /* SYSRESETREQ */
    for (;;) {}
}
static void reboot_from_flash(void) {
    uint32_t key = *(volatile uint32_t *)(FLASH_WIN + 0x20), len = *(volatile uint32_t *)(FLASH_WIN + 0x24);
    uint32_t prot = *(volatile uint32_t *)(FLASH_WIN + 0x1C), memcfg = *(volatile uint32_t *)(FLASH_WIN + 0x28);
    __disable_irq();
    if (key == 0x0B11FFACu && prot == 0 && memcfg == 2 && len && len <= 0x8000 && !(len & 3))
        ram_reload_and_reset(len);
    NVIC_SystemReset();                              /* header invalid: restart current image */
}

/* ======================================================================= events */
static const char *mode_name(void) { return mode == MODE_WASH ? "WASH" : "CURE"; }
static void evt(const char *a, const char *b) { con_puts("EVT "); con_puts(a); if (b) { con_putc(' '); con_puts(b); } con_nl(); }

static void set_mode(uint8_t m) {
    mode = m;
    time_s = total_s = (m == MODE_WASH) ? cfg.wash_default_s : cfg.cure_default_s;
    evt("MODE", mode_name());
}
static void set_running(uint8_t r) {
    if (r && time_s <= 0) return;
    if (running == r) return;
    running = r;
    evt("STATE", r ? "RUNNING" : "PAUSED");
}
static void adjust_time(int32_t d) {        /* buttons: +-30 s, clamp 0..1800 like original */
    int32_t t = time_s + d;
    if (t > 1800) t = 1800;
    if (t < 0) t = 0;
    time_s = total_s = t;
}

/* ======================================================================= auto (original behaviour) */
static uint8_t wash_fwd_started, wash_rev_started, cure_motor_on;
static uint32_t alarm_t, finish_t;
static uint8_t finish_phase;                /* 0 = none, 1..6 alternating blank/show */

static void leds_auto(void) {
    /* original levels: mode LED low = lit; START LED lit (low) when idle, off when running;
       +/- LEDs only lit while their button is held (handled by button code). */
    pin_set(LED[L_WASH], mode != MODE_WASH);
    pin_set(LED[L_CURE], mode != MODE_CURE);
    pin_set(LED[L_START], running);
    pin_set(LED[L_PLUS], btn_level[B_PLUS] || running);       /* original: +/- inactive while running */
    pin_set(LED[L_MINUS], btn_level[B_MINUS] || running);
}

static void auto_step(void) {
    uint32_t now = ms;

    /* finish sequence (original: 3 x [~165 ms beep + blank, ~345 ms show]) */
    if (finished_flag) {
        finished_flag = 0;
        if (mode == MODE_WASH) motor_request(RQ_STOP_RAMP, 0, 0); else motor_request(RQ_OFF_NOW, 0, 0);
        pin_set(UV, 0);
        running = 0;
        cure_motor_on = wash_fwd_started = wash_rev_started = 0;
        finish_phase = 1; finish_t = now;
        evt("DONE", mode_name());
    }
    if (finish_phase) {
        uint32_t dur = (finish_phase & 1) ? 165 : 345;
        blank = finish_phase & 1;
        beep_ms = (finish_phase & 1) ? 2 : 0;
        if (now - finish_t >= dur) { finish_t = now; if (++finish_phase > 6) { finish_phase = 0; blank = 0; set_mode(mode); } }
        pin_set(LED[L_WASH], 1); pin_set(LED[L_CURE], 1); pin_set(LED[L_START], 0);
        if (!motor_busy() && m_applied == 0) pin_set(M_NEN, 1);
        return;
    }

    if (mode == MODE_WASH) {
        pin_set(UV, 0);
        timer_paused = 0;
        if (running) {
            leds_auto();
            pin_set(M_NEN, 0);
            if (time_s < total_s / 2) {                         /* second half: reverse */
                if (!wash_rev_started) { pin_set(M_A13, 0); pin_set(M_A12, 1); m_hold = cfg.wash_period; motor_request(RQ_WASH_REVERSE, 1, 0); wash_rev_started = 1; evt("MOTOR", "REVERSE"); }
            } else if (!wash_fwd_started) {
                pin_set(M_A13, 0); pin_set(M_A12, 1); pin_set(M_A14, 1);
                m_hold = cfg.wash_period;
                motor_request(RQ_WASH_START, 0, 0);
                wash_fwd_started = 1;
            }
        } else {
            leds_auto();
            wash_fwd_started = wash_rev_started = 0;
            if (m_applied || mp != MP_IDLE) { if (mp != MP_STOP && mot_req.type == RQ_NONE) motor_request(RQ_STOP_RAMP, 0, 0); }
            else pin_set(M_NEN, 1);
        }
        return;
    }

    /* CURE */
    int lid_open = btn_level[B_LID];
    if (running && lid_open && interlock) {                     /* lid alarm (original) */
        if (!timer_paused) {
            timer_paused = 1; alarm_t = now;
            motor_request(RQ_OFF_NOW, 0, 0);
            pin_set(M_NEN, 1); pin_set(UV, 0);
            cure_motor_on = 0;
        }
        uint32_t ph = (now - alarm_t) % 510;
        int on = ph < 165;                                      /* beep + LEDs phase (original timing) */
        blank = on;                                             /* display blinks "LId" in the other phase */
        colon_on = 0;
        disp_text("LId");
        disp_override_until = now + 50;
        beep_ms = on ? 2 : 0;
        for (int i = 0; i < NLED; i++) pin_set(LED[i], !on);
        return;
    }
    if (timer_paused) { timer_paused = 0; blank = 0; beep_ms = 0; colon_on = 1; disp_override_until = now; }
    leds_auto();
    pin_set(M_DIR, cfg.cure_dir);
    if (running) {
        pin_set(M_NEN, 0);
        pin_set(UV, !lid_open || !interlock);
        if (!cure_motor_on) {
            pin_set(M_A13, 0); pin_set(M_A12, 1); pin_set(M_A14, 0);
            motor_request(RQ_CONST, cfg.cure_dir, cfg.cure_period);
            cure_motor_on = 1;
        }
    } else {
        pin_set(UV, 0);
        if (cure_motor_on || m_applied) { motor_request(RQ_OFF_NOW, 0, 0); cure_motor_on = 0; }
        pin_set(M_NEN, 1);
    }
}

/* buttons: original semantics (release-triggered; +/- auto-repeat after 2 s) */
static uint32_t repeat_t;
static void buttons_step(void) {
    uint32_t now = ms;
    for (int b = 0; b < NBTN; b++) {
        if (!btn_edge[b]) continue;
        btn_edge[b] = 0;
        int lvl = btn_level[b];
        if (b == B_LID) { evt("LID", lvl ? "OPEN" : "CLOSED"); continue; }
        if (!lvl) {                                             /* press */
            con_puts("EVT BTN "); con_puts(BTN_NAME[b]); con_puts(" DOWN\r\n");
            if (cfg.key_beeps && (b == B_START || !running) && !finish_phase && !manual && !buttons_locked)
                key_beep = 100;                                 /* original: beep while held, max 100 ms */
            repeat_t = now;
            continue;
        }
        uint32_t held = now - btn_down_ms[b];                   /* release */
        key_beep = 0;
        con_puts("EVT BTN "); con_puts(BTN_NAME[b]); con_puts(" UP "); con_putu(held); con_nl();
        if (manual || buttons_locked || finish_phase) continue;
        switch (b) {
        case B_START: if (held < 2000) set_running(!running); break;
        case B_PLUS:  if (!running && held < 2000) adjust_time(30); break;
        case B_MINUS: if (!running && held < 2000) adjust_time(-30); break;
        case B_WASH:  if (!running && held < 2000 && mode != MODE_WASH) set_mode(MODE_WASH); break;
        case B_CURE:  if (!running && held < 4000 && mode != MODE_CURE) set_mode(MODE_CURE); break;
        }
    }
    /* +/- held >= 2 s: repeat every ~86 ms (original: 25 display refreshes) */
    if (!manual && !buttons_locked && !running && !finish_phase) {
        for (int b = B_MINUS; b <= B_PLUS; b++) {
            if (!btn_level[b] && now - btn_down_ms[b] >= 2000 && now - repeat_t >= 86) {
                repeat_t = now; adjust_time(b == B_PLUS ? 30 : -30);
            }
        }
    }
}

/* ======================================================================= boot animation (original) */
static void delay_real(uint32_t d) { uint32_t t = ms; while (ms - t < d) {} }
static void boot_animation(void) {
    static const uint8_t frames[16][2] = { {0,5},{0,0},{1,0},{2,0},{3,0},{3,1},{3,2},{3,3},
                                           {2,3},{1,3},{0,3},{0,4},{0,6},{1,6},{2,6},{3,6} };
    colon_on = 0;
    key_beep = 0; beep_ms = 332;                                /* original 800 "ms" beep */
    delay_real(336);
    for (int f = 0; f < 16; f++) {
        for (int i = 0; i < 4; i++) disp[i] = 0;
        disp[frames[f][0]] = 1 << frames[f][1];
        delay_real(83);
    }
    for (int i = 0; i < 4; i++) disp[i] = 0x7F;                 /* 8888 */
    delay_real(208);
    for (int i = 0; i < 4; i++) disp[i] = 0x3F;                 /* 0000 */
    delay_real(42);
    colon_on = 1;
}

/* ======================================================================= console commands */
static void status(void) {
    con_puts("STAT mode="); con_puts(mode_name());
    con_puts(" state="); con_puts(finish_phase ? "DONE" : timer_paused ? "LID_PAUSED" : running ? "RUNNING" : "IDLE");
    con_puts(" time="); con_puti(time_s); con_puts(" total="); con_puti(total_s);
    con_puts(" lid="); con_puts(btn_level[B_LID] ? "OPEN" : "CLOSED");
    con_puts(" uv="); con_putu(pin_out(UV));
    con_puts(" motor_en="); con_putu(!pin_out(M_NEN));
    con_puts(" dir="); con_putu(pin_out(M_DIR));
    con_puts(" period="); con_putu(m_applied);
    uint32_t hz = step_hz_x10(m_applied), rpm = rpm_x100(m_applied);
    con_puts(" step_hz="); con_putu(hz / 10); con_putc('.'); con_putu(hz % 10);
    con_puts(" rpm="); con_putu(rpm / 100); con_putc('.'); con_putu((rpm / 10) % 10); con_putu(rpm % 10);
    con_puts(" leds=");
    for (int i = 0; i < NLED; i++) con_putc(pin_out(LED[i]) ? '0' : '1');
    con_puts(" buttons=");
    for (int b = 0; b < B_LID; b++) con_putc(btn_level[b] ? '0' : '1');
    con_puts(" buzzer="); con_putu(pin_out(BUZZER));
    con_puts(" manual="); con_putu(manual);
    con_puts(" interlock="); con_putu(interlock);
    con_puts(" locked="); con_putu(buttons_locked);
    con_puts(" uptime="); con_putu(ms / 1000);
    con_nl();
}
static void config_print(void) {
    con_puts("CFG wash_default="); con_putu(cfg.wash_default_s);
    con_puts(" cure_default="); con_putu(cfg.cure_default_s);
    con_puts(" wash_period="); con_putu(cfg.wash_period);
    con_puts(" cure_period="); con_putu(cfg.cure_period);
    con_puts(" cure_dir="); con_putu(cfg.cure_dir);
    con_puts(" steps_per_rev="); con_putu(cfg.steps_per_rev);
    con_puts(" key_beeps="); con_putu(cfg.key_beeps);
    con_puts(" rev_down_ms="); con_putu(cfg.rev_down_ms);
    con_puts(" rev_dwell_ms="); con_putu(cfg.rev_dwell_ms);
    con_puts(" rev_up_ms="); con_putu(cfg.rev_up_ms);
    con_nl();
}
static const char HELP[] =
"Commands (case-insensitive). Replies OK/ERR, events EVT ...\r\n"
" STATUS | S                      full status line\r\n"
" STREAM <ms>                     periodic STATUS (0=off)\r\n"
" MODE WASH|CURE                  select mode (loads its default time)\r\n"
" START | PAUSE | TOGGLE | STOP   run control (STOP = pause + reload default)\r\n"
" TIME [s]                        get/set remaining (sets total too)\r\n"
" PRESS START|PLUS|MINUS|WASH|CURE  simulate a short press\r\n"
" CFG                             show settings\r\n"
" CFG WASH_DEFAULT|CURE_DEFAULT <s>\r\n"
" CFG WASH_PERIOD|CURE_PERIOD <pwm>  speed (smaller = faster; orig 55 / 35156)\r\n"
" CFG CURE_DIR 0|1 | CFG SPR <steps/rev> | CFG KEY_BEEPS 0|1\r\n"
" CFG REV_DOWN|REV_DWELL|REV_UP <ms>  wash reversal: slow-down / pause / speed-up\r\n"
" SAVE | FACTORY                  persist settings / restore defaults\r\n"
" BEEP <ms> | TONE <hz> <ms>      buzzer\r\n"
" DISP <text> [ms] | DISP AUTO    display overlay\r\n"
" LOCK | UNLOCK                   ignore/accept physical buttons\r\n"
" INTERLOCK ON | INTERLOCK OFF CONFIRM   lid interlock for UV/cure (resets on boot)\r\n"
" MANUAL ON|OFF                   manual output control:\r\n"
"   LED WASH|CURE|PLUS|MINUS|START|ALL 0|1, UV 0|1, BUZ 0|1\r\n"
"   MOTOR RUN <period> [dir] | MOTOR RAMP <period> [dir] | MOTOR STOP | MOTOR EN 0|1 | MOTOR DIR 0|1\r\n"
" PIN <A-E><0-15> [0|1|IN|OUT]    raw pin\r\n"
" PEEK <hexaddr> | POKE <hexaddr> <hexval>\r\n"
" FL READ <off> <len<=64> | FL CRC <off> <len> | FL UNLOCK | FL ERASE <off> | FL WRITE <off> <hex>\r\n"
" REBOOT [WARM] | VER             REBOOT reloads firmware from flash; WARM just resets\r\n";

static char *tok(char **s) {
    while (**s == ' ') (*s)++;
    if (!**s) return NULL;
    char *t = *s;
    while (**s && **s != ' ') (*s)++;
    if (**s) *(*s)++ = 0;
    return t;
}
static int eq(const char *a, const char *b) {
    if (!a) return 0;
    while (*a && *b) { char x = *a, y = *b; if (x >= 'a' && x <= 'z') x -= 32; if (y >= 'a' && y <= 'z') y -= 32; if (x != y) return 0; a++; b++; }
    return !*a && !*b;
}
static int num(const char *s, int32_t *v) { if (!s) return 0; char *e; long x = strtol(s, &e, 0); if (*e) return 0; *v = x; return 1; }
static int hexnum(const char *s, uint32_t *v) { if (!s) return 0; char *e; *v = strtoul(s, &e, 16); return !*e; }
static void ok(void) { con_puts("OK\r\n"); }
static void err(const char *m) { con_puts("ERR "); con_puts(m); con_nl(); }
static int need_manual(void) { if (!manual) { err("MANUAL ON first"); return 0; } return 1; }

static uint32_t fl_unlock_until;
static void cmd(char *line) {
    char *p = line;
    char *c = tok(&p);
    if (!c) return;
    char *a = tok(&p), *b = tok(&p), *d = tok(&p);
    int32_t v, w;

    if (eq(c, "STATUS") || eq(c, "S")) status();
    else if (eq(c, "HELP") || eq(c, "?")) con_puts(HELP);
    else if (eq(c, "VER")) { con_puts("OK " FW_VERSION " " __DATE__ " " __TIME__ "\r\n"); }
    else if (eq(c, "STREAM")) { if (num(a, &v) && v >= 0) { stream_ms = v; ok(); } else err("STREAM <ms>"); }
    else if (eq(c, "MODE")) {
        if (!a) { con_puts("OK "); con_puts(mode_name()); con_nl(); }
        else if (running) err("running");
        else if (eq(a, "WASH")) { set_mode(MODE_WASH); ok(); }
        else if (eq(a, "CURE")) { set_mode(MODE_CURE); ok(); }
        else err("MODE WASH|CURE");
    }
    else if (eq(c, "START"))  { if (time_s <= 0) err("time is 0"); else { set_running(1); ok(); } }
    else if (eq(c, "PAUSE"))  { set_running(0); ok(); }
    else if (eq(c, "TOGGLE")) { set_running(!running); ok(); }
    else if (eq(c, "STOP"))   { set_running(0); set_mode(mode); ok(); }
    else if (eq(c, "TIME")) {
        if (!a) { con_puts("OK "); con_puti(time_s); con_putc(' '); con_puti(total_s); con_nl(); }
        else if (num(a, &v) && v >= 0 && v <= 5999) { time_s = total_s = v; ok(); }
        else err("TIME 0..5999");
    }
    else if (eq(c, "PRESS")) {
        int bi = -1;
        for (int i = 0; i < B_LID; i++) if (eq(a, BTN_NAME[i])) bi = i;
        if (bi < 0) { err("PRESS START|PLUS|MINUS|WASH|CURE"); return; }
        if (cfg.key_beeps) key_beep = 80;
        switch (bi) {
        case B_START: set_running(!running); break;
        case B_PLUS:  if (!running) adjust_time(30); break;
        case B_MINUS: if (!running) adjust_time(-30); break;
        case B_WASH:  if (!running && mode != MODE_WASH) set_mode(MODE_WASH); break;
        case B_CURE:  if (!running && mode != MODE_CURE) set_mode(MODE_CURE); break;
        }
        ok();
    }
    else if (eq(c, "CFG")) {
        if (!a) { config_print(); return; }
        if (!num(b, &v) || v < 0) { err("CFG <name> <value>"); return; }
        if (eq(a, "WASH_DEFAULT") && v <= 5999) cfg.wash_default_s = v;
        else if (eq(a, "CURE_DEFAULT") && v <= 5999) cfg.cure_default_s = v;
        else if (eq(a, "WASH_PERIOD") && v >= 20) cfg.wash_period = v;
        else if (eq(a, "CURE_PERIOD") && v >= 20 && v < 0x0FFFFFFF) cfg.cure_period = v;
        else if (eq(a, "CURE_DIR") && v <= 1) cfg.cure_dir = v;
        else if (eq(a, "SPR") && v > 0) cfg.steps_per_rev = v;
        else if (eq(a, "KEY_BEEPS") && v <= 1) cfg.key_beeps = v;
        else if (eq(a, "REV_DOWN") && v >= 500 && v <= 30000) cfg.rev_down_ms = v;
        else if (eq(a, "REV_DWELL") && v <= 30000) cfg.rev_dwell_ms = v;
        else if (eq(a, "REV_UP") && v >= 1000 && v <= 60000) cfg.rev_up_ms = v;
        else { err("bad name/value"); return; }
        if (eq(a, "CURE_PERIOD") && running && mode == MODE_CURE) cure_motor_on = 0;   /* apply now */
        ok();
    }
    else if (eq(c, "SAVE"))    { if (settings_save()) err("flash"); else ok(); }
    else if (eq(c, "FACTORY")) { cfg = CFG_DEFAULT; ok(); }
    else if (eq(c, "BEEP"))    { if (num(a, &v) && v > 0 && v <= 10000) { beep_ms = v; ok(); } else err("BEEP <ms>"); }
    else if (eq(c, "TONE"))    { if (num(a, &v) && num(b, &w) && w > 0 && w <= 10000) { tone_start(v, w); ok(); } else err("TONE <hz> <ms>"); }
    else if (eq(c, "DISP")) {
        if (eq(a, "AUTO")) { disp_override_until = ms; ok(); }
        else if (a) { disp_text(a); disp_override_until = ms + ((num(b, &v) && v > 0) ? (uint32_t)v : 3000); ok(); }
        else err("DISP <text> [ms] | DISP AUTO");
    }
    else if (eq(c, "LOCK"))   { buttons_locked = 1; ok(); }
    else if (eq(c, "UNLOCK")) { buttons_locked = 0; ok(); }
    else if (eq(c, "INTERLOCK")) {
        if (eq(a, "ON")) { interlock = 1; ok(); }
        else if (eq(a, "OFF") && eq(b, "CONFIRM")) { interlock = 0; evt("WARNING", "INTERLOCK OFF - UV can run with lid open"); ok(); }
        else err("INTERLOCK ON | INTERLOCK OFF CONFIRM");
    }
    else if (eq(c, "MANUAL")) {
        if (eq(a, "ON")) {
            set_running(0); finish_phase = 0; blank = 0; timer_paused = 0; colon_on = 1; manual = 1;
            motor_request(RQ_OFF_NOW, 0, 0); pin_set(UV, 0); ok();
        } else if (eq(a, "OFF")) {
            manual = 0; buz_manual = 0; pin_set(UV, 0); motor_request(RQ_OFF_NOW, 0, 0);
            cure_motor_on = wash_fwd_started = wash_rev_started = 0; ok();
        } else err("MANUAL ON|OFF");
    }
    else if (eq(c, "LED")) {
        if (!need_manual()) return;
        if (!num(b, &v)) { err("LED <name|ALL> 0|1"); return; }
        int found = 0;
        for (int i = 0; i < NLED; i++) if (eq(a, "ALL") || eq(a, LED_NAME[i])) { pin_set(LED[i], !v); found = 1; }
        if (found) ok(); else err("LED WASH|CURE|PLUS|MINUS|START|ALL");
    }
    else if (eq(c, "UV")) {
        if (!a) { con_puts("OK "); con_putu(pin_out(UV)); con_nl(); return; }
        if (!need_manual()) return;
        if (!num(a, &v)) { err("UV 0|1"); return; }
        if (v && interlock && btn_level[B_LID]) { err("lid open (INTERLOCK OFF CONFIRM to override)"); return; }
        pin_set(UV, v); ok();
    }
    else if (eq(c, "BUZ")) { if (!need_manual()) return; if (num(a, &v)) { buz_manual = v != 0; ok(); } else err("BUZ 0|1"); }
    else if (eq(c, "MOTOR")) {
        if (!need_manual()) return;
        int32_t dir = pin_out(M_DIR);
        if (eq(a, "STOP")) { motor_request(RQ_STOP_RAMP, 0, 0); ok(); }
        else if (eq(a, "OFF")) { motor_request(RQ_OFF_NOW, 0, 0); ok(); }
        else if ((eq(a, "RUN") || eq(a, "RAMP")) && num(b, &v) && v >= 20) {
            if (d && !num(d, &dir)) { err("dir 0|1"); return; }
            pin_set(M_NEN, 0);
            motor_request(eq(a, "RUN") ? RQ_CONST : RQ_TO_TARGET, dir != 0, v); ok();
        }
        else if (eq(a, "EN") && num(b, &v)) { pin_set(M_NEN, !v); ok(); }
        else if (eq(a, "DIR") && num(b, &v)) { pin_set(M_DIR, v != 0); ok(); }
        else if (eq(a, "PIN") && b && num(d, &v)) {               /* MOTOR PIN A12|A13|A14|D0 0|1 */
            if (eq(b, "A12")) pin_set(M_A12, v); else if (eq(b, "A13")) pin_set(M_A13, v);
            else if (eq(b, "A14")) pin_set(M_A14, v); else if (eq(b, "D0")) pin_set(M_D0, v);
            else { err("A12|A13|A14|D0"); return; }
            ok();
        }
        else err("MOTOR RUN|RAMP <period> [dir] | STOP | OFF | EN 0|1 | DIR 0|1 | PIN A12|A13|A14|D0 0|1");
    }
    else if (eq(c, "PIN")) {
        static GPIO_TypeDef *const ports[5] = { GPIOA, GPIOB, GPIOC, GPIOD, GPIOE };
        char pc = a ? (a[0] & ~32) : 0;
        int32_t n;
        if (pc < 'A' || pc > 'E' || !num(a + 1, &n) || n < 0 || n > 15) { err("PIN <A-E><0-15> [0|1|IN|OUT]"); return; }
        GPIO_TypeDef *g = ports[pc - 'A'];
        if (!b) { con_puts("OK in="); con_putu((g->IDR >> n) & 1); con_puts(" out="); con_putu((g->ODR >> n) & 1); con_puts(" dir="); con_putu((g->DIR >> n) & 1); con_nl(); }
        else if (eq(b, "IN"))  { g->DIR &= ~(1u << n); ok(); }
        else if (eq(b, "OUT")) { g->DIR |= (1u << n); ok(); }
        else if (num(b, &v))   { (&g->DATAPIN0)[n] = v != 0; ok(); }
        else err("PIN <A-E><0-15> [0|1|IN|OUT]");
    }
    else if (eq(c, "PEEK")) { uint32_t ad; if (hexnum(a, &ad) && !(ad & 3)) { con_puts("OK "); con_puthex(*(volatile uint32_t *)ad, 8); con_nl(); } else err("PEEK <hexaddr>"); }
    else if (eq(c, "POKE")) { uint32_t ad, val; if (hexnum(a, &ad) && hexnum(b, &val) && !(ad & 3)) { *(volatile uint32_t *)ad = val; ok(); } else err("POKE <hexaddr> <hexval>"); }
    else if (eq(c, "FL")) {
        uint32_t off, len;
        if (eq(a, "READ") && hexnum(b, &off) && hexnum(d, &len) && len && len <= 64 && off + len <= FLASH_SIZE) {
            con_puts("OK ");
            for (uint32_t i = 0; i < len; i++) con_puthex(fl_byte(off + i), 2);
            con_nl();
        } else if (eq(a, "CRC") && hexnum(b, &off) && hexnum(d, &len) && off + len <= FLASH_SIZE) {
            con_puts("OK "); con_puthex(crc32_flash(off, len), 8); con_nl();
        } else if (eq(a, "UNLOCK")) {
            fl_unlock_until = ms + 30000; con_puts("OK image sectors writable for 30 s\r\n");
        } else if (eq(a, "ERASE") && hexnum(b, &off)) {
            if (off < 0x8000 && (int32_t)(ms - fl_unlock_until) >= 0) { err("FL UNLOCK first (image area)"); return; }
            int r = fl_erase(off); if (r) { con_puts("ERR erase "); con_puti(r); con_nl(); } else ok();
        } else if (eq(a, "WRITE") && hexnum(b, &off) && d) {
            uint8_t buf[64]; uint32_t n = strlen(d) / 2;
            if (n == 0 || n > 64 || (strlen(d) & 1)) { err("hex, max 64 bytes"); return; }
            if (off < 0x8000 && (int32_t)(ms - fl_unlock_until) >= 0) { err("FL UNLOCK first (image area)"); return; }
            for (uint32_t i = 0; i < n; i++) { char h[3] = { d[2*i], d[2*i+1], 0 }; char *e; buf[i] = strtoul(h, &e, 16); if (*e) { err("bad hex"); return; } }
            int r = fl_write(off, buf, n); if (r) { con_puts("ERR write "); con_puti(r); con_nl(); } else ok();
        } else err("FL READ|CRC <off> <len> | FL UNLOCK | FL ERASE <off> | FL WRITE <off> <hex> (hex numbers)");
    }
    else if (eq(c, "REBOOT")) {                     /* reloads the image from flash (runs new firmware) */
        con_puts("OK rebooting\r\n"); delay_real(20);
        if (eq(a, "WARM")) { __disable_irq(); NVIC_SystemReset(); }   /* restart without reloading */
        reboot_from_flash();
    }
    else err("unknown command (HELP)");
}

/* ======================================================================= main */
int main(void) {
    SystemInit();                                   /* PLL from 24 MHz XTAL -> 60 MHz, as original */
    settings_load();

    /* outputs to safe levels (latch first, no glitches) */
    out_init(UV, 0);
    out_init(BUZZER, 0);
    for (int i = 0; i < 4; i++) out_init(DIGIT[i], 1);
    for (int i = 0; i < 7; i++) out_init(SEG[i], 1);
    out_init(COLON, 1);
    for (int i = 0; i < NLED; i++) out_init(LED[i], 1);
    out_init(M_NEN, 1); out_init(M_D0, 1); out_init(M_DIR, 0);
    out_init(M_A13, 0); out_init(M_A12, 1); out_init(M_A14, 0);
    for (int i = 0; i < NBTN; i++) GPIO_Init(BTN[i].port, BTN[i].pin, 0, 0, 0, 0);   /* external pull-ups */
    for (int i = 0; i < NBTN; i++) btn_level[i] = pin_in(BTN[i]);
    con_init();
    NVIC_SetPriority(SysTick_IRQn, 1);
    SysTick_Config(SystemCoreClock / 1000);
    NVIC_SetPriority(SysTick_IRQn, 1);
    NVIC_SetPriority(TIMR1_IRQn, 2);
    NVIC_EnableIRQ(TIMR1_IRQn);

    con_puts("\r\n" FW_VERSION " ready. HELP for commands.\r\n");
    boot_animation();
    set_mode(MODE_CURE);                            /* power-on default: cure, 10:00 */
    status();

    static char line[160];
    static uint8_t llen;
    for (;;) {
        /* console input */
        while (rx_t != rx_h) {
            char ch = rxb[rx_t]; rx_t = (rx_t + 1) & 255;
            if (ch == '\r' || ch == '\n') { if (llen) { line[llen] = 0; cmd(line); llen = 0; } }
            else if (llen < sizeof line - 1) line[llen++] = ch;
        }

        buttons_step();
        if (!manual) auto_step();

        /* display: overlay text, else remaining time */
        if (!manual && (int32_t)(ms - disp_override_until) >= 0 && !blank) disp_time(time_s);

        if (stream_ms && ms - stream_t >= stream_ms) { stream_t = ms; status(); }
    }
}
