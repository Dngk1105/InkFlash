# ESP32-S3-WROOM-1_WROOM-1U_DATASHEET_EN - Hardware Technical Specification

> **Extracted via:** DeepSheet + Docling Pipeline  
> **Source Document:** esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf  

---

## Page 1


ESP32-S3-WROOM-1
ESP32-S3-WROOM-1U
Datasheet
Version 1.8
2.4 GHz Wi-Fi (802.11b/g/n) and Bluetooth® 5 module
Built around ESP32-S3 series of SoCs, Xtensa® dual-core 32-bit LX7 microprocessor
Flash up to 16 MB, PSRAM up to 16 MB
Up to 36 GPIOs, rich set of peripherals
On-board PCB antenna or external antenna connector
ESP32-S3-WROOM-1 ESP32-S3-WROOM-1U
www.espressif.com


---

## Page 2


1 ModuleOverview
1 Module Overview
Note:
CheckthelinkortheQRcodetomakesurethatyouusethelatestversionofthisdocument:
https://www.espressif.com/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf
1.1 Features
CPU and On-Chip Memory – 4strappingGPIOs
• SPI,LCDinterface,Camerainterface,UART,I2C,
• ESP32-S3seriesofSoCsembedded,Xtensa®
I2S,remotecontrol,pulsecounter,LEDPWM,
dual-core32-bitLX7microprocessor(with
full-speedUSB2.0OTG,USBSerial/JTAG
singleprecisionFPU),upto240MHz
controller,MCPWM,SD/MMChostcontroller,
• 384KBROM GDMA,TWAI® controller(compatiblewithISO
• 512KBSRAM 11898-1),ADC,touchsensor,temperature
sensor,timersandwatchdogs
• 16KBSRAMinRTC
• Upto16MBPSRAM Integrated Components on Module
• 40MHzcrystaloscillator
Wi-Fi
• Upto16MBQuadSPIflash
• 802.11b/g/n
Antenna Options
• Bitrate: 802.11nupto150Mbps
• ESP32-S3-WROOM-1: On-boardPCBantenna
• A-MPDUandA-MSDUaggregation
• ESP32-S3-WROOM-1U:Externalantennaviaa
• 0.4µsguardintervalsupport
connector
• Centerfrequencyrangeofoperatingchannel:
Operating Conditions
2412~2484MHz
• Operatingvoltage/Powersupply: 3.0~3.6V
Bluetooth
• Operatingambienttemperature:
• BluetoothLE:Bluetooth5,Bluetoothmesh
– 65°Cversion: –40~65 °C
• Speed: 125Kbps,500Kbps,1Mbps,2Mbps
– 85°Cversion: –40~85 °C
• Advertisingextensions
– 105°Cversion: –40~105 °C
• Multipleadvertisementsets
Certification
• Channelselectionalgorithm#2
• Internalco-existencemechanismbetweenWi-Fi • RFcertification: Seecertificates
andBluetoothtosharethesameantenna • Greencertification: RoHS/REACH
Peripherals Test
• 36GPIOs • HTOL/HTSL/uHAST/TCT/ESD
EspressifSystems 2 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 3


1 ModuleOverview
1.2 Series Comparison
ESP32-S3-WROOM-1andESP32-S3-WROOM-1Uaretwopowerful,genericWi-Fi+BluetoothLEMCUmodules
thatarebuiltaroundtheESP32-S3seriesofSoCs. Ontopofarichsetofperipherals,theaccelerationfor
neuralnetworkcomputingandsignalprocessingworkloadsprovidedbytheSoCmakethemodulesanideal
choiceforawidevarietyofapplicationscenariosrelatedtoAIandArtificialIntelligenceofThings(AIoT),such
aswakeworddetection,speechcommandsrecognition,facedetectionandrecognition,smarthome,smart
appliances,smartcontrolpanel,smartspeaker,etc.
ESP32-S3-WROOM-1comeswithaPCBantenna. ESP32-S3-WROOM-1Ucomeswithanexternalantenna
connector. AwideselectionofmodulevariantsareavailableforcustomersasshowninTable1-1and1-2. H4
seriesmodulesoperateat–40~105°Cambienttemperature,R8andR16Vseriesmodulesoperateat–40~
65°Cambienttemperature,andothermodulevariantsoperateat–40~85°Cambienttemperature. ForR8
andR16VseriesmoduleswithOctalSPIPSRAM,ifthePSRAMECCfunctionisenabled,themaximumambient
temperaturecanbeimprovedto85 °C,whiletheusablesizeofPSRAMwillbereducedby1/16.
1
Table1-1. ESP32-S3-WROOM-1SeriesComparison
5 6
AmbientTemp. Size
2 3,4 4
PartNumber Flash PSRAM
(°C) (mm)
ESP32-S3-WROOM-1-N4 4MB(QuadSPI) - –40~85
ESP32-S3-WROOM-1-N8 8MB(QuadSPI) - –40~85
ESP32-S3-WROOM-1-N16 16MB(QuadSPI) - –40~85
18.0
ESP32-S3-WROOM-1-H4 4MB(QuadSPI) - –40~105
×
ESP32-S3-WROOM-1-N4R2 4MB(QuadSPI) 2MB(QuadSPI) –40~85
25.5
ESP32-S3-WROOM-1-N8R2 8MB(QuadSPI) 2MB(QuadSPI) –40~85
×
ESP32-S3-WROOM-1-N16R2 16MB(QuadSPI) 2MB(QuadSPI) –40~85
3.1
ESP32-S3-WROOM-1-N4R8 4MB(QuadSPI) 8MB(OctalSPI) –40~65
ESP32-S3-WROOM-1-N8R8 8MB(QuadSPI) 8MB(OctalSPI) –40~65
ESP32-S3-WROOM-1-N16R8 16MB(QuadSPI) 8MB(OctalSPI) –40~65
7
ESP32-S3-WROOM-1-N16R16VA 16MB(QuadSPI) 16MB(OctalSPI) –40~65
1
ThistablesharesthesamenotespresentedinTable1-2below.
Table1-2. ESP32-S3-WROOM-1USeriesComparison
5 6
AmbientTemp. Size
2 3,4 4
PartNumber Flash PSRAM
(°C) (mm)
ESP32-S3-WROOM-1U-N4 4MB(QuadSPI) - –40~85
ESP32-S3-WROOM-1U-N8 8MB(QuadSPI) - –40~85
ESP32-S3-WROOM-1U-N16 16MB(QuadSPI) - –40~85
18.0
ESP32-S3-WROOM-1U-H4 4MB(QuadSPI) - –40~105
×
ESP32-S3-WROOM-1U-N4R2 4MB(QuadSPI) 2MB(QuadSPI) –40~85
19.2
ESP32-S3-WROOM-1U-N8R2 8MB(QuadSPI) 2MB(QuadSPI) –40~85
×
ESP32-S3-WROOM-1U-N16R2 16MB(QuadSPI) 2MB(QuadSPI) –40~85
3.2
ESP32-S3-WROOM-1U-N4R8 4MB(QuadSPI) 8MB(OctalSPI) –40~65
ESP32-S3-WROOM-1U-N8R8 8MB(QuadSPI) 8MB(OctalSPI) –40~65
ESP32-S3-WROOM-1U-N16R8 16MB(QuadSPI) 8MB(OctalSPI) –40~65
7
ESP32-S3-WROOM-1U-N16R16VA 16MB(QuadSPI) 16MB(OctalSPI) –40~65
EspressifSystems 3 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 5


