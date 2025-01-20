#ifndef MAIN_PALETTE_PALETTE_H_
#define MAIN_PALETTE_PALETTE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} tRGBcolor;

#define PALETTE_IRON	0

tRGBcolor *getPalette(uint8_t paletteNum, uint16_t steps);
void freePalette(tRGBcolor *pPalette);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_PALETTE_PALETTE_H_ */
