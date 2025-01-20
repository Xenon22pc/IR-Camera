#ifndef _HTPA_H_
#define _HTPA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"

// Choose sensor model
#define HTPA32x32dR2L2_1HiSiF5_0_Gain3k3
// #define HTPA32x32dR2L2_1HiSiF5_0_Gain3k3_Extended
// #define HTPA32x32dR2L2_1HiSiF5_0_Precise	
// #define HTPA32x32dR2L2_1SiF5_0_N2
// #define HTPA32x32dR2L1_6HiGe_Gain3k3
// #define HTPA32x32dR2L1k8_0k7HiGe	
// #define HTPA32x32dR2L1k8_0k7HiGe_TaExtended				// same like the above but with a larger working ambient temperature range
// #define HTPA32x32dR2L2_85Hi_Gain3k3		
// #define HTPA32x32dR2L3_6HiSi_Rev1_Gain3k3
// #define HTPA32x32dR2L3_6HiSi_Rev1_Gain3k3_TaExtended		// same like the above but with a larger working ambient temperature range
// #define HTPA32x32dR2L5_0HiGeF7_7_Gain3k3	
// #define HTPA32x32dR2L5_0HiGeF7_7_Gain3k3_TaExtended		// same like the above but with a larger working ambient temperature range
// #define HTPA32x32dR2L5_0HiGeF7_7_Gain3k3_Fever			// higher resolution (not higher accuracy) but limited to Ta between +5 and +50 *C
// #define HTPA32x32dR2L7_0HiSi_Gain3k3

// Device constants
#define HTPA_ROWS 32
#define HTPA_COLS 32
#define HTPA_PIXELS (HTPA_ROWS * HTPA_COLS)
#define HTPA_BLOCKS 4
#define HTPA_PIXELS_PER_BLOCK 128
#define HTPA_VDD_PERIOD      10000

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
#define EEPROM_NROFDEFPIX        0x007F  // NrOfDefPix
#define EEPROM_DEADPIXADDR       0x0080  // DeadPixAddr
#define EEPROM_DEADPIXMASK       0x00B0  // DeadPixMask
#define EEPROM_VDDCOMPGRAD       0x0340  // VddCompGrad
#define EEPROM_VDDCOMPOFF        0x0540  // VddCompOff
#define EEPROM_THGRAD            0x0740  // ThGrad
#define EEPROM_THOFFSET          0x0F40  // ThOffset
#define EEPROM_P                 0x1740  // P

// Register addresses
#define HTPA_CONFIG_REG   0x01
#define HTPA_STATUS_REG   0x02
#define HTPA_TRIM_REG1    0x03
#define HTPA_TRIM_REG2    0x04
#define HTPA_TRIM_REG3    0x05
#define HTPA_TRIM_REG4    0x06
#define HTPA_TRIM_REG5    0x07
#define HTPA_TRIM_REG6    0x08
#define HTPA_TRIM_REG7    0x09
#define HTPA_READ_TOP     0x0A
#define HTPA_READ_BOTTOM  0x0B

// Configuration bits
#define CONFIG_WAKEUP      (1 << 0)
#define CONFIG_BLIND       (1 << 1)
#define CONFIG_VDD_MEAS    (1 << 2)
#define CONFIG_START       (1 << 3)

#define STATUS_EOC         (1 << 0)
#define STATUS_BLIND       (1 << 1)
#define STATUS_VDD_MEAS    (1 << 2)

#define HTPA_OK     0
#define HTPA_ERR    -1
#define CHECK_ERROR(err) if (err != 0) { return err; }

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
    uint16_t DeadPixAdr[24];
    uint8_t DeadPixMask[12];
    int16_t VddCompGrad[HTPA_BLOCKS * 2][HTPA_COLS];
    int16_t VddCompOff[HTPA_BLOCKS * 2][HTPA_COLS];
    int16_t ThGrad[HTPA_ROWS][HTPA_COLS];
    int16_t ThOffset[HTPA_ROWS][HTPA_COLS];
    uint16_t P[HTPA_ROWS][HTPA_COLS];
} HTPA_EEPROM_Data_t;

typedef struct {
    uint16_t PTAT[8];
    uint16_t PTATav;
    uint16_t VDD[8];
    uint16_t VDDav;
    double pix_c[HTPA_ROWS][HTPA_COLS];
    uint16_t pixelData[HTPA_ROWS][HTPA_COLS];
    uint16_t electricalOffsets[HTPA_BLOCKS * 2][HTPA_COLS];
    double pixelTemps[HTPA_ROWS][HTPA_COLS];
    double ambientTemp;
} HTPA_Data_t;

int HTPA_Init(HTPA_Data_t *data, HTPA_EEPROM_Data_t *eeprom, int i2c_num, int sda_pin, int scl_pin);
int HTPA_LoadCalibration(HTPA_EEPROM_Data_t *eeprom, bool user_calibration);
void HTPA_PrintPixelTemps(HTPA_Data_t *data);
void HTPA_CalculatePixelSensitivity(HTPA_Data_t *data, HTPA_EEPROM_Data_t *eeprom);
uint8_t HTPA_WaitDataReady(uint32_t timeout_ms);
int HTPA_GetPixels(HTPA_Data_t *data, bool vdd_meas);
int HTPA_GetElOffsets(HTPA_Data_t *data);
void HTPA_SortData(HTPA_Data_t *data);
void HTPA_CalculateTemperatures(HTPA_Data_t *data, HTPA_EEPROM_Data_t *eeprom);
void HTPA_PixelMasking(HTPA_Data_t *data, HTPA_EEPROM_Data_t *eeprom);
int HTPA_CaptureData(HTPA_Data_t *data, HTPA_EEPROM_Data_t *eeprom);

int HTPA_ReadEEPROM(HTPA_EEPROM_Data_t *eeprom);
void HTPA_PrintEEPROM(HTPA_EEPROM_Data_t *eeprom);

// I2C communication functions (to be implemented by user)
extern int HTPA_I2C_Init(int i2c_num, int sda_pin, int scl_pin, uint32_t clk_speed);
extern int HTPA_I2C_DeInit(int i2c_num);
extern int HTPA_I2C_Write(uint8_t reg, uint8_t *data, uint16_t len);
extern int HTPA_I2C_Read(uint8_t reg, uint8_t *data, uint16_t len);
extern int HTPA_EEPROM_Read(uint16_t addr, uint8_t *data, uint16_t len);

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

#ifdef __cplusplus
}
#endif

#endif