Contents
Contents
1 Module Overview
2
1.1 Features 2
1.2 SeriesComparison 3
1.3 Applications 4
2 Block Diagram
9
3 Pin Definitions
10
3.1 PinLayout 10
3.2 PinDescription 11
4 Boot Configurations
13
4.1 ChipBootModeControl 14
4.2 VDD_SPIVoltageControl 15
4.3 ROMMessagesPrintingControl 15
4.4 JTAGSignalSourceControl 15
4.5 ChipPower-upandReset 16
5 Peripherals
17
5.1 PeripheralOverview 17
5.2 PeripheralDescription 17
5.2.1 ConnectivityInterface 17
5.2.1.1 UARTController 17
5.2.1.2 I2CInterface 18
5.2.1.3 I2SInterface 18
5.2.1.4 LCDandCameraController 19
5.2.1.5 SerialPeripheralInterface(SPI) 19
5.2.1.6 Two-WireAutomotiveInterface(TWAI®) 21
5.2.1.7 USB2.0OTGFull-SpeedInterface 21
5.2.1.8 USBSerial/JTAGController 22
5.2.1.9 SD/MMCHostController 23
5.2.1.10 MotorControlPWM(MCPWM) 24
5.2.1.11 RemoteControlPeripheral(RMT) 24
5.2.1.12 PulseCountController(PCNT) 25
5.2.2 AnalogSignalProcessing 25
5.2.2.1 SARADC 25
5.2.2.2 TemperatureSensor 25
5.2.2.3 TouchSensor 26
6 Electrical Characteristics
27
6.1 AbsoluteMaximumRatings 27
6.2 RecommendedOperatingConditions 27
6.3 DCCharacteristics(3.3V,25 °C) 27
EspressifSystems 5 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 6


Contents
6.4 CurrentConsumptionCharacteristics 28
6.4.1 CurrentConsumptioninActiveMode 28
6.4.2 CurrentConsumptioninOtherModes 29
6.5 MemorySpecifications 30
7 RF Characteristics
32
7.1 Wi-FiRadio 32
7.1.1 Wi-FiRFTransmitter(TX)Characteristics 32
7.1.2 Wi-FiRFReceiver(RX)Characteristics 33
7.2 BluetoothLERadio 34
7.2.1 BluetoothLERFTransmitter(TX)Characteristics 35
7.2.2 BluetoothLERFReceiver(RX)Characteristics 36
8 Module Schematics
39
9 Peripheral Schematics
41
10 Physical Dimensions
42
10.1 ModuleDimensions 42
10.2 DimensionsofExternalAntennaConnector 43
11 PCB Layout Recommendations
45
11.1 PCBLandPattern 45
11.2 ModulePlacementforPCBDesign 46
12 Product Handling
47
12.1 StorageConditions 47
12.2 ElectrostaticDischarge(ESD) 47
12.3 ReflowProfile 47
12.4 UltrasonicVibration 48
Datasheet Versioning
49
Related Documentation and Resources
50
Revision History
51
EspressifSystems 6 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 7


ListofTables
List of Tables
1
1-1 ESP32-S3-WROOM-1SeriesComparison 3
1-2 ESP32-S3-WROOM-1USeriesComparison 3
3-1 PinDefinitions 11
4-1 DefaultConfigurationofStrappingPins 13
4-2 DescriptionofTimingParametersfortheStrappingPins 14
4-3 ChipBootModeControl 14
4-4 VDD_SPIVoltageControl 15
4-5 JTAGSignalSourceControl 16
4-6 DescriptionofTimingParametersforPower-upandReset 16
6-1 AbsoluteMaximumRatings 27
6-2 RecommendedOperatingConditions 27
6-3 DCCharacteristics(3.3V,25 °C) 27
6-4 CurrentConsumptionforWi-Fi(2.4GHz)inActiveMode 28
6-5 CurrentConsumptionforBluetoothLEinActiveMode 28
6-6 CurrentConsumptioninModem-sleepMode 29
6-7 CurrentConsumptioninLow-PowerModes 30
6-8 FlashSpecifications 30
6-9 PSRAMSpecifications 31
7-1 Wi-FiRFCharacteristics 32
7-2 TXPowerwithSpectralMaskandEVMMeeting802.11Standards 32
1
7-3 TXEVMTest 32
7-4 RXSensitivity 33
7-5 MaximumRXLevel 34
7-6 RXAdjacentChannelRejection 34
7-7 BluetoothLERFCharacteristics 34
7-8 BluetoothLE-TransmitterCharacteristics-1Mbps 35
7-9 BluetoothLE-TransmitterCharacteristics-2Mbps 35
7-10 BluetoothLE-TransmitterCharacteristics-125Kbps 35
7-11 BluetoothLE-TransmitterCharacteristics-500Kbps 36
7-12 BluetoothLE-ReceiverCharacteristics-1Mbps 36
7-13 BluetoothLE-ReceiverCharacteristics-2Mbps 36
7-14 BluetoothLE-ReceiverCharacteristics-125Kbps 37
7-15 BluetoothLE-ReceiverCharacteristics-500Kbps 37
EspressifSystems 7 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 8


