/**
 * @file htpa32x32.h
 * @brief Header file for HTPA32x32 thermal sensor library
 */

#ifndef HTPA32X32_H
#define HTPA32X32_H

#ifdef __cplusplus
extern "C" {
#endif

// Device constants
#define HTPA32x32_ROWS 32
#define HTPA32x32_COLS 32
#define HTPA32x32_PIXELS (HTPA32x32_ROWS * HTPA32x32_COLS)
#define HTPA32x32_BLOCKS 4
#define HTPA32x32_PIXELS_PER_BLOCK 128

// EEPROM Addresses
#define EEPROM_PIXC_MIN          0x0000  // PixCmin (float)
#define EEPROM_PIXC_MAX          0x0004  // PixCmax (float)
#define EEPROM_GRADSCALE         0x0008  // gradScale
#define EEPROM_TN                0x000B  // TN as 16 bit unsigned
#define EEPROM_EPSILON           0x000D  // epsilon
#define EEPROM_MBIT_CALIB        0x001A  // MBIT(calib)
#define EEPROM_BIAS_CALIB        0x001B  // BIAS(calib)
#define EEPROM_CLK_CALIB         0x001C  // CLK(calib)
#define EEPROM_BPA_CALIB         0x001D  // BPA(calib)
#define EEPROM_PU_CALIB          0x001E  // PU(calib)
#define EEPROM_ARRAYTYPE         0x0022  // Arraytype
#define EEPROM_VDDTH1            0x0026  // VDD_th1
#define EEPROM_VDDTH2            0x0028  // VDD_th2
#define EEPROM_PTAT_GRAD         0x0034  // PTAT_gradient (float)
#define EEPROM_PTAT_OFFSET       0x0038  // PTAT_offset (float)
#define EEPROM_PTAT_TH1          0x003C  // PTAT_th1
#define EEPROM_PTAT_TH2          0x003E  // PTAT_th2
#define EEPROM_VDDSCGRAD         0x004E  // VddScGrad
#define EEPROM_VDDSCOFF          0x004F  // VddScOff
#define EEPROM_GLOBALOFF         0x0054  // GlobalOff
#define EEPROM_GLOBALGAIN        0x0055  // GlobalGain
#define EEPROM_MBIT_USER         0x0060  // MBIT(user)
#define EEPROM_BIAS_USER         0x0061  // BIAS(user)
#define EEPROM_CLK_USER          0x0062  // CLK(user)
#define EEPROM_BPA_USER          0x0063  // BPA(user)
#define EEPROM_PU_USER           0x0064  // PU(user)
#define EEPROM_DEVICEID          0x0074  // DeviceID
#define EEPROM_DEADPIXADDR       0x0080  // DeadPixAddr
#define EEPROM_DEADPIXMASK       0x00B0  // DeadPixMask
#define EEPROM_VDDCOMPGRAD       0x0340  // VddCompGrad
#define EEPROM_VDDCOMPOFF        0x0540  // VddCompOff
#define EEPROM_THGRAD            0x0740  // ThGrad
#define EEPROM_THOFFSET          0x0F40  // ThOffset
#define EEPROM_P                 0x1740  // P

// Register addresses
#define HTPA32x32_CONFIG_REG   0x01
#define HTPA32x32_STATUS_REG   0x02
#define HTPA32x32_TRIM_REG1    0x03
#define HTPA32x32_TRIM_REG2    0x04
#define HTPA32x32_TRIM_REG3    0x05
#define HTPA32x32_TRIM_REG4    0x06
#define HTPA32x32_TRIM_REG5    0x07
#define HTPA32x32_TRIM_REG6    0x08
#define HTPA32x32_TRIM_REG7    0x09
#define HTPA32x32_READ_TOP     0x0A
#define HTPA32x32_READ_BOTTOM  0x0B

// Configuration bits
#define CONFIG_WAKEUP      (1 << 0)
#define CONFIG_BLIND       (1 << 1)
#define CONFIG_VDD_MEAS    (1 << 2)
#define CONFIG_START       (1 << 3)

#define STATUS_EOC         (1 << 0)
#define STATUS_BLIND       (1 << 1)
#define STATUS_VDD_MEAS    (1 << 2)

// #define HTPA32x32dR2L1_6HiGe_Gain3k3
// #define HTPA32x32dR2L2_1SiF5_0_N2
#define HTPA32x32dR2L2_1HiSiF5_0_Gain3k3
// #define HTPA32x32dR2L1k8_0k7HiGe		
// #define HTPA32x32dR2L1k8_0k7HiGe_TaExtended				// same like the above but with a larger working ambient temperature range
// #define HTPA32x32dR2L2_1HiSiF5_0_Precise	
// #define HTPA32x32dR2L2_1HiSiF5_0_Gain3k3_Extended
// #define HTPA32x32dR2L2_85Hi_Gain3k3		
// #define HTPA32x32dR2L3_6HiSi_Rev1_Gain3k3
// #define HTPA32x32dR2L3_6HiSi_Rev1_Gain3k3_TaExtended		// same like the above but with a larger working ambient temperature range
// #define HTPA32x32dR2L5_0HiGeF7_7_Gain3k3	
// #define HTPA32x32dR2L5_0HiGeF7_7_Gain3k3_TaExtended		// same like the above but with a larger working ambient temperature range
// #define HTPA32x32dR2L5_0HiGeF7_7_Gain3k3_Fever			// higher resolution (not higher accuracy) but limited to Ta between +5 and +50 *C
// #define HTPA32x32dR2L7_0HiSi_Gain3k3


#ifdef HTPA32x32dR2L5_0HiGeF7_7_Gain3k3_Fever
	#define TABLENUMBER		113
	#define PCSCALEVAL		100000000 //327000000000
	#define NROFTAELEMENTS 	19
	#define NROFADELEMENTS 	1595
	#define TAEQUIDISTANCE	25
	#define ADEXPBITS		3
	#define TABLEOFFSET		1024 
#endif		

#ifdef HTPA32x32dR2L5_0HiGeF7_7_Gain3k3
	#define TABLENUMBER		113
	#define PCSCALEVAL		100000000 //327000000000
	#define NROFTAELEMENTS 	7
	#define NROFADELEMENTS 	1595
	#define TAEQUIDISTANCE	100
	#define ADEXPBITS		6
	#define TABLEOFFSET		1024   
#endif	

#ifdef HTPA32x32dR2L5_0HiGeF7_7_Gain3k3_TaExtended
	#define TABLENUMBER		113
	#define PCSCALEVAL		100000000 //327000000000
	#define NROFTAELEMENTS 	12
	#define NROFADELEMENTS 	1595
	#define TAEQUIDISTANCE	100
	#define ADEXPBITS		6
	#define TABLEOFFSET		1024 
#endif		

#ifdef HTPA32x32dR2L1_6HiGe_Gain3k3
	#define TABLENUMBER		119
	#define PCSCALEVAL		100000000 //327000000000
	#define NROFTAELEMENTS 	7
	#define NROFADELEMENTS 	1595
	#define TAEQUIDISTANCE	100
	#define ADEXPBITS		6
	#define TABLEOFFSET		1024
#endif	

#ifdef HTPA32x32dR2L2_1SiF5_0_N2
	#define TABLENUMBER		130
	#define PCSCALEVAL		100000000
	#define NROFTAELEMENTS 	7
	#define NROFADELEMENTS 	1595
	#define TAEQUIDISTANCE	100
	#define ADEXPBITS		6
	#define TABLEOFFSET		192  
#endif	

#ifdef HTPA32x32dR2L2_1HiSiF5_0_Gain3k3
	#define TABLENUMBER		114
	#define PCSCALEVAL		100000000
	#define NROFTAELEMENTS 	7
	#define NROFADELEMENTS 	1595
	#define TAEQUIDISTANCE	100
	#define ADEXPBITS		6
	#define TABLEOFFSET		1024
#endif	

#ifdef HTPA32x32dR2L2_1HiSiF5_0_Gain3k3_Extended
	#define TABLENUMBER		114
	#define PCSCALEVAL		100000000
	#define NROFTAELEMENTS 	12
	#define NROFADELEMENTS 	1595
	#define TAEQUIDISTANCE	100
	#define ADEXPBITS		6
	#define TABLEOFFSET		1792
#endif	

#ifdef HTPA32x32dR2L2_1HiSiF5_0_Precise
	#define TABLENUMBER		116
	#define PCSCALEVAL		100000000
	#define NROFTAELEMENTS 	22
	#define NROFADELEMENTS 	1000
	#define TAEQUIDISTANCE	50
	#define ADEXPBITS		5
	#define TABLEOFFSET		1024  
#endif	

#ifdef HTPA32x32dR2L2_85Hi_Gain3k3
	#define TABLENUMBER		127
	#define PCSCALEVAL		100000000
	#define NROFTAELEMENTS 	7
	#define NROFADELEMENTS 	1595
	#define TAEQUIDISTANCE	100
	#define ADEXPBITS		6
	#define TABLEOFFSET		1024  
#endif	

#ifdef HTPA32x32dR2L3_6HiSi_Rev1_Gain3k3
	#define TABLENUMBER		117
	#define PCSCALEVAL		100000000
	#define NROFTAELEMENTS 	7
	#define NROFADELEMENTS 	1595
	#define TAEQUIDISTANCE	100
	#define ADEXPBITS		6
	#define TABLEOFFSET		1024
#endif	

#ifdef HTPA32x32dR2L3_6HiSi_Rev1_Gain3k3_TaExtended
	#define TABLENUMBER		117
	#define PCSCALEVAL		100000000
	#define NROFTAELEMENTS 	12
	#define NROFADELEMENTS 	1595
	#define TAEQUIDISTANCE	100
	#define ADEXPBITS		6
	#define TABLEOFFSET		1024
#endif		

#ifdef HTPA32x32dR2L7_0HiSi_Gain3k3
	#define TABLENUMBER		118
	#define PCSCALEVAL		100000000
	#define NROFTAELEMENTS 	7
	#define NROFADELEMENTS 	1595
	#define TAEQUIDISTANCE	100
	#define ADEXPBITS		6
	#define TABLEOFFSET		640
#endif	

#ifdef HTPA32x32dR2L1k8_0k7HiGe
	#define TABLENUMBER		115	
	#define PCSCALEVAL		100000000
	#define NROFTAELEMENTS 	10
	#define NROFADELEMENTS 	471
	#define TAEQUIDISTANCE	100
	#define ADEXPBITS		6
	#define TABLEOFFSET		1024
#endif	

#ifdef HTPA32x32dR2L1k8_0k7HiGe_TaExtended
	#define TABLENUMBER		115	
	#define PCSCALEVAL		100000000
	#define NROFTAELEMENTS 	12
	#define NROFADELEMENTS 	471
	#define TAEQUIDISTANCE	100
	#define ADEXPBITS		6
	#define TABLEOFFSET		1024 
#endif	

typedef struct {
    float PixCmin;
    float PixCmax;
    uint8_t gradScale;
    uint16_t TN;
    uint8_t epsilon;
    uint8_t MBIT_calib;
    uint8_t BIAS_calib;
    uint8_t CLK_calib;
    uint8_t BPA_calib;
    uint8_t PU_calib;
    uint8_t Arraytype;
    uint16_t VDD_th1;
    uint16_t VDD_th2;
    float PTAT_gradient;
    float PTAT_offset;
    uint16_t PTAT_th1;
    uint16_t PTAT_th2;
    uint8_t VddScGrad;
    uint8_t VddScOff;
    int8_t GlobalOff;
    uint16_t GlobalGain;
    uint8_t MBIT_user;
    uint8_t BIAS_user;
    uint8_t CLK_user;
    uint8_t BPA_user;
    uint8_t PU_user;
    uint32_t DeviceID;
    uint8_t NrOfDefPix;
    uint16_t DeadPixAdr[48];
    uint8_t DeadPixMask[24];
    int16_t VddCompGrad[HTPA32x32_PIXELS_PER_BLOCK * 2];
    int16_t VddCompOff[HTPA32x32_PIXELS_PER_BLOCK * 2];
    int16_t ThGrad[HTPA32x32_ROWS][HTPA32x32_COLS];
    int16_t ThOffset[HTPA32x32_ROWS][HTPA32x32_COLS];
    uint16_t P[HTPA32x32_ROWS][HTPA32x32_COLS];
} HTPA32x32_EEPROM_Data;

typedef struct {
    uint16_t PTAT[8];
    uint16_t PTATav;
    uint16_t VDD[8];
    uint16_t VDDav;
    float pix_c[HTPA32x32_ROWS][HTPA32x32_COLS];
    uint16_t pixelData[HTPA32x32_ROWS][HTPA32x32_COLS];
    uint16_t electricalOffsets[8][HTPA32x32_COLS];
    float pixelTemps[HTPA32x32_ROWS][HTPA32x32_COLS];
    float ambientTemp;
} HTPA32x32_Data;


// Function declarations
bool HTPA32x32_Init(int i2c_num, int sda_pin, int scl_pin);
bool HTPA32x32_LoadEEPROM_Data(HTPA32x32_EEPROM_Data *eeprom_data);
bool HTPA32x32_LoadCalibration(HTPA32x32_EEPROM_Data *eeprom_data, bool user_calibration);
bool HTPA32x32_CalculatePixelSensitivity(HTPA32x32_Data *data, HTPA32x32_EEPROM_Data *eeprom_data);
bool HTPA32x32_GetElOffsets(HTPA32x32_Data *data);
bool HTPA32x32_GetPixels(HTPA32x32_Data *data, bool vdd_meas);
void HTPA32x32_SortData(HTPA32x32_Data *data);
bool HTPA32x32_CalculateTemperatures(HTPA32x32_Data *data, HTPA32x32_EEPROM_Data *eeprom_data);
float HTPA32x32_GetPixelTemp(HTPA32x32_Data *data, uint8_t row, uint8_t col);
float HTPA32x32_GetAmbientTemp(HTPA32x32_Data *data);

// I2C communication functions (to be implemented by user)
extern bool HTPA32x32_I2C_Init(int i2c_num, int sda_pin, int scl_pin);
extern bool HTPA32x32_I2C_Write(uint8_t reg, uint8_t *data, uint16_t len);
extern bool HTPA32x32_I2C_Read(uint8_t reg, uint8_t *data, uint16_t len);
extern bool HTPA32x32_EEPROM_Read(uint16_t addr, uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // HTPA32X32_H