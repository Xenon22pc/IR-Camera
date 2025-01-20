#include <TFT_eSPI.h>
#include "driver/i2c.h"
#include <math.h>
#include "htpa.h"
#include "palette.h"

// TFT_eSPI display
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr[2] = {TFT_eSprite(&tft), TFT_eSprite(&tft)};
uint16_t* sprPtr[2];

// HTPA sensor data
HTPA_Data_t htpa_data;
HTPA_EEPROM_Data_t htpa_eeprom;

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
    #define HQtermWidth				((termWidth - 1) * iSteps)
    #define HQtermHeight			((termHeight - 1) * iSteps)

    #define imageWidth 				HQtermWidth
    #define	imageHeight 			HQtermHeight
#endif

static int16_t *TermoImage16;
#if (CALC_MODE == CALC_MODE_INTERPOL)
    static int16_t *TermoHqImage16;
#endif

static tRGBcolor *pPalette;
static uint16_t PaletteSteps = 0;

#if (CALC_MODE == CALC_MODE_DIRECT)
void DrawImage(int16_t *pImage, tRGBcolor *pPalette, uint16_t PaletteSize, uint16_t X, uint16_t Y, uint8_t pixelWidth, uint8_t pixelHeight, float minTemp)
{
    int cnt = 0;
	for (int row = 0; row < termHeight; row++) {
		for (int col = 0; col < termWidth; col++, cnt++) {

			int16_t colorIdx = pImage[cnt] - (minTemp * 10);

			if (colorIdx < 0)
				colorIdx = 0;
			if (colorIdx >= PaletteSize)
				colorIdx = PaletteSize - 1;

	    	uint16_t color = tft.color565(pPalette[colorIdx].r, pPalette[colorIdx].g, pPalette[colorIdx].b);
			tft.fillRect(col * pixelWidth + X, row * pixelHeight + Y, pixelWidth, pixelHeight, color);
		}
	}
}
#endif

#if (CALC_MODE == CALC_MODE_INTERPOL)
void DrawHQImage(int16_t *pImage, tRGBcolor *pPalette, uint16_t PaletteSize, uint16_t X, uint16_t Y, float minTemp)
{
	int cnt = 0;
	for (int row = 0; row < HQtermHeight; row++)
	{
		for (int col = 0; col < HQtermWidth; col++, cnt++)
		{
			int16_t colorIdx = pImage[cnt] - (minTemp * 10);

			if (colorIdx < 0)
				colorIdx = 0;
			if (colorIdx >= PaletteSize)
				colorIdx = PaletteSize - 1;

	    	uint16_t color = tft.color565(pPalette[colorIdx].r, pPalette[colorIdx].g, pPalette[colorIdx].b);
	    	tft.drawPixel((HQtermWidth - col - 1) + X, row + Y, color);
		}
	}
}
#endif