ListofFigures
List of Figures
2-1 ESP32-S3-WROOM-1BlockDiagram 9
2-2 ESP32-S3-WROOM-1UBlockDiagram 9
3-1 PinLayout(TopView) 10
4-1 VisualizationofTimingParametersfortheStrappingPins 14
4-2 VisualizationofTimingParametersforPower-upandReset 16
8-1 ESP32-S3-WROOM-1Schematics 39
8-2 ESP32-S3-WROOM-1USchematics 40
9-1 PeripheralSchematics 41
10-1 ESP32-S3-WROOM-1PhysicalDimensions 42
10-2 ESP32-S3-WROOM-1UPhysicalDimensions 42
10-3 DimensionsofExternalAntennaConnector 43
11-1 ESP32-S3-WROOM-1RecommendedPCBLandPattern 45
11-2 ESP32-S3-WROOM-1URecommendedPCBLandPattern 46
12-1 ReflowProfile 47
EspressifSystems 8 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 11


3 PinDefinitions
3.2 Pin Description
Themodulehas41pins. SeepindefinitionsinTable3-1PinDefinitions.
Forexplanationsofpinnamesandfunctionnames,aswellasconfigurationsofperipheralpins,pleasereferto
ESP32-S3SeriesDatasheet.
Table3-1. PinDefinitions
a
Name No. Type Function
GND 1 P GND
3V3 2 P Powersupply
High: on,enablesthechip.
EN 3 I Low: off,thechippowersoff.
Note: DonotleavetheENpinfloating.
IO4 4 I/O/T RTC_GPIO4,GPIO4,TOUCH4,ADC1_CH3
IO5 5 I/O/T RTC_GPIO5,GPIO5,TOUCH5,ADC1_CH4
IO6 6 I/O/T RTC_GPIO6,GPIO6,TOUCH6,ADC1_CH5
IO7 7 I/O/T RTC_GPIO7,GPIO7,TOUCH7,ADC1_CH6
IO15 8 I/O/T RTC_GPIO15,GPIO15,U0RTS,ADC2_CH4,XTAL_32K_P
IO16 9 I/O/T RTC_GPIO16,GPIO16,U0CTS,ADC2_CH5,XTAL_32K_N
IO17 10 I/O/T RTC_GPIO17,GPIO17,U1TXD,ADC2_CH6
IO18 11 I/O/T RTC_GPIO18,GPIO18,U1RXD,ADC2_CH7,CLK_OUT3
IO8 12 I/O/T RTC_GPIO8,GPIO8,TOUCH8,ADC1_CH7,SUBSPICS1
IO19 13 I/O/T RTC_GPIO19,GPIO19,U1RTS,ADC2_CH8,CLK_OUT2,USB_D-
IO20 14 I/O/T RTC_GPIO20,GPIO20,U1CTS,ADC2_CH9,CLK_OUT1,USB_D+
IO3 15 I/O/T RTC_GPIO3,GPIO3,TOUCH3,ADC1_CH2
IO46 16 I/O/T GPIO46
IO9 17 I/O/T RTC_GPIO9,GPIO9,TOUCH9,ADC1_CH8,FSPIHD,SUBSPIHD
RTC_GPIO10,GPIO10,TOUCH10,ADC1_CH9,FSPICS0,FSPIIO4,
IO10 18 I/O/T
SUBSPICS0
IO11 19 I/O/T RTC_GPIO11,GPIO11,TOUCH11,ADC2_CH0,FSPID,FSPIIO5,SUBSPID
RTC_GPIO12,GPIO12,TOUCH12,ADC2_CH1,FSPICLK,FSPIIO6,
IO12 20 I/O/T
SUBSPICLK
IO13 21 I/O/T RTC_GPIO13,GPIO13,TOUCH13,ADC2_CH2,FSPIQ,FSPIIO7,SUBSPIQ
RTC_GPIO14,GPIO14,TOUCH14,ADC2_CH3,FSPIWP,FSPIDQS,
IO14 22 I/O/T
SUBSPIWP
IO21 23 I/O/T RTC_GPIO21,GPIO21
c
IO47 24 I/O/T SPICLK_P_DIFF,GPIO47,SUBSPICLK_P_DIFF
c
IO48 25 I/O/T SPICLK_N_DIFF,GPIO48,SUBSPICLK_N_DIFF
IO45 26 I/O/T GPIO45
IO0 27 I/O/T RTC_GPIO0,GPIO0
b
IO35 28 I/O/T SPIIO6,GPIO35,FSPID,SUBSPID
b
IO36 29 I/O/T SPIIO7,GPIO36,FSPICLK,SUBSPICLK
b
IO37 30 I/O/T SPIDQS,GPIO37,FSPIQ,SUBSPIQ
IO38 31 I/O/T GPIO38,FSPIWP,SUBSPIWP
Cont’donnextpage
EspressifSystems 11 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 12


3 PinDefinitions
Table3-1–cont’dfrompreviouspage
a
Name No. Type Function
IO39 32 I/O/T MTCK,GPIO39,CLK_OUT3,SUBSPICS1
IO40 33 I/O/T MTDO,GPIO40,CLK_OUT2
IO41 34 I/O/T MTDI,GPIO41,CLK_OUT1
IO42 35 I/O/T MTMS,GPIO42
RXD0 36 I/O/T U0RXD,GPIO44,CLK_OUT2
TXD0 37 I/O/T U0TXD,GPIO43,CLK_OUT1
IO2 38 I/O/T RTC_GPIO2,GPIO2,TOUCH2,ADC1_CH1
IO1 39 I/O/T RTC_GPIO1,GPIO1,TOUCH1,ADC1_CH0
GND 40 P GND
EPAD 41 P GND
a
P:powersupply;I:input;O:output;T:highimpedance. Pinfunctionsinboldfontarethedefault
pinfunctions. Forpin28∼30,thedefaultfunctionisdecidedbyeFusebit.
b
FormoduleswithOctalSPIPSRAM,i.e.,modulesembeddedwithESP32-S3R8orESP32-S3R16V,
pinsIO35,IO36,andIO37areconnectedtotheOctalSPIPSRAMandarenotavailableforother
uses.
c
FormodulesembeddedwithESP32-S3R16V,astheVDD_SPIvoltageoftheESP32-S3R16Vchip
is set to 1.8 V, the working voltage for GPIO47 and GPIO48 is also 1.8 V, which is different from
otherGPIOs.
EspressifSystems 12 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 15


