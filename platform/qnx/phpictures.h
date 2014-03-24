/*
 * Copyright 2012-2014, Mike Gorchak.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 */

#ifndef __PHPICTURES_H__
#define __PHPICTURES_H__

#define PH_ICON_WIDTH    48
#define PH_ICON_HEIGHT   48

#define PH_SICON_WIDTH   10
#define PH_SICON_HEIGHT  20

#define PH_SSICON_WIDTH  8
#define PH_SSICON_HEIGHT 8

extern unsigned char openfile_image[PH_ICON_HEIGHT][PH_ICON_WIDTH*4];
extern unsigned char information_image[PH_ICON_HEIGHT][PH_ICON_WIDTH*4];
extern unsigned char settings_image[PH_ICON_HEIGHT][PH_ICON_WIDTH*4];
extern unsigned char previous_image[PH_ICON_HEIGHT][PH_ICON_WIDTH*4];
extern unsigned char next_image[PH_ICON_HEIGHT][PH_ICON_WIDTH*4];
extern unsigned char zoomin_image[PH_ICON_HEIGHT][PH_ICON_WIDTH*4];
extern unsigned char zoomout_image[PH_ICON_HEIGHT][PH_ICON_WIDTH*4];
extern unsigned char search_image[PH_ICON_HEIGHT][PH_ICON_WIDTH*4];
extern unsigned char search_forward_image[PH_SICON_HEIGHT][PH_SICON_WIDTH*4];
extern unsigned char search_backward_image[PH_SICON_HEIGHT][PH_SICON_WIDTH*4];
extern unsigned char search_close_image[PH_SSICON_HEIGHT][PH_SSICON_WIDTH*4];
extern unsigned char search_case_image[PH_SSICON_HEIGHT][PH_SSICON_WIDTH*4];
extern unsigned char key_image[PH_ICON_HEIGHT][PH_ICON_WIDTH*4];
extern unsigned char rotation_image[PH_ICON_HEIGHT][PH_ICON_WIDTH*4];
extern unsigned char properties_image[PH_ICON_HEIGHT][PH_ICON_WIDTH*4];

#endif /* __PHPICTURES_H__ */
