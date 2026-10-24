# BQ25180 - Hardware Technical Specification

> **Extracted via:** DeepSheet + Docling Pipeline  
> **Source Document:** BQ25180.pdf  

---

## Page 1


BQ25180
SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023
BQ25180 I2C Controlled, 1-Cell, 1-A Linear Battery Charger with Power Path and Ship
Mode
1 Features 2 Applications
• 1-A Power path linear battery charger • TWS headset and charging case
– 3.0-V to 5.9-V input voltage operating range • Smart glasses, AR and VR
optimized for battery to battery charging and • Smart watches and other wearable devices
USB adapter • Retail automation and payment
– 25-V tolerant input voltage • Building automation
– Configurable battery regulation voltage with
3 Description
0.5% accuracy from 3.6 V to 4.65 V in 10-mV
steps The BQ25180 is a linear battery charger IC focusing
– 5-mA to 1-A configurable fast charge current on small solution size and low quiescent current for
– 55-mΩ battery FET ON resistance extending battery life. The device is available in an
– Up to 2.5-A discharge current to support high 8-ball chipscale package which does not need HDI
system loads PCB process for fabrication thereby reducing the PCB
– Configurable termination current down to 0.5 cost. The device can support up to 1-A charging and
mA system loads of up to 2.5 A.
– Configurable NTC charging profile thresholds
Device Information
including JEITA support
PART NUMBER PACKAGE(1) BODY SIZE (NOM)
– Power cycle and advanced reset mechanisms
to recover system BQ25180 DSBGA (8) 1.6 mm x 1.1 mm
• Power path management for powering the system
(1) For all available packages, see the orderable addendum at
and charging the battery
the end of the data sheet.
– Regulated system voltage (SYS) ranging from
4.4 V to 4.9 V in addition to battery voltage
tracking and input pass-though options
– Configurable input current limit VBUS IN SYS 10uF Re L g o u a la d ted
1uF
– Selectable adapter or battery power for system
– Dynamic power path management optimizes
charging from weak adapters /INT
Device
SCL Control
• Ultra low quiescent current modes SDA BAT
Host
1uF
– 15-nA Shutdown mode
– 3.2-μA Ship mode with button press wake VIO TS/MR +
–
– 3 μA in Battery Only mode NTC
BQ25180
– 30-μA input adapter Iq in Sleep mode
• One push-button wake-up and reset input
GND
• Integrated fault protection
– Input overvoltage protection (V )
IN_OVP
Simplified Schematic
– Battery undervoltage protection (V )
BUVLO
– Battery short protection (BATSC)
– Battery overcurrent protection (BATOCP)
– Input current limit protection (ILIM)
– Thermal regulation (TREG) and thermal
shutdown (TSHUT)
– Battery thermal fault protection (TS)
– Watchdog and safety timer fault
– System short protection
– System overvoltage protection
An IMPORTANT NOTICE at the end of this data sheet addresses availability, warranty, changes, use in safety-critical applications,
intellectual property matters and other important disclaimers. PRODUCTION DATA.


---

## Page 2


BQ25180
SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023 www.ti.com
Table of Contents
1 Features............................................................................1 8.4 Device Functional Modes..........................................25
2 Applications.....................................................................1 8.5 Register Maps...........................................................26
3 Description.......................................................................1 9 Application and Implementation..................................41
4 Revision History..............................................................2 9.1 Application Information.............................................41
5 Description (continued)..................................................3 9.2 Typical Application....................................................41
6 Pin Configuration and Functions...................................4 10 Power Supply Recommendations..............................48
7 Specifications..................................................................5 11 Layout...........................................................................49
7.1 Absolute Maximum Ratings........................................5 11.1 Layout Guidelines...................................................49
7.2 ESD Ratings...............................................................5 11.2 Layout Example......................................................49
7.3 Thermal Information....................................................5 12 Device and Documentation Support..........................50
7.4 Recommended Operating Conditions.........................5 12.1 Device Support.......................................................50
7.5 Electrical Characteristics.............................................6 12.2 Receiving Notification of Documentation Updates..50
7.6 Timing Requirements................................................10 12.3 Support Resources.................................................50
7.7 Typical Characteristics..............................................11 12.4 Trademarks.............................................................50
8 Detailed Description......................................................12 12.5 Electrostatic Discharge Caution..............................50
8.1 Overview...................................................................12 12.6 Glossary..................................................................50
8.2 Functional Block Diagram.........................................16 13 Mechanical, Packaging, and Orderable
8.3 Feature Description...................................................16 Information....................................................................51
4 Revision History
NOTE: Page numbers for previous revisions may differ from page numbers in the current version.
Changes from Revision B (January 2022) to Revision C (January 2023) Page
• Removed figure from Section 8.3.1 .................................................................................................................16
Changes from Revision A (December 2021) to Revision B (January 2022) Page
• Removed operating ambient...............................................................................................................................5
• Removed OTP_VLOWV.....................................................................................................................................6
• Removed Standalone.........................................................................................................................................6
• Removed /PG Isink.............................................................................................................................................6
• Removed t and F ....................................................................................................................10
HW_RESET I2C_CLK
• Updated conditions for Typical Characteristics ................................................................................................11
• Added legend for Figure 7-1 ............................................................................................................................11
• Changed Figure 8-4 and Figure 8-5 ................................................................................................................21
• Changed MR input to Pushbutton input in Table 8-6 .......................................................................................25
• Updated Section 8.5 Register Maps reset values.............................................................................................26
• Changed Device_ID description in the Table 8-21 ...........................................................................................26
Changes from Revision * (September 2021) to Revision A (December 2021) Page
• Changed from Advance Information to Production Data....................................................................................1
2 Submit Document Feedback Copyright © 2023 Texas Instruments Incorporated
Product Folder Links: BQ25180


---

## Page 3


BQ25180
www.ti.com SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023
5 Description (continued)
The battery is charged using a standard Li-ion or LiFePO4 charge profile with three phases: precharge, constant
current and constant voltage. Thermal regulation provides the maximum charge current while managing the
device temperature. The charger is also optimized for battery to battery charging with 3-V minimum input voltage
operation and can withstand 25-V absolute maximum line transients. The device integrates a single push-button
input and reset circuitry to reduce the total solution footprint.
Copyright © 2023 Texas Instruments Incorporated Submit Document Feedback 3
Product Folder Links: BQ25180


---

## Page 4