4 BootConfigurations
InSPIBootmode,theROMbootloaderloadsandexecutestheprogramfromSPIflashtobootthe
system.
InJointDownloadBootmode,userscandownloadbinaryfilesintoflashusingUART0orUSBinterface. Itis
alsopossibletodownloadbinaryfilesintoSRAMandexecuteitfromSRAM.
InadditiontoSPIBootandJointDownloadBootmodes,ESP32-S3alsosupportsSPIDownloadBootmode.
Fordetails,pleaseseeESP32-S3TechnicalReferenceManual>ChapterChipBootControl.
4.2 VDD_SPI Voltage Control
DependingonthevalueofEFUSE_VDD_SPI_FORCE,thevoltagecanbecontrolledintwoways.
Table4-4. VDD_SPIVoltageControl
2
VDD_SPIpowersource Voltage EFUSE_VDD_SPI_FORCE GPIO45 EFUSE_VDD_SPI_TIEH
VDD3P3_RTCviaR 3.3V 0
SPI
0 Ignored
FlashVoltageRegulator 1.8V 1
FlashVoltageRegulator 1.8V 0
1 Ignored
VDD3P3_RTCviaR 3.3V 1
SPI
1
Boldmarksthedefaultvalueandconfiguration.
2
SeeESP32-S3SeriesDatasheet>SectionPowerScheme.
4.3 ROM Messages Printing Control
DuringbootprocessthemessagesbytheROMcodecanbeprintedto:
• (Default)UART0andUSBSerial/JTAGcontroller
• USBSerial/JTAGcontroller
• UART0
TheROMmessagesprintingtoUARTorUSBSerial/JTAGcontrollercanberespectivelydisabledbyconfiguring
registersandeFuse. Fordetailedinformation,pleaserefertoESP32-S3TechnicalReferenceManual>
ChapterChipBootControl.
4.4 JTAG Signal Source Control
ThestrappingpinGPIO3canbeusedtocontrolthesourceofJTAGsignalsduringtheearlybootprocess. This
pindoesnothaveanyinternalpullresistorsandthestrappingvaluemustbecontrolledbytheexternalcircuit
thatcannotbeinahighimpedancestate.
AsTable4-5shows,GPIO3isusedincombinationwithEFUSE_DIS_PAD_JTAG,EFUSE_DIS_USB_JTAG,and
EFUSE_STRAP_JTAG_SEL.
EspressifSystems 15 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 16


4 BootConfigurations
Table4-5. JTAGSignalSourceControl
JTAGSignalSource EFUSE_DIS_PAD_JTAG EFUSE_DIS_USB_JTAG EFUSE_STRAP_JTAG_SEL GPIO3
0 0 0 Ignored
USBSerial/JTAGController 0 0 1 1
1 0 Ignored Ignored
0 0 1 0
2
JTAGpins
0 1 Ignored Ignored
JTAGisdisabled 1 1 Ignored Ignored
1
Boldmarksthedefaultvalueandconfiguration.
2
JTAGpinsrefertoMTDI,MTCK,MTMS,andMTDO.
4.5 Chip Power-up and Reset
Oncethepowerissuppliedtothechip,itspowerrailsneedashorttimetostabilize. Afterthat,EN–thepin
usedforpower-upandreset–ispulledhightoactivatethechip. ForinformationonENaswellaspower-up
andresettiming,seeFigure4-2andTable4-6.
tSTBL tRST
2.8V
VDDA,
VDD3P3,
VDD3P3_RTC,
VDD3P3_CPU
V
IL_nRST
EN
Figure4-2. VisualizationofTimingParametersforPower-upandReset
Table4-6. DescriptionofTimingParametersforPower-upandReset
Parameter Description Min(µs)
Time reserved for the power rails of VDDA, VDD3P3,
t VDD3P3_RTC, and VDD3P3_CPU to stabilize before the EN 50
STBL
pinispulledhightoactivatethechip
Time reserved for EN to stay below V to reset the chip
IL_nRST
t 50
RST
(seeTable6-3)
EspressifSystems 16 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 18


5 Peripherals
• SpecialcharacterAT_CMDdetection
• RS485protocol
• IrDAprotocol
• High-speeddatacommunicationusingGDMA
• UARTaswake-upsource
• Softwareandhardwareflowcontrol
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterUARTController.
PinAssignment
Fordetails,seeESP32-S3SeriesDatasheet>SectionPeripheralPinAssignment.
5.2.1.2 I2C Interface
ESP32-S3hastwoI2CbusinterfaceswhichareusedforI2Cmastermodeorslavemode,dependingonthe
user’sconfiguration.
FeatureList
• Standardmode(100kbit/s)
• Fastmode(400kbit/s)
• Upto800kbit/s(constrainedbySCLandSDApull-upstrength)
• 7-bitand10-bitaddressingmode
• Doubleaddressingmode(slaveaddressingandslaveregisteraddressing)
ThehardwareprovidesacommandabstractionlayertosimplifytheusageoftheI2Cperipheral.
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterI2CController.
PinAssignment
Fordetails,seeESP32-S3SeriesDatasheet>SectionPeripheralPinAssignment.
5.2.1.3 I2S Interface
ESP32-S3includestwostandardI2Sinterfaces. Theycanoperateinmastermodeorslavemode,in
full-duplexmodeorhalf-duplexcommunicationmode,andcanbeconfiguredtooperatewithan8-bit,16-bit,
24-bit,or32-bitresolutionasaninputoroutputchannel. BCKclockfrequency,from10kHzupto40MHz,is
supported.
TheI2SinterfacehasadedicatedDMAcontroller. ItsupportsTDMPCM,TDMMSBalignment,TDMLSB
alignment,TDMPhillips,andPDMinterface.
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterI2SController.
EspressifSystems 18 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 19


