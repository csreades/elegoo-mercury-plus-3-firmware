// ===== SFC_QSPI_Enable @ 000000c0

/* Called from Reset if chip ID 0x854012 and SFC not in QSPI mode */

void SFC_QSPI_Enable(void)

{
  undefined4 *puVar1;
  
  puVar1 = SFC;
  do {
  } while ((SFC[2] & 1) != 0);
  *SFC = 0x1026;
  puVar1[9] = 1;
  puVar1[7] = 0x200;
  puVar1[5] = 1;
  do {
  } while ((puVar1[5] & 1) != 0);
  *puVar1 = 0x820;
  puVar1[8] = 0xeb050206;
  *puVar1 = 0x800;
  return;
}


// ===== __rt_entry_thunk @ 00000108 (jumps to main @ 0x212)

void main(void)

{
  (*0x213)();
  return;
}


// ===== time_to_digits @ 00000114

/* secs -> g.digit[0..3] = mm:ss */

void time_to_digits(undefined4 param_1)

{
  int iVar1;
  undefined8 uVar2;
  undefined8 uVar3;
  
  uVar2 = __aeabi_idivmod(param_1,0x3c);
  uVar3 = __aeabi_idivmod((int)uVar2,10);
  iVar1 = RAM_20000000;
  *(undefined8 *)(RAM_20000000 + 0x14) = uVar3;
  uVar2 = __aeabi_idivmod((int)((ulonglong)uVar2 >> 0x20),10);
  *(undefined8 *)(iVar1 + 0x1c) = uVar2;
  return;
}


// ===== delay_ms @ 00000138

/* busy-wait, calibrated for 24 MHz */

void delay_ms(int param_1)

{
  int iVar1;
  int iVar2;
  
  for (; param_1 != 0; param_1 = param_1 + -1) {
    iVar2 = 0x26;
    do {
      iVar1 = 0x82;
      do {
        iVar1 = iVar1 + -1;
      } while (iVar1 != 0);
      iVar2 = iVar2 + -1;
    } while (iVar2 != 0);
  }
  return;
}


// ===== boot_beep_and_animation @ 00000154

void boot_beep_and_animation(void)

{
  int iVar1;
  undefined4 uVar2;
  
  uVar2 = GPIOE;
  GPIO_SetBit(GPIOE,4);
  delay_ms(800);
  GPIO_ClrBit(uVar2,4);
  delay_ms(10);
  iVar1 = RAM_20000000;
  *(undefined1 *)(RAM_20000000 + 3) = 1;
  *(undefined1 *)(iVar1 + 6) = 0;
  *(undefined1 *)(iVar1 + 7) = 0;
  GPIO_SetBit(GPIOA,0xf);
  boot_animation();
  GPIO_ClrBit(uVar2,3);
  *(undefined1 *)(iVar1 + 1) = 0;
  *(undefined1 *)(iVar1 + 2) = 0;
  return;
}


// ===== TIMR2_init_1s @ 0000019c

/* TIMR2 period = SystemCoreClock (1 s), IRQ on */

void TIMR2_init_1s(void)

{
  undefined4 uVar1;
  
  uVar1 = TIMR2;
  TIMR_Init(TIMR2,0,*RAM_2000009c,1);
  TIMR_Start(uVar1);
  return;
}


// ===== TIMR0_init_1ms @ 000001b6

/* TIMR0 period = SystemCoreClock/1000 (1 ms), IRQ on */

void TIMR0_init_1ms(void)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = __aeabi_uidivmod(*RAM_2000009c,1000);
  iVar2 = TIMR2 + -0x80;
  TIMR_Init(iVar2,0,uVar1,1);
  TIMR_Start(iVar2);
  return;
}


// ===== io_init_buzzer_uv_lid @ 000001dc

/* PE4 buzzer out, PA8 UV out, PB6 lid in */

void io_init_buzzer_uv_lid(void)

{
  GPIO_Init(GPIOE,4,1,0,0,0);
  GPIO_Init(GPIOA,8,1,0,0,0);
  GPIO_Init(GPIOB,6,0,0,0,0);
  return;
}


// ===== main @ 00000212

void main(void)

{
  char *pcVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  
  SystemInit();
  display_init();
  motor_io_init();
  leds_init();
  buttons_init();
  io_init_buzzer_uv_lid();
  TIMR0_init_1ms();
  TIMR2_init_1s();
  boot_beep_and_animation();
  pcVar1 = RAM_20000000;
  do {
    while( true ) {
      time_to_digits(*(undefined4 *)(pcVar1 + 0xc));
      display_refresh(*(undefined4 *)(pcVar1 + 0x14),*(undefined4 *)(pcVar1 + 0x18),
                      *(undefined4 *)(pcVar1 + 0x1c),*(undefined4 *)(pcVar1 + 0x20));
      uVar2 = GPIOE;
      if (*pcVar1 != '\0') break;
      if (pcVar1[4] == '\0') {
        GPIO_ClrBit(GPIOE,3);
        uVar3 = GPIOA;
        GPIO_ClrBit(GPIOA,9);
        GPIO_SetBit(uVar3,10);
        GPIO_SetBit(uVar3,0xb);
        GPIO_SetBit(uVar2,2);
        pcVar1[6] = '\0';
        pcVar1[7] = '\0';
        motor_ramp_down_stop();
        GPIO_SetBit(uVar3,0xf);
      }
      else if (pcVar1[4] == '\x01') {
        GPIO_SetBit(GPIOE,3);
        uVar3 = GPIOA;
        GPIO_SetBit(GPIOA,0xb);
        GPIO_SetBit(uVar2,2);
        GPIO_ClrBit(uVar3,9);
        GPIO_SetBit(uVar3,10);
        GPIO_ClrBit(uVar3,0xf);
        if (*(int *)(pcVar1 + 0xc) < *(int *)(pcVar1 + 0x10) / 2) {
          pcVar1[5] = '\x01';
          if (pcVar1[7] == '\0') {
            if (*(int *)(pcVar1 + 0x28) != 0) {
              motor_ramp_down_reverse();
            }
            GPIO_ClrBit(uVar3,0xf);
            motor_pwm_start_wash();
            motor_ramp_up_wash();
            pcVar1[7] = '\x01';
          }
        }
        else {
          pcVar1[5] = '\0';
          if (pcVar1[6] == '\0') {
            GPIO_ClrBit(uVar3,0xf);
            GPIO_SetBit(uVar3,0xe);
            motor_pwm_start_wash();
            motor_ramp_up_wash();
            pcVar1[6] = '\x01';
          }
        }
      }
    }
    if (*pcVar1 == '\x01') {
      GPIO_ClrBit(GPIOD,1);
      uVar2 = GPIOA;
      if (pcVar1[4] == '\0') {
        GPIO_ClrBit(GPIOA,8);
        uVar3 = GPIOE;
        GPIO_ClrBit(GPIOE,3);
        GPIO_ClrBit(uVar2,10);
        GPIO_SetBit(uVar2,9);
        GPIO_SetBit(uVar2,0xb);
        GPIO_SetBit(uVar3,2);
        pcVar1[8] = '\0';
        PWM_SetPeriod(2);
        PWM_SetHDuty(2,0);
        GPIO_SetBit(uVar2,0xf);
      }
      else if (pcVar1[4] == '\x01') {
        GPIO_ClrBit(GPIOA,0xf);
        uVar3 = GPIOB;
        iVar4 = GPIO_GetBit(GPIOB,6);
        if (iVar4 == 0) {
          GPIO_SetBit(uVar2,8);
        }
        else {
          GPIO_ClrBit(uVar2,8);
        }
        uVar2 = GPIOA;
        GPIO_SetBit(GPIOA,0xb);
        GPIO_SetBit(GPIOE,2);
        GPIO_ClrBit(uVar2,10);
        GPIO_SetBit(uVar2,9);
        if (pcVar1[8] == '\0') {
          GPIO_ClrBit(GPIOA,0xe);
          motor_pwm_start_cure();
          pcVar1[8] = '\x01';
        }
        while ((iVar4 = GPIO_GetBit(uVar3,6), iVar4 == 1 && (pcVar1[4] == '\x01'))) {
          PWM_SetPeriod(2,0);
          PWM_SetHDuty(2,0);
          uVar2 = GPIOA;
          GPIO_SetBit(GPIOA,0xf);
          GPIO_ClrBit(uVar2,8);
          TIMR_Stop(TIMR2);
          GPIO_ClrBit(uVar2,9);
          GPIO_ClrBit(uVar2,10);
          GPIO_ClrBit(uVar2,0xb);
          uVar2 = GPIOE;
          GPIO_ClrBit(GPIOE,2);
          GPIO_ClrBit(uVar2,3);
          GPIO_SetBit(uVar2,4);
          uVar2 = GPIOC;
          iVar4 = 0;
          while ((iVar4 < 0x32 && (iVar5 = GPIO_GetBit(uVar3,6), iVar5 == 1))) {
            GPIO_SetBit(uVar3,0xf);
            GPIO_SetBit(uVar2,2);
            GPIO_SetBit(uVar2,4);
            GPIO_SetBit(uVar2,5);
            delay_ms(8);
            iVar4 = iVar4 + 1;
          }
          uVar2 = GPIOA;
          GPIO_SetBit(GPIOA,9);
          GPIO_SetBit(uVar2,10);
          GPIO_SetBit(uVar2,0xb);
          uVar2 = GPIOE;
          GPIO_SetBit(GPIOE,2);
          GPIO_SetBit(uVar2,3);
          GPIO_ClrBit(uVar2,4);
          pcVar1[8] = '\0';
          iVar4 = 0;
          while ((iVar4 < 0x32 && (iVar5 = GPIO_GetBit(uVar3,6), iVar5 == 1))) {
            time_to_digits(*(undefined4 *)(pcVar1 + 0xc));
            display_refresh(*(undefined4 *)(pcVar1 + 0x14),*(undefined4 *)(pcVar1 + 0x18),
                            *(undefined4 *)(pcVar1 + 0x1c),*(undefined4 *)(pcVar1 + 0x20));
            iVar4 = iVar4 + 1;
          }
        }
        TIMR_Start(TIMR2);
        GPIO_SetBit(GPIOE,3);
      }
    }
  } while( true );
}


// ===== finish_beep_unused @ 00000538

/* not referenced */

void finish_beep_unused(void)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = GPIOE;
  GPIO_SetBit(GPIOE,4);
  delay_ms(100);
  GPIO_ClrBit(uVar1,4);
  delay_ms(10);
  iVar2 = RAM_20000000;
  *(undefined4 *)(RAM_20000000 + 0xc) = 0;
  *(undefined1 *)(iVar2 + 4) = 0;
  GPIO_SetBit(GPIOB,0xf);
  uVar1 = GPIOC;
  GPIO_SetBit(GPIOC,2);
  GPIO_SetBit(uVar1,4);
  GPIO_SetBit(uVar1,5);
  return;
}


// ===== TIMR2_IRQ_countdown_1s @ 00000584

/* decrements g.time_s while running; finish => 3 beeps, UV off, motor stop */

void TIMR2_IRQ_countdown_1s(void)

{
  char *pcVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  int iVar4;
  
  TIMR_INTClr(TIMR2);
  pcVar1 = RAM_20000000;
  if ((RAM_20000000[4] == '\x01') && (0 < *(int *)(RAM_20000000 + 0xc))) {
    *(int *)(RAM_20000000 + 0xc) = *(int *)(RAM_20000000 + 0xc) + -1;
  }
  if (*(int *)(pcVar1 + 0xc) < 1) {
    if (*pcVar1 == '\0') {
      motor_ramp_down_stop();
    }
    pcVar1[0xc] = '\0';
    pcVar1[0xd] = '\0';
    pcVar1[0xe] = '\0';
    pcVar1[0xf] = '\0';
    if (pcVar1[4] == '\x01') {
      *pcVar1 = '\x03';
      uVar2 = GPIOA;
      GPIO_SetBit(GPIOA,10);
      GPIO_SetBit(uVar2,9);
      uVar2 = GPIOE;
      GPIO_ClrBit(GPIOE,3);
      GPIO_SetBit(uVar2,4);
      pcVar1[0x24] = '\0';
      uVar3 = GPIOC;
      uVar2 = GPIOB;
      pcVar1[0x25] = '\0';
      pcVar1[0x26] = '\0';
      pcVar1[0x27] = '\0';
      do {
        GPIO_SetBit(uVar2,0xf);
        GPIO_SetBit(uVar3,2);
        GPIO_SetBit(uVar3,4);
        GPIO_SetBit(uVar3,5);
        delay_ms(8);
        iVar4 = *(int *)(pcVar1 + 0x24);
        *(int *)(pcVar1 + 0x24) = iVar4 + 1;
      } while (iVar4 + 1 < 0x32);
      GPIO_ClrBit(GPIOE,4);
      pcVar1[0x24] = '\0';
      pcVar1[0x25] = '\0';
      pcVar1[0x26] = '\0';
      pcVar1[0x27] = '\0';
      do {
        time_to_digits(*(undefined4 *)(pcVar1 + 0xc));
        display_refresh(*(undefined4 *)(pcVar1 + 0x14),*(undefined4 *)(pcVar1 + 0x18),
                        *(undefined4 *)(pcVar1 + 0x1c),*(undefined4 *)(pcVar1 + 0x20));
        iVar4 = *(int *)(pcVar1 + 0x24);
        *(int *)(pcVar1 + 0x24) = iVar4 + 1;
      } while (iVar4 + 1 < 0x32);
      GPIO_SetBit(GPIOE,4);
      pcVar1[0x24] = '\0';
      pcVar1[0x25] = '\0';
      pcVar1[0x26] = '\0';
      pcVar1[0x27] = '\0';
      do {
        GPIO_SetBit(uVar2,0xf);
        GPIO_SetBit(uVar3,2);
        GPIO_SetBit(uVar3,4);
        GPIO_SetBit(uVar3,5);
        delay_ms(8);
        iVar4 = *(int *)(pcVar1 + 0x24);
        *(int *)(pcVar1 + 0x24) = iVar4 + 1;
      } while (iVar4 + 1 < 0x32);
      GPIO_ClrBit(GPIOE,4);
      pcVar1[0x24] = '\0';
      pcVar1[0x25] = '\0';
      pcVar1[0x26] = '\0';
      pcVar1[0x27] = '\0';
      do {
        time_to_digits(*(undefined4 *)(pcVar1 + 0xc));
        display_refresh(*(undefined4 *)(pcVar1 + 0x14),*(undefined4 *)(pcVar1 + 0x18),
                        *(undefined4 *)(pcVar1 + 0x1c),*(undefined4 *)(pcVar1 + 0x20));
        iVar4 = *(int *)(pcVar1 + 0x24);
        *(int *)(pcVar1 + 0x24) = iVar4 + 1;
      } while (iVar4 + 1 < 0x32);
      GPIO_SetBit(GPIOE,4);
      pcVar1[0x24] = '\0';
      pcVar1[0x25] = '\0';
      pcVar1[0x26] = '\0';
      pcVar1[0x27] = '\0';
      do {
        GPIO_SetBit(uVar2,0xf);
        GPIO_SetBit(uVar3,2);
        GPIO_SetBit(uVar3,4);
        GPIO_SetBit(uVar3,5);
        delay_ms(8);
        iVar4 = *(int *)(pcVar1 + 0x24);
        *(int *)(pcVar1 + 0x24) = iVar4 + 1;
      } while (iVar4 + 1 < 0x32);
      GPIO_ClrBit(GPIOE,4);
      pcVar1[0x24] = '\0';
      pcVar1[0x25] = '\0';
      pcVar1[0x26] = '\0';
      pcVar1[0x27] = '\0';
      do {
        time_to_digits(*(undefined4 *)(pcVar1 + 0xc));
        display_refresh(*(undefined4 *)(pcVar1 + 0x14),*(undefined4 *)(pcVar1 + 0x18),
                        *(undefined4 *)(pcVar1 + 0x1c),*(undefined4 *)(pcVar1 + 0x20));
        iVar4 = *(int *)(pcVar1 + 0x24);
        *(int *)(pcVar1 + 0x24) = iVar4 + 1;
      } while (iVar4 + 1 < 0x32);
      pcVar1[3] = '\x01';
      pcVar1[2] = '\0';
      pcVar1[1] = '\0';
      uVar2 = GPIOA;
      GPIO_ClrBit(GPIOA,8);
      PWM_SetPeriod(2,0);
      PWM_SetHDuty(2,0);
      pcVar1[0x28] = '\0';
      pcVar1[0x29] = '\0';
      pcVar1[0x2a] = '\0';
      pcVar1[0x2b] = '\0';
      GPIO_SetBit(uVar2,0xf);
    }
    pcVar1[4] = '\0';
  }
  return;
}


