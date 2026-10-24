# MAX98357A - Hardware Technical Specification

> **Extracted via:** DeepSheet + Docling Pipeline  
> **Source Document:** MAX98357A.pdf  

---

## Page 1


EVALUATION KIT AVAILABLE
MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
General Description Features
The MAX98357A/MAX98357B are digital pulse-code S Single-Supply Operation (2.5V to 5.5V)
modulation (PCM) input Class D power amplifiers that
S 3.2W Output Power into 4I at 5V
provide Class AB audio performance with Class D effi-
ciency. These ICs offer five selectable gain settings S 2.4mA Quiescent Current
(3dB, 6dB, 9dB, 12dB, and 15dB) in I2S/left-justified
mode set by a single gain select input and a fixed 12dB
S 92% Efficiency (RL = 8I, POUT = 1W)
gain in TDM mode. S 25µVRMS Output Noise (AV = 15dB)
The digital audio interface is highly flexible with the
S Low 0.015% THD+N at 1kHz
MAX98357A supporting I2S data and the MAX98357B sup-
porting left-justified data. Both ICs support 8 channel time S No MCLK Required
division multiplexed (TDM) data. The digital audio interface
S Sample Rates of 8kHz to 96kHz
accepts specified sample rates between 8kHz and 96kHz
for all supported data formats. The ICs can be configured S Supports Left, Right, or (Left/2 + Right/2) Output
to produce a left channel, right channel, or (left/2 + right/2) S Sophisticated Edge Rate Control Enables
output from the stereo input data. The ICs operate using
Filterless Class D Outputs
16/24/32-bit data for I2S and left-justified modes as well as
16-bit or 32-bit data using TDM mode. The ICs eliminate S 77dB PSRR at 1kHz
the need for the external MCLK signal that is typically used
S Low RF Susceptibility Rejects TDMA
for PCM communication. This reduces EMI and possible
Noise from GSM Radios
board coupling issues in addition to reducing the size and
pin count of the ICs. S Extensive Click-and-Pop Reduction Circuitry
The ICs also feature a very high wideband jitter tolerance S Robust Short-Circuit and Thermal Protection
(12ns typ) on BCLK and LRCLK to provide robust operation.
S Available in Space-Saving Packages:
Active emissions-limiting, edge-rate limiting, and over-
1.345mm x 1.435mm WLP (0.4mm Pitch)
shoot control circuitry greatly reduce EMI. A filterless
spread-spectrum modulation scheme eliminates the and 3mm x 3mm TQFN
need for output filtering found in traditional Class D
devices and reduces the component count of the solution.
The ICs are available in 9-pin WLP (1.345mm x 1.435mm
Simplified Block Diagram
x 0.64mm) and 16-pin TQFN (3mm x 3mm x 0.75mm)
packages and are specified over the -40NC to +85NC
temperature range.
Applications
SHUTDOWN
Notebook and Netbook Computers AND MAX98357A GAIN
CHANNEL MAX98357B CONTROL
Cellular Phones SELECT
Tablets
DIGITAL CLASS D
PCM AUDIO DAC OUTPUT
INPUT INTERFACE STAGE
Ordering Information appears at end of data sheet.
Functional Diagram appears at end of data sheet.
For pricing, delivery, and ordering information, please contact Maxim Direct at
1-888-629-4642, or visit Maxim Integrated’s website at www.maximintegrated.com. 19-6779; Rev 7; 2/16


---

## Page 2


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
TABLE OF CONTENTS
General Description ............................................................................ 1
Applications .................................................................................. 1
Features ..................................................................................... 1
Simplified Block Diagram........................................................................ 1
Absolute Maximum Ratings ...................................................................... 4
Package Thermal Characteristics.................................................................. 4
Electrical Characteristics ........................................................................ 4
Typical Operating Characteristics ................................................................. 9
General ....................................................................................9
Speaker Amplifier ............................................................................9
Pin Configurations ............................................................................ 17
Pin Description............................................................................... 17
Detailed Description........................................................................... 18
Digital Audio Interface Modes..................................................................18
MCLK Elimination.........................................................................18
BCLK Jitter Tolerance .....................................................................18
BCLK Polarity............................................................................18
LRCLK Polarity...........................................................................18
Standby Mode ...........................................................................19
DAC Digital Filters...........................................................................19
SD_MODE and Shutdown Operation.............................................................19
I2S and Left Justified Mode.................................................................21
TDM Mode..............................................................................21
Class D Speaker Amplifier ....................................................................30
Ultra-Low EMI Filterless Output Stage.........................................................30
Speaker Current Limit .....................................................................30
Gain Selection ...........................................................................30
Click-and-Pop Suppression.................................................................30
Applications Information........................................................................ 31
Filterless Class D Operation ...................................................................35
Power-Supply Input..........................................................................35
Layout and Grounding .......................................................................35
Functional Diagram ........................................................................... 35
WLP Applications Information..................................................................36
Ordering Information .......................................................................... 36
Package Information........................................................................... 37
Revision History .............................................................................. 40
Maxim Integrated 2