5 Peripherals
PinAssignment
Fordetails,seeESP32-S3SeriesDatasheet>SectionPeripheralPinAssignment.
5.2.1.4 LCD and Camera Controller
TheLCDandCameracontrollerofESP32-S3consistsofaLCDmoduleandacameramodule.
TheLCDmoduleisdesignedtosendparallelvideodatasignals,anditsbussupports8-bit~16-bitparallel
RGB,I8080,andMOTO6800interfaces. Theseinterfacesoperateat40MHzorlower,andsupportconversion
amongRGB565,YUV422,YUV420,andYUV411.
Thecameramoduleisdesignedtoreceiveparallelvideodatasignals,anditsbussupportsan8-bit~16-bit
DVPimagesensor,withclockfrequencyofupto40MHz. Thecamerainterfacesupportsconversionamong
RGB565,YUV422,YUV420,andYUV411.
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterLCDandCameraController.
PinAssignment
Fordetails,seeESP32-S3SeriesDatasheet>SectionPeripheralPinAssignment.
5.2.1.5 Serial Peripheral Interface (SPI)
ESP32-S3hasthefollowingSPIinterfaces:
• SPI0usedbyESP32-S3’sGDMAcontrollerandcachetoaccessin-packageoroff-packageflash/PSRAM
• SPI1usedbytheCPUtoaccessin-packageoroff-packageflash/PSRAM
• SPI2isageneralpurposeSPIcontrollerwithaccesstoaDMAchannelallocatedbytheGDMAcontroller
• SPI3isageneralpurposeSPIcontrollerwithaccesstoaDMAchannelallocatedbytheGDMAcontroller
FeatureList
• SPI0andSPI1:
– SupportsSingleSPI,DualSPI,QuadSPI,OctalSPI,QPI,andOPImodes
– 8-lineSPImodesupportssingledatarate(SDR)anddoubledatarate(DDR)
– Configurableclockfrequencywithamaximumof120MHzfor8-lineSPISDR/DDRmodes
– Datatransmissionisinbytes
• SPI2:
– Supportsoperationasamasterorslave
– ConnectstoaDMAchannelallocatedbytheGDMAcontroller
– SupportsSingleSPI,DualSPI,QuadSPI,OctalSPI,QPI,andOPImodes
– Configurableclockpolarity(CPOL)andphase(CPHA)
– Configurableclockfrequency
– Datatransmissionisinbytes
EspressifSystems 19 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 20


5 Peripherals
– Configurablereadandwritedatabitorder: most-significantbit(MSB)first,orleast-significantbit
(LSB)first
– Asamaster
* Supports2-linefull-duplexcommunicationwithclockfrequencyupto80MHz
* Full-duplex8-lineSPImodesupportssingledatarate(SDR)only
* Supports1-,2-,4-,8-linehalf-duplexcommunicationwithclockfrequencyupto80MHz
* Half-duplex8-lineSPImodesupportsbothsingledatarate(upto80MHz)anddoubledatarate
(upto40MHz)
* ProvidessixSPI_CSpinsforconnectionwithsixindependentSPIslaves
* ConfigurableCSsetuptimeandholdtime
– Asaslave
* Supports2-linefull-duplexcommunicationwithclockfrequencyupto60MHz
* Supports1-,2-,4-linehalf-duplexcommunicationwithclockfrequencyupto60MHz
* Full-duplexandhalf-duplex8-lineSPImodesupportssingledatarate(SDR)only
• SPI3:
– Supportsoperationasamasterorslave
– ConnectstoaDMAchannelallocatedbytheGDMAcontroller
– SupportsSingleSPI,DualSPI,QuadSPI,andQPImodes
– Configurableclockpolarity(CPOL)andphase(CPHA)
– Configurableclockfrequency
– Datatransmissionisinbytes
– Configurablereadandwritedatabitorder: most-significantbit(MSB)first,orleast-significantbit
(LSB)first
– Asamaster
* Supports2-linefull-duplexcommunicationwithclockfrequencyupto80MHz
* Supports1-,2-,4-linehalf-duplexcommunicationwithclockfrequencyupto80MHz
* ProvidesthreeSPI_CSpinsforconnectionwiththreeindependentSPIslaves
* ConfigurableCSsetuptimeandholdtime
– Asaslave
* Supports2-linefull-duplexcommunicationwithclockfrequencyupto60MHz
* Supports1-,2-,4-linehalf-duplexcommunicationwithclockfrequencyupto60MHz
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterSPIController.
PinAssignment
Fordetails,seeESP32-S3SeriesDatasheet>SectionPeripheralPinAssignment.
EspressifSystems 20 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 21


5 Peripherals
5.2.1.6 Two-Wire Automotive Interface (TWAI®)
TheTwo-WireAutomotiveInterface(TWAI®)isamulti-master,multi-castcommunicationprotocolwitherror
detectionandsignalingaswellasinbuiltmessageprioritiesandarbitration.
FeatureList
• CompatiblewithISO11898-1protocol(CANSpecification2.0)
• Standardframeformat(11-bitID)andextendedframeformat(29-bitID)
• Bitratesfrom1Kbit/sto1Mbit/s
• Multiplemodesofoperation:
– Normal
– ListenOnly
– Self-Test(noacknowledgmentrequired)
• 64-bytereceiveFIFO
• Acceptancefilter(singleanddualfiltermodes)
• Errordetectionandhandling:
– Errorcounters
– Configurableerrorinterruptthreshold
– Errorcodecapture
– Arbitrationlostcapture
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterTwo-wireAutomotiveInterface.
PinAssignment
Fordetails,seeESP32-S3SeriesDatasheet>SectionPeripheralPinAssignment.
5.2.1.7 USB 2.0 OTG Full-Speed Interface
ESP32-S3featuresafull-speedUSBOTGinterfacealongwithanintegratedtransceiver. TheUSBOTG
interfacecomplieswiththeUSB2.0specification.
GeneralFeatures
• FSandLSdatarates
• HNPandSRPasA-deviceorB-device
• DynamicFIFO(DFIFO)sizing
• Multiplemodesofmemoryaccess
– Scatter/GatherDMAmode
– BufferDMAmode
EspressifSystems 21 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 22