// ===== TIMR0_IRQ_buttons_1ms @ 0000072c

/* polls all buttons every 1 ms */

void TIMR0_IRQ_buttons_1ms(void)

{
  char *pcVar1;
  
  TIMR_INTClr(TIMR2 + -0x80);
  pcVar1 = RAM_20000000;
  if (*RAM_20000000 != '\x03') {
    btn_start_process();
  }
  if (pcVar1[4] == '\0' && pcVar1[3] == '\0') {
    btn_plusminus_process();
  }
  if (pcVar1[4] == '\0' && pcVar1[1] == '\0') {
    btn_wash_process();
  }
  if (pcVar1[4] == '\0' && pcVar1[2] == '\0') {
    btn_cure_process();
  }
  return;
}


// ===== display_refresh_n @ 00000768

/* refresh 4-digit display n times (~8 ms each) */

void display_refresh_n(int param_1)

{
  int iVar1;
  int iVar2;
  
  iVar1 = RAM_20000000;
  for (; param_1 != 0; param_1 = param_1 + -1) {
    display_refresh(*(undefined4 *)(iVar1 + 0x14),*(undefined4 *)(iVar1 + 0x18),
                    *(undefined4 *)(iVar1 + 0x1c),*(undefined4 *)(iVar1 + 0x20));
    iVar2 = 1;
    do {
      time_to_digits(*(undefined4 *)(iVar1 + 0xc));
      display_refresh(*(undefined4 *)(iVar1 + 0x14),*(undefined4 *)(iVar1 + 0x18),
                      *(undefined4 *)(iVar1 + 0x1c),*(undefined4 *)(iVar1 + 0x20));
      iVar2 = iVar2 + -1;
    } while (iVar2 != 0);
  }
  return;
}


// ===== leds_init @ 000007b4

/* PA9 PA10 PA11 PE2 PE3 outputs, all set high */

void leds_init(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  
  uVar1 = GPIOA;
  GPIO_Init(GPIOA,9,1,0,0,0);
  GPIO_Init(uVar1,10,1,0,0,0);
  GPIO_Init(uVar1,0xb,1,0,0,0);
  uVar2 = GPIOE;
  GPIO_Init(GPIOE,2,1,0,0,0);
  GPIO_Init(uVar2,3,1,0,0,0);
  GPIO_SetBit(uVar1,9);
  GPIO_SetBit(uVar1,10);
  GPIO_SetBit(uVar1,0xb);
  GPIO_SetBit(uVar2,2);
  GPIO_SetBit(uVar2,3);
  return;
}


// ===== buttons_init @ 00000836

/* PB1..PB5 inputs (no internal pulls) */

void buttons_init(void)

{
  undefined4 uVar1;
  
  uVar1 = GPIOB;
  GPIO_Init(GPIOB,5,0,0,0,0);
  GPIO_Init(uVar1,4,0,0,0,0);
  GPIO_Init(uVar1,3,0,0,0,0);
  GPIO_Init(uVar1,2,0,0,0,0);
  GPIO_Init(uVar1,1,0,0,0,0);
  return;
}


// ===== btn_start_read @ 0000088e

/* PB1 */

undefined4 btn_start_read(void)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = GPIOB;
  iVar2 = GPIO_GetBit(GPIOB,1);
  if (iVar2 == 0) {
    iVar2 = GPIO_GetBit(uVar1,1);
    if (iVar2 == 0) {
      GPIO_SetBit(GPIOE,4);
      return 1;
    }
  }
  else {
    iVar2 = GPIO_GetBit(uVar1,1);
    if (iVar2 == 1) {
      return 0;
    }
  }
  return 0;
}


// ===== btn_start_process @ 000008cc

/* short press toggles run if time>0 */

undefined4 btn_start_process(void)

{
  ushort uVar1;
  char *pcVar2;
  undefined4 uVar3;
  int iVar4;
  
  iVar4 = btn_start_read();
  pcVar2 = RAM_2000002c;
  if ((iVar4 == 1) && (*(short *)(RAM_2000002c + 2) == 0)) {
    *RAM_2000002c = '\x01';
    pcVar2[2] = '\x01';
    pcVar2[3] = '\0';
  }
  if (iVar4 == 1) {
    uVar1 = *(ushort *)(pcVar2 + 2);
    if ((uVar1 != 0) && (uVar1 <= 0x2710)) {
      *(ushort *)(pcVar2 + 2) = uVar1 + 1;
    }
  }
  if (99 < *(ushort *)(pcVar2 + 2)) {
    GPIO_ClrBit(GPIOE,4);
  }
  uVar3 = GPIOE;
  if (((iVar4 == 0) && (*(ushort *)(pcVar2 + 2) != 0)) && (*(ushort *)(pcVar2 + 2) < 2000)) {
    GPIO_ClrBit(GPIOE,4);
    if (*pcVar2 != '\0') {
      if ((*RAM_20000004 == '\0') && (0 < *RAM_2000000c)) {
        *RAM_20000004 = '\x01';
        GPIO_SetBit(uVar3,3);
      }
      else {
        *RAM_20000004 = '\0';
      }
      pcVar2[2] = '\0';
      pcVar2[3] = '\0';
      return 1;
    }
    pcVar2[2] = '\0';
    pcVar2[3] = '\0';
  }
  else if ((iVar4 == 0) && (1999 < *(ushort *)(pcVar2 + 2))) {
    if (*pcVar2 != '\0') {
      pcVar2[2] = '\0';
      pcVar2[3] = '\0';
      return 2;
    }
    pcVar2[2] = '\0';
    pcVar2[3] = '\0';
  }
  return 0;
}


// ===== btn_plusminus_read @ 00000970

/* PB3 = +, PB2 = - */

undefined4 btn_plusminus_read(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int iVar3;
  
  uVar2 = GPIOB;
  iVar3 = GPIO_GetBit(GPIOB,3);
  if ((iVar3 == 0) || (iVar3 = GPIO_GetBit(uVar2,2), iVar3 == 0)) {
    iVar3 = GPIO_GetBit(uVar2,3);
    uVar1 = GPIOE;
    if (iVar3 == 0) {
      GPIO_ClrBit(GPIOA,0xb);
      GPIO_SetBit(uVar1,4);
      return 2;
    }
    iVar3 = GPIO_GetBit(uVar2,2);
    if (iVar3 == 0) {
      GPIO_ClrBit(uVar1,2);
      GPIO_SetBit(uVar1,4);
      return 3;
    }
  }
  else {
    iVar3 = GPIO_GetBit(uVar2,3);
    if ((iVar3 == 1) && (iVar3 = GPIO_GetBit(uVar2,2), iVar3 == 1)) {
      return 0;
    }
  }
  return 0;
}


// ===== btn_plusminus_process @ 000009f4

/* +/-30 s, clamp 0..1800 s, auto-repeat after 2 s hold */

undefined4 btn_plusminus_process(void)

{
  ushort uVar1;
  char *pcVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int *piVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  undefined4 *puVar8;
  int iVar9;
  
  iVar9 = btn_plusminus_read();
  pcVar2 = RAM_2000002c;
  if ((iVar9 != 0) && (*(short *)(RAM_2000002c + 4) == 0)) {
    *RAM_2000002c = (char)iVar9;
    pcVar2[4] = '\x01';
    pcVar2[5] = '\0';
  }
  if (iVar9 != 0) {
    uVar1 = *(ushort *)(pcVar2 + 4);
    if ((uVar1 != 0) && (uVar1 <= 0x2710)) {
      *(ushort *)(pcVar2 + 4) = uVar1 + 1;
    }
  }
  if (99 < *(ushort *)(pcVar2 + 4)) {
    GPIO_ClrBit(GPIOE,4);
  }
  puVar8 = RAM_20000018;
  puVar7 = RAM_20000014;
  puVar6 = RAM_20000020;
  piVar5 = RAM_2000000c;
  uVar4 = GPIOE;
  if (((iVar9 == 0) && (*(ushort *)(pcVar2 + 4) != 0)) && (*(ushort *)(pcVar2 + 4) < 2000)) {
    GPIO_ClrBit(GPIOE,4);
    uVar3 = GPIOA;
    if (*pcVar2 == '\x02') {
      GPIO_SetBit(GPIOA,0xb);
      GPIO_SetBit(uVar4,2);
      iVar9 = *piVar5;
      *piVar5 = iVar9 + 0x1e;
      *RAM_20000010 = iVar9 + 0x1e;
      GPIO_SetBit(uVar3,0xb);
      GPIO_SetBit(uVar4,2);
      if (0x707 < *piVar5) {
        *piVar5 = 0x708;
      }
    }
    else if (*pcVar2 == '\x03') {
      iVar9 = *piVar5;
      *piVar5 = iVar9 + -0x1e;
      *RAM_20000010 = iVar9 + -0x1e;
      GPIO_SetBit(uVar4,2);
      uVar3 = GPIOA;
      GPIO_SetBit(GPIOA,0xb);
      if (*piVar5 < 1) {
        *piVar5 = 0;
      }
      GPIO_SetBit(uVar3,0xb);
      GPIO_SetBit(uVar4,2);
    }
    pcVar2[4] = '\0';
    pcVar2[5] = '\0';
    return 1;
  }
  if ((iVar9 == 0) || (*(ushort *)(pcVar2 + 4) < 2000)) {
    return 0;
  }
  if (*RAM_2000002c == '\x02') {
    while (iVar9 = GPIO_GetBit(GPIOB,3), iVar9 == 0) {
      iVar9 = *piVar5 + 0x1e;
      *piVar5 = iVar9;
      *RAM_20000010 = iVar9;
      if (0x707 < iVar9) {
        *piVar5 = 0x708;
      }
      iVar9 = 0;
      do {
        time_to_digits(*piVar5);
        display_refresh(*puVar7,*RAM_20000018,*RAM_2000001c,*puVar6);
        iVar9 = iVar9 + 1;
      } while (iVar9 < 0x19);
    }
    GPIO_SetBit(GPIOA,0xb);
    GPIO_SetBit(GPIOE,2);
  }
  else if (*RAM_2000002c == '\x03') {
    while (iVar9 = GPIO_GetBit(GPIOB,2), iVar9 == 0) {
      iVar9 = *piVar5;
      *piVar5 = iVar9 + -0x1e;
      *RAM_20000010 = iVar9 + -0x1e;
      iVar9 = 0;
      do {
        time_to_digits(*piVar5);
        display_refresh(*puVar7,*puVar8,*RAM_2000001c,*RAM_20000020);
        iVar9 = iVar9 + 1;
      } while (iVar9 < 0x19);
      if (*piVar5 < 1) {
        *piVar5 = 0;
      }
    }
    GPIO_SetBit(GPIOE,2);
    GPIO_SetBit(GPIOA,0xb);
  }
  pcVar2 = RAM_2000002c;
  pcVar2[4] = '\0';
  pcVar2[5] = '\0';
  return 2;
}


// ===== btn_wash_read @ 00000bd2

/* PB5 */

undefined4 btn_wash_read(void)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = GPIOB;
  iVar2 = GPIO_GetBit(GPIOB,5);
  if (iVar2 == 0) {
    iVar2 = GPIO_GetBit(uVar1,5);
    if (iVar2 == 0) {
      GPIO_SetBit(GPIOE,4);
      return 4;
    }
  }
  else {
    iVar2 = GPIO_GetBit(uVar1,5);
    if (iVar2 == 1) {
      return 0;
    }
  }
  return 0;
}


// ===== btn_wash_process @ 00000c10

/* mode=0 (wash), PA9 low, PA10 high */

undefined4 btn_wash_process(void)

{
  ushort uVar1;
  char *pcVar2;
  undefined4 uVar3;
  int iVar4;
  
  iVar4 = btn_wash_read();
  pcVar2 = RAM_2000002c;
  if ((iVar4 == 4) && (*(short *)(RAM_2000002c + 6) == 0)) {
    *RAM_2000002c = '\x04';
    pcVar2[6] = '\x01';
    pcVar2[7] = '\0';
  }
  if (iVar4 == 4) {
    uVar1 = *(ushort *)(pcVar2 + 6);
    if ((uVar1 != 0) && (uVar1 <= 0x2710)) {
      *(ushort *)(pcVar2 + 6) = uVar1 + 1;
    }
  }
  if (99 < *(ushort *)(pcVar2 + 6)) {
    GPIO_ClrBit(GPIOE,4);
  }
  if (((iVar4 == 0) && (*(ushort *)(pcVar2 + 6) != 0)) && (*(ushort *)(pcVar2 + 6) < 2000)) {
    GPIO_ClrBit(GPIOE,4);
    if (*pcVar2 != '\0') {
      *RAM_20000000 = 0;
      uVar3 = GPIOA;
      GPIO_ClrBit(GPIOA,9);
      GPIO_SetBit(uVar3,10);
      *RAM_20000002 = 0;
      *RAM_20000001 = 1;
      *RAM_20000003 = 0;
      *RAM_2000000c = 0;
      pcVar2[6] = '\0';
      pcVar2[7] = '\0';
      return 1;
    }
    pcVar2[6] = '\0';
    pcVar2[7] = '\0';
  }
  else if ((iVar4 == 0) && (1999 < *(ushort *)(pcVar2 + 6))) {
    if (*pcVar2 != '\0') {
      pcVar2[6] = '\0';
      pcVar2[7] = '\0';
      return 2;
    }
    pcVar2[6] = '\0';
    pcVar2[7] = '\0';
  }
  return 0;
}


// ===== btn_cure_read @ 00000cba

/* PB4 */

undefined4 btn_cure_read(void)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = GPIOB;
  iVar2 = GPIO_GetBit(GPIOB,4);
  if (iVar2 == 0) {
    iVar2 = GPIO_GetBit(uVar1,4);
    if (iVar2 == 0) {
      GPIO_SetBit(GPIOE,4);
      return 5;
    }
  }
  else {
    iVar2 = GPIO_GetBit(uVar1,4);
    if (iVar2 == 1) {
      return 0;
    }
  }
  return 0;
}


// ===== btn_cure_process @ 00000cf8

/* mode=1 (cure), PA10 low, PA9 high */

undefined4 btn_cure_process(void)

{
  ushort uVar1;
  char *pcVar2;
  undefined4 uVar3;
  int iVar4;
  
  iVar4 = btn_cure_read();
  pcVar2 = RAM_2000002c;
  if ((iVar4 == 5) && (*(short *)(RAM_2000002c + 8) == 0)) {
    *RAM_2000002c = '\x05';
    pcVar2[8] = '\x01';
    pcVar2[9] = '\0';
  }
  if (iVar4 == 5) {
    uVar1 = *(ushort *)(pcVar2 + 8);
    if ((uVar1 != 0) && (uVar1 <= 0x2710)) {
      *(ushort *)(pcVar2 + 8) = uVar1 + 1;
    }
  }
  if (99 < *(ushort *)(pcVar2 + 8)) {
    GPIO_ClrBit(GPIOE,4);
  }
  if (((iVar4 == 0) && (*(ushort *)(pcVar2 + 8) != 0)) && (*(ushort *)(pcVar2 + 8) < 4000)) {
    GPIO_ClrBit(GPIOE,4);
    if (*pcVar2 != '\0') {
      *RAM_20000000 = 1;
      uVar3 = GPIOA;
      GPIO_ClrBit(GPIOA,10);
      GPIO_SetBit(uVar3,9);
      *RAM_20000002 = 1;
      *RAM_20000001 = 0;
      *RAM_20000003 = 0;
      *RAM_2000000c = 0;
      pcVar2[8] = '\0';
      pcVar2[9] = '\0';
      return 1;
    }
    pcVar2[8] = '\0';
    pcVar2[9] = '\0';
  }
  else if ((iVar4 == 0) && (3999 < *(ushort *)(pcVar2 + 8))) {
    if (*pcVar2 != '\0') {
      pcVar2[8] = '\0';
      pcVar2[9] = '\0';
      return 2;
    }
    pcVar2[8] = '\0';
    pcVar2[9] = '\0';
  }
  return 0;
}


// ===== display_init @ 00000dcc

/* 7-seg: segs PB7-PB14, digits PB15 PC2 PC4 PC5 */

