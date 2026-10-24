# TPS22916 - Hardware Technical Specification

> **Extracted via:** DeepSheet + Docling Pipeline  
> **Source Document:** TPS22916.pdf  

---

## Page 1


TPS22916
SLVSDO5F – JULY 2017 – REVISED DECEMBER 2021
TPS22916xx 1-V–5.5-V, 2-A, 60-mΩ Ultra-Low Leakage Load Switch
1 Features 3 Description
• Input operating voltage range (V ): 1 V–5.5 V The TPS22916xx is a small, single channel load
IN
• Maximum continuous current (I ): 2 A switch using a low leakage P-Channel MOSFET for
MAX
• ON-resistance (R ): minimum power loss. Advanced gate control design
ON
– 5 V = 60 mΩ (typ.), 100 mΩ (85 °C max.) supports operating voltages as low as 1 V with
IN
– 1.8 V = 100 mΩ (typ.), 150 mΩ (85 °C max.) minimal increase in ON-resistance and power loss.
IN
– 1 V = 200 mΩ (typ.), 325 mΩ (85 °C max.)
IN Multiple timing options are available to support
• Ultra-low power consumption:
various system loading conditions. For heavy
– ON state (I Q ): 0.5 µA (typ.), 1 µA (max.) capacitive loads, the slow turn-on timing in the C
– OFF state (I SD ): 10 nA (typ.), 100 nA (max.) version minimizes the inrush current. In cases with
– TPS22916BL/CL/CNL (I SD ): 100 nA (typ.), 300 light capacitive loads, the fast timing in the B version
nA (max.) reduces required wait time.
• Smart ON pin pulldown (R ):
PD
The switch ON state is controlled by a digital input
– ON ≥ V (I ): 10 nA (max.)
IH ON
that is capable of interfacing directly with low-voltage
– ON ≤ V (R ): 750 kΩ (typ.)
IL PD
control signals. Both Active High and Active Low (L)
• Slow Timing in C Version Limits Inrush Current:
versions are available. When power is first applied,
– 5-V turn-on time (t ): 1400 µs at 5 mV/µs
ON a smart pulldown is used to keep the ON pin from
– 1.8-V turn-on time (t ): 3000 µs at 1 mV/µs
ON floating until system sequencing is complete. AFter
– 1-V turn-on time (t ): 6500 µs at 0.3 mV/µs
ON the ON pin is deliberately driven high (≥V ), the smart
• Fast timing in b version reduces wait time: IH
pulldown is disconnected to prevent unnecessary
– 5-V turn-on time (t ): 115 µs at 57 mV/µs
ON power loss.
– 1.8-V turn-on time (t ): 250 µs at 12 mV/µs
ON
– 1-V turn-on time (t ): 510 µs at 3.3 mV/µs The TPS22916xx is available in a small, space
ON
• Always-ON true Reverse Current Blocking (RCB): saving 0.78 mm × 0.78 mm, 0.4-mm pitch, 0.5-
– Activation current (I ): –500 mA (typ.) mm height 4-pin Wafer-Chip-Scale (WCSP) package
RCB
– Reverse leakage (I ): –300 nA (max.) (YFP). The device is characterized for operation over
IN,RCB
• Quick Output Discharge (QOD): 150 Ω (typ.) a temperature range of –40 °C to +85 °C.
(N version has no QOD) Device Information(1)
• Active low enable option (L versions)
PART NUMBER PACKAGE BODY SIZE (NOM)
2 Applications TPS22916xx WCSP (4) 0.78 mm × 0.78 mm
• Wearables (1) For all available packages, see the orderable addendum at
• Smartphones the end of the data sheet.
• Tablets Device Comparison Table
• Portable speakers
VERSION TIMING QOD ENABLE (ON)
TPS22916B Fast Yes Active High
TPS22916BL Fast Yes Active Low
TPS22916C Slow Yes Active High
TPS22916CN Slow No Active High
TPS22916CL Slow Yes Active Low
TPS22916CNL Slow No Active Low
VIN VOUT
VIN +
±
CIN CL RL
H
ON GND
TPS22916xx
L
Copyright © 2017, Texas Instruments Incorporated
Simplified Schematic
An IMPORTANT NOTICE at the end of this data sheet addresses availability, warranty, changes, use in safety-critical applications,
intellectual property matters and other important disclaimers. PRODUCTION DATA.


---

