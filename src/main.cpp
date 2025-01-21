#include <TFT_eSPI.h>
#include "driver/i2c.h"
#include "freertos/semphr.h"

#include "htpa.h"
#include "palette.h"

// TFT_eSPI display
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
uint16_t* sprPtr;

// HTPA sensor data
HTPA_Data_t htpa_data;
HTPA_EEPROM_Data_t htpa_eeprom;
SemaphoreHandle_t htpa_mutex;
SemaphoreHandle_t data_ready_sem;

static tRGBcolor *pPalette;
static uint16_t PaletteSteps = 0;

#define SW_VERSION_MAJOR	1
#define SW_VERSION_MINOR	0

#define CALC_MODE_DIRECT		0
#define CALC_MODE_INTERPOL		1
#define CALC_MODE				CALC_MODE_INTERPOL

#define MIN_TEMP				-40
#define MAX_TEMP				300
#define MIN_TEMPSCALE_DELTA		20
#define SCALE_DEFAULT_MIN		10
#define SCALE_DEFAULT_MAX		50
#define AUTOSCALE_MODE

#define dispWidth 				320
#define dispHeight				240

#define termWidth				32
#define termHeight				32

#define blockSize				7

#if (CALC_MODE == CALC_MODE_DIRECT)
    #define imageWidth				(blockSize * termWidth)
    #define imageHeight 			(blockSize * termHeight)
#endif

#if (CALC_MODE == CALC_MODE_INTERPOL)
    #define iSteps					blockSize
    #define INT_MODE		
    #define imageWidth 				(termWidth * iSteps)
    #define	imageHeight 			(termHeight * iSteps)
#endif



#if (CALC_MODE == CALC_MODE_DIRECT)
void DrawImage(HTPA_Data_t* htpa_data, tRGBcolor *pPalette, uint16_t PaletteSize, uint16_t X, uint16_t Y, uint8_t pixelWidth, uint8_t pixelHeight, float minTemp)
{
	spr[0].fillSprite(TFT_BLACK);
	spr[1].fillSprite(TFT_BLACK);
	for (int row = 0; row < termHeight; row++) {
		for (int col = 0; col < termWidth; col++) {

			int16_t colorIdx = (int16_t)(htpa_data->pixelTemps[row][termWidth - col - 1] * 10.0) - (minTemp * 10);

			if (colorIdx < 0)
				colorIdx = 0;
			if (colorIdx >= PaletteSize)
				colorIdx = PaletteSize - 1;

	    	uint16_t color = tft.color565(pPalette[colorIdx].r, pPalette[colorIdx].g, pPalette[colorIdx].b);
			uint8_t sel = row < (termHeight / 2) ? 0 : 1;
			spr[sel].fillRect(col * pixelWidth + X, row * pixelHeight + Y, pixelWidth, pixelHeight, color);
		}
		if(row == (termHeight / 2 - 1)) tft.pushImageDMA(0, 0, imageWidth, imageHeight / 2, sprPtr[0]);
		if(row == (termHeight - 1)) tft.pushImageDMA(0, imageHeight / 2, imageWidth, imageHeight / 2, sprPtr[1]);
	}
}
#endif

#if (CALC_MODE == CALC_MODE_INTERPOL)
void DrawHQImage(HTPA_Data_t* htpa_data, tRGBcolor *pPalette, uint16_t PaletteSize, uint16_t X, uint16_t Y, float minTemp)
{
    spr.fillSprite(TFT_BLACK);
    
    for (int row = 0; row < imageHeight; row++)
    {
        int baseRow = row / iSteps;
        int nextRow = (baseRow + 1) >= termHeight ? baseRow : baseRow + 1;
        int stepY = row % iSteps;
        
        for (int col = 0; col < imageWidth; col++)
        {
            int baseCol = col / iSteps;
            int nextCol = (baseCol + 1) >= termWidth ? baseCol : baseCol + 1;
            int stepX = col % iSteps;
            
            // Интерполяция температуры между четырьмя соседними точками
            float temp1 = htpa_data->pixelTemps[baseRow][termWidth - baseCol - 1];
            float temp2 = htpa_data->pixelTemps[baseRow][termWidth - nextCol - 1];
            float temp3 = htpa_data->pixelTemps[nextRow][termWidth - baseCol - 1];
            float temp4 = htpa_data->pixelTemps[nextRow][termWidth - nextCol - 1];
            
            // Билинейная интерполяция
            float fx = (float)stepX / iSteps;
            float fy = (float)stepY / iSteps;
            
            float temp = temp1 * (1 - fx) * (1 - fy) +
                        temp2 * fx * (1 - fy) +
                        temp3 * (1 - fx) * fy +
                        temp4 * fx * fy;
            
            int16_t colorIdx = (int16_t)(temp * 10.0) - (minTemp * 10);
            
            if (colorIdx < 0)
                colorIdx = 0;
            if (colorIdx >= PaletteSize)
                colorIdx = PaletteSize - 1;
                
            uint16_t color = tft.color565(pPalette[colorIdx].r, pPalette[colorIdx].g, pPalette[colorIdx].b);
            uint8_t sel = row < (imageHeight / 2) ? 0 : 1;
            spr.drawPixel(col + X, row + Y, color);
        }
    }
    tft.pushImageDMA(0, 0, imageWidth, imageHeight, sprPtr);
}
#endif