void display_init(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  
  uVar1 = GPIOB;
  GPIO_Init(GPIOB,0xf,1,0,0,0);
  uVar2 = GPIOC;
  GPIO_Init(GPIOC,2,1,0,0,0);
  GPIO_Init(uVar2,4,1,0,0,0);
  GPIO_Init(uVar2,5,1,0,0,0);
  GPIO_Init(uVar1,0xe,1,0,0,0);
  GPIO_Init(uVar1,0xc,1,0,0,0);
  GPIO_Init(uVar1,10,1,0,0,0);
  GPIO_Init(uVar1,8,1,0,0,0);
  GPIO_Init(uVar1,7,1,0,0,0);
  GPIO_Init(uVar1,0xd,1,0,0,0);
  GPIO_Init(uVar1,0xb,1,0,0,0);
  GPIO_Init(uVar1,9,1,0,0,0);
  GPIO_SetBit(uVar1,0xf);
  GPIO_SetBit(uVar2,2);
  GPIO_SetBit(uVar2,4);
  GPIO_SetBit(uVar2,5);
  GPIO_SetBit(uVar1,0xe);
  GPIO_SetBit(uVar1,0xc);
  GPIO_SetBit(uVar1,10);
  GPIO_SetBit(uVar1,8);
  GPIO_SetBit(uVar1,7);
  GPIO_SetBit(uVar1,0xd);
  GPIO_SetBit(uVar1,0xb);
  GPIO_SetBit(uVar1,9);
  return;
}


// ===== motor_io_init @ 00000ef6

/* PD0 PD1 PA12 PA13 PA14 PA15 */

void motor_io_init(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  
  uVar1 = GPIOD;
  GPIO_Init(GPIOD,0,1,0,0,0);
  uVar2 = GPIOA;
  GPIO_Init(GPIOA,0xf,1,0,0,0);
  GPIO_Init(uVar1,1,1,0,0,0);
  GPIO_Init(uVar2,0xd,1,0,0,0);
  GPIO_Init(uVar2,0xc,1,0,0,0);
  GPIO_Init(uVar2,0xe,1,0,0,0);
  GPIO_SetBit(uVar2,0xf);
  GPIO_SetBit(uVar1,0);
  GPIO_ClrBit(uVar1,1);
  GPIO_ClrBit(uVar2,0xd);
  GPIO_SetBit(uVar2,0xc);
  return;
}


// ===== display_select_digit @ 00000f88

/* bitmask -> PB15,PC2,PC4,PC5 (active low) */

void display_select_digit(uint param_1)

{
  undefined4 uVar1;
  
  if ((int)(param_1 << 0x1c) < 0) {
    GPIO_ClrBit(GPIOB,0xf);
  }
  else {
    GPIO_SetBit(GPIOB,0xf);
  }
  uVar1 = GPIOC;
  if ((int)(param_1 << 0x1d) < 0) {
    GPIO_ClrBit(GPIOC,2);
  }
  else {
    GPIO_SetBit(GPIOC,2);
  }
  if ((int)(param_1 << 0x1e) < 0) {
    GPIO_ClrBit(uVar1,4);
  }
  else {
    GPIO_SetBit(uVar1,4);
  }
  if ((param_1 & 1) == 0) {
    GPIO_SetBit(uVar1,5);
    return;
  }
  GPIO_ClrBit(uVar1,5);
  return;
}


// ===== display_write_segments @ 00000fea

/* bit7..1 = a,b,c,d,e,f,g -> PB14,PB12,PB10,PB8,PB7,PB13,PB11 */

void display_write_segments(int param_1)

{
  undefined4 uVar1;
  
  uVar1 = GPIOB;
  if (param_1 << 0x18 < 0) {
    GPIO_SetBit(GPIOB,0xe);
  }
  else {
    GPIO_ClrBit(GPIOB,0xe);
  }
  if (param_1 << 0x19 < 0) {
    GPIO_SetBit(uVar1,0xc);
  }
  else {
    GPIO_ClrBit(uVar1,0xc);
  }
  if (param_1 << 0x1a < 0) {
    GPIO_SetBit(uVar1,10);
  }
  else {
    GPIO_ClrBit(uVar1,10);
  }
  if (param_1 << 0x1b < 0) {
    GPIO_SetBit(uVar1,8);
  }
  else {
    GPIO_ClrBit(uVar1,8);
  }
  if (param_1 << 0x1c < 0) {
    GPIO_SetBit(uVar1,7);
  }
  else {
    GPIO_ClrBit(uVar1,7);
  }
  if (param_1 << 0x1d < 0) {
    GPIO_SetBit(uVar1,0xd);
  }
  else {
    GPIO_ClrBit(uVar1,0xd);
  }
  if (param_1 << 0x1e < 0) {
    GPIO_SetBit(uVar1,0xb);
    return;
  }
  GPIO_ClrBit(uVar1,0xb);
  return;
}


// ===== display_show_pattern @ 0000108c

/* (digitmask, idx in chase table @0x20000060) */

void display_show_pattern(undefined4 param_1,int param_2)

{
  display_select_digit();
  display_write_segments(*(undefined4 *)(RAM_20000060 + param_2 * 4));
  return;
}


// ===== display_show_digit_all @ 000010a0

void display_show_digit_all(int param_1)

{
  display_select_digit(0xff);
  display_write_segments(*(undefined4 *)(RAM_20000060 + -0x28 + param_1 * 4));
  return;
}


// ===== display_refresh @ 000010b8

/* multiplex 4 digits, 2 ms each, blocking */

void display_refresh(int param_1,int param_2,int param_3,int param_4)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int iVar3;
  
  uVar1 = GPIOB;
  GPIO_ClrBit(GPIOB,9);
  GPIO_ClrBit(uVar1,0xf);
  uVar2 = GPIOC;
  GPIO_SetBit(GPIOC,2);
  GPIO_SetBit(uVar2,4);
  GPIO_SetBit(uVar2,5);
  iVar3 = RAM_20000060 + -0x28;
  display_write_segments(*(undefined4 *)(iVar3 + param_1 * 4));
  delay_ms(2);
  GPIO_SetBit(uVar1,0xf);
  GPIO_ClrBit(uVar2,2);
  GPIO_SetBit(uVar2,4);
  GPIO_SetBit(uVar2,5);
  display_write_segments(*(undefined4 *)(iVar3 + param_2 * 4));
  delay_ms(2);
  GPIO_SetBit(uVar1,0xf);
  GPIO_SetBit(uVar2,2);
  GPIO_ClrBit(uVar2,4);
  GPIO_SetBit(uVar2,5);
  display_write_segments(*(undefined4 *)(iVar3 + param_3 * 4));
  delay_ms(2);
  GPIO_SetBit(uVar1,0xf);
  GPIO_SetBit(uVar2,2);
  GPIO_SetBit(uVar2,4);
  GPIO_ClrBit(uVar2,5);
  display_write_segments(*(undefined4 *)(iVar3 + param_4 * 4));
  delay_ms(2);
  return;
}


// ===== boot_animation @ 00001190

/* segment chase */

void boot_animation(void)

{
  display_show_pattern(8,5);
  delay_ms(200);
  display_show_pattern(8,0);
  delay_ms(200);
  display_show_pattern(4,0);
  delay_ms(200);
  display_show_pattern(2,0);
  delay_ms(200);
  display_show_pattern(1,0);
  delay_ms(200);
  display_show_pattern(1);
  delay_ms(200);
  display_show_pattern(1,2);
  delay_ms(200);
  display_show_pattern(1,3);
  delay_ms(200);
  display_show_pattern(2,3);
  delay_ms(200);
  display_show_pattern(4,3);
  delay_ms(200);
  display_show_pattern(8,3);
  delay_ms(200);
  display_show_pattern(8,4);
  delay_ms(200);
  display_show_pattern(8,6);
  delay_ms(200);
  display_show_pattern(4,6);
  delay_ms(200);
  display_show_pattern(2,6);
  delay_ms(200);
  display_show_pattern(1,6);
  delay_ms(200);
  display_show_digit_all(8);
  delay_ms(500);
  display_show_digit_all(0);
  delay_ms(100);
  return;
}


// ===== motor_pwm_start_wash @ 000012a4

/* PWM1A on PC3, clkdiv 8, period 30000 */

void motor_pwm_start_wash(void)

{
  undefined4 uVar1;
  undefined1 local_1c;
  undefined1 local_1b;
  int local_18;
  int local_14;
  undefined2 local_10;
  undefined1 local_e;
  undefined1 local_d;
  undefined1 local_c;
  undefined1 local_b;
  
  SystemInit();
  uVar1 = GPIOA;
  GPIO_ClrBit(GPIOA,0xd);
  GPIO_SetBit(uVar1,0xc);
  local_1c = 0;
  local_1b = 3;
  local_18 = 0x7530;
  local_14 = 0x7530 >> 1;
  local_10 = 0;
  local_e = 1;
  local_d = 0;
  local_c = 0;
  local_b = 0;
  switchD_000018a2::PWM_Init(2,&local_1c);
  PORT_Init(2,3,2,0);
  PWM_Start(2);
  *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) | 0x303;
  PWM_SetPeriod(2,0);
  PWM_SetHDuty(2,0);
  return;
}


// ===== motor_pwm_start_cure @ 00001314

/* PWM1A on PC3, clkdiv 8, period 35156 (slow turntable) */

void motor_pwm_start_cure(void)

{
  undefined4 uVar1;
  undefined1 local_1c;
  undefined1 local_1b;
  int local_18;
  int local_14;
  undefined2 local_10;
  undefined1 local_e;
  undefined1 local_d;
  undefined1 local_c;
  undefined1 local_b;
  
  SystemInit();
  uVar1 = GPIOA;
  GPIO_ClrBit(GPIOA,0xd);
  GPIO_SetBit(uVar1,0xc);
  local_1c = 0;
  local_1b = 3;
  local_18 = 0x8954;
  local_14 = 0x8954 >> 1;
  local_10 = 0;
  local_e = 1;
  local_d = 0;
  local_c = 0;
  local_b = 0;
  switchD_000018a2::PWM_Init(2,&local_1c);
  PORT_Init(2,3,2,0);
  PWM_Start(2);
  *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) | 0x303;
  return;
}


// ===== motor_ramp_up_wash @ 000013d4

/* DIR=PD1 from g.dir; period 1000->700, 600->54 */

void motor_ramp_up_wash(void)

{
  undefined4 uVar1;
  uint *puVar2;
  char *pcVar3;
  
  uVar1 = GPIOD;
  GPIO_ClrBit(GPIOD,1);
  if (*RAM_20000005 == '\x01') {
    GPIO_SetBit(uVar1,1);
  }
  else {
    GPIO_ClrBit(uVar1,1);
  }
  puVar2 = RAM_20000028;
  *RAM_20000028 = 1000;
  pcVar3 = RAM_20000004;
  while ((700 < *puVar2 && (*pcVar3 == '\x01'))) {
    PWM_SetPeriod(2);
    PWM_SetHDuty(2,*puVar2 >> 1);
    display_refresh_n(1);
    *puVar2 = *puVar2 - 1;
  }
  *puVar2 = 600;
  while ((0x36 < *puVar2 && (*pcVar3 == '\x01'))) {
    PWM_SetPeriod(2);
    PWM_SetHDuty(2,*puVar2 >> 1);
    display_refresh_n(1);
    *puVar2 = *puVar2 - 1;
  }
  return;
}


// ===== motor_ramp_down_stop @ 00001464

/* period -> 300 then PWM off */

void motor_ramp_down_stop(void)

{
  uint *puVar1;
  
  puVar1 = RAM_20000028;
  if (*RAM_20000028 != 0) {
    while (*puVar1 < 300) {
      PWM_SetPeriod(2);
      PWM_SetHDuty(2,*puVar1 >> 1);
      display_refresh_n(1);
      *puVar1 = *puVar1 + 1;
    }
    PWM_SetPeriod(2,0);
    PWM_SetHDuty(2,0);
    *puVar1 = 0;
  }
  return;
}


// ===== motor_ramp_down_reverse @ 000014ac

/* period 54 -> 500 then off */

void motor_ramp_down_reverse(void)

{
  uint *puVar1;
  uint uVar2;
  
  puVar1 = RAM_20000028;
  *RAM_20000028 = 0x36;
  do {
    PWM_SetPeriod(2,*puVar1);
    PWM_SetHDuty(2,*puVar1 >> 1);
    display_refresh_n(1);
    uVar2 = *puVar1;
    *puVar1 = uVar2 + 1;
  } while (uVar2 + 1 < 500);
  PWM_SetPeriod(2,0);
  PWM_SetHDuty(2,0);
  *puVar1 = 0;
  return;
}


// ===== GPIO_Init @ 00001514

/* (GPIOx, pin, dir, pullup, pulldown, opendrain) - Synwit SDK */

void GPIO_Init(int param_1,uint param_2,int param_3,int param_4,int param_5,int param_6)

{
  int iVar1;
  uint *puVar2;
  int iVar3;
  uint uVar4;
  
  iVar1 = GPIOC;
  iVar3 = GPIOC * 0x40000;
  uVar4 = 1 << (param_2 & 0xff);
  if (param_1 == GPIOC) {
    *(uint *)(iVar3 + 8) = *(uint *)(iVar3 + 8) | 4;
    PORT_Init(2,param_2,0,1,param_4);
    if (param_3 == 1) {
      *(uint *)(iVar1 + 4) = *(uint *)(iVar1 + 4) | uVar4;
    }
    else {
      *(uint *)(iVar1 + 4) = *(uint *)(iVar1 + 4) & ~uVar4;
    }
    if (param_4 == 1) {
      0x400a0100[8] = 0x400a0100[8] | uVar4;
    }
    else {
      0x400a0100[8] = 0x400a0100[8] & ~uVar4;
    }
    if (param_5 == 1) {
      0x400a0200[8] = 0x400a0200[8] | uVar4;
    }
    else {
      0x400a0200[8] = 0x400a0200[8] & ~uVar4;
    }
    if (param_6 == 1) {
      0x400a0400[8] = 0x400a0400[8] | uVar4;
      return;
    }
    0x400a0400[8] = 0x400a0400[8] & ~uVar4;
    return;
  }
  if (param_1 < GPIOC) {
    if (param_1 + 0xbffc0000 == 0) {
      *(uint *)(iVar3 + 8) = *(uint *)(iVar3 + 8) | 1;
      PORT_Init(0,param_2,0,1,param_4);
      iVar1 = -0xbffc0000;
      if (param_3 == 1) {
        *(uint *)(iVar1 + 4) = *(uint *)(iVar1 + 4) | uVar4;
      }
      else {
        *(uint *)(iVar1 + 4) = *(uint *)(iVar1 + 4) & ~uVar4;
      }
      if (param_4 == 1) {
        *0x400a0100 = *0x400a0100 | uVar4;
      }
      else {
        *0x400a0100 = *0x400a0100 & ~uVar4;
      }
      if (param_5 == 1) {
        *0x400a0200 = *0x400a0200 | uVar4;
      }
      else {
        *0x400a0200 = *0x400a0200 & ~uVar4;
      }
      if (param_6 == 1) {
        *0x400a0400 = *0x400a0400 | uVar4;
        return;
      }
      *0x400a0400 = *0x400a0400 & ~uVar4;
      return;
    }
    if (param_1 + 0xbffc0000 == 0x800) {
      *(uint *)(iVar3 + 8) = *(uint *)(iVar3 + 8) | 2;
      PORT_Init(1,param_2,0,1,param_4);
      if (param_3 == 1) {
        *(uint *)(GPIOB + 4) = *(uint *)(GPIOB + 4) | uVar4;
      }
      else {
        *(uint *)(GPIOB + 4) = *(uint *)(GPIOB + 4) & ~uVar4;
      }
      if (param_4 == 1) {
        0x400a0100[4] = 0x400a0100[4] | uVar4;
      }
      else {
        0x400a0100[4] = 0x400a0100[4] & ~uVar4;
      }
      if (param_5 == 1) {
        0x400a0200[4] = 0x400a0200[4] | uVar4;
      }
      else {
        0x400a0200[4] = 0x400a0200[4] & ~uVar4;
      }
      if (param_6 == 1) {
        0x400a0400[4] = 0x400a0400[4] | uVar4;
        return;
      }
      0x400a0400[4] = 0x400a0400[4] & ~uVar4;
    }
  }
  else {
    if (param_1 - GPIOC == 0x800) {
      *(uint *)(iVar3 + 8) = *(uint *)(iVar3 + 8) | 8;
      PORT_Init(3,param_2,0,1,param_4);
      if (param_3 == 1) {
        *(uint *)(GPIOD + 4) = *(uint *)(GPIOD + 4) | uVar4;
      }
      else {
        *(uint *)(GPIOD + 4) = *(uint *)(GPIOD + 4) & ~uVar4;
      }
      if (param_4 == 1) {
        0x400a0100[0xc] = 0x400a0100[0xc] | uVar4;
      }
      else {
        0x400a0100[0xc] = 0x400a0100[0xc] & ~uVar4;
      }
      if (param_5 == 1) {
        0x400a0200[0xc] = 0x400a0200[0xc] | uVar4;
      }
      else {
        0x400a0200[0xc] = 0x400a0200[0xc] & ~uVar4;
      }
      if (param_6 == 1) {
        0x400a0400[0xc] = 0x400a0400[0xc] | uVar4;
        return;
      }
      0x400a0400[0xc] = 0x400a0400[0xc] & ~uVar4;
      return;
    }
    if (param_1 - GPIOC == 0x60000) {
      *(uint *)(iVar3 + 0xc) = *(uint *)(iVar3 + 0xc) | 1;
      PORT_Init(4,param_2,0,1,param_4);
      if (param_3 == 1) {
        *(uint *)(GPIOE + 4) = *(uint *)(GPIOE + 4) | uVar4;
      }
      else {
        *(uint *)(GPIOE + 4) = *(uint *)(GPIOE + 4) & ~uVar4;
      }
      puVar2 = 0x400a0100 + 0x10;
      if (param_4 == 1) {
        *puVar2 = *puVar2 | uVar4;
      }
      else {
        *puVar2 = *puVar2 & ~uVar4;
      }
      puVar2 = 0x400a0200 + 0x10;
      if (param_5 == 1) {
        *puVar2 = *puVar2 | uVar4;
      }
      else {
        *puVar2 = *puVar2 & ~uVar4;
      }
      puVar2 = 0x400a0400 + 0x10;
      if (param_6 == 1) {
        *puVar2 = *puVar2 | uVar4;
        return;
      }
      *puVar2 = *puVar2 & ~uVar4;
      return;
    }
  }
  return;
}


