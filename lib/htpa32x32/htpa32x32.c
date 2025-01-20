/**
 * @file htpa32x32.c
 * @brief Implementation of HTPA32x32 thermal sensor library
 */

#include "string.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "esp32-hal.h"
#include "htpa32x32.h"
#include "lookuptable32x32.h"

uint8_t data_top[4][258];
uint8_t data_bottom[4][258];
uint8_t electrical_offset_top[258], electrical_offset_bottom[258];

bool HTPA32x32_Init(int i2c_num, int sda_pin, int scl_pin) {
    if (!HTPA32x32_I2C_Init(i2c_num, sda_pin, scl_pin)) {
        return false;
    }
    uint8_t config = CONFIG_WAKEUP;
    if (!HTPA32x32_I2C_Write(HTPA32x32_CONFIG_REG, &config, 1)) {
        return false;
    }
    return true;
}

bool HTPA32x32_LoadEEPROM_Data(HTPA32x32_EEPROM_Data *eeprom) {
    if (!eeprom) return false;

    // Read EEPROM parameters
    if (!HTPA32x32_EEPROM_Read(EEPROM_PIXC_MIN,     (uint8_t*)&eeprom->PixCmin,        4) ||
        !HTPA32x32_EEPROM_Read(EEPROM_PIXC_MAX,     (uint8_t*)&eeprom->PixCmax,        4) ||
        !HTPA32x32_EEPROM_Read(EEPROM_GRADSCALE,    &eeprom->gradScale,                1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_TN,           (uint8_t*)&eeprom->TN,             2) ||
        !HTPA32x32_EEPROM_Read(EEPROM_EPSILON,      &eeprom->epsilon,                  1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_MBIT_CALIB,   &eeprom->MBIT_calib,               1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_BIAS_CALIB,   &eeprom->BIAS_calib,               1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_CLK_CALIB,    &eeprom->CLK_calib,                1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_BPA_CALIB,    &eeprom->BPA_calib,                1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_PU_CALIB,     &eeprom->PU_calib,                 1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_ARRAYTYPE,    &eeprom->Arraytype,                1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_VDDTH1,       (uint8_t*)&eeprom->VDD_th1,        2) ||
        !HTPA32x32_EEPROM_Read(EEPROM_VDDTH2,       (uint8_t*)&eeprom->VDD_th2,        2) ||
        !HTPA32x32_EEPROM_Read(EEPROM_PTAT_GRAD,    (uint8_t*)&eeprom->PTAT_gradient,  4) ||
        !HTPA32x32_EEPROM_Read(EEPROM_PTAT_OFFSET,  (uint8_t*)&eeprom->PTAT_offset,    4) ||
        !HTPA32x32_EEPROM_Read(EEPROM_PTAT_TH1,     (uint8_t*)&eeprom->PTAT_th1,       2) ||
        !HTPA32x32_EEPROM_Read(EEPROM_PTAT_TH2,     (uint8_t*)&eeprom->PTAT_th2,       2) ||
        !HTPA32x32_EEPROM_Read(EEPROM_VDDSCGRAD,    &eeprom->VddScGrad,                1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_VDDSCOFF,     &eeprom->VddScOff,                 1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_GLOBALOFF,    (uint8_t*)&eeprom->GlobalOff,      1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_GLOBALGAIN,   (uint8_t*)&eeprom->GlobalGain,     2) ||
        !HTPA32x32_EEPROM_Read(EEPROM_MBIT_USER,    &eeprom->MBIT_user,                1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_BIAS_USER,    &eeprom->BIAS_user,                1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_CLK_USER,     &eeprom->CLK_user,                 1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_BPA_USER,     &eeprom->BPA_user,                 1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_PU_CALIB,     &eeprom->PU_user,                  1) ||
        !HTPA32x32_EEPROM_Read(EEPROM_DEVICEID,     (uint8_t*)&eeprom->DeviceID,       4) ||
        !HTPA32x32_EEPROM_Read(EEPROM_VDDCOMPGRAD,  (uint8_t*)eeprom->VddCompGrad,     (HTPA32x32_PIXELS_PER_BLOCK * 2 * 2)) ||
        !HTPA32x32_EEPROM_Read(EEPROM_VDDCOMPOFF,   (uint8_t*)eeprom->VddCompOff,      (HTPA32x32_PIXELS_PER_BLOCK * 2 * 2)) ||
        !HTPA32x32_EEPROM_Read(EEPROM_THGRAD,       (uint8_t*)eeprom->ThGrad,          (HTPA32x32_PIXELS * 2)) ||
        !HTPA32x32_EEPROM_Read(EEPROM_THOFFSET,     (uint8_t*)eeprom->ThOffset,        (HTPA32x32_PIXELS * 2)) ||
        !HTPA32x32_EEPROM_Read(EEPROM_P,            (uint8_t*)eeprom->P,               (HTPA32x32_PIXELS * 2))) {
            return false;
    }

    printf("PixCmin: %f, PixCmax: %f\r\n", eeprom->PixCmin, eeprom->PixCmax);
    printf("gradScale: %d\r\n", eeprom->gradScale);
    printf("TN: %d\r\n", eeprom->TN);
    printf("epsilon: %d\r\n", eeprom->epsilon);
    printf("Calibration: MBIT  BIAS  CLK  BPA  PU\r\n");
    printf("              %02x    %02x   %02x   %02x   %02x\r\n", 
                        eeprom->MBIT_calib, eeprom->BIAS_calib, eeprom->CLK_calib, 
                        eeprom->BPA_calib, eeprom->PU_calib);
    printf("Arraytype: %d\r\n", eeprom->Arraytype);
    printf("VDD_th1: %d\r\n", eeprom->VDD_th1);
    printf("VDD_th2: %d\r\n", eeprom->VDD_th2);
    printf("PTAT_gradient: %f\r\n", eeprom->PTAT_gradient);
    printf("PTAT_offset: %f\r\n", eeprom->PTAT_offset);
    printf("PTAT_th1: %d\r\n", eeprom->PTAT_th1);
    printf("PTAT_th2: %d\r\n", eeprom->PTAT_th2);
    printf("VddScGrad: %d\r\n", eeprom->VddScGrad);
    printf("VddScOff: %d\r\n", eeprom->VddScOff);
    printf("GlobalOff: %d\r\n", eeprom->GlobalOff);
    printf("GlobalGain: %d\r\n", eeprom->GlobalGain);
    printf("User Calibration: MBIT  BIAS  CLK  BPA  PU\r\n");
    printf("                   %02x    %02x   %02x   %02x   %02x\r\n", 
                        eeprom->MBIT_user, eeprom->BIAS_user, eeprom->CLK_user, 
                        eeprom->BPA_user, eeprom->PU_user);
    printf("DeviceID: %lu\r\n", eeprom->DeviceID);

    // Dead pixel information (optional, can continue if this fails)
    HTPA32x32_EEPROM_Read(EEPROM_DEADPIXADDR,       (uint8_t*)eeprom->DeadPixAdr,      48);
    HTPA32x32_EEPROM_Read(EEPROM_DEADPIXMASK,       eeprom->DeadPixMask,               24);

    return true;
}