---

## Page 3


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
LIST OF FIGURES
Figure 1. I2S Audio Interface Timing Diagram (MAX98357A) ............................................ 8
Figure 2. Left-Justified Audio Interface Timing Diagram (MAX98357B) .................................... 8
Figure 3. TDM Audio Interface Timing Diagram....................................................... 8
Figure 4. SD_MODE Resistor Connected Using Open Drain Driver...................................... 20
Figure 5. SD_MODE Resistor Connected Using Push-Pull Driver........................................ 20
Figure 6. MAX98357A I2S Digital Audio Interface Timing, 16-Bit Resolution ............................... 22
Figure 7. MAX98357A I2S Digital Audio Interface Timing, 32-Bit Resolution................................ 23
Figure 8. MAX98357B Left-Justified Digital Audio Interface Timing, 16-Bit Resolution ....................... 24
Figure 9. MAX98357B Left-Justified Digital Audio Interface Timing, 32-Bit Resolution ....................... 25
Figure 10. MAX98357A TDM 16-Bit DAI Timing ..................................................... 26
Figure 11. MAX98357A TDM 32-Bit DAI Timing...................................................... 27
Figure 12. MAX98357B TDM 16-Bit DAI Timing ..................................................... 28
Figure 13. MAX98357B TDM 32-Bit DAI Timing ..................................................... 29
Figure 14. EMI with 12in of Speaker Cable and No Output Filtering...................................... 30
Figure 15. Left-Channel PCM Operation with 6dB Gain ............................................... 31
Figure 16. Left-Channel PCM Operation with 12dB Gain .............................................. 31
Figure 17. Right-Channel PCM Operation with 6dB Gain .............................................. 32
Figure 18. (Left/2 + Right/2) PCM Operation with 6dB Gain............................................ 32
Figure 19. Stereo PCM Operation Using Two ICs .................................................... 33
Figure 20. Channel TDM Operation (Gain Fixed at 12dB).............................................. 34
Figure 21. MAX98357A/MAX98357B WLP Ball Dimensions . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 36
LIST OF TABLES
Table 1. RMS Jitter Tolerance.................................................................... 18
Table 2. BCLK Polarity......................................................................... 18
Table 3. LRCLK Polarity........................................................................ 18
Table 4. Digital Filter Settings.................................................................... 19
Table 5. SD_MODE Control...................................................................... 19
Table 6. Examples of SD_MODE Pullup Resistor Values ............................................... 19
Table 7. TDM Mode Channel Selection ............................................................ 21
Table 8. Gain Selection ........................................................................ 30
Maxim Integrated 3


---

## Page 4


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
ABSOLUTE MAXIMUM RATINGS
VDD, LRCLK, BCLK, and DIN to GND ....................-0.3V to +6V Continuous Power Dissipation (TA = +70NC)
All Other Pins to GND ..............................-0.3V to (VDD + 0.3V) WLP (derate 13.7mW/NC above +70NC)....................1096mW
Continuous Current In/Out of VDD/GND/OUT_ .................Q1.6A TQFN (derate 20.8mW/NC above +70NC)..................1666mW
Continuous Input Current (all other pins) ........................Q20mA Junction Temperature .....................................................+150NC
Duration of OUT_ Short Circuit to GND or VDD….. ...Continuous Operating Temperature Range ..........................-40NC to +85NC
Duration of OUTP Short to OUTN .............................Continuous Storage Temperature Range ............................-65NC to +150NC
Soldering Temperature (reflow) ......................................+230NC
Stresses beyond those listed under “Absolute Maximum Ratings” may cause permanent damage to the device. These are stress ratings only, and functional opera-
tion of the device at these or any other conditions beyond those indicated in the operational sections of the specifications is not implied. Exposure to absolute
maximum rating conditions for extended periods may affect device reliability.
PACKAGE THERMAL CHARACTERISTICS (Note 1)
WLP TQFN
Junction-to-Ambient Thermal Resistance (qJA) ..........73 °C/W Junction-to-Ambient Thermal Resistance (qJA) ..........48 °C/W
Junction-to-Case Thermal Resistance (qJC) ...............50 °C/W Junction-to-Case Thermal Resistance (qJC) .................7 °C/W
Note 1: Package thermal resistances were obtained using the method described in JEDEC specification JESD51-7, using a four-layer
board. For detailed information on package thermal considerations, refer to www.maximintegrated.com/thermal-tutorial.
ELECTRICAL CHARACTERISTICS
(VDD = 5V, VGND = 0V, GAIN_SLOT = VDD. BCLK = 3.072MHz, LRCLK = 48kHz, speaker loads (ZSPK) connected between OUTP
and OUTN, ZSPK = J, TA = TMIN to TMAX, unless otherwise noted. Typical values are at TA = +25NC.) (Note 2)
PARAMETER SYMBOL CONDITIONS MIN TYP MAX UNITS
Supply Voltage Range VDD Guaranteed by PSSR test 2.5 5.5 V
Undervoltage Lockout UVLO 1.4 1.8 2.3 V
TA = +25NC 2.75 3.35
Quiescent Current IDD mA
TA = +25NC, VDD = 3.7V 2.4 2.85
Shutdown Current ISHDN SD_MODE = 0V, TA = +25NC 0.6 2 FA
Standby Current ISTNDBY SD_MODE = 1.8V, no BCLK, TA = +25NC 340 400 FA
Turn-On Time tON 7 7.5 ms
Output Offset Voltage VOS TA = +25NC, gain = 15dB Q0.3 Q2.5 mV
Peak voltage, TA = Into shutdown -72
+25NC, A-weighted,
Click-and-Pop Level KCP
32 samples per
dBV
second (Note 3) Out of shutdown -66
VDD = 2.5V to 5.5V, TA = +25NC 60 75
f = 217Hz,
77
Power-Supply Rejection Ratio PSRR TA = +25NC 200mVP-P ripple dB
(Notes 3, 4) f = 10kHz,
60
200mVP-P ripple
Maxim Integrated 4


