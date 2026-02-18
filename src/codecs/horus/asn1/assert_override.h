#include <stdio.h>
#ifndef ASSERT_OVERRIDE
#define ASSERT_OVERRIDE
extern int assert_value;
#define assert(x) if((x) == 0){assert_value=1;return;}
#define ASSERT_OR_RETURN_FALSE(_Expression) if((_Expression) == 0){assert_value=1;return TRUE;} else {return FALSE;}
#endif