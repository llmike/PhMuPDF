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

#include <libgen.h>

#include "phpdf.h"
#include "phcallbacks.h"

int phpdf_validate_password(void* data, const char* password)
{
    photon_mupdf_t* app=(photon_mupdf_t*)data;

    if (fz_authenticate_password(app->pdfdoc, (char*)password))
    {
        return Pt_PWD_ACCEPT;
    }

    app->pdf_password_tries++;
    if (app->pdf_password_tries==3)
    {
        return Pt_PWD_REJECT;
    }

    return Pt_PWD_RETRY;
}

int phpdf_openfile(photon_mupdf_t* app)
{
    fz_stream*      file;
    fz_device*      pdfdev;
    int             fd;
    char            fname_local[PATH_MAX+NAME_MAX+1];
    char*           fname;
    int             it=0;
    int             jt=0;
    PtArg_t         winargs[32];
    uint32_t        winargc=0;
    PhDim_t         temp_dim;
    PhPoint_t       temp_point;
    PtCallback_t    temp_cb;
    PtRawCallback_t raw_cb;
    PtWidget_t*     progress_shadow=NULL;
    PtWidget_t*     progress=NULL;
    int             spare=0;

    /* Redraw main window */
    PtDamageWidget(app->phwindow);

    /* Destroy page cache */
    phpdf_destroy_page_cache(app);

    /* Close any opened previous file */
    if (app->pdf_opened)
    {
        phpdf_closefile(app);
    }

    /* Reset cursor */
    PtSetResource(app->phcontent, Pt_ARG_CURSOR_TYPE, Ph_CURSOR_POINTER, 0);
    PtSetResource(app->phcontent, Pt_ARG_CURSOR_COLOR, PgRGB(0xFF, 0xFF, 0xE0), 0);

    fd=open(app->filename, O_RDONLY);
    if (fd==-1)
    {
        PtNotice(app->phwindow, NULL, "File open error", NULL,
            "Can't open requested file!", NULL, NULL, NULL,
            Pt_MODAL);
        strcpy(app->window.title, PHMUPDF_DEFAULT_TITLE);
        PtSetResource(app->phwindow, Pt_ARG_WINDOW_TITLE, app->window.title, strlen(app->window.title));
        app->pdf_opened=0;
        app->filename_set=0;
        return -1;
    }

    /* Shutdown mupdf library, if required */
    if (app->pdfctx!=NULL)
    {
        fz_free_context(app->pdfctx);
        app->pdfctx=NULL;
    }

    /* Initialize mupdf library */
    if (app->settings.fast_render)
    {
        app->pdfctx=fz_new_context(NULL, NULL, 0);
    }
    else
    {
        app->pdfctx=fz_new_context(NULL, NULL, app->settings.cache_size);
    }

    /* Set anti-aliasing level */
    fz_set_aa_level(app->pdfctx, app->settings.aa_level);

    /* Destroy document context, if required */
    if (app->pdfdoc)
    {
        fz_close_document(app->pdfdoc);
        app->pdfdoc=NULL;
    }

    fz_try(app->pdfctx)
    {
        file=fz_open_fd(app->pdfctx, fd);

        if (strstr(app->filename, ".xps") ||
            strstr(app->filename, ".XPS") ||
            strstr(app->filename, ".rels") ||
            strstr(app->filename, ".RELS"))
        {
            app->pdfdoc=(fz_document*)xps_open_document_with_stream(app->pdfctx, file);
        }
        else
        {
            if (strstr(app->filename, ".cbz") ||
                strstr(app->filename, ".CBZ"))
            {
                app->pdfdoc=(fz_document*)cbz_open_document_with_stream(app->pdfctx, file);
            }
            else
            {
                if (strstr(app->filename, ".png") ||
                    strstr(app->filename, ".PNG") ||
                    strstr(app->filename, ".jpg") ||
                    strstr(app->filename, ".JPG") ||
                    strstr(app->filename, ".jpeg") ||
                    strstr(app->filename, ".JPEG") ||
                    strstr(app->filename, ".jfif") ||
                    strstr(app->filename, ".JFIF") ||
                    strstr(app->filename, ".tif") ||
                    strstr(app->filename, ".TIF") ||
                    strstr(app->filename, ".tiff") ||
                    strstr(app->filename, ".TIFF"))
                {
                    app->pdfdoc=(fz_document*)image_open_document_with_stream(app->pdfctx, file);
                }
                else
                {
                    app->pdfdoc=(fz_document*)pdf_open_document_with_stream(app->pdfctx, file);
                }
            }
        }

        fz_close(file);

        app->pdf_pagecount=fz_count_pages(app->pdfdoc);
        app->pdf_outline=fz_load_outline(app->pdfdoc);

        if (fz_needs_password(app->pdfdoc))
        {
            int result;
            PhImage_t* temp_key_img;

            /* Create temporary image which will be destroyed in the dialog */
            temp_key_img=PhCreateImage(NULL, PH_ICON_WIDTH, PH_ICON_HEIGHT, Pg_IMAGE_DIRECT_8888, NULL, 0, 0);
            PhReleaseImage(temp_key_img);
            temp_key_img->image=(char*)key_image;
            temp_key_img->bpl=PH_ICON_WIDTH*4;
            temp_key_img->flags=Ph_RELEASE_ALPHA;
            /* Enable per pixel alpha blending */
            temp_key_img->alpha=calloc(1, sizeof(PgAlpha_t));
            temp_key_img->alpha->alpha_op=Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs;

            app->pdf_password_tries=0;

            result=PtPassword(app->phwindow, NULL, "Password protected PDF document",
                temp_key_img, "Enter password for PDF document:", NULL, NULL, NULL, NULL,
                phpdf_validate_password, (void*)app, NULL, Pt_MODAL);

            if (result!=Pt_PWD_ACCEPT)
            {
                if (result==Pt_PWD_REJECT)
                {
                    PtNotice(app->phwindow, NULL, "File can't be opened", NULL,
                        "You have typed an incorrect password three times!", NULL, NULL, NULL,
                        Pt_MODAL);
                }

                strcpy(app->window.title, PHMUPDF_DEFAULT_TITLE);
                PtSetResource(app->phwindow, Pt_ARG_WINDOW_TITLE, app->window.title, strlen(app->window.title));
                app->pdf_opened=0;
                app->filename_set=0;
                return -1;
            }
        }
    }
    fz_catch(app->pdfctx)
    {
        PtNotice(app->phwindow, NULL, "File processing error", NULL,
            "Requested file contains bad structure!", NULL, NULL, NULL,
            Pt_MODAL);

        strcpy(app->window.title, PHMUPDF_DEFAULT_TITLE);
        PtSetResource(app->phwindow, Pt_ARG_WINDOW_TITLE, app->window.title, strlen(app->window.title));
        app->pdf_opened=0;
        app->filename_set=0;
        return -1;
    }

    /* Set main window title */
    strncpy(fname_local, app->filename, PATH_MAX+NAME_MAX+1);
    fname=basename(fname_local);
    strncpy(fname_local, fname, PATH_MAX+NAME_MAX+1);
    strcat(fname_local+strlen(fname_local), " - ");
    strcat(fname_local+strlen(fname_local), PHMUPDF_DEFAULT_TITLE);
    strncpy(app->window.title, fname_local, 1023);
    PtSetResource(app->phwindow, Pt_ARG_WINDOW_TITLE, app->window.title, strlen(app->window.title));

    /* Update page count */
    sprintf(fname_local, "of %d", app->pdf_pagecount);
    PtSetResource(app->nbpages, Pt_ARG_TEXT_STRING, fname_local, 0);

    /* Unblock input of page number */
    app->pdf_currpage=1;
    PtSetResource(app->nbpage, Pt_ARG_FLAGS, Pt_FALSE, Pt_BLOCKED);
    PtSetResource(app->nbpage, Pt_ARG_TEXT_STRING, "1", 0);

    /* Remove focus from input widgets */
    PtGiveFocus(app->phcontent, NULL);

    /* Flush all widget changes */
    PtFlush();

    app->pdf_opened=1;

    /* Create progress bar shadow */
    winargc=0;
    temp_point.x=(app->window.dimension.w-300)/2+1;
    temp_point.y=(app->window.dimension.h-40)/2+1;
    temp_dim.h=30;
    temp_dim.w=300;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0x20, 0x20, 0x20), 0);
    progress_shadow=PtCreateWidget(PtPane, app->phcontent, winargc, winargs);
    PtRealizeWidget(progress_shadow);
    winargc=0;
    temp_point.x=(app->window.dimension.w-300)/2-1;
    temp_point.y=(app->window.dimension.h-40)/2-1;
    temp_dim.h=30;
    temp_dim.w=300;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_PROGRESS_BAR_COLOR, PgRGB(0x28, 0x28, 0x28), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE, Pt_BOTTOM_ETCH | Pt_RIGHT_ETCH);
    /* Pre-cache only first 4 pages */
    if (PHMUPDF_RENDER_MAX_IMAGES>4)
    {
        if (app->pdf_pagecount>4)
        {
            spare=4;
        }
        else
        {
            spare=app->pdf_pagecount;
        }
    }
    else
    {
        if (app->pdf_pagecount>PHMUPDF_RENDER_MAX_IMAGES)
        {
            spare=PHMUPDF_RENDER_MAX_IMAGES;
        }
        else
        {
            spare=app->pdf_pagecount;
        }
    }

    PtSetArg(&winargs[winargc++], Pt_ARG_GAUGE_MAXIMUM, app->pdf_pagecount+spare, 0);
    progress=PtCreateWidget(PtProgress, app->phcontent, winargc, winargs);
    PtRealizeWidget(progress);
    PtFlush();

    /* Now load all pages description into the memory */
    app->pdf_pages=calloc(1, sizeof(*app->pdf_pages)*app->pdf_pagecount);
    /* Cycle through all pages */
    fz_try(app->pdfctx)
    {
        fz_matrix temp_ctm;
        fz_rect temp_rect;

        app->pdf_totallines=0;
        app->pdf_zoommax=INT_MAX;
        for (it=0; it<app->pdf_pagecount; it++)
        {
            PtSetResource(progress, Pt_ARG_GAUGE_VALUE, it, 0);
            PtDamageWidget(app->phcontent);
            PtFlush();

            app->pdf_pages[it].page=fz_load_page(app->pdfdoc, it);
            fz_bound_page(app->pdfdoc, app->pdf_pages[it].page, &app->pdf_pages[it].page_bbox);

            if (app->settings.fast_render)
            {
                fz_cookie cookie={0};

                cookie.incomplete_ok=1;
                app->pdf_pages[it].page_list=fz_new_display_list(app->pdfctx);
                pdfdev=fz_new_list_device(app->pdfctx, app->pdf_pages[it].page_list);
                fz_run_page(app->pdfdoc, app->pdf_pages[it].page, pdfdev, &fz_identity, &cookie);
                fz_free_device(pdfdev);
            }
            else
            {
                app->pdf_pages[it].page_list=NULL;
            }

            /* Respect settings */
            if ((app->settings.fast_render)&&(app->settings.fast_search))
            {
                app->pdf_pages[it].page_sheet=fz_new_text_sheet(app->pdfctx);
                app->pdf_pages[it].page_text=fz_new_text_page(app->pdfctx);
                pdfdev=fz_new_text_device(app->pdfctx, app->pdf_pages[it].page_sheet, app->pdf_pages[it].page_text);
                fz_run_display_list(app->pdf_pages[it].page_list, pdfdev, &fz_identity, &fz_infinite_rect, NULL);
                fz_free_device(pdfdev);
            }
            else
            {
                app->pdf_pages[it].page_text=NULL;
                app->pdf_pages[it].page_sheet=NULL;
            }

            /* Load text links */
            app->pdf_pages[it].page_links=fz_load_links(app->pdfdoc, app->pdf_pages[it].page);

            /* Obtain dimensions of rotated image */
            fz_scale(&app->pdf_pages[it].page_ctm, 1.0f, 1.0f);
            fz_concat(&app->pdf_pages[it].page_ctm, &app->pdf_pages[it].page_ctm, fz_rotate(&temp_ctm, app->rotation));
            app->pdf_pages[it].page_bbox_rot=app->pdf_pages[it].page_bbox;
            fz_transform_rect(&app->pdf_pages[it].page_bbox_rot, &app->pdf_pages[it].page_ctm);

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

            /* Obtain dimensions of scaled image */
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

            /* If fast render is not enabled, remove page data */
            if (app->settings.fast_render==0)
            {
                if (app->pdf_pages[it].page)
                {
                    fz_free_page(app->pdfdoc, app->pdf_pages[it].page);
                    app->pdf_pages[it].page=NULL;
                }
            }
        }
        /* Add some gap between pages, 16 pixels between pages */
        app->pdf_totallines+=app->pdf_pagecount*PHMUPDF_GAP_BETWEEN_PAGES;
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
    }

    fz_catch(app->pdfctx)
    {
        /* Destroy progress bar widgets */
        if (progress)
        {
            PtDestroyWidget(progress);
            progress=NULL;
        }
        if (progress_shadow)
        {
            PtDestroyWidget(progress_shadow);
            progress_shadow=NULL;
        }
        PtDamageWidget(app->phcontent);
        PtFlush();

        for (it=0; it<app->pdf_pagecount; it++)
        {
            if (app->pdf_pages[it].page_links)
            {
                fz_drop_link(app->pdfctx, app->pdf_pages[it].page_links);
                app->pdf_pages[it].page_links=NULL;
            }
            if (app->pdf_pages[it].page_text)
            {
                fz_free_text_page(app->pdfctx, app->pdf_pages[it].page_text);
                app->pdf_pages[it].page_text=NULL;
            }
            if (app->pdf_pages[it].page_sheet)
            {
                fz_free_text_sheet(app->pdfctx, app->pdf_pages[it].page_sheet);
                app->pdf_pages[it].page_sheet=NULL;
            }
            if (app->pdf_pages[it].page_list)
            {
                fz_drop_display_list(app->pdfctx, app->pdf_pages[it].page_list);
                app->pdf_pages[it].page_list=NULL;
            }
            if (app->pdf_pages[it].page)
            {
                fz_free_page(app->pdfdoc, app->pdf_pages[it].page);
                app->pdf_pages[it].page=NULL;
            }
        }
        free(app->pdf_pages);
        app->pdf_pages=NULL;

        PtNotice(app->phwindow, NULL, "File processing error", NULL,
            "Can't parse all document pages!", NULL, NULL, NULL,
            Pt_MODAL);

        strcpy(app->window.title, PHMUPDF_DEFAULT_TITLE);
        PtSetResource(app->phwindow, Pt_ARG_WINDOW_TITLE, app->window.title, strlen(app->window.title));
        app->pdf_opened=0;
        app->filename_set=0;
        return -1;
    }

    /* Pre-cache first pages */
    for (jt=0; jt<spare; jt++)
    {
        /* Load first page at slot zero */
        phpdf_load_page(app, jt, jt);

        PtSetResource(progress, Pt_ARG_GAUGE_VALUE, it+jt, 0);
        PtDamageWidget(app->phcontent);
        PtFlush();
    }

    /* Destroy progress bar widgets */
    if (progress)
    {
        PtDestroyWidget(progress);
        progress=NULL;
    }
    if (progress_shadow)
    {
        PtDestroyWidget(progress_shadow);
        progress_shadow=NULL;
    }
    PtDamageWidget(app->phcontent);
    PtFlush();

    /* Create page scroll bar (vertical) */
    winargc=0;
    temp_point.x=app->window.dimension.w-PHMUPDF_SCROLLBAR_WIDTH;
    temp_point.y=0;
    temp_dim.h=app->window.dimension.h;
    temp_dim.w=PHMUPDF_SCROLLBAR_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_COLOR, PgRGB(0xC0, 0xC0, 0xC0), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_MAXIMUM, app->pdf_totallines, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_GAUGE_VALUE, 0, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_INCREMENT, 1, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_PAGE_INCREMENT,
        app->window.dimension.h-PHMUPDF_NAVIBAR_HEIGHT, 0);

    temp_cb.event_f=phmupdf_scrollbar_move_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_SCROLL_MOVE, &temp_cb, 0);

    raw_cb.event_mask=Ph_EV_KEY;
    raw_cb.event_f=phmupdf_scrollbar_callback_raw;
    raw_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_RAW, &raw_cb, 0);

    app->phpagescroll=PtCreateWidget(PtScrollbar, app->phcontent, winargc, winargs);
    PtRealizeWidget(app->phpagescroll);

    /* Unblock input of zoom level */
    {
        char text[16];

        app->pdf_zoom=PHMUPDF_ZOOM_FIT_WIDTH;
        PtSetResource(app->nbzoomlevel, Pt_ARG_FLAGS, Pt_FALSE, Pt_BLOCKED);
        PtSetResource(app->nbzoomlevel, Pt_ARG_CBOX_SEL_ITEM, 5, 0);
        sprintf(text, "%d%%", app->pdf_zoommax);
        PtSetResource(app->nbzoomlevel, Pt_ARG_TEXT_STRING, text, 0);
    }

    app->search_hit=-1;
    app->search_hit_length=-1;
    app->display_found=0;

    /* Give focus to scrollbar widget */
    PtGiveFocus(app->phpagescroll, NULL);
    PtFlush();

    /* Update window content */
    PtDamageWidget(app->phcontent);

    /* Update cursor in case if it points to link or text */
    if ((app->last_x!=-1) && (app->last_y!=-1))
    {
        phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
        phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
    }

    return 0;
}