## Page 2


TPS22916
SLVSDO5F – JULY 2017 – REVISED DECEMBER 2021 www.ti.com
Table of Contents
1 Features............................................................................1 9 Application and Implementation..................................18
2 Applications.....................................................................1 9.1 Application Information.............................................18
3 Description.......................................................................1 9.2 Typical Application....................................................18
4 Revision History..............................................................2 10 Power Supply Recommendations..............................19
5 Pin Configuration and Functions...................................3 11 Layout...........................................................................20
6 Specifications..................................................................4 11.1 Layout Guidelines...................................................20
6.1 Absolute Maximum Ratings........................................4 11.2 Layout Example......................................................20
6.2 ESD Ratings...............................................................4 11.3 Thermal Considerations..........................................20
6.3 Recommended Operating Conditions.........................4 12 Device and Documentation Support..........................21
6.4 Thermal Information....................................................4 12.1 Documentation Support..........................................21
6.5 Electrical Characteristics.............................................5 12.2 Receiving Notification of Documentation Updates..21
6.6 Switching Characteristics............................................6 12.3 Support Resources.................................................21
6.7 Typical Characteristics................................................8 12.4 Trademarks.............................................................21
7 Parameter Measurement Information..........................15 12.5 Electrostatic Discharge Caution..............................21
8 Detailed Description......................................................16 12.6 Glossary..................................................................21
8.1 Overview...................................................................16 13 Mechanical, Packaging, and Orderable
8.2 Functional Block Diagram.........................................16 Information....................................................................21
8.3 Feature Description...................................................16 13.1 Tape and Reel Information......................................22
8.4 Device Functional Modes..........................................17
4 Revision History
NOTE: Page numbers for previous revisions may differ from page numbers in the current version.
Changes from Revision E (September 2020) to Revision F (December 2021) Page
• Added TPS22916CNL and TPS22916BL orderables to the data sheet.............................................................1
Changes from Revision D (October 2019) to Revision E (September 2020) Page
• Updated the numbering format for tables, figures and cross-references throughout the document...................1
Changes from Revision C (October 2018) to Revision D (October 2019) Page
• Changed package dimensions from 0.74 mm x 0.74 mm to 0.78 mm x 0.78 mm..............................................1
Changes from Revision B (December 2017) to Revision C (October 2018) Page
• Changed Package Drawing Dimensions .........................................................................................................21
Changes from Revision A (September 2017) to Revision B (December 2017) Page
• Changed Pinout drawing labeled Laser Marking................................................................................................1
Changes from Revision * (July 2017) to Revision A (September 2017) Page
• Changed device document from Advanced Info to Production Data .................................................................1
2 Submit Document Feedback Copyright © 2021 Texas Instruments Incorporated
Product Folder Links: TPS22916


---

## Page 3


TPS22916
www.ti.com SLVSDO5F – JULY 2017 – REVISED DECEMBER 2021
5 Pin Configuration and Functions
ON GND GND ON
B B
VIN VOUT VOUT VIN
A A
2 1 1 2
Figure 5-1. YFP Package 4-Pin WSON Laser Figure 5-2. YFP Package 4-Pin WSON Bump View
Marking View
Table 5-1. Pin Functions
PIN
TYPE DESCRIPTION
NO. NAME
A1 VOUT Power Switch output
A2 VIN Power Switch input
B1 GND Ground Device ground
B2 ON Digital input Device enable
Copyright © 2021 Texas Instruments Incorporated Submit Document Feedback 3
Product Folder Links: TPS22916


---

## Page 4