BQ25180
SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023 www.ti.com
6 Pin Configuration and Functions
1 2
A /INT IN
B SCL SYS
C
SDA BAT
D TS/MR GND
Figure 6-1. YBG Package 8-Pin DSBGA (Top View)
Table 6-1. Pin Functions
PIN
I/O(1) DESCRIPTION
NAME NO.
IN A2 P DC Input Power Supply. IN is connected to the external DC supply. Bypass IN to GND with at
least 1 μF of capacitance using a ceramic capacitor.
SYS B2 P Regulated System Output. Connect at least 10-μF ceramic capacitor (at least >1 μF of ceramic
capacitance with DC bias derating) from SYS to GND as close to the SYS and GND pins as
possible.
BAT C2 P Battery Connection. Connect to the positive terminal of the battery. Bypass BAT to GND with at
least 1 μF of ceramic capacitance.
GND D2 - Ground connection. Connect to the ground plane of the circuit.
SCL B1 I/O I2C Interface Clock. Connect SCL to the logic rail through a 10-kΩ pullup resistor.
SDA C1 I/O I2C Interface Data. Connect SDA to the logic rail through a 10-kΩ pullup resistor.
/INT A1 O INT is an open-drain output that signals fault interrupts. When a fault occurs, a 128-μs active low
pulse is sent out as an interrupt for the host. INT is enabled/disabled using the MASK_INT bit in
the control register. Can be pulled up to the logic rail through a 1-kΩ to 20-kΩ resistor.
TS/MR D1 I/O Manual Reset Input/ NTC thermistor pin. TS/MR is a general purpose input that must be held
low for greater than t to go into Ship mode or perform a hardware reset. It can also be
LPRESS
used to detect shorter button press durations such as t and t TSMR may be driven
WAKE1 WAKE2
by a momentary push-button or a MOS switch. The TSMR pin can also have an NTC thermistor
connected on to it.
(1) I = Input, O = Output, I/O = Input or Output, G = Ground, P = Power.
4 Submit Document Feedback Copyright © 2023 Texas Instruments Incorporated
Product Folder Links: BQ25180


---

## Page 5


BQ25180
www.ti.com SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023
7 Specifications
7.1 Absolute Maximum Ratings
over operating free-air temperature range (unless otherwise noted)(1)
MIN MAX UNIT
Input Voltage IN -0.3 25 V
Voltage All other pins -0.3 5.5 V
Input Current (DC) IN 1.1 A
SYS Discharge Current(DC) SYS 1.5 A
SYS Discharge Current (tpulse
SYS 2.5 A
<20ms)
Output Sink Current /INT 20 mA
T Junction temperature -40 150 °C
J
T Storage temperature -65 150 °C
stg
(1) Stresses beyond those listed under Absolute Maximum Rating may cause permanent damage to the device. These are stress
ratings only, which do not imply functional operation of the device at these or any other conditions beyond those indicated
under Recommended Operating Condition. Exposure to absolute-maximum-rated conditions for extended periods may affect device
reliability.
7.2 ESD Ratings
VALUE UNIT
Human body model (HBM), per ANSI/ESDA/
±2500
JEDEC JS-001, all pins(1)
V Electrostatic discharge V
(ESD)
Charged device model (CDM), per ANSI/ESDA/
±1500
JEDEC JS-002, all pins(2)
(1) JEDEC document JEP155 states that 500-V HBM allows safe manufacturing with a standard ESD control process.
(2) JEDEC document JEP157 states that 250-V CDM allows safe manufacturing with a standard ESD control process.
7.3 Thermal Information
BQ25180
THERMAL METRIC YBG (DSBGA) UNIT
8 PIN
R Junction-to-ambient thermal resistance (EVM(2)) 65 °C/W
θJA
R Junction-to-ambient thermal resistance (JEDEC(1)) 107.1 °C/W
θJA
R Junction-to-case (top) thermal resistance 0.9 °C/W
θJC(top)
R Junction-to-board thermal resistance 30.3 °C/W
θJB
Ψ Junction-to-top characterization parameter 0.3 °C/W
JT
Ψ Junction-to-board characterization parameter 30.3 °C/W
JB
R Junction-to-case (bottom) thermal resistance N/A °C/W
θJC(bot)
(1) For more information about traditional and new thermal metrics, see the Semiconductor and IC Package Thermal Metrics application
report.
(2) 1oz Copper, 2-layer board
7.4 Recommended Operating Conditions
over operating free-air temperature range (unless otherwise noted)
MIN NOM MAX UNIT
VBAT Battery Voltage Range 2.2 4.6 V
VIN Input Voltage Range 2.7 5.5 V
Copyright © 2023 Texas Instruments Incorporated Submit Document Feedback 5
Product Folder Links: BQ25180


---

## Page 6


BQ25180
SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023 www.ti.com
7.4 Recommended Operating Conditions (continued)
over operating free-air temperature range (unless otherwise noted)
MIN NOM MAX UNIT
IIN Input Current Range (IN to SYS) 1.1 A
IBAT Battery Discharge Current (BAT to SYS) 1.5 A
TJ Operating Junction Temperature Range -40 125 °C
7.5 Electrical Characteristics
VIN = 5V, VBAT = 3.6V. -40 °C < TJ < 125 °C unless otherwise noted. Typical data at TJ = 25 °C
PARAMETER TEST CONDITIONS MIN TYP MAX UNIT
INPUT CURRENTS
VBAT = 3.6V, VIN = 5V, Charge enabled,
I Input supply quiescent current 0.75 1 mA
Q_IN ICHG = 0mA, SYSREG = 4.5V
VBAT = 3.6V, VIN = 5V, Charge enabled,
I Input supply quiescent current 0.660 0.850 mA
Q_IN ICHG = 0mA, SYSREG = Passthrough
I SLEEP input current VIN = 3.6V, VBAT = 3.7V 30 µA
SLEEP_IN
V <V or floating, Watchdog
IN UVLO
I Battery quiescent current disabled, Push button disabled, I2C 3 3.5 µA
Q_BAT
functional. VBAT =3.6V T = 25 °C
J
V <V , VBAT =3.6V, Push-button
I Battery quiescent current IN UVLO 4 5 µA
Q_BAT function enabled, 0 °C < T < 85 °C
J
I VIN = 0V, Ship Mode, VBAT = 3.6V,
BAT_SHUT Battery discharge current in Ship Mode 15 nA
Adapter Sense wake enabled.
DOWN
VBAT = 3.6V, Push button function
I Battery discharge current in Ship Mode enabled (average current), 0 °C < T < 3.2 4.5 µA
BAT_SHIP J
85 °C
POWER-PATH MANAGEMENT AND INPUT
V Input voltage operating range 3 5.5 V
IN_OP
V
IN_UVLO Exit IN undervoltage lock-out IN rising 3 V
Z
V Enter IN undervoltage lock-out IN falling 2.7 V
IN_UVLO
V IN voltage to start charging IN rising 3 3.15 V
IN_LOWV
V
IN_LOWV IN voltage to stop charging IN falling 2.95 3.1 V
Z
V IN voltage threshold to enter shipmode IN falling 1.09 1.3 1.66 V
IN_PORZ
V Exit sleep mode threshold IN rising, VIN - VBAT, VBAT= 4V 100 135 185 mV
SLEEPZ
V Sleep mode threshold hysteresis IN falling, VIN - VBAT, VBAT= 4V 72 mV
SLEEP
V VIN overvoltage rising threshold IN rising 5.5 5.7 5.9 V
IN_OVP
V
IN_OV_H IN overvoltage hysteresis IN falling 125 mV
YS
VBAT = 3.6V, IBAT_OCP= 00 0.5 A
VBAT = 3.6V, IBAT_OCP= 01 1 A
I BATOCP(Reverse OCP only)
BAT_OCP
VBAT = 3.6V, IBAT_OCP= 10 1.5 A
VBAT = 3.6V, IBAT_OCP= 11 Disabled A
VBAT = 3.6V, VBAT > V , VSYS<
VBSUP1 Enter supplement mode threshold BUVLO 40 mV
VBAT-VBSUP1
VBSUP2 Exit supplement mode threshold V > V , VSYS>VBAT-VBSUP2 20 mV
BAT BUVLO
6 Submit Document Feedback Copyright © 2023 Texas Instruments Incorporated
Product Folder Links: BQ25180


