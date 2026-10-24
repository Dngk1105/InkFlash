# RT9080 - Hardware Technical Specification

> **Extracted via:** DeepSheet + Docling Pipeline  
> **Source Document:** RT9080.pdf  

---

## Page 1


RT9080
2A I , 600mA Low-Dropout Linear Voltage Regulator
Q
1 General Description 3 Features
The RT9080 series is a low-dropout (LDO) linear ⚫ 2A Quiescent Current at No Load
voltage regulator featuring an enable function, ⚫ PSRR = 75dB at 1kHz
operating over a wide input voltage range from 1.2V to ⚫ Adjustable Output Voltage Available for Specific
5.5V. The RT9080 delivers a maximum output current Applications
of 600mA and is designed for low-power operation, all ⚫ 2% Output Accuracy
within a compact package. ⚫ 600mA (VIN  2.3V) Output Current
Low (0.1A) Disable Current
The features of low quiescent current, as low as 2A, ⚫
Operating Input Voltage: 1.2V to 5.5V
and almost zero disable current, are ideal for powering ⚫
battery equipment for a longer service life. The ⚫
Dropout Voltage: 0.31V at 600mA when VOUT  3V
Support Fixed Output Voltage: 0.8V to 3.3V
RT9080 series is stable with the ceramic output ⚫
Stable with Ceramic or Tantalum Capacitor
capacitor over its wide input range from 1.2V to 5.5V ⚫
Current-Limit Protection
and the entire range of output load current (0mA to ⚫
Over-Temperature Protection
600mA). The recommended junction temperature ⚫
Available in TSOT-23-5 and ZQFN-4L 1x1 (ZDFN-
range is −40C to 125C, and the ambient temperature ⚫
4L 1x1) Packages
range is −40C to 85C.
4 Applications
2 Ordering Information
Portable, Battery Powered Equipment
RT9080- ⚫
Pin 1 Orientation ⚫ Ultra Low Power Microcontrollers
Empty: Quadrant 1 or Quadrant 3
Notebook Computers
(2): Quadrant 2, Follow EIA-481 ⚫
(ZQFN-4L 1x1 Only)
5 Marking Information
(1)
Package Type
For marking information, contact our sales
J5: TSOT-23-5
QZ: ZQFN-4L 1x1 (Z-Type) representative directly or through a Richtek distributor
located in your area.
Lead Plating System
G: Richtek Green Policy Compliant
(2)
Output Voltage
08 : 0.8V
:
2H : 2.85V
:
33 : 3.3V
Special Request : Any voltage between
0.8V and 3.3V under specific business
agreement
Pin Function
RT9080: Without SNS Pin
RT9080N: With SNS Pin
Note 1.
⚫ Marked with (1) indicated: Compatible with the current
requirements of IPC/JEDEC J-STD-020.
⚫ Marked with (2) indicated: Richtek products are Richtek
Green Policy compliant.
Copyright © 2024 Richtek Technology Corporation. All rights reserved. is a registered trademark of Richtek Technology Corporation.
DS9080-09 December 2024 www.richtek.com
1


---

## Page 2


RT9080
6 Simplified Application Circuit
Copyright © 2024 Richtek Technology Corporation. All rights reserved. is a registered trademark of Richtek Technology Corporation.
www.richtek.com DS9080-09 December 2024
2
V
E
IN
N
C1 IN
μ F
V
E
IN
N
R T
G
9
N
0 8
D
0
V O
S
U T
N S *
R 1
R 2
*: P in o p
C O U
tio n b
T
y p a rt n u m
V O U T
b e r


---

## Page 3