bool HTPA32x32_LoadCalibration(HTPA32x32_EEPROM_Data *eeprom, bool user_calibration) {
    if (!eeprom) return false;
    uint8_t MBIT = user_calibration ? eeprom->MBIT_user : eeprom->MBIT_calib;
    uint8_t BIAS = user_calibration ? eeprom->BIAS_user : eeprom->BIAS_calib;
    uint8_t CLK = user_calibration ? eeprom->CLK_user : eeprom->CLK_calib;
    uint8_t BPA = user_calibration ? eeprom->BPA_user : eeprom->BPA_calib;
    uint8_t PU = user_calibration ? eeprom->PU_user : eeprom->PU_calib;
    
    if (!HTPA32x32_I2C_Write(HTPA32x32_TRIM_REG1, &MBIT, 1) ||
        !HTPA32x32_I2C_Write(HTPA32x32_TRIM_REG2, &BIAS, 1) ||
        !HTPA32x32_I2C_Write(HTPA32x32_TRIM_REG3, &BIAS, 1) ||
        !HTPA32x32_I2C_Write(HTPA32x32_TRIM_REG4, &CLK, 1) ||
        !HTPA32x32_I2C_Write(HTPA32x32_TRIM_REG5, &BPA, 1) ||
        !HTPA32x32_I2C_Write(HTPA32x32_TRIM_REG6, &BPA, 1) ||
        !HTPA32x32_I2C_Write(HTPA32x32_TRIM_REG7, &PU, 1)) {
            return false;
    }

    return true;
}