---

## Page 5


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
ELECTRICAL CHARACTERISTICS (continued)
(VDD = 5V, VGND = 0V, GAIN_SLOT = VDD. BCLK = 3.072MHz, LRCLK = 48kHz, speaker loads (ZSPK) connected between OUTP
and OUTN, ZSPK = J, TA = TMIN to TMAX, unless otherwise noted. Typical values are at TA = +25NC.) (Note 2)
PARAMETER SYMBOL CONDITIONS MIN TYP MAX UNITS
ZSPK = 4I + 33FH 3.2
THD+N 10%, ZSPK = 8I + 68FH 1.8
gain = 12dB
ZSPK = 8I + 68FH,
0.93
VDD = 3.7V
Output Power (Note 3) POUT W
ZSPK = 4I + 33FH 2.5
THD+N = 1%, ZSPK = 8I + 68FH 1.4
gain = 12dB
ZSPK = 8I + 68FH,
0.77
VDD = 3.7V
f = 1kHz, POUT = 1W, TA = +25NC,
0.02 0.06
ZSPK = 4I + 33FH, WLP
Total Harmonic Distortion +
THD+N
f = 1kHz, POUT = 1W, TA = +25NC,
0.02 %
Noise ZSPK = 4I + 33FH, TQFN
f = 1kHz, POUT = 0.5W, TA = +25NC,
0.013
ZSPK = 8I + 68FH
A-weighted,
Dynamic Range DR 105 dB
VRMS = 2.54V, 24- or 32-bit data
Output Noise VN A-weighted, 24- or 32-bit data (Note 4) 25 FVRMS
GAIN_SLOT = GND through 100kI 14.4 15 15.6
GAIN_SLOT = GND 11.4 12 12.6
Gain (Relative to a 2.1dBV
Reference Level)
AV GAIN_SLOT = unconnected 8.4 9 9.6 dB
GAIN_SLOT = VDD 5.4 6 6.6
GAIN_SLOT = VDD through 100kI 2.4 3 3.6
Current Limit ILIM 2.8 A
Efficiency ε
ZSPK = 8I + 68FH, THD+N = 10%,
92 %
f = 1kHz, gain = 12dB
DAC Gain Error 1 %
Frequency Response -0.2 +0.2 dB
Class D Switching Frequency fOSC 330 kHz
Spread-Spectrum Bandwidth Q20 kHz
DAC DIGITAL FILTERS
VOICE MODE IIR LOWPASS FILTER (LRCLK < 30kHz)
0.443
Ripple limit cutoff
x fS
Passband Cutoff fPLP Hz
0.446
-3dB cutoff
x fS
0.464
Stopband Cutoff fSLP
x fS
Hz
Stopband Attenuation f > fSLP 75 dB
Maxim Integrated 5


---