int phpdf_closefile(photon_mupdf_t* app)
{
    /* Destroy page cache */
    phpdf_destroy_page_cache(app);

    if (app->pdf_pages)
    {
        int it;

        /* Cycle through all pages */
        for (it=0; it<app->pdf_pagecount; it++)
        {
            if (app->pdf_pages[it].page_links)
            {
                fz_drop_link(app->pdfctx, app->pdf_pages[it].page_links);
                app->pdf_pages[it].page_links=NULL;
            }
            if (app->pdf_pages[it].page_text)
            {
                fz_free_text_page(app->pdfctx, app->pdf_pages[it].page_text);
                app->pdf_pages[it].page_text=NULL;
            }
            if (app->pdf_pages[it].page_sheet)
            {
                fz_free_text_sheet(app->pdfctx, app->pdf_pages[it].page_sheet);
                app->pdf_pages[it].page_sheet=NULL;
            }
            if (app->pdf_pages[it].page_list)
            {
                fz_drop_display_list(app->pdfctx, app->pdf_pages[it].page_list);
                app->pdf_pages[it].page_list=NULL;
            }
            if (app->pdf_pages[it].page)
            {
                fz_free_page(app->pdfdoc, app->pdf_pages[it].page);
                app->pdf_pages[it].page=NULL;
            }

            app->pdf_pages[it].page_zoom=0.0f;
            memset(&app->pdf_pages[it].page_bbox, 0x00, sizeof(app->pdf_pages[it].page_bbox));
            memset(&app->pdf_pages[it].page_bbox_rot, 0x00, sizeof(app->pdf_pages[it].page_bbox_rot));
            memset(&app->pdf_pages[it].page_bbox_draw, 0x00, sizeof(app->pdf_pages[it].page_bbox_draw));
            memset(&app->pdf_pages[it].page_ctm, 0x00, sizeof(app->pdf_pages[it].page_ctm));
        }
        free(app->pdf_pages);
        app->pdf_pages=NULL;
    }

    /* Destroy scrollbar */
    if (app->phpagescroll)
    {
        PtDestroyWidget(app->phpagescroll);
        app->phpagescroll=NULL;
    }

    /* Remove focus from input widgets */
    PtGiveFocus(app->phcontent, NULL);

    /* On file close remove total pages string */
    PtSetResource(app->nbpages, Pt_ARG_TEXT_STRING, "", 0);

    /* Block input of page number */
    PtSetResource(app->nbpage, Pt_ARG_TEXT_STRING, "", 0);
    PtSetResource(app->nbpage, Pt_ARG_FLAGS, Pt_TRUE, Pt_BLOCKED);

    /* Block input of zoom level */
    PtSetResource(app->nbzoomlevel, Pt_ARG_TEXT_STRING, "", 0);
    PtSetResource(app->nbzoomlevel, Pt_ARG_FLAGS, Pt_TRUE, Pt_BLOCKED);

    /* Destroy search bar if present */
    if (app->phsearch_pane)
    {
        PtDestroyWidget(app->phsearch_pane);
        app->phsearch_pane=NULL;
    }
    if (app->phsearch_shadow)
    {
        PtDestroyWidget(app->phsearch_shadow);
        app->phsearch_shadow=NULL;
    }

    /* Remove outline */
    if (app->pdf_outline)
    {
        fz_free_outline(app->pdfctx, app->pdf_outline);
        app->pdf_outline=NULL;
    }

    /* Remove PDF document resources */
    if (app->pdfdoc)
    {
        fz_close_document(app->pdfdoc);
        app->pdfdoc=NULL;
    }

    /* Flush all waiting warnings */
    fz_flush_warnings(app->pdfctx);

    /* Destroy context */
    if (app->pdfctx!=NULL)
    {
        fz_free_context(app->pdfctx);
        app->pdfctx=NULL;
    }

    /* Clear total lines param */
    app->pdf_totallines=0;

    /* Mark status of opened PDF document */
    app->pdf_opened=0;

    /* Flush all changes */
    PtDamageWidget(app->phcontent);
    PtFlush();

    return 0;
}