RT9080
Table of Contents
1 General Description ---------------------------------------- 1 18 Application Information ---------------------------------- 13
2 Ordering Information -------------------------------------- 1 18.1 Enable ----------------------------------------------- 13
3 Features -------------------------------------------------------- 1 18.2 Adjustable Output Voltage Setting ------------ 13
4 Applications -------------------------------------------------- 1 18.3 Current Limit ---------------------------------------- 14
5 Marking Information ---------------------------------------- 1 18.4 Thermal Considerations ------------------------- 14
6 Simplified Application Circuit --------------------------- 2 18.5 Layout Considerations --------------------------- 15
7 Pin Configuration ------------------------------------------- 4 19 Outline Dimension ----------------------------------------- 17
8 Functional Pin Description ------------------------------ 4 19.1 TSOT-23-5 ------------------------------------------ 17
9 Functional Block Diagram ------------------------------- 4 19.2 ZQFN-4L 1x1 Package -------------------------- 18
10 Absolute Maximum Ratings ----------------------------- 5 20 Footprint Information ------------------------------------- 19
11 ESD Ratings -------------------------------------------------- 5 20.1 TSOT-23-5 Package ----------------------------- 19
12 Recommended Operating Conditions --------------- 5 20.2 ZQFN-4L 1x1 Package -------------------------- 20
13 Thermal Information --------------------------------------- 5 21 Packing Information --------------------------------------- 21
14 Electrical Characteristics --------------------------------- 6 21.1 Tape and Reel Data ------------------------------ 21
15 Typical Application Circuit ------------------------------- 7 21.2 Tape and Reel Packing -------------------------- 24
16 Typical Operating Characteristics --------------------- 8 21.3 Packing Material Anti-ESD Property --------- 27
17 Operation ----------------------------------------------------- 12 22 Datasheet Revision History ----------------------------- 28
17.1 Basic Operation ----------------------------------- 12
17.2 Pass Transistor ------------------------------------ 12
17.3 Error Amplifier -------------------------------------- 12
17.4 Chip Enable and Shutdown -------------------- 12
17.5 Current-Limit Protection ------------------------- 12
17.6 Over-Temperature Protection ------------------ 12
17.7 Output Active Discharge ------------------------- 12
Copyright © 2024 Richtek Technology Corporation. All rights reserved. is a registered trademark of Richtek Technology Corporation.
DS9080-09 December 2024 www.richtek.com
3


---

## Page 4


RT9080
7 Pin Configuration
(TOP VIEW)
Copyright © 2024 Richtek Technology Corporation. All rights reserved. is a registered trademark of Richtek Technology Corporation.
www.richtek.com DS9080-09 December 2024
4
V O
V
U
5
IN
T
G
2
N D
S N S
E
/N
4
3
N
C
VOUT 1 4 VIN
SGND
GND 2 5 3 EN
TSOT-23-5 ZQFN-4L 1x1 (ZDFN-4L 1x1)
8 Functional Pin Description
Pin No.
ZQFN-4L 1x1 Pin Name Pin Function
TSOT-23-5
(ZDFN-4L 1x1)
Supply input. A general 1F ceramic capacitor should be placed as
1 4 VIN
close as possible to this pin for better noise rejection.
Ground. The exposed pad must be soldered to a large PCB and
2 2 GND
connected to GND for maximum power dissipation.
Enable control input. Connect this pin to logic-high to enable the
3 3 1EN regulator. Pull this pin below 0.4V to turn the regulator off, reducing
the quiescent current to a fraction of its operating value.
Output voltage sense pin for RT9080N only. This pin is used to set
SNS the desired output voltage via an external resistive divider. The
SNS pin voltage is 0.8V typically.
4 -- No internal connection. Leaving this pin floating does not affect the
functionality of the device. By connecting this pin to GND, design
NC
engineers can extend the GND copper coverage on the PCB top
layer to enhance the thermal convection.
LDO output pin. A 1F or larger ceramic capacitor (0.7F or greater
effective capacitance) is required for stability. Place the output
5 1 VOUT
capacitor as close to the device as possible and minimize the
impedance between the VOUT pin and load.
5 (Exposed Substrate of the IC. Connect to the GND plane for maximum
-- SGND
Pad) thermal dissipation.
9 Functional Block Diagram
VIN VOUT
GND Current/Thermal
Sense SNS
R1
Bandgap
EN
Reference R2
- +
(without sense
function)
(with sense
function)
EN


---

## Page 5


