#include "htpa.h"
#include "driver/i2c.h"
#include "esp32-hal.h"
#include "sensordef_32x32.h"
#include "lookuptable.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

uint8_t data_top[4][258];
uint8_t data_bottom[4][258];
uint8_t electrical_offset_top[258];
uint8_t electrical_offset_bottom[258];

uint8_t read_EEPROM_byte(uint8_t deviceaddress, uint16_t eeaddress) {
    uint8_t data;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (deviceaddress << 1) | I2C_MASTER_WRITE, 1);
    i2c_master_write_byte(cmd, eeaddress >> 8, 1);
    i2c_master_write_byte(cmd, eeaddress & 0xFF, 1);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (deviceaddress << 1) | I2C_MASTER_READ, 1);
    i2c_master_read_byte(cmd, &data, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return data;
}

int HTPA_Init(HTPA_Data_t *data, HTPA_EEPROM_Data_t *eeprom, int i2c_num, int sda_pin, int scl_pin) {

    CHECK_ERROR(HTPA_I2C_Init(i2c_num, sda_pin, scl_pin, CLOCK_SENSOR));
    CHECK_ERROR(HTPA_ReadEEPROM(eeprom));
    // HTPA_PrintEEPROM(eeprom);
    uint8_t config = CONFIG_WAKEUP;
    CHECK_ERROR(HTPA_I2C_Write(HTPA_CONFIG_REG, &config, 1));
    CHECK_ERROR(HTPA_LoadCalibration(eeprom, false));

    HTPA_CalculatePixelSensitivity(data, eeprom);

    if (eeprom->TN != TABLENUMBER) {
        printf("\n\nHINT:\tConnected sensor does not match the selected look up table.");
        printf("\n\tThe calculated temperatures could be wrong!");
        printf("\n\tChange device in sensordef_32x32.h to sensor with tablenumber ");
        printf("%u", eeprom->TN);
    }
    return HTPA_OK;
}

int HTPA_LoadCalibration(HTPA_EEPROM_Data_t *eeprom, bool user_calibration) {
    if (!eeprom) return HTPA_ERR;
    uint8_t MBIT = user_calibration ? eeprom->MBIT_user : eeprom->MBIT_calib;
    uint8_t BIAS = user_calibration ? eeprom->BIAS_user : eeprom->BIAS_calib;
    uint8_t CLK = user_calibration ? eeprom->CLK_user : eeprom->CLK_calib;
    uint8_t BPA = user_calibration ? eeprom->BPA_user : eeprom->BPA_calib;
    uint8_t PU = user_calibration ? eeprom->PU_user : eeprom->PU_calib;
    
    CHECK_ERROR(HTPA_I2C_Write(HTPA_TRIM_REG1, &MBIT, 1));
    CHECK_ERROR(HTPA_I2C_Write(HTPA_TRIM_REG2, &BIAS, 1));
    CHECK_ERROR(HTPA_I2C_Write(HTPA_TRIM_REG3, &BIAS, 1));
    CHECK_ERROR(HTPA_I2C_Write(HTPA_TRIM_REG4, &CLK, 1));
    CHECK_ERROR(HTPA_I2C_Write(HTPA_TRIM_REG5, &BPA, 1));
    CHECK_ERROR(HTPA_I2C_Write(HTPA_TRIM_REG6, &BPA, 1));
    CHECK_ERROR(HTPA_I2C_Write(HTPA_TRIM_REG7, &PU, 1));

    return HTPA_OK;
}

void HTPA_PrintPixelTemps(HTPA_Data_t *data) {
    printf("Print Pixel Temps\r\n");
    for (int m = 0; m < 32; m++) {
        for (int n = 0; n < 32; n++) {
            printf("%.2f ",data->pixelTemps[m][n] );
        }
        printf("\r\n");
    }
}

void HTPA_CalculatePixelSensitivity(HTPA_Data_t *data, HTPA_EEPROM_Data_t *eeprom) {
    for (int i = 0; i < HTPA_ROWS; i++) {
        for (int j = 0; j < HTPA_COLS; j++) {
            // calc sensitivity coefficients (see datasheet, chapter: 11.5 Object Temperature)
            double pixc_steps = eeprom->PixCmax - eeprom->PixCmin;
            pixc_steps = pixc_steps / 65535;
            pixc_steps = pixc_steps * eeprom->P[i][j];
            pixc_steps = pixc_steps + eeprom->PixCmin;
            pixc_steps = pixc_steps * 1.0 * eeprom->epsilon / 100;
            data->pix_c[i][j] = pixc_steps * 1.0 * eeprom->GlobalGain / 10000;
        }
    }
}