## Page 6


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
ELECTRICAL CHARACTERISTICS (continued)
(VDD = 5V, VGND = 0V, GAIN = VDD. BCLK = 3.072MHz, LRCLK = 48kHz, speaker loads (ZSPK) connected between OUTP and
OUTN, ZSPK = J, TA = TMIN to TMAX, unless otherwise noted. Typical values are at TA = +25NC.) (Note 2)
PARAMETER SYMBOL CONDITIONS MIN TYP MAX UNITS
AUDIO MODE FIR LOWPASS FILTER (30kHz < LRCLK < 50kHz)
0.43
Ripple limit cutoff
x fS
0.47
Passband Cutoff fPLP -3dB cutoff
x fS
Hz
0.5
-6.02dB cutoff
x fS
0.58
Stopband Cutoff fSLP
x fS
Hz
Stopband Attenuation f > fSLP 60 dB
AUDIO MODE FIR LOWPASS FILTER (LRCLK > 50kHz)
0.24
Ripple limit cutoff
x fS
Passband Cutoff fPLP Hz
0.31
-3dB cutoff
x fS
0.477
Stopband Cutoff fSLP
x fS
Hz
Stopband Attenuation f < fSLP 60 dB
DIGITAL AUDIO INTERFACE
LRCLK Range 1 fS1 7.6 8 8.4
LRCLK Range 2 fS2 15.2 16 16.8
kHz
LRCLK Range 3 fS3 30.4 48 50.4
LRCLK Range 4 fS4 83.8 96 100.8
I2S/left justified mode 16/24/32
Resolution Bits
TDM mode 16/32
BCLK Frequency Range fBCLKH BCLK must be 32, 48, or 64X of LRCLK 0.2432 25.804 MHz
BCLK High Time tBCLKH 15 ns
BCLK Low Time tBCLKL 15 ns
Maximum Low Frequency
RMS jitter below 40kHz 0.5
BCLK and LRCLK Jitter
ns
Maximum High Frequency
RMS jitter above 40kHz 12
BCLK and LRCLK Jitter
Input High Voltage VIH Digital audio inputs 1.3 V
Maxim Integrated 6


---

## Page 7


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
ELECTRICAL CHARACTERISTICS (continued)
(VDD = 5V, VGND = 0V, GAIN = VDD. BCLK = 3.072MHz, LRCLK = 48kHz, speaker loads (ZSPK) connected between OUTP and
OUTN, ZSPK = J, TA = TMIN to TMAX, unless otherwise noted. Typical values are at TA = +25NC.) (Note 2)
PARAMETER SYMBOL CONDITIONS MIN TYP MAX UNITS
Input Low Voltage VIL Digital audio inputs 0.6 V
Input Leakage Current IIH, IIL VIN = 0V, VDD = 5.5V, TA = +25NC -1 +1 FA
Input Capacitance CIN 3 pF
DIN to BCLK Setup Time tSETUP 10 ns
LRCLK to BCLK Setup Time tSYNCSET 10 ns
DIN to BCLK Hold Time tHOLD 10 ns
LRCLK to BCLK Hold Time tSYNCHOLD 10 ns
SD_MODE COMPARATOR TRIP POINTS
B0 0.08 0.16 0.355
See SD_MODE and shutdown operation
B1 0.65 0.77 0.825 V
for details
B2 1.245 1.4 1.5
SD_MODE Pulldown Resistor RPD 92 100 108 kI
GAIN_SLOT COMPARATOR TRIP POINTS
0.65 x 0.85 x
AV = 3dB gain
VDD VDD
0.9 x
AV = 6dB gain
VDD
VDD
V_GAIN_ 0.4 x 0.6 x
SLOT
AV = 9dB gain
VDD VDD
V
0.1 x
AV = 12dB gain 0
VDD
0.15 x 0.35 x
AV = 15dB gain
VDD VDD
Note 2: 100% production tested at TA = +25NC. Specifications over temperature limits are guaranteed by design.
Note 3: Class D amplifier testing performed with a resistive load in series with an inductor to simulate an actual speaker load. For
RL = 8I, LL = 68FH. For RL = 4I, LL = 33FH.
Note 4: Digital silence used for input signal.
Note 5: Dynamic range measured using the EIAJ method. -60dBFS 1kHz output signal, A-weighted, and normalized to 0dBFS.
f =20Hz to 20kHz.
Maxim Integrated 7


---