5 Peripherals
– Slavemode
• Canchooseintegratedtransceiverorexternaltransceiver
• UtilizingintegratedtransceiverwithUSBSerial/JTAGbytime-divisionmultiplexingwhenonlyintegrated
transceiverisused
• SupportUSBOTGusingoneofthetransceiverswhileUSBSerial/JTAGusingtheotheronewhenboth
integratedtransceiverorexternaltransceiverareused
DeviceModeFeatures
• Endpointnumber0alwayspresent(bi-directional,consistingofEP0INandEP0OUT)
• Sixadditionalendpoints(endpointnumbers1to6),configurableasINorOUT
• MaximumoffiveINendpointsconcurrentlyactiveatanytime(includingEP0IN)
• AllOUTendpointsshareasingleRXFIFO
• EachINendpointhasadedicatedTXFIFO
HostModeFeatures
• Eightchannels(pipes)
– Acontrolpipeconsistsoftwochannels(INandOUT),asINandOUTtransactionsmustbehandled
separately. OnlyControltransfertypeissupported.
– EachoftheothersevenchannelsisdynamicallyconfigurabletobeINorOUT,andsupportsBulk,
Isochronous,andInterrupttransfertypes.
• AllchannelsshareanRXFIFO,non-periodicTXFIFO,andperiodicTXFIFO.ThesizeofeachFIFOis
configurable.
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterUSBOn-The-Go.
PinAssignment
Fordetails,seeESP32-S3SeriesDatasheet>SectionPeripheralPinAssignment.
5.2.1.8 USB Serial/JTAG Controller
ESP32-S3integratesaUSBSerial/JTAGcontroller.
FeatureList
• USBFull-speeddevice.
• CanbeconfiguredtoeitheruseinternalUSBPHYofESP32-S3orexternalPHYviaGPIOmatrix.
• Fixedfunctiondevice,hardwiredforCDC-ACM(CommunicationDeviceClass-AbstractControlModel)
andJTAGadapterfunctionality.
• TwoOUTEndpoints,threeINEndpointsinadditiontoControlEndpoint0;Upto64-bytedatapayload
size.
EspressifSystems 22 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 23


5 Peripherals
• InternalPHY,sonoorveryfewexternalcomponentsneededtoconnecttoahostcomputer.
• CDC-ACMadherentserialportemulationisplug-and-playonmostmodernOSes.
• JTAGinterfaceallowsfastcommunicationwithCPUdebugcoreusingacompactrepresentationofJTAG
instructions.
• CDC-ACMsupportshostcontrollablechipresetandentryintodownloadmode.
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterUSBSerial/JTAGController.
PinAssignment
Fordetails,seeESP32-S3SeriesDatasheet>SectionPeripheralPinAssignment.
5.2.1.9 SD/MMC Host Controller
ESP32-S3hasanSD/MMCHostcontroller.
FeatureList
• SecureDigital(SD)memoryversion3.0andversion3.01
• SecureDigitalI/O(SDIO)version3.0
• ConsumerElectronicsAdvancedTransportArchitecture(CE-ATA)version1.1
• MultimediaCards(MMCversion4.41,eMMCversion4.5andversion4.51)
• Upto80MHzclockoutput
• Threedatabusmodes:
– 1-bit
– 4-bit(supportstwoSD/SDIO/MMC4.41cards,andoneSDcardoperatingat1.8Vin4-bitmode)
– 8-bit
Note:
Whenworkingat80MHz,theclockphaseadjustmentislimitedandonlyphase0°and180°aresupported. ThePCB
layoutshouldbeoptimizedaccordinglytoensuretimingclosure.
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterSD/MMCHostController.
PinAssignment
Fordetails,seeESP32-S3SeriesDatasheet>SectionPeripheralPinAssignment.
FeatureList
• Cangenerateadigitalwaveformwithconfigurableperiodsanddutycycle. Thedutycycleresolutioncan
beupto14bitswithina1msperiod
• Multipleclocksources,includingAPBclockandexternalmaincrystalclock
• CanoperatewhentheCPUisinLight-sleepmode
EspressifSystems 23 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 24


5 Peripherals
• Gradualincreaseordecreaseofdutycycle,usefulfortheLEDRGBcolor-fadinggenerator
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterLEDPWMController.
PinAssignment
Fordetails,seeESP32-S3SeriesDatasheet>SectionPeripheralPinAssignment.
5.2.1.10 Motor Control PWM (MCPWM)
ESP32-S3integratestwoMCPWMsthatcanbeusedtodrivedigitalmotorsandsmartlight. EachMCPWM
peripheralhasoneclockdivider(prescaler),threePWMtimers,threePWMoperators,andacapturemodule.
PWMtimersareusedforgeneratingtimingreferences. ThePWMoperatorsgeneratedesiredwaveformbased
onthetimingreferences. AnyPWMoperatorcanbeconfiguredtousethetimingreferencesofanyPWM
timers. DifferentPWMoperatorscanusethesamePWMtimer’stimingreferencestoproducerelatedPWM
signals. PWMoperatorscanalsousedifferentPWMtimers’valuestoproducethePWMsignalsthatwork
alone. DifferentPWMtimerscanalsobesynchronizedtogether.
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterMotorControlPWM.
PinAssignment
Fordetails,seeESP32-S3SeriesDatasheet>SectionPeripheralPinAssignment.
5.2.1.11 Remote Control Peripheral (RMT)
TheRemoteControlPeripheral(RMT)isdesignedtosendandreceiveinfraredremotecontrolsignals.
FeatureList
• FourTXchannels
• FourRXchannels
• Supportmultiplechannels(programmable)transmittingdatasimultaneously
• Eightchannelssharea384x32-bitRAM
• SupportmodulationonTXpulses
• SupportfilteringanddemodulationonRXpulses
• WrapTXmode
• WrapRXmode
• ContinuousTXmode
• DMAaccessforTXmodeonchannel3
• DMAaccessforRXmodeonchannel7
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterRemoteControlPeripheral.
PinAssignment
Fordetails,seeESP32-S3SeriesDatasheet>SectionPeripheralPinAssignment.
EspressifSystems 24 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 25


5 Peripherals
5.2.1.12 Pulse Count Controller (PCNT)
Thepulsecountcontroller(PCNT)capturespulseandcountspulseedgesthroughmultiplemodes.
FeatureList
• Fourindependentpulsecounters(units)thatcountfrom1to65535
• Eachunitconsistsoftwoindependentchannelssharingonepulsecounter
• Allchannelshaveinputpulsesignals(e.g. sig_ch0_un)withtheircorrespondingcontrolsignals(e.g.
ctrl_ch0_un)
• Independentlyfilterglitchesofinputpulsesignals(sig_ch0_unandsig_ch1_un)andcontrolsignals
(ctrl_ch0_unandctrl_ch1_un)oneachunit
• Eachchannelhasthefollowingparameters:
1. Selectionbetweencountingonpositiveornegativeedgesoftheinputpulsesignal
2. ConfigurationtoIncrement,Decrement,orDisablecountermodeforcontrolsignal’shighandlow
states
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterPulseCountController.
PinAssignment
Fordetails,seeESP32-S3SeriesDatasheet>SectionPeripheralPinAssignment.
5.2.2 Analog Signal Processing
Thissubsectiondescribescomponentsonthechipthatsenseandprocessreal-worlddata.
5.2.2.1 SAR ADC
ESP32-S3integratestwo12-bitSARADCsandsupportsmeasurementson20channels(analog-enabledpins).
Forpower-savingpurpose,theULPcoprocessorsinESP32-S3canalsobeusedtomeasurevoltageinsleep
modes. Byusingthresholdsettingsorothermethods,wecanawakentheCPUfromsleepmodes.
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterOn-ChipSensorsandAnalogSignal
Processing.
PinAssignment
Fordetails,seeESP32-S3SeriesDatasheet>SectionPeripheralPinAssignment.
5.2.2.2 Temperature Sensor
Thetemperaturesensorgeneratesavoltagethatvarieswithtemperature. Thevoltageisinternallyconverted
viaanADCintoadigitalvalue.
Thetemperaturesensorhasarangeof–40°Cto125 °C.Itisdesignedprimarilytosensethetemperature
changesinsidethechip. Thetemperaturevaluedependsonfactorssuchasmicrocontrollerclockfrequency
orI/Oload. Generally,thechip’sinternaltemperatureishigherthantheambienttemperature.
EspressifSystems 25 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 26