TPS22916
SLVSDO5F – JULY 2017 – REVISED DECEMBER 2021 www.ti.com
6 Specifications
6.1 Absolute Maximum Ratings
Over operating free-air temperature range (unless otherwise noted)(1)
MIN MAX UNIT
V Input voltage –0.3 6 V
IN
V Output voltage –0.3 6 V
OUT
V Enable voltage –0.3 6 V
ON
I Maximum continuous switch current 2 A
MAX
I Maximum pulsed switch current, pulse < 300-µs, 2% duty cycle 2.5 A
PLS
T Maximum junction temperature 125 °C
J,MAX
T Storage temperature –65 150 °C
STG
T Maximum Lead temperature (10-s soldering time) 300 °C
LEAD
(1) Stresses beyond those listed under Absolute Maximum Ratings may cause permanent damage to the device. These are stress
ratings only, which do not imply functional operation of the device at these or any other conditions beyond those indicated under
Recommended Operating Conditions. Exposure to absolute-maximum-rated conditions for extended periods may affect device
reliability.
6.2 ESD Ratings
VALUE UNIT
Human-body model (HBM), per ANSI/ESDA/JEDEC JS-001(1) ±2000
V (ESD) Electrostatic discharge Charged-device model (CDM), per JEDEC specification JESD22- V
±500
C101(2)
(1) JEDEC document JEP155 states that 500-V HBM allows safe manufacturing with a standard ESD control process. Manufacturing with
less than 500-V HBM is possible with the necessary precautions. Pins listed as ±2000 V may actually have higher performance.
(2) JEDEC document JEP157 states that 250-V CDM allows safe manufacturing with a standard ESD control process. Manufacturing with
less than 250-V CDM is possible with the necessary precautions. Pins listed as ±500 V may actually have higher performance.
6.3 Recommended Operating Conditions
Over operating free-air temperature range (unless otherwise noted)
MIN MAX UNIT
V Input voltage 1 5.5 V
IN
V Output voltage 0 5.5 V
OUT
V High-level input voltage, ON 1 5.5 V
IH
V Low-level input voltage, ON 0 0.35 V
IL
T Operating free-air temperature –40 85 °C
A
6.4 Thermal Information
TPS22916xx
Thermal Parameters(1) YFP (WCSP) UNIT
4 PINS
θ Junction-to-ambient thermal resistance 193 °C/W
JA
θ Junction-to-case (top) thermal resistance 2.3 °C/W
JCtop
θ Junction-to-board thermal resistance 36 °C/W
JB
ψ Junction-to-top characterization parameter 12 °C/W
JT
ψ Junction-to-board characterization parameter 36 °C/W
JB
(1) For more information about traditional and new thermal metrics, see the Semiconductor and IC Package Thermal Metrics application
report.
4 Submit Document Feedback Copyright © 2021 Texas Instruments Incorporated
Product Folder Links: TPS22916


---

## Page 5


TPS22916
www.ti.com SLVSDO5F – JULY 2017 – REVISED DECEMBER 2021
6.5 Electrical Characteristics
Unless otherwise noted, the specification in the following table applies for all variants over the entire recommended power
supply voltage range of 1 V to 5.5 V unless noted otherwise. Typical Values are at 25 °C.
PARAMETER TEST CONDITIONS T MIN TYP MAX UNIT
J
INPUT SUPPLY (VIN)
I V Quiescent current Enabled, V = Open –40 °C to +85 °C 0.5 1.0 µA
Q,VIN IN OUT
I V Shutdown current Disabled, V = GND –40 °C to +85 °C 10 100 nA
SD,VIN IN OUT
(TPS22916B/C/CN)
Disabled, V = GND (TPS22916BL/CL/ –40 °C to +85 °C 100 300
OUT nA
CNL)
ON-RESISTANCE
(R )
ON
25 °C 60 80
V = 5 V –40 °C to +85 °C 100
IN
–40 °C to +105 °C 120
25 °C 70 90
V = 3.6 V –40 °C to +85 °C 120
IN
–40 °C to +105 °C 140
25 °C 100 125
R ON-Resistance I = 200 mA V = 1.8 V –40 °C to +85 °C 150 mΩ
ON OUT IN
–40 °C to +105 °C 175
25 °C 150 200
V = 1.2 V –40 °C to +85 °C 250
IN
–40 °C to +105 °C 300
25 °C 200 275
V = 1 V –40 °C to +85 °C 325
IN
–40 °C to +105 °C 375
ENABLE PIN (ON)
I ON Pin leakage Enabled –40 °C to +85 °C –10 10 nA
ON
R Smart Pull Down Resistance Disabled –40 °C to +85 °C 750 kΩ
PD
REVERSE CURRENT BLOCKING
(RCB)
I RCB Activation Current Enabled, V > V –40 °C to +85 °C -500 mA
RCB OUT IN
t RCB Activation time Enabled, V > V + 200mV –40 °C to +85 °C 10 µs
RCB OUT IN
V RCB Release Voltage Enabled, V > V –40 °C to +85 °C 25 mV
RCB OUT IN
I VIN Reverse Leakage Current 0 V ≤ V + V ≤ V ≤ 5.5 V –40 °C to +85 °C –300 nA
IN,RCB IN RCB OUT
QUICK OUTPUT DISCHARGE
(QOD)
QOD(1) Output discharge resistance Disabled (Not in TPS22916CN/CNL) –40 °C to +85 °C 150 Ω
(1) For more information on which devices include quick output discharge, see the Device Functional Modes section.
Copyright © 2021 Texas Instruments Incorporated Submit Document Feedback 5
Product Folder Links: TPS22916