// ===== GPIO_SetBit @ 00001786

void GPIO_SetBit(uint *param_1,uint param_2)

{
  *param_1 = *param_1 | 1 << (param_2 & 0xff);
  return;
}


// ===== GPIO_ClrBit @ 00001792

void GPIO_ClrBit(uint *param_1,uint param_2)

{
  *param_1 = *param_1 & ~(1 << (param_2 & 0xff));
  return;
}


// ===== GPIO_InvBit @ 0000179e

void GPIO_InvBit(uint *param_1,uint param_2)

{
  *param_1 = *param_1 ^ 1 << (param_2 & 0xff);
  return;
}


// ===== GPIO_GetBit @ 000017aa

uint GPIO_GetBit(int param_1,uint param_2)

{
  return *(uint *)(param_1 + 0x30) >> (param_2 & 0xff) & 1;
}


// ===== PORT_Init @ 00001884

/* WARNING: Function: __ARM_common_switch8 replaced with injection: switch8_r3 */
/* WARNING (jumptable): Removing unreachable block (ram,0x000018a2) */
/* WARNING: Removing unreachable block (ram,0x000018a2) */
/* (port idx, pin, func, digital_in_en) */

code * PORT_Init(code *param_1,dword *param_2,int param_3,code *param_4)

{
  uint uVar1;
  code *pcVar2;
  code *pcVar3;
  code *pcVar4;
  uint uVar5;
  code *pcVar6;
  code *extraout_r3;
  code *pcVar7;
  code *pcVar8;
  code *pcVar9;
  uint uVar10;
  bool bVar11;
  bool bVar12;
  undefined8 uVar13;
  undefined1 in_q4 [16];
  
  pcVar8 = (code *)(0xf << ((int)param_2 << 2 & 0xffU));
  pcVar7 = (code *)(param_3 << ((int)param_2 << 2 & 0xffU));
  uVar5 = (int)((int)param_2 + -8) * 4;
  uVar1 = uVar5 & 0xff;
  pcVar9 = (code *)(0xf << uVar1);
  uVar5 = uVar5 & 0xff;
  pcVar4 = (code *)(param_3 << uVar5);
  uVar10 = (uint)param_2 & 0xff;
  bVar12 = uVar10 == 0 &&
           (uVar5 == 0 &&
            (uVar1 == 0 && (((int)param_2 + -8) * 2 & 0x80000000U) != 0 ||
            uVar1 != 0 && (0xf << uVar1 - 1 & 0x80000000U) != 0) ||
           uVar5 != 0 && (param_3 << uVar5 - 1 & 0x80000000U) != 0) ||
           uVar10 != 0 && (1 << uVar10 - 1 & 0x80000000U) != 0;
  uVar10 = 1 << uVar10;
  bVar11 = param_1 == (code *)0x0;
  pcVar3 = param_1;
  if ((code *)(uint)bRam000018a6 <= param_1) {
    pcVar3 = (code *)(uint)bRam000018a6;
  }
  pcVar6 = (code *)((uint)(byte)pcVar3[0x18a7] * 2);
  pcVar2 = param_1;
  switch(pcVar3[0x18a7]) {
  case (code)0x0:
    pcVar8 = (code *)((int)param_1 << 0x10);
  case (code)0x1:
    pcVar4 = (code *)((uint)pcVar4 & (uint)pcVar7);
switchD_000018a2_caseD_2:
    pcVar9 = (code *)(uint)(byte)pcVar6[0x11];
switchD_000018a2_caseD_3:
    pcVar7 = pcVar6;
switchD_000018a2_caseD_4:
    bVar12 = (code *)((int)&Reset + 3) < param_2;
switchD_000018a2_caseD_5:
    if (bVar12) {
switchD_000018a2_caseD_e:
      pcVar2 = pcRam000019dc;
switchD_000018a2_caseD_f:
      param_2 = *(dword **)(pcVar2 + 4);
switchD_000018a2_caseD_10:
      param_2 = (dword *)((uint)param_2 & ~(uint)pcVar9);
switchD_000018a2_caseD_11:
      *(dword **)(pcVar2 + 4) = param_2;
switchD_000018a2_caseD_12:
      param_2 = *(dword **)(pcVar2 + 4);
switchD_000018a2_caseD_13:
      param_2 = (dword *)((uint)param_2 | (uint)pcVar4);
switchD_000018a2_caseD_14:
      *(dword **)(pcVar2 + 4) = param_2;
    }
    else {
switchD_000018a2_caseD_6:
      pcVar2 = pcRam000019dc;
switchD_000018a2_caseD_7:
      param_2 = *(dword **)pcVar2;
switchD_000018a2_caseD_8:
      param_2 = (dword *)((uint)param_2 & ~(uint)pcVar8);
switchD_000018a2_caseD_9:
      *(dword **)pcVar2 = param_2;
switchD_000018a2_caseD_a:
      param_2 = *(dword **)pcVar2;
switchD_000018a2_caseD_b:
      param_2 = (dword *)((uint)param_2 | (uint)pcVar7);
switchD_000018a2_caseD_c:
      *(dword **)pcVar2 = param_2;
switchD_000018a2_caseD_d:
    }
switchD_000018a2_caseD_15:
    pcVar2 = param_4;
switchD_000018a2_caseD_16:
    bVar11 = pcVar2 == (code *)0x0;
switchD_000018a2_caseD_17:
    if (bVar11) {
switchD_000018a2_caseD_1d:
      pcVar2 = pcRam000019e0;
switchD_000018a2_caseD_1e:
      param_2 = *(dword **)pcVar2;
switchD_000018a2_caseD_1f:
      param_2 = (dword *)((uint)param_2 & ~uVar10);
switchD_000018a2_caseD_20:
      *(dword **)pcVar2 = param_2;
switchD_000018a2_caseD_21:
      return param_1;
    }
  case (code)0x18:
    pcVar2 = pcRam000019e0;
  case (code)0x19:
    param_2 = *(dword **)pcVar2;
  case (code)0x1a:
    param_2 = (dword *)((uint)param_2 | uVar10);
  case (code)0x1b:
    *(dword **)pcVar2 = param_2;
switchD_000018a2_caseD_1c:
    return param_1;
  case (code)0x2:
    goto switchD_000018a2_caseD_2;
  case (code)0x3:
    goto switchD_000018a2_caseD_3;
  case (code)0x4:
    goto switchD_000018a2_caseD_4;
  case (code)0x5:
    goto switchD_000018a2_caseD_5;
  case (code)0x6:
    goto switchD_000018a2_caseD_6;
  case (code)0x7:
    goto switchD_000018a2_caseD_7;
  case (code)0x8:
    goto switchD_000018a2_caseD_8;
  case (code)0x9:
    goto switchD_000018a2_caseD_9;
  case (code)0xa:
    goto switchD_000018a2_caseD_a;
  case (code)0xb:
    goto switchD_000018a2_caseD_b;
  case (code)0xc:
    goto switchD_000018a2_caseD_c;
  case (code)0xd:
    goto switchD_000018a2_caseD_d;
  case (code)0xe:
    goto switchD_000018a2_caseD_e;
  case (code)0xf:
    goto switchD_000018a2_caseD_f;
  case (code)0x10:
    goto switchD_000018a2_caseD_10;
  case (code)0x11:
    goto switchD_000018a2_caseD_11;
  case (code)0x12:
    goto switchD_000018a2_caseD_12;
  case (code)0x13:
    goto switchD_000018a2_caseD_13;
  case (code)0x14:
    goto switchD_000018a2_caseD_14;
  case (code)0x15:
    goto switchD_000018a2_caseD_15;
  case (code)0x16:
    goto switchD_000018a2_caseD_16;
  case (code)0x17:
    goto switchD_000018a2_caseD_17;
  case (code)0x1c:
    goto switchD_000018a2_caseD_1c;
  case (code)0x1d:
    goto switchD_000018a2_caseD_1d;
  case (code)0x1e:
    goto switchD_000018a2_caseD_1e;
  case (code)0x1f:
    goto switchD_000018a2_caseD_1f;
  case (code)0x20:
    goto switchD_000018a2_caseD_20;
  case (code)0x21:
    goto switchD_000018a2_caseD_21;
  case (code)0x22:
    bVar12 = (code *)((int)&Reset + 3) < param_2;
  case (code)0x23:
    if (bVar12) {
switchD_000018a2_caseD_2c:
      pcVar2 = pcRam000019dc;
switchD_000018a2_caseD_2d:
      param_2 = *(dword **)(pcVar2 + 0x14);
switchD_000018a2_caseD_2e:
      param_2 = (dword *)((uint)param_2 & ~(uint)pcVar9);
switchD_000018a2_caseD_2f:
      *(dword **)(pcVar2 + 0x14) = param_2;
switchD_000018a2_caseD_30:
      param_2 = *(dword **)(pcVar2 + 0x14);
switchD_000018a2_caseD_31:
      param_2 = (dword *)((uint)param_2 | (uint)pcVar4);
switchD_000018a2_caseD_32:
      *(dword **)(pcVar2 + 0x14) = param_2;
    }
    else {
switchD_000018a2_caseD_24:
      pcVar2 = pcRam000019dc;
switchD_000018a2_caseD_25:
      param_2 = *(dword **)(pcVar2 + 0x10);
switchD_000018a2_caseD_26:
      param_2 = (dword *)((uint)param_2 & ~(uint)pcVar8);
switchD_000018a2_caseD_27:
      *(dword **)(pcVar2 + 0x10) = param_2;
switchD_000018a2_caseD_28:
      param_2 = *(dword **)(pcVar2 + 0x10);
switchD_000018a2_caseD_29:
      param_2 = (dword *)((uint)param_2 | (uint)pcVar7);
switchD_000018a2_caseD_2a:
      *(dword **)(pcVar2 + 0x10) = param_2;
switchD_000018a2_caseD_2b:
    }
switchD_000018a2_caseD_33:
    pcVar2 = param_4;
switchD_000018a2_caseD_34:
    bVar11 = pcVar2 == (code *)0x0;
switchD_000018a2_caseD_35:
    if (bVar11) {
switchD_000018a2_caseD_3b:
      pcVar2 = pcRam000019e0;
switchD_000018a2_caseD_3c:
      param_2 = *(dword **)(pcVar2 + 0x10);
switchD_000018a2_caseD_3d:
      param_2 = (dword *)((uint)param_2 & ~uVar10);
switchD_000018a2_caseD_3e:
      *(dword **)(pcVar2 + 0x10) = param_2;
switchD_000018a2_caseD_3f:
      return param_1;
    }
switchD_000018a2_caseD_36:
    pcVar2 = pcRam000019e0;
switchD_000018a2_caseD_37:
    param_2 = *(dword **)(pcVar2 + 0x10);
switchD_000018a2_caseD_38:
    param_2 = (dword *)((uint)param_2 | uVar10);
switchD_000018a2_caseD_39:
    *(dword **)(pcVar2 + 0x10) = param_2;
switchD_000018a2_caseD_3a:
    return param_1;
  case (code)0x24:
    goto switchD_000018a2_caseD_24;
  case (code)0x25:
    goto switchD_000018a2_caseD_25;
  case (code)0x26:
    goto switchD_000018a2_caseD_26;
  case (code)0x27:
    goto switchD_000018a2_caseD_27;
  case (code)0x28:
    goto switchD_000018a2_caseD_28;
  case (code)0x29:
    goto switchD_000018a2_caseD_29;
  case (code)0x2a:
    goto switchD_000018a2_caseD_2a;
  case (code)0x2b:
    goto switchD_000018a2_caseD_2b;
  case (code)0x2c:
    goto switchD_000018a2_caseD_2c;
  case (code)0x2d:
    goto switchD_000018a2_caseD_2d;
  case (code)0x2e:
    goto switchD_000018a2_caseD_2e;
  case (code)0x2f:
    goto switchD_000018a2_caseD_2f;
  case (code)0x30:
    goto switchD_000018a2_caseD_30;
  case (code)0x31:
    goto switchD_000018a2_caseD_31;
  case (code)0x32:
    goto switchD_000018a2_caseD_32;
  case (code)0x33:
    goto switchD_000018a2_caseD_33;
  case (code)0x34:
    goto switchD_000018a2_caseD_34;
  case (code)0x35:
    goto switchD_000018a2_caseD_35;
  case (code)0x36:
    goto switchD_000018a2_caseD_36;
  case (code)0x37:
    goto switchD_000018a2_caseD_37;
  case (code)0x38:
    goto switchD_000018a2_caseD_38;
  case (code)0x39:
    goto switchD_000018a2_caseD_39;
  case (code)0x3a:
    goto switchD_000018a2_caseD_3a;
  case (code)0x3b:
    goto switchD_000018a2_caseD_3b;
  case (code)0x3c:
    goto switchD_000018a2_caseD_3c;
  case (code)0x3d:
    goto switchD_000018a2_caseD_3d;
  case (code)0x3e:
    goto switchD_000018a2_caseD_3e;
  case (code)0x3f:
    goto switchD_000018a2_caseD_3f;
  case (code)0x40:
    bVar12 = (code *)((int)&Reset + 3) < param_2;
  case (code)0x41:
    if (bVar12) {
switchD_000018a2_caseD_4a:
      pcVar2 = pcRam000019dc;
switchD_000018a2_caseD_4b:
      param_2 = *(dword **)(pcVar2 + 0x24);
switchD_000018a2_caseD_4c:
      param_2 = (dword *)((uint)param_2 & ~(uint)pcVar9);
switchD_000018a2_caseD_4d:
      *(dword **)(pcVar2 + 0x24) = param_2;
switchD_000018a2_caseD_4e:
      param_2 = *(dword **)(pcVar2 + 0x24);
switchD_000018a2_caseD_4f:
      param_2 = (dword *)((uint)param_2 | (uint)pcVar4);
switchD_000018a2_caseD_50:
      *(dword **)(pcVar2 + 0x24) = param_2;
    }
    else {
switchD_000018a2_caseD_42:
      pcVar2 = pcRam000019dc;
switchD_000018a2_caseD_43:
      param_2 = *(dword **)(pcVar2 + 0x20);
switchD_000018a2_caseD_44:
      param_2 = (dword *)((uint)param_2 & ~(uint)pcVar8);
switchD_000018a2_caseD_45:
      *(dword **)(pcVar2 + 0x20) = param_2;
switchD_000018a2_caseD_46:
      param_2 = *(dword **)(pcVar2 + 0x20);
switchD_000018a2_caseD_47:
      param_2 = (dword *)((uint)param_2 | (uint)pcVar7);
switchD_000018a2_caseD_48:
      *(dword **)(pcVar2 + 0x20) = param_2;
switchD_000018a2_caseD_49:
    }
switchD_000018a2_caseD_51:
    pcVar2 = param_4;
switchD_000018a2_caseD_52:
    bVar11 = pcVar2 == (code *)0x0;
switchD_000018a2_caseD_53:
    if (bVar11) {
switchD_000018a2_caseD_59:
      pcVar2 = pcRam000019e0;
switchD_000018a2_caseD_5a:
      param_2 = *(dword **)(pcVar2 + 0x20);
switchD_000018a2_caseD_5b:
      param_2 = (dword *)((uint)param_2 & ~uVar10);
switchD_000018a2_caseD_5c:
      *(dword **)(pcVar2 + 0x20) = param_2;
switchD_000018a2_caseD_5d:
      return param_1;
    }
switchD_000018a2_caseD_54:
    pcVar2 = pcRam000019e0;
switchD_000018a2_caseD_55:
    param_2 = *(dword **)(pcVar2 + 0x20);
switchD_000018a2_caseD_56:
    param_2 = (dword *)((uint)param_2 | uVar10);
switchD_000018a2_caseD_57:
    *(dword **)(pcVar2 + 0x20) = param_2;
switchD_000018a2_caseD_58:
    return param_1;
  case (code)0x42:
    goto switchD_000018a2_caseD_42;
  case (code)0x43:
    goto switchD_000018a2_caseD_43;
  case (code)0x44:
    goto switchD_000018a2_caseD_44;
  case (code)0x45:
    goto switchD_000018a2_caseD_45;
  case (code)0x46:
    goto switchD_000018a2_caseD_46;
  case (code)0x47:
    goto switchD_000018a2_caseD_47;
  case (code)0x48:
    goto switchD_000018a2_caseD_48;
  case (code)0x49:
    goto switchD_000018a2_caseD_49;
  case (code)0x4a:
    goto switchD_000018a2_caseD_4a;
  case (code)0x4b:
    goto switchD_000018a2_caseD_4b;
  case (code)0x4c:
    goto switchD_000018a2_caseD_4c;
  case (code)0x4d:
    goto switchD_000018a2_caseD_4d;
  case (code)0x4e:
    goto switchD_000018a2_caseD_4e;
  case (code)0x4f:
    goto switchD_000018a2_caseD_4f;
  case (code)0x50:
    goto switchD_000018a2_caseD_50;
  case (code)0x51:
    goto switchD_000018a2_caseD_51;
  case (code)0x52:
    goto switchD_000018a2_caseD_52;
  case (code)0x53:
    goto switchD_000018a2_caseD_53;
  case (code)0x54:
    goto switchD_000018a2_caseD_54;
  case (code)0x55:
    goto switchD_000018a2_caseD_55;
  case (code)0x56:
    goto switchD_000018a2_caseD_56;
  case (code)0x57:
    goto switchD_000018a2_caseD_57;
  case (code)0x58:
    goto switchD_000018a2_caseD_58;
  case (code)0x59:
    goto switchD_000018a2_caseD_59;
  case (code)0x5a:
    goto switchD_000018a2_caseD_5a;
  case (code)0x5b:
    goto switchD_000018a2_caseD_5b;
  case (code)0x5c:
    goto switchD_000018a2_caseD_5c;
  case (code)0x5d:
    goto switchD_000018a2_caseD_5d;
  case (code)0x5e:
    bVar12 = (code *)((int)&Reset + 3) < param_2;
  case (code)0x5f:
    if (bVar12) {
switchD_000018a2_caseD_68:
      pcVar2 = pcRam000019dc;
switchD_000018a2_caseD_69:
      param_2 = *(dword **)(pcVar2 + 0x34);
switchD_000018a2_caseD_6a:
      param_2 = (dword *)((uint)param_2 & ~(uint)pcVar9);
switchD_000018a2_caseD_6b:
      *(dword **)(pcVar2 + 0x34) = param_2;
switchD_000018a2_caseD_6c:
      param_2 = *(dword **)(pcVar2 + 0x34);
switchD_000018a2_caseD_6d:
      param_2 = (dword *)((uint)param_2 | (uint)pcVar4);
switchD_000018a2_caseD_6e:
      *(dword **)(pcVar2 + 0x34) = param_2;
    }
    else {
switchD_000018a2_caseD_60:
      pcVar2 = pcRam000019dc;
switchD_000018a2_caseD_61:
      param_2 = *(dword **)(pcVar2 + 0x30);
switchD_000018a2_caseD_62:
      param_2 = (dword *)((uint)param_2 & ~(uint)pcVar8);
switchD_000018a2_caseD_63:
      *(dword **)(pcVar2 + 0x30) = param_2;
switchD_000018a2_caseD_64:
      param_2 = *(dword **)(pcVar2 + 0x30);
switchD_000018a2_caseD_65:
      param_2 = (dword *)((uint)param_2 | (uint)pcVar7);
switchD_000018a2_caseD_66:
      *(dword **)(pcVar2 + 0x30) = param_2;
switchD_000018a2_caseD_67:
    }
switchD_000018a2_caseD_6f:
    pcVar2 = param_4;
switchD_000018a2_caseD_70:
    bVar11 = pcVar2 == (code *)0x0;
switchD_000018a2_caseD_71:
    if (bVar11) {
switchD_000018a2_caseD_77:
      pcVar2 = pcRam000019e0;
switchD_000018a2_caseD_78:
      param_2 = *(dword **)(pcVar2 + 0x30);
switchD_000018a2_caseD_79:
      param_2 = (dword *)((uint)param_2 & ~uVar10);
switchD_000018a2_caseD_7a:
      *(dword **)(pcVar2 + 0x30) = param_2;
switchD_000018a2_caseD_7b:
      return param_1;
    }
switchD_000018a2_caseD_72:
    pcVar2 = pcRam000019e0;
switchD_000018a2_caseD_73:
    param_2 = *(dword **)(pcVar2 + 0x30);
switchD_000018a2_caseD_74:
    param_2 = (dword *)((uint)param_2 | uVar10);
switchD_000018a2_caseD_75:
    *(dword **)(pcVar2 + 0x30) = param_2;
switchD_000018a2_caseD_76:
    return param_1;
  case (code)0x60:
    goto switchD_000018a2_caseD_60;
  case (code)0x61:
    goto switchD_000018a2_caseD_61;
  case (code)0x62:
    goto switchD_000018a2_caseD_62;
  case (code)0x63:
    goto switchD_000018a2_caseD_63;
  case (code)0x64:
    goto switchD_000018a2_caseD_64;
  case (code)0x65:
    goto switchD_000018a2_caseD_65;
  case (code)0x66:
    goto switchD_000018a2_caseD_66;
  case (code)0x67:
    goto switchD_000018a2_caseD_67;
  case (code)0x68:
    goto switchD_000018a2_caseD_68;
  case (code)0x69:
    goto switchD_000018a2_caseD_69;
  case (code)0x6a:
    goto switchD_000018a2_caseD_6a;
  case (code)0x6b:
    goto switchD_000018a2_caseD_6b;
  case (code)0x6c:
    goto switchD_000018a2_caseD_6c;
  case (code)0x6d:
    goto switchD_000018a2_caseD_6d;
  case (code)0x6e:
    goto switchD_000018a2_caseD_6e;
  case (code)0x6f:
    goto switchD_000018a2_caseD_6f;
  case (code)0x70:
    goto switchD_000018a2_caseD_70;
  case (code)0x71:
    goto switchD_000018a2_caseD_71;
  case (code)0x72:
    goto switchD_000018a2_caseD_72;
  case (code)0x73:
    goto switchD_000018a2_caseD_73;
  case (code)0x74:
    goto switchD_000018a2_caseD_74;
  case (code)0x75:
    goto switchD_000018a2_caseD_75;
  case (code)0x76:
    goto switchD_000018a2_caseD_76;
  case (code)0x77:
    goto switchD_000018a2_caseD_77;
  case (code)0x78:
    goto switchD_000018a2_caseD_78;
  case (code)0x79:
    goto switchD_000018a2_caseD_79;
  case (code)0x7a:
    goto switchD_000018a2_caseD_7a;
  case (code)0x7b:
    goto switchD_000018a2_caseD_7b;
  case (code)0x7c:
    pcVar2 = pcRam000019dc;
  case (code)0x7d:
    pcVar2 = pcVar2 + 0x40;
switchD_000018a2_caseD_7e:
    bVar12 = (code *)((int)&Reset + 3) < param_2;
switchD_000018a2_caseD_7f:
    if (bVar12) {
switchD_000018a2_caseD_87:
      param_2 = *(dword **)(pcVar2 + 4);
switchD_000018a2_caseD_88:
      param_2 = (dword *)((uint)param_2 & ~(uint)pcVar9);
switchD_000018a2_caseD_89:
      *(dword **)(pcVar2 + 4) = param_2;
switchD_000018a2_caseD_8a:
      param_2 = *(dword **)(pcVar2 + 4);
switchD_000018a2_caseD_8b:
      param_2 = (dword *)((uint)param_2 | (uint)pcVar4);
switchD_000018a2_caseD_8c:
      *(dword **)(pcVar2 + 4) = param_2;
    }
    else {
switchD_000018a2_caseD_80:
      param_2 = *(dword **)pcVar2;
switchD_000018a2_caseD_81:
      param_2 = (dword *)((uint)param_2 & ~(uint)pcVar8);
switchD_000018a2_caseD_82:
      *(dword **)pcVar2 = param_2;
switchD_000018a2_caseD_83:
      param_2 = *(dword **)pcVar2;
switchD_000018a2_caseD_84:
      param_2 = (dword *)((uint)param_2 | (uint)pcVar7);
switchD_000018a2_caseD_85:
      *(dword **)pcVar2 = param_2;
switchD_000018a2_caseD_86:
    }
switchD_000018a2_caseD_8d:
    pcVar2 = pcRam000019e0;
switchD_000018a2_caseD_8e:
    param_2 = (dword *)param_4;
switchD_000018a2_caseD_8f:
    pcVar2 = pcVar2 + 0x40;
switchD_000018a2_caseD_90:
    bVar11 = (code *)param_2 == (code *)0x0;
switchD_000018a2_caseD_91:
    if (bVar11) {
switchD_000018a2_caseD_96:
      param_2 = *(dword **)pcVar2;
switchD_000018a2_caseD_97:
      param_2 = (dword *)((uint)param_2 & ~uVar10);
switchD_000018a2_caseD_98:
      *(dword **)pcVar2 = param_2;
switchD_000018a2_caseD_99:
      return param_1;
    }
switchD_000018a2_caseD_92:
    param_2 = *(dword **)pcVar2;
switchD_000018a2_caseD_93:
    param_2 = (dword *)((uint)param_2 | uVar10);
switchD_000018a2_caseD_94:
    *(dword **)pcVar2 = param_2;
switchD_000018a2_caseD_95:
    return param_1;
  case (code)0x7e:
    goto switchD_000018a2_caseD_7e;
  case (code)0x7f:
    goto switchD_000018a2_caseD_7f;
  case (code)0x80:
    goto switchD_000018a2_caseD_80;
  case (code)0x81:
    goto switchD_000018a2_caseD_81;
  case (code)0x82:
    goto switchD_000018a2_caseD_82;
  case (code)0x83:
    goto switchD_000018a2_caseD_83;
  case (code)0x84:
    goto switchD_000018a2_caseD_84;
  case (code)0x85:
    goto switchD_000018a2_caseD_85;
  case (code)0x86:
    goto switchD_000018a2_caseD_86;
  case (code)0x87:
    goto switchD_000018a2_caseD_87;
  case (code)0x88:
    goto switchD_000018a2_caseD_88;
  case (code)0x89:
    goto switchD_000018a2_caseD_89;
  case (code)0x8a:
    goto switchD_000018a2_caseD_8a;
  case (code)0x8b:
    goto switchD_000018a2_caseD_8b;
  case (code)0x8c:
    goto switchD_000018a2_caseD_8c;
  case (code)0x8d:
    goto switchD_000018a2_caseD_8d;
  case (code)0x8e:
    goto switchD_000018a2_caseD_8e;
  case (code)0x8f:
    goto switchD_000018a2_caseD_8f;
  case (code)0x90:
    goto switchD_000018a2_caseD_90;
  case (code)0x91:
    goto switchD_000018a2_caseD_91;
  case (code)0x92:
    goto switchD_000018a2_caseD_92;
  case (code)0x93:
    goto switchD_000018a2_caseD_93;
  case (code)0x94:
    goto switchD_000018a2_caseD_94;
  case (code)0x95:
    goto switchD_000018a2_caseD_95;
  case (code)0x96:
    goto switchD_000018a2_caseD_96;
  case (code)0x97:
    goto switchD_000018a2_caseD_97;
  case (code)0x98:
    goto switchD_000018a2_caseD_98;
  case (code)0x99:
    goto switchD_000018a2_caseD_99;
  case (code)0x9a:
  case (code)0x9b:
switchD_000018a2_caseD_9c:
code_r0x000019e0:
    param_1 = (code *)((int)param_1 << 0xc);
switchD_000018a2_caseD_9e:
code_r0x000019e4:
    param_2 = (dword *)RAM_2000007c;
code_r0x000019e6:
    pcVar4 = (code *)((int)param_1 << 2);
code_r0x000019e8:
    param_2 = *(dword **)((int)param_2 + (int)pcVar4);
code_r0x000019ea:
    pcVar4 = 0xbffba000;
switchD_000018a2_caseD_a3:
    param_2 = (dword *)((int)param_2 + (int)pcVar4);
    bVar11 = (code *)param_2 == (code *)0x0;
switchD_000018a2_caseD_a4:
    pcVar4 = PWMG;
switchD_000018a2_caseD_a5:
    if (bVar11) {
switchD_000018a2_caseD_b3:
      bVar11 = ((uint)param_1 & 1) == 0;
switchD_000018a2_caseD_b4:
      if (bVar11) {
switchD_000018a2_caseD_bb:
        param_1 = *(code **)(pcVar4 + 0x20);
switchD_000018a2_caseD_bc:
        param_1 = (code *)((uint)param_1 >> 1);
switchD_000018a2_caseD_bd:
        param_1 = (code *)((int)param_1 << 1);
switchD_000018a2_caseD_be:
        *(code **)(pcVar4 + 0x20) = param_1;
switchD_000018a2_caseD_bf:
        return param_1;
      }
switchD_000018a2_caseD_b5:
      param_1 = *(code **)(pcVar4 + 0x20);
switchD_000018a2_caseD_b6:
      param_2 = (dword *)((int)&0xeb050206 + 3);
switchD_000018a2_caseD_b7:
      param_2 = (dword *)((int)param_2 + 1);
switchD_000018a2_caseD_b8:
      param_1 = (code *)((uint)param_1 & ~(uint)param_2);
switchD_000018a2_caseD_b9:
      *(code **)(pcVar4 + 0x20) = param_1;
switchD_000018a2_caseD_ba:
      return param_1;
    }
switchD_000018a2_caseD_a6:
    bVar11 = param_2 == &IRQ;
switchD_000018a2_caseD_a7:
    if (bVar11) {
switchD_000018a2_caseD_c0:
      bVar11 = ((uint)param_1 & 1) == 0;
switchD_000018a2_caseD_c1:
      if (bVar11) {
switchD_000018a2_caseD_c8:
        param_1 = *(code **)(pcVar4 + 0x20);
switchD_000018a2_caseD_c9:
        param_2 = (dword *)&UNK_00000002;
switchD_000018a2_caseD_ca:
        param_1 = (code *)((uint)param_1 & ~(uint)param_2);
switchD_000018a2_caseD_cb:
        *(code **)(pcVar4 + 0x20) = param_1;
switchD_000018a2_caseD_cc:
        return param_1;
      }
switchD_000018a2_caseD_c2:
      param_1 = *(code **)(pcVar4 + 0x20);
switchD_000018a2_caseD_c3:
      param_2 = (dword *)&UNK_00000001;
switchD_000018a2_caseD_c4:
      param_2 = (dword *)((int)param_2 << 9);
switchD_000018a2_caseD_c5:
      param_1 = (code *)((uint)param_1 & ~(uint)param_2);
switchD_000018a2_caseD_c6:
      *(code **)(pcVar4 + 0x20) = param_1;
switchD_000018a2_caseD_c7:
      return param_1;
    }
switchD_000018a2_caseD_a8:
    bVar11 = param_2 == &DWORD_00000080;
switchD_000018a2_caseD_a9:
    if (bVar11) {
switchD_000018a2_caseD_cd:
      bVar11 = ((uint)param_1 & 1) == 0;
switchD_000018a2_caseD_ce:
      if (bVar11) {
switchD_000018a2_caseD_d5:
        param_1 = *(code **)(pcVar4 + 0x20);
switchD_000018a2_caseD_d6:
        param_2 = &Reset;
switchD_000018a2_caseD_d7:
        param_1 = (code *)((uint)param_1 & ~(uint)param_2);
switchD_000018a2_caseD_d8:
        *(code **)(pcVar4 + 0x20) = param_1;
switchD_000018a2_caseD_d9:
        return param_1;
      }
switchD_000018a2_caseD_cf:
      param_1 = *(code **)(pcVar4 + 0x20);
switchD_000018a2_caseD_d0:
      param_2 = (dword *)&UNK_00000001;
switchD_000018a2_caseD_d1:
      param_2 = (dword *)((int)param_2 << 10);
switchD_000018a2_caseD_d2:
      param_1 = (code *)((uint)param_1 & ~(uint)param_2);
switchD_000018a2_caseD_d3:
      *(code **)(pcVar4 + 0x20) = param_1;
switchD_000018a2_caseD_d4:
      return param_1;
    }
switchD_000018a2_caseD_aa:
    bVar11 = (code *)param_2 == SFC_QSPI_Enable;
switchD_000018a2_caseD_ab:
    if (bVar11) {
switchD_000018a2_caseD_ac:
      bVar11 = ((uint)param_1 & 1) == 0;
switchD_000018a2_caseD_ad:
      if (bVar11) {
switchD_000018a2_caseD_da:
        param_1 = *(code **)(pcVar4 + 0x20);
switchD_000018a2_caseD_db:
        param_2 = &NMI;
switchD_000018a2_caseD_dc:
        param_1 = (code *)((uint)param_1 & ~(uint)param_2);
switchD_000018a2_caseD_dd:
        *(code **)(pcVar4 + 0x20) = param_1;
switchD_000018a2_caseD_de:
        return param_1;
      }
switchD_000018a2_caseD_ae:
      param_1 = *(code **)(pcVar4 + 0x20);
switchD_000018a2_caseD_af:
      param_2 = (dword *)((int)pcVar4 >> 0x13);
switchD_000018a2_caseD_b0:
      param_1 = (code *)((uint)param_1 & ~(uint)param_2);
switchD_000018a2_caseD_b1:
      *(code **)(pcVar4 + 0x20) = param_1;
    }
switchD_000018a2_caseD_b2:
    return param_1;
  case (code)0x9c:
    goto switchD_000018a2_caseD_9c;
  case (code)0x9d:
    goto code_r0x000019e0;
  case (code)0x9e:
    goto switchD_000018a2_caseD_9e;
  case (code)0x9f:
    goto code_r0x000019e4;
  case (code)0xa0:
    goto code_r0x000019e6;
  case (code)0xa1:
    goto code_r0x000019e8;
  case (code)0xa2:
    goto code_r0x000019ea;
  case (code)0xa3:
    goto switchD_000018a2_caseD_a3;
  case (code)0xa4:
    goto switchD_000018a2_caseD_a4;
  case (code)0xa5:
    goto switchD_000018a2_caseD_a5;
  case (code)0xa6:
    goto switchD_000018a2_caseD_a6;
  case (code)0xa7:
    goto switchD_000018a2_caseD_a7;
  case (code)0xa8:
    goto switchD_000018a2_caseD_a8;
  case (code)0xa9:
    goto switchD_000018a2_caseD_a9;
  case (code)0xaa:
    goto switchD_000018a2_caseD_aa;
  case (code)0xab:
    goto switchD_000018a2_caseD_ab;
  case (code)0xac:
    goto switchD_000018a2_caseD_ac;
  case (code)0xad:
    goto switchD_000018a2_caseD_ad;
  case (code)0xae:
    goto switchD_000018a2_caseD_ae;
  case (code)0xaf:
    goto switchD_000018a2_caseD_af;
  case (code)0xb0:
    goto switchD_000018a2_caseD_b0;
  case (code)0xb1:
    goto switchD_000018a2_caseD_b1;
  case (code)0xb2:
    goto switchD_000018a2_caseD_b2;
  case (code)0xb3:
    goto switchD_000018a2_caseD_b3;
  case (code)0xb4:
    goto switchD_000018a2_caseD_b4;
  case (code)0xb5:
    goto switchD_000018a2_caseD_b5;
  case (code)0xb6:
    goto switchD_000018a2_caseD_b6;
  case (code)0xb7:
    goto switchD_000018a2_caseD_b7;
  case (code)0xb8:
    goto switchD_000018a2_caseD_b8;
  case (code)0xb9:
    goto switchD_000018a2_caseD_b9;
  case (code)0xba:
    goto switchD_000018a2_caseD_ba;
  case (code)0xbb:
    goto switchD_000018a2_caseD_bb;
  case (code)0xbc:
    goto switchD_000018a2_caseD_bc;
  case (code)0xbd:
    goto switchD_000018a2_caseD_bd;
  case (code)0xbe:
    goto switchD_000018a2_caseD_be;
  case (code)0xbf:
    goto switchD_000018a2_caseD_bf;
  case (code)0xc0:
    goto switchD_000018a2_caseD_c0;
  case (code)0xc1:
    goto switchD_000018a2_caseD_c1;
  case (code)0xc2:
    goto switchD_000018a2_caseD_c2;
  case (code)0xc3:
    goto switchD_000018a2_caseD_c3;
  case (code)0xc4:
    goto switchD_000018a2_caseD_c4;
  case (code)0xc5:
    goto switchD_000018a2_caseD_c5;
  case (code)0xc6:
    goto switchD_000018a2_caseD_c6;
  case (code)0xc7:
    goto switchD_000018a2_caseD_c7;
  case (code)0xc8:
    goto switchD_000018a2_caseD_c8;
  case (code)0xc9:
    goto switchD_000018a2_caseD_c9;
  case (code)0xca:
    goto switchD_000018a2_caseD_ca;
  case (code)0xcb:
    goto switchD_000018a2_caseD_cb;
  case (code)0xcc:
    goto switchD_000018a2_caseD_cc;
  case (code)0xcd:
    goto switchD_000018a2_caseD_cd;
  case (code)0xce:
    goto switchD_000018a2_caseD_ce;
  case (code)0xcf:
    goto switchD_000018a2_caseD_cf;
  case (code)0xd0:
    goto switchD_000018a2_caseD_d0;
  case (code)0xd1:
    goto switchD_000018a2_caseD_d1;
  case (code)0xd2:
    goto switchD_000018a2_caseD_d2;
  case (code)0xd3:
    goto switchD_000018a2_caseD_d3;
  case (code)0xd4:
    goto switchD_000018a2_caseD_d4;
  case (code)0xd5:
    goto switchD_000018a2_caseD_d5;
  case (code)0xd6:
    goto switchD_000018a2_caseD_d6;
  case (code)0xd7:
    goto switchD_000018a2_caseD_d7;
  case (code)0xd8:
    goto switchD_000018a2_caseD_d8;
  case (code)0xd9:
    goto switchD_000018a2_caseD_d9;
  case (code)0xda:
    goto switchD_000018a2_caseD_da;
  case (code)0xdb:
    goto switchD_000018a2_caseD_db;
  case (code)0xdc:
    goto switchD_000018a2_caseD_dc;
  case (code)0xdd:
    goto switchD_000018a2_caseD_dd;
  case (code)0xde:
    goto switchD_000018a2_caseD_de;
  case (code)0xdf:
  case (code)0xe0:
    pcVar9 = param_1;
switchD_000018a2_caseD_e1:
    pcVar6 = (code *)param_2;
switchD_000018a2_caseD_e2:
    param_2 = (dword *)RAM_2000007c;
switchD_000018a2_caseD_e3:
    param_1 = (code *)((int)pcVar9 << 2);
switchD_000018a2_caseD_e4:
    pcVar7 = *(code **)((int)param_2 + (int)param_1);
switchD_000018a2_caseD_e5:
    param_1 = (code *)&UNK_00000001;
switchD_000018a2_caseD_e6:
    param_1 = (code *)((int)param_1 << 0x1e);
switchD_000018a2_caseD_e7:
    param_2 = *(dword **)(param_1 + 8);
switchD_000018a2_caseD_e8:
    pcVar4 = (code *)((int)param_1 >> 0x12);
switchD_000018a2_caseD_e9:
    param_2 = (dword *)((uint)param_2 | (uint)pcVar4);
switchD_000018a2_caseD_ea:
    *(dword **)(param_1 + 8) = param_2;
switchD_000018a2_caseD_eb:
    pcVar8 = PWMG;
switchD_000018a2_caseD_ec:
    param_1 = (code *)(uint)(byte)pcVar6[1];
switchD_000018a2_caseD_ed:
    *(code **)pcVar8 = param_1;
switchD_000018a2_caseD_ee:
    param_1 = pcVar9;
    pcVar9 = param_1;
switchD_000018a2_caseD_ef:
    switchD_000018a2::caseD_9f(param_1);
    pcVar6 = extraout_r3;
switchD_000018a2_caseD_f1:
    param_1 = (code *)(uint)(byte)*pcVar6;
    break;
  case (code)0xe1:
    goto switchD_000018a2_caseD_e1;
  case (code)0xe2:
    goto switchD_000018a2_caseD_e2;
  case (code)0xe3:
    goto switchD_000018a2_caseD_e3;
  case (code)0xe4:
    goto switchD_000018a2_caseD_e4;
  case (code)0xe5:
    goto switchD_000018a2_caseD_e5;
  case (code)0xe6:
    goto switchD_000018a2_caseD_e6;
  case (code)0xe7:
    goto switchD_000018a2_caseD_e7;
  case (code)0xe8:
    goto switchD_000018a2_caseD_e8;
  case (code)0xe9:
    goto switchD_000018a2_caseD_e9;
  case (code)0xea:
    goto switchD_000018a2_caseD_ea;
  case (code)0xeb:
    goto switchD_000018a2_caseD_eb;
  case (code)0xec:
    goto switchD_000018a2_caseD_ec;
  case (code)0xed:
    goto switchD_000018a2_caseD_ed;
  case (code)0xee:
    goto switchD_000018a2_caseD_ee;
  case (code)0xef:
    goto switchD_000018a2_caseD_ef;
  case (code)0xf0:
    uVar13 = VectorShiftRightNarrow(in_q4,0x12,4,1);
    SatQ(uVar13,4,1);
    break;
  case (code)0xf1:
    goto switchD_000018a2_caseD_f1;
  case (code)0xf3:
    goto switchD_000018a2_caseD_f3;
  case (code)0xf4:
    goto switchD_000018a2_caseD_f4;
  case (code)0xf5:
    goto switchD_000018a2_caseD_f5;
  case (code)0xf6:
    goto switchD_000018a2_caseD_f6;
  case (code)0xf7:
    goto switchD_000018a2_caseD_f7;
  case (code)0xf8:
    goto switchD_000018a2_caseD_f8;
  case (code)0xf9:
    goto switchD_000018a2_caseD_f9;
  case (code)0xfa:
    goto switchD_000018a2_caseD_fa;
  case (code)0xfb:
    goto switchD_000018a2_caseD_fb;
  case (code)0xfc:
    goto switchD_000018a2_caseD_fc;
  case (code)0xfd:
    goto switchD_000018a2_caseD_fd;
  case (code)0xfe:
    goto switchD_000018a2_caseD_fe;
  case (code)0xff:
    goto switchD_000018a2_caseD_ff;
  }
  bVar11 = param_1 == (code *)0x0;
switchD_000018a2_caseD_f3:
  if (bVar11) {
switchD_000018a2_caseD_f6:
    param_2 = (dword *)0x0;
  }
  else {
switchD_000018a2_caseD_f4:
    param_2 = (dword *)((int)&Reset + 1);
switchD_000018a2_caseD_f5:
  }
switchD_000018a2_caseD_f7:
  *(dword **)pcVar7 = param_2;
switchD_000018a2_caseD_f8:
  param_2 = (dword *)((int)pcVar9 << 0x1f);
switchD_000018a2_caseD_f9:
  param_1 = (code *)0x0;
switchD_000018a2_caseD_fa:
  pcVar9 = 0xbffba000;
switchD_000018a2_caseD_fb:
  bVar11 = (code *)param_2 == (code *)0x0;
switchD_000018a2_caseD_fc:
  if (bVar11) {
    *(dword *)(pcVar7 + 4) = *(dword *)(pcVar6 + 4);
    *(dword *)(pcVar7 + 8) = *(dword *)(pcVar6 + 8) << 1;
    *(uint *)(pcVar7 + 0xc) = (uint)*(ushort *)(pcVar6 + 0xc);
    if (pcVar6[0xe] == (code)0x0) {
      *(uint *)(pcVar7 + 0x1c) = *(uint *)(pcVar7 + 0x1c) & 0xfffffffe;
    }
    else {
      *(uint *)(pcVar7 + 0x1c) = *(uint *)(pcVar7 + 0x1c) | 1;
    }
    *(code **)(pcVar8 + 0x24) = param_1;
    *(code **)(pcVar8 + 0x3c) = param_1;
    pcVar7 = pcVar7 + (int)pcVar9;
    if (pcVar7 == (code *)0x0) {
      *(dword *)(pcVar8 + 0x28) = 1;
      *(dword *)(pcVar8 + 0x2c) = 1;
      if (pcVar6[0x10] == (code)0x0) {
        *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xfffffffe;
      }
      else {
        *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 1;
      }
      if (pcVar6[0xf] == (code)0x0) {
        *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xfffeffff;
      }
      else {
        *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 0x10000;
      }
      if (pcVar6[0x11] == (code)0x0) {
        *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) & 0xfffffffe;
      }
      else {
        *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) | 1;
      }
      pcVar3 = (code *)0x0;
      if ((pcVar6[0xf] != (code)0x0 || pcVar6[0x10] != (code)0x0) || pcVar6[0x11] != (code)0x0) {
        pcVar3 = (code *)FUN_00002454(7);
      }
    }
    else {
      if (pcVar7 != (code *)&IRQ) {
        if (pcVar7 == (code *)&DWORD_00000080) {
          *(dword *)(pcVar8 + 0x28) = 0x10;
          *(dword *)(pcVar8 + 0x2c) = 0x10;
          if (pcVar6[0x10] == (code)0x0) {
            *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xffffffef;
          }
          else {
            *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 0x10;
          }
          if (pcVar6[0xf] == (code)0x0) {
            *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xffefffff;
          }
          else {
            *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 0x100000;
          }
          if (pcVar6[0x11] == (code)0x0) {
            *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) & 0xffffffef;
          }
          else {
            *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) | 0x10;
          }
          if ((pcVar6[0xf] == (code)0x0 && pcVar6[0x10] == (code)0x0) && pcVar6[0x11] == (code)0x0)
          {
            return (code *)0x0;
          }
          pcVar3 = (code *)FUN_00002454(0x1c);
        }
        else {
          if (pcVar7 != SFC_QSPI_Enable) {
            return pcVar7;
          }
          *(dword *)(pcVar8 + 0x28) = 0x40;
          *(dword *)(pcVar8 + 0x2c) = 0x40;
          if (pcVar6[0x10] == (code)0x0) {
            *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xffffffbf;
          }
          else {
            *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 0x40;
          }
          if (pcVar6[0xf] == (code)0x0) {
            *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xffbfffff;
          }
          else {
            *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 0x400000;
          }
          if (pcVar6[0x11] == (code)0x0) {
            *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) & 0xffffffbf;
          }
          else {
            *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) | 0x40;
          }
          pcVar3 = (code *)0x0;
          if ((pcVar6[0xf] != (code)0x0 || pcVar6[0x10] != (code)0x0) || pcVar6[0x11] != (code)0x0)
          {
            pcVar3 = (code *)FUN_00002454(0x1d);
            return pcVar3;
          }
        }
        return pcVar3;
      }
      *(dword *)(pcVar8 + 0x28) = 4;
      *(dword *)(pcVar8 + 0x2c) = 4;
      if (pcVar6[0x10] == (code)0x0) {
        *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xfffffffb;
      }
      else {
        *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 4;
      }
      if (pcVar6[0xf] == (code)0x0) {
        *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xfffbffff;
      }
      else {
        *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 0x40000;
      }
      if (pcVar6[0x11] == (code)0x0) {
        *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) & 0xfffffffb;
      }
      else {
        *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) | 4;
      }
      pcVar3 = (code *)0x0;
      if ((pcVar6[0xf] != (code)0x0 || pcVar6[0x10] != (code)0x0) || pcVar6[0x11] != (code)0x0) {
        pcVar3 = (code *)FUN_00002454(0x1a);
        return pcVar3;
      }
    }
    return pcVar3;
  }