## Page 8


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
tBCLK
tBCLKH
VIH VIH VIH tBCLKL
BCLK (INPUT) VIL VIL VIL
tSYNCSET tSYNCHOLD
LRCLK (INPUT) VIH
VIL
tSETUP tHOLD
DIN (INPUT) VIH LEFT MSB RIGHT MSB
VIL
Figure 1. I2S Audio Interface Timing Diagram (MAX98357A)
tBCLK
tBCLKH
VIH VIH VIH tBCLKL
BCLK (INPUT) VIL VIL VIL
tSYNCSET tSYNCHOLD
VIH
LRCLK (INPUT)
VIL
tSETUP tHOLD
DIN (INPUT) LEFT MSB VIH RIGHT MSB
VIL
Figure 2. Left-Justified Audio Interface Timing Diagram (MAX98357B)
tBCLK
tBCLKH tBCLKL
tBCLK
tBCLKL tBCLKH
BCLK (INPUT) VIH VIH BCLK (input) VIH VIH
VIL VIL VIL VIL
tSYNCSET tSYNCSET
tSYNCHOLD tSYNCHOLD
LRCLK (INPUT) VIH LRCLK (input) VIH
VIL VIL
tSETUP tHOLD tSETUP tHOLD
DIN (INPUT) MSB VIH DIN (input) MSB VIH
VIL VIL
MAX98357A MAX98357B
Figure 3. TDM Audio Interface Timing Diagram
Maxim Integrated 8


---

## Page 15


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
Pin Configurations
TOP VIEW
BUMP SIDE DOWN
MAX98357A MAX98357B
+
SD_MODE VDD OUTP
A1 A2 A3
13
DIN GAIN_SLOT OUTN
14 7
B1 B2 B3
15 6
BCLK GND LRCLK
C1 C2 C3 16 5
WLP
Pin Description
PIN
NAME FUNCTION
WLP TQFN
Shutdown and Channel Select. Pull SD_MODE low to place the device in shutdown. In I2S
A1 4 SD_MODE or LJ mode, SD_MODE selects the data channel (Table 5). In TDM mode, SD_MODE and
GAIN_SLOT are both used for channel selection (Table 7).
A2 7, 8 VDD Power-Supply Input
A3 9 OUTP Positive Speaker Amplifier Output
B1 1 DIN Digital Input Signal
Gain and Channel Selection. In I2S and LJ mode determines amplifier output gain (Table 8)
GAIN_
B2 2 In TDM mode, used for channel selection with SD_MODE (Table 7). In TDM mode, gain is
SLOT
fixed at 12dB.
B3 10 OUTN Negative Speaker Amplifier Output
C1 16 BCLK Bit Clock Input
C2 3, 11, 15 GND Ground
C3 14 LRCLK Frame Clock. Left/right clock for I2S and LJ mode. Sync clock for TDM mode.
5, 6,
— N.C. No Connection
12, 13
Exposed Pad. The exposed pad is not internally connected. Connect the exposed page to a
— — EP
solid ground plane for thermal dissipation.
Maxim Integrated 15
DNG EDOM_DS
8
.C.N
NID
NTUO PTUO
12 10 9
VDD
LRCLK VDD
GND N.C.
BCLK N.C.
1 3 4
DNG
TOLS_NIAG
TOP VIEW
11
N.C.
MAX98357A
MAX98357B
+
2
TQFN


---