5 Peripherals
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterOn-ChipSensorsandAnalogSignal
Processing.
5.2.2.3 Touch Sensor
ESP32-S3has14capacitive-sensingGPIOs,whichdetectvariationsinducedbytouchingorapproachingthe
GPIOswithafingerorotherobjects. Thelow-noisenatureofthedesignandthehighsensitivityofthecircuit
allowrelativelysmallpadstobeused. Arraysofpadscanalsobeused,sothatalargerareaormorepoints
canbedetected. Thetouchsensingperformancecanbefurtherenhancedbythewaterproofdesignand
digitalfilteringfeature.
Note:
ESP32-S3touchsensorhasnotpassedtheConductedSusceptibility(CS)testfornow,andthushaslimitedapplication
scenarios.
Fordetails,seeESP32-S3TechnicalReferenceManual>ChapterOn-ChipSensorsandAnalogSignal
Processing.
PinAssignment
Fordetails,seeESP32-S3SeriesDatasheet>SectionPeripheralPinAssignment.
EspressifSystems 26 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 27


5 Peripherals
6 Electrical Characteristics
6.1 Absolute Maximum Ratings
StressesabovethoselistedinTable6-1AbsoluteMaximumRatingsmaycausepermanentdamagetothe
device. Thesearestressratingsonlyandfunctionaloperationofthedeviceattheseoranyotherconditions
beyondthoseindicatedunderTable6-2RecommendedOperatingConditionsisnotimplied. Exposureto
absolute-maximum-ratedconditionsforextendedperiodsmayaffectdevicereliability.
Table6-1. AbsoluteMaximumRatings
Symbol Parameter Min Max Unit
VDD33 Powersupplyvoltage –0.3 3.6 V
T Storagetemperature –40 105 °C
STORE
6.2 Recommended Operating Conditions
Table6-2. RecommendedOperatingConditions
Symbol Parameter Min Typ Max Unit
VDD33 Powersupplyvoltage 3.0 3.3 3.6 V
I Currentdeliveredbyexternalpowersupply 0.5 — — A
VDD
65°Cversion 65
T Operatingambienttemperature 85°Cversion –40 — 85 °C
A
105°Cversion 105
6.3 DC Characteristics (3.3 V, 25 °C)
Table6-3. DCCharacteristics(3.3V,25 °C)
Parameter Description Min Typ Max Unit
C Pincapacitance — 2 — pF
IN
1 1
V High-levelinputvoltage 0.75×VDD — VDD +0.3 V
IH
1
V Low-levelinputvoltage –0.3 — 0.25×VDD V
IL
I High-levelinputcurrent — — 50 nA
IH
I Low-levelinputcurrent — — 50 nA
IL
2 1
V High-leveloutputvoltage 0.8×VDD — — V
OH
2 1
V Low-leveloutputvoltage — — 0.1×VDD V
OL
1
High-levelsourcecurrent(VDD =3.3V,
I — 40 — mA
OH
V >=2.64V,PAD_DRIVER=3)
OH
1
Low-levelsinkcurrent(VDD =3.3V,V =
OL
I — 28 — mA
OL
0.495V,PAD_DRIVER=3)
R Internalweakpull-upresistor — 45 — kΩ
PU
R Internalweakpull-downresistor — 45 — kΩ
PD
EspressifSystems 27 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 28


5 Peripherals
Chipresetreleasevoltage(ENvoltageis
1 1
V 0.75×VDD — VDD +0.3 V
IH_nRST
withinthespecifiedrange)
Chipresetvoltage(ENvoltageiswithinthe
1
V –0.3 — 0.25×VDD V
IL_nRST
specifiedrange)
1
VDD–voltagefromapowerpinofarespectivepowerdomain.
2
V andV aremeasuredusinghigh-impedanceload.
OH OL
6.4 Current Consumption Characteristics
6.4.1 Current Consumption in Active Mode
Withtheuseofadvancedpower-managementtechnologies,themodulecanswitchbetweendifferentpower
modes. Fordetailsondifferentpowermodes,pleaserefertoESP32-S3SeriesDatasheet>SectionPower
ManagementUnit.
Thecurrentconsumptionmeasurementsaretakenwitha3.3Vsupplyat25°Cambienttemperature.
TXcurrentconsumptionisratedata100%dutycycle.
RXcurrentconsumptionisratedwhentheperipheralsaredisabledandtheCPUidle.
Table6-4. CurrentConsumptionforWi-Fi(2.4GHz)inActiveMode
WorkMode RFCondition Description Peak(mA)
802.11b,1Mbps,@20.5dBm 355
802.11g,54Mbps,@18dBm 297
TX
802.11n,HT20,MCS7,@17.5dBm 286
Active(RFworking)
802.11n,HT40,MCS7,@17dBm 285
802.11b/g/n,HT20 95
RX
802.11n,HT40 97
Table6-5. CurrentConsumptionforBluetoothLEinActiveMode
WorkMode RFCondition Description Peak(mA)
BluetoothLE@20.0dBm 344
TX BluetoothLE@9.0dBm 202
Active(RFworking) BluetoothLE@0dBm 187
BluetoothLE@–15.0dBm 119
RX BluetoothLE 93
EspressifSystems 28 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 42