switchD_000018a2_caseD_fd:
  param_2 = *(dword **)(pcVar6 + 4);
switchD_000018a2_caseD_fe:
  *(dword **)(pcVar7 + 0x10) = param_2;
switchD_000018a2_caseD_ff:
  *(dword *)(pcVar7 + 0x14) = *(dword *)(pcVar6 + 8) << 1;
  *(uint *)(pcVar7 + 0x18) = (uint)*(ushort *)(pcVar6 + 0xc);
  if (pcVar6[0xe] == (code)0x0) {
    *(uint *)(pcVar7 + 0x1c) = *(uint *)(pcVar7 + 0x1c) & 0xfffffffd;
  }
  else {
    *(uint *)(pcVar7 + 0x1c) = *(uint *)(pcVar7 + 0x1c) | 2;
  }
  *(code **)(pcVar8 + 0x24) = param_1;
  *(code **)(pcVar8 + 0x3c) = param_1;
  pcVar7 = pcVar7 + (int)pcVar9;
  if (pcVar7 == (code *)0x0) {
    *(dword *)(pcVar8 + 0x28) = 2;
    *(dword *)(pcVar8 + 0x2c) = 2;
    if (pcVar6[0x10] == (code)0x0) {
      *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xfffffffd;
    }
    else {
      *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 2;
    }
    if (pcVar6[0xf] == (code)0x0) {
      *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xfffdffff;
    }
    else {
      *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 0x20000;
    }
    if (pcVar6[0x11] == (code)0x0) {
      *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) & 0xfffffffd;
    }
    else {
      *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) | 2;
    }
    if ((pcVar6[0xf] == (code)0x0 && pcVar6[0x10] == (code)0x0) && pcVar6[0x11] == (code)0x0) {
      return (code *)0x0;
    }
    pcVar3 = (code *)FUN_00002454(7);
  }
  else {
    if (pcVar7 != (code *)&IRQ) {
      if (pcVar7 == (code *)&DWORD_00000080) {
        *(dword *)(pcVar8 + 0x28) = 0x20;
        *(dword *)(pcVar8 + 0x2c) = 0x20;
        if (pcVar6[0x10] == (code)0x0) {
          *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xffffffdf;
        }
        else {
          *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 0x20;
        }
        if (pcVar6[0xf] == (code)0x0) {
          *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xffdfffff;
        }
        else {
          *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 0x200000;
        }
        if (pcVar6[0x11] == (code)0x0) {
          *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) & 0xffffffdf;
        }
        else {
          *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) | 0x20;
        }
        if ((pcVar6[0xf] == (code)0x0 && pcVar6[0x10] == (code)0x0) && pcVar6[0x11] == (code)0x0) {
          return (code *)0x0;
        }
        pcVar3 = (code *)FUN_00002454(0x1c);
      }
      else {
        if (pcVar7 != SFC_QSPI_Enable) {
          return pcVar7;
        }
        *(dword *)(pcVar8 + 0x28) = 0x80;
        *(dword *)(pcVar8 + 0x2c) = 0x80;
        if (pcVar6[0x10] == (code)0x0) {
          *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xffffff7f;
        }
        else {
          *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 0x80;
        }
        if (pcVar6[0xf] == (code)0x0) {
          *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xff7fffff;
        }
        else {
          *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 0x800000;
        }
        if (pcVar6[0x11] == (code)0x0) {
          *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) & 0xffffff7f;
        }
        else {
          *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) | 0x80;
        }
        pcVar3 = (code *)0x0;
        if ((pcVar6[0xf] != (code)0x0 || pcVar6[0x10] != (code)0x0) || pcVar6[0x11] != (code)0x0) {
          pcVar3 = (code *)FUN_00002454(0x1d);
          return pcVar3;
        }
      }
      return pcVar3;
    }
    *(dword *)(pcVar8 + 0x28) = 8;
    *(dword *)(pcVar8 + 0x2c) = 8;
    if (pcVar6[0x10] == (code)0x0) {
      *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xfffffff7;
    }
    else {
      *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 8;
    }
    if (pcVar6[0xf] == (code)0x0) {
      *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) & 0xfff7ffff;
    }
    else {
      *(uint *)(pcVar8 + 0x1c) = *(uint *)(pcVar8 + 0x1c) | 0x80000;
    }
    if (pcVar6[0x11] == (code)0x0) {
      *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) & 0xfffffff7;
    }
    else {
      *(uint *)(pcVar8 + 0x38) = *(uint *)(pcVar8 + 0x38) | 8;
    }
    pcVar3 = (code *)0x0;
    if ((pcVar6[0xf] != (code)0x0 || pcVar6[0x10] != (code)0x0) || pcVar6[0x11] != (code)0x0) {
      pcVar3 = (code *)FUN_00002454(0x1a);
      return pcVar3;
    }
  }
  return pcVar3;
}