## Page 16


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
Detailed Description BCLK Jitter Tolerance
The ICs feature a BCLK jitter tolerance of 0.5ns for RMS
The MAX98357A/MAX98357B are digital PCM input jitter below 40kHz and 12ns for wideband RMS jitter while
Class D power amplifiers. The MAX98357A accepts maintaining a dynamic range greater than 98dB (Table 1).
standard I2S data through DIN, BCLK, and LRCLK while
BCLK Polarity
the MAX98357B accepts left-justified data through the
When operating in I2S/left justified mode, incoming serial
same inputs. Both versions also accept 16-bit or 32-bit
data is always clocked-in on the rising edge of BCLK.
TDM data with up to eight slots. The digital audio inter-
In TDM mode, the MAX98357A clocks-in serial data on
face eliminates the need for an external MCLK signal that
the rising edge of BCLK while the MAX98357B clocks in
is typically required for I2S data transmission.
serial data on the falling edge of BCLK (Table 2).
SD_MODE selects which data word is output by the
LRCLK Polarity
amplifier and is used to put the ICs into shutdown. These
LRCLK specifies whether left-channel data or right-
devices offer five gain settings in I2S/left-justified mode
channel data is currently being read by the digital audio
and a fixed 12dB gain in TDM mode. Channel selection
interface. The MAX98357A indicates the left channel
in TDM mode is set with the combination of SD_MODE
word when LRCLK is low, and the MAX98357B indicates
and GAIN_SLOT (Table 7).
the left channel word when LRCLK is high (Table 3).
The MAX98357A/MAX98357B feature low-quiescent cur-
LRCLK ONLY supports 8kHz, 16kHz, 32kHz, 44.1kHz,
rent, comprehensive click-and-pop suppression, and
48kHz, 88.2kHz and 96kHz frequencies. LRCLK clocks at
excellent RF immunity. The ICs offer Class AB audio
11.025kHz, 12kHz, 22.05kHz and 24kHz are NOT sup-
performance with Class D efficiency in a minimal board-
ported. Do not remove LRCLK while BCLK is present.
space solution. The Class D amplifier features spread-
Removing LRCLK while BCLK is present can cause unex-
spectrum modulation with edge-rate and overshoot
pected output behavior including a large DC output voltage.
control circuitry that offers significant improvements in
switch-mode amplifier radiated emissions. The amplifier Standby Mode
features click-and-pop suppression that reduces audi- The ICs automatically enter standby mode when BCLK
ble transients on startup and shutdown. The amplifier is removed. If BCLK stops toggling, the ICs automati-
includes thermal-overload and short-circuit protection.
Table 1. RMS Jitter Tolerance
Digital Audio Interface Modes
FREQUENCY RMS JITTER TOLERANCE (ns)
The input stage of the digital audio interface is highly flexi-
ble, supporting 8kHz–96kHz sampling rates with 16/24/32- < 40kHz 0.5
bit resolution for I2S/left justified data as well as up to a 40kHz–BCLK 12
8-slot, 16-bit or 32-bit time division multiplexed (TDM)
format. When LRCLK has a 50% duty cycle the data Table 2. BCLK Polarity
format is determined by the part number selection
(MAX98357A/MAX98357B). When a frame sync pulse MODE PART NUMBER BCLK POLARITY
is used for the LRCLK the data format is automatically I2S MAX98357A Rising edge
configured in TDM mode. The frame sync pulse indicates
Left-justified MAX98357B Rising edge
the beginning of the first time slot.
MAX98357A Rising edge
TDM
MCLK Elimination
MAX98357B Falling edge
The ICs eliminate the need for the external MCLK sig-
nal that is typically used for PCM communication. This
Table 3. LRCLK Polarity
reduces EMI and possible board coupling issues in addi-
tion to reducing the size and pin-count of the ICs.
PART NUMBER LRCLK POLARITY (LEFT CHANNEL)
MAX98357A Low
MAX98357B High
Maxim Integrated 16


---

## Page 17


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
cally enter standby mode. In standby mode, the Class Drive SD_MODE high to select the left word of the stereo
D speaker is turned off and the outputs go into a high- input data. Drive SD_MODE high through a sufficiently
impedance state, ensuring that unwanted current is not small resistor to select the right word of the stereo input
transferred to the load during this condition. Standby data. Drive SD_MODE high through a sufficiently large
mode has reduced power consumption from normal resistor to select both the left and right words of the
operation (340µA), but does not reach as low as full stereo input data (left/2 + right/2). RLARGE and RSMALL
shutdown (0.6µA). Standby mode can be used to reduce are determined by the VDDIO voltage (logic voltage from
power consumption when no GPIO µs available to pull control interface) that is driving SD_MODE according to
SD_MODE low. the following two equations:
DAC Digital Filters RSMALL (kI) = 94.0 x VDDIO - 100
The DAC features a digital lowpass filter that is automati- RLARGE (kI) = 222.2 x VDDIO - 100
cally configured for voice playback or music playback
When the devices are configured in left-channel mode
based on the sample rate that is used. This filter elimi-
(SD_MODE is directly driven to logic-high by the con-
nates the effect of aliasing and any other high-frequency
trol interface), take care to avoid violating the Absolute
noise that might otherwise be present. Table 4 shows the
Maximum Ratings limits for SD_MODE. Ensuring that
digital filter settings that are automatically selected.
VDD is always greater than VDDIO is one way to prevent
SD_MODE and Shutdown Operation SD_MODE from violating the Absolute Maximum Ratings
The ICs feature a low-power shutdown mode, drawing limits. If this is not possible in the application (e.g., if VDD
less than 0.6FA (typ) of supply current. During shutdown, < 3.0V and VDDIO = 3.3V), then it is necessary to add a
all internal blocks are turned off, including setting the small resistance (~2kI) in series with SD_MODE to limit
output stage to a high-impedance state. Drive SD_MODE the current into the SD_MODE pin. This is not a concern
low to put the ICs into shutdown. when using the right channel or (left/2 + right/2) modes.
The state of SD_MODE determines the audio channel Figure 4 and Figure 5 show how to connect an external
that is sent to the amplifier output (Table 5). resistor to SD_MODE when using an open-drain driver or
a push-pull driver.
Table 4. Digital Filter Settings
LRCLK -3dB CUTOFF RIPPLE LIMIT CUTOFF STOPBAND CUTOFF STOPBAND
FREQUENCY FREQUENCY FREQUENCY FREQUENCY ATTENUATION (dB)
fLRCLK < 30kHz 0.446 x fLRCLK 0.443 x fLRCLK 0.464 x fLRCLK 75
30kHz < fLRCLK < 50kHz 0.47 x fLRCLK 0.43 x fLRCLK 0.58 x fLRCLK 60
fLRCLK > 50kHz 0.31 x fLRCLK 0.24 x fLRCLK 0.477 x fLRCLK 60
Table 5. SD_MODE Control
SD_MODE STATUS SELECTED CHANNEL
High VSD_MODE > B2 trip point Left
Pullup through RSMALL B2 trip point > VSD_MODE > B1 trip point Right
Pullup through RLARGE B1 trip point > VSD_MODE > B0 trip point (Left/2 + right/2)
Low B0 trip point > VSD_MODE Shutdown
Table 6. Examples of SD_MODE Pullup Resistor Values
LOGIC VOLTAGE LEVEL (VDDIO) (V) RSMALL (kI) RLARGE (kI)
1.8 69.8 300
3.3 210.2 634
Maxim Integrated 17


