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

#ifndef __PHPDF_H__
#define __PHPDF_H__

#include "phmupdf.h"

int phpdf_openfile(photon_mupdf_t* app);
int phpdf_closefile(photon_mupdf_t* app);
int phpdf_load_page(photon_mupdf_t* app, int pageno, int slotno);
int phpdf_destroy_page_cache(photon_mupdf_t* app);
int phpdf_find_mostdistance_page(photon_mupdf_t* app, int pageno);

#endif /* __PHPDF_H__ */