---

## Page 6


TPS22916
SLVSDO5F – JULY 2017 – REVISED DECEMBER 2021 www.ti.com
6.6 Switching Characteristics
Unless otherwise noted, the typical characteristics in the following table applies over the entire recommended power supply
voltage range of 1 V to 5.5 V at 25 °C with a load of C = 0.1µF, R = 10Ω.
L L
PARAMETER TEST CONDITIONS MIN TYP MAX UNIT
TPS22916B,
TPS22916BL
V = 5 V 115
IN
V = 3.6 V 140
IN
t Turn On Time V = 1.8 V 250 µs
ON IN
V = 1.2 V 350
IN
V = 1 V 510
IN
V = 5 V 70
IN
V = 3.6 V 80
IN
t Rise Time V = 1.8 V 130 µs
RISE IN
V = 1.2 V 190
IN
V = 1 V 240
IN
V = 5 V 57
IN
V = 3.6 V 36
IN
SR Slew Rate V = 1.8 V 12 mV/µs
ON IN
V = 1.2 V 5.1
IN
V = 1 V 3.3
IN
V = 5 V 5
IN
V = 3.6 V 5
IN
t Turn Off Time V = 1.8 V 10 µs
OFF IN
V = 1.2 V 15
IN
V = 1 V 25
IN
C = 0.1 µF, R = 10 Ω(1) 2.3
L L
t Fall Time µs
FALL C = 1µF, R = Open(1) 315
L L
6 Submit Document Feedback Copyright © 2021 Texas Instruments Incorporated
Product Folder Links: TPS22916


---

## Page 8