---

## Page 18


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
PROCESSOR VDDIO LEFT MODE
MAX98357A
MAX98357B
R
B2 (1.4V typ)
GPIO
VSD_MODE
RIGHT MODE
100kI
±8%
B1 (0.77V typ)
LEFT/2 + RIGHT/2
MODE
B0 (0.16V typ)
Figure 4. SD_MODE Resistor Connected Using Open-Drain Driver
PROCESSOR
MAX98357A
VDDIO LEFT MODE
MAX98357B
B2 (1.4V typ)
GPIO
R VSD_MODE
RIGHT MODE
100kI
±8%
B1 (0.77V typ)
LEFT/2 + RIGHT/2
MODE
B0 (0.16V typ)
Figure 5. SD_MODE Resistor Connected Using Push-Pull Driver
Maxim Integrated 18


---

## Page 19


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
I2S and Left Justified Mode RSMALL and RLARGE are dependent on the voltage level
The MAX98357A follows standard I2S timing by allowing of VDDIO. See Table 6 for pullup resistor values.
a delay of one BCLK cycle after the LRCLK transition
TDM Mode
before the beginning of a new data word (Figure 6 and
TDM mode is automatically detected by monitoring the
Figure 7). The MAX98357B follows the left justified timing
short channel sync pulse on LRCLK. The frequency
specification by aligning the LRCLK transitions with the
detector circuit detects the bit depth. In TDM mode,
beginning of a new data word (Figure 8 and Figure 9).
the MAX98357A/MAX98357B has a fixed gain of 12dB.
LRCLK ONLY supports 8kHz, 16kHz, 32kHz, 44.1kHz,
GAIN_SLOT and SD_MODE are used to select to which
48kHz, 88.2kHz, and 96kHz frequencies. LRCLK clocks
of 8 channels of TDM data the parts respond. Table 7
at 11.025kHz, 12kHz, 22.05kHz and 24kHz are NOT
shows the connections for GAIN_SLOT and SD_MODE
supported. Do not remove LRCLK while BLCK is pres-
for channel selection. The MAX98357A data is valid on
ent. Removing LRCLK while BCLK is present can cause
the BCLK rising edge. The MAX98357B data is valid on
unexpected output behavior, including a large DC output
the BCLK falling edge.
voltage.
Figure 10, Figure 11, Figure 12, and Figure 13 show TDM
The digital audio interface output mode is chosen by the
operation, in which a frame-sync pulse is used for LRCLK.
voltage at SD_MODE. Table 5 shows how the available
In TDM mode, there must be 128 (16-bit mode) or 256 (32-
modes are selected. Trip point B0–B2 are shown the
bit mode) BCLK cycles per frame. In TDM mode, the ICs
Electrical Characteristics in the SD_MODE Comparator
only accept 16-bit or 32-bit formatted data and any of the
Trip Points section. Values for SD_MODE pullup resistors
8 TDM slots can be selected.
Table 7. TDM Mode Channel Selection
SD_MODE GAIN_SLOT CHANNEL BITS
Low X Off N/A
VDD GND 0 16/32
VDD VDD with 0I 1 16/32
VDD Float 2 16/32
VDD VDD with 100kI 3 16/32
VDD GND with 100kI 4 16/32
VDD through RLARGE GND 5 16/32
VDD through RLARGE Float 6 16/32
VDD through RLARGE VDD 7 16/32
Maxim Integrated 19


---

