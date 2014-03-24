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

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
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

#include "phmupdf.h"
#include "phpictures.h"
#include "phcallbacks.h"
#include "phpdf.h"

photon_mupdf_t application;

int create_window(photon_mupdf_t* app)
{
    PtArg_t         winargs[64];
    uint32_t        winargc=0;
    int32_t         status;
    PhPoint_t       temp_point;
    PhDim_t         temp_dim;
    PtCallback_t    temp_cb;
    PtCallback_t    temp_cb2;
    PtRawCallback_t raw_cb;

    PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_TITLE, app->window.title, strlen(app->window.title));
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE, Pt_BASIC_PREVENT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_MANAGED_FLAGS, Pt_FALSE,
             Ph_WM_APP_DEF_MANAGED | Ph_WM_RESIZE);
    PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_MANAGED_FLAGS, Pt_TRUE,
             Ph_WM_BACKDROP | Ph_WM_TOFRONT | Ph_WM_COLLAPSE | Ph_WM_FFRONT |
             Ph_WM_HELP | Ph_WM_HIDE |  Ph_WM_MOVE |
             Ph_WM_MENU | Ph_WM_RESTORE | Ph_WM_TASKBAR | Ph_WM_CLOSE |
             Ph_WM_TOBACK | Ph_WM_MAX | Ph_WM_FOCUS);
    PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_NOTIFY_FLAGS, Pt_FALSE,
             Ph_WM_HELP);
    PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_NOTIFY_FLAGS, Pt_TRUE,
             Ph_WM_COLLAPSE | Ph_WM_FOCUS | Ph_WM_MAX |
             Ph_WM_MOVE | Ph_WM_RESIZE | Ph_WM_RESTORE | Ph_WM_HIDE);
    PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_TRUE,
             Ph_WM_RENDER_CLOSE | Ph_WM_RENDER_MENU | Ph_WM_RENDER_MIN |
             Ph_WM_RENDER_TITLE | Ph_WM_RENDER_MOVE | Ph_WM_RENDER_ASAPP |
             Ph_WM_RENDER_MAX | Ph_WM_RENDER_BORDER);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &app->window.dimension, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_MINIMUM_DIM, &app->window.dimension, 0);

    temp_cb.event_f=phmupdf_main_callback_destroyed;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_IS_DESTROYED, &temp_cb, 0);

    raw_cb.event_mask=Ph_EV_KEY;
    raw_cb.event_f=phmupdf_main_callback_raw;
    raw_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_RAW, &raw_cb, 0);

    /* Finally create the window */
    app->phwindow=PtCreateWidget(PtWindow, Pt_NO_PARENT, winargc, winargs);
    if (app->phwindow==NULL)
    {
        return -1;
    }

    /* Create PtOSContainer widget for flicker-free updating */
    winargc=0;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &app->window.dimension, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0x70, 0x70, 0x70), 0);

    /* Create the PtOSContainer widget */
    app->phoscontainer=PtCreateWidget(PtOSContainer, app->phwindow, winargc, winargs);
    if (app->phoscontainer==NULL)
    {
        return -1;
    }

    /* Create PtRaw widget for drawings */
    winargc=0;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &app->window.dimension, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0x70, 0x70, 0x70), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_RAW_DRAW_F, phmupdf_draw_content, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POINTER, (void*)app, 0);
    raw_cb.event_mask=Ph_EV_BUT_PRESS | Ph_EV_PTR_MOTION_BUTTON |
        Ph_EV_PTR_MOTION_NOBUTTON | Ph_EV_BOUNDARY;
    raw_cb.event_f=phmupdf_content_callback_raw;
    raw_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_RAW, &raw_cb, 0);

    /* Create the PtRaw widget */
    app->phcontent=PtCreateWidget(PtRaw, app->phoscontainer, winargc, winargs);
    if (app->phcontent==NULL)
    {
        return -1;
    }

    /* Create PtPane widget for shadow of navigation menu */
    winargc=0;
    temp_point.x=(app->window.dimension.w-PHMUPDF_NAVIBAR_WIDTH)/2+1;
    temp_point.y=app->window.dimension.h-PHMUPDF_NAVIBAR_HEIGHT+1;
    temp_dim.h=PHMUPDF_NAVIBAR_HEIGHT;
    temp_dim.w=PHMUPDF_NAVIBAR_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0x20, 0x20, 0x20), 0);

    /* Create the PtPane widget */
    app->phnavibar_shadow=PtCreateWidget(PtPane, app->phcontent, winargc, winargs);
    if (app->phnavibar_shadow==NULL)
    {
        return -1;
    }

    /* Create PtPane widget for navigation menu */
    winargc=0;
    temp_point.x=(app->window.dimension.w-PHMUPDF_NAVIBAR_WIDTH)/2-1;
    temp_point.y=app->window.dimension.h-PHMUPDF_NAVIBAR_HEIGHT-1;
    temp_dim.h=PHMUPDF_NAVIBAR_HEIGHT;
    temp_dim.w=PHMUPDF_NAVIBAR_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xD8, 0xD8, 0xD8), 0);

    /* Create the PtPane widget */
    app->phnavibar=PtCreateWidget(PtPane, app->phcontent, winargc, winargs);
    if (app->phnavibar==NULL)
    {
        return -1;
    }

    /* Create "Open File" image */
    app->nbopenfile_img=PhCreateImage(NULL, PH_ICON_WIDTH, PH_ICON_HEIGHT, Pg_IMAGE_DIRECT_8888, NULL, 0, 0);
    PhReleaseImage(app->nbopenfile_img);
    app->nbopenfile_img->image=(char*)openfile_image;
    app->nbopenfile_img->bpl=PH_ICON_WIDTH*4;
    app->nbopenfile_img->flags=Ph_RELEASE_ALPHA;
    /* Enable per pixel alpha blending */
    app->nbopenfile_img->alpha=calloc(1, sizeof(PgAlpha_t));
    app->nbopenfile_img->alpha->alpha_op=Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs;

    /* Create PtButton widget in the navigation menu to open new file */
    winargc=0;
    temp_point.x=0;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT;
    temp_dim.w=PH_ICON_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
         Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES | Pt_STATIC_GRADIENT);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE,
         Pt_FLAT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_IMAGE, app->nbopenfile_img, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Open file", 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE,
        Pt_HIGHLIGHTED | Pt_GETS_FOCUS | Pt_FOCUS_RENDER);

    temp_cb.event_f=phmupdf_openfile_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);

    /* Create the PtButton widget */
    app->nbopenfile=PtCreateWidget(PtButton, app->phnavibar, winargc, winargs);
    if (app->nbopenfile==NULL)
    {
        return -1;
    }

    /* Create "Information" image */
    app->nbinformation_img=PhCreateImage(NULL, PH_ICON_WIDTH, PH_ICON_HEIGHT, Pg_IMAGE_DIRECT_8888, NULL, 0, 0);
    PhReleaseImage(app->nbinformation_img);
    app->nbinformation_img->image=(char*)information_image;
    app->nbinformation_img->bpl=PH_ICON_WIDTH*4;
    app->nbinformation_img->flags=Ph_RELEASE_ALPHA;
    /* Enable per pixel alpha blending */
    app->nbinformation_img->alpha=calloc(1, sizeof(PgAlpha_t));
    app->nbinformation_img->alpha->alpha_op=Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs;

    /* Create PtButton widget in the navigation menu to show information */
    winargc=0;
    temp_point.x=PHMUPDF_NAVIBAR_WIDTH-PH_ICON_WIDTH-10;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT;
    temp_dim.w=PH_ICON_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
         Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES | Pt_STATIC_GRADIENT);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE,
         Pt_FLAT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_IMAGE, app->nbinformation_img, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Information about product", 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE,
        Pt_HIGHLIGHTED | Pt_GETS_FOCUS | Pt_FOCUS_RENDER);

    temp_cb.event_f=phmupdf_information_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);

    /* Create the PtButton widget */
    app->nbinformation=PtCreateWidget(PtButton, app->phnavibar, winargc, winargs);
    if (app->nbinformation==NULL)
    {
        return -1;
    }

    /* Create separator */
    winargc=0;
    temp_point.x=PHMUPDF_NAVIBAR_WIDTH-PH_ICON_WIDTH-21;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT+4;
    temp_dim.w=8;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_FALSE, Pt_SEP_ORIENTATION);
    PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_TRUE, Pt_SEP_VERTICAL);
    PtCreateWidget(PtSeparator, app->phnavibar, winargc, winargs);

    /* Create "Settings" image */
    app->nbsettings_img=PhCreateImage(NULL, PH_ICON_WIDTH, PH_ICON_HEIGHT, Pg_IMAGE_DIRECT_8888, NULL, 0, 0);
    PhReleaseImage(app->nbsettings_img);
    app->nbsettings_img->image=(char*)settings_image;
    app->nbsettings_img->bpl=PH_ICON_WIDTH*4;
    app->nbsettings_img->flags=Ph_RELEASE_ALPHA;
    /* Enable per pixel alpha blending */
    app->nbsettings_img->alpha=calloc(1, sizeof(PgAlpha_t));
    app->nbsettings_img->alpha->alpha_op=Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs;

    /* Create PtButton widget in the navigation menu to show settings */
    winargc=0;
    temp_point.x=PHMUPDF_NAVIBAR_WIDTH-PH_ICON_WIDTH*2-26;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT;
    temp_dim.w=PH_ICON_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
         Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES | Pt_STATIC_GRADIENT);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE,
         Pt_FLAT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_IMAGE, app->nbsettings_img, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Settings", 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE,
        Pt_HIGHLIGHTED | Pt_GETS_FOCUS | Pt_FOCUS_RENDER);

    temp_cb.event_f=phmupdf_settings_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);

    /* Create the PtButton widget */
    app->nbsettings=PtCreateWidget(PtButton, app->phnavibar, winargc, winargs);
    if (app->nbsettings==NULL)
    {
        return -1;
    }

    /* Create separator */
    winargc=0;
    temp_point.x=PHMUPDF_NAVIBAR_WIDTH-PH_ICON_WIDTH*2-37;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT+4;
    temp_dim.w=8;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_FALSE, Pt_SEP_ORIENTATION);
    PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_TRUE, Pt_SEP_VERTICAL);
    PtCreateWidget(PtSeparator, app->phnavibar, winargc, winargs);

    /* Create separator */
    winargc=0;
    temp_point.x=PH_ICON_WIDTH+2;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT+4;
    temp_dim.w=8;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_FALSE, Pt_SEP_ORIENTATION);
    PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_TRUE, Pt_SEP_VERTICAL);
    PtCreateWidget(PtSeparator, app->phnavibar, winargc, winargs);

    /* Create "Previous" image */
    app->nbprevious_img=PhCreateImage(NULL, PH_ICON_WIDTH, PH_ICON_HEIGHT, Pg_IMAGE_DIRECT_8888, NULL, 0, 0);
    PhReleaseImage(app->nbprevious_img);
    app->nbprevious_img->image=(char*)previous_image;
    app->nbprevious_img->bpl=PH_ICON_WIDTH*4;
    app->nbprevious_img->flags=Ph_RELEASE_ALPHA;
    /* Enable per pixel alpha blending */
    app->nbprevious_img->alpha=calloc(1, sizeof(PgAlpha_t));
    app->nbprevious_img->alpha->alpha_op=Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs;

    /* Create PtButton widget in the navigation menu to show previous page */
    winargc=0;
    temp_point.x=PH_ICON_WIDTH+4+9;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT;
    temp_dim.w=PH_ICON_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
         Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES | Pt_STATIC_GRADIENT);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE,
         Pt_FLAT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_IMAGE, app->nbprevious_img, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Previous page", 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE,
        Pt_HIGHLIGHTED | Pt_GETS_FOCUS | Pt_FOCUS_RENDER);

    temp_cb.event_f=phmupdf_previous_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);

    /* Create the PtButton widget */
    app->nbprevious=PtCreateWidget(PtButton, app->phnavibar, winargc, winargs);
    if (app->nbprevious==NULL)
    {
        return -1;
    }

    /* Create "Next" image */
    app->nbnext_img=PhCreateImage(NULL, PH_ICON_WIDTH, PH_ICON_HEIGHT, Pg_IMAGE_DIRECT_8888, NULL, 0, 0);
    PhReleaseImage(app->nbnext_img);
    app->nbnext_img->image=(char*)next_image;
    app->nbnext_img->bpl=PH_ICON_WIDTH*4;
    app->nbnext_img->flags=Ph_RELEASE_ALPHA;
    /* Enable per pixel alpha blending */
    app->nbnext_img->alpha=calloc(1, sizeof(PgAlpha_t));
    app->nbnext_img->alpha->alpha_op=Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs;

    /* Create PtButton widget in the navigation menu to show next page */
    winargc=0;
    temp_point.x=PH_ICON_WIDTH*2+4+12;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT;
    temp_dim.w=PH_ICON_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
         Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES | Pt_STATIC_GRADIENT);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE,
         Pt_FLAT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_IMAGE, app->nbnext_img, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Next page", 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE,
        Pt_HIGHLIGHTED | Pt_GETS_FOCUS | Pt_FOCUS_RENDER);

    temp_cb.event_f=phmupdf_next_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);

    /* Create the PtButton widget */
    app->nbnext=PtCreateWidget(PtButton, app->phnavibar, winargc, winargs);
    if (app->nbnext==NULL)
    {
        return -1;
    }

    /* Create PtText widget in the navigation menu to edit page number */
    winargc=0;
    temp_point.x=PH_ICON_WIDTH*3+4+18;
    temp_point.y=4;
    temp_dim.h=24;
    temp_dim.w=64;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_MAX_LENGTH, 6, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
         Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES |
         Pt_ALL_INLINES | Pt_STATIC_GRADIENT);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE,
         Pt_FLAT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_HORIZONTAL_ALIGNMENT, Pt_CENTER, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xC0, 0xC0, 0xC0), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Jump to page", 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_TRUE, Pt_BLOCKED | Pt_CALLBACKS_ACTIVE);

    temp_cb.event_f=phmupdf_pagecheck_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_MODIFY_VERIFY, &temp_cb, 0);
    temp_cb2.event_f=phmupdf_pageselect_callback;
    temp_cb2.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb2, 0);
    raw_cb.event_mask=Ph_EV_KEY;
    raw_cb.event_f=phmupdf_pageraw_callback;
    raw_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_RAW, &raw_cb, 0);

    /* Create the PtText widget */
    app->nbpage=PtCreateWidget(PtText, app->phnavibar, winargc, winargs);
    if (app->nbpage==NULL)
    {
        return -1;
    }

    /* Create PtLabel widget in the navigation menu to show amount of pages */
    winargc=0;
    temp_point.x=PH_ICON_WIDTH*3+4+18;
    temp_point.y=28;
    temp_dim.h=19;
    temp_dim.w=64;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "", 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_HORIZONTAL_ALIGNMENT, Pt_CENTER, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xC0, 0xC0, 0xC0), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Total pages", 0);

    /* Create the PtLabel widget */
    app->nbpages=PtCreateWidget(PtLabel, app->phnavibar, winargc, winargs);
    if (app->nbpages==NULL)
    {
        return -1;
    }

    /* Create separator */
    winargc=0;
    temp_point.x=PH_ICON_WIDTH*3+89;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT+4;
    temp_dim.w=8;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_FALSE, Pt_SEP_ORIENTATION);
    PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_TRUE, Pt_SEP_VERTICAL);
    PtCreateWidget(PtSeparator, app->phnavibar, winargc, winargs);

    /* Create "Search" image */
    app->nbsearch_img=PhCreateImage(NULL, PH_ICON_WIDTH, PH_ICON_HEIGHT, Pg_IMAGE_DIRECT_8888, NULL, 0, 0);
    PhReleaseImage(app->nbsearch_img);
    app->nbsearch_img->image=(char*)search_image;
    app->nbsearch_img->bpl=PH_ICON_WIDTH*4;
    app->nbsearch_img->flags=Ph_RELEASE_ALPHA;
    /* Enable per pixel alpha blending */
    app->nbsearch_img->alpha=calloc(1, sizeof(PgAlpha_t));
    app->nbsearch_img->alpha->alpha_op=Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs;

    /* Create PtButton widget in the navigation menu to search */
    winargc=0;
    temp_point.x=PHMUPDF_NAVIBAR_WIDTH-PH_ICON_WIDTH*3-40;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT;
    temp_dim.w=PH_ICON_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
         Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES | Pt_STATIC_GRADIENT);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE,
         Pt_FLAT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_IMAGE, app->nbsearch_img, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Search", 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE,
        Pt_HIGHLIGHTED | Pt_GETS_FOCUS | Pt_FOCUS_RENDER);

    temp_cb.event_f=phmupdf_search_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);

    /* Create the PtButton widget */
    app->nbsearch=PtCreateWidget(PtButton, app->phnavibar, winargc, winargs);
    if (app->nbsearch==NULL)
    {
        return -1;
    }

    /* Create separator */
    winargc=0;
    temp_point.x=PHMUPDF_NAVIBAR_WIDTH-PH_ICON_WIDTH*4-3;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT+4;
    temp_dim.w=8;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_FALSE, Pt_SEP_ORIENTATION);
    PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_TRUE, Pt_SEP_VERTICAL);
    PtCreateWidget(PtSeparator, app->phnavibar, winargc, winargs);

    /* Create "Zoom In" image */
    app->nbzoomin_img=PhCreateImage(NULL, PH_ICON_WIDTH, PH_ICON_HEIGHT, Pg_IMAGE_DIRECT_8888, NULL, 0, 0);
    PhReleaseImage(app->nbzoomin_img);
    app->nbzoomin_img->image=(char*)zoomin_image;
    app->nbzoomin_img->bpl=PH_ICON_WIDTH*4;
    app->nbzoomin_img->flags=Ph_RELEASE_ALPHA;
    /* Enable per pixel alpha blending */
    app->nbzoomin_img->alpha=calloc(1, sizeof(PgAlpha_t));
    app->nbzoomin_img->alpha->alpha_op=Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs;

    /* Create PtButton widget in the navigation menu to zoom in */
    winargc=0;
    temp_point.x=PH_ICON_WIDTH*4+52;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT;
    temp_dim.w=PH_ICON_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
         Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES | Pt_STATIC_GRADIENT);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE,
         Pt_FLAT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_IMAGE, app->nbzoomin_img, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Zoom in", 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE,
        Pt_HIGHLIGHTED | Pt_GETS_FOCUS | Pt_FOCUS_RENDER);

    temp_cb.event_f=phmupdf_zoomin_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);

    /* Create the PtButton widget */
    app->nbzoomin=PtCreateWidget(PtButton, app->phnavibar, winargc, winargs);
    if (app->nbzoomin==NULL)
    {
        return -1;
    }

    /* Create "Zoom Out" image */
    app->nbzoomout_img=PhCreateImage(NULL, PH_ICON_WIDTH, PH_ICON_HEIGHT, Pg_IMAGE_DIRECT_8888, NULL, 0, 0);
    PhReleaseImage(app->nbzoomout_img);
    app->nbzoomout_img->image=(char*)zoomout_image;
    app->nbzoomout_img->bpl=PH_ICON_WIDTH*4;
    app->nbzoomout_img->flags=Ph_RELEASE_ALPHA;
    /* Enable per pixel alpha blending */
    app->nbzoomout_img->alpha=calloc(1, sizeof(PgAlpha_t));
    app->nbzoomout_img->alpha->alpha_op=Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs;

    /* Create PtButton widget in the navigation menu to zoom out */
    winargc=0;
    temp_point.x=PHMUPDF_NAVIBAR_WIDTH-PH_ICON_WIDTH*4-54-63-64;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT;
    temp_dim.w=PH_ICON_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
         Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES | Pt_STATIC_GRADIENT);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE,
         Pt_FLAT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_IMAGE, app->nbzoomout_img, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Zoom out", 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE,
        Pt_HIGHLIGHTED | Pt_GETS_FOCUS | Pt_FOCUS_RENDER);

    temp_cb.event_f=phmupdf_zoomout_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);

    /* Create the PtButton widget */
    app->nbzoomout=PtCreateWidget(PtButton, app->phnavibar, winargc, winargs);
    if (app->nbzoomout==NULL)
    {
        return -1;
    }

    /* Create PtComboBox widget in the navigation menu to edit zoom level */
    winargc=0;
    temp_point.x=PH_ICON_WIDTH*5+59;
    temp_point.y=12;
    temp_dim.h=26;
    temp_dim.w=70;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_MAX_LENGTH, 10, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_SELECTION_FILL_COLOR, PgRGB(0x60, 0x60, 0x60), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_CBOX_TEXT_FILL_COLOR, PgRGB(0xC0, 0xC0, 0xC0), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_TRUE, Pt_BLOCKED);

    temp_cb.event_f=phmupdf_zoomcheck_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_MODIFY_VERIFY, &temp_cb, 0);
    temp_cb2.event_f=phmupdf_zoomselect_callback;
    temp_cb2.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb2, 0);
    raw_cb.event_mask=Ph_EV_KEY;
    raw_cb.event_f=phmupdf_zoomraw_callback;
    raw_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_RAW, &raw_cb, 0);

    /* Create the PtComboBox widget */
    app->nbzoomlevel=PtCreateWidget(PtComboBox, app->phnavibar, winargc, winargs);
    if (app->nbzoomlevel==NULL)
    {
        return -1;
    }

    /* Fill combox data */
    {
        const char* cb_zoom_items[12]=
        {
            "10%",
            "25%",
            "50%",
            "75%",
            "100%",
            "125%",
            "150%",
            "200%",
            "300%",
            "400%",
            "Fit Page",
            "Fit Width"
        };

        PtListAddItems(app->nbzoomlevel, cb_zoom_items, 12, 0);
    }

    /* Create "Rotation" image */
    app->nbrotation_img=PhCreateImage(NULL, PH_ICON_WIDTH, PH_ICON_HEIGHT, Pg_IMAGE_DIRECT_8888, NULL, 0, 0);
    PhReleaseImage(app->nbrotation_img);
    app->nbrotation_img->image=(char*)rotation_image;
    app->nbrotation_img->bpl=PH_ICON_WIDTH*4;
    app->nbrotation_img->flags=Ph_RELEASE_ALPHA;
    /* Enable per pixel alpha blending */
    app->nbrotation_img->alpha=calloc(1, sizeof(PgAlpha_t));
    app->nbrotation_img->alpha->alpha_op=Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs;

    /* Create PtButton widget in the navigation menu to zoom out */
    winargc=0;
    temp_point.x=PHMUPDF_NAVIBAR_WIDTH-PH_ICON_WIDTH*4-55;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT;
    temp_dim.w=PH_ICON_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
         Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES | Pt_STATIC_GRADIENT);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE,
         Pt_FLAT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_IMAGE, app->nbrotation_img, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Rotation", 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE,
        Pt_HIGHLIGHTED | Pt_GETS_FOCUS | Pt_FOCUS_RENDER);

    temp_cb.event_f=phmupdf_rotation_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);

    /* Create the PtButton widget */
    app->nbrotation=PtCreateWidget(PtButton, app->phnavibar, winargc, winargs);
    if (app->nbrotation==NULL)
    {
        return -1;
    }

    /* Create separator */
    winargc=0;
    temp_point.x=PHMUPDF_NAVIBAR_WIDTH-PH_ICON_WIDTH*4-54-12;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT+4;
    temp_dim.w=8;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_FALSE, Pt_SEP_ORIENTATION);
    PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_TRUE, Pt_SEP_VERTICAL);
    PtCreateWidget(PtSeparator, app->phnavibar, winargc, winargs);

    /* Create "Properties" image */
    app->nbproperties_img=PhCreateImage(NULL, PH_ICON_WIDTH, PH_ICON_HEIGHT, Pg_IMAGE_DIRECT_8888, NULL, 0, 0);
    PhReleaseImage(app->nbproperties_img);
    app->nbproperties_img->image=(char*)properties_image;
    app->nbproperties_img->bpl=PH_ICON_WIDTH*4;
    app->nbproperties_img->flags=Ph_RELEASE_ALPHA;
    /* Enable per pixel alpha blending */
    app->nbproperties_img->alpha=calloc(1, sizeof(PgAlpha_t));
    app->nbproperties_img->alpha->alpha_op=Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs;

    /* Create PtButton widget in the navigation menu to zoom out */
    winargc=0;
    temp_point.x=PHMUPDF_NAVIBAR_WIDTH-PH_ICON_WIDTH*5-71;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT;
    temp_dim.w=PH_ICON_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
         Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES | Pt_STATIC_GRADIENT);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE,
         Pt_FLAT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_IMAGE, app->nbproperties_img, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Document properties", 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE,
        Pt_HIGHLIGHTED | Pt_GETS_FOCUS | Pt_FOCUS_RENDER);

    temp_cb.event_f=phmupdf_properties_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);

    /* Create the PtButton widget */
    app->nbproperties=PtCreateWidget(PtButton, app->phnavibar, winargc, winargs);
    if (app->nbproperties==NULL)
    {
        return -1;
    }

    /* Create separator */
    winargc=0;
    temp_point.x=PHMUPDF_NAVIBAR_WIDTH-PH_ICON_WIDTH*5-70-12;
    temp_point.y=0;
    temp_dim.h=PH_ICON_HEIGHT+4;
    temp_dim.w=8;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_FALSE, Pt_SEP_ORIENTATION);
    PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_TRUE, Pt_SEP_VERTICAL);
    PtCreateWidget(PtSeparator, app->phnavibar, winargc, winargs);

    /* Show widget */
    status=PtRealizeWidget(app->phwindow);
    if (status!=0)
    {
        PtDestroyWidget(app->phwindow);
    }

    /* Flush all widget operations */
    PtFlush();

    /* Now it is safe to free image structures */
    /* Photon will release the rest at exit    */
    free(app->nbopenfile_img);
    app->nbopenfile_img=NULL;
    free(app->nbinformation_img);
    app->nbinformation_img=NULL;
    free(app->nbsettings_img);
    app->nbsettings_img=NULL;
    free(app->nbprevious_img);
    app->nbprevious_img=NULL;
    free(app->nbnext_img);
    app->nbnext_img=NULL;
    free(app->nbzoomin_img);
    app->nbzoomin_img=NULL;
    free(app->nbzoomout_img);
    app->nbzoomout_img=NULL;
    free(app->nbsearch_img);
    app->nbsearch_img=NULL;
    free(app->nbrotation_img);
    app->nbrotation_img=NULL;
    free(app->nbproperties_img);
    app->nbproperties_img=NULL;

    /* Get current window position */
    {
        PhPoint_t* current_position;

        PtGetResource(app->phwindow, Pt_ARG_POS, &current_position, 0);
        app->window.position=*current_position;
    }

    /* Window has been successfully created */
    return 0;
}