uint8_t HTPA_WaitDataReady(uint32_t timeout_ms) {
   uint8_t status = 0;
   uint32_t start = millis();

   do {
       if (HTPA_I2C_Read(HTPA_STATUS_REG, &status, 1)) {
           break;
       }
       
       if (status & STATUS_EOC) {
           break;
       }

       if (millis() - start > timeout_ms) {
           break;
       }
       delay(1);
   } while (1);
   return status;
}

int HTPA_GetElOffsets(HTPA_Data_t *data) {
    uint8_t config = CONFIG_WAKEUP | CONFIG_START | CONFIG_BLIND;
    if (HTPA_I2C_Write(HTPA_CONFIG_REG, &config, 1)) {
        return HTPA_ERR;
    }
    uint8_t status = HTPA_WaitDataReady(1000);
    if (HTPA_I2C_Read(HTPA_READ_TOP, electrical_offset_top, 258) ||
        HTPA_I2C_Read(HTPA_READ_BOTTOM, electrical_offset_bottom, 258)) {
        return HTPA_ERR;
    }
    return HTPA_OK;
}

int HTPA_GetPixels(HTPA_Data_t *data, bool vdd_meas) {
    uint8_t config = CONFIG_WAKEUP | CONFIG_START;
    if (vdd_meas) config |= CONFIG_VDD_MEAS;

    for (int block = 0; block < HTPA_BLOCKS; block++) {
        config = (config & ~(0x03 << 4)) | ((block & 0x03) << 4);
        if (HTPA_I2C_Write(HTPA_CONFIG_REG, &config, 1)) {
            return HTPA_ERR;
        }
        // wait for end of conversion bit
        uint8_t status = HTPA_WaitDataReady(1000);
        if (HTPA_I2C_Read(HTPA_READ_TOP, data_top[block], 258) ||
            HTPA_I2C_Read(HTPA_READ_BOTTOM, data_bottom[block], 258)) {
            return HTPA_ERR;
        }
        if(vdd_meas) {
            data->VDD[block] = (uint16_t)((data_top[block][0] << 8) | data_top[block][1]);
            data->VDD[block + 4] = (uint16_t)((data_bottom[block][0] << 8) | data_bottom[block][1]);
        } else {
            data->PTAT[block] = (uint16_t)((data_top[block][0] << 8) | data_top[block][1]);
            data->PTAT[block + 4] = (uint16_t)((data_bottom[block][0] << 8) | data_bottom[block][1]);
        }
    }
    return HTPA_OK;
}