bool HTPA32x32_CalculatePixelSensitivity(HTPA32x32_Data *data, HTPA32x32_EEPROM_Data *eeprom) {
    if (!data || !eeprom) return false;

    for (int i = 0; i < HTPA32x32_ROWS; i++) {
        for (int j = 0; j < HTPA32x32_COLS; j++) {
            data->pix_c[i][j] = (( (float)(eeprom->P[i][j] * (eeprom->PixCmax - eeprom->PixCmin)) / 65535.0f ) + eeprom->PixCmin) + 
            ((float)eeprom->epsilon / 100.0f) + ((float)eeprom->GlobalGain / 10000.0f);
        }
    }
    return true;
}

uint8_t HTPA32x32_WaitDataReady(uint32_t timeout_ms) {
   uint8_t status = 0;
   uint32_t start = millis();

   do {
       if (!HTPA32x32_I2C_Read(HTPA32x32_STATUS_REG, &status, 1)) {
           break;
       }
       
       if (status & STATUS_EOC) {
           break;
       }

       if (millis() - start > timeout_ms) {
           break;
       }
   } while (1);
   return status;
}

bool HTPA32x32_GetElOffsets(HTPA32x32_Data *data) {
    uint8_t config = CONFIG_WAKEUP | CONFIG_START | CONFIG_BLIND;
    if (!HTPA32x32_I2C_Write(HTPA32x32_CONFIG_REG, &config, 1)) {
        return false;
    }
    uint8_t status = HTPA32x32_WaitDataReady(1000);
    if (!HTPA32x32_I2C_Read(HTPA32x32_READ_TOP, electrical_offset_top, 258) ||
        !HTPA32x32_I2C_Read(HTPA32x32_READ_BOTTOM, electrical_offset_bottom, 258)) {
        return false;
    }
    return true;
}

bool HTPA32x32_GetPixels(HTPA32x32_Data *data, bool vdd_meas) {
    uint8_t config = CONFIG_WAKEUP | CONFIG_START;
    if (vdd_meas) config |= CONFIG_VDD_MEAS;

    for (int block = 0; block < HTPA32x32_BLOCKS; block++) {
        config |= ((block & 0x03) << 4);
        if (!HTPA32x32_I2C_Write(HTPA32x32_CONFIG_REG, &config, 1)) {
            return false;
        }
        // wait for end of conversion bit
        uint8_t status = HTPA32x32_WaitDataReady(1000);
        if (!HTPA32x32_I2C_Read(HTPA32x32_READ_TOP, data_top[block], 258) ||
            !HTPA32x32_I2C_Read(HTPA32x32_READ_BOTTOM, data_bottom[block], 258)) {
            return false;
        }
        if(vdd_meas) {
            data->VDD[block] = (uint16_t)((data_top[block][0] << 8) | data_top[block][1]);
            data->VDD[block + 4] = (uint16_t)((data_bottom[block][0] << 8) | data_bottom[block][1]);
        } else {
            data->PTAT[block] = (uint16_t)((data_top[block][0] << 8) | data_top[block][1]);
            data->PTAT[block + 4] = (uint16_t)((data_bottom[block][0] << 8) | data_bottom[block][1]);
        }

    }
    return true;
}

void HTPA32x32_SortData(HTPA32x32_Data *data) {
    
    for (int block = 0; block < HTPA32x32_BLOCKS; block++) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < HTPA32x32_COLS; j++) {
                data->pixelData[block * 4 + i][j] = (uint16_t)(data_top[block][2 * (j + i * 32) + 2] << 8 | data_top[block][2 * (j + i * 32) + 3]);
                data->pixelData[block * 4 + i + 16][j] = (uint16_t)(data_bottom[3 - block][2 * (j + (3 - i) * 32) + 2] << 8 | data_bottom[3 - block][2 * (j + (3 - i) * 32) + 3]);
            }
        }

        for (int j = 0; j < HTPA32x32_COLS; j++) {
            data->electricalOffsets[block][j] = (uint16_t)(electrical_offset_top[2 * (j + block * 32) + 2] << 8   | electrical_offset_top[2 * (j + block * 32) + 3]);
            data->electricalOffsets[block + 4][j] = (uint16_t)(electrical_offset_bottom[2 * (j + block * 32) + 2] << 8 | electrical_offset_bottom[2 * (j + block * 32) + 3]);
        }
    }


    // Calculate averages after all blocks are read
    uint32_t ptat_sum = 0;
    uint32_t vdd_sum = 0;
    for (int i = 0; i < 8; i++) {
        vdd_sum += data->VDD[i];
        ptat_sum += data->PTAT[i];
    }
    data->VDDav = (uint16_t)(vdd_sum / 8);
    data->PTATav = (uint16_t)(ptat_sum / 8);
}