RT9080
10 Absolute Maximum Ratings
(Note 2)
VIN, VOUT, SNS, EN to GND --------------------------------------------------------------------------------- −0.3V to 6.5V
⚫
VOUT to VIN ------------------------------------------------------------------------------------------------------- −6.5V to 0.3V
⚫
Lead Temperature (Soldering, 10 sec.) --------------------------------------------------------------------- 260C
⚫
Junction Temperature ------------------------------------------------------------------------------------------- 150C
⚫
Storage Temperature Range ---------------------------------------------------------------------------------- −65C to 150C
⚫
Note 2. Stresses beyond those listed under “Absolute Maximum Ratings” may cause permanent damage to the device. These
are stress ratings only, and functional operation of the device at these or any other conditions beyond those indicated
in the operational sections of the specifications is not implied. Exposure to absolute maximum rating conditions may
affect device reliability.
11 ESD Ratings
(Note 3)
ESD Susceptibility
⚫
HBM (Human Body Model) ------------------------------------------------------------------------------------- 2kV
Note 3. Devices are ESD sensitive. Handling precautions are recommended.
12 Recommended Operating Conditions
(Note 4)
Input Voltage, VIN ------------------------------------------------------------------------------------------------ 1.2V to 5.5V
⚫
Junction Temperature Range ---------------------------------------------------------------------------------- −40C to 125C
⚫
Ambient Temperature Range ---------------------------------------------------------------------------------- −40C to 85C
⚫
Note 4. The device is not guaranteed to function outside its operating conditions.
13 Thermal Information
(Note 5 and Note 6)
ZQFN-4L 1x1
Thermal Parameter TSOT-23-5 Unit
(ZDFN-4L 1x1)
JA Junction-to-ambient thermal resistance (JEDEC standard) 189.4 291.4 C/W
JC(Top) Junction-to-case (top) thermal resistance 75.9 163 C/W
JC(Bottom) Junction-to-case (bottom) thermal resistance 55.8 90.7 C/W
JA(EVB) Junction-to-ambient thermal resistance (specific EVB) 100.7 236 C/W
JC(Top) Junction-to-top characterization parameter 21.6 52.3 C/W
JB Junction-to-board characterization parameter 67.2 189.1 C/W
Note 5. For more information about thermal parameter, see the Application and Definition of Thermal Resistances report,
AN061.
Note 6. 
JA
(EVB),  JC(Top), and 
JB
are measured on a high effective-thermal-conductivity four-layer test board which is in size
of 70mm x 50mm; furthermore, all layers with 1 oz. Cu. Thermal resistance/parameter values may vary depending on
the PCB material, layout, and test environmental conditions.
Copyright © 2024 Richtek Technology Corporation. All rights reserved. is a registered trademark of Richtek Technology Corporation.
DS9080-09 December 2024 www.richtek.com
5


---

## Page 6


RT9080
14 Electrical Characteristics
(VVOUT + 1 < VIN < 5.5V, TA = 25C, unless otherwise specified.)
Parameter Symbol Test Conditions Min Typ Max Unit
Output Voltage VVOUT 0.8 -- 3.3 V
Output Voltage Accuracy VVOUT_ACC ILOAD = 1mA −2 -- 2 %
SNS Reference Voltage
(for RT9080N-08GJ5 VREF ILOAD = 1mA -- 0.8 -- V
only)
0.8V  VVOUT  1.05V (TSOT-23-5) -- 1.05 1.33
0.8V  VVOUT  1.05V (ZQFN-4L 1x1) -- 1.05 1.63
1.05V  VVOUT  1.2V -- 0.8 1.13
Dropout Voltage
1.2V  VVOUT  1.5V -- 0.71 1.03
(ILOAD = 600mA) VDROP 1.5V  VVOUT  1.8V -- 0.57 0.93 V
(Note 7)
1.8V  VVOUT  2.1V -- 0.57 0.83
2.1V  VVOUT  2.5V -- 0.41 0.73
2.5V  VVOUT  3V -- 0.36 0.63
3V  VVOUT -- 0.31 0.53
ILOAD = 0mA, VVOUT  5.5V
Quiescent Current IQ -- 2 4 A
VVIN  VVOUT + VDROP
Shutdown GND Current VEN = 0V -- 0.1 0.5 A
(Note 8)
ISHDN
VEN = 0V, VVOUT = 0V -- 0.1 0.5 A
EN Input Current IEN VEN = 5.5V -- -- 0.1 A
1.2V  VVIN  1.5V -- 0.3 0.6
Line Regulation VLINE_REG ILOAD = 1mA 1.5V  VVIN  1.8V -- 0.15 0.3 %
1.8V  VVIN  5.5V -- 0.13 0.35
Load Regulation VLOAD_REG 1mA < ILOAD < 600mA -- 0.5 1 %
Power Supply Rejection VVIN = 3V, ILOAD = 50mA,
PSRR -- 75 -- dB
Ratio COUT = 1F, VVOUT = 2.5V, f = 1kHz
COUT = 1F, VVOUT = 0.8V -- 26 --
ILOAD = 150mA,
VVOUT = 1.2V -- 37 --
Output Voltage Noise Vn BW = 10Hz to VRMS
100kHz,
VVOUT = 1.8V -- 39 --
VVIN = VVOUT + 1V VVOUT = 3.3V -- 42 --
Current Limit ILIM VVOUT = 90%VOUT(Normal) 610 1100 -- mA
EN Input Voltage Rising
VEN_R VVIN = 5V 0.9 -- --
Threshold
V
EN Input Voltage Falling
VEN_F VVIN = 5V -- -- 0.4
Threshold
Over‐Temperature
TOTP ILOAD = 30mA, VVIN  1.5V -- 150 -- C
Protection Threshold
Over‐Temperature
TOTP_HYS -- 20 -- C
Protection Hysteresis
Discharge Resistor RDISCHG EN = 0V, VVOUT = 0.1V -- 80 -- 
Note 7. The dropout voltage is defined as VVIN − VVOUT when VVOUT is 98% of the normal value of VVOUT.
Note 8. The specification is tested at wafer stage and guaranteed by design after assembly.
Copyright © 2024 Richtek Technology Corporation. All rights reserved. is a registered trademark of Richtek Technology Corporation.
www.richtek.com DS9080-09 December 2024
6


