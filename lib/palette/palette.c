#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "palette.h"

static tRGBcolor *getIronPalette(uint16_t steps)
{
    if (steps % 4)
    	steps = (steps / 4) * 4 + 4;

    tRGBcolor *Buffer = heap_caps_malloc(steps * sizeof(tRGBcolor), MALLOC_CAP_8BIT);
	if (!Buffer)
	    return 0;

	uint16_t partSize = steps >> 2;
	tRGBcolor KeyColors[5] =
	{
			{0x00, 0x00, 0x00},
			{0x20, 0x00, 0x8C},
			{0xCC, 0x00, 0x77},
			{0xFF, 0xD7, 0x00},
			{0xFF, 0xFF, 0xFF}
	};

	tRGBcolor *pPalette = Buffer;
	for (uint8_t part = 0; part < 4; part++)
	{
	    for (uint16_t step = 0; step < partSize; step++)
		{
		    float n = (float)step / (float) (partSize - 1);

	    	pPalette->r = ((float)KeyColors[part].r) * (1.0f - n) + ((float)KeyColors[part + 1].r) * n;
	    	pPalette->g = ((float)KeyColors[part].g) * (1.0f - n) + ((float)KeyColors[part + 1].g) * n;
	    	pPalette->b = ((float)KeyColors[part].b) * (1.0f - n) + ((float)KeyColors[part + 1].b) * n;

	    	pPalette++;
		}
	}

	return Buffer;
}

tRGBcolor *getPalette(uint8_t paletteNum, uint16_t steps)
{
	switch (paletteNum)
	{
		case PALETTE_IRON:
			return getIronPalette(steps);
		default:
			return 0;
	}
}

void freePalette(tRGBcolor *pPalette)
{
	if (pPalette)
		heap_caps_free(pPalette);
}
