// Based on HamLib's locator routines
// OK1TE 2018-10

#include "locator.h"
#include "config.h"

const static uint8_t loc_char_range[] = { 18, 10, 24, 10, 24, 10 };
const float precision = 1E+7;

uint8_t longlat2locator(int32_t longitude, int32_t latitude, char locator[]) {
	if (!locator)
		return 0;

	for (uint8_t x_or_y = 0; x_or_y < 2; ++x_or_y) {
		float ordinate = ((x_or_y == 0) ? (longitude / 2) / precision : latitude / precision) + 90;
		uint32_t divisions = 1;

		for (uint8_t pair = 0; pair < PAIR_COUNT; ++pair) {
			divisions *= loc_char_range[pair];
			const float square_size = 180.0 / divisions;

			uint8_t locvalue = (uint8_t) (ordinate / square_size);
			ordinate -= square_size * locvalue;
			locvalue += (loc_char_range[pair] == 10) ? '0':'A';
			locator[pair * 2 + x_or_y] = locvalue;
		}
	}

	locator[PAIR_COUNT * 2] = 0;

	return 1;
}