void HTPA_SortData(HTPA_Data_t *data) {

    // Read pixel data
    for (int block = 0; block < 4; block++) {
        for (int j = 0; j < 32; j++) {
            for (int i = 0; i < 4; i++) {
                data->pixelData[block * 4 + i][j] = (data_top[block][2 * (j + i * 32) + 2] << 8) | data_top[block][2 * (j + i * 32) + 3];
                data->pixelData[16 + block * 4 + i][j] = (data_bottom[3 - block][2 * j + (3 - i) * 64 + 2] << 8) | data_bottom[3 - block][2 * j + (3 - i) * 64 + 3];
            }
        }
    }

    // Read electrical offsets
    for (int j = 0; j < 32; j++) {
        for (int i = 0; i < 4; i++) {
            data->electricalOffsets[i][j] = (electrical_offset_top[2 * (j + i * 32) + 2] << 8) | electrical_offset_top[2 * (j + i * 32) + 3];
            data->electricalOffsets[4 + i][j] = (electrical_offset_bottom[2 * (j + (3 - i) * 32) + 2] << 8) | electrical_offset_bottom[2 * (j + (3 - i) * 32) + 3];
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

void HTPA_CalculateTemperatures(HTPA_Data_t *data, HTPA_EEPROM_Data_t *eeprom) {
    if(data->PTATav == 0 || data->VDDav == 0) return;

    uint16_t table_row, table_col;
    int32_t vx, vy, dta;

    // Calculate ambient temperature
    data->ambientTemp = data->PTATav * eeprom->PTAT_gradient + eeprom->PTAT_offset;
        // printf("PTATav: %d, PTAT_gradient: %.2f, PTAT_offset: %.2f\n", data->PTATav, eeprom->PTAT_gradient, eeprom->PTAT_offset);
        // printf("    ambientTemp: %.2f\n", data->ambientTemp);

    // find column of lookup table
    for (int i = 0; i < NROFTAELEMENTS; i++) {
        if (data->ambientTemp > XTATemps[i]) {
            table_col = i;
        }
    }
    dta = data->ambientTemp - XTATemps[table_col];

    for (int i = 0; i < HTPA_ROWS; i++) {
        for (int j = 0; j < HTPA_COLS; j++) {
            // Thermal offset
            double v_comp = (data->pixelData[i][j] - (eeprom->ThGrad[i][j] * (double)data->PTATav) / (1 << eeprom->gradScale) - eeprom->ThOffset[i][j]);
                // printf("pixelData: %d, eeprom->ThGrad: %d, PTATav: %d, eeprom->ThOffset: %d\n", 
                //     data->pixelData[i][j], eeprom->ThGrad[i][j], data->PTATav, eeprom->ThOffset[i][j]);
                // printf("    v_comp: %f\n", v_comp);

            // Electrical offset
            uint8_t selectRow = (i < HTPA_ROWS / 2) ? i % 4 : i % 4 + 4;
            double v_el = v_comp - data->electricalOffsets[selectRow][j];
                // printf("elOffsets: %u\n", data->electricalOffsets[selectRow][j]);
                // printf("    v_el: %f\n", v_el);

            // VDD compensation
            double vdd_calc_steps = eeprom->VddCompGrad[selectRow][j] * data->PTATav;
            vdd_calc_steps = vdd_calc_steps / ( 1 << eeprom->VddScGrad );
            vdd_calc_steps = vdd_calc_steps + eeprom->VddCompOff[selectRow][j];

            vdd_calc_steps = vdd_calc_steps * (data->VDDav - eeprom->VDD_th1 - ((eeprom->VDD_th2 - eeprom->VDD_th1) / (eeprom->PTAT_th2 - eeprom->PTAT_th1)) * (data->PTATav - eeprom->PTAT_th1));
            vdd_calc_steps = vdd_calc_steps / (1 << eeprom->VddScOff);
            double v_vdd_comp = v_el - vdd_calc_steps;
                // printf("v_vdd_comp: %f\n", v_vdd_comp);

            // Pixel sensitivity
            double v_pixc = ( v_vdd_comp * PCSCALEVAL) / data->pix_c[i][j];
                // printf("pixc: %d\n", data->pix_c[i][j]);
                // printf("v_pixc: %f\n", v_pixc);

            // Find correct temp for this sensor in lookup table and do a bilinear interpolation
            table_row = v_pixc + TABLEOFFSET;
            table_row = table_row >> ADEXPBITS;

            // bilinear interpolation
            vx = ((((double)TempTable[table_row][table_col + 1] - (double)TempTable[table_row][table_col]) * (double)dta) / (double)TAEQUIDISTANCE) + (double)TempTable[table_row][table_col];
            vy = ((((double)TempTable[table_row + 1][table_col + 1] - (double)TempTable[table_row + 1][table_col]) * (double)dta) / (double)TAEQUIDISTANCE) + (double)TempTable[table_row + 1][table_col];
            data->pixelTemps[i][j] = (double)((vy - vx) * ((double)(v_pixc + TABLEOFFSET) - (double)YADValues[table_row]) / (1 << ADEXPBITS) + (double)vx);

            // Apply global offset
            data->pixelTemps[i][j] = data->pixelTemps[i][j] + eeprom->GlobalOff;
            data->pixelTemps[i][j] = data->pixelTemps[i][j] / 10.0 - 273.15;
                // printf("temp: %f\n", data->pixelTemps[i][j]);
        }
    }
}

void HTPA_PixelMasking(HTPA_Data_t *data, HTPA_EEPROM_Data_t *eeprom) {

    uint8_t number_neighbours[24];
    uint32_t temp_defpix[24];

    for (int i = 0; i < eeprom->NrOfDefPix; i++) {
        number_neighbours[i] = 0;
        temp_defpix[i] = 0;
        
        if (eeprom->DeadPixAdr[i] < 512) {
            // top half
            if ((eeprom->DeadPixMask[i] & 1) == 1) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5) - 1][(eeprom->DeadPixAdr[i] % 32)];
            }

            if ((eeprom->DeadPixMask[i] & 2) == 2) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5) - 1][(eeprom->DeadPixAdr[i] % 32) + 1];
            }

            if ((eeprom->DeadPixMask[i] & 4) == 4) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5)][(eeprom->DeadPixAdr[i] % 32) + 1];
            }

            if ((eeprom->DeadPixMask[i] & 8) == 8) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5) + 1][(eeprom->DeadPixAdr[i] % 32) + 1];
            }

            if ((eeprom->DeadPixMask[i] & 16) == 16) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5) + 1][(eeprom->DeadPixAdr[i] % 32)];
            }

            if ((eeprom->DeadPixMask[i] & 32) == 32) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5) + 1][(eeprom->DeadPixAdr[i] % 32) - 1];
            }

            if ((eeprom->DeadPixMask[i] & 64) == 64) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5)][(eeprom->DeadPixAdr[i] % 32) - 1];
            }

            if ((eeprom->DeadPixMask[i] & 128) == 128) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5) - 1][(eeprom->DeadPixAdr[i] % 32) - 1];
            }
        } else {

            // bottom half
            if ((eeprom->DeadPixMask[i] & 1 << 0) == 1 << 0) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5) + 1][(eeprom->DeadPixAdr[i] % 32)];
            }

            if ((eeprom->DeadPixMask[i] & 2) == 2) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5) + 1][(eeprom->DeadPixAdr[i] % 32) + 1];
            }

            if ((eeprom->DeadPixMask[i] & 4) == 4) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5)][(eeprom->DeadPixAdr[i] % 32) + 1];
            }

            if ((eeprom->DeadPixMask[i] & 8) == 8) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5) - 1][(eeprom->DeadPixAdr[i] % 32) + 1];
            }

            if ((eeprom->DeadPixMask[i] & 16) == 16) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5) - 1][(eeprom->DeadPixAdr[i] % 32)];
            }

            if ((eeprom->DeadPixMask[i] & 32) == 32) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5) - 1][(eeprom->DeadPixAdr[i] % 32) - 1];
            }

            if ((eeprom->DeadPixMask[i] & 64) == 64) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5)][(eeprom->DeadPixAdr[i] % 32) - 1];
            }

            if ((eeprom->DeadPixMask[i] & 128) == 128) {
                number_neighbours[i]++;
                temp_defpix[i] = temp_defpix[i] + data->pixelTemps[(eeprom->DeadPixAdr[i] >> 5) + 1][(eeprom->DeadPixAdr[i] % 32) - 1];
            }
        }

        temp_defpix[i] = temp_defpix[i] / number_neighbours[i];
        data->pixelTemps[eeprom->DeadPixAdr[i] >> 5][eeprom->DeadPixAdr[i] % 32] = temp_defpix[i];
    }
}