// ===== caseD_9f @ 000019e4

void switchD_000018a2::caseD_9f(uint param_1)

{
  int iVar1;
  
  iVar1 = *(int *)(RAM_2000007c + param_1 * 4) + 0xbffba000;
  if (iVar1 == 0) {
    if ((param_1 & 1) == 0) {
      *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) & 0xfffffffe;
      return;
    }
    *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) & 0xfffffeff;
    return;
  }
  if (iVar1 == 0x40) {
    if ((param_1 & 1) == 0) {
      *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) & 0xfffffffd;
      return;
    }
    *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) & 0xfffffdff;
    return;
  }
  if (iVar1 == 0x80) {
    if ((param_1 & 1) == 0) {
      *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) & 0xfffffffb;
      return;
    }
    *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) & 0xfffffbff;
    return;
  }
  if (iVar1 == 0xc0) {
    if ((param_1 & 1) == 0) {
      *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) & 0xfffffff7;
      return;
    }
    *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) & ~(PWMG >> 0x13);
  }
  return;
}


// ===== PWM_Init @ 00001a64

void switchD_000018a2::PWM_Init(uint param_1,int param_2)

{
  dword dVar1;
  uint *puVar2;
  int iVar3;
  undefined4 uVar4;
  char *extraout_r3;
  undefined4 *puVar5;
  
  puVar2 = PWMG;
  puVar5 = *(undefined4 **)(RAM_2000007c + param_1 * 4);
  dVar1 = SYS_CLKEN0;
  SYS_CLKEN0 = dVar1 | 0x1000;
  *PWMG = (uint)*(byte *)(param_2 + 1);
  caseD_9f(param_1);
  if (*extraout_r3 == '\0') {
    uVar4 = 0;
  }
  else {
    uVar4 = 5;
  }
  *puVar5 = uVar4;
  iVar3 = 0xbffba000;
  if ((param_1 & 1) != 0) {
    puVar5[4] = *(undefined4 *)(extraout_r3 + 4);
    puVar5[5] = *(int *)(extraout_r3 + 8) << 1;
    puVar5[6] = (uint)*(ushort *)(extraout_r3 + 0xc);
    if (extraout_r3[0xe] == '\0') {
      puVar5[7] = puVar5[7] & 0xfffffffd;
    }
    else {
      puVar5[7] = puVar5[7] | 2;
    }
    puVar2[9] = 0;
    puVar2[0xf] = 0;
    iVar3 = (int)puVar5 + iVar3;
    if (iVar3 == 0) {
      puVar2[10] = 2;
      puVar2[0xb] = 2;
      if (extraout_r3[0x10] == '\0') {
        puVar2[7] = puVar2[7] & 0xfffffffd;
      }
      else {
        puVar2[7] = puVar2[7] | 2;
      }
      if (extraout_r3[0xf] == '\0') {
        puVar2[7] = puVar2[7] & 0xfffdffff;
      }
      else {
        puVar2[7] = puVar2[7] | 0x20000;
      }
      if (extraout_r3[0x11] == '\0') {
        puVar2[0xe] = puVar2[0xe] & 0xfffffffd;
      }
      else {
        puVar2[0xe] = puVar2[0xe] | 2;
      }
      if ((extraout_r3[0xf] == '\0' && extraout_r3[0x10] == '\0') && extraout_r3[0x11] == '\0') {
        return;
      }
      FUN_00002454(7);
    }
    else {
      if (iVar3 != 0x40) {
        if (iVar3 == 0x80) {
          puVar2[10] = 0x20;
          puVar2[0xb] = 0x20;
          if (extraout_r3[0x10] == '\0') {
            puVar2[7] = puVar2[7] & 0xffffffdf;
          }
          else {
            puVar2[7] = puVar2[7] | 0x20;
          }
          if (extraout_r3[0xf] == '\0') {
            puVar2[7] = puVar2[7] & 0xffdfffff;
          }
          else {
            puVar2[7] = puVar2[7] | 0x200000;
          }
          if (extraout_r3[0x11] == '\0') {
            puVar2[0xe] = puVar2[0xe] & 0xffffffdf;
          }
          else {
            puVar2[0xe] = puVar2[0xe] | 0x20;
          }
          if ((extraout_r3[0xf] == '\0' && extraout_r3[0x10] == '\0') && extraout_r3[0x11] == '\0')
          {
            return;
          }
          FUN_00002454(0x1c);
        }
        else {
          if (iVar3 != 0xc0) {
            return;
          }
          puVar2[10] = 0x80;
          puVar2[0xb] = 0x80;
          if (extraout_r3[0x10] == '\0') {
            puVar2[7] = puVar2[7] & 0xffffff7f;
          }
          else {
            puVar2[7] = puVar2[7] | 0x80;
          }
          if (extraout_r3[0xf] == '\0') {
            puVar2[7] = puVar2[7] & 0xff7fffff;
          }
          else {
            puVar2[7] = puVar2[7] | 0x800000;
          }
          if (extraout_r3[0x11] == '\0') {
            puVar2[0xe] = puVar2[0xe] & 0xffffff7f;
          }
          else {
            puVar2[0xe] = puVar2[0xe] | 0x80;
          }
          if ((extraout_r3[0xf] != '\0' || extraout_r3[0x10] != '\0') || extraout_r3[0x11] != '\0')
          {
            FUN_00002454(0x1d);
            return;
          }
        }
        return;
      }
      puVar2[10] = 8;
      puVar2[0xb] = 8;
      if (extraout_r3[0x10] == '\0') {
        puVar2[7] = puVar2[7] & 0xfffffff7;
      }
      else {
        puVar2[7] = puVar2[7] | 8;
      }
      if (extraout_r3[0xf] == '\0') {
        puVar2[7] = puVar2[7] & 0xfff7ffff;
      }
      else {
        puVar2[7] = puVar2[7] | 0x80000;
      }
      if (extraout_r3[0x11] == '\0') {
        puVar2[0xe] = puVar2[0xe] & 0xfffffff7;
      }
      else {
        puVar2[0xe] = puVar2[0xe] | 8;
      }
      if ((extraout_r3[0xf] != '\0' || extraout_r3[0x10] != '\0') || extraout_r3[0x11] != '\0') {
        FUN_00002454(0x1a);
        return;
      }
    }
    return;
  }
  puVar5[1] = *(undefined4 *)(extraout_r3 + 4);
  puVar5[2] = *(int *)(extraout_r3 + 8) << 1;
  puVar5[3] = (uint)*(ushort *)(extraout_r3 + 0xc);
  if (extraout_r3[0xe] == '\0') {
    puVar5[7] = puVar5[7] & 0xfffffffe;
  }
  else {
    puVar5[7] = puVar5[7] | 1;
  }
  puVar2[9] = 0;
  puVar2[0xf] = 0;
  iVar3 = (int)puVar5 + iVar3;
  if (iVar3 == 0) {
    puVar2[10] = 1;
    puVar2[0xb] = 1;
    if (extraout_r3[0x10] == '\0') {
      puVar2[7] = puVar2[7] & 0xfffffffe;
    }
    else {
      puVar2[7] = puVar2[7] | 1;
    }
    if (extraout_r3[0xf] == '\0') {
      puVar2[7] = puVar2[7] & 0xfffeffff;
    }
    else {
      puVar2[7] = puVar2[7] | 0x10000;
    }
    if (extraout_r3[0x11] == '\0') {
      puVar2[0xe] = puVar2[0xe] & 0xfffffffe;
    }
    else {
      puVar2[0xe] = puVar2[0xe] | 1;
    }
    if ((extraout_r3[0xf] != '\0' || extraout_r3[0x10] != '\0') || extraout_r3[0x11] != '\0') {
      FUN_00002454(7);
    }
  }
  else {
    if (iVar3 != 0x40) {
      if (iVar3 == 0x80) {
        puVar2[10] = 0x10;
        puVar2[0xb] = 0x10;
        if (extraout_r3[0x10] == '\0') {
          puVar2[7] = puVar2[7] & 0xffffffef;
        }
        else {
          puVar2[7] = puVar2[7] | 0x10;
        }
        if (extraout_r3[0xf] == '\0') {
          puVar2[7] = puVar2[7] & 0xffefffff;
        }
        else {
          puVar2[7] = puVar2[7] | 0x100000;
        }
        if (extraout_r3[0x11] == '\0') {
          puVar2[0xe] = puVar2[0xe] & 0xffffffef;
        }
        else {
          puVar2[0xe] = puVar2[0xe] | 0x10;
        }
        if ((extraout_r3[0xf] == '\0' && extraout_r3[0x10] == '\0') && extraout_r3[0x11] == '\0') {
          return;
        }
        FUN_00002454(0x1c);
      }
      else {
        if (iVar3 != 0xc0) {
          return;
        }
        puVar2[10] = 0x40;
        puVar2[0xb] = 0x40;
        if (extraout_r3[0x10] == '\0') {
          puVar2[7] = puVar2[7] & 0xffffffbf;
        }
        else {
          puVar2[7] = puVar2[7] | 0x40;
        }
        if (extraout_r3[0xf] == '\0') {
          puVar2[7] = puVar2[7] & 0xffbfffff;
        }
        else {
          puVar2[7] = puVar2[7] | 0x400000;
        }
        if (extraout_r3[0x11] == '\0') {
          puVar2[0xe] = puVar2[0xe] & 0xffffffbf;
        }
        else {
          puVar2[0xe] = puVar2[0xe] | 0x40;
        }
        if ((extraout_r3[0xf] != '\0' || extraout_r3[0x10] != '\0') || extraout_r3[0x11] != '\0') {
          FUN_00002454(0x1d);
          return;
        }
      }
      return;
    }
    puVar2[10] = 4;
    puVar2[0xb] = 4;
    if (extraout_r3[0x10] == '\0') {
      puVar2[7] = puVar2[7] & 0xfffffffb;
    }
    else {
      puVar2[7] = puVar2[7] | 4;
    }
    if (extraout_r3[0xf] == '\0') {
      puVar2[7] = puVar2[7] & 0xfffbffff;
    }
    else {
      puVar2[7] = puVar2[7] | 0x40000;
    }
    if (extraout_r3[0x11] == '\0') {
      puVar2[0xe] = puVar2[0xe] & 0xfffffffb;
    }
    else {
      puVar2[0xe] = puVar2[0xe] | 4;
    }
    if ((extraout_r3[0xf] != '\0' || extraout_r3[0x10] != '\0') || extraout_r3[0x11] != '\0') {
      FUN_00002454(0x1a);
      return;
    }
  }
  return;
}