int change_window_size(photon_mupdf_t* app)
{
    PhPoint_t     temp_point;
    PhDim_t       temp_dim;
    unsigned int* old_scroll_position;
    unsigned int  old_totallines=0;
    unsigned int  old_sc=0;
    int           it=0;
    char          text[16];

    if (app->window.dimension.w<PHMUPDF_MINIMUM_WIDTH)
    {
        app->window.dimension.w=PHMUPDF_MINIMUM_WIDTH;
    }
    if (app->window.dimension.h<PHMUPDF_MINIMUM_HEIGHT)
    {
        app->window.dimension.h=PHMUPDF_MINIMUM_HEIGHT;
    }

    if (app->phpagescroll)
    {
        PtGetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, &old_scroll_position, 0);
        old_sc=*old_scroll_position;
        old_totallines=app->pdf_totallines;

        temp_point.x=app->window.dimension.w-PHMUPDF_SCROLLBAR_WIDTH;
        temp_point.y=0;
        temp_dim.h=app->window.dimension.h;
        temp_dim.w=PHMUPDF_SCROLLBAR_WIDTH;
        PtSetResource(app->phpagescroll, Pt_ARG_POS, &temp_point, 0);
        PtSetResource(app->phpagescroll, Pt_ARG_DIM, &temp_dim, 0);
    }

    /* Move navigation bar and it's shadow */
    temp_point.x=(app->window.dimension.w-PHMUPDF_NAVIBAR_WIDTH)/2+1;
    temp_point.y=app->window.dimension.h-PHMUPDF_NAVIBAR_HEIGHT+1;
    PtSetResource(app->phnavibar_shadow, Pt_ARG_POS, &temp_point, 0);
    temp_point.x=(app->window.dimension.w-PHMUPDF_NAVIBAR_WIDTH)/2-1;
    temp_point.y=app->window.dimension.h-PHMUPDF_NAVIBAR_HEIGHT-1;
    PtSetResource(app->phnavibar, Pt_ARG_POS, &temp_point, 0);

    /* Move search bar and it's shadow if present */
    if (app->phsearch_shadow)
    {
        temp_point.x=app->window.dimension.w-PHMUPDF_SEARCHBAR_WIDTH-PHMUPDF_SCROLLBAR_WIDTH+1;
        temp_point.y=1;
        PtSetResource(app->phsearch_shadow, Pt_ARG_POS, &temp_point, 0);
    }
    if (app->phsearch_pane)
    {
        temp_point.x=app->window.dimension.w-PHMUPDF_SEARCHBAR_WIDTH-PHMUPDF_SCROLLBAR_WIDTH-1;
        temp_point.y=0;
        PtSetResource(app->phsearch_pane, Pt_ARG_POS, &temp_point, 0);
    }

    PtSetResource(app->phcontent, Pt_ARG_DIM, &app->window.dimension, 0);
    PtSetResource(app->phoscontainer, Pt_ARG_DIM, &app->window.dimension, 0);
    PtSetResource(app->phwindow, Pt_ARG_DIM, &app->window.dimension, 0);
    PtFlush();

    /* Recalculate zoom level of each page to fit the window bounds */
    if (app->pdf_pages)
    {
        app->pdf_totallines=0;
        app->pdf_zoommax=INT_MAX;
        for (it=0; it<app->pdf_pagecount; it++)
        {
            fz_matrix temp_ctm;
            fz_rect temp_rect;

            if ((app->pdf_pages[it].page_bbox_rot.x1-app->pdf_pages[it].page_bbox_rot.x0)!=0)
            {
                app->pdf_pages[it].page_zoom=(float)(app->window.dimension.w-4-PHMUPDF_SCROLLBAR_WIDTH)*100.0f/
                    (float)(app->pdf_pages[it].page_bbox_rot.x1-app->pdf_pages[it].page_bbox_rot.x0);
            }
            else
            {
                app->pdf_pages[it].page_zoom=0.0f;
            }

            /* Adjust maximum zoom */
            if (app->pdf_zoommax>app->pdf_pages[it].page_zoom)
            {
                app->pdf_zoommax=app->pdf_pages[it].page_zoom;
            }

            fz_scale(&app->pdf_pages[it].page_ctm,
                app->pdf_pages[it].page_zoom/100.0f,
                app->pdf_pages[it].page_zoom/100.0f);
            fz_concat(&app->pdf_pages[it].page_ctm, &app->pdf_pages[it].page_ctm, fz_rotate(&temp_ctm, app->rotation));
            temp_rect=app->pdf_pages[it].page_bbox;
            fz_round_rect(&app->pdf_pages[it].page_bbox_draw,
                fz_transform_rect(&temp_rect, &app->pdf_pages[it].page_ctm));
            if (app->pdf_pages[it].page_bbox_draw.x0<0)
            {
                fz_concat(&app->pdf_pages[it].page_ctm, &app->pdf_pages[it].page_ctm, fz_translate(&temp_ctm, -app->pdf_pages[it].page_bbox_draw.x0, 0));
            }
            if (app->pdf_pages[it].page_bbox_draw.y0<0)
            {
                fz_concat(&app->pdf_pages[it].page_ctm, &app->pdf_pages[it].page_ctm, fz_translate(&temp_ctm, 0, -app->pdf_pages[it].page_bbox_draw.y0));
            }
            temp_rect=app->pdf_pages[it].page_bbox;
            fz_round_rect(&app->pdf_pages[it].page_bbox_draw,
                fz_transform_rect(&temp_rect, &app->pdf_pages[it].page_ctm));
            app->pdf_totallines+=(app->pdf_pages[it].page_bbox_draw.y1-app->pdf_pages[it].page_bbox_draw.y0)+1;
        }

        /* Add some gap between pages, 16 pixels between pages */
        app->pdf_totallines+=app->pdf_pagecount*PHMUPDF_GAP_BETWEEN_PAGES;
    }
    if (it!=0)
    {
        if (app->pdf_totallines<PHMUPDF_NAVIBAR_HEIGHT+PHMUPDF_GAP_BETWEEN_PAGES)
        {
            app->pdf_totallines=1;
        }
        else
        {
            app->pdf_totallines-=PHMUPDF_NAVIBAR_HEIGHT+PHMUPDF_GAP_BETWEEN_PAGES;
        }
    }

    if (app->phpagescroll)
    {
        app->pdf_zoom=PHMUPDF_ZOOM_FIT_WIDTH;
        sprintf(text, "%d%%", app->pdf_zoommax);
        PtSetResource(app->nbzoomlevel, Pt_ARG_TEXT_STRING, text, 0);
    }

    /* Adjust scrollbar parameters */
    if (app->phpagescroll)
    {
        PtSetResource(app->phpagescroll, Pt_ARG_MAXIMUM, app->pdf_totallines, 0);
        PtSetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE,
            (uint32_t)((uint64_t)app->pdf_totallines*old_sc/old_totallines), 0);
        PtSetResource(app->phpagescroll, Pt_ARG_PAGE_INCREMENT,
            app->window.dimension.h-PHMUPDF_NAVIBAR_HEIGHT, 0);
    }

    phpdf_destroy_page_cache(app);
    phmupdf_search_recreate_boxes(app);

    if (app->phpagescroll)
    {
        /* Update cursor in case if it points to link or text */
        if ((app->last_x!=-1) && (app->last_y!=-1))
        {
            phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
            phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
        }
    }

    PtDamageWidget(app->phcontent);
    PtFlush();

    return 0;
}

