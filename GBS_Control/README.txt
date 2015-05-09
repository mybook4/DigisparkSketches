Quick notes:

"ProgramArray240p.h" and "ProgramArray480i.h" were created
from dooklink's GBS 8200/8220 Custom Firmware project 
(https://github.com/dooklink/gbs-control/blob/master/settings/).  

The values were transposed to a C array so they could be utilized 
in the microcontroller program.  To use custom values, simply 
replace the contents of the array(s) or add new files with new 
arrays (and include/use as needed in the GBS_Control.ino file).


This program was tested to work a Digispark Pro microcontroller board.
An illustration of pins is provided in the file DigisparkProDiagram2.png

The setup was as follows (obviously not to scale):

-------------------        --------------   VGA     --------
|RGB SCART source |  SCART |  GBS 8220  |---------->|  TV  |
|with LM1881 sync |------->|(P8 shorted)|           --------
|stripper inline  |        --------------
-------------------             | | | P5
                                | | | 
                                | | | 
                          |-----| | |-----| 
                          |       |       |
                      SDA |       | GND   | SCL
                   --------------------------------
                   |    PB0      GND     PB2      | 
                   |                              | 
                   |        Digispark Pro         | 
                   |                              | 
                   |    VCC      PA7     GND      | 
                   --------------------------------
                         |        |       |
            |--/\/\/\/---|        |--||---|---/\/\/\/--|
            |    R1               |  C1         R2     |
            |                     |                    |
            |              ---------------             |
            |--------------| SPDT switch |-------------|
               "240p"      ---------------    "480i"
                                 |_|

The values of R1, R2, and C1 are dependent upon the bouncing time of your SPDT switch.