void DrawScale(uint16_t X, uint16_t Y, uint16_t Width, uint16_t Height)
{
	tRGBcolor *Buffer = getPalette(PALETTE_IRON, Height);
	if (!Buffer)
	    return;

    for (int i = 0; i < Height; i++)
	{
		uint16_t color = tft.color565(Buffer[i].r, Buffer[i].g, Buffer[i].b);
		tft.fillRect(X, Y + Height - i - 1, Width, 1, color);
	}

    freePalette(Buffer);
}

void DrawCenterTempColor(uint16_t cX, uint16_t cY, float Temp, tRGBcolor *color)
{
	uint8_t offMin = 5;
	uint8_t offMax = 10;
	uint8_t offTwin = 1;

	tft.drawLine(cX - offTwin, cY - offMin, cX - offTwin, cY - offMax, tft.color565(color->r, color->g, color->b));
	tft.drawLine(cX + offTwin, cY - offMin, cX + offTwin, cY - offMax, tft.color565(color->r, color->g, color->b));

	tft.drawLine(cX - offTwin, cY + offMin, cX - offTwin, cY + offMax, tft.color565(color->r, color->g, color->b));
	tft.drawLine(cX + offTwin, cY + offMin, cX + offTwin, cY + offMax, tft.color565(color->r, color->g, color->b));

	tft.drawLine(cX - offMin, cY - offTwin, cX - offMax, cY - offTwin, tft.color565(color->r, color->g, color->b));
	tft.drawLine(cX - offMin, cY + offTwin, cX - offMax, cY + offTwin, tft.color565(color->r, color->g, color->b));

	tft.drawLine(cX + offMin, cY - offTwin, cX + offMax, cY - offTwin, tft.color565(color->r, color->g, color->b));
	tft.drawLine(cX + offMin, cY + offTwin, cX + offMax, cY + offTwin, tft.color565(color->r, color->g, color->b));

	if ((Temp > -100) && (Temp < 500)) {
        char str[16] = {0};
		sprintf(str, "%.1f", Temp);
        tft.setTextColor(tft.color565(color->r, color->g, color->b));
        tft.drawString(str, cX + 8, cY + 8, 2);
    }
}

void DrawCenterTemp(uint16_t X, uint16_t Y, uint16_t Width, uint16_t Height, float Temp)
{
	uint16_t cX = (Width >> 1) + X;
	uint16_t cY = (Height >> 1) + Y;
	tRGBcolor colorTFT_BLACK = {0, 0, 0};
	tRGBcolor colorTFT_WHITE = {255, 255, 255};

	DrawCenterTempColor(cX + 1, cY + 1, Temp, &colorTFT_BLACK);
	DrawCenterTempColor(cX, cY, Temp, &colorTFT_WHITE);
}

void DrawBattery(uint16_t X, uint16_t Y, float capacity)
{
	uint16_t Color = TFT_GREEN;
	if (capacity < 80)
		Color = tft.color565(249, 166, 2);
	if (capacity < 50)
		Color = TFT_RED;

	tft.drawRect(X, Y, X + 17, Y + 9, TFT_WHITE);
	tft.fillRect(X + 17, Y + 2, X + 19, Y + 6, TFT_WHITE);

	tft.fillRect(X + 12, Y + 2, X + 15, Y + 7, capacity < 80 ? TFT_BLACK : Color);
	tft.fillRect(X + 7, Y + 2, X + 10, Y + 7, capacity < 50 ? TFT_BLACK : Color);
	tft.fillRect(X + 2, Y + 2, X + 5, Y + 7, capacity < 25 ? TFT_BLACK : Color);
}
//==============================================================================


