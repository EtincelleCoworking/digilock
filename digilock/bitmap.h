//
//  bitmap.h
//  digilock
//
//  Created by System Administrator on 12/29/14.
//  Copyright (c) 2014 etincelle. All rights reserved.
//

#ifndef digilock_bitmap_h
#define digilock_bitmap_h

#ifdef __cplusplus
extern "C" {
#endif


int write_bmp(const char *filename, int width, int height, char *rgb);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
