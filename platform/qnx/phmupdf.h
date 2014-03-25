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

#ifndef __PHMUPDF_H__
#define __PHMUPDF_H__

#include <limits.h>

#include <Ph.h>
#include <Pt.h>
#include <photon/PtProto.h>
#include <photon/PkKeyDef.h>

#include "mupdf/fitz.h"
#include "mupdf/pdf.h"
#include "mupdf/xps.h"
#include "mupdf/cbz.h"
#include "mupdf/img.h"

#include "phpictures.h"

#define PHMUPDF_MINIMUM_WIDTH         (832)
#define PHMUPDF_MINIMUM_HEIGHT        (480)
#define PHMUPDF_NAVIBAR_WIDTH         (PHMUPDF_MINIMUM_WIDTH-84)
#define PHMUPDF_NAVIBAR_HEIGHT        (PH_ICON_HEIGHT+8)
#define PHMUPDF_SEARCHBAR_WIDTH       (180)
#define PHMUPDF_SEARCHBAR_HEIGHT      (36)
#define PHMUPDF_SCROLLBAR_WIDTH       (20)

#define PHW_WINDOW_CHANGE_POSITION    0x00000001
#define PHW_WINDOW_CHANGE_SIZE        0x00000002
#define PHW_WINDOW_EXIT_REQUESTED     0x00000004
#define PHW_WINDOW_MAXIMIZE_EVENT     0x00000008

/* Maximum images in offsreen cache */
#define PHMUPDF_RENDER_MAX_IMAGES     16

/* Space in pixels between pages on the screen */
#define PHMUPDF_GAP_BETWEEN_PAGES     16

/* PhMuPDF default window title with version */
#define PHMUPDF_DEFAULT_TITLE         "PhMuPDF 1.3"

typedef struct phmupdf_page
{
    fz_page*         page;
    fz_display_list* page_list;
    fz_rect          page_bbox;
    fz_rect          page_bbox_rot;
    fz_irect         page_bbox_draw;
    fz_matrix        page_ctm;
    fz_text_sheet*   page_sheet;
    fz_text_page*    page_text;
    fz_link*         page_links;
    float            page_zoom;
} phmupdf_page_t;

typedef struct photon_window
{
    unsigned int event_flags;
    PhDim_t      old_dimension;
    PhDim_t      dimension;
    PhPoint_t    position;
    PhPoint_t    old_position;
    char         title[1024];
} photon_window_t;

typedef struct phmupdf_image
{
    PhImage_t*            page_image_phi;
    PdOffscreenContext_t* page_image_off;
    int                   pageno;
} phmupdf_image_t;

typedef struct phmupdf_settings
{
    unsigned int cache_size;
    int          bw_render;
    int          off_render;
    int          fast_search;
    int          fast_render;
    char         lastpath[PATH_MAX+NAME_MAX+1];
    int          aa_level;
} phmupdf_settings_t;

#define PHMUPDF_ZOOM_FIT_WIDTH          -1
#define PHMUPDF_ZOOM_FIT_PAGE           -2

#define PHMUPDF_JUMP_STACK_SIZE         32

#define PHMUPDF_SEARCH_MAX_LENGTH       512

#define PHMUPDF_SELECTION_MAX_LENGTH    4096

