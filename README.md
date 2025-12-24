# MulberryOS
Bare metal OS for Raspberry Pi Zero 2 W written in C.
End goal: Music player with touch enabled e-ink display

## Constraints
- Primary UI: E-ink display + touch gestures will be used
- Debugging: Logs will be printed through UART
- Audio Output: USB DAC will be utilised due e-ink HAT constraint

## Roadmap
- Boot + UART Logging 
- Memory Management
- Timer + Interrupts + Shell
- VFS + FAT32 + WAV Parsing 
- SD Read
- USB Host Foundation
- USB Audio Class
- E-INK UI + Touch