10 PhysicalDimensions
10 Physical Dimensions
10.1 Module Dimensions
2.0±5.52
15.61
9.0
x
04
5.1
72.1
9.0
x
04
18±0.2 3.1±0.15
15.8
Top View Side View Bottom View
6.71
40 x 0.45 1
50.1
0.8
0.5
40 x Ø0.55
3.7
40 x 0.85
6
Unit: mm
0.9
5 0.
Ø
13.97 1.27 2.015
ESP32-S3-WROOM-1U Dimensions
15.61
9.0
x
04
5.1
72.1
18±0.2
10.75 0.5
15.65
3.7
40 x 0.45 40 x 0.85
Top View Bottom View
5.71
0.9
Side View
2.0±2.91
9.0
x 04
0.8
40 x Ø0.55
1.08
64.2
1.1
Unit: mm
3.2±0.15
3
13.97 1.27 2.015
1.31 5 0. Ø 7.5
9.0
9.0
92.01
92.01
7.3
7.3
7.5
Figure10-1. ESP32-S3-WROOM-1PhysicalDimensions
2.0±5.52
15.61
9.0
x
04
5.1
72.1
9.0
x
04
18±0.2 3.1±0.15
15.8
Top View Side View Bottom View
6.71
40 x 0.45 1
50.1
0.8
0.5
40 x Ø0.55
3.7
40 x 0.85
6
Unit: mm
0.9
5 0.
Ø
13.97 1.27 2.015
ESP32-S3-WROOM-1U Dimensions
15.61
9.0
x
04
5.1
72.1
18±0.2
10.75
0.5
15.65
3.7
40 x 0.45 40 x 0.85
Top View Bottom View
5.71
0.9
Side View
2.0±2.91
9.0
x 04
0.8
40 x Ø0.55
1.08
64.2
1.1
Unit: mm
3.2±0.15
3
13.97 1.27 2.015
1.31 5 0. Ø 7.5
9.0
9.0
92.01
92.01
7.3
7.3
7.5
Figure10-2. ESP32-S3-WROOM-1UPhysicalDimensions
Note:
Forinformationabouttape,reel,andproductmarking,pleaserefertoESP32-S3ModulePackagingInformation.
EspressifSystems 42 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 43


10 PhysicalDimensions
10.2 Dimensions of External Antenna Connector
ESP32-S3-WROOM-1UusesthefirstgenerationexternalantennaconnectorasshowninFigure10-3
DimensionsofExternalAntennaConnector. Thisconnectoriscompatiblewiththefollowing
connectors:
• U.FLSeriesconnectorfromHirose
• MHFIconnectorfromI-PEX
• AMCconnectorfromAmphenol
Unit: mm
Figure10-3. DimensionsofExternalAntennaConnector
EspressifSystems 43 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 51


RevisionHistory
Revision History
Date Version Releasenotes
• Updated“OrderingCode”to“PartNumber”globally
• UpdatedthePinAssignmentpartforeachsubsectioninSection5.2Pe-
ripheralDescription
2026-03-02 v1.8
• AddedanoteinSection5.2.1.9SD/MMCHostController
• UpdatedTable6-7CurrentConsumptioninLow-PowerModes
• AddedTable6-5CurrentConsumptionforBluetoothLEinActiveMode
2025-11-18 v1.7
• AddedSection4.5ChipPower-upandReset
• AddedSection6.5MemorySpecifications
• Added the external antenna information for certification in Section 10.2
2025-07-25 v1.6 DimensionsofExternalAntennaConnector
• AddedSectionDatasheetVersioning
• Otherminorchanges
• Added a note about the pin mapping between the chip and the in-
2025-06-10 v1.5 packageflash/PSRANinSection2BlockDiagram
• RenamedmodulevariantsESP32-S3-WROOM-1-N16R16VandESP32-S3-
WROOM-1U-N16R16V to ESP32-S3-WROOM-1-N16R16VA and ESP32-S3-
WROOM-1U-N16R16VA
• AddedareferencetothechiprevisioninformationinthenoteinSection
Section1.2SeriesComparison
• UpdatedSection1.3Applications
• RestructuredthepreviousSectionStrappingPinsasSection4BootCon-
figurations
• AddedSection5.2PeripheralDescription
• Divided Section Electrical Characteristics into Section 6 Electrical Char-
2024-11-14 v1.4
acteristicsandSection7RFCharacteristicswithupdatedformattingand
wording
• DividedSectionPhysicalDimensionsandPCBLandPatternintoSection
10PhysicalDimensionsand11PCBLayoutRecommendationsandadded
Section11.2ModulePlacementforPCBDesign
• Added the 3D model link of ESP32-S3-WROOM-1U in Section 11.1 PCB
LandPattern
• UpdatedFigure12-1ReflowProfile
• Otherminorupdatestoformattingandwording
Cont’donnextpage
EspressifSystems 51 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback


---

## Page 52


RevisionHistory
Cont’dfrompreviouspage
Date Version Releasenotes
• Added the new module variants ESP32-S3-WROOM-1-N16R16VA and
ESP32-S3-WROOM-1U-N16R16VA,andupdatedtherelatedinformation
• UpdatedtablenotestoTable1-2ESP32-S3-WROOM-1USeriesCompari-
son
• UpdatedSection4.1ChipBootModeControl
2023-11-24 v1.3
• UpdatedthemoduleschematicsinSection8ModuleSchematics
• Updated the physical dimensions figure in Section 10.1 Module Dimen-
sions
• Otherminorupdates
• UpdateSectionStrappingPins
• UpdateSection6.4CurrentConsumptionCharacteristics
• UpdatetheminimumvalueofRFtransmitpowerinSection7.2.1Bluetooth
LERFTransmitter(TX)Characteristics
2023-03-07 v1.2 • UpdatedescriptionsinSection9PeripheralSchematics
• AdddescriptionsinSection11.1PCBLandPattern
• UpdateSection12.4
• Otherminorchanges
• UpdateTable1-1andTable1-2
2022-07-22 v1.1 • Otherminorupdates
• UpdateBluetoothLERFdata
• UpdatepowerconsumptiondatainTable6-7
2022-04-21 v1.0 • Addcertificationandtestinformation
• UpdateSectionStrappingPins
2021-10-29 v0.6 Overallupdateforchipevision1
2021-07-19 v0.5.1 Preliminaryrelease,forchiprevision0
EspressifSystems 52 ESP32-S3-WROOM-1&WROOM-1UDatasheetv1.8
SubmitDocumentationFeedback