// ===== PWM_Start @ 00001dfa

void PWM_Start(uint param_1)

{
  int iVar1;
  
  iVar1 = *(int *)(RAM_2000007c + param_1 * 4) + 0xbffba000;
  if (iVar1 == 0) {
    if ((param_1 & 1) == 0) {
      *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) | 1;
      return;
    }
    *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) | 0x100;
    return;
  }
  if (iVar1 == 0x40) {
    if ((param_1 & 1) == 0) {
      *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) | 2;
      return;
    }
    *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) | 0x200;
    return;
  }
  if (iVar1 == 0x80) {
    if ((param_1 & 1) == 0) {
      *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) | 4;
      return;
    }
    *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) | 0x400;
    return;
  }
  if (iVar1 == 0xc0) {
    if ((param_1 & 1) == 0) {
      *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) | 8;
      return;
    }
    *(uint *)(PWMG + 0x20) = *(uint *)(PWMG + 0x20) | PWMG >> 0x13;
  }
  return;
}


// ===== PWM_SetPeriod @ 00001e7a

void PWM_SetPeriod(uint param_1,undefined4 param_2)

{
  int iVar1;
  
  iVar1 = *(int *)(RAM_2000007c + param_1 * 4);
  if ((param_1 & 1) == 0) {
    *(undefined4 *)(iVar1 + 4) = param_2;
    return;
  }
  *(undefined4 *)(iVar1 + 0x10) = param_2;
  return;
}


// ===== PWM_SetHDuty @ 00001e9e

void PWM_SetHDuty(uint param_1,int param_2)

{
  int iVar1;
  
  iVar1 = *(int *)(RAM_2000007c + param_1 * 4);
  if ((param_1 & 1) == 0) {
    *(int *)(iVar1 + 8) = param_2 << 1;
    return;
  }
  *(int *)(iVar1 + 0x14) = param_2 << 1;
  return;
}


// ===== FUN_00002454 @ 00002454

void FUN_00002454(uint param_1)

{
  *0xe000e100 = 1 << (param_1 & 0x1f);
  return;
}


// ===== TIMR_Stop @ 00002478

void TIMR_Stop(int param_1)