inline void* ff_memcpy(void* b, const void* a, size_t n)
{
    /* Use compiler vectorization with SSE2 extension */
    char* s1 = b;
    const char* s2 = a;

    for(; 0<n; --n)
    {
        *s1++ = *s2++;
    }

    return b;
}

int phpdf_load_page(photon_mupdf_t* app, int pageno, int slotno)
{
    fz_device* pdf_draw_device=NULL;
    fz_pixmap* pdf_pixmap=NULL;
    fz_rect    temp_rect;
    uint8_t*   iline;
    uint8_t*   oline;
    int        pixmap_components;
    int        it;
    int        jt;

    if (app->pdf_pages[pageno].page==NULL)
    {
        app->pdf_pages[pageno].page=fz_load_page(app->pdfdoc, pageno);
    }

    if (app->pdf_pages[pageno].page_list==NULL)
    {
        fz_cookie cookie={0};

        cookie.incomplete_ok=1;
        app->pdf_pages[pageno].page_list=fz_new_display_list(app->pdfctx);
        pdf_draw_device=fz_new_list_device(app->pdfctx, app->pdf_pages[pageno].page_list);
        fz_run_page(app->pdfdoc, app->pdf_pages[pageno].page, pdf_draw_device, &fz_identity, &cookie);
        fz_free_device(pdf_draw_device);
        pdf_draw_device=NULL;
    }

    /* Render PDF page to system memory */
    if (app->settings.bw_render)
    {
        pdf_pixmap=fz_new_pixmap_with_bbox(app->pdfctx, fz_device_gray(app->pdfctx),
            &app->pdf_pages[pageno].page_bbox_draw);
    }
    else
    {
        pdf_pixmap=fz_new_pixmap_with_bbox(app->pdfctx, fz_device_bgr(app->pdfctx),
            &app->pdf_pages[pageno].page_bbox_draw);
    }
    fz_clear_pixmap_with_value(app->pdfctx, pdf_pixmap, 255);
    pdf_draw_device=fz_new_draw_device(app->pdfctx, pdf_pixmap);
    fz_rect_from_irect(&temp_rect, &app->pdf_pages[pageno].page_bbox_draw);
    fz_run_display_list(app->pdf_pages[pageno].page_list, pdf_draw_device,
        &app->pdf_pages[pageno].page_ctm, &temp_rect, NULL);
    fz_free_device(pdf_draw_device);

    /* If fast render is not enabled, destroy page list */
    if ((app->settings.fast_render==0) || (app->settings.fast_search==0))
    {
        if (app->pdf_pages[pageno].page_list)
        {
            fz_drop_display_list(app->pdfctx, app->pdf_pages[pageno].page_list);
            app->pdf_pages[pageno].page_list=NULL;
        }
        if (app->pdf_pages[pageno].page)
        {
            fz_free_page(app->pdfdoc, app->pdf_pages[pageno].page);
            app->pdf_pages[pageno].page=NULL;
        }
    }

    /* Create photon image */
    if (app->page_image[slotno].page_image_phi!=NULL)
    {
        /* Destroy an image if it is present in slot */
        PgShmemDestroy(app->page_image[slotno].page_image_phi->image);
        app->page_image[slotno].page_image_phi->flags&=~(Ph_RELEASE_IMAGE);
        PhReleaseImage(app->page_image[slotno].page_image_phi);
        free(app->page_image[slotno].page_image_phi);
        app->page_image[slotno].page_image_phi=NULL;
        app->page_image[slotno].pageno=-1;
    }
    if (app->page_image[slotno].page_image_off!=NULL)
    {
        PhDCRelease(app->page_image[slotno].page_image_off);
        app->page_image[slotno].page_image_off=NULL;
    }

    if (app->settings.off_render)
    {
        app->page_image[slotno].page_image_off=PdCreateOffscreenContext(app->display_format,
            (app->pdf_pages[pageno].page_bbox_draw.x1-app->pdf_pages[pageno].page_bbox_draw.x0),
            (app->pdf_pages[pageno].page_bbox_draw.y1-app->pdf_pages[pageno].page_bbox_draw.y0),
            Pg_OSC_MEM_2D_WRITABLE | Pg_OSC_MEM_2D_READABLE | Pg_OSC_MEM_PAGE_ALIGN);
    }
    /* Check if offscreen surface was not created or it was disabled */
    if (app->page_image[slotno].page_image_off==NULL)
    {
        app->page_image[slotno].page_image_phi=PhCreateImage(NULL,
            (app->pdf_pages[pageno].page_bbox_draw.x1-app->pdf_pages[pageno].page_bbox_draw.x0),
            (app->pdf_pages[pageno].page_bbox_draw.y1-app->pdf_pages[pageno].page_bbox_draw.y0),
            app->display_format, NULL, 0, 1);
    }

    pixmap_components=fz_pixmap_components(app->pdfctx, pdf_pixmap);

    /* Copy image content */
    if (app->settings.bw_render)
    {
        void* temp_ptr=NULL;
        int   w=0;
        int   h=0;
        int   pitch=0;

        if (app->page_image[slotno].page_image_off!=NULL)
        {
            temp_ptr=PdGetOffscreenContextPtr(app->page_image[slotno].page_image_off);
            w=app->page_image[slotno].page_image_off->dim.w;
            h=app->page_image[slotno].page_image_off->dim.h;
            pitch=app->page_image[slotno].page_image_off->pitch;
        }
        else
        {
            temp_ptr=app->page_image[slotno].page_image_phi->image;
            w=app->page_image[slotno].page_image_phi->size.w;
            h=app->page_image[slotno].page_image_phi->size.h;
            pitch=app->page_image[slotno].page_image_phi->bpl;
        }

        for (it=0; it<h; it++)
        {
            iline=(unsigned char*)fz_pixmap_samples(app->pdfctx, pdf_pixmap);
            iline=iline+it*fz_pixmap_width(app->pdfctx, pdf_pixmap)*pixmap_components;
            oline=(uint8_t*)temp_ptr+it*pitch;

            switch(app->display_format)
            {
                case Pg_IMAGE_DIRECT_1555:
                     for (jt=0; jt<w; jt++)
                     {
                         *((uint16_t*)oline)=(*(iline)>>3)|((*(iline)>>3)<<5)|((*(iline)>>3)<<10);
                         iline+=pixmap_components;
                         oline+=2;
                     }
                     break;
                case Pg_IMAGE_DIRECT_565:
                     for (jt=0; jt<w; jt++)
                     {
                         *((uint16_t*)oline)=(*(iline)>>3)|((*(iline)>>2)<<5)|((*(iline)>>3)<<11);
                         iline+=pixmap_components;
                         oline+=2;
                     }
                     break;
                case Pg_IMAGE_DIRECT_8888:
                     for (jt=0; jt<w; jt++)
                     {
                         *((uint32_t*)oline)=*(iline)+(*(iline)<<8)+(*(iline)<<16);
                         iline+=pixmap_components;
                         oline+=4;
                     }
                     break;
                case Pg_IMAGE_DIRECT_888:
                     for (jt=0; jt<w; jt++)
                     {
                         *((uint32_t*)oline)=*(iline)+(*(iline)<<8)+(*(iline)<<16);
                         iline+=pixmap_components;
                         oline+=3;
                     }
                     break;
            }
        }
    }
    else
    {
        void* temp_ptr=NULL;
        int   w=0;
        int   h=0;
        int   pitch=0;

        if (app->page_image[slotno].page_image_off!=NULL)
        {
            temp_ptr=PdGetOffscreenContextPtr(app->page_image[slotno].page_image_off);
            w=app->page_image[slotno].page_image_off->dim.w;
            h=app->page_image[slotno].page_image_off->dim.h;
            pitch=app->page_image[slotno].page_image_off->pitch;
        }
        else
        {
            temp_ptr=app->page_image[slotno].page_image_phi->image;
            w=app->page_image[slotno].page_image_phi->size.w;
            h=app->page_image[slotno].page_image_phi->size.h;
            pitch=app->page_image[slotno].page_image_phi->bpl;
        }

        for (it=0; it<h; it++)
        {
            iline=(unsigned char*)fz_pixmap_samples(app->pdfctx, pdf_pixmap);
            iline=iline+it*fz_pixmap_width(app->pdfctx, pdf_pixmap)*pixmap_components;
            oline=(uint8_t*)temp_ptr+it*pitch;

            switch(app->display_format)
            {
                case Pg_IMAGE_DIRECT_1555:
                     if (w & 1)
                     {
                         for (jt=0; jt<w; jt++)
                         {
                             *((uint16_t*)oline)=(*(iline+0)>>3)|((*(iline+1)>>3)<<5)|((*(iline+2)>>3)<<10);
                             iline+=pixmap_components;
                             oline+=2;
                         }
                     }
                     else
                     {
                         /* Try to process two pixels at once */
                         for (jt=0; jt<w/2; jt++)
                         {
                             *((uint32_t*)oline)=(*(iline+0)>>3)|((*(iline+1)>>3)<<5)|((*(iline+2)>>3)<<10)|
                                               (((*(iline+4)>>3)|((*(iline+5)>>3)<<5)|((*(iline+6)>>3)<<10))<<16);
                             iline+=pixmap_components<<1;
                             oline+=4;
                         }
                     }
                     break;
                case Pg_IMAGE_DIRECT_565:
                     if (w & 1)
                     {
                         for (jt=0; jt<w; jt++)
                         {
                             *((uint16_t*)oline)=(*(iline+0)>>3)|((*(iline+1)>>2)<<5)|((*(iline+2)>>3)<<11);
                             iline+=pixmap_components;
                             oline+=2;
                         }
                     }
                     else
                     {
                         /* Try to process two pixels at once */
                         for (jt=0; jt<w>>1; jt++)
                         {
                             *((uint32_t*)oline)=(*(iline+0)>>3)|((*(iline+1)>>2)<<5)|((*(iline+2)>>3)<<11)|
                                               (((*(iline+4)>>3)|((*(iline+5)>>2)<<5)|((*(iline+6)>>3)<<11))<<16);
                             iline+=pixmap_components<<1;
                             oline+=4;
                         }
                     }
                     break;
                case Pg_IMAGE_DIRECT_8888:
                     ff_memcpy(oline, iline, fz_pixmap_width(app->pdfctx, pdf_pixmap)*pixmap_components);
                     break;
                case Pg_IMAGE_DIRECT_888:
                     for (jt=0; jt<w; jt++)
                     {
                         *((uint32_t*)oline)=*(iline+0)+(*(iline+1)<<8)+(*(iline+2)<<16);
                         iline+=pixmap_components;
                         oline+=3;
                     }
                     break;
            }
        }
    }

    /* Store pageno in image cache */
    app->page_image[slotno].pageno=pageno;

    /* Free PDF image */
    fz_drop_pixmap(app->pdfctx, pdf_pixmap);

    return 0;
}