---

## Page 7


BQ25180
www.ti.com SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023
7.5 Electrical Characteristics (continued)
VIN = 5V, VBAT = 3.6V. -40 °C < TJ < 125 °C unless otherwise noted. Typical data at TJ = 25 °C
PARAMETER TEST CONDITIONS MIN TYP MAX UNIT
VIN = 5V, ILIM =50mA 40 50 60 mA
VIN = 5V, ILIM =100mA 80 90 98 mA
VIN = 5V, ILIM= 200mA 180 200 220 mA
VIN = 5V, ILIM= 300mA 270 300 330 mA
ILIM Input Current Limit
VIN = 5V, ILIM= 380mA 360 380 400 mA
VIN = 5V, ILIM= 500mA 450 475 498 mA
VIN = 5V, ILIM =665mA 630 665 700 mA
VIN = 5V, ILIM= 1050mA 995 1050 1100 mA
V
INDPM_A VINDPM accuracy VINDPM target is not disabled -3 3 %
CC
Input voltage threshold when input current
VINDPM target =4.2V 4.2 V
is reduced
Input voltage threshold when input current
V VINDPM target =4.5V 4.5 V
INDPM is reduced
Input voltage threshold when input current
VINDPM target =4.7V 4.7 V
is reduced
SYS voltage threshold when charge VBAT = 3.6V, VSYS = V + VBAT
V DPPM 0.1 V
DPPM current is reduced before charge current is reduced.
V
SYS_REG Programmable SYS voltage regulation VIN = 5V, VBAT = 3.6V, RSYS = 100ohm,
-2 2 %
_ACCURAC accuracy SYS regulation target = 4.4V to 4.9V
Y
Minimum SYS voltage when in battery
V VBAT < 3.6V 3.8 V
MINSYS tracking mode
Voltage regulation threshold for SYS
V
SYS_TRA when VBAT >3.6V in battery tracking VBAT = 4V, VSYS = VBAT + V 225 mV
SYS_TRACK
CK mode
R SYS pull down resistance V = 3.6V 25 Ω
SYS_PD SYS
BATTERY CHARGER
R Battery FET on-resistance VBAT = 4.5V, IBAT =500mA 55 90 mΩ
ON_BAT
R Input FET on-resistance IN = 5V, IIN = 1A 270 470 mΩ
ON_IN
V Typical BAT charge voltage regulation
REG_RA 10mV steps, programmabe through I2C 3.5 4.65 V
range
NGE
V BAT charge voltage accuracy, summary All VBATREG settings, typical
REG_AC –0.5 0.5 %
for all settings measurement at VBATREG = 4.2V
C
I
CHG_RAN Typical charge current regulation range V > V 5 1000 mA
OUT LOWV
GE
I Charge current accuracy VIN = 5V, Fastcharge >=40mA –10 10 %
CHG_ACC
I Charge current accuracy Fastcharge current = 40mA 36 40 44 mA
CHG_ACC
I Charge current accuracy Fastcharge current = 630mA 567 630 693 mA
CHG_ACC
Typical pre-charge current, as percentage
I V < V 20 %
PRECHG of ICHG OUT LOWV
I
PRECHG_ Precharge current accuracy Fastcharge current >=40mA –10 10 %
ACC
Typical termination current, as percentage % of
I V = VBATREG 10
TERM of ICHG OUT Icharge
I
TERM_AC Termination current accuracy IBAT = 3mA (IFCHG = 30mA) Tj = 25 °C –10 10 %
C
I
TERM_AC Termination current accuracy IBAT = 3mA (IFCHG = 30mA) Tj = 25 °C 2.7 3.3 mA
C
Copyright © 2023 Texas Instruments Incorporated Submit Document Feedback 7
Product Folder Links: BQ25180


---

## Page 8


BQ25180
SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023 www.ti.com
7.5 Electrical Characteristics (continued)
VIN = 5V, VBAT = 3.6V. -40 °C < TJ < 125 °C unless otherwise noted. Typical data at TJ = 25 °C
PARAMETER TEST CONDITIONS MIN TYP MAX UNIT
Pre-charge to fast-charge transition
V VLOWVSEL = 3.0V, VBAT rising 2.9 3 3.1 V
LOWV threshold
Pre-charge to fast-charge transition
V VLOWVSEL = 2.8V, VBAT rising 2.7 2.8 2.9 V
LOWV threshold
V
LOWV_H Battery LOWV hysteresis All settings 100 mV
YS
Battery UVLO, VBAT falling BUVLO setting = b000 3 V
Battery UVLO, VBAT falling BUVLO setting = b011 2.8 V
Battery UVLO, VBAT falling BUVLO setting = b100 2.6 V
V
BUVLO
Battery UVLO, VBAT falling BUVLO setting = b101 2.4 V
Battery UVLO, VBAT falling BUVLO setting = b110 2.2 V
Battery UVLO, VBAT falling BUVLO setting = b111 2.0 V
V Any BUVLO Setting, value above VBAT,
BUVLO_H Battery UVLO hysteresis, VBAT rising 110 150 190 mV
VIN = 5V
YS
Battery only power up voltage, VBAT
V -40C < Tj < 125C 3.08 3.21 3.46 V
BATPOR rising
BAT falling, VRCH bit = 0 75 100 130 mV
V
RCH
BAT falling, VRCH bit = 1 175 200 230 mV
Short on battery threshold for trickle
V 1.6 1.8 2.0 V
BATSC charge, VBAT rising
V
BATSC_H Battery short circuit voltage hysteresis 200 mV
YS
I Trickle Charge Current VBAT<V 8 mA
BATSC BATSC
TERMPERATURE REGULATION AND TEMPERATURE SHUTDOWN
T Typical junction temperature regulation THERM_REG = 00 100 °C
REG
T Typical junction temperature regulation THERM_REG = 11 Disabled
REG
T
SHUT_RI Thermal shutdown rising threshold Temperature increasing 150 °C
SING
T
SHUT_FA Thermal shutdown falling threshold Temperature decreasing 135 °C
LLING
BATTERY NTC MONITOR
I TS nominal bias current 36.5 38 39.5 µA
TS_BIAS
V Cold - 00 @ ≈. 0 °C, default VIN = 5V 0.9575 1.0075 1.0575 V
T1_Entry
V Cold - 01 @ ≈. 3 °C VIN = 5V 0.8450 0.8900 0.9325 V
T2_Entry
V Cold - 10 @ ≈. 5 °C VIN = 5V 0.7775 0.8200 0.8600 V
T3_Entry
V Cold - 11 @ ≈. -3 °C VIN = 5V 1.0850 1.1425 1.2000 V
T4_Entry
V Cool - 00 @ ≈. 10 °C, default VIN = 5V 0.6350 0.6700 0.7025 V
T5_Entry
V Warm - 00 @ ≈. 45 °C, default VIN = 5V 0.1730 0.1850 0.198 V
T6_Entry
V Hot - 00 @ ≈. 60 °C, default VIN = 5V 0.1050 0.1150 0.1250 V
T7_Entry
V Hot - 01 @ ≈. 65 °C VIN = 5V 0.0875 0.0975 0.1075 V
T8_Entry
V Hot - 10 @ ≈. 50 °C VIN = 5V 0.1475 0.1575 0.1675 V
T9_Entry
V Hot - 11 @ ≈. 45 °C VIN = 5V 0.1750 0.1850 0.1950 V
T10_Entry
V Cold - 00 @ ≈. 5 °C, default VIN = 5V 0.7775 0.8200 0.8600 V
T1_Exit
V Cold - 01 @ ≈. 8 °C VIN = 5V 0.6875 0.7250 0.7600 V
T2_Exit
V Cold - 10 @ ≈. 10 °C VIN = 5V 0.6350 0.6700 0.7025 V
T3_Exit
V Cold - 11 @ ≈. 2 °C VIN = 5V 0.8800 0.9275 0.9725 V
T4_Exit
V Cool - 00 @ ≈. 15 °C, default VIN = 5V 0.5225 0.5500 0.5775 V
T5_Exit
8 Submit Document Feedback Copyright © 2023 Texas Instruments Incorporated
Product Folder Links: BQ25180