void handle_window_event(photon_mupdf_t* app)
{
    uint8_t    eventbuffer[8192];
    PhEvent_t* event=(PhEvent_t*)eventbuffer;
    int32_t    status;
    uint32_t   finish=0;

    do {
        status=PhEventPeek(event, 8192);
        switch (status)
        {
            case Ph_RESIZE_MSG:
                 {
                     fprintf(stderr, "PhMuPDF: Event size too large for buffer\n");
                     return;
                 }
                 break;
            case Ph_EVENT_MSG:
                 {
                     /* Pass event to Widgets Toolkit */
                     PtEventHandler(event);

                     /* Event is ready */
                     switch (event->type)
                     {
                         case Ph_EV_WM:
                              {
                                  PhWindowEvent_t* wmevent=NULL;

                                  /* Get associated event data */
                                  wmevent=PhGetData(event);
                                  if (wmevent==NULL)
                                  {
                                      break;
                                  }

                                  switch (wmevent->event_f)
                                  {
                                      case Ph_WM_RESIZE:
                                           {
                                               app->window.event_flags|=PHW_WINDOW_CHANGE_SIZE;
                                               app->window.event_flags|=PHW_WINDOW_CHANGE_POSITION;
                                               app->window.event_flags&=~(PHW_WINDOW_MAXIMIZE_EVENT);
                                               app->window.dimension=wmevent->size;
                                               app->window.position=wmevent->pos;
                                           }
                                           break;
                                      case Ph_WM_MAX:
                                           {
                                               /* Avoid double maximize events */
                                               if ((app->window.event_flags & PHW_WINDOW_MAXIMIZE_EVENT)==0)
                                               {
                                                   PhPoint_t* current_position;
                                                   PhDim_t* current_size;

                                                   PtGetResource(app->phwindow, Pt_ARG_POS, &current_position, 0);
                                                   PtGetResource(app->phwindow, Pt_ARG_DIM, &current_size, 0);

                                                   app->window.event_flags|=PHW_WINDOW_CHANGE_SIZE;
                                                   app->window.event_flags|=PHW_WINDOW_CHANGE_POSITION;
                                                   app->window.event_flags|=PHW_WINDOW_MAXIMIZE_EVENT;
                                                   app->window.old_dimension=app->window.dimension;
                                                   app->window.old_position=app->window.position;
                                                   app->window.dimension=*current_size;
                                                   app->window.position=*current_position;

                                                   PtSetResource(app->phwindow, Pt_ARG_WINDOW_STATE, Pt_TRUE,
                                                       Ph_WM_STATE_ISMAX);
                                               }
                                           }
                                           break;
                                      case Ph_WM_RESTORE:
                                           {
                                               app->window.event_flags|=PHW_WINDOW_CHANGE_SIZE;
                                               app->window.event_flags|=PHW_WINDOW_CHANGE_POSITION;
                                               app->window.event_flags&=~(PHW_WINDOW_MAXIMIZE_EVENT);

                                               app->window.dimension=app->window.old_dimension;
                                               app->window.position=app->window.old_position;

                                               PtSetResource(app->phwindow, Pt_ARG_WINDOW_STATE, Pt_FALSE,
                                                   Ph_WM_STATE_ISMAX);
                                           }
                                           break;
                                      case Ph_WM_MOVE:
                                           {
                                               PhPoint_t* current_position;

                                               PtGetResource(app->phwindow, Pt_ARG_POS, &current_position, 0);
                                               app->window.position=*current_position;
                                               app->window.event_flags&=~(PHW_WINDOW_MAXIMIZE_EVENT);
                                           }
                                           break;
                                      case Ph_WM_FOCUS:
                                           {
                                               if (wmevent->event_state==Ph_WM_EVSTATE_FOCUS)
                                               {
                                                   PhCursorInfo_t phcursor;
                                                   short x, y;

                                                   PhQueryCursor(PhInputGroup(NULL), &phcursor);
                                                   PtGetAbsPosition(app->phcontent, &x, &y);
                                                   x=phcursor.pos.x-x;
                                                   y=phcursor.pos.y-y;
                                                   app->last_x=x;
                                                   app->last_y=y;

                                                   if (app->phpagescroll)
                                                   {
                                                       /* Update cursor in case if it points to link or text */
                                                       if ((app->last_x!=-1) && (app->last_y!=-1))
                                                       {
                                                           phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
                                                           phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
                                                       }
                                                   }
                                               }
                                               else
                                               {
                                                   app->last_x=-1;
                                                   app->last_y=-1;
                                               }
                                           }
                                           break;
                                  }
                              }
                              break;
                     }
                 }
                 break;
            case 0:
                 {
                     /* All events are read */
                     finish=1;
                 }
                 break;
            case -1:
                 {
                     /* Error occured in event reading */
                     fprintf(stderr, "PhMuPDF: Can't read event\n");
                     return;
                 }
                 break;
        }

        if (finish!=0)
        {
            break;
        }
    } while (1);
}