int HTPA_CaptureData(HTPA_Data_t *data, HTPA_EEPROM_Data_t *eeprom) {
    static uint32_t lastGetVDD = 0;
    if(millis() - lastGetVDD > HTPA_VDD_PERIOD || lastGetVDD == 0) {
        CHECK_ERROR(HTPA_GetPixels(data, true));
        lastGetVDD = millis();
    } else {
        CHECK_ERROR(HTPA_GetPixels(data, false));
    }
    CHECK_ERROR(HTPA_GetElOffsets(data));
    HTPA_SortData(data);
    HTPA_CalculateTemperatures(data, eeprom);
    HTPA_PixelMasking(data, eeprom);
    // HTPA_PrintPixelTemps();
    return HTPA_OK;
}


/*-------------------------------------------------------------------------------*/
/* EEPROM Functions                                                              */
/*-------------------------------------------------------------------------------*/

int HTPA_ReadEEPROM(HTPA_EEPROM_Data_t *eeprom) {
    if (!eeprom) return HTPA_ERR;

    // Read EEPROM parameters
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_PIXC_MIN,     (uint8_t*)&eeprom->PixCmin,        4));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_PIXC_MAX,     (uint8_t*)&eeprom->PixCmax,        4));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_GRADSCALE,    &eeprom->gradScale,                1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_TN,           (uint8_t*)&eeprom->TN,             2));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_EPSILON,      &eeprom->epsilon,                  1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_MBIT_CALIB,   &eeprom->MBIT_calib,               1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_BIAS_CALIB,   &eeprom->BIAS_calib,               1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_CLK_CALIB,    &eeprom->CLK_calib,                1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_BPA_CALIB,    &eeprom->BPA_calib,                1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_PU_CALIB,     &eeprom->PU_calib,                 1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_ARRAYTYPE,    &eeprom->Arraytype,                1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_VDDTH1,       (uint8_t*)&eeprom->VDD_th1,        2));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_VDDTH2,       (uint8_t*)&eeprom->VDD_th2,        2));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_PTAT_GRAD,    (uint8_t*)&eeprom->PTAT_gradient,  4));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_PTAT_OFFSET,  (uint8_t*)&eeprom->PTAT_offset,    4));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_PTAT_TH1,     (uint8_t*)&eeprom->PTAT_th1,       2));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_PTAT_TH2,     (uint8_t*)&eeprom->PTAT_th2,       2));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_VDDSCGRAD,    &eeprom->VddScGrad,                1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_VDDSCOFF,     &eeprom->VddScOff,                 1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_GLOBALOFF,    (uint8_t*)&eeprom->GlobalOff,      1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_GLOBALGAIN,   (uint8_t*)&eeprom->GlobalGain,     2));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_MBIT_USER,    &eeprom->MBIT_user,                1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_BIAS_USER,    &eeprom->BIAS_user,                1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_CLK_USER,     &eeprom->CLK_user,                 1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_BPA_USER,     &eeprom->BPA_user,                 1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_PU_CALIB,     &eeprom->PU_user,                  1));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_DEVICEID,     (uint8_t*)&eeprom->DeviceID,       4));
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_NROFDEFPIX,   &eeprom->NrOfDefPix,               1));

    
    if(eeprom->NrOfDefPix != 0) {
        // --- DeadPixAdr ---
        CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_DEADPIXADDR, (uint8_t*)eeprom->DeadPixAdr, eeprom->NrOfDefPix * 2));
        for (int i = 0; i < eeprom->NrOfDefPix; i++) {
            if (eeprom->DeadPixAdr[i] > 512) { // adaptedAdr:
                eeprom->DeadPixAdr[i] = 1024 + 512 - eeprom->DeadPixAdr[i] + 2 * (eeprom->DeadPixAdr[i] % 32) - 32;
            }
        }
        // --- DeadPixMask ---
        CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_DEADPIXMASK, (uint8_t*)eeprom->DeadPixMask, eeprom->NrOfDefPix));
    }

    //---VddCompGrad---
    // top half
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_VDDCOMPGRAD, (uint8_t*)eeprom->VddCompGrad, 4 * 32 * 2));
    // bottom half (backwards)
    for (int i = 0; i < HTPA_BLOCKS; i++) {
        CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_VDDCOMPGRAD + 0x100 + i * 32 * 2, (uint8_t*)eeprom->VddCompGrad[3 - i], 32 * 2));
    }

    //---VddCompOff---
    // top half
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_VDDCOMPOFF, (uint8_t*)eeprom->VddCompOff, 4 * 32 * 2));
    // bottom half (backwards)
    for (int i = 0; i < HTPA_BLOCKS; i++) {
        CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_VDDCOMPOFF + 0x100 + i * 32 * 2, (uint8_t*)eeprom->VddCompOff[3 - i], 32 * 2));
    }

    // --- ThGrad ---
    // top half
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_THGRAD, (uint8_t*)eeprom->ThGrad, 16 * 32 * 2));
    // bottom half (backwards)
    for (int i = 0; i < HTPA_COLS / 2; i++) {
        CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_THGRAD + 0x400 + i * 32 * 2, (uint8_t*)eeprom->ThGrad[31 - i], 32 * 2));
    }

    // --- ThOffset_ij ---
    // top half
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_THOFFSET, (uint8_t*)eeprom->ThOffset, 16 * 32 * 2));
    // bottom half (backwards)
    for (int i = 0; i < HTPA_COLS / 2; i++) {
        CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_THOFFSET + 0x400 + i * 32 * 2, (uint8_t*)eeprom->ThOffset[31 - i], 32 * 2));
    }

    // --- P ---
    // top half
    CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_P, (uint8_t*)eeprom->P, 16 * 32 * 2));
    // bottom half (backwards)
    for (int i = 0; i < HTPA_COLS / 2; i++) {
        CHECK_ERROR(HTPA_EEPROM_Read(EEPROM_P + 0x400 + i * 32 * 2, (uint8_t*)eeprom->P[31 - i], 32 * 2));
    }

    return HTPA_OK;
}

void HTPA_PrintEEPROM(HTPA_EEPROM_Data_t *eeprom) {
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
    printf("DeviceID: %X\r\n", eeprom->DeviceID);
    printf("NrOfDefPix: %d\r\n", eeprom->NrOfDefPix);
}