typedef struct photon_mupdf
{
    fz_context*      pdfctx;
    fz_document*     pdfdoc;
    fz_outline*      pdf_outline;
    phmupdf_page_t*  pdf_pages;
    int              pdf_pagecount;
    uint64_t         pdf_totallines;
    int              pdf_currpage;
    int              pdf_zoom;
    int              pdf_zoommax;
    int              pdf_zoommin;
    int              pdf_opened;
    int              pdf_password_tries;
    int              selection;
    fz_point         selection_start;
    fz_point         selection_stop;
    fz_irect         selection_box[PHMUPDF_SELECTION_MAX_LENGTH];
    unsigned int     selection_boxes;
    unsigned int     selection_page;

    PtFileSelectionInfo_t fileinfo;
    char             filename[PATH_MAX+NAME_MAX+1];
    int              filename_set;
    int              fileopen_dialog;

    /* Settings stuff */
    phmupdf_settings_t settings;

    /* Render stuff */
    unsigned long    display_format;
    phmupdf_image_t  page_image[PHMUPDF_RENDER_MAX_IMAGES];
    int              last_x;
    int              last_y;
    unsigned int     jump_stack[PHMUPDF_JUMP_STACK_SIZE];
    int              jump_stack_ptr;
    int              jump_stack_last_valid;
    int              last_cursor_type;
    float            rotation;

    photon_window_t  window;
    PtWidget_t*      phwindow;
    /* About window data */
    PtWidget_t*      phwindow_about;
    PtBlockedList_t* about_bl;
    /* Settings window data */
    PtWidget_t*      phwindow_settings;
    PtWidget_t*      phsettings_fast_search;
    PtWidget_t*      phsettings_fs_low_mem;
    PtWidget_t*      phsettings_fast_search_group;
    PtWidget_t*      phsettings_fast_render;
    PtWidget_t*      phsettings_fr_low_mem;
    PtWidget_t*      phsettings_bw_render;
    PtWidget_t*      phsettings_off_render;
    PtWidget_t*      phsettings_aa_level;
    PtBlockedList_t* settings_bl;
    /* Rotation window data */
    PtBlockedList_t* rotation_bl;
    PtWidget_t*      rotation_dialog;
    PtWidget_t*      rotation_angle;
    /* Properties window data */
    PtBlockedList_t* properties_bl;
    PtWidget_t*      properties_dialog;
    /* Search panel data */
    PtWidget_t*      phsearch_shadow;
    PtWidget_t*      phsearch_pane;
    PtWidget_t*      phsearch_text;
    PtWidget_t*      phsearch_forward;
    PtWidget_t*      phsearch_backward;
    PtWidget_t*      phsearch_close;
    PtWidget_t*      phsearch_case;
    PhImage_t*       search_forward_img;
    PhImage_t*       search_backward_img;
    PhImage_t*       search_close_img;
    PhImage_t*       search_case_img;
    int              case_search;
    char             last_search[PHMUPDF_SEARCH_MAX_LENGTH];
    int              search_hit;
    int              search_hit_length;
    int              search_hit_page;
    int              display_found;
    fz_irect         display_box[PHMUPDF_SEARCH_MAX_LENGTH];
    unsigned int     display_boxes;
    unsigned int     display_page;
    /* Main window data */
    PtWidget_t*      phoscontainer;
    PtWidget_t*      phcontent;
    PtWidget_t*      phpagescroll;
    /* Navigation bar resources */
    PtWidget_t*      phnavibar;
    PtWidget_t*      phnavibar_shadow;
    PtWidget_t*      nbopenfile;
    PhImage_t*       nbopenfile_img;
    PtWidget_t*      nbinformation;
    PhImage_t*       nbinformation_img;
    PtWidget_t*      nbsettings;
    PhImage_t*       nbsettings_img;
    PtWidget_t*      nbprevious;
    PhImage_t*       nbprevious_img;
    PtWidget_t*      nbnext;
    PhImage_t*       nbnext_img;
    PtWidget_t*      nbpage;
    PtWidget_t*      nbpages;
    PtWidget_t*      nbsearch;
    PhImage_t*       nbsearch_img;
    PtWidget_t*      nbzoomin;
    PhImage_t*       nbzoomin_img;
    PtWidget_t*      nbzoomout;
    PhImage_t*       nbzoomout_img;
    PtWidget_t*      nbzoomlevel;
    PtWidget_t*      nbrotation;
    PhImage_t*       nbrotation_img;
    PtWidget_t*      nbproperties;
    PhImage_t*       nbproperties_img;
} photon_mupdf_t;

#endif /* __PHMUPDF_H__ */