{
  int iVar1;
  
  iVar1 = param_1 - BTIMR0;
  if (param_1 == BTIMR0) {
    *0x40048c40 = *0x40048c40 & 0xfffffffe;
    return;
  }
  if (param_1 < BTIMR0) {
    param_1 = param_1 + 0xbffb9800;
    if (param_1 == 0) {
      *0x40046c40 = *0x40046c40 & 0xfffffffe;
      return;
    }
    if (param_1 == 0x40) {
      *0x40046c40 = *0x40046c40 & 0xfffffffd;
      return;
    }
    if (param_1 == 0x80) {
      *0x40046c40 = *0x40046c40 & 0xfffffffb;
      return;
    }
    if (param_1 == 0xc0) {
      *0x40046c40 = *0x40046c40 & 0xfffffff7;
    }
  }
  else {
    if (iVar1 == 0x40) {
      *0x40048c40 = *0x40048c40 & 0xfffffffd;
      return;
    }
    if (iVar1 == 0x80) {
      *0x40048c40 = *0x40048c40 & 0xfffffffb;
      return;
    }
    if (iVar1 == 0xc0) {
      *0x40048c40 = *0x40048c40 & 0xfffffff7;
      return;
    }
  }
  return;
}


// ===== TIMR_Init @ 000024f0

void TIMR_Init(uint *param_1,uint param_2,uint param_3,int param_4)

{
  int iVar1;
  
  iVar1 = 0xbffb9800 * -0x80000;
  if ((((param_1 == (uint *)-0xbffb9800) || (param_1 == TIMR1)) ||
      (param_1 == TIMR1 + 0x10)) || (param_1 == TIMR1 + 0x20)) {
    *(uint *)(iVar1 + 8) = *(uint *)(iVar1 + 8) | 0x800;
    TIMR_Stop(param_1);
    param_1[2] = param_1[2] & 0xfffffff0;
    param_1[2] = param_1[2] | param_2;
    *param_1 = param_3;
    param_1[4] = 0;
    param_1[5] = 1;
    if (param_4 == 0) {
      param_1[3] = param_1[3] & 0xfffffffe;
    }
    else {
      param_1[3] = param_1[3] | 1;
    }
  }
  else if (((param_1 == BTIMR0) || (param_1 == BTIMR0 + 0x10)) ||
          ((param_1 == BTIMR0 + 0x20 || (param_1 == BTIMR0 + 0x30)))) {
    *(uint *)(iVar1 + 0xc) = *(uint *)(iVar1 + 0xc) | 0x400000;
    TIMR_Stop(param_1);
    *param_1 = param_3 & 0xffffff;
    param_1[2] = param_3 >> 0x18;
    param_1[5] = 1;
    if (param_4 == 0) {
      param_1[3] = param_1[3] & 0xfffffffe;
    }
    else {
      param_1[3] = param_1[3] | 1;
    }
  }
  iVar1 = (int)param_1 - (int)BTIMR0;
  if (param_1 == BTIMR0) {
    if (param_4 != 0) {
      NVIC_EnableIRQ(8);
    }
  }
  else if ((int)param_1 < (int)BTIMR0) {
    iVar1 = (int)param_1 + 0xbffb9800;
    if (iVar1 == 0) {
      if (param_4 != 0) {
        NVIC_EnableIRQ(5);
      }
    }
    else if (iVar1 == 0x40) {
      if (param_4 != 0) {
        NVIC_EnableIRQ(10);
      }
    }
    else if (iVar1 == 0x80) {
      if (param_4 != 0) {
        NVIC_EnableIRQ(0x17);
      }
    }
    else if ((iVar1 == 0xc0) && (param_4 != 0)) {
      NVIC_EnableIRQ(0x18);
    }
  }
  else if (iVar1 == 0x40) {
    if (param_4 != 0) {
      NVIC_EnableIRQ(0x11);
    }
  }
  else if (iVar1 == 0x80) {
    if (param_4 != 0) {
      NVIC_EnableIRQ(0xf);
    }
  }
  else if ((iVar1 == 0xc0) && (param_4 != 0)) {
    NVIC_EnableIRQ(9);
  }
  return;
}


// ===== TIMR_Start @ 00002632

void TIMR_Start(int param_1)

{
  int iVar1;
  
  iVar1 = param_1 - BTIMR0;
  if (param_1 == BTIMR0) {
    *0x40048c40 = *0x40048c40 | 1;
    return;
  }
  if (param_1 < BTIMR0) {
    param_1 = param_1 + 0xbffb9800;
    if (param_1 == 0) {
      *0x40046c40 = *0x40046c40 | 1;
      return;
    }
    if (param_1 == 0x40) {
      *0x40046c40 = *0x40046c40 | 2;
      return;
    }
    if (param_1 == 0x80) {
      *0x40046c40 = *0x40046c40 | 4;
      return;
    }
    if (param_1 == 0xc0) {
      *0x40046c40 = *0x40046c40 | 8;
    }
  }
  else {
    if (iVar1 == 0x40) {
      *0x40048c40 = *0x40048c40 | 2;
      return;
    }
    if (iVar1 == 0x80) {
      *0x40048c40 = *0x40048c40 | 4;
      return;
    }
    if (iVar1 == 0xc0) {
      *0x40048c40 = *0x40048c40 | 8;
      return;
    }
  }
  return;
}


// ===== TIMR_INTClr @ 00002744

void TIMR_INTClr(int param_1)

{
  *(undefined4 *)(param_1 + 0x14) = 1;
  return;
}


// ===== NVIC_EnableIRQ @ 000028ec

void NVIC_EnableIRQ(uint param_1)

{
  *0xe000e100 = 1 << (param_1 & 0x1f);
  return;
}


// ===== SystemCoreClockUpdate @ 00002900
// decompile failed
// ===== switchToHRC @ 00002994

void switchToHRC(void)

{
  dword dVar1;
  
  *0x400aa000 = 1;
  dVar1 = SYS;
  SYS = dVar1 | 1;
  return;
}


// ===== FUN_000029a4 @ 000029a4

void FUN_000029a4(void)

{
  dword dVar1;
  uint uVar2;
  
  switchToHRC();
  *(uint *)(0x400aa000 + 0x20) = *(uint *)(0x400aa000 + 0x20) | 0x111;
  uVar2 = 0;
  do {
    uVar2 = uVar2 + 1;
  } while (uVar2 < 1000);
  SYS_CLKDIVx_ON = 1;
  dVar1 = SYS;
  SYS = dVar1 & 0xffffffe3;
  dVar1 = SYS;
  SYS = dVar1 | 8;
  dVar1 = SYS;
  SYS = dVar1 & 0xfffffffd;
  dVar1 = SYS;
  SYS = dVar1 & 0xfffffffe;
  return;
}


// ===== FUN_000029ec @ 000029ec

void FUN_000029ec(void)

{
  int iVar1;
  uint *puVar2;
  
  switchToHRC();
  iVar1 = 0x400aa000 + 0x40;
  *(undefined4 *)(0x400aa000 + 0x50) = 1;
  puVar2 = (uint *)(iVar1 * 0x1000000);
  puVar2[1] = 1;
  *puVar2 = *puVar2 & 0xffffffe3;
  *puVar2 = *puVar2;
  *puVar2 = *puVar2 & 0xfffffffd;
  *puVar2 = *puVar2 & 0xfffffffe;
  return;
}


// ===== FUN_00002a1c @ 00002a1c

void FUN_00002a1c(void)

{
  int iVar1;
  uint uVar2;
  uint *puVar3;
  
  PORT_Init(2,1,7,0);
  PORT_Init(2,0,7);
  *(uint *)(0x400aa000 + 0x20) = *(uint *)(0x400aa000 + 0x20) | 0x70022;
  iVar1 = 0x400aa000;
  uVar2 = 0;
  do {
    uVar2 = uVar2 + 1;
  } while (uVar2 < 1000);
  puVar3 = (uint *)(0x400aa000 + 0x40);
  *puVar3 = *puVar3 & 0xfffffffd;
  *(uint *)(iVar1 + 0x44) = *(uint *)(iVar1 + 0x44) & 0xfce0fe00;
  *(uint *)(iVar1 + 0x44) = *(uint *)(iVar1 + 0x44) | 0x6001e;
  *puVar3 = *puVar3 & 0xfffffffb;
  do {
  } while (*(int *)(iVar1 + 0x4c) == 0);
  *puVar3 = *puVar3 | 1;
  return;
}


// ===== switchToXTAL_PLL @ 00002a82

/* PC0/PC1 = XTAL_OUT/XTAL_IN */

void switchToXTAL_PLL(int param_1)

{
  dword dVar1;
  
  switchToHRC();
  FUN_00002a1c();
  SYS_CLKDIVx_ON = 1;
  dVar1 = SYS;
  SYS = dVar1 & 0xffffffe3;
  dVar1 = SYS;
  SYS = dVar1 | 4;
  if (param_1 == 0) {
    dVar1 = SYS;
    SYS = dVar1 & 0xfffffffd;
  }
  else {
    dVar1 = SYS;
    SYS = dVar1 | 2;
  }
  dVar1 = SYS;
  SYS = dVar1 & 0xfffffffe;
  return;
}


// ===== FUN_00002ac2 @ 00002ac2

void FUN_00002ac2(int param_1)

{
  dword dVar1;
  uint uVar2;
  
  switchToHRC();
  PORT_Init(2,1,7,0);
  PORT_Init(2,0,7);
  *(uint *)(0x400aa000 + 0x20) = *(uint *)(0x400aa000 + 0x20) | 0x70022;
  uVar2 = 0;
  do {
    uVar2 = uVar2 + 1;
  } while (uVar2 < 1000);
  SYS_CLKDIVx_ON = 1;
  dVar1 = SYS;
  SYS = dVar1 & 0xffffffe3;
  dVar1 = SYS;
  SYS = dVar1 | 0xc;
  if (param_1 == 0) {
    dVar1 = SYS;
    SYS = dVar1 & 0xfffffffd;
  }
  else {
    dVar1 = SYS;
    SYS = dVar1 | 2;
  }
  dVar1 = SYS;
  SYS = dVar1 & 0xfffffffe;
  return;
}


// ===== SystemInit @ 00002b9c

void SystemInit(void)

{
  dword dVar1;
  
  dVar1 = SYS_CLKEN0;
  SYS_CLKEN0 = dVar1 | 0x2000000;
  switchToXTAL_PLL(0);
  SystemCoreClockUpdate();
  return;
}


// ===== Reset_Handler @ 00002bd8

void Reset_Handler(void)

{
  int iVar1;
  dword dVar2;
  
  iVar1 = DAT_4000008c;
  if ((iVar1 == 0x854012) && (dVar2 = SFC, (dVar2 >> 10 & 3) != 2)) {
    SFC_QSPI_Enable();
  }
                    /* WARNING: Could not recover jumptable at 0x00002c0c. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*0x101)();
  return;
}


// ===== Exc2_Handler @ 00002c0e

void Exc2_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== Exc3_Handler @ 00002c10

void Exc3_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== Exc11_Handler @ 00002c12

void Exc11_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== Exc14_Handler @ 00002c14

void Exc14_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== Exc15_Handler @ 00002c16

void Exc15_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== GPIOA_Handler @ 00002c18

void GPIOA_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== GPIOA0_Handler @ 00002c1a

void GPIOA0_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== UART0_Handler @ 00002c1c

void UART0_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== UART1_Handler @ 00002c1e

void UART1_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== SPI0_Handler @ 00002c20

void SPI0_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== DMA_Handler @ 00002c24

void DMA_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== PWM0_Handler @ 00002c26

void PWM0_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== BTIMR0_Handler @ 00002c28

void BTIMR0_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== BTIMR3_Handler @ 00002c2a

void BTIMR3_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== TIMR1_Handler @ 00002c2c

void TIMR1_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== WDT_Handler @ 00002c2e

void WDT_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== GPIOA4_Handler @ 00002c30

void GPIOA4_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== I2C0_Handler @ 00002c32

void I2C0_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== GPIOA5_Handler @ 00002c34

void GPIOA5_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== BTIMR2_Handler @ 00002c36

void BTIMR2_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== GPIOC_CORDIC_Handler @ 00002c38

void GPIOC_CORDIC_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== SPI1_BTIMR1_Handler @ 00002c3a

void SPI1_BTIMR1_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== GPIOB_BOD_Handler @ 00002c3c

void GPIOB_BOD_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== GPIOC4_ADC0_Handler @ 00002c3e

void GPIOC4_ADC0_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== GPIOD_RTC_Handler @ 00002c40

void GPIOD_RTC_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== XTAL_ACMP_Handler @ 00002c42

void XTAL_ACMP_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== UART2_GPIOB4_Handler @ 00002c44

void UART2_GPIOB4_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== TIMR3_HALL_Handler @ 00002c48

void TIMR3_HALL_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== GPIOC5_I2C1_Handler @ 00002c4a

void GPIOC5_I2C1_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== PWM1_GPIOB6_Handler @ 00002c4c

void PWM1_GPIOB6_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== UART3_GPIOB5_Handler @ 00002c4e

void UART3_GPIOB5_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== PWM2_FLASH_Handler @ 00002c50

void PWM2_FLASH_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== PWM3_CACHE_Handler @ 00002c52

void PWM3_CACHE_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== ADC1_GPIOA14_Handler @ 00002c54

void ADC1_GPIOA14_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== GPIOE_PWMHALT_Handler @ 00002c56

void GPIOE_PWMHALT_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}


// ===== __aeabi_uidivmod @ 00002c5c

int __aeabi_uidivmod(uint param_1,uint param_2)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  
  iVar1 = 0;
  uVar2 = 0x20;
  while (uVar3 = uVar2 - 1, 0 < (int)uVar2) {
    uVar2 = uVar3;
    if (param_2 <= param_1 >> (uVar3 & 0xff)) {
      param_1 = param_1 - (param_2 << (uVar3 & 0xff));
      iVar1 = iVar1 + (1 << (uVar3 & 0xff));
    }
  }
  return iVar1;
}


// ===== __aeabi_idivmod @ 00002c88

int __aeabi_idivmod(int param_1,int param_2)

{
  int iVar1;
  bool bVar2;
  bool bVar3;
  
  bVar2 = param_1 < 0;
  if (bVar2) {
    param_1 = -param_1;
  }
  bVar3 = param_2 < 0;
  if (bVar3) {
    param_2 = -param_2;
  }
  iVar1 = __aeabi_uidivmod(param_1,param_2);
  if (bVar2 != bVar3) {
    iVar1 = -iVar1;
  }
  return iVar1;
}


// ===== __scatterload @ 00002cb0

void __scatterload(void)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  uint extraout_r3;
  undefined4 *puVar3;
  uint uVar4;
  
  puVar1 = puRam00002cd0;
  for (puVar3 = puRam00002ccc; puVar3 < puVar1; puVar3 = puVar3 + 4) {
    (*(code *)puVar3[3])(*puVar3,puVar3[1],puVar3[2]);
  }
  uVar2 = main();
  uVar4 = (uint)puRam00002ccc & 0xff;
  if (extraout_r3 < uVar4) {
    uVar4 = extraout_r3;
  }
                    /* WARNING: Could not recover jumptable at 0x00002cec. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*(code *)((uint)*(byte *)(uVar4 + 0x2ccd) * 2 + 0x2ccd))(uVar2);
  return;
}


// ===== __ARM_common_switch8 @ 00002cd4

/* WARNING: This is an inlined function */

void __ARM_common_switch8(void)

{
  uint in_r3;
  uint uVar1;
  int unaff_lr;
  
  uVar1 = (uint)*(byte *)(unaff_lr + -1);
  if (in_r3 < *(byte *)(unaff_lr + -1)) {
    uVar1 = in_r3;
  }
                    /* WARNING: Could not recover jumptable at 0x00002cec. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*(code *)(unaff_lr + (uint)*(byte *)(unaff_lr + uVar1) * 2))();
  return;
}


// ===== thunk_FUN_00002cf6 @ 00002cee

void thunk_FUN_00002cf6(undefined4 *param_1,undefined4 *param_2,int param_3)

{
  undefined4 uVar1;
  
  for (; param_3 != 0; param_3 = param_3 + -4) {
    uVar1 = *param_1;
    param_1 = param_1 + 1;
    *param_2 = uVar1;
    param_2 = param_2 + 1;
  }
  return;
}


// ===== FUN_00002cf6 @ 00002cf6

void FUN_00002cf6(undefined4 *param_1,undefined4 *param_2,int param_3)

{
  undefined4 uVar1;
  
  for (; param_3 != 0; param_3 = param_3 + -4) {
    uVar1 = *param_1;
    param_1 = param_1 + 1;
    *param_2 = uVar1;
    param_2 = param_2 + 1;
  }
  return;
}