void DrawScale(uint16_t X, uint16_t Y, uint16_t Width, uint16_t Height, float minTemp, float maxTemp)
{
	tRGBcolor *Buffer = getPalette(PALETTE_IRON, Height);
	if (!Buffer)
	    return;

    for (int i = 0; i < Height; i++)
	{
		uint16_t color = tft.color565(Buffer[i].r, Buffer[i].g, Buffer[i].b);
		tft.drawFastHLine(X, Y + Height - i - 1, Width, color);
	}

    freePalette(Buffer);

    char str[16] = {0};
    int16_t TextWidth = 0;

    sprintf(str, "%.0f", maxTemp);
    TextWidth = tft.textWidth(str, 1);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawString(str, X + (Width - TextWidth) / 2, Y + 2, 1);

    sprintf(str, "%.0f", minTemp);
    TextWidth = tft.textWidth(str, 1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(str, X + (Width - TextWidth) / 2, Y + Height - 10, 1);
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
        tft.setTextColor(tft.color565(color->r, color->g, color->b), TFT_BLACK);
        tft.setCursor(cX + 8, cY + 8);
        tft.printf("%.1f", Temp);
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

#if (CALC_MODE == CALC_MODE_INTERPOL)
void InterpolateImage(int16_t *pImage, int16_t *pHdImage)
{
	for (uint16_t row = 0; row < termHeight; row++)
	{
		for (uint16_t col = 0; col < (termWidth - 1); col++)
		{
			uint16_t ImageIdx = row * termWidth + col;
			int16_t tempStart = pImage[ImageIdx];
			int16_t tempEnd = pImage[ImageIdx + 1];

			for (uint16_t step = 0; step < iSteps; step++)
			{
#ifdef INT_MODE
				uint32_t Idx = (row * HQtermWidth + col) * iSteps + step;
				pHdImage[Idx] = tempStart * (iSteps - step) / iSteps + tempEnd * step / iSteps;
#else
				float n = (float)step / (float) (iSteps - 1);
				uint32_t Idx = (row * HQtermWidth + col) * iSteps + step;
				pHdImage[Idx] = tempStart * (1.0f - n) + tempEnd * n;
#endif
			}
		}
	}

	for (uint16_t col = 0; col < HQtermWidth; col++)
	{
		for (uint16_t row = 0; row < termHeight; row++)
		{
			int16_t tempStart = pHdImage[row * iSteps * HQtermWidth + col];
			int16_t tempEnd = pHdImage[(row + 1) * iSteps * HQtermWidth + col];

			for (uint16_t step = 1; step < iSteps; step++)
			{
#ifdef INT_MODE
				uint32_t Idx = (row * iSteps + step) * HQtermWidth + col;
				pHdImage[Idx] = tempStart * (iSteps - step) / iSteps + tempEnd * step / iSteps;
#else
				float n = (float)step / (float) (iSteps - 1);
				uint32_t Idx = (row * iSteps + step) * HQtermWidth + col;
				pHdImage[Idx] = tempStart * (1.0f - n) + tempEnd * n;
#endif
			}
		}
	}
}
#endif

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


//==============================================================================
void setup()
{
	int result;
    float minTemp = 0;
    float maxTemp = 0;
    float minTempNew = SCALE_DEFAULT_MIN;
    float maxTempNew = SCALE_DEFAULT_MAX;

    // Used for fps measuring
    uint32_t frameCount = 0;
    uint32_t lastFPSCheck = 0;

    tft.init();
    tft.initDMA();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);

    // sprPtr[0] = (uint16_t*)spr[0].createSprite(imageWidth, imageHeight / 2);
    // sprPtr[1] = (uint16_t*)spr[1].createSprite(imageWidth, imageHeight / 2);
    // spr[1].setViewport(0, -imageHeight / 2, imageWidth, imageHeight);
    // spr[0].setTextDatum(MC_DATUM);
    // spr[1].setTextDatum(MC_DATUM);

    // TODO - add battery voltage 
    // uint32_t BatteryVoltage = getBatteryVoltage();
    // printf(" VBAT=%d mV\n", BatteryVoltage);

    if (HTPA_Init(&htpa_data, &htpa_eeprom, I2C_NUM_0, GPIO_NUM_16, GPIO_NUM_4)) {
        printf("Failed init HTPA sensor!\r\n");
    } else {
        printf("Init HTPA sensor done\r\n");
    }

    TermoImage16 = (int16_t*)heap_caps_malloc((termWidth * termHeight) << 1, MALLOC_CAP_8BIT);
    if (!TermoImage16) printf("TermoImage16 malloc error\r\n");

#if (CALC_MODE == CALC_MODE_INTERPOL)
    TermoHqImage16 = (int16_t*)heap_caps_malloc((HQtermWidth * HQtermHeight) << 1, MALLOC_CAP_8BIT);
    if (!TermoHqImage16) printf("TermoHqImage16 malloc error\r\n");
#endif

    lastFPSCheck = millis();

    while (1)
    {
        if ((minTempNew != minTemp) || (maxTempNew != maxTemp))
        {
        	float Delta = maxTempNew - minTempNew;
        	if (Delta < MIN_TEMPSCALE_DELTA)
        	{
        		minTempNew -= (MIN_TEMPSCALE_DELTA - Delta) / 2;
        		maxTempNew += (MIN_TEMPSCALE_DELTA - Delta) / 2;
        	}

        	minTemp = minTempNew;
        	maxTemp = maxTempNew;

        	freePalette(pPalette);
        	PaletteSteps = (uint16_t)((maxTemp - minTemp) * 10);
        	pPalette = getPalette(PALETTE_IRON, PaletteSteps);
        	DrawScale(imageWidth + 2, (dispHeight - imageHeight) >> 1, dispWidth - imageWidth - 2, imageHeight, minTemp, maxTemp);
        }

        if (HTPA_CaptureData(&htpa_data, &htpa_eeprom)) {
            printf("Failed Capture Data!\r\n");
        }

        // TODO - add battery voltage
        // float VBAT = ((float)getBatteryVoltage()) / 1000;
    	// float capacity = VBAT * 125 - 400;
    	// if (capacity > 100)
    	// 	capacity = 100;
    	// if (capacity < 0)
    	// 	capacity = 0;
    	// dispcolor_printf_Bg(137, 228, FONTID_6X8M, tft.color565(160, 96, 0), TFT_BLACK, "VBAT=%.2fV ", VBAT);
    	// DrawBattery(300, 2, capacity++);

        for (uint8_t i = 0; i < termHeight; i++) {
            for (uint8_t j = 0; j < termWidth; j++) {
                TermoImage16[i * termHeight + j] = (int16_t)(htpa_data.pixelTemps[i][j] * 10.0);
            }
        }

#if (CALC_MODE == CALC_MODE_DIRECT)
    	if (pPalette)
    		DrawImage(TermoImage16, pPalette, PaletteSteps, 0, 0, blockSize, blockSize, minTemp);
#endif

#if (CALC_MODE == CALC_MODE_INTERPOL)
    	InterpolateImage(TermoImage16, TermoHqImage16);
    	if (pPalette)
    		DrawHQImage(TermoHqImage16, pPalette, PaletteSteps, 0, (dispHeight - imageHeight) >> 1, minTemp);
#endif

    	double MainTemp =
    			htpa_data.pixelTemps[(termHeight >> 1) - 1][(termWidth >> 1) - 1] +
    			htpa_data.pixelTemps[(termHeight >> 1) - 1][(termWidth >> 1)] +
    			htpa_data.pixelTemps[(termHeight >> 1)][(termWidth >> 1) - 1] +
				htpa_data.pixelTemps[(termHeight >> 1)][(termWidth >> 1)];
    	MainTemp /= 4;
    	DrawCenterTemp(0, (dispHeight - imageHeight) >> 1, imageWidth, imageHeight, MainTemp);

        float minT = 300;
        float maxT = -40;
    	for (uint8_t i = 0; i < termHeight; i++) {
            for(uint8_t j = 0; j < termWidth; j++) {
                float pix_temp = (float)htpa_data.pixelTemps[i][j];
                if (maxT < pix_temp)
                    maxT = pix_temp;
                if (minT > pix_temp)
                    minT = pix_temp;
    	    }
        }
		if (maxT > MAX_TEMP)
			maxT = MAX_TEMP;
		if (minT < MIN_TEMP)
			minT = MIN_TEMP;

        char str[16] = {0};

        frameCount++;
        uint32_t currentMillis = millis();
        if (currentMillis - lastFPSCheck >= 1000) {
            float current_FPS = frameCount * 1000.0f / (currentMillis - lastFPSCheck);
            frameCount = 0;
            lastFPSCheck = currentMillis;

            tft.setTextColor(tft.color565(32, 32, 192), TFT_BLACK);
            tft.setCursor(1, 228);
            tft.printf("MIN=%2.1f ", minT);

            tft.setTextColor(tft.color565(192, 32, 32), TFT_BLACK);
            tft.setCursor(69, 228);
            tft.printf("MAX=%2.1f ", maxT);

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

void loop() {
}