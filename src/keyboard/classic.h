#ifndef CLASSIC_H
#define CLASSIC_H

#define PS2_PORT 0x64
#define PS2_COMMAND_ENABLE_FIRST_PORT 0xAE

// 位掩码
#define CLASSIC_KEYBOARD_KEY_RELESED 0x80   

#define ISR_KEYBOARD_INTERRUPT 0x21
#define CLASSIC_INPUT_PORT 0x60


struct keyboard;

struct keyboard *classic_init();
void classic_keyboard_handle_interrupt();

#endif