---

## Page 7


RT9080
15 Typical Application Circuit
Copyright © 2024 Richtek Technology Corporation. All rights reserved. is a registered trademark of Richtek Technology Corporation.
DS9080-09 December 2024 www.richtek.com
7
V
E
IN
N
C1 IN
μ F
V
E
IN
N
R T
G
9
N
0 8
D
0
V O
S
U
N
T
S
F o r R T 9 0 8 0
C(E
N
O U
ffe
Tc
tiv e C
V O
a p
U
a
T
c ita n c e  1 μ F )
Figure 1. Application with Sense Function (RT9080) / without Sense Function (RT9080N)
RT9080N
VIN
CIN
VIN VOUT VOUT
1μF R1 COUT
(Effective Capacitance 1μF)
SNS
EN EN
R2
GND

Figure 2. Adjustable Output Voltage Application Circuit


---

## Page 13


RT9080
18 Application Information
(Note 9)
Like any low dropout linear voltage regulator, the RT9080’s external input and output capacitors must be properly
selected for stability and performance. Use a 1F or larger input capacitor and place it close to the IC's VIN and
GND pins. Any output capacitor meeting the minimum 1m ESR (Equivalent Series Resistance) requirement and
having a capacitance larger than 1F may be used. Place the output capacitor close to the IC's VOUT and GND
pins. Increasing capacitance and decreasing ESR can improve the circuit's PSRR and line transient response.
18.1 Enable
The RT9080 series has an EN pin to turn on or turn off the regulator. When the EN pin is in logic-high, the regulator
will be turned on. The shutdown current is almost 0A typical. The EN pin may be directly tied to VIN to keep the
part on. The Enable input is CMOS logic and should not be left floating.
18.2 Adjustable Output Voltage Setting
Because of the small input current at the SNS pin, the RT9080N with the SNS pin can also work as an adjustable
output voltage LDO. Figure 2 gives the connections for the adjustable output voltage application. The resistor
divider from VOUT to SNS sets the output voltage when in regulation.
The voltage on the SNS pin sets the output voltage and is determined by the values of R1 and R2. To maintain a
good temperature coefficient of the output voltage, the values of R1 and R2 should be selected with consideration
for the temperature coefficient of the input current at the SNS pin. A current greater than 50A in the resistor
divider is recommended to meet the above requirement. The adjustable output voltage can be calculated using
the formula provided in Equation 1:
R1 + R2
VOUT = VSNS (1)
R2
where VSNS is determined by the output voltage options listed in the ordering information for the RT9080N. The
maximum adjustable output voltage can be as high as the input voltage minus the dropout voltage.
When we choose 51k and 16k as R1 and R2, respectively, and select a 0.8V output at the SNS pin, the
adjustable output voltage will be approximately 3.35V. The temperature coefficient, as shown in Figure 4, remains
excellent for this type of application.
Figure 3. Temperature Coefficient of Adjustable Output Voltage
Copyright © 2024 Richtek Technology Corporation. All rights reserved. is a registered trademark of Richtek Technology Corporation.
DS9080-09 December 2024 www.richtek.com
13


---

## Page 14