## Page 20


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
16 BITS/CHANNEL SD_MODE = VDD
LRCLK LEFT RIGHT LEFT
BCLK
DIN D15 D14 D13 D12 D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0 D15 D14 D13 D12 D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0 D15 D14
IGNORED
16 BITS/CHANNEL SD_MODE PULL UP THROUGH RSMALL (70K)
LRCLK LEFT RIGHT LEFT
BCLK
DIN D15 D14 D13 D12 D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0 D15 D14 D13 D12 D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0 D15 D14
IGNORED
16 BITS/CHANNEL SD_MODE PULL UP THROUGH RLARGE (300k)
LRCLK LEFT RIGHT LEFT
BCLK
DIN D15 D14 D13 D12 D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0 D15 D14 D13 D12 D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0 D15 D14
Figure 6. MAX98357A I2S Digital Audio Interface Timing, 16-Bit Resolution
Maxim Integrated 20


---

## Page 34


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
WLP Applications Information Ordering Information
For the latest application details on WLP construction,
dimensions, tape carrier information, PCB techniques,
TEMP PIN- TOP
bump-pad layout, and recommended reflow temperature PART
RANGE PACKAGE MARK
profile, as well as the latest information on reliability testing
results, refer to the Application Note 1891: Wafer-Level MAX98357AETE+ -40NC to 16 TQFN —
Packaging (WLP) and Its Applications. Figure 21 shows +85NC
the dimensions of the WLP balls used on the ICs. -40NC to
MAX98357AETE+T 16 TQFN —
+85NC
-40NC to
0.24mm MAX98357AEWL+T 9 WLP +AKM
+85NC
-40NC to
MAX98357AGTE/V+* 16 TQFN +AKV
+105NC
-40NC to
MAX98357BETE+ 16 TQFN —
+85NC
-40NC to
MAX98357BETE+T 16 TQFN —
+85NC
-40NC to
MAX98357BEWL+T 9 WLP +AKN
+85NC
+Denotes a lead(Pb)-free/RoHS-compliant package.
T = Tape and reel.
0.21mm /V denotes an automotive-qualified part.
*Future product—contact factory for availability.
Figure 21. MAX98357A/MAX98357B WLP Ball Dimensions
Maxim Integrated 34


---

## Page 35


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
Package Information
For the latest package outline information and land patterns (footprints), go to www.maximintegrated.com/packages. Note that a
“+”, “#”, or “-” in the package code indicates RoHS status only. Package drawings may show a different suffix character, but the
drawing pertains to the package regardless of RoHS status.
PACKAGE TYPE PACKAGE CODE OUTLINE NO. LAND PATTERN NO.
9 WLP W91F1+1 21-0896 Refer to Application Note 1891
16 TQFN T1633+4 21-0136 90-0031
0.05 S
maxim
integrated
TM
0.05 AB
TIE WANG 06/27/14
Maxim Integrated 35


---

## Page 36


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
Package Information (continued)
For the latest package outline information and land patterns (footprints), go to www.maximintegrated.com/packages. Note that a
“+”, “#”, or “-” in the package code indicates RoHS status only. Package drawings may show a different suffix character, but the
drawing pertains to the package regardless of RoHS status.
Maxim Integrated 36


---

## Page 37


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
Package Information (continued)
For the latest package outline information and land patterns (footprints), go to www.maximintegrated.com/packages. Note that a
“+”, “#”, or “-” in the package code indicates RoHS status only. Package drawings may show a different suffix character, but the
drawing pertains to the package regardless of RoHS status.
Maxim Integrated 37


---

## Page 38


MAX98357A/MAX98357B
PCM Input Class D Audio Power Amplifiers
Revision History
REVISION REVISION PAGES
DESCRIPTION
NUMBER DATE CHANGED
0 9/13 Initial release —
Added two new TOCs, replaced TOC 29, updated Figures 1–3, and made various 1, 4–20,
1 11/13
corrections 29–32, 34
2 8/14 Added THD+N for TQFN package with typical spec 5
3 1/15 Updated spread-spectrum bandwidth spec 5, 28
4 2/15 Added automotive-qualified part 34
5 6/15 Updated TOCs 30a and 30b 12
6 8/15 Corrected package outline for WLP package 36
7 2/16 Removed future product references 34
Maxim Integrated cannot assume responsibility for use of any circuitry other than circuitry entirely embodied in a Maxim Integrated product. No circuit patent
licenses are implied. Maxim Integrated reserves the right to change the circuitry and specifications without notice at any time. The parametric values (min and
max limits) shown in the Electrical Characteristics table are guaranteed. Other parametric values quoted in this data sheet are provided for guidance.
Maxim Integrated 160 Rio Robles, San Jose, CA 95134 USA 1-408-601-1000 38
© 2016 Maxim Integrated Products, Inc. Maxim Integrated and the Maxim Integrated logo are trademarks of Maxim Integrated Products, Inc.