6.7 Typical Characteristics
6.7.1 Typical Electrical Characteristics
The typical characteristics curves in this section apply to all devices unless otherwise noted.
Temperature (°C)
):m(
NOR
220
1 V 3.6 V
200 1.2 V 5 V
1.8 V
180
160
140
120
100
80
60
40
-40 -20 0 20 40 60 80 100 120
D004
VIN (V)
Enabled
Figure 6-1. ON-Resistance vs Temperature
):m(
NOR
220
105qC
200 85qC
25qC
180 (cid:16)40qC
160
140
120
100
80
60
40
1 1.5 2 2.5 3 3.5 4 4.5 5 5.5
D003
Enabled
Figure 6-2. ON-Resistance vs Input voltage
VIN (V)
)An(
NIV,DSI
50
85qC
45 25qC
40
40qC
35
30
25
20
15
10
5
0
1 1.5 2 2.5 3 3.5 4 4.5 5 5.5
D002
VIN (V)
TPS22916C, TPS22916CN, TPS22916B V ≤ V
ON IL
Figure 6-3. Shutdown Current
)An(
NIV,DSI
250
200
150
100
50 85 °C
15 °C
(cid:16)40 °C
0
1 1.5 2 2.5 3 3.5 4 4.5 5 5.5
D015
TPS22916CL, TPS22916BL, V ≥ V
ON IH
TPS22916CNL
Figure 6-4. Shutdown Current (Active Low)
VIN (V)
)An(
NIV,QI
800
750
700
650
600
550
500
450
400 85qC
350 25qC
(cid:16)40qC
300
1 1.5 2 2.5 3 3.5 4 4.5 5 5.5
D001
VIN (V)
Enabled
Figure 6-5. Quiescent Current
)V(
NOV
TPS22916
SLVSDO5F – JULY 2017 – REVISED DECEMBER 2021 www.ti.com
0.72
VIH
0.7 VIL
0.68
0.66
0.64
0.62
0.6
0.58
0.56
0.54
1 1.5 2 2.5 3 3.5 4 4.5 5 5.5
D006
–40 °C to +85 °C
Figure 6-6. ON Pin Threshold
8 Submit Document Feedback Copyright © 2021 Texas Instruments Incorporated
Product Folder Links: TPS22916


---

## Page 9


6.7.1 Typical Electrical Characteristics (continued)
The typical characteristics curves in this section apply to all devices unless otherwise noted.
Temperature (qC)
):k(
DPR
1050
1000
950
900
850
800
750
700
650
-40 -20 0 20 40 60 80 100
D005
VIN (V)
V ≤ V
ON IL
Figure 6-7. ON Pin Smart Pulldown
):(
DOQ
TPS22916
www.ti.com SLVSDO5F – JULY 2017 – REVISED DECEMBER 2021
260
85qC
25qC
240 (cid:16)40qC
220
200
180
160
140
1 1.5 2 2.5 3 3.5 4 4.5 5 5.5
D007
TPS22916C, TPS22916CL, TPS22916B, TPS22916BL
Figure 6-8. Quick Output Discharge
Copyright © 2021 Texas Instruments Incorporated Submit Document Feedback 9
Product Folder Links: TPS22916


---

## Page 10


6.7.2 Typical Switching Characteristics
VIN (V)
)sP(
NOt
600
85 °C
550 25 °C
500 (cid:16)40 °C
450
400
350
300
250
200
150
100
1 1.5 2 2.5 3 3.5 4 4.5 5 5.5
D009
C L = 0.1 µF R L = 10 Ω TPS22916B V IN = 5 V C L = 0.1 μF R L = 10 Ω
Figure 6-9. Fast Turn-On Time Figure 6-10. Fast Turn-On at 5 V
VIN (V)
)sP(
ESIRt
275
85qC
250 25qC
(cid:16)40qC
225
200
175
150
125
100
75
50
1 1.5 2 2.5 3 3.5 4 4.5 5 5.5
D010
C L = 0.1 µF R L = 10 Ω TPS22916B V IN = 3.6 V C L = 0.1 μF R L = 10 Ω
Figure 6-11. Fast Rise Time Figure 6-12. Fast Turn-On at 3.6 V
VIN (V)
)sP/Vm(
NORS
TPS22916
SLVSDO5F – JULY 2017 – REVISED DECEMBER 2021 www.ti.com
80
85qC
25qC
(cid:16)40qC
60
40
20
0
1 1.5 2 2.5 3 3.5 4 4.5 5 5.5
D008
C L = 0.1 µF R L = 10 Ω TPS22916B V IN = 1 V C L = 0.1 μF R L = 10 Ω
Figure 6-13. Fast Slew Rate Figure 6-14. Fast Turn-On at 1 V
10 Submit Document Feedback Copyright © 2021 Texas Instruments Incorporated
Product Folder Links: TPS22916


---

## Page 20


TPS22916
SLVSDO5F – JULY 2017 – REVISED DECEMBER 2021 www.ti.com
11 Layout
11.1 Layout Guidelines
For best performance, all traces must be as short as possible. To be most effective, the input and output
capacitors must be placed close to the device to minimize the effects that parasitic trace inductances can have
on normal operation. Using wide traces for VIN, VOUT, and GND helps minimize the parasitic electrical effects.
11.2 Layout Example
Equation 3 shows an example for these devices. Notice the connection to system ground between the V
OUT
Bypass Capacitor ground and the GND pin of the load switch,. This connection creates a ground barrier which
helps to reduce the ground noise seen by the device.
To GPIO
control
GND
Gnd
Via ON
B2 B1 Gnd
V IN Bypass Via V OUT Bypass
Capacitor Capacitor
VIN A2 A1 VOUT
VIAto Power Ground Plane
Figure 11-1. TPS22916xx Layout
11.3 Thermal Considerations
The maximum IC junction temperature must be restricted to 125 °C under normal operating conditions. To
calculate the maximum allowable dissipation, P for a given output current and ambient temperature, use
D(max)
Equation 5 as a guideline:
T -T
P = J(MAX) A
D(MAX) R
θJA (5)
Where,
P = maximum allowable power dissipation
D(max)
T = maximum allowable junction temperature
J(max)
T = ambient temperature for the device
A
θ = junction to air thermal impedance. See the Thermal Information section.
JA
20 Submit Document Feedback Copyright © 2021 Texas Instruments Incorporated
Product Folder Links: TPS22916


---

## Page 21


TPS22916
www.ti.com SLVSDO5F – JULY 2017 – REVISED DECEMBER 2021
12 Device and Documentation Support
12.1 Documentation Support
12.1.1 Related Documentation
For related documentation see the following:
Texas Instruments, TPS22916 Load Switch Evaluation Module User's Guide
12.2 Receiving Notification of Documentation Updates
To receive notification of documentation updates, navigate to the device product folder on ti.com. Click on
Subscribe to updates to register and receive a weekly digest of any product information that has changed. For
change details, review the revision history included in any revised document.
12.3 Support Resources
TI E2E™ support forums are an engineer's go-to source for fast, verified answers and design help — straight
from the experts. Search existing answers or ask your own question to get the quick design help you need.
Linked content is provided "AS IS" by the respective contributors. They do not constitute TI specifications and do
not necessarily reflect TI's views; see TI's Terms of Use.
12.4 Trademarks
TI E2E™ is a trademark of Texas Instruments.
All trademarks are the property of their respective owners.
12.5 Electrostatic Discharge Caution
This integrated circuit can be damaged by ESD. Texas Instruments recommends that all integrated circuits be handled
with appropriate precautions. Failure to observe proper handling and installation procedures can cause damage.
ESD damage can range from subtle performance degradation to complete device failure. Precision integrated circuits may
be more susceptible to damage because very small parametric changes could cause the device not to meet its published
specifications.
12.6 Glossary
TI Glossary This glossary lists and explains terms, acronyms, and definitions.
13 Mechanical, Packaging, and Orderable Information
The following pages include mechanical, packaging, and orderable information. This information is the most
current data available for the designated devices. This data is subject to change without notice and revision of
this document. For browser-based versions of this data sheet, refer to the left-hand navigation.
Copyright © 2021 Texas Instruments Incorporated Submit Document Feedback 21
Product Folder Links: TPS22916


---

## Page 24


TPS22916
SLVSDO5F – JULY 2017 – REVISED DECEMBER 2021 www.ti.com
PACKAGE OUTLINE
YFP0004 DSBGA- 0.5 mm max height
SCALE 10.000
DIE SIZE BALLGRIDARRAY
B E A
BALLA1
CORNER
D
C
0.5 MAX
SEATING PLANE
0.19
0.13 BALLTYP 0.05 C
0.4
TYP
B
SYMM D: Max = 0.81 mm, Min = 0.75 mm
0.4
TYP E: Max = 0.81 mm, Min = 0.75 mm
A
0.25
4X
0.21 1 2
0.015 C A B
SYMM
4223507/A 01/2017
NOTES:
1.All linear dimensions are in millimeters.Any dimensions in parenthesis are for reference only. Dimensioning and tolerancing
perASMEY14.5M.
2.This drawing is subject to change without notice.
www.ti.com
24 Submit Document Feedback Copyright © 2021 Texas Instruments Incorporated
Product Folder Links: TPS22916


---

## Page 25


TPS22916
www.ti.com SLVSDO5F – JULY 2017 – REVISED DECEMBER 2021
EXAMPLE BOARD LAYOUT
YFP0004 DSBGA- 0.5 mm max height
DIE SIZE BALLGRIDARRAY
(0.4)TYP
4X ( 0.23)
1 2
A
SYMM
(0.4)TYP
B
SYMM
LAND PATTERN EXAMPLE
EXPOSED METALSHOWN
SCALE:50X
( 0.23) 0.05 MAX 0.05 MIN METALUNDER
METAL SOLDER MASK
SOLD O E P R E M N A IN S G K E M X E P T O AL SED EXP M O E S T E A D L S ( O 0 L . D 23 E ) R MASK
OPENING
NON-SOLDER MASK SOLDER MASK
DEFINED DEFINED
(PREFERRED)
SOLDER MASK DETAILS
NOTTO SCALE
4223507/A 01/2017
NOTES: (continued)
3. Final dimensions may vary due to manufacturing tolerance considerations and also routing constraints.
For more information, seeTexas Instruments literature number SNVA009 (www.ti.com/lit/snva009).
www.ti.com
Copyright © 2021 Texas Instruments Incorporated Submit Document Feedback 25
Product Folder Links: TPS22916


---

## Page 27


PACKAGE OPTION ADDENDUM
www.ti.com 9-Nov-2025
PACKAGING INFORMATION
Orderable part number Status Material type Package | Pins Package qty | Carrier RoHS Lead finish/ MSL rating/ Op temp (°C) Part marking
(1) (2) (3) Ball material Peak reflow (6)
(4) (5)
TPS22916BLYFPR Active Production DSBGA (YFP) | 4 3000 | LARGE T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 Q
TPS22916BLYFPR.A Active Production DSBGA (YFP) | 4 3000 | LARGE T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 Q
TPS22916BYFPR Active Production DSBGA (YFP) | 4 3000 | LARGE T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 (BA, R)
TPS22916BYFPR.A Active Production DSBGA (YFP) | 4 3000 | LARGE T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 (BA, R)
TPS22916BYFPT Active Production DSBGA (YFP) | 4 250 | SMALL T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 (BA, R)
TPS22916BYFPT.A Active Production DSBGA (YFP) | 4 250 | SMALL T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 (BA, R)
TPS22916CLYFPR Active Production DSBGA (YFP) | 4 3000 | LARGE T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 B9
TPS22916CLYFPR.A Active Production DSBGA (YFP) | 4 3000 | LARGE T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 B9
TPS22916CLYFPT Active Production DSBGA (YFP) | 4 250 | SMALL T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 B9
TPS22916CLYFPT.A Active Production DSBGA (YFP) | 4 250 | SMALL T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 B9
TPS22916CNLYFPR Active Production DSBGA (YFP) | 4 3000 | LARGE T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 S
TPS22916CNLYFPR.A Active Production DSBGA (YFP) | 4 3000 | LARGE T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 S
TPS22916CNYFPR Active Production DSBGA (YFP) | 4 3000 | LARGE T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 B8
TPS22916CNYFPR.A Active Production DSBGA (YFP) | 4 3000 | LARGE T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 B8
TPS22916CNYFPT Active Production DSBGA (YFP) | 4 250 | SMALL T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 B8
TPS22916CNYFPT.A Active Production DSBGA (YFP) | 4 250 | SMALL T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 B8
TPS22916CYFPR Active Production DSBGA (YFP) | 4 3000 | LARGE T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 B7
TPS22916CYFPR.A Active Production DSBGA (YFP) | 4 3000 | LARGE T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 B7
TPS22916CYFPT Active Production DSBGA (YFP) | 4 250 | SMALL T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 B7
TPS22916CYFPT.A Active Production DSBGA (YFP) | 4 250 | SMALL T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 B7
(1) Status: For more details on status, see our product life cycle.
(2) Material type: When designated, preproduction parts are prototypes/experimental devices, and are not yet approved or released for full production. Testing and final process, including without limitation quality assurance,
reliability performance testing, and/or process qualification, may not yet be complete, and this item is subject to further changes or possible discontinuation. If available for ordering, purchases will be subject to an additional
waiver at checkout, and are intended for early internal evaluation purposes only. These items are sold without warranties of any kind.
(3) RoHS values: Yes, No, RoHS Exempt. See the TI RoHS Statement for additional information and value definition.
Addendum-Page 1


---

## Page 28


PACKAGE OPTION ADDENDUM
www.ti.com 9-Nov-2025
(4) Lead finish/Ball material: Parts may have multiple material finish options. Finish options are separated by a vertical ruled line. Lead finish/Ball material values may wrap to two lines if the finish value exceeds the maximum
column width.
(5) MSL rating/Peak reflow: The moisture sensitivity level ratings and peak solder (reflow) temperatures. In the event that a part has multiple moisture sensitivity ratings, only the lowest level per JEDEC standards is shown.
Refer to the shipping label for the actual reflow temperature that will be used to mount the part to the printed circuit board.
(6) Part marking: There may be an additional marking, which relates to the logo, the lot trace code information, or the environmental category of the part.
Multiple part markings will be inside parentheses. Only one part marking contained in parentheses and separated by a "~" will appear on a part. If a line is indented then it is a continuation of the previous line and the two
combined represent the entire part marking for that device.
Important Information and Disclaimer:The information provided on this page represents TI's knowledge and belief as of the date that it is provided. TI bases its knowledge and belief on information provided by third parties, and
makes no representation or warranty as to the accuracy of such information. Efforts are underway to better integrate information from third parties. TI has taken and continues to take reasonable steps to provide representative
and accurate information but may not have conducted destructive testing or chemical analysis on incoming materials and chemicals. TI and TI suppliers consider certain information to be proprietary, and thus CAS numbers
and other limited information may not be available for release.
In no event shall TI's liability arising out of such information exceed the total purchase price of the TI part(s) at issue in this document sold by TI to Customer on an annual basis.
Addendum-Page 2