int main(int argc, char* argv[])
{
    struct timespec ts={0, 1};

    /* Initialize file selection variables */
    memset(&application.fileinfo, 0x00, sizeof(application.fileinfo));
    application.filename[0]=0x00;
    application.filename_set=0;
    application.jump_stack_ptr=-1;
    application.jump_stack_last_valid=-1;
    application.last_search[0]=0x00;
    application.case_search=0;
    application.fileopen_dialog=0;

    /* Initialize default settings */
    application.settings.cache_size=FZ_STORE_DEFAULT;
    application.settings.bw_render=0;
    application.settings.off_render=1;
    application.settings.fast_search=1;
    application.settings.fast_render=1;
    application.settings.lastpath[0]='~';
    application.settings.lastpath[1]=0x00;
    application.settings.aa_level=8;

    /* Load settings */
    {
        int   fd=-1;
        char* home=getenv("HOME");
        char  text[PATH_MAX];

        sprintf(text, "%s/.phmupdf", home);
        fd=open(text, O_RDONLY);
        if (fd!=-1)
        {
            if (read(fd, &application.settings, sizeof(application.settings))!=sizeof(application.settings))
            {
                /* Initialize default settings */
                application.settings.cache_size=FZ_STORE_DEFAULT;
                application.settings.bw_render=0;
                application.settings.off_render=1;
                application.settings.fast_search=1;
                application.settings.fast_render=1;
                application.settings.lastpath[0]='~';
                application.settings.lastpath[1]=0x00;
                application.settings.aa_level=8;
            }
            close(fd);
        }
    }

    /* Use any given argument as file name */
    if (argc>1)
    {
        strncpy(application.filename, argv[1], PATH_MAX+NAME_MAX);
        application.filename_set=1;
    }

    if (PtInit(NULL)==-1)
    {
        fprintf(stderr, "PhMuPDF: Can't connect to Photon\n");
        return 1;
    }

    /* Get display pixel format for a conversion routines */
    {
        PdOffscreenContext_t* main_display_ctx=NULL;

        application.display_format=Pg_IMAGE_DIRECT_565;

        /* Try to make duplicate of display context */
        main_display_ctx=PdCreateOffscreenContext(0, 0, 0, Pg_OSC_MAIN_DISPLAY);
        /* Check if underlying driver has full support of layer */
        if (main_display_ctx!=NULL)
        {
            application.display_format=main_display_ctx->format;
            PhDCRelease(main_display_ctx);
        }
        else
        {
            PgDisplaySettings_t mode_settings;
            PgVideoModeInfo_t   mode_info;

            /* In very rare cases we have to check display format in the video  */
            /* mode settings. If video mode is not generic, this function fails */
            if (PgGetVideoMode(&mode_settings)==0)
            {
                if (PgGetVideoModeInfo(mode_settings.mode, &mode_info)==0)
                {
                    application.display_format=mode_info.type;
                }
                else
                {
                    fprintf(stderr, "PhMuPDF: Can't get display pixel format\n");
                    return 1;
                }
            }
            else
            {
                fprintf(stderr, "PhMuPDF: Can't get display pixel format\n");
                return 1;
            }
        }
    }

    /* Check for supported display format */
    if ((application.display_format!=Pg_IMAGE_DIRECT_565) &&
        (application.display_format!=Pg_IMAGE_DIRECT_1555) &&
        (application.display_format!=Pg_IMAGE_DIRECT_888) &&
        (application.display_format!=Pg_IMAGE_DIRECT_8888))
    {
        fprintf(stderr, "PhMuPDF: unsupported display pixel format\n");
        return 1;
    }

    strcpy(application.window.title, PHMUPDF_DEFAULT_TITLE);
    application.window.dimension.w=PHMUPDF_MINIMUM_WIDTH;
    application.window.dimension.h=PHMUPDF_MINIMUM_HEIGHT;
    application.phwindow_about=NULL;
    application.pdf_zoommin=5;

    create_window(&application);

    if (application.filename_set)
    {
        /* Open requested file */
        phpdf_openfile(&application);
    }

    do {
        handle_window_event(&application);
        nanosleep(&ts, NULL);
        if (application.window.event_flags & PHW_WINDOW_EXIT_REQUESTED)
        {
            application.window.event_flags&=~(PHW_WINDOW_EXIT_REQUESTED);
            break;
        }
        if (application.window.event_flags & PHW_WINDOW_CHANGE_POSITION)
        {
            PtSetResource(application.phwindow, Pt_ARG_POS, &application.window.position, 0);
            PtFlush();
            application.window.event_flags&=~(PHW_WINDOW_CHANGE_POSITION);
        }
        if (application.window.event_flags & PHW_WINDOW_CHANGE_SIZE)
        {
            change_window_size(&application);
            application.window.event_flags&=~(PHW_WINDOW_CHANGE_SIZE);
        }
    } while(1);

    /* Free resources */
    phpdf_destroy_page_cache(&application);
    if (application.pdf_opened)
    {
        phpdf_closefile(&application);
    }

    /* Save settings */
    {
        int   fd=-1;
        char* home=getenv("HOME");
        char  text[PATH_MAX];

        sprintf(text, "%s/.phmupdf", home);
        fd=open(text, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (fd!=-1)
        {
            write(fd, &application.settings, sizeof(application.settings));
            close(fd);
        }
    }

    return 0;
}