---

## Page 9


BQ25180
www.ti.com SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023
7.5 Electrical Characteristics (continued)
VIN = 5V, VBAT = 3.6V. -40 °C < TJ < 125 °C unless otherwise noted. Typical data at TJ = 25 °C
PARAMETER TEST CONDITIONS MIN TYP MAX UNIT
V Warm - 00 @ ≈. 41 °C, default VIN = 5V 0.2080 0.2200 0.235 V
T6_Exit
V Hot - 00 @ ≈. 55 °C, default VIN = 5V 0.1250 0.1350 0.1450 V
T7_Exit
V Hot - 01 @ ≈. 60 °C VIN = 5V 0.1050 0.1150 0.1250 V
T8_Exit
V Hot - 10 @ ≈. 45 °C VIN = 5V 0.1750 0.1850 0.1950 V
T9_Exit
V Hot - 11 @ ≈. 40 °C VIN = 5V 0.2100 0.2200 0.23 V
T10_Exit
TS monitoring enable threshold
V VTSMR<VTS_ENZ for TS function to be TS Rising, VIN = 5V 1.8 2.1 2.8 V
TS_ENZ
enabled
V
TS_CLAM TS maximum voltage clamp TS open-circuit (float), VIN = 5V 2.2 2.8 3.3 V
P
PUSH BUTTON TIMERS AND THRESHOLDS
I Adapter present 36.5 38 39.5 µA
TSMR
I Battery only mode 60 µA
TSMR
TSMR voltage to detect a button press
V 90 mV
TSMR event, battery only mode
TSMR voltage to detect a button press
V 90 mV
TSMR event, adapter present
WAKE1 Timer. Time from TSMR low MR_WAKE1_TIMER = 0 300 ms
t
WAKE1 detection MR_WAKE1_TIMER = 1 1 s
WAKE2 Timer. Time from TSMR low MR_WAKE2_TIMER = 0 2 s
t
WAKE2 detection MR_WAKE2_TIMER = 1 3 s
t RESET_WARN Timer. Time prior to HW
RESET_W MR_RESET_WARN = 0 0.9 1 1.1 s
RESET
ARN
MR_LPRESS = 00 4.5 5 5.5 s
Long Press timer. Time from button press MR_LPRESS = 01 9 10 11 s
t
LPRESS detection to long press action. MR_LPRESS = 10 13.5 15 16.5 s
MR_LPRESS = 11 18 20 22 s
AUTOWAKE = 00 0.5 s
t RESTART( RESTART Timer. Time from HW Reset to AUTOWAKE = 01 1 s
AUTOWAKE SYS power up AUTOWAKE = 10 2 s
)
AUTOWAKE = 11 4 s
BATTERY CHARGING TIMERS
t Charge safety timer Programmable range 180 720 min
MAXCHG
t Precharge safety timer 0.25 * t
PRECHG MAXCHG
I2C INTERFACE
V Input low threshold level VPULLUP = 1.8V, SDA and SCL 0.4 V
IL
V Input high threshold level VPULLUP = 1.8V, SDA and SCL 1.3 V
IH
V Output low threshold level IL = 5mA, sink current, V =1.8V 0.4 V
OL PULLUP
I High-Level leakage current V = 1.8V 1 µA
LKG PULLUP
LOGIC PINS
IL = 5mA, sink current, V
V Output low threshold level PULLUP 0.4 V
OL =3.3V, /INT pin
I High-Level leakage current V = 3.3V, /INT pin 1 µA
LKG PULLUP
Copyright © 2023 Texas Instruments Incorporated Submit Document Feedback 9
Product Folder Links: BQ25180


---

## Page 10


BQ25180
SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023 www.ti.com
7.6 Timing Requirements
MIN NOM MAX UNIT
INPUT
t VIN_OVP deglitch, VIN falling 30 ms
VIN_OVPZ_DGL
t Deglitch time to enter SLEEP, VIN falling 64 µs
SLEEP_DGL
BATTERY CHARGER
t Recovery time, BATOCP during Discharge Mode 250 ms
REC_SC
Retry window for SYS or BAT short circuit
t 2 s
RETRY_SC recovery(BATOCP)
Deglitch time to disconnect the BATFET when VBAT <
t 60 µs
BUVLO V setting
BUVLO
t TS turnon-time (battery only mode) 4 ms
TS_DUTY_ON
t TS turnoff time (battery only mode) 196 ms
TS_DUTY_OFF
DIGITAL CLOCK, WATCHDOG and PUSHBUTTON
t I2C interface reset timer, adjustable 40 160 Disabled s
WDOG
t I2C interface inactive reset timer 500 ms
I2CRESET
t Wake timer to count for shipmode (WAKE2 DefaultTimer) 2 s
SHIPWAKE
10 Submit Document Feedback Copyright © 2023 Texas Instruments Incorporated
Product Folder Links: BQ25180


---

## Page 14


BQ25180
SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023 www.ti.com
8.1.1.2 Precharge
When battery voltage is above the V but lower than V threshold, the battery is charged with the
BATSC LOWV
precharge current level. The precharge current (IPRECHARGE) can be programmed through I2C and can be
adjusted by the host. Once the battery voltage reaches V , the charger will then operate in Fast Charge
LOWV
mode, charging the battery at ICHG.
During precharge, the safety timer is set to 25% of the safety timer value during fast charge. In the case where
termination is disabled, precharge current is set to 20% of fast charge current setting.
8.1.1.3 Fast Charge
The charger has two main control loops that control charging when V > V : the Constant Current (CC)
BAT LOWV
and Constant Voltage (CV) loops. When the CC loop is dominant, the battery is charged at the maximum charge
current level I , unless there is a TS fault condition (JEITA operation), VINDPM is active, thermal regulation
CHG
or DPPM is active. (See respective sections for details on these modes of operation). Once the battery voltage
approaches the battery regulation target, the CV loops becomes more dominant and the charging current starts
tapering off. Once the charging current reaches the termination current (I ) the charge is done, Charge_done
TERM
status is set. If the I2C setting of VBATREG is set higher than 4.65 V, the battery regulation voltage is still
maintained at 4.65 V. The device will switch to fastcharge mode based on VLOWV setting on the register map.
8.1.1.4 Termination
The device will automatically terminate charging once the charge current reaches ITERM, which is
programmable through I2C. After termination the charger will operate in high impedance mode, disabling the
BATFET to disconnect the battery. Power is provided to the system (SYS) by IN supply as long as V > V ,
IN UVLO
V > V + V and V < V .
IN BAT SLEEPZ IN IN_OVP
Termination is only enabled when the charger CV loop is active in fast charge operation. Termination is disabled
if the charge current reaches I while the VINDPM, DPPM, or thermal regulation loops are active. The
TERM
charger will only go into the termination when the current drops to I due to the battery reaching the target
TERM
voltage and not due to the charge current limitation imposed by the previously mentioned controlled loops.
Post termination, the battery FET is disabled and the voltage on BAT pin is monitored to check if it has dropped
to the VRCH threshold. If it does, a new charge cycle is established. The safety timers are reset. During charging
or even when charge is done, a higher SYS load will be supported through the supplement operation.
14 Submit Document Feedback Copyright © 2023 Texas Instruments Incorporated
Product Folder Links: BQ25180


