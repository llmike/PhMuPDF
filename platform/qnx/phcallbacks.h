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

#ifndef __PHCALLBACKS_H__
#define __PHCALLBACKS_H__

#include <limits.h>
#include <libgen.h>

#include <Ph.h>
#include <Pt.h>
#include <photon/PtProto.h>
#include <photon/PkKeyDef.h>

int phmupdf_change_zoom_level(photon_mupdf_t* app);
void phmupdf_search_recreate_boxes(photon_mupdf_t* app);

void phmupdf_content_find_link(photon_mupdf_t* app, int x, int y, int buttons);
void phmupdf_content_find_text(photon_mupdf_t* app, int x, int y, int buttons);

int phmupdf_main_callback_destroyed(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_main_callback_raw(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_content_callback_raw(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_information_callback_ok(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_information_callback_destroyed(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_information_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_openfile_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_pagecheck_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_pageselect_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_pageraw_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_zoomcheck_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_zoomselect_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_zoomraw_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_zoomin_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_zoomout_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_rotation_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_properties_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_scrollbar_move_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_scrollbar_callback_raw(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_previous_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_next_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_search_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);
int phmupdf_settings_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info);

void phmupdf_draw_content(PtWidget_t* widget, PhTile_t* damage);

#endif /* __PHCALLBACKS_H__ */