// Task for HTPA sensor reading (Core 0)
void htpaSensorTask(void *pvParameters) {
    while(1) {
        if (HTPA_CaptureData(&htpa_data, &htpa_eeprom) == HTPA_OK) {
            xSemaphoreGive(data_ready_sem);
        } else {
            printf("Failed Capture Data!\r\n");
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

// Task for display and interpolation (Core 1)
void displayTask(void *pvParameters) {
    uint32_t frameCount = 0;
    uint32_t lastFPSCheck = 0;
    float minTemp = 0;
    float maxTemp = 0;
    float minTempNew = SCALE_DEFAULT_MIN;
    float maxTempNew = SCALE_DEFAULT_MAX;

	DrawScale(dispWidth - 52, 0, 50, imageHeight);

    while(1) {
        if(xSemaphoreTake(data_ready_sem, pdMS_TO_TICKS(100)) == pdTRUE) {
			xSemaphoreTake(htpa_mutex, portMAX_DELAY);
            #if (CALC_MODE == CALC_MODE_DIRECT)
                if (pPalette)
                    DrawImage(&htpa_data, pPalette, PaletteSteps, 0, 0, blockSize, blockSize, minTemp);
            #endif

            #if (CALC_MODE == CALC_MODE_INTERPOL)
                if (pPalette)
                    DrawHQImage(&htpa_data, pPalette, PaletteSteps, 0, 0, minTemp);
            #endif

            float minT = 300;
            float maxT = -40;
            
            for (uint8_t i = 0; i < termHeight; i++) {
                for(uint8_t j = 0; j < termWidth; j++) {
                    float pix_temp = (float)htpa_data.pixelTemps[i][j];
                    maxT = max(maxT, pix_temp);
                    minT = min(minT, pix_temp);
                }
            }

            double MainTemp =
                htpa_data.pixelTemps[(termHeight >> 1) - 1][(termWidth >> 1) - 1] +
                htpa_data.pixelTemps[(termHeight >> 1) - 1][(termWidth >> 1)] +
                htpa_data.pixelTemps[(termHeight >> 1)][(termWidth >> 1) - 1] +
                htpa_data.pixelTemps[(termHeight >> 1)][(termWidth >> 1)];
            MainTemp /= 4;
            xSemaphoreGive(htpa_mutex);

            maxT = min(maxT, (float)MAX_TEMP);
            minT = max(minT, (float)MIN_TEMP);

            DrawCenterTemp(0, 0, imageWidth, imageHeight, MainTemp);
			
			if ((minTempNew != minTemp) || (maxTempNew != maxTemp)) {
					float Delta = maxTempNew - minTempNew;
					if (Delta < MIN_TEMPSCALE_DELTA) {
						minTempNew -= (MIN_TEMPSCALE_DELTA - Delta) / 2;
						maxTempNew += (MIN_TEMPSCALE_DELTA - Delta) / 2;
					}

					minTemp = minTempNew;
					maxTemp = maxTempNew;

					freePalette(pPalette);
					PaletteSteps = (uint16_t)((maxTemp - minTemp) * 10);
					pPalette = getPalette(PALETTE_IRON, PaletteSteps);
					char str[16] = {0};
					int16_t TextWidth = 0;

					sprintf(str, "%2.0f", maxTemp);
					TextWidth = tft.textWidth(str, 2);
					tft.setTextColor(TFT_BLACK, TFT_WHITE);
					tft.fillRect(dispWidth - 52 + (50 - TextWidth) / 2, 2, 50, tft.fontHeight(2), TFT_WHITE);
					tft.drawString(str, dispWidth - 52 + (50 - TextWidth) / 2, 2, 2);

					sprintf(str, "%2.0f", minTemp);
					TextWidth = tft.textWidth(str, 2);
					tft.fillRect(dispWidth - 52 + (50 - TextWidth) / 2, imageHeight - tft.fontHeight(2), 50, tft.fontHeight(2), TFT_BLACK);
					tft.setTextColor(TFT_WHITE, TFT_BLACK);
					tft.drawString(str, dispWidth - 52 + (50 - TextWidth) / 2, imageHeight - tft.fontHeight(2), 2);
				}

            frameCount++;
            uint32_t currentMillis = millis();
            if (currentMillis - lastFPSCheck >= 1000) {
                float current_FPS = frameCount * 1000.0f / (currentMillis - lastFPSCheck);
                frameCount = 0;
                lastFPSCheck = currentMillis;

                tft.setTextColor(tft.color565(32, 32, 192), TFT_BLACK);
                tft.setCursor(1, 228);
                tft.printf("MIN: %2.1f ", minT);

                tft.setTextColor(tft.color565(192, 32, 32), TFT_BLACK);
                tft.setCursor(69, 228);
                tft.printf("MAX: %2.1f ", maxT);

                tft.setTextColor(TFT_GREEN, TFT_BLACK);
                tft.setCursor(138, 228);
                tft.printf("FPS: %2.1f", current_FPS);
            }

            #ifdef AUTOSCALE_MODE
                minTempNew = minT;
                maxTempNew = maxT;
            #endif
        }
    }
}

//==============================================================================
void setup() {
    htpa_mutex = xSemaphoreCreateMutex();
    data_ready_sem = xSemaphoreCreateBinary();

    tft.init();
    tft.initDMA();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);

    sprPtr = (uint16_t*)spr.createSprite(imageWidth, imageHeight);
    spr.setViewport(0, 0, imageWidth, imageHeight);
    spr.setTextDatum(MC_DATUM);
    tft.startWrite();

    if (HTPA_Init(&htpa_data, &htpa_eeprom, I2C_NUM_0, GPIO_NUM_16, GPIO_NUM_4)) {
        printf("Failed init HTPA sensor!\r\n");
        return;
    }

    xTaskCreatePinnedToCore(
        htpaSensorTask,
        "HTPA_Task",
        4096,
        NULL,
        1,
        NULL,
        0  // Core 0
    );

    xTaskCreatePinnedToCore(
        displayTask,
        "Display_Task",
        8192,
        NULL,
        1,
        NULL,
        1  // Core 1
    );
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}