---

## Page 15


BQ25180
www.ti.com SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023
Regulation Voltage
VSET
VRCH
Battery Voltage
Charge Current
ISET
Charge Current
VLOWV
VBATSC
IPRECHG
ITERM
IBATSC
Trickle Charge Pre-charge Fast-Charge Taper-Charge Charge Re-
CC CV Done charge
Precharge Timer Safety Timer
Figure 8-2. Typical Charging Profile of a Battery
Copyright © 2023 Texas Instruments Incorporated Submit Document Feedback 15
Product Folder Links: BQ25180


---

## Page 19


BQ25180
www.ti.com SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023
MASK_ILIM will prevent an interrupt from being issued but does not override the ILIM behavior itself. The ILIM
value can be programmed dynamically through the I2C by the host. The ILIM settings of 100mA and 500mA are
designed to be the maximum value to support standard systems.
8.3.7 Protection Mechanisms
8.3.7.1 Input Overvoltage Protection
Input overvoltage protection protects the device and downstream components connected to SYS, and BAT
against damage from overvoltage on the input supply. When VIN > V , a VIN overvoltage condition is
IN_OVP
determined to exist. During the VIN overvoltage condition, the device turns the input FET OFF, battery discharge
FET ON, sends a single 128-μs pulse on INT, and the fault bit (VIN_OVP_FAULT_FLAG) is updated over I2C.
The VIN_PGOOD_STAT bit also is affected by the VIN overvoltage condition as the VIN powergood condition
will fail. Once the VIN overvoltage condition is removed (V ≤ V - V ), the VIN_OVP_STAT bit is
IN IN_OVP IN_OV_HYS
cleared and the device returns to normal operation. Thereafter, a VIN powergood condition is determined if VIN
> VBAT + V and VIN > V .
SLEEPZ IN_UVLO
8.3.7.2 Battery Undervoltage Lockout
In order to prevent deep discharge of the battery the device integrates a battery undervoltage lockout feature
which will disengage the BAT to SYS path when voltage at the battery drops below the programmed BUVLO
setting present in the CHARGERCTRL1 register. BUVLO status can also be read when a valid voltage on VIN is
present.
8.3.7.3 System Overvoltage Protection
The system overvoltage protection is to prevent SYS from overshooting to a high voltage due to the input supply.
SYS_OVP will momentarily disconnect the blocking FETs and re-engage when the thresholds have dropped to
less than the SYS_OVP_FALLING threshold.
The SYS_OVP_RISING threshold is typically 105% of the target SYS voltage and the SYS_OVP_FALLING
threshold is 102.5% of the target SYS voltage.
8.3.7.4 System Short Protection
When a valid adapter is connected to the device, the device turns ON the input blocking FET for 5 ms and it
detects the SYS pin to be shorted (voltage on SYS <1.6V). In this scenario, the device will turn OFF the input
FET for ~200 μs and turn it back ON for 5 ms for SYS to rise above 1.6V. If after 10 tries, the SYS short still
persists, the device will turn OFF SYS until adapter is connected again.
8.3.7.5 Battery Overcurrent Protection
In order to protect the device from overcurrent and prevent excessive battery discharge current, the device
detects if the current on the battery FET exceeds IBAT_OCP. If the BATOCP limit is reached, the battery
discharge FET is turned off and the device starts operating in hiccup mode, re-enabling the BATFET t
REC_SC
(250 ms) after being turned OFF by the overcurrent condition. If the overcurrent condition is triggered upon retry
for 4 to 7 consecutive times in a 2-s window, the BATFET shall then remain off until a valid VIN is connected
(VIN = VIN_POWERGOOD). If the overcurrent condition and hiccup operation occur while in supplement mode
where VIN is already present, VIN must be toggled in order for the BATFET to be enabled and start another
detection cycle.
8.3.7.6 Safety Timer and Watchdog Timer
At the beginning of each charge cycle mode (Precharge or Fast Charge), the device starts the respective mode
safety timer. If charging has not terminated before the programmed safety time, t expires or the device
MAXCHG
does not exit the precharge mode before t expires, charging is disabled. The precharge safety time,
PRECHG
t , is 25% of t . When a safety timer fault occurs, a single 128-μs pulse is sent on the INT pin and
PRECHG MAXCHG
the STAT and FAULT bits of the status registers are updated over I2C.
The charge enable bit or input power must be toggled in order to clear the safety timer fault.
Copyright © 2023 Texas Instruments Incorporated Submit Document Feedback 19
Product Folder Links: BQ25180


---

## Page 20


BQ25180
SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023 www.ti.com
If the safety timer has expired, the device will produce an interrupt and update the SAFETY_TMR_FAULT_FLAG
bit on the register map. The safety timer duration is programmable using the SAFETY_TIMER_1:0 bits. When
the safety timer is active, changing the safety timer duration resets the safety timer. The device also contains
a 2XTMR_EN bit that doubles the safety timer duration to prevent premature safety timer expiration when the
charge current is reduced by a high load on SYS (DPM operation- causing VDPPM to be enabled), VINDPM,
thermal regulation, or a NTC (JEITA) condition. When the 2XTMR_EN bit is set, the timer is allowed to run
at half speed when any loop is active other than CC or CV. In the event where during CC mode the battery
voltage drops to push the charger into precharge mode, (due to a large load on battery, thermal events, and so
forth) the safety timer will reset counting through precharge and then resetting the fast charge safety timer. If the
device entered battery supplement mode while in precharge, CC or CV mode, while the charger is not disabled,
the device will suspend the safety timer until charging can resume again. This prevents the safety timer from
resetting when a supplement condition is caused.
In addition to the safety timer, the device contains a watchdog timer that monitors the host through the I2C
interface. The watchdog timer is enabled by default and may be disabled by the host through an I2C transaction.
Once the initial transaction is received, the watchdog timer is started. The watchdog timer is reset by any
transaction by the host using the I2C interface. If the watchdog timer expires without a reset from the I2C
interface, all charger parameters registers (ICHG, IPRECHARGE, ITERM,VLOWV, and so forth) are reset to
the default values. The watchdog timer can be set through the WATCHDOG_SEL_1:0 bits either in battery only
mode or when an adapter is present.
Table 8-4. Watchdog Settings
WATCHDOG_SEL_1:0 ACTION
00 Device will only perform a software reset after 160s of the last I2C transaction
01 Device will issue a HW_Reset after 160s of last I2C transation
10 Device will issue a HW_Reset after 40s of the last I2C transaction
11 Watchdog functionality is completely disabled
8.3.7.7 Thermal Protection and Thermal Regulation
During operation, to protect the device from damage due to overheating, the junction temperature of the die,
T , is monitored. When T reaches T , the device stops charging operation and VSYS is shutdown.
J J SHUT_RISING
If in the case where T > T prior to power being applied to the device (either battery or adapter),
J SHUT_RISING
the input FET or BATFET will not turn ON, regardless of the TSMR pin. Thereafter if temperature falls below
T , the device will automatically power up if VIN is present or if in battery only mode.
SHUT_FALLING
During the charging process, to prevent overheating in the device, the device monitors the junction temperature
of the die and reduces the charging current once T reaches the thermal regulation threshold (T ) based
J REG
on bits set by the THERM_REG setting. If the charge current is reduced to 0, the battery supplies the current
needed to supply the SYS output. Thermal regulation can be disabled through I2C.
Ensure that system power dissipation is under the limit of the device. The power dissipated by the device can be
calculated using the following equation:
P = P + P
DISS SYS BAT
Where:
P = (V – V ) * I
SYS IN SYS IN
P = (V – V ) * I
BAT SYS BAT BAT
The die junction temperature, T , can be estimated based on the expected board performance using the
J
following equation:
T = T + θ * P
J A JA DISS
θ is largely driven by board layout. For more information about traditional and new thermal metrics, see the IC
JA
Package Thermal Metrics Application Report. Under typical conditions, the time spent in this state is very short.
20 Submit Document Feedback Copyright © 2023 Texas Instruments Incorporated
Product Folder Links: BQ25180


