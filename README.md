# CS140E Final Project: PIC32 ICSP Programmer on the Raspberry Pi A+
## khuang and choiben214

### Github Repo:
https://github.com/choiben314/pic-programmer

## What Is It?

We created an external programming tool to load programs on to the PIC32 using the Raspberry Pi A+. Currently, we are still in the process of adding support for loading programs, but the programmer successfully checks for device status and enters the serial execution mode needed to load programs.

## Why?

We worked with the PIC32 in a mechatronics course (ME218B) using an external programming tool (SNAP) and wanted to learn the inner-workings of the programming tool and how it actually loads programs on to the PIC. We also wanted to explore JTAG and ICSP programming.

## Challenges

### What We Tried?

### What's Left?

## Useful Resources
- [PIC32MX Flash Programing Specification](http://ww1.microchip.com/downloads/en/devicedoc/61145g.pdf)
    - most useful resource by far
    - contains useful instructions, block diagrams, and timing diagrams describing how to put PIC32 into programming mode and load programming executive and actual program code
- [Arduino ICSP Programmer Repo for PIC16](https://github.com/jaromir-sukuba/a-p-prog)
    - similar project to ours but there are many differences between PIC16 family and PIC32 family
- [PIC32MX Section 5: Flash Programming Documentation](https://ww1.microchip.com/downloads/en/DeviceDoc/60001121g.pdf
    - more general overview of flash programming than first link
)
- [PIC32 Programming and Diagnostics](https://ww1.microchip.com/downloads/en/DeviceDoc/61129F.pdf
)
- [PIC32 Family Datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU32/ProductDocuments/DataSheets/PIC32MX1XX2XX283644-PIN_Datasheet_DS60001168L.pdf
)
    - useful for PIC32 pinouts 
- [ICSP Guide](https://ww1.microchip.com/downloads/en/DeviceDoc/30277d.pdf
)
- [General Overview of ICSP](https://en.wikipedia.org/wiki/In-system_programming
)
- [Technical Guide to JTAG](https://www.xjtag.com/about-jtag/jtag-a-technical-overview/)
- [ArduPIC32 Repo - Arduino JTAG programmer for PIC32](https://github.com/tekaikko/ardupic32)


