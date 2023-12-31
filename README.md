# DeskControl LOGICDATA

The code represents a custom desk controller for controls from the company [LOGICDATA](https://www.logicdata.net/).  
It was designed for usage with a desk equipped with the 2 button [HSM-OD](https://web.archive.org/web/20231231132102/https://www.logicdata.net/de/wp-content/uploads/sites/2/2017/06/Datasheet_HSM_German-Rev4.pdf) handset with the 7 pin MAS 70 connector (DIN 45329).  
The handset is connected e.g. to a LOGICDATA COMPACT control.

### Warning
Note this warning in the LOGICDATA literature:  
Danger: it is not allowed to connect self constructed products to
LOGICDATA motor controls.  
To prevent damage of the unit, use only
components suitable for LOGICDATA motor controls.

### Credits
The control is based on work by other people:  
- desk control app for a computer written in C++ [https://www.mikrocontroller.net/topic/373579](https://www.mikrocontroller.net/topic/373579)  
- decoding the logicdata protocol [https://www.mikrocontroller.net/topic/369941](https://www.mikrocontroller.net/topic/369941)  

## Current features
- Double tap a direction to go to a target height
- Saving a lower and upper height by pressing both bottons simultaneously
- Recognition when the LOGICDATA ISP (Intelligent System Protection) is triggered

## Hardware
- [Arduino Pro Mini 5V (or clone)](https://docs.arduino.cc/retired/boards/arduino-pro-mini)
- 7-Pin DIN connector splitter (1 male -> 2 female): This is used, that both the original handset and the control can be easily connected and replaced. The split cable is available under the part number: ACS-CBL-HSY  
Alternatively it can be purchased on [amazon.de](https://www.amazon.de) or self made.
- 7-Pin DIN connector male: This connector is connected to the Arduino and the splitter

The splitter connects the control box with the stock hand switch and Arduino.  
```   
   A                    A (male)   ... LOGICDATA control box (female)
   |  
  / \                   B (female) ... original hand switch (male)
 B   C                  C (female) ... Arduino Pro Mini (male)
```

## Functionality
The function of the DeskController differs from that of [boris_b](https://www.mikrocontroller.net/topic/373579).  
It is not a hardware MITM solution. The original handset always remains connected to the control unit via the splitter and therefore sends all signals.  
The DeskController does not intervene in manual driving mode. 

However, if a double-click is detected (2 presses <500ms), the desk drives until a programmed height is reached.  
The current height is read from the protocol via a software serial.  
Two different heights can be programmed (sitting and standing), which are stored in the EEPROM and are therefore retained even after a power loss.  
If no values have been saved, the default values upperLimit = 115 and lowerLimit = 78 are used.

In addition, the program monitors the change in height during the automatic run.  
If the driving of the desk is interrupted, the output is switched off and a red LED indicates that monitoring has been triggered.  
This occurs if, for example 
- the down arrow is pressed during the upward driving or vice versa
- The ISP triggers

## Schematic
The schematic shows the connections needed from the Arduino Pro Mini to the 7-pin connector. 
The numbering are when you view the connector on the solder side.
![schematic](/images/schematic.png)

## Links
- project for a standing desk with a handset with display: [https://github.com/phord/RoboDesk](https://github.com/phord/RoboDesk)  
- fork of RoboDesk based on the d1 mini: [https://github.com/mtfurlan/RoboDesk](https://github.com/mtfurlan/RoboDesk)  
- Info on additional hardware: [baumeister-schack.de](https://web.archive.org/web/20231231143816/https://www.baumeister-schack.de/sites/default/files/2022-09/22_data_sheet_-steuerung-compact_de2022.pdf)