---

## Page 21


8.3.8 Pushbutton Wake and Reset Input
The pushbutton function implemented through the TSMR pin has three main functions. First, it serves as a
means to wake the device from ultra-low power modes like ship mode. Second, it serves as a short button press
detector, sending an interrupt to the host when the button driving the TSMR pin has been pressed for Wake1,
Wake2, or long press durations. This allows the implementation of different functions in the end application
such as menu selection and control. Finally it serves as a means to get the device into ship mode or reset the
system by performing a power cycle/ hardware reset (shut down SYS and automatically powering it back on)
after detecting a long button press. The timing for the short and long button press duration is programmable
through I2C for added flexibility and allows system designers to customize the end user experience of a specific
application. Note that if a specific timer duration is changed through I2C while that timer is active and has not
expired, the new programmed value will be ignored until the timer expires and/or is reset by new push button
action. In battery only mode the device will automatically pulse the TSMR current source ON for t
TS_DUTY_ON
duration and turn it OFF for t duration to check if a button is pressed. If a button press is registered,
TS_DUTY_OFF
the device will begin counting against Wake1, Wake2 or long press durations. This button press detection routine
in battery only mode is run as long as it is enabled by the EN_PUSH bit. When a valid adapter is present, the
TSMR current source is always ON to monitor charging.
8.3.8.1 Pushbutton Wake or Short Button Press Functions
There are two programmable wake or short button press timers, WAKE1 and WAKE2. There are no specific
actions taken by the t or t durations other than issuing an interrupt and updating the wake registers.
WAKE1 WAKE2
For a wake from shipmode event when the button press is enabled, the push button has to be low for t
shipwake
before the device can turn ON the SYS rail.
In the case where a valid V (V > V ) is connected prior to the t timer expiring, the device will exit
IN IN UVLO shipwake
shipmode immediately regardless of the TS/MR or wake timer state. Refer to Section 8.5 for more details.
8.3.8.2 Pushbutton Reset or Long Button Press Functions
Depending on the configuration set on the pushbutton long press action register bits, the device will perform a
shipmode entry or hardware reset or completely ignore the long button press action.
TS/MR
INT
VSYS
SW Reset
t
1EKAW
t
2EKAW
t
SSERPL
t
RESTART
VIN
PB_LPRESS_ACTION 01 – Hardware Reset Don’t care Default
t
NRAW_TESER
BQ25180
www.ti.com SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023
128us
Figure 8-4. Pushbutton Long Press Reset
Copyright © 2023 Texas Instruments Incorporated Submit Document Feedback 21
Product Folder Links: BQ25180


---

## Page 22


TS/MR
INT
SHIPMODE
1EKAWt 2EKAWt
SSERPLt
Shipmode enabled when
TS/MR is high
VIN
SYS
PB_LPRESS_ACTION Don’t care Ready to Enter Shipmode
NRAW_TESERt
BQ25180
SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023 www.ti.com
128 µs
Figure 8-5. Pushbutton Long Press Shipmode
8.3.9 15-Second Timeout for HW Reset
Based on the I2C register bit WATCHDOG_15S_ENABLE the device can perform a HW reset/power cycle in the
same manner a long button press or HW_RESET would. This 15-second watchdog or timeout is gated upon
V > V + V so that the HW reset would only occur if the host does not respond after a charger is
IN VBAT SLEEPZ
connected and VIN_PGOOD_STAT is set.
If the charger is connected and the host responds before the 15-second watchdog expires, the part continues in
normal operation and starts the normal 50-second watchdog timer if enabled. The 15-second watchdog may be
enabled/disabled through I2C with the WATCHDOG_15S_ENABLE bit.
8.3.10 Hardware Reset
The device is capable of a hardware reset to completely powercycle the system. This is partcularly useful when
a soft reset on the MCU or host fails to work. Below is a sequence of events during a hadware reset:
1. Turn OFF (if adapter is present) input blocking FET (Q1/Q2)
2. Turn OFF battery FET (Q3)
3. Engage pulldown on SYS
4. Start the Autowake timer
5. Once the Autowake timer expires, disconnect the pulldown on SYS
6. Reset all registers to default
7. Turn ON battery FET and input FET (if applicable)
8.3.11 Software Reset
When a software reset is issued either through a watchdog action configurable through the WATCHDOG_SEL
bits or register reset configurable through the REG_RST bit, the device will reset all of the registers to the
defaults. Any bits loaded through OTP memory are also loaded. If the device was waiting to go to shipmode (all
conditions for entering ship are fulfilled except adapter removal), a hardware or software reset will cancel the
pending shipmode request. If the shipmode request was written through I2C, the host can cancel the ship entry
by clearing the bit before shipmode entry has happened.
8.3.12 Interrupt Indicator (/INT) Pin
The device contains an open-drain output that signals its status and is valid only after the device has completed
start-up into a valid state. If the part starts into a fault, interrupts will not be sent.
22 Submit Document Feedback Copyright © 2023 Texas Instruments Incorporated
Product Folder Links: BQ25180


---

## Page 23