int phpdf_destroy_page_cache(photon_mupdf_t* app)
{
    int it;

    for (it=0; it<PHMUPDF_RENDER_MAX_IMAGES; it++)
    {
        /* Create photon image */
        if (app->page_image[it].page_image_phi!=NULL)
        {
            /* Destroy an image if it is present in slot */
            PgShmemDestroy(app->page_image[it].page_image_phi->image);
            app->page_image[it].page_image_phi->flags&=~(Ph_RELEASE_IMAGE);
            PhReleaseImage(app->page_image[it].page_image_phi);
            free(app->page_image[it].page_image_phi);
            app->page_image[it].page_image_phi=NULL;
        }
        if (app->page_image[it].page_image_off!=NULL)
        {
            PhDCRelease(app->page_image[it].page_image_off);
            app->page_image[it].page_image_off=NULL;
        }
        app->page_image[it].pageno=-1;
    }

    /* Update new cache */
    PtDamageWidget(app->phcontent);

    return 0;
}

int phpdf_find_mostdistance_page(photon_mupdf_t* app, int pageno)
{
    int slotno=0;
    int distance=0;
    int it;

    /* Check for free pages first */
    for (it=0; it<PHMUPDF_RENDER_MAX_IMAGES; it++)
    {
        if (app->page_image[it].pageno==-1)
        {
            return it;
        }
    }

    for (it=0; it<PHMUPDF_RENDER_MAX_IMAGES; it++)
    {
        if (abs(app->page_image[it].pageno-pageno)>distance)
        {
            distance=abs(app->page_image[it].pageno-pageno);
            slotno=it;
        }
    }

    return slotno;
}