bool HTPA32x32_CalculateTemperatures(HTPA32x32_Data *data, HTPA32x32_EEPROM_Data *eeprom) {
    if (!data || !eeprom) return false;

    if(data->PTATav == 0 || data->VDDav == 0) return false;

    // Calculate ambient temperature
    data->ambientTemp = data->PTATav * eeprom->PTAT_gradient + eeprom->PTAT_offset;

    uint16_t table_col = 0;
    int32_t dta;
    for (int i = 0; i < NROFTAELEMENTS; i++) {
        if (data->ambientTemp > XTATemps[i]) {
            table_col = i;
        }
    }
    dta = data->ambientTemp - XTATemps[table_col];

    // Process each pixel
    for (int i = 0; i < HTPA32x32_ROWS; i++) {
        for (int j = 0; j < HTPA32x32_COLS; j++) {

            // Thermal offset
            float v_comp = data->pixelData[i][j] - ((float)(eeprom->ThGrad[i][j] * data->PTATav) / (1 << eeprom->gradScale)) - eeprom->ThOffset[i][j];
                // printf("ThGrad: %d\n", eeprom->ThGrad[i][j]);
                // printf("PTATav: %d\n", data->PTATav);
                // printf("ThOffset: %d\n", eeprom->ThOffset[i][j]);
                // printf("    v_comp: %f\n", v_comp);

            // Electrical offset
            int pixel_offset = (i < HTPA32x32_ROWS / 2) ? (j + i * 32) % 128  :  (j + i * 32) % 128 + 128;
            int el_offset_row = (i < HTPA32x32_ROWS / 2) ? i % 4  :  i % 4 + 4;
            float v_el = v_comp - (float)data->electricalOffsets[el_offset_row][j];
                // printf("electricalOffsets: %u\n", data->electricalOffsets[el_offset_row][j]);
                // printf("    v_el: %f\n", v_el);

            // VDD compensation
            float v_vdd_comp = v_el - ( (float)( (float)( eeprom->VddCompGrad[pixel_offset] * data->PTATav ) / ( 1 << eeprom->VddScGrad ) +
                      eeprom->VddCompOff[pixel_offset] ) / (1 << eeprom->VddScOff)) * (data->VDDav - eeprom->VDD_th1 - 
                      ( (float)(eeprom->VDD_th2 - eeprom->VDD_th1) / (float)(eeprom->PTAT_th2 - eeprom->PTAT_th1)) * (data->PTATav - eeprom->PTAT_th1));
                // printf("VddCompGrad: %d\n", eeprom->VddCompGrad[pixel_offset]);
                // printf("VddCompOff: %d\n", eeprom->VddCompOff[pixel_offset]);
                // printf("    v_vdd_comp: %f\n", v_vdd_comp);

            // Calculate compensated voltage
            float v_pixc = ( v_vdd_comp * PCSCALEVAL) / data->pix_c[i][j];
                // printf("pix_c: %f\n", data->pix_c[i][j]);
                // printf("    v_pixc: %f\n", v_pixc);
            
            // Get final temperature using lookup table
            uint16_t table_row = v_pixc + TABLEOFFSET;
            table_row = table_row >> ADEXPBITS;

            // bilinear interpolation
            int tx  = TempTable[table_row][table_col];
            int tx1 = TempTable[table_row][table_col + 1];
            int ty  = TempTable[table_row + 1][table_col];
            int ty1 = TempTable[table_row + 1][table_col + 1];
            int vx  = (((tx1 - tx) * dta) / TAEQUIDISTANCE) + tx;
            int vy  = (((ty1 - ty) * dta) / TAEQUIDISTANCE) + ty;
            int temp = ((vy - vx) * ((v_pixc + TABLEOFFSET) - YADValues[table_row]) / (1 << ADEXPBITS) + vx);
           
            // Apply global offset
            temp += eeprom->GlobalOff;
            data->pixelTemps[i][j] = (temp / 10.0f) - 273.2f;
                // printf("temp: %d\n", temp);
                // printf("pixelTemps: %f\n", data->pixelTemps[i][j]);
        }
    }


    return true;
}

float HTPA32x32_GetPixelTemp(HTPA32x32_Data *data, uint8_t i, uint8_t j) {
    if (!data || i >= HTPA32x32_ROWS || j >= HTPA32x32_COLS) {
        return 0.0f;
    }
    return data->pixelTemps[i][j];
}

float HTPA32x32_GetAmbientTemp(HTPA32x32_Data *data) {
    return data ? data->ambientTemp : 0.0f;
}