BQ25180
www.ti.com SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023
The /INT pin is normally in high impedance and is pulled low for 128 μs when an interrupt condition occurs.
When a fault or status change occurs or any other condition that generates an interrupt, a 128-μs pulse (/INT pin
pulled down) is sent on /INT to notify the host.
Interrupts can be masked through I2C. If the interrupt condition occurs while the interrupt is masked an interrupt
pulse will not be sent. If the interrupt is unmasked while the fault condition is still present, an interrupt pulse
will not be sent until the /INT trigger condition occurs while unmasked. Below are a list of interrupts that can be
masked through I2C.
Table 8-5. Mask Bit
MASK BIT ACTION
ILIM_INT_MASK Do not issue an /INT pulse when ILIM limiting occurs
VDPM_INT_MASK Do not issue an /INT pulse when VINDPM or DDPM is active
TS_INT_MASK Do not issue an /INT pulse when any of the TS events have occured.
TREG_INT_MASK Do not issue an /INT pulse when TREG is actively reducing the current
PG_INT_MASK Do not issue an /INT pulse when VIN meets VIN_PG condition
BAT_INT_MASK Do not issue an /INT pulse when BATOCP or BUVLO event is triggered
CHG_STATUS_INT_MASK Do not send an interrupt anytime there is a charging status change.
8.3.13 External NTC Monitoring (TS)
8.3.13.1 TS Biasing and Function
The device can be configured to meet JEITA requirements or a simpler HOT/COLD function only. Additionally,
the TS charger control function can be disabled through the TS_EN bit. This will only disable the TS charge
action but the faults are still reported based on the TS voltage. To satisfy the JEITA requirements, four
temperature thresholds are monitored: cold battery threshold, cool battery threshold, warm battery threshold,
and hot battery threshold. These temperatures correspond to the VCOLD, VCOOL, VWARM, and VHOT
thresholds in the Electrical Characteristics table. Charging and safety timers are suspended when V < V
TS HOT
or V > V . When V < V < V , the charging current is reduced to the value programmed in the
TS COLD COOL TS COLD
TS_Setting register/bit TS_ICHG_0. When V < V < V , the battery regulation voltage is reduced by 100
HOT TS WARM
mV or 200 mV based on the value programmed in the TS_VRCG_0 bit within the TS_Setting register.
For devices where the TS function is not needed, tie a 10-kΩ resistor to the TS pin.
There is an active voltage clamp present on this device which will prevent the voltage on the TSMR pin from
rising above the VTS_CLAMP threshold. This will particularly be ON when the TSMR pin is floating. The bit
TS_OPEN_STAT is set when this clamp is active. This will also be ON regardless of the TS_EN bit. The interrupt
is asserted as long as the TS_INT mask is not written.
The bits TS_HOT/TS_COLD, TS_WARM, and TS_COOL will allow these thresholds to be adjusted. The
hysteresis will also move along with these thresholds. When the TS_WARM condition occurs, the device will
lower the battery target regulation voltage by TS_VRCG but will not modify the VBAT_CTRL register.
The TS_ICHG bit will reduce charging current based on the factor described in the register map when the TSMR
pin hits a TS_COOL condition. The TREG function will still be based on this reduced threshold.
The TS_VRCG_0 bit will reduce the charging voltage when the TSMR pin hits the TS_WARM threshold. The
factor will be based on the register map.
When the button is detected as pressed (TSMR pin low) during the charging process, charging will be
momentarily suspended until the button is high again. When charging is disabled in any of the TS faults, trickle
charging is also disabled. In a TS fault where the current is reduced (COOL), the trickle charging current is not
altered.
Copyright © 2023 Texas Instruments Incorporated Submit Document Feedback 23
Product Folder Links: BQ25180


---

## Page 24


BQ25180
SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023 www.ti.com
8.3.14 I2C Interface
The device uses an I2C compatible interface to program and read control parameters, status bits, and so forth.
I2C ™ is a 2-wire serial interface developed by Philips Semiconductor (see I2C-Bus Specification, Version 2.1,
January 2000). The bus consists of a data line (SDA) and a clock line (SCL) with pullup structures. When
the bus is idle, both SDA and SCL lines are pulled high. All of the I2C compatible devices connect to the I2C
bus through open drain I/O pins, SDA and SCL. A master device, usually a microcontroller or a digital signal
processor, controls the bus. The master is responsible for generating the SCL signal and device addresses. The
master also generates specific conditions that indicate the START and STOP of data transfer. A slave device
receives and/or transmits data on the bus under control of the master device.
The device works as a preipheral and supports the following data transfer modes, as defined in the I2C Bus™
Specification: standard mode (100 kbps) and fast mode (400 kbps). The interface adds flexibility to the battery
charge solution, enabling most functions to be programmed to new values depending on the instantaneous
application requirements.
Register contents remain intact as long as V or V voltages remain above their respective undervoltage
BAT IN
lockout thresholds and the device is not in shutdown mode.
The data transfer protocol for standard and fast modes is exactly the same; therefore, they are referred to as the
F/S-mode in this document. The device only supports 7-bit addressing. The device 7-bit address is 0x6A (8-bit
shifted address is 0xD4).
8.3.14.1 F/S Mode Protocol
The master initiates a data transfer by generating a start condition. The start condition is when a high-to-low
transition occurs on the SDA line while SCL is high, as shown in Figure 8-6. All I2C-compatible devices should
recognize a start condition.
DATA
CLK
S P
START Condition STOPCondition
Figure 8-6. START and STOP Condition
The master then generates the SCL pulses, and transmits the 8-bit address and the read/write direction bit R/W
on the SDA line. During all transmissions, the master ensures that data is valid. A valid data condition requires
the SDA line to be stable during the entire high period of the clock pulse (see Figure 8-7). All devices recognize
the address sent by the master and compare it to their internal fixed addresses. Only the slave device with a
matching address generates an acknowledge (see Figure 8-8) by pulling the SDA line low during the entire high
period of the ninth SCL cycle. Upon detecting this acknowledge, the master knows that communication link with
a slave has been established.
DATA
CLK
Data Line Change
Stable; of Data
Data Valid Allowed
Figure 8-7. Bit Transfer on the Serial Interface
24 Submit Document Feedback Copyright © 2023 Texas Instruments Incorporated
Product Folder Links: BQ25180


---

## Page 26


BQ25180
SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023 www.ti.com
Table 8-6. Function Availability Based on Primary Mode of Operation
FUNCTION CHARGE/ADAPTER MODE BATTERY MODE SHIP MODE SHUTDOWN MODE
Input overvoltage Yes Yes No No
Input undervoltage Yes Yes Yes Yes
Battery overcurrent Yes, if enabled Yes Yes, if enabled No
Battery undervoltage Yes Yes No No
Input DPM Yes, if enabled No No No
Dynamic power path
Yes, if enabled No No No
management
BATFET Yes Yes No No
TS measurement Yes No No No
Battery charging Yes, if enabled No No No
ILIM Yes (Register Value) No No No
Pushbutton input Yes Yes, if enabled Yes No
INT output Yes Yes No No
I2C Yes Yes No No
8.5 Register Maps
8.5.1 I2C Registers
Table 8-7 lists the memory-mapped registers for the I2C registers. All register offset addresses not listed in Table
8-7 should be considered as reserved locations and the register contents should not be modified.
Table 8-7. I2C Registers
Offset Acronym Register Name Section
0x0 STAT0 Charger Status Go
0x1 STAT1 Charger Status and Faults Go
0x2 FLAG0 Charger Flag Registers Go
0x3 VBAT_CTRL Battery Voltage Control Go
0x4 ICHG_CTRL Fast Charge Current Control Go
0x5 CHARGECTRL0 Charger Control 0 Go
0x6 CHARGECTRL1 Charger Control 1 Go
0x7 IC_CTRL IC Control Go
0x8 TMR_ILIM Timer and Input Current Limit Control Go
0x9 SHIP_RST Shipmode, Reset and Pushbutton Control Go
0xA SYS_REG SYS Regulation Voltage Control Go
0xB TS_CONTROL TS Control Go
0xC MASK_ID MASK and Device ID Go
Complex bit access types are encoded to fit into small table cells. Table 8-8 shows the codes that are used for
access types in this section.
Table 8-8. I2C Access Type Codes
Access Type Code Description
Read Type
R R Read
26 Submit Document Feedback Copyright © 2023 Texas Instruments Incorporated
Product Folder Links: BQ25180