RT9080
The minimum recommended current of 50A in the resistor divider makes the application no longer an ultra-low
quiescent LDO. Figure 4 presents another fine adjustable output voltage application that allow the LDO to continue
operating with low power consumption. The fine-tuning range is recommended to be less than 50mV (R1  91k)
to maintain a good temperature coefficient of the output voltage.
Copyright © 2024 Richtek Technology Corporation. All rights reserved. is a registered trademark of Richtek Technology Corporation.
www.richtek.com DS9080-09 December 2024
14
1 μ F
V
E
IN
N
R T 9
G
0
N
8
D
0 N
V O
S
U
N
T
S
R 1
5 6 p F /N C
1 μ F
Figure 4. Fine Adjustable Output Voltage Application Circuit
There is no additional current consumption in the above application. However, the temperature coefficient of the
output voltage will be degraded by the input current at the SNS pin. If the tuning range is larger than 50mV, a
compensation capacitor (56pF) is required to maintain the stability of the output voltage. The fine adjustable output
voltage can be calculated using the formula provided in Equation 2:
VOUT =VSNS + ISNSR1 (2)
where ISNS is the input current at the SNS pin (typically 550nA at room temperature) and VSNS is determined by
the output voltage options listed in the ordering information for the RT9080N.
18.3 Current Limit
The RT9080 series features an independent current limiter that monitors and controls the gate voltage of the pass
transistor, limiting the output current to 1.1A (typical). When the output voltage decreases further, the current
limiting level is reduced to around 0.6A, which is named fold-back current limit. This allows the output to be shorted
to ground indefinitely without damaging the device.
18.4 Thermal Considerations
For continuous operation, do not exceed absolute maximum junction temperature. The maximum power
dissipation depends on the thermal resistance of the IC package, PCB layout, rate of surrounding airflow, and
difference between junction and ambient temperature. The maximum power dissipation can be calculated by the
following formula:
PD(MAX) = (TJ(MAX) − TA) / JA
where TJ(MAX) is the maximum junction temperature, TA is the ambient temperature, and JA is the junction to
ambient thermal resistance.
For recommended operating condition specifications, the maximum junction temperature is 125C and TA is the
ambient temperature. The junction to ambient thermal resistance, θJA(EVB), is highly package dependent. For
TSOT-23-5 package, the thermal resistance, θJA(EVB), is 100.7C/W on a standard JEDEC 51-7 four-layer thermal
test board. For ZQFN-4L 1x1 (ZDFN-4L 1x1) package, the thermal resistance, θJA(EVB), is 236C/W on a two-layer
Richtek evaluation board. The maximum power dissipation at TA = 25C can be calculated by the following formula:
PD(MAX) = (125C − 25C) / (100.7C/W) = 0.99W for TSOT-23-5 package
PD(MAX) = (125C − 25C) / (236C/W) = 0.42W for ZQFN-4L 1x1 (ZDFN-4L 1x1) package
The maximum power dissipation depends on the operating ambient temperature for fixed TJ(MAX) and thermal
resistance, θJA(EVB). The derating curves in Figure 5 allow the designer to see the effect of rising ambient
temperature on the maximum power dissipation.


---

## Page 15


RT9080
1.4
1.2
1.0
0.8
0.6
0.4
0.2
0.0
0 25 50 75 100 125
Figure 5. Derating Curves of Maximum Power Dissipation
18.5 Layout Considerations
For best performance of the RT9080 series, the PCB layout suggestions below are highly recommended:
Input capacitor must be placed as close as possible to the IC to minimize the power loop area.
⚫
Minimize the power trace length and avoid using vias for the input and output capacitors connections.
⚫
Figure 6 and Figure 7 show the layout examples that help minimize inductive parasitic components, reduce load
transients, and ensure good circuit stability.
Copyright © 2024 Richtek Technology Corporation. All rights reserved. is a registered trademark of Richtek Technology Corporation.
DS9080-09 December 2024 www.richtek.com
15
1
)W(
noitapissiD
rewoP
mumixaM
Four-Layer PCB for TSOT-23-5 package
Two-Layer Richtek EVB for ZQFN (ZDFN)-4L 1x1
package
ZQFN-4L 1x1 (ZDFN-4L 1x1)
TSOT-23-5
Ambient Temperature (°C)


---

## Page 28


RT9080
22 Datasheet Revision History
Version Date Description Item
08 2024/3/22 Modify Rewrite
General Description on page 1
Features on page 1
Ordering Information on page 1
Electrical Characteristics on page 6
09 2024/11/11 Modify
Typical Application Circuit on page 7
Application Information on page 13, 16
Packing Information on page 20 to 26
-Added packing information
Copyright © 2024 Richtek Technology Corporation. All rights reserved. is a registered trademark of Richtek Technology Corporation.
www.richtek.com DS9080-09 December 2024
28