---

## Page 27


BQ25180
www.ti.com SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023
Table 8-8. I2C Access Type Codes (continued)
Access Type Code Description
RC R Read
C to Clear
Write Type
W W Write
Reset or Default Value
-n Value after reset or the default
value
Copyright © 2023 Texas Instruments Incorporated Submit Document Feedback 27
Product Folder Links: BQ25180


---

## Page 29


BQ25180
www.ti.com SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023
8.5.1.2 STAT1 Register (Offset = 0x1) [Reset = X]
STAT1 is shown in Figure 8-11 and described in Table 8-10.
Return to the Summary Table.
Figure 8-11. STAT1 Register
7 6 5 4 3 2 1 0
VIN_OVP_STA BUVLO_STAT RESERVED TS_STAT_1:0 SAFETY_TMR_ WAKE1_FLAG WAKE2_FLAG
T FAULT_FLAG
R-1b0 R-X R-X R-2b00 RC-1b0 RC-1b0 RC-1b0
Table 8-10. STAT1 Register Field Descriptions
Bit Field Type Reset Description
7 VIN_OVP_STAT R 1b0 VIN_OVP Fault
1b0 = Not Active
1b1 = Active
6 BUVLO_STAT R X Battery UVLO Status
1b0 = Not Active
1b1 = Active
5 RESERVED R X Reserved
4-3 TS_STAT_1:0 R 2b00 TS Status
2b00 = Normal
2b01 = VTS < VHOT or VTS > VCOLD(charging suspended)
2b10 = VCOOL < VTS < VCOLD (Charging current reduced by value
set by TS_Registers)
2b11 = VWARM > VTS > VHOT (Charging voltage reduced by value
set by TS_Registers)
2 SAFETY_TMR_FAULT_F RC 1b0 Safety Timer Expired Fault Cleared only after CE is toggled.
LAG 1b0 = Not Active
1b1 = Active
1 WAKE1_FLAG RC 1b0 Wake 1 Timer Flag
1b0 = Does not meet Wake 1 Condition
1b1 = Met Wake 1 Condition
0 WAKE2_FLAG RC 1b0 Wake 2 Timer Flag
1b0 = Does not meet Wake 2 Condition
1b1 = Met Wake2 Condition
Copyright © 2023 Texas Instruments Incorporated Submit Document Feedback 29
Product Folder Links: BQ25180


---

## Page 30


BQ25180
SLUSE99C – SEPTEMBER 2021 – REVISED JANUARY 2023 www.ti.com
8.5.1.3 FLAG0 Register (Offset = 0x2) [Reset = X]
FLAG0 is shown in Figure 8-12 and described in Table 8-11.
Return to the Summary Table.
Figure 8-12. FLAG0 Register
7 6 5 4 3 2 1 0
TS_FAULT ILIM_ACTIVE_ VDPPM_ACTIV VINDPM_ACTI THERMREG_A VIN_OVP_FAU BUVLO_FAULT BAT_OCP_FAU
FLAG E_FLAG VE_FLAG CTIVE_FLAG LT_FLAG _FLAG LT
RC-X RC-X RC-X RC-X RC-X RC-X RC-X RC-X
Table 8-11. FLAG0 Register Field Descriptions
Bit Field Type Reset Description
7 TS_FAULT RC X TS_Fault
1b0 = No TS Fault detected
1b1 = TS Fault detected
6 ILIM_ACTIVE_FLAG RC X ILIM Active
1b0 = NO ILIM Fault detected
1b1 = ILIM Fault detected
5 VDPPM_ACTIVE_FLAG RC X VDPPM FLAG
1b0 = VDPPM fault not detected
1b1 = VDPPM fault detected
4 VINDPM_ACTIVE_FLAG RC X VINDPM FLAG
1b0 = VINDPM fault not detected
1b1 = VINDPM fault detected
3 THERMREG_ACTIVE_FL RC X Thermal Regulation FLAG
AG 1b0 = No thermal regulation detected
1b1 = Thermal regulation has occured
2 VIN_OVP_FAULT_FLAG RC X VIN_OVP FLAG
1b0 = VIN_OVP fault not detected
1b1 = VIN_OVP fault detected
1 BUVLO_FAULT_FLAG RC X Battery undervoltage FLAG
1b0 = Battery undervoltage fault not detected
1b1 = Battery undervoltage fault detected
0 BAT_OCP_FAULT RC X Battery overcurrent protection
1b0 = Battery overcurrent condition not detected
1b1 = Battery overcurrent condition detected
30 Submit Document Feedback Copyright © 2023 Texas Instruments Incorporated
Product Folder Links: BQ25180


---

## Page 55


PACKAGE OPTION ADDENDUM
www.ti.com 9-Nov-2025
PACKAGING INFORMATION
Orderable part number Status Material type Package | Pins Package qty | Carrier RoHS Lead finish/ MSL rating/ Op temp (°C) Part marking
(1) (2) (3) Ball material Peak reflow (6)
(4) (5)
BQ25180YBGR Active Production DSBGA (YBG) | 8 3000 | LARGE T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 B180
BQ25180YBGR.A Active Production DSBGA (YBG) | 8 3000 | LARGE T&R Yes SNAGCU Level-1-260C-UNLIM -40 to 85 B180
(1) Status: For more details on status, see our product life cycle.
(2) Material type: When designated, preproduction parts are prototypes/experimental devices, and are not yet approved or released for full production. Testing and final process, including without limitation quality assurance,
reliability performance testing, and/or process qualification, may not yet be complete, and this item is subject to further changes or possible discontinuation. If available for ordering, purchases will be subject to an additional
waiver at checkout, and are intended for early internal evaluation purposes only. These items are sold without warranties of any kind.
(3) RoHS values: Yes, No, RoHS Exempt. See the TI RoHS Statement for additional information and value definition.
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
Addendum-Page 1


---

## Page 56


PACKAGE MATERIALS INFORMATION
www.ti.com 20-Jan-2023
TAPE AND REEL INFORMATION
REEL DIMENSIONS TAPE DIMENSIONS
K0 P1
W
B0
Reel
Diameter
Cavity A0
A0 Dimension designed to accommodate the component width
B0 Dimension designed to accommodate the component length
K0 Dimension designed to accommodate the component thickness
W Overall width of the carrier tape
P1 Pitch between successive cavity centers
Reel Width (W1)
QUADRANT ASSIGNMENTS FOR PIN 1 ORIENTATION IN TAPE
Sprocket Holes
Q1 Q2 Q1 Q2
Q3 Q4 Q3 Q4 User Direction of Feed
Pocket Quadrants
*All dimensions are nominal
Device Package Package Pins SPQ Reel Reel A0 B0 K0 P1 W Pin1
Type Drawing Diameter Width (mm) (mm) (mm) (mm) (mm) Quadrant
(mm) W1 (mm)
BQ25180YBGR DSBGA YBG 8 3000 180.0 8.4 1.15 1.75 0.65 4.0 8.0 Q1
Pack Materials-Page 1
