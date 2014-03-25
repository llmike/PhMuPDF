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

#include <ctype.h>
#include <utf8.h>

#include "phmupdf.h"
#include "phcallbacks.h"
#include "phpdf.h"

int phmupdf_change_zoom_level(photon_mupdf_t* app)
{
    unsigned int* old_scroll_position;
    unsigned int  old_totallines=0;
    unsigned int  old_sc=0;
    int           it=0;
    char          text[16];
    float         zoom_page_max;
    float         temp_zoom;
    float         temp_zoom2;
    float         temp_pagezoom;

    if (app->phpagescroll)
    {
        PtGetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, &old_scroll_position, 0);
        old_sc=*old_scroll_position;
        old_totallines=app->pdf_totallines;
    }

    /* Recalculate zoom level of each page to fit the window bounds */
    if (app->pdf_pages)
    {
        app->pdf_totallines=0;
        switch (app->pdf_zoom)
        {
            case PHMUPDF_ZOOM_FIT_WIDTH:
                 for (it=0; it<app->pdf_pagecount; it++)
                 {
                     fz_matrix temp_ctm;
                     fz_rect temp_rect;

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
                 break;
            case PHMUPDF_ZOOM_FIT_PAGE:
                 zoom_page_max=10000000.0f;
                 temp_pagezoom=0;
                 for (it=0; it<app->pdf_pagecount; it++)
                 {
                     temp_zoom=(float)(app->window.dimension.w-4-PHMUPDF_SCROLLBAR_WIDTH)*100.0f/
                         (float)(app->pdf_pages[it].page_bbox.x1-app->pdf_pages[it].page_bbox.x0);
                     temp_zoom2=(float)(app->window.dimension.h-4-PHMUPDF_NAVIBAR_HEIGHT)*100.0f/
                         (float)(app->pdf_pages[it].page_bbox.y1-app->pdf_pages[it].page_bbox.y0);
                     if (temp_zoom2<temp_zoom)
                     {
                         temp_zoom=temp_zoom2;
                     }

                     if (zoom_page_max>temp_zoom)
                     {
                         zoom_page_max=temp_zoom;
                         temp_pagezoom=app->pdf_pages[it].page_zoom;
                     }
                 }
                 app->pdf_zoom=zoom_page_max*app->pdf_zoommax/temp_pagezoom;
                 for (it=0; it<app->pdf_pagecount; it++)
                 {
                     fz_matrix temp_ctm;
                     fz_rect temp_rect;

                     fz_scale(&app->pdf_pages[it].page_ctm,
                         app->pdf_pages[it].page_zoom*app->pdf_zoom/app->pdf_zoommax/100.0f,
                         app->pdf_pages[it].page_zoom*app->pdf_zoom/app->pdf_zoommax/100.0f);
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
                 break;
            default:
                 for (it=0; it<app->pdf_pagecount; it++)
                 {
                     fz_matrix temp_ctm;
                     fz_rect temp_rect;

                     fz_scale(&app->pdf_pages[it].page_ctm,
                         app->pdf_pages[it].page_zoom*app->pdf_zoom/app->pdf_zoommax/100.0f,
                         app->pdf_pages[it].page_zoom*app->pdf_zoom/app->pdf_zoommax/100.0f);
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
                 break;
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

    if (app->pdf_zoom==PHMUPDF_ZOOM_FIT_WIDTH)
    {
        sprintf(text, "%d%%", app->pdf_zoommax);
        PtSetResource(app->nbzoomlevel, Pt_ARG_TEXT_STRING, text, 0);
    }
    else
    {
        sprintf(text, "%d%%", app->pdf_zoom);
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

    /* Update cursor in case if it points to link or text */
    if ((app->last_x!=-1) && (app->last_y!=-1))
    {
        phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
        phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
    }

    return 0;
}

void phmupdf_push_position(photon_mupdf_t* app, unsigned int position)
{
    int it;

    /* Store old position in history for navigation */
    if (app->jump_stack_ptr==-1)
    {
        app->jump_stack_ptr=0;
    }
    if (app->jump_stack_ptr<PHMUPDF_JUMP_STACK_SIZE)
    {
        app->jump_stack[app->jump_stack_ptr]=position;
        app->jump_stack_last_valid=app->jump_stack_ptr;
        app->jump_stack_ptr++;
    }
    else
    {
        /* Move all stack down */
        for (it=0; it<PHMUPDF_JUMP_STACK_SIZE-1; it++)
        {
            app->jump_stack[it]=app->jump_stack[it+1];
        }
        app->jump_stack[app->jump_stack_ptr-1]=position;
    }
}

int phmupdf_main_callback_destroyed(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    app=(photon_mupdf_t*)data;

    app->window.event_flags|=PHW_WINDOW_EXIT_REQUESTED;

    /* Free resources */
    phpdf_destroy_page_cache(app);
    if (app->pdf_opened)
    {
        phpdf_closefile(app);
    }

    /* Save settings */
    {
        int fd=-1;
        char* home=getenv("HOME");
        char  text[PATH_MAX];

        sprintf(text, "%s/.phmupdf", home);
        fd=open(text, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (fd!=-1)
        {
            write(fd, &app->settings, sizeof(app->settings));
            close(fd);
        }
    }

    return Pt_CONTINUE;
}

int phmupdf_main_callback_raw(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    switch (info->event->type)
    {
         case Ph_EV_KEY:
              {
                  PhKeyEvent_t* keyevent=NULL;

                  keyevent=PhGetData(info->event);
                  if (keyevent==NULL)
                  {
                      break;
                  }

                  if ((keyevent->key_flags & Pk_KF_Key_Repeat)==Pk_KF_Key_Repeat)
                  {
                      break;
                  }

                  /* Check if key has been translated to the symbol */
                  if ((keyevent->key_flags & Pk_KF_Sym_Valid)==Pk_KF_Sym_Valid)
                  {
                      switch(keyevent->key_sym)
                      {
                          case Pk_F1:
                               {
                                   phmupdf_information_callback(widget, data, info);
                                   return Pt_CONSUME;
                               }
                               break;
                          case Pk_F3:
                               {
                                   phmupdf_openfile_callback(widget, data, info);
                                   return Pt_CONSUME;
                               }
                               break;
                          case Pk_F7:
                               {
                                   phmupdf_search_callback(widget, data, info);
                                   return Pt_CONSUME;
                               }
                               break;
                      }
                  }
                  else
                  {
                      if ((keyevent->key_flags & Pk_KF_Scan_Valid)==Pk_KF_Scan_Valid)
                      {
                          switch(keyevent->key_scan)
                          {
                              case 0x000C: /* Ctrl-minus */
                              case 0x004A: /* Ctrl-minus on KP */
                                   {
                                       phmupdf_zoomout_callback(widget, data, info);
                                       return Pt_CONSUME;
                                   }
                                   break;
                              case 0x000D: /* Ctrl-plus */
                              case 0x004E: /* Ctrl-plus on KP */
                                   {
                                       phmupdf_zoomin_callback(widget, data, info);
                                       return Pt_CONSUME;
                                   }
                                   break;
                              case 0x0013: /* Ctrl-R */
                                   {
                                       if (keyevent->key_mods==Pk_KM_Ctrl)
                                       {
                                           phmupdf_rotation_callback(widget, data, info);
                                       }
                                       return Pt_CONSUME;
                                   }
                                   break;
                              case 0x0018: /* Ctrl-O */
                                   {
                                       if (keyevent->key_mods==Pk_KM_Ctrl)
                                       {
                                           phmupdf_openfile_callback(widget, data, info);
                                       }
                                       return Pt_CONSUME;
                                   }
                                   break;
                              case 0x0017: /* Ctrl-I */
                                   {
                                       if (keyevent->key_mods==Pk_KM_Ctrl)
                                       {
                                           phmupdf_properties_callback(widget, data, info);
                                       }
                                       return Pt_CONSUME;
                                   }
                                   break;
                              case 0x0021: /* Ctrl-F */
                                   {
                                       if (keyevent->key_mods==Pk_KM_Ctrl)
                                       {
                                           phmupdf_search_callback(widget, data, info);
                                           return Pt_CONSUME;
                                       }
                                   }
                                   break;
                              case 0x001F: /* Ctrl-S */
                                   {
                                       if (keyevent->key_mods==Pk_KM_Ctrl)
                                       {
                                           phmupdf_settings_callback(widget, data, info);
                                           return Pt_CONSUME;
                                       }
                                   }
                                   break;
                          }
                      }
                  }
              }
              break;
    }

    return Pt_CONTINUE;
}

void phmupdf_content_find_link(photon_mupdf_t* app, int x, int y, int buttons)
{
    unsigned int*     scroll_position;
    unsigned int      sc=0;
    unsigned int      sc_fixed=0;
    unsigned int      global_offset=0;
    int               pageno=-1;
    PhPoint_t         position;
    int               it;
    unsigned int      offset;

    PtGetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, &scroll_position, 0);
    sc=*scroll_position;
    sc_fixed=sc;

    do {
        /* Find a page, which corresponds current position */
        offset=0;
        pageno=-1;
        for (it=0; it<app->pdf_pagecount; it++)
        {
            if ((sc>=offset) &&
                (sc<offset+(app->pdf_pages[it].page_bbox_draw.y1-app->pdf_pages[it].page_bbox_draw.y0+1)+PHMUPDF_GAP_BETWEEN_PAGES))
            {
                pageno=it;
                offset=sc-offset;
                break;
            }
            offset+=(app->pdf_pages[it].page_bbox_draw.y1-app->pdf_pages[it].page_bbox_draw.y0+1)+PHMUPDF_GAP_BETWEEN_PAGES;
        }

        if (pageno==-1)
        {
            return;
        }

        /* Check on what page mouse has been clicked */
        if ((app->window.dimension.w-PHMUPDF_SCROLLBAR_WIDTH)>
            (app->pdf_pages[pageno].page_bbox_draw.x1-app->pdf_pages[pageno].page_bbox_draw.x0+1))
        {
            position.x=(app->window.dimension.w-PHMUPDF_SCROLLBAR_WIDTH-
                (app->pdf_pages[pageno].page_bbox_draw.x1-app->pdf_pages[pageno].page_bbox_draw.x0+1))/2;
        }
        else
        {
            position.x=2;
        }

        /* Check if click was outside of the page */
        if (x<position.x)
        {
            PtSetResource(app->phcontent, Pt_ARG_CURSOR_TYPE, app->last_cursor_type, 0);
            if (app->last_cursor_type==Ph_CURSOR_INSERT)
            {
                PtSetResource(app->phcontent, Pt_ARG_CURSOR_COLOR, PgRGB(0x00, 0x00, 0x00), 0);
            }
            else
            {
                PtSetResource(app->phcontent, Pt_ARG_CURSOR_COLOR, PgRGB(0xFF, 0xFF, 0xE0), 0);
            }
            return;
        }
        if (x>position.x+(app->pdf_pages[pageno].page_bbox_draw.x1-app->pdf_pages[pageno].page_bbox_draw.x0+1))
        {
            PtSetResource(app->phcontent, Pt_ARG_CURSOR_TYPE, app->last_cursor_type, 0);
            if (app->last_cursor_type==Ph_CURSOR_INSERT)
            {
                PtSetResource(app->phcontent, Pt_ARG_CURSOR_COLOR, PgRGB(0x00, 0x00, 0x00), 0);
            }
            else
            {
                PtSetResource(app->phcontent, Pt_ARG_CURSOR_COLOR, PgRGB(0xFF, 0xFF, 0xE0), 0);
            }
            return;
        }

        /* Check if click was inside of this page */
        it=global_offset+(app->pdf_pages[pageno].page_bbox_draw.y1-app->pdf_pages[pageno].page_bbox_draw.y0+1)-offset;
        if (app->window.dimension.h<it)
        {
            it=app->window.dimension.h;
        }
        if ((y>=global_offset) && (y<it))
        {
            x=x-position.x;
            y=y+offset-global_offset;

            /* Now search these coordinates through the rest of links on this page */
            if (app->pdf_pages[pageno].page_links)
            {
                fz_link*  link;
                fz_point  p={x, y};
                fz_matrix ctm;

                fz_invert_matrix(&ctm, &app->pdf_pages[pageno].page_ctm);
                fz_transform_point(&p, &ctm);

                for (link=app->pdf_pages[pageno].page_links; link; link=link->next)
                {
                    if (p.x>=link->rect.x0 && p.x<=link->rect.x1)
                    {
                        if (p.y>=link->rect.y0 && p.y<=link->rect.y1)
                        {
                            app->last_cursor_type=Ph_CURSOR_FINGER;
                            break;
                        }
                    }
                }

                /* Check if user clicked to this link */
                if ((link) && (buttons & Ph_BUTTON_SELECT))
                {
                    switch (link->dest.kind)
                    {
                        case FZ_LINK_URI:
                             {
                                 char text[PATH_MAX+NAME_MAX+1];

                                 /* Launch browser */
                                 snprintf(text, PATH_MAX+NAME_MAX+1, "firefox %s", link->dest.ld.uri.uri);
                                 system(text);
                             }
                             break;
                        case FZ_LINK_GOTO:
                             {
                                 char         text[16];
                                 unsigned int page_offset=0;

                                 sprintf(text, "%d", link->dest.ld.gotor.page+1);
                                 PtSetResource(app->nbpage, Pt_ARG_TEXT_STRING, text, 0);
                                 app->pdf_currpage=link->dest.ld.gotor.page+1;
                                 PtDamageWidget(app->nbpage);

                                 /* Check if goto structure has valid point */
                                 if ((link->dest.ld.gotor.flags & (fz_link_flag_l_valid | fz_link_flag_t_valid))==
                                     (fz_link_flag_l_valid | fz_link_flag_t_valid))
                                 {
                                     int temp_y;

                                     temp_y=app->pdf_pages[app->pdf_currpage].page_bbox.y1-app->pdf_pages[app->pdf_currpage].page_bbox.y0;
                                     temp_y=temp_y-link->dest.ld.gotor.lt.y;
                                     temp_y=(float)temp_y*app->pdf_pages[app->pdf_currpage].page_zoom/100.0f;
                                     page_offset+=temp_y;
                                 }

                                 if (app->phpagescroll)
                                 {
                                     int it;
                                     unsigned int position;

                                     /* Adjust scrollbar to show required page */
                                     position=0;
                                     for (it=0; it<app->pdf_currpage-1; it++)
                                     {
                                         position+=(app->pdf_pages[it].page_bbox_draw.y1-app->pdf_pages[it].page_bbox_draw.y0)+1;
                                         position+=PHMUPDF_GAP_BETWEEN_PAGES;
                                     }
                                     position+=page_offset+1;
                                     PtSetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, position, 0);
                                     PtDamageWidget(app->phcontent);

                                     /* Update cursor in case if it points to link or text */
                                     if ((app->last_x!=-1) && (app->last_y!=-1))
                                     {
                                         phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
                                         phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
                                     }

                                     PtFlush();

                                     /* Store old position in history for navigation */
                                     phmupdf_push_position(app, sc_fixed);
                                 }
                             }
                             break;
                        default:
                             break;
                    }

                    /* Do not change cursor form */
                    PtSetResource(app->phcontent, Pt_ARG_CURSOR_TYPE, app->last_cursor_type, 0);
                    if (app->last_cursor_type==Ph_CURSOR_INSERT)
                    {
                        PtSetResource(app->phcontent, Pt_ARG_CURSOR_COLOR, PgRGB(0x00, 0x00, 0x00), 0);
                    }
                    else
                    {
                        PtSetResource(app->phcontent, Pt_ARG_CURSOR_COLOR, PgRGB(0xFF, 0xFF, 0xE0), 0);
                    }
                    return;
                }
                if (link)
                {
                    /* Do not change cursor form */
                    PtSetResource(app->phcontent, Pt_ARG_CURSOR_TYPE, app->last_cursor_type, 0);
                    if (app->last_cursor_type==Ph_CURSOR_INSERT)
                    {
                        PtSetResource(app->phcontent, Pt_ARG_CURSOR_COLOR, PgRGB(0x00, 0x00, 0x00), 0);
                    }
                    else
                    {
                        PtSetResource(app->phcontent, Pt_ARG_CURSOR_COLOR, PgRGB(0xFF, 0xFF, 0xE0), 0);
                    }
                    return;
                }
            }

            PtSetResource(app->phcontent, Pt_ARG_CURSOR_TYPE, app->last_cursor_type, 0);
            if (app->last_cursor_type==Ph_CURSOR_INSERT)
            {
                PtSetResource(app->phcontent, Pt_ARG_CURSOR_COLOR, PgRGB(0x00, 0x00, 0x00), 0);
            }
            else
            {
                PtSetResource(app->phcontent, Pt_ARG_CURSOR_COLOR, PgRGB(0xFF, 0xFF, 0xE0), 0);
            }
            return;
        }

        global_offset+=((app->pdf_pages[pageno].page_bbox_draw.y1-app->pdf_pages[pageno].page_bbox_draw.y0+1)-offset)+PHMUPDF_GAP_BETWEEN_PAGES;
        sc+=((app->pdf_pages[pageno].page_bbox_draw.y1-app->pdf_pages[pageno].page_bbox_draw.y0+1)-offset)+PHMUPDF_GAP_BETWEEN_PAGES;
        pageno++;
        /* No reason to draw outside of window dimensions */
        if (global_offset>=app->window.dimension.h)
        {
            break;
        }
        /* It is a last page */
        if (pageno==app->pdf_pagecount-1)
        {
            break;
        }
    }  while(1);

    PtSetResource(app->phcontent, Pt_ARG_CURSOR_TYPE, app->last_cursor_type, 0);
    if (app->last_cursor_type==Ph_CURSOR_INSERT)
    {
        PtSetResource(app->phcontent, Pt_ARG_CURSOR_COLOR, PgRGB(0x00, 0x00, 0x00), 0);
    }
    else
    {
        PtSetResource(app->phcontent, Pt_ARG_CURSOR_COLOR, PgRGB(0xFF, 0xFF, 0xE0), 0);
    }
}

void phmupdf_content_find_text(photon_mupdf_t* app, int x, int y, int buttons)
{
    fz_device*        pdf_device;
    unsigned int*     scroll_position;
    unsigned int      sc=0;
    unsigned int      global_offset=0;
    int               pageno=-1;
    PhPoint_t         position;
    int               it;
    unsigned int      offset;
    int               quit=0;

    PtGetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, &scroll_position, 0);
    sc=*scroll_position;

    do {
        /* Find a page, which corresponds current position */
        offset=0;
        pageno=-1;
        for (it=0; it<app->pdf_pagecount; it++)
        {
            if ((sc>=offset) &&
                (sc<offset+(app->pdf_pages[it].page_bbox_draw.y1-app->pdf_pages[it].page_bbox_draw.y0+1)+PHMUPDF_GAP_BETWEEN_PAGES))
            {
                pageno=it;
                offset=sc-offset;
                break;
            }
            offset+=(app->pdf_pages[it].page_bbox_draw.y1-app->pdf_pages[it].page_bbox_draw.y0+1)+PHMUPDF_GAP_BETWEEN_PAGES;
        }

        if (pageno==-1)
        {
            return;
        }

        /* Check on what page mouse has been clicked */
        if ((app->window.dimension.w-PHMUPDF_SCROLLBAR_WIDTH)>
            (app->pdf_pages[pageno].page_bbox_draw.x1-app->pdf_pages[pageno].page_bbox_draw.x0+1))
        {
            position.x=(app->window.dimension.w-PHMUPDF_SCROLLBAR_WIDTH-
                (app->pdf_pages[pageno].page_bbox_draw.x1-app->pdf_pages[pageno].page_bbox_draw.x0+1))/2;
        }
        else
        {
            position.x=2;
        }

        /* Check if click was outside of the page */
        if (x<position.x)
        {
            app->last_cursor_type=Ph_CURSOR_POINTER;
            return;
        }
        if (x>position.x+(app->pdf_pages[pageno].page_bbox_draw.x1-app->pdf_pages[pageno].page_bbox_draw.x0+1))
        {
            app->last_cursor_type=Ph_CURSOR_POINTER;
            return;
        }

        /* Check if click was inside of this page */
        it=global_offset+(app->pdf_pages[pageno].page_bbox_draw.y1-app->pdf_pages[pageno].page_bbox_draw.y0+1)-offset;
        if (app->window.dimension.h<it)
        {
            it=app->window.dimension.h;
        }
        if ((y>=global_offset) && (y<it))
        {
            int found_box=0;

            x=x-position.x;
            y=y+offset-global_offset;

            /* Check if page was not loaded */
            if (app->pdf_pages[pageno].page==NULL)
            {
                app->pdf_pages[pageno].page=fz_load_page(app->pdfdoc, pageno);
            }
            /* Check if page list was not loaded */
            if (app->pdf_pages[pageno].page_list==NULL)
            {
                fz_cookie cookie={0};

                cookie.incomplete_ok=1;
                app->pdf_pages[pageno].page_list=fz_new_display_list(app->pdfctx);
                pdf_device=fz_new_list_device(app->pdfctx, app->pdf_pages[pageno].page_list);
                fz_run_page(app->pdfdoc, app->pdf_pages[pageno].page, pdf_device, &fz_identity, &cookie);
                fz_free_device(pdf_device);
                pdf_device=NULL;
            }
            /* Check if text was not loaded */
            if (app->pdf_pages[pageno].page_text==NULL)
            {
                app->pdf_pages[pageno].page_sheet=fz_new_text_sheet(app->pdfctx);
                app->pdf_pages[pageno].page_text=fz_new_text_page(app->pdfctx);
                pdf_device=fz_new_text_device(app->pdfctx, app->pdf_pages[pageno].page_sheet, app->pdf_pages[pageno].page_text);
                fz_run_display_list(app->pdf_pages[pageno].page_list, pdf_device, &fz_identity, &fz_infinite_rect, NULL);
                fz_free_device(pdf_device);
            }


    if ((!app->selection) && (buttons & Ph_BUTTON_SELECT))
    {
        fz_point  p={x, y};
        fz_matrix ctm;

        fz_invert_matrix(&ctm, &app->pdf_pages[pageno].page_ctm);
        fz_transform_point(&p, &ctm);

        app->selection=1;
        app->selection_start=p;
        app->selection_page=pageno;
    }

    if ((app->selection) && (!(buttons & Ph_BUTTON_SELECT)))
    {
        fz_point  p={x, y};
        fz_matrix ctm;

        fz_invert_matrix(&ctm, &app->pdf_pages[pageno].page_ctm);
        fz_transform_point(&p, &ctm);

        app->selection=1;
        app->selection_stop=p;
        app->selection_page=pageno;
        if ((app->selection_start.x==app->selection_stop.x) &&
            (app->selection_start.y==app->selection_stop.y))
        {
            /* Reset selection on empty click */
            app->selection=0;
            app->selection_page=-1;
        }
    }

            /* Now search these coordinates through the rest of text blocks on this page */
            if (app->pdf_pages[pageno].page_text)
            {
                fz_point  p={x, y};
                fz_matrix ctm;
                fz_rect   bbox;
                fz_text_page*  page=app->pdf_pages[pageno].page_text;
                fz_text_block* block;
                fz_text_line*  line;
                fz_text_span*  span;
                int blocks;
                int i;

                fz_invert_matrix(&ctm, &app->pdf_pages[pageno].page_ctm);
                fz_transform_point(&p, &ctm);
                for (blocks=0; blocks<page->len; blocks++)
                {
                    if (page->blocks[blocks].type!=FZ_PAGE_BLOCK_TEXT)
                    {
                        continue;
                    }
                    block=page->blocks[blocks].u.text;

                    for (line=block->lines; line<block->lines+block->len; line++)
                    {
                        for (span=line->first_span; span; span=span->next)
                        {
                            for (i=0; i<span->len; i++)
                            {
                                fz_text_char_bbox(&bbox, span, i);
                                if (p.x>=bbox.x0 && p.x<=bbox.x1)
                                {
                                    if (p.y>=bbox.y0 && p.y<=bbox.y1)
                                    {
                                        app->last_cursor_type=Ph_CURSOR_INSERT;
                                        quit=1;
                                        found_box=1;
                                        break;
                                    }
                                }
                            }
                            if (quit)
                            {
                                break;
                            }
                        }
                        if (quit)
                        {
                            break;
                        }
                    }
                    if (quit)
                    {
                        break;
                    }
                    line=NULL;
                }
                if (line)
                {
                    /* Do not change cursor form */
                    quit=1;
                }
            }
            if (!found_box)
            {
                app->last_cursor_type=Ph_CURSOR_POINTER;
            }
            quit=1;
        }

        /* Release page data if it is requested by user */
        if ((app->settings.fast_render==0) || (app->settings.fast_search==0))
        {
            if (app->pdf_pages[app->display_page].page_text)
            {
                fz_free_text_page(app->pdfctx, app->pdf_pages[app->display_page].page_text);
                app->pdf_pages[app->display_page].page_text=NULL;
            }
            if (app->pdf_pages[app->display_page].page_sheet)
            {
                fz_free_text_sheet(app->pdfctx, app->pdf_pages[app->display_page].page_sheet);
                app->pdf_pages[app->display_page].page_sheet=NULL;
            }
            if ((app->pdf_pages[app->display_page].page_list)&&(app->settings.fast_render==0))
            {
                fz_drop_display_list(app->pdfctx, app->pdf_pages[app->display_page].page_list);
                app->pdf_pages[app->display_page].page_list=NULL;
            }
            if ((app->pdf_pages[app->display_page].page)&&(app->settings.fast_render==0))
            {
                fz_free_page(app->pdfdoc, app->pdf_pages[app->display_page].page);
                app->pdf_pages[app->display_page].page=NULL;
            }
        }

        if (quit)
        {
            quit=0;
            return;
        }

        global_offset+=((app->pdf_pages[pageno].page_bbox_draw.y1-app->pdf_pages[pageno].page_bbox_draw.y0+1)-offset)+PHMUPDF_GAP_BETWEEN_PAGES;
        sc+=((app->pdf_pages[pageno].page_bbox_draw.y1-app->pdf_pages[pageno].page_bbox_draw.y0+1)-offset)+PHMUPDF_GAP_BETWEEN_PAGES;
        pageno++;
        /* No reason to draw outside of window dimensions */
        if (global_offset>=app->window.dimension.h)
        {
            break;
        }
        /* It is a last page */
        if (pageno==app->pdf_pagecount-1)
        {
            break;
        }
    }  while(1);

    app->last_cursor_type=Ph_CURSOR_POINTER;
}

int phmupdf_content_callback_raw(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    short x, y;

    app=(photon_mupdf_t*)data;

    /* Just trace mouse position */
    switch (info->event->type)
    {
         case Ph_EV_BOUNDARY:
              {
                  switch (info->event->subtype)
                  {
                      case Ph_EV_PTR_LEAVE_TO_PARENT:
                      case Ph_EV_PTR_LEAVE_TO_CHILD:
                           {
                               app->last_x=-1;
                               app->last_y=-1;
                           }
                           break;
                  }
              }
              break;
         case Ph_EV_BUT_PRESS:
         case Ph_EV_BUT_RELEASE:
         case Ph_EV_PTR_MOTION_BUTTON:
         case Ph_EV_PTR_MOTION_NOBUTTON:
              {
                  PhPointerEvent_t* ptrevent=NULL;

                  ptrevent=PhGetData(info->event);
                  if (ptrevent==NULL)
                  {
                      break;
                  }

                  PtGetAbsPosition(app->phcontent, &x, &y);
                  x=ptrevent->pos.x-x;
                  y=ptrevent->pos.y-y;

                  app->last_x=x;
                  app->last_y=y;
              }
              break;
    }

    /* Don't do anything if document is not loaded */
    if ((app->pdf_opened==0) || (app->phpagescroll==NULL) || (app->pdf_pages==NULL))
    {
        return Pt_CONTINUE;
    }

    /* Do the rest of job if document was loaded */
    switch (info->event->type)
    {
         case Ph_EV_BUT_PRESS:
         case Ph_EV_BUT_RELEASE:
         case Ph_EV_PTR_MOTION_BUTTON:
         case Ph_EV_PTR_MOTION_NOBUTTON:
              {
                  PhPointerEvent_t* ptrevent=NULL;

                  ptrevent=PhGetData(info->event);
                  if (ptrevent==NULL)
                  {
                      break;
                  }

                  if ((info->event->type==Ph_EV_BUT_RELEASE) && (ptrevent->buttons & Ph_BUTTON_SELECT))
                  {
                      ptrevent->buttons=0;
                  }

                  if (info->event->type==Ph_EV_PTR_MOTION_BUTTON)
                  {
                      phmupdf_content_find_text(app, x, y, ptrevent->buttons);
                      /* Do not automatically press on links */
                      phmupdf_content_find_link(app, x, y, 0);
                  }
                  else
                  {
                      phmupdf_content_find_text(app, x, y, ptrevent->buttons);
                      phmupdf_content_find_link(app, x, y, ptrevent->buttons);
                  }

                  return Pt_CONTINUE;
              }
              break;
    }

    return Pt_CONTINUE;
}

int phmupdf_information_callback_ok(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    app=(photon_mupdf_t*)data;

    PtDestroyWidget(app->phwindow_about);

    return Pt_CONTINUE;
}

int phmupdf_information_callback_destroyed(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    app=(photon_mupdf_t*)data;

    app->phwindow_about=NULL;

    /* Unblock main window */
    PtSetResource(app->phwindow, Pt_ARG_WINDOW_STATE,
        Pt_FALSE, Ph_WM_STATE_ISBLOCKED);
    PtUnblockWindows(app->about_bl);

    return Pt_CONTINUE;
}

int phmupdf_information_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    PtArg_t         winargs[32];
    uint32_t        winargc=0;
    int32_t         status;
    PhDim_t         temp_dim;
    PhPoint_t       temp_point;
    PtCallback_t    temp_cb;

    app=(photon_mupdf_t*)data;

    if (app->phwindow_about==NULL)
    {
        temp_point.x=app->window.position.x+(app->window.dimension.w-280)/2;
        temp_point.y=app->window.position.y+(app->window.dimension.h-245)/2;
        temp_dim.w=280;
        temp_dim.h=245;
        PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_TITLE, "About MuPDF for Photon", 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_FALSE,
                 Ph_WM_RENDER_MIN | Ph_WM_RENDER_MAX |
                 Ph_WM_RENDER_COLLAPSE | Ph_WM_RENDER_RESIZE);
        PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_TRUE,
                 Ph_WM_RENDER_CLOSE | Ph_WM_RENDER_MENU |
                 Ph_WM_RENDER_TITLE | Ph_WM_RENDER_MOVE |
                 Ph_WM_RENDER_BORDER);
        PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_MANAGED_FLAGS, Pt_TRUE,
                 Ph_WM_APP_DEF_MANAGED);
        PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_STATE, Pt_TRUE,
                 Ph_WM_STATE_ISFOCUS | Ph_WM_STATE_ISFRONT);
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xC0, 0xC0, 0xC0), 0);

        temp_cb.event_f=phmupdf_information_callback_destroyed;
        temp_cb.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_IS_DESTROYED, &temp_cb, 0);

        /* Finally create the window */
        app->phwindow_about=PtCreateWidget(PtWindow, Pt_NO_PARENT, winargc, winargs);
        if (app->phwindow_about==NULL)
        {
            return -1;
        }

        /* Put PtButton widget under multitext */
        winargc=0;
        temp_point.x=110;
        temp_point.y=209;
        temp_dim.w=60;
        temp_dim.h=16;
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Ok", 0);
        temp_cb.event_f=phmupdf_information_callback_ok;
        temp_cb.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);
        PtCreateWidget(PtButton, app->phwindow_about, winargc, winargs);

        /* Put PtMultiText widget over window */
        winargc=0;
        temp_dim.w=280;
        temp_dim.h=205;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xC0, 0xC0, 0xC0), 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
            Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES | Pt_STATIC_GRADIENT);
        PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_FLAGS, Pt_FALSE,
            Pt_CURSOR_VISIBLE | Pt_EDITABLE);
        PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING,
            "\n                 MuPDF for Photon 1.3\n\n"
            " MuPDF 1.3\n Copyright 2006-2013 Artifex Software, Inc.\n\n"
            " PhMuPDF 1.3\n Copyright 2012-2014 Mike Gorchak\n\n"
            " All used icons were created by\n VisualPharm (http://www.visualpharm.com)", 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_TRUE, Pt_BLOCKED);
        PtCreateWidget(PtMultiText, app->phwindow_about, winargc, winargs);

        /* Show widget */
        status=PtRealizeWidget(app->phwindow_about);
        if (status!=0)
        {
            PtDestroyWidget(app->phwindow_about);
        }

        /* Block main window */
        PtSetResource(app->phwindow, Pt_ARG_WINDOW_STATE,
            Pt_TRUE, Ph_WM_STATE_ISBLOCKED);
        app->about_bl=PtBlockWindow(app->phwindow, Ph_CURSOR_NOINPUT,
            Ph_CURSOR_DEFAULT_COLOR);

        PtFlush();
    }
    else
    {
        PtSetResource(app->phwindow_about, Pt_ARG_WINDOW_STATE,
            Pt_TRUE, Ph_WM_STATE_ISFOCUS | Ph_WM_STATE_ISFRONT);

        PtFlush();
    }

    return Pt_CONTINUE;
}

int phmupdf_openfile_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    char            dirpath[PATH_MAX+NAME_MAX+1]={0};
    char*           dir=NULL;
    int             result;

    app=(photon_mupdf_t*)data;

    if (app->fileopen_dialog==1)
    {
        return -1;
    }

    app->fileopen_dialog=1;
    result=PtFileSelection(app->phwindow, NULL, "PhMuPDF: open file to read", app->settings.lastpath,
        "*.pdf *.PDF *.xps *.XPS *.rels *.RELS *.cbz *.CBZ *.ai *.AI *.png *.PNG *.jpg *.JPG *.jpeg *.JPEG *.jfif *.JFIF *.tif *.TIF *.tiff *.TIFF", NULL, NULL, "nskd", &app->fileinfo, 0);

    /* Something weird happen */
    if (result==-1)
    {
        app->fileopen_dialog=0;
        return -1;
    }

    /* Check if user have aborted file selection */
    if (app->fileinfo.ret!=Pt_FSDIALOG_BTN1)
    {
        app->fileopen_dialog=0;
        return Pt_CONTINUE;
    }

    /* Store directory and filename */
    strncpy(app->filename, app->fileinfo.path, PATH_MAX+NAME_MAX);
    strncpy(dirpath, app->fileinfo.path, PATH_MAX+NAME_MAX);
    dir=dirname(dirpath);
    strncpy(app->settings.lastpath, dir, PATH_MAX+NAME_MAX);
    app->filename_set=1;

    /* Open new file */
    phpdf_openfile(app);

    app->fileopen_dialog=0;

    return Pt_CONTINUE;
}

int phmupdf_pagecheck_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    PtTextCallback_t* tcb=(PtTextCallback_t*)info->cbdata;
    int it;

    /* Check if user tries to input correct data */
    if (tcb->length!=0)
    {
        for (it=0; it<tcb->length; it++)
        {
            if ((tcb->text[it]<'0') || (tcb->text[it]>'9'))
            {
                /* Do not allow such modifications to text */
                tcb->doit=0;
                return Pt_CONTINUE;
            }
        }
    }

    /* Allow such modifications to text */
    tcb->doit=1;

    return Pt_CONTINUE;
}

int phmupdf_pageselect_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    PtTextCallback_t* tcb=(PtTextCallback_t*)info->cbdata;
    char text[16];
    int  pageno;

    app=(photon_mupdf_t*)data;

    if (info->reason_subtype==Pt_EDIT_ACTIVATE)
    {
        if (sscanf(tcb->text, "%d", &pageno)==1)
        {
            if (pageno>app->pdf_pagecount)
            {
                /* Just replace string */
                pageno=app->pdf_pagecount;
                sprintf(text, "%d", app->pdf_pagecount);
                PtSetResource(widget, Pt_ARG_TEXT_STRING, text, 0);
            }
            else
            {
                if (pageno==0)
                {
                    pageno=1;
                    text[0]='1';
                    text[1]=0x00;
                    PtSetResource(widget, Pt_ARG_TEXT_STRING, text, 0);
                }
                else
                {
                    sprintf(text, "%d", pageno);
                    PtSetResource(widget, Pt_ARG_TEXT_STRING, text, 0);
                }
            }
        }
        else
        {
            pageno=app->pdf_currpage;
            sprintf(text, "%d", app->pdf_currpage);
            PtSetResource(widget, Pt_ARG_TEXT_STRING, text, 0);
        }
        app->pdf_currpage=pageno;
        PtContainerNullFocus(widget, NULL);

        /* Adjust scrollbar settings */
        if (app->phpagescroll)
        {
            int it;
            unsigned int position;

            PtGiveFocus(app->phpagescroll, NULL);

            /* Adjust scrollbar to show required page */
            position=0;
            for (it=0; it<app->pdf_currpage-1; it++)
            {
                position+=(app->pdf_pages[it].page_bbox_draw.y1-app->pdf_pages[it].page_bbox_draw.y0)+1;
                position+=PHMUPDF_GAP_BETWEEN_PAGES;
            }
            PtSetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, position, 0);

            /* Update cursor in case if it points to link or text */
            if ((app->last_x!=-1) && (app->last_y!=-1))
            {
                phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
                phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
            }
        }

        /* Update visible information */
        PtDamageWidget(app->phcontent);
    }

    return Pt_CONTINUE;
}

int phmupdf_pageraw_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    char text[16];

    app=(photon_mupdf_t*)data;

    switch (info->event->type)
    {
        case Ph_EV_KEY:
             {
                  PhKeyEvent_t* keyevent=NULL;

                  keyevent=PhGetData(info->event);
                  if (keyevent==NULL)
                  {
                      break;
                  }

                  if ((keyevent->key_flags & Pk_KF_Key_Repeat)==Pk_KF_Key_Repeat)
                  {
                      break;
                  }

                  /* Check if key has been translated to the symbol */
                  if ((keyevent->key_flags & Pk_KF_Sym_Valid)==Pk_KF_Sym_Valid)
                  {
                      switch(keyevent->key_sym)
                      {
                          case Pk_Escape:
                               {
                                   sprintf(text, "%d", app->pdf_currpage);
                                   PtSetResource(widget, Pt_ARG_TEXT_STRING, text, 0);
                                   PtContainerNullFocus(widget, NULL);
                                   if (app->phpagescroll)
                                   {
                                       PtGiveFocus(app->phpagescroll, NULL);
                                   }
                                   return Pt_CONSUME;
                               }
                               break;
                      }
                  }
             }
             break;
    }

    return Pt_CONTINUE;
}

int phmupdf_zoomcheck_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t*   app;
    PtTextCallback_t* tcb=(PtTextCallback_t*)info->cbdata;
    int  it;

    app=(photon_mupdf_t*)data;

    if ((tcb->length==8) && (strcmp(tcb->text, "Fit Page")==0))
    {
        /* TODO: Handle fit page */
        PtContainerNullFocus(widget, NULL);
        if (app->phpagescroll)
        {
            PtGiveFocus(app->phpagescroll, NULL);
        }

        app->pdf_zoom=PHMUPDF_ZOOM_FIT_PAGE;
        phmupdf_change_zoom_level(app);
        phmupdf_search_recreate_boxes(app);
        app->selection=0;

        tcb->doit=0;
        return Pt_CONTINUE;
    }

    /* Handle fit width */
    if ((tcb->length==9) && (strcmp(tcb->text, "Fit Width")==0))
    {
        PtContainerNullFocus(widget, NULL);
        if (app->phpagescroll)
        {
            PtGiveFocus(app->phpagescroll, NULL);
        }

        app->pdf_zoom=PHMUPDF_ZOOM_FIT_WIDTH;
        phmupdf_change_zoom_level(app);
        phmupdf_search_recreate_boxes(app);
        app->selection=0;

        tcb->doit=0;
        return Pt_CONTINUE;
    }

    /* Handle 10% width */
    if ((tcb->length==3) && (strcmp(tcb->text, "10%")==0))
    {
        PtContainerNullFocus(widget, NULL);
        if (app->phpagescroll)
        {
            PtGiveFocus(app->phpagescroll, NULL);
        }

        app->pdf_zoom=10;
        if (app->pdf_zoom>app->pdf_zoommax)
        {
            app->pdf_zoom=PHMUPDF_ZOOM_FIT_WIDTH;
        }
        phmupdf_change_zoom_level(app);
        phmupdf_search_recreate_boxes(app);
        app->selection=0;

        tcb->doit=0;
        return Pt_CONTINUE;
    }

    /* Handle 25% width */
    if ((tcb->length==3) && (strcmp(tcb->text, "25%")==0))
    {
        PtContainerNullFocus(widget, NULL);
        if (app->phpagescroll)
        {
            PtGiveFocus(app->phpagescroll, NULL);
        }

        app->pdf_zoom=25;
        if (app->pdf_zoom>app->pdf_zoommax)
        {
            app->pdf_zoom=PHMUPDF_ZOOM_FIT_WIDTH;
        }
        phmupdf_change_zoom_level(app);
        phmupdf_search_recreate_boxes(app);
        app->selection=0;

        tcb->doit=0;
        return Pt_CONTINUE;
    }

    /* Handle 50% width */
    if ((tcb->length==3) && (strcmp(tcb->text, "50%")==0))
    {
        PtContainerNullFocus(widget, NULL);
        if (app->phpagescroll)
        {
            PtGiveFocus(app->phpagescroll, NULL);
        }

        app->pdf_zoom=50;
        if (app->pdf_zoom>app->pdf_zoommax)
        {
            app->pdf_zoom=PHMUPDF_ZOOM_FIT_WIDTH;
        }
        phmupdf_change_zoom_level(app);
        phmupdf_search_recreate_boxes(app);
        app->selection=0;

        tcb->doit=0;
        return Pt_CONTINUE;
    }

    /* Handle 75% width */
    if ((tcb->length==3) && (strcmp(tcb->text, "75%")==0))
    {
        PtContainerNullFocus(widget, NULL);
        if (app->phpagescroll)
        {
            PtGiveFocus(app->phpagescroll, NULL);
        }

        app->pdf_zoom=75;
        if (app->pdf_zoom>app->pdf_zoommax)
        {
            app->pdf_zoom=PHMUPDF_ZOOM_FIT_WIDTH;
        }
        phmupdf_change_zoom_level(app);
        phmupdf_search_recreate_boxes(app);
        app->selection=0;

        tcb->doit=0;
        return Pt_CONTINUE;
    }

    /* Handle 100% width */
    if ((tcb->length==4) && (strcmp(tcb->text, "100%")==0))
    {
        PtContainerNullFocus(widget, NULL);
        if (app->phpagescroll)
        {
            PtGiveFocus(app->phpagescroll, NULL);
        }

        app->pdf_zoom=100;
        if (app->pdf_zoom>app->pdf_zoommax)
        {
            app->pdf_zoom=PHMUPDF_ZOOM_FIT_WIDTH;
        }
        phmupdf_change_zoom_level(app);
        phmupdf_search_recreate_boxes(app);
        app->selection=0;

        tcb->doit=0;
        return Pt_CONTINUE;
    }

    /* Handle 125% width */
    if ((tcb->length==4) && (strcmp(tcb->text, "125%")==0))
    {
        PtContainerNullFocus(widget, NULL);
        if (app->phpagescroll)
        {
            PtGiveFocus(app->phpagescroll, NULL);
        }

        app->pdf_zoom=125;
        if (app->pdf_zoom>app->pdf_zoommax)
        {
            app->pdf_zoom=PHMUPDF_ZOOM_FIT_WIDTH;
        }
        phmupdf_change_zoom_level(app);
        phmupdf_search_recreate_boxes(app);
        app->selection=0;

        tcb->doit=0;
        return Pt_CONTINUE;
    }

    /* Handle 150% width */
    if ((tcb->length==4) && (strcmp(tcb->text, "150%")==0))
    {
        PtContainerNullFocus(widget, NULL);
        if (app->phpagescroll)
        {
            PtGiveFocus(app->phpagescroll, NULL);
        }

        app->pdf_zoom=150;
        if (app->pdf_zoom>app->pdf_zoommax)
        {
            app->pdf_zoom=PHMUPDF_ZOOM_FIT_WIDTH;
        }
        phmupdf_change_zoom_level(app);
        phmupdf_search_recreate_boxes(app);
        app->selection=0;

        tcb->doit=0;
        return Pt_CONTINUE;
    }

    /* Handle 200% width */
    if ((tcb->length==4) && (strcmp(tcb->text, "200%")==0))
    {
        PtContainerNullFocus(widget, NULL);
        if (app->phpagescroll)
        {
            PtGiveFocus(app->phpagescroll, NULL);
        }

        app->pdf_zoom=200;
        if (app->pdf_zoom>app->pdf_zoommax)
        {
            app->pdf_zoom=PHMUPDF_ZOOM_FIT_WIDTH;
        }
        phmupdf_change_zoom_level(app);
        phmupdf_search_recreate_boxes(app);
        app->selection=0;

        tcb->doit=0;
        return Pt_CONTINUE;
    }

    /* Handle 300% width */
    if ((tcb->length==4) && (strcmp(tcb->text, "300%")==0))
    {
        PtContainerNullFocus(widget, NULL);
        if (app->phpagescroll)
        {
            PtGiveFocus(app->phpagescroll, NULL);
        }

        app->pdf_zoom=300;
        if (app->pdf_zoom>app->pdf_zoommax)
        {
            app->pdf_zoom=PHMUPDF_ZOOM_FIT_WIDTH;
        }
        phmupdf_change_zoom_level(app);
        phmupdf_search_recreate_boxes(app);
        app->selection=0;

        tcb->doit=0;
        return Pt_CONTINUE;
    }

    /* Handle 400% width */
    if ((tcb->length==4) && (strcmp(tcb->text, "400%")==0))
    {
        PtContainerNullFocus(widget, NULL);
        if (app->phpagescroll)
        {
            PtGiveFocus(app->phpagescroll, NULL);
        }

        app->pdf_zoom=400;
        if (app->pdf_zoom>app->pdf_zoommax)
        {
            app->pdf_zoom=PHMUPDF_ZOOM_FIT_WIDTH;
        }
        phmupdf_change_zoom_level(app);
        phmupdf_search_recreate_boxes(app);
        app->selection=0;

        tcb->doit=0;
        return Pt_CONTINUE;
    }

    /* Check if user tries to input correct data */
    if (tcb->length!=0)
    {
        for (it=0; it<tcb->length; it++)
        {
            if ((tcb->text[it]<'0') || (tcb->text[it]>'9'))
            {
                if (tcb->text[it]!='%')
                {
                    /* Do not allow such modifications to text */
                    tcb->doit=0;
                    return Pt_CONTINUE;
                }
            }
        }
    }

    /* Allow such modifications to text */
    tcb->doit=1;

    return Pt_CONTINUE;
}

int phmupdf_zoomselect_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t*   app;
    PtTextCallback_t* tcb=(PtTextCallback_t*)info->cbdata;
    char text[16];
    int  new_zoom;

    app=(photon_mupdf_t*)data;

    if (info->reason_subtype==Pt_EDIT_ACTIVATE)
    {
        if (sscanf(tcb->text, "%d%%", &new_zoom)==1)
        {
            if (new_zoom<app->pdf_zoommin)
            {
                new_zoom=app->pdf_zoommin;
                sprintf(text, "%d%%", app->pdf_zoommin);
                PtSetResource(widget, Pt_ARG_TEXT_STRING, text, 0);
            }
            else
            {
                if (new_zoom>app->pdf_zoommax)
                {
                    new_zoom=app->pdf_zoommax;
                    sprintf(text, "%d%%", app->pdf_zoommax);
                    PtSetResource(widget, Pt_ARG_TEXT_STRING, text, 0);
                }
                else
                {
                    sprintf(text, "%d%%", new_zoom);
                    PtSetResource(widget, Pt_ARG_TEXT_STRING, text, 0);
                }
            }
        }
        else
        {
            new_zoom=app->pdf_zoom;
            sprintf(text, "%d%%", app->pdf_zoom);
            PtSetResource(widget, Pt_ARG_TEXT_STRING, text, 0);
        }
        app->pdf_zoom=new_zoom;
        PtContainerNullFocus(widget, NULL);
        if (app->phpagescroll)
        {
            PtGiveFocus(app->phpagescroll, NULL);
        }

        phmupdf_change_zoom_level(app);
        phmupdf_search_recreate_boxes(app);
        app->selection=0;
    }

    return Pt_CONTINUE;
}

int phmupdf_zoomraw_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    char text[16];

    app=(photon_mupdf_t*)data;

    switch (info->event->type)
    {
        case Ph_EV_KEY:
             {
                  PhKeyEvent_t* keyevent=NULL;

                  keyevent=PhGetData(info->event);
                  if (keyevent==NULL)
                  {
                      break;
                  }

                  if ((keyevent->key_flags & Pk_KF_Key_Repeat)==Pk_KF_Key_Repeat)
                  {
                      break;
                  }

                  /* Check if key has been translated to the symbol */
                  if ((keyevent->key_flags & Pk_KF_Sym_Valid)==Pk_KF_Sym_Valid)
                  {
                      switch(keyevent->key_sym)
                      {
                          case Pk_Escape:
                               {
                                   switch(app->pdf_zoom)
                                   {
                                       case 10:
                                            PtSetResource(app->nbzoomlevel, Pt_ARG_CBOX_SEL_ITEM, 1, 0);
                                            break;
                                       case 25:
                                            PtSetResource(app->nbzoomlevel, Pt_ARG_CBOX_SEL_ITEM, 2, 0);
                                            break;
                                       case 50:
                                            PtSetResource(app->nbzoomlevel, Pt_ARG_CBOX_SEL_ITEM, 3, 0);
                                            break;
                                       case 75:
                                            PtSetResource(app->nbzoomlevel, Pt_ARG_CBOX_SEL_ITEM, 4, 0);
                                            break;
                                       case 100:
                                            PtSetResource(app->nbzoomlevel, Pt_ARG_CBOX_SEL_ITEM, 5, 0);
                                            break;
                                       case 125:
                                            PtSetResource(app->nbzoomlevel, Pt_ARG_CBOX_SEL_ITEM, 6, 0);
                                            break;
                                       case 150:
                                            PtSetResource(app->nbzoomlevel, Pt_ARG_CBOX_SEL_ITEM, 7, 0);
                                            break;
                                       case 200:
                                            PtSetResource(app->nbzoomlevel, Pt_ARG_CBOX_SEL_ITEM, 8, 0);
                                            break;
                                       case 300:
                                            PtSetResource(app->nbzoomlevel, Pt_ARG_CBOX_SEL_ITEM, 9, 0);
                                            break;
                                       case 400:
                                            PtSetResource(app->nbzoomlevel, Pt_ARG_CBOX_SEL_ITEM, 10, 0);
                                            break;
                                       default:
                                           switch(app->pdf_zoom)
                                           {
                                               case PHMUPDF_ZOOM_FIT_WIDTH:
                                                    sprintf(text, "%d%%", app->pdf_zoommax);
                                                    PtSetResource(widget, Pt_ARG_TEXT_STRING, text, 0);
                                                    PtSetResource(app->nbzoomlevel, Pt_ARG_CBOX_SEL_ITEM, 12, 0);
                                                    break;
                                               case PHMUPDF_ZOOM_FIT_PAGE:
                                                    sprintf(text, "%d%%", app->pdf_zoom);
                                                    PtSetResource(widget, Pt_ARG_TEXT_STRING, text, 0);
                                                    PtSetResource(app->nbzoomlevel, Pt_ARG_CBOX_SEL_ITEM, 11, 0);
                                                    break;
                                               default:
                                                    sprintf(text, "%d%%", app->pdf_zoom);
                                                    PtSetResource(widget, Pt_ARG_TEXT_STRING, text, 0);
                                                    PtSetResource(app->nbzoomlevel, Pt_ARG_CBOX_SEL_ITEM, 0, 0);
                                                    break;
                                           }
                                           break;
                                   }
                                   PtContainerNullFocus(widget, NULL);
                                   if (app->phpagescroll)
                                   {
                                       PtGiveFocus(app->phpagescroll, NULL);
                                   }
                                   return Pt_CONSUME;
                               }
                               break;
                      }
                  }
             }
             break;
    }

    return Pt_CONTINUE;
}

int phmupdf_zoomin_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    int old_zoom;

    app=(photon_mupdf_t*)data;
    old_zoom=app->pdf_zoom;

    if ((app->phpagescroll==NULL) || (app->pdf_pages==NULL))
    {
        return Pt_CONTINUE;
    }

    if (app->pdf_zoom==PHMUPDF_ZOOM_FIT_WIDTH)
    {
        app->pdf_zoom=app->pdf_zoommax;
    }

    if ((app->pdf_zoom>=app->pdf_zoommin) && (app->pdf_zoom<10))
    {
        app->pdf_zoom=10;
    }
    else
    {
        if ((app->pdf_zoom>=app->pdf_zoommin) && (app->pdf_zoom<25))
        {
            app->pdf_zoom=25;
        }
        else
        {
            if ((app->pdf_zoom>=25) && (app->pdf_zoom<50))
            {
                app->pdf_zoom=50;
            }
            else
            {
                if ((app->pdf_zoom>=50) && (app->pdf_zoom<75))
                {
                    app->pdf_zoom=75;
                }
                else
                {
                    if ((app->pdf_zoom>=75) && (app->pdf_zoom<100))
                    {
                        app->pdf_zoom=100;
                    }
                    else
                    {
                        if ((app->pdf_zoom>=100) && (app->pdf_zoom<125))
                        {
                            app->pdf_zoom=125;
                        }
                        else
                        {
                            if ((app->pdf_zoom>=125) && (app->pdf_zoom<150))
                            {
                                app->pdf_zoom=150;
                            }
                            else
                            {
                                if ((app->pdf_zoom>=150) && (app->pdf_zoom<200))
                                {
                                    app->pdf_zoom=200;
                                }
                                else
                                {
                                    if ((app->pdf_zoom>=200) && (app->pdf_zoom<300))
                                    {
                                        app->pdf_zoom=300;
                                    }
                                    else
                                    {
                                        if ((app->pdf_zoom>=300) && (app->pdf_zoom<400))
                                        {
                                            app->pdf_zoom=400;
                                        }
                                        else
                                        {
                                            if ((app->pdf_zoom>=400) && (app->pdf_zoom<app->pdf_zoommax))
                                            {
                                                app->pdf_zoom=app->pdf_zoommax;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (app->pdf_zoom>app->pdf_zoommax)
    {
        app->pdf_zoom=app->pdf_zoommax;
    }

    if (old_zoom!=app->pdf_zoom)
    {
        phmupdf_change_zoom_level(app);
        phmupdf_search_recreate_boxes(app);
        app->selection=0;
    }

    return Pt_CONTINUE;
}

int phmupdf_zoomout_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    int old_zoom;

    app=(photon_mupdf_t*)data;
    old_zoom=app->pdf_zoom;

    if ((app->phpagescroll==NULL) || (app->pdf_pages==NULL))
    {
        return Pt_CONTINUE;
    }

    if (app->pdf_zoom==PHMUPDF_ZOOM_FIT_WIDTH)
    {
        app->pdf_zoom=app->pdf_zoommax;
    }

    if ((app->pdf_zoom>app->pdf_zoommin) && (app->pdf_zoom<=10))
    {
        app->pdf_zoom=app->pdf_zoommin;
    }
    else
    {
        if ((app->pdf_zoom>10) && (app->pdf_zoom<=25))
        {
            app->pdf_zoom=10;
        }
        else
        {
            if ((app->pdf_zoom>25) && (app->pdf_zoom<=50))
            {
                app->pdf_zoom=25;
            }
            else
            {
                if ((app->pdf_zoom>50) && (app->pdf_zoom<=75))
                {
                    app->pdf_zoom=50;
                }
                else
                {
                    if ((app->pdf_zoom>75) && (app->pdf_zoom<=100))
                    {
                        app->pdf_zoom=75;
                    }
                    else
                    {
                        if ((app->pdf_zoom>100) && (app->pdf_zoom<=125))
                        {
                            app->pdf_zoom=100;
                        }
                        else
                        {
                            if ((app->pdf_zoom>125) && (app->pdf_zoom<=150))
                            {
                                app->pdf_zoom=125;
                            }
                            else
                            {
                                if ((app->pdf_zoom>150) && (app->pdf_zoom<=200))
                                {
                                    app->pdf_zoom=150;
                                }
                                else
                                {
                                    if ((app->pdf_zoom>200) && (app->pdf_zoom<=300))
                                    {
                                        app->pdf_zoom=200;
                                    }
                                    else
                                    {
                                        if ((app->pdf_zoom>300) && (app->pdf_zoom<=400))
                                        {
                                            app->pdf_zoom=300;
                                        }
                                        else
                                        {
                                            if (app->pdf_zoom>400)
                                            {
                                                app->pdf_zoom=400;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (app->pdf_zoom<app->pdf_zoommin)
    {
        app->pdf_zoom=app->pdf_zoommin;
    }

    if (old_zoom!=app->pdf_zoom)
    {
        phmupdf_change_zoom_level(app);
        phmupdf_search_recreate_boxes(app);
        app->selection=0;
    }

    return Pt_CONTINUE;
}

int phmupdf_rotation_callback_dialog(int button, void* data)
{
    photon_mupdf_t* app;
    double*         data_ptr;
    int             it;

    app=(photon_mupdf_t*)data;

    if (button==1)
    {
        /* Ok button */
        PtGetResource(app->rotation_angle, Pt_ARG_NUMERIC_VALUE, &data_ptr, 0);
        if (app->rotation!=(float)*data_ptr)
        {
            app->rotation=(float)*data_ptr;
            app->pdf_zoommax=INT_MAX;

            /* Re-calculate matrices and zoom levels */
            for (it=0; it<app->pdf_pagecount; it++)
            {
                fz_matrix temp_ctm;

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
            }
            app->pdf_zoom=PHMUPDF_ZOOM_FIT_WIDTH;
            phmupdf_change_zoom_level(app);
            phmupdf_search_recreate_boxes(app);
            app->selection=0;
        }
    }

    /* Cancel button */
    PtUnblockWindows(app->rotation_bl);
    PtDestroyWidget(app->rotation_dialog);
    app->rotation_dialog=NULL;
    app->rotation_angle=NULL;

    return Pt_CONTINUE;
}

int phmupdf_rotation_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    PtDialogInfo_t  dialog_info;
    PtWidget_t*     dialog_pane;
    PtArg_t         winargs[32];
    uint32_t        winargc=0;
    PhPoint_t       temp_point;
    PhDim_t         temp_dim;
    double          min;
    double          max;
    double          value;
    static char const* buttons[2]={"Ok", "Cancel"};
    const char*     label="Enter rotation angle:";

    app=(photon_mupdf_t*)data;

    if ((app->phpagescroll==NULL) || (app->pdf_pages==NULL))
    {
        return Pt_CONTINUE;
    }

    /* Create the PtPane widget */
    winargc=0;
    temp_dim.h=32;
    temp_dim.w=250;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    dialog_pane=PtCreateWidget(PtPane, app->phwindow, winargc, winargs);
    if (dialog_pane==NULL)
    {
        return Pt_CONTINUE;
    }

    /* Create the PtNumericFloat widget */
    winargc=0;
    temp_point.x=140;
    temp_point.y=4;
    temp_dim.h=28;
    temp_dim.w=56;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    min=-360.0f;
    PtSetArg(&winargs[winargc++], Pt_ARG_NUMERIC_MIN, &min, 0);
    max=360.0f;
    PtSetArg(&winargs[winargc++], Pt_ARG_NUMERIC_MAX, &max, 0);
    value=app->rotation;
    PtSetArg(&winargs[winargc++], Pt_ARG_NUMERIC_VALUE, &value, 0);
    app->rotation_angle=PtCreateWidget(PtNumericFloat, dialog_pane, winargc, winargs);
    if (app->rotation_angle==NULL)
    {
        return Pt_CONTINUE;
    }

    /* Create text label */
    winargc=0;
    temp_point.x=0;
    temp_point.y=4;
    temp_dim.h=28;
    temp_dim.w=132;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, label, 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);

    dialog_info.parent=app->phwindow;
    dialog_info.pane=dialog_pane;
    dialog_info.title="Rotation angle in degrees";
    dialog_info.buttons=buttons;
    dialog_info.btn_fonts=NULL;
    dialog_info.nbtns=2;
    dialog_info.def_btn=1;
    dialog_info.esc_btn=2;
    dialog_info.width=250;
    dialog_info.callback=phmupdf_rotation_callback_dialog;
    dialog_info.data=(void*)app;
    app->rotation_dialog=PtCreateDialog(&dialog_info);

    /* Put focus to input widget */
    PtGiveFocus(app->rotation_angle, NULL);

    /* Show dialog and block parent */
    app->rotation_bl=PtBlockWindow(app->phwindow, Ph_CURSOR_NOINPUT,
        Ph_CURSOR_DEFAULT_COLOR);

    /* Center window */
    temp_point.x=app->window.position.x+(app->window.dimension.w-250)/2;
    temp_point.y=app->window.position.y+(app->window.dimension.h-32)/2;
    PtSetResource(app->rotation_dialog, Pt_ARG_POS, &temp_point, 0);

    PtRealizeWidget(app->rotation_dialog);

    return Pt_CONTINUE;
}

int phmupdf_properties_callback_dialog(int button, void* data)
{
    photon_mupdf_t* app;

    app=(photon_mupdf_t*)data;

    PtUnblockWindows(app->properties_bl);
    PtDestroyWidget(app->properties_dialog);
    app->properties_dialog=NULL;

    return Pt_CONTINUE;
}

char* parse_pdf_date(char* date)
{
    char temp[256]={0};
    int  tempptr=0;

    if ((strlen(date)<2)||(date[0]!='D')||(date[1]!=':'))
    {
        return date;
    }
    if (strlen(date)>=10)
    {
        temp[tempptr++]=date[8];
        if (temp[tempptr-1]=='0')
        {
            temp[tempptr-1]=date[9];
        }
        else
        {
            temp[tempptr++]=date[9];
        }
        if (temp[tempptr-1]=='1')
        {
            temp[tempptr++]='s';
            temp[tempptr++]='t';
        }
        else
        {
            if (temp[tempptr-1]=='2')
            {
                temp[tempptr++]='n';
                temp[tempptr++]='d';
            }
            else
            {
                if (temp[tempptr-1]=='3')
                {
                    temp[tempptr++]='r';
                    temp[tempptr++]='d';
                }
                else
                {
                    temp[tempptr++]='t';
                    temp[tempptr++]='h';
                }
            }
        }

        temp[tempptr++]=' ';
        temp[tempptr]=0x00;

        switch((date[6]-'0')*10+date[7]-'0')
        {
            case 1:
                 strcat(temp, "January");
                 break;
            case 2:
                 strcat(temp, "February");
                 break;
            case 3:
                 strcat(temp, "March");
                 break;
            case 4:
                 strcat(temp, "April");
                 break;
            case 5:
                 strcat(temp, "May");
                 break;
            case 6:
                 strcat(temp, "June");
                 break;
            case 7:
                 strcat(temp, "Jule");
                 break;
            case 8:
                 strcat(temp, "August");
                 break;
            case 9:
                 strcat(temp, "September");
                 break;
            case 10:
                 strcat(temp, "October");
                 break;
            case 11:
                 strcat(temp, "November");
                 break;
            case 12:
                 strcat(temp, "December");
                 break;
            default:
                 return date;
        }
        tempptr=strlen(temp);
        temp[tempptr++]=' ';
        temp[tempptr++]=date[2];
        temp[tempptr++]=date[3];
        temp[tempptr++]=date[4];
        temp[tempptr++]=date[5];
        temp[tempptr++]=0x00;
    }

    if (strlen(date)>=16)
    {
        tempptr=strlen(temp);
        temp[tempptr++]=',';
        temp[tempptr++]=' ';
        temp[tempptr++]=date[10];
        temp[tempptr++]=date[11];
        temp[tempptr++]=':';
        temp[tempptr++]=date[12];
        temp[tempptr++]=date[13];
        temp[tempptr++]=':';
        temp[tempptr++]=date[14];
        temp[tempptr++]=date[15];
        temp[tempptr++]=0x00;
    }

    if (strlen(date)>=17)
    {
        if (date[16]=='Z')
        {
            tempptr=strlen(temp);
            temp[tempptr++]=' ';
            temp[tempptr++]='U';
            temp[tempptr++]='T';
            temp[tempptr++]='C';
            temp[tempptr++]=0x00;
        }
    }

    if (strlen(date)>=19)
    {
        tempptr=strlen(temp);
        temp[tempptr++]=' ';
        temp[tempptr++]='U';
        temp[tempptr++]='T';
        temp[tempptr++]='C';

        if (date[16]=='+')
        {
            temp[tempptr++]='+';
        }
        if (date[16]=='-')
        {
            temp[tempptr++]='-';
        }
        temp[tempptr++]=date[17];
        temp[tempptr++]=date[18];
        temp[tempptr++]=0x00;
    }

    if (strlen(date)>=23)
    {
        if ((date[19]=='\'')&&(date[22]=='\''))
        {
            tempptr=strlen(temp);
            temp[tempptr++]=':';
            temp[tempptr++]=date[20];
            temp[tempptr++]=date[21];
            temp[tempptr++]=0x00;
        }
    }

    strcpy(date, temp);

    return date;
}

int phmupdf_properties_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    PtDialogInfo_t  dialog_info;
    PtWidget_t*     dialog_pane;
    char            metabuf[256];
    char**          metabuf_ptr;
    PtArg_t         winargs[32];
    uint32_t        winargc=0;
    PhPoint_t       temp_point;
    PhDim_t         temp_dim;
    static char const* buttons[1]={"Close"};

    app=(photon_mupdf_t*)data;
    metabuf_ptr=(char**)metabuf;

    if ((app->phpagescroll==NULL) || (app->pdf_pages==NULL))
    {
        return Pt_CONTINUE;
    }

    /* Create the PtPane widget */
    winargc=0;
    temp_dim.h=284;
    temp_dim.w=600;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    dialog_pane=PtCreateWidget(PtPane, app->phwindow, winargc, winargs);
    if (dialog_pane==NULL)
    {
        return Pt_CONTINUE;
    }

    /* Create labels */
    winargc=0;
    temp_point.x=0;
    temp_point.y=0;
    temp_dim.h=16;
    temp_dim.w=100;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "File format:", 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Encryption:", 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Permission:", 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=33;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Title:", 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Author:", 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Subject:", 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Keywords:", 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Creator:", 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Producer:", 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Creation:", 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Modification:", 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);

    /* Create labels with data */
    winargc=0;
    temp_point.x=100;
    temp_point.y=0;
    temp_dim.h=16;
    temp_dim.w=600-100;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    if (fz_meta(app->pdfdoc, FZ_META_FORMAT_INFO, metabuf, 256)<0)
    {
        strcpy(metabuf, "unknown");
    }
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, metabuf, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xF0, 0xF0, 0xF0), 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);

    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    if (fz_meta(app->pdfdoc, FZ_META_CRYPT_INFO, metabuf, 256)!=0)
    {
        strcpy(metabuf, "none");
    }
    if (strcmp(metabuf, "None")==0)
    {
        strcpy(metabuf, "none");
    }
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, metabuf, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xF0, 0xF0, 0xF0), 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);

    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    metabuf[0] = 0;
    if (fz_meta(app->pdfdoc, FZ_META_HAS_PERMISSION, NULL, FZ_PERMISSION_PRINT)==0)
    {
        strcat(metabuf, "print, ");
    }
    if (fz_meta(app->pdfdoc, FZ_META_HAS_PERMISSION, NULL, FZ_PERMISSION_CHANGE)==0)
    {
        strcat(metabuf, "modify, ");
    }
    if (fz_meta(app->pdfdoc, FZ_META_HAS_PERMISSION, NULL, FZ_PERMISSION_COPY)==0)
    {
        strcat(metabuf, "copy, ");
    }
    if (fz_meta(app->pdfdoc, FZ_META_HAS_PERMISSION, NULL, FZ_PERMISSION_NOTES)==0)
    {
        strcat(metabuf, "annotate, ");
    }
    if (strlen(metabuf)>2)
    {
        metabuf[strlen(metabuf)-2]=0x00;
    }
    else
    {
        strcpy(metabuf, "none");
    }
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, metabuf, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xF0, 0xF0, 0xF0), 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=33;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    metabuf[0]=0;
    *metabuf_ptr=(char*)"Title";
    if (fz_meta(app->pdfdoc, FZ_META_INFO, metabuf, 256)<=0)
    {
        strcpy(metabuf, "untitled");
    }
    if (strlen(metabuf)==0)
    {
        strcpy(metabuf, "n/a");
    }
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, metabuf, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xF0, 0xF0, 0xF0), 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    metabuf[0]=0;
    *metabuf_ptr=(char*)"Author";
    if (fz_meta(app->pdfdoc, FZ_META_INFO, metabuf, 256)<=0)
    {
        strcpy(metabuf, "n/a");
    }
    if (strlen(metabuf)==0)
    {
        strcpy(metabuf, "n/a");
    }
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, metabuf, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xF0, 0xF0, 0xF0), 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    metabuf[0]=0;
    *metabuf_ptr=(char*)"Subject";
    if (fz_meta(app->pdfdoc, FZ_META_INFO, metabuf, 256)<=0)
    {
        strcpy(metabuf, "n/a");
    }
    if (strlen(metabuf)==0)
    {
        strcpy(metabuf, "n/a");
    }
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, metabuf, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xF0, 0xF0, 0xF0), 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    metabuf[0]=0;
    *metabuf_ptr=(char*)"Keywords";
    if (fz_meta(app->pdfdoc, FZ_META_INFO, metabuf, 256)<=0)
    {
        strcpy(metabuf, "n/a");
    }
    if (strlen(metabuf)==0)
    {
        strcpy(metabuf, "n/a");
    }
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, metabuf, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xF0, 0xF0, 0xF0), 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    metabuf[0]=0;
    *metabuf_ptr=(char*)"Creator";
    if (fz_meta(app->pdfdoc, FZ_META_INFO, metabuf, 256)<=0)
    {
        strcpy(metabuf, "n/a");
    }
    if (strlen(metabuf)==0)
    {
        strcpy(metabuf, "n/a");
    }
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, metabuf, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xF0, 0xF0, 0xF0), 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    metabuf[0]=0;
    *metabuf_ptr=(char*)"Producer";
    if (fz_meta(app->pdfdoc, FZ_META_INFO, metabuf, 256)<=0)
    {
        strcpy(metabuf, "n/a");
    }
    if (strlen(metabuf)==0)
    {
        strcpy(metabuf, "n/a");
    }
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, metabuf, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xF0, 0xF0, 0xF0), 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    metabuf[0]=0;
    *metabuf_ptr=(char*)"CreationDate";
    if (fz_meta(app->pdfdoc, FZ_META_INFO, metabuf, 256)<=0)
    {
        strcpy(metabuf, "n/a");
    }
    if (strlen(metabuf)==0)
    {
        strcpy(metabuf, "n/a");
    }
    parse_pdf_date(metabuf);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, metabuf, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xF0, 0xF0, 0xF0), 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);
    winargc=0;
    temp_point.y+=23;
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    metabuf[0]=0;
    *metabuf_ptr=(char*)"ModDate";
    if (fz_meta(app->pdfdoc, FZ_META_INFO, metabuf, 256)<=0)
    {
        strcpy(metabuf, "n/a");
    }
    if (strlen(metabuf)==0)
    {
        strcpy(metabuf, "n/a");
    }
    parse_pdf_date(metabuf);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, metabuf, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xF0, 0xF0, 0xF0), 0);
    PtCreateWidget(PtLabel, dialog_pane, winargc, winargs);

    dialog_info.parent=app->phwindow;
    dialog_info.pane=dialog_pane;
    dialog_info.title="Document properties";
    dialog_info.buttons=buttons;
    dialog_info.btn_fonts=NULL;
    dialog_info.nbtns=1;
    dialog_info.def_btn=1;
    dialog_info.esc_btn=1;
    dialog_info.width=600;
    dialog_info.callback=phmupdf_properties_callback_dialog;
    dialog_info.data=(void*)app;
    app->properties_dialog=PtCreateDialog(&dialog_info);

    /* Show dialog and block parent */
    app->properties_bl=PtBlockWindow(app->phwindow, Ph_CURSOR_NOINPUT,
        Ph_CURSOR_DEFAULT_COLOR);

    /* Center window */
    temp_point.x=app->window.position.x+(app->window.dimension.w-600)/2;
    temp_point.y=app->window.position.y+(app->window.dimension.h-284)/2;
    PtSetResource(app->properties_dialog, Pt_ARG_POS, &temp_point, 0);

    /* Reset all resize flags */
    PtSetResource(dialog_pane, Pt_ARG_RESIZE_FLAGS, Pt_FALSE, -1);

    PtRealizeWidget(app->properties_dialog);

    return Pt_CONTINUE;
}

int phmupdf_scrollbar_move_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t*        app;
    PtScrollbarCallback_t* cbdata=NULL;
    int                    it;
    unsigned int           offset;

    app=(photon_mupdf_t*)data;

    if (info->reason==Pt_CB_SCROLL_MOVE)
    {
        cbdata=(PtScrollbarCallback_t*)info->cbdata;
        switch(cbdata->action)
        {
            case Pt_SCROLL_DECREMENT:
                 if (cbdata->position>=PHMUPDF_GAP_BETWEEN_PAGES)
                 {
                     cbdata->position-=PHMUPDF_GAP_BETWEEN_PAGES;
                 }
                 else
                 {
                     cbdata->position=0;
                 }
                 PtSetResource(widget, Pt_ARG_GAUGE_VALUE, cbdata->position, 0);
                 break;
            case Pt_SCROLL_INCREMENT:
                 cbdata->position+=PHMUPDF_GAP_BETWEEN_PAGES;
                 if (cbdata->position>=app->pdf_totallines)
                 {
                     cbdata->position=app->pdf_totallines-1;
                 }
                 PtSetResource(widget, Pt_ARG_GAUGE_VALUE, cbdata->position, 0);
                 break;
            case Pt_SCROLL_PAGE_INCREMENT:
            case Pt_SCROLL_PAGE_DECREMENT:
            case Pt_SCROLL_TO_MAX:
            case Pt_SCROLL_TO_MIN:
            case Pt_SCROLL_RELEASED:
            case Pt_SCROLL_DRAGGED:
            case Pt_SCROLL_SET:
            case Pt_SCROLL_JUMP:
                 break;
            default:
                 return Pt_CONTINUE;
        }
    }

    /* Update current page widget */
    offset=0;
    for (it=0; it<app->pdf_pagecount; it++)
    {
        if ((cbdata->position>=offset) &&
            (cbdata->position<offset+(app->pdf_pages[it].page_bbox_draw.y1-app->pdf_pages[it].page_bbox_draw.y0+1)+PHMUPDF_GAP_BETWEEN_PAGES))
        {
            char text[16];

            sprintf(text, "%d", it+1);
            PtSetResource(app->nbpage, Pt_ARG_TEXT_STRING, text, 0);
            app->pdf_currpage=it+1;
        }
        offset+=(app->pdf_pages[it].page_bbox_draw.y1-app->pdf_pages[it].page_bbox_draw.y0+1)+PHMUPDF_GAP_BETWEEN_PAGES;
    }

    /* Update cursor in case if it points to link or text */
    if ((app->last_x!=-1) && (app->last_y!=-1))
    {
        phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
        phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
    }

    /* Update visible information */
    PtDamageWidget(app->phcontent);

    return Pt_CONTINUE;
}

int phmupdf_scrollbar_callback_raw(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    unsigned int    position=-1;

    app=(photon_mupdf_t*)data;

    switch (info->event->type)
    {
         case Ph_EV_KEY:
              {
                  PhKeyEvent_t* keyevent=NULL;

                  keyevent=PhGetData(info->event);
                  if (keyevent==NULL)
                  {
                      break;
                  }

                  if ((keyevent->key_flags & Pk_KF_Key_Repeat)==Pk_KF_Key_Repeat)
                  {
                      break;
                  }

                  /* Check if key has been translated to the symbol */
                  if ((keyevent->key_flags & Pk_KF_Sym_Valid)==Pk_KF_Sym_Valid)
                  {
                      switch(keyevent->key_sym)
                      {
                          case Pk_BackSpace:
                               {
                                   unsigned int* scroll_position;

                                   PtGetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, &scroll_position, 0);

                                   if (app->jump_stack_ptr!=-1)
                                   {
                                       position=app->jump_stack[app->jump_stack_ptr-1];
                                       if (app->jump_stack_ptr==1)
                                       {
                                           app->jump_stack_ptr=-1;
                                       }
                                       else
                                       {
                                           app->jump_stack_ptr--;
                                       }
                                   }
                               }
                               break;
                          case Pk_Left:
                               {
                                   if (keyevent->key_mods==0)
                                   {
                                       phmupdf_previous_callback(widget, data, info);
                                       return Pt_CONSUME;
                                   }
                                   if ((keyevent->key_mods==Pk_KM_Alt) || (keyevent->key_mods==Pk_KM_AltGr))
                                   {
                                       unsigned int* scroll_position;

                                       PtGetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, &scroll_position, 0);

                                       if (app->jump_stack_ptr!=-1)
                                       {
                                           position=app->jump_stack[app->jump_stack_ptr-1];
                                           if (app->jump_stack_ptr==1)
                                           {
                                               app->jump_stack_ptr=-1;
                                           }
                                           else
                                           {
                                               app->jump_stack_ptr--;
                                           }
                                       }
                                   }
                               }
                               break;
                          case Pk_Right:
                               {
                                   if (keyevent->key_mods==0)
                                   {
                                       phmupdf_next_callback(widget, data, info);
                                       return Pt_CONSUME;
                                   }
                                   if ((keyevent->key_mods==Pk_KM_Alt) || (keyevent->key_mods==Pk_KM_AltGr))
                                   {
                                       if (app->jump_stack_ptr==-1)
                                       {
                                           if (app->jump_stack_last_valid!=-1)
                                           {
                                               position=app->jump_stack[0];
                                               app->jump_stack_ptr=1;
                                           }
                                       }
                                       else
                                       {
                                           if (app->jump_stack_ptr-1<app->jump_stack_last_valid)
                                           {
                                               position=app->jump_stack[app->jump_stack_ptr];
                                               app->jump_stack_ptr++;
                                           }
                                       }
                                   }
                               }
                               break;
                      }
                  }
              }
              break;
    }

    if (position!=-1)
    {
        unsigned int offset;
        int          it;

        PtSetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, position, 0);
        PtDamageWidget(app->phcontent);

        /* Update cursor in case if it points to link or text */
        if ((app->last_x!=-1) && (app->last_y!=-1))
        {
            phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
            phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
        }

        PtFlush();

        /* Convert position to page */
        offset=0;
        for (it=0; it<app->pdf_pagecount; it++)
        {
            if ((position>=offset) &&
                (position<offset+(app->pdf_pages[it].page_bbox_draw.y1-app->pdf_pages[it].page_bbox_draw.y0+1)+PHMUPDF_GAP_BETWEEN_PAGES))
            {
                char text[16];

                sprintf(text, "%d", it+1);
                PtSetResource(app->nbpage, Pt_ARG_TEXT_STRING, text, 0);
                app->pdf_currpage=it+1;
            }
            offset+=(app->pdf_pages[it].page_bbox_draw.y1-app->pdf_pages[it].page_bbox_draw.y0+1)+PHMUPDF_GAP_BETWEEN_PAGES;
        }

        return Pt_CONSUME;
    }

    return Pt_CONTINUE;
}

int phmupdf_previous_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    char            text[16];

    app=(photon_mupdf_t*)data;

    if ((app->phpagescroll==NULL) || (app->pdf_pages==NULL))
    {
        return Pt_CONTINUE;
    }

    app->pdf_currpage--;
    if (app->pdf_currpage<=0)
    {
        app->pdf_currpage=1;
    }

    sprintf(text, "%d", app->pdf_currpage);
    PtSetResource(app->nbpage, Pt_ARG_TEXT_STRING, text, 0);

    /* Adjust scrollbar settings */
    if (app->phpagescroll)
    {
        int it;
        unsigned int position;

        PtGiveFocus(app->phpagescroll, NULL);

        /* Adjust scrollbar to show required page */
        position=0;
        for (it=0; it<app->pdf_currpage-1; it++)
        {
            position+=(app->pdf_pages[it].page_bbox_draw.y1-app->pdf_pages[it].page_bbox_draw.y0)+1;
            position+=PHMUPDF_GAP_BETWEEN_PAGES;
        }
        PtSetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, position, 0);

        /* Update cursor in case if it points to link or text */
        if ((app->last_x!=-1) && (app->last_y!=-1))
        {
            phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
            phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
        }
    }

    /* Update cursor in case if it points to link or text */
    if ((app->last_x!=-1) && (app->last_y!=-1))
    {
        phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
        phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
    }

    /* Update visible information */
    PtDamageWidget(app->phcontent);

    return Pt_CONTINUE;
}

int phmupdf_next_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    char            text[16];

    app=(photon_mupdf_t*)data;

    if ((app->phpagescroll==NULL) || (app->pdf_pages==NULL))
    {
        return Pt_CONTINUE;
    }

    app->pdf_currpage++;
    if (app->pdf_currpage>=app->pdf_pagecount)
    {
        app->pdf_currpage=app->pdf_pagecount;
    }

    sprintf(text, "%d", app->pdf_currpage);
    PtSetResource(app->nbpage, Pt_ARG_TEXT_STRING, text, 0);

    /* Adjust scrollbar settings */
    if (app->phpagescroll)
    {
        int          it;
        unsigned int position;

        PtGiveFocus(app->phpagescroll, NULL);

        /* Adjust scrollbar to show required page */
        position=0;
        for (it=0; it<app->pdf_currpage-1; it++)
        {
            position+=(app->pdf_pages[it].page_bbox_draw.y1-app->pdf_pages[it].page_bbox_draw.y0)+1;
            position+=PHMUPDF_GAP_BETWEEN_PAGES;
        }
        PtSetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, position, 0);

        /* Update cursor in case if it points to link or text */
        if ((app->last_x!=-1) && (app->last_y!=-1))
        {
            phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
            phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
        }
    }

    /* Update cursor in case if it points to link or text */
    if ((app->last_x!=-1) && (app->last_y!=-1))
    {
        phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
        phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
    }

    /* Update visible information */
    PtDamageWidget(app->phcontent);

    return Pt_CONTINUE;
}

int phmupdf_search_text_length(fz_text_page* page)
{
    fz_text_block* block;
    fz_text_line* line;
    fz_text_span* span;
    int len=0;
    int blocks;

    for (blocks=0; blocks<page->len; blocks++)
    {
        if (page->blocks[blocks].type!=FZ_PAGE_BLOCK_TEXT)
        {
            continue;
        }
        block=page->blocks[blocks].u.text;
        for (line=block->lines; line<block->lines+block->len; line++)
        {
            for (span=line->first_span; span; span=span->next)
            {
                len+=span->len;
            }

            len++;
        }
    }

    return len;
}

fz_char_and_box* phmupdf_search_uni_charat(fz_char_and_box* cab, fz_text_page* page, int idx)
{
    return fz_text_char_at(cab, page, idx);
}

fz_irect phmupdf_search_bbox_charat(fz_text_page* page, int idx)
{
    fz_irect temp_irect;
    fz_char_and_box cab;

    fz_irect_from_rect(&temp_irect, &phmupdf_search_uni_charat(&cab, page, idx)->bbox);

    return temp_irect;
}

int phmupdf_search_text_charat(fz_text_page* page, int idx)
{
    fz_char_and_box cab;

    return phmupdf_search_uni_charat(&cab, page, idx)->c;
}

int phmupdf_search_text_match(photon_mupdf_t* app, wchar_t* s, fz_text_page* page, int n)
{
    int orig=n;
    int c;

    while ((c=*s++))
    {
        if (((c==' ')||(c=='_')||(c=='-')||(c==',')||(c=='.')) &&
            ((phmupdf_search_text_charat(page, n)==' ')||(phmupdf_search_text_charat(page, n)=='_')||
             (phmupdf_search_text_charat(page, n)=='-')||(phmupdf_search_text_charat(page, n)=='.')||
             (phmupdf_search_text_charat(page, n)==',')))
        {
            while((phmupdf_search_text_charat(page, n)==' ')||(phmupdf_search_text_charat(page, n)=='_')||
                  (phmupdf_search_text_charat(page, n)=='-')||(phmupdf_search_text_charat(page, n)=='.')||
                  (phmupdf_search_text_charat(page, n)==','))
            {
                n++;
            }
        }
        else
        {
            if (!app->case_search)
            {
                if (wctolower((wchar_t)c)!=wctolower((wchar_t)phmupdf_search_text_charat(page, n)))
                {
                    return 0;
                }
            }
            else
            {
                if (c!=phmupdf_search_text_charat(page, n))
                {
                    return 0;
                }
            }
            n++;
        }
    }

    return n-orig;
}

void phmupdf_search_recreate_boxes(photon_mupdf_t* app)
{
    fz_device* pdf_device=NULL;
    fz_irect   hitbox;
    fz_irect   bbox;
    int        jt;

    /* Abort execution if there is no highlighted text */
    if ((app->display_found==0) || (app->search_hit==-1) ||
        (app->search_hit_length==-1))
    {
        /* Update content */
        PtDamageWidget(app->phcontent);
        PtFlush();
        return;
    }

    /* Check if page was not loaded */
    if (app->pdf_pages[app->display_page].page==NULL)
    {
        app->pdf_pages[app->display_page].page=fz_load_page(app->pdfdoc, app->display_page);
    }
    /* Check if page list was not loaded */
    if (app->pdf_pages[app->display_page].page_list==NULL)
    {
        fz_cookie cookie={0};

        cookie.incomplete_ok=1;
        app->pdf_pages[app->display_page].page_list=fz_new_display_list(app->pdfctx);
        pdf_device=fz_new_list_device(app->pdfctx, app->pdf_pages[app->display_page].page_list);
        fz_run_page(app->pdfdoc, app->pdf_pages[app->display_page].page, pdf_device, &fz_identity, &cookie);
        fz_free_device(pdf_device);
        pdf_device=NULL;
    }
    /* Check if text was not loaded */
    if (app->pdf_pages[app->display_page].page_text==NULL)
    {
        app->pdf_pages[app->display_page].page_sheet=fz_new_text_sheet(app->pdfctx);
        app->pdf_pages[app->display_page].page_text=fz_new_text_page(app->pdfctx);
        pdf_device=fz_new_text_device(app->pdfctx, app->pdf_pages[app->display_page].page_sheet, app->pdf_pages[app->display_page].page_text);
        fz_run_display_list(app->pdf_pages[app->display_page].page_list, pdf_device, &fz_identity, &fz_infinite_rect, NULL);
        fz_free_device(pdf_device);
    }

    /* Determine text boxes to highlight */
    hitbox=fz_empty_irect;
    app->display_boxes=0;

    for (jt=app->search_hit; jt<app->search_hit+app->search_hit_length; jt++)
    {
        bbox=phmupdf_search_bbox_charat(app->pdf_pages[app->display_page].page_text, jt);
        if (fz_is_empty_irect(&bbox))
        {
            if (!fz_is_empty_irect(&hitbox))
            {
                fz_rect temp_rect;

                fz_round_rect(&app->display_box[app->display_boxes++], 
                    fz_transform_rect(fz_rect_from_irect(&temp_rect, &hitbox),
                    &app->pdf_pages[app->display_page].page_ctm));
            }
            hitbox=fz_empty_irect;
        }
        else
        {
            fz_rect temp_rect;
            fz_rect temp_rect2;

            fz_round_rect(&hitbox, fz_union_rect(fz_rect_from_irect(&temp_rect2, &hitbox),
                                   fz_rect_from_irect(&temp_rect, &bbox)));
        }
    }

    if (!fz_is_empty_irect(&hitbox))
    {
        fz_rect temp_rect;

        fz_round_rect(&app->display_box[app->display_boxes++],
            fz_transform_rect(fz_rect_from_irect(&temp_rect, &hitbox),
            &app->pdf_pages[app->display_page].page_ctm));
    }

    /* Release page data if it is requested by user */
    if ((app->settings.fast_render==0) || (app->settings.fast_search==0))
    {
        if (app->pdf_pages[app->display_page].page_text)
        {
            fz_free_text_page(app->pdfctx, app->pdf_pages[app->display_page].page_text);
            app->pdf_pages[app->display_page].page_text=NULL;
        }
        if (app->pdf_pages[app->display_page].page_sheet)
        {
            fz_free_text_sheet(app->pdfctx, app->pdf_pages[app->display_page].page_sheet);
            app->pdf_pages[app->display_page].page_sheet=NULL;
        }
        if ((app->pdf_pages[app->display_page].page_list)&&(app->settings.fast_render==0))
        {
            fz_drop_display_list(app->pdfctx, app->pdf_pages[app->display_page].page_list);
            app->pdf_pages[app->display_page].page_list=NULL;
        }
        if ((app->pdf_pages[app->display_page].page)&&(app->settings.fast_render==0))
        {
            fz_free_page(app->pdfdoc, app->pdf_pages[app->display_page].page);
            app->pdf_pages[app->display_page].page=NULL;
        }
    }

    /* Update selection */
    PtDamageWidget(app->phcontent);
    PtFlush();
}

int phmupdf_search_forward_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    char*           textptr;
    char*           textptr_utf8;
    int             it;
    int             jt;
    fz_device*      pdf_device=NULL;
    char            text[16];
    wchar_t         wc_text[PHMUPDF_SEARCH_MAX_LENGTH];
    int             wc_text_length=0;

    app=(photon_mupdf_t*)data;

    /* Get UTF8 string */
    PtGetResource(app->phsearch_text, Pt_ARG_TEXT_STRING, &textptr, 0);

    /* Convert UTF8 (multibyte) string to wide string */
    it=0;
    textptr_utf8=textptr;
    do {
        int charlen;
        int restlen;
        int result;

        utf8strlen(textptr, &restlen);
        charlen=utf8len(textptr, restlen);
        result=utf8towc(&wc_text[it], textptr, charlen);
        if (result==-1)
        {
            wc_text[it]=' ';
        }
        it++;
        textptr+=charlen;
    } while(*textptr!=0);
    wc_text[it]=0x00000000;
    wc_text_length=it;
    textptr=textptr_utf8;

    /* Check if it will a new search, if so clear history */
    if (strcmp(textptr, app->last_search)!=0)
    {
        app->search_hit=-1;
        app->search_hit_length=-1;
    }
    if (app->search_hit_page!=app->pdf_currpage-1)
    {
        app->search_hit=-1;
        app->search_hit_length=-1;
    }

    for (it=app->pdf_currpage-1; it<app->pdf_pagecount; it++)
    {
        /* Update current page */
        sprintf(text, "%d", it+1);
        PtSetResource(app->nbpage, Pt_ARG_TEXT_STRING, text, 0);
        PtDamageWidget(app->nbpage);
        PtFlush();

        /* Check if page was not loaded */
        if (app->pdf_pages[it].page==NULL)
        {
            app->pdf_pages[it].page=fz_load_page(app->pdfdoc, it);
        }
        /* Check if page list was not loaded */
        if (app->pdf_pages[it].page_list==NULL)
        {
            fz_cookie cookie={0};

            cookie.incomplete_ok=1;
            app->pdf_pages[it].page_list=fz_new_display_list(app->pdfctx);
            pdf_device=fz_new_list_device(app->pdfctx, app->pdf_pages[it].page_list);
            fz_run_page(app->pdfdoc, app->pdf_pages[it].page, pdf_device, &fz_identity, &cookie);
            fz_free_device(pdf_device);
            pdf_device=NULL;
        }
        /* Check if text was not loaded */
        if (app->pdf_pages[it].page_text==NULL)
        {
            app->pdf_pages[it].page_sheet=fz_new_text_sheet(app->pdfctx);
            app->pdf_pages[it].page_text=fz_new_text_page(app->pdfctx);
            pdf_device=fz_new_text_device(app->pdfctx, app->pdf_pages[it].page_sheet, app->pdf_pages[it].page_text);
            fz_run_display_list(app->pdf_pages[it].page_list, pdf_device, &fz_identity, &fz_infinite_rect, NULL);
            fz_free_device(pdf_device);
        }

        /* Check if current page has any text */
        if ((app->pdf_pages[it].page_text==NULL)||(app->pdf_pages[it].page_sheet==NULL))
        {
            /* Clear history */
            app->search_hit=-1;
            app->search_hit_length=-1;
            continue;
        }

        /* Now we can search if text matches on this page */
        {
            int matchlen;
            int test;
            int len;
            int smallest_y=INT_MAX;

            len=phmupdf_search_text_length(app->pdf_pages[it].page_text);

            if (app->search_hit>=0)
            {
                test=app->search_hit+wc_text_length;
            }
            else
            {
                test=0;
            }

            while (test<len)
            {
                matchlen=phmupdf_search_text_match(app, wc_text, app->pdf_pages[it].page_text, test);
                if (matchlen)
                {
                    app->search_hit=test;
                    app->search_hit_length=matchlen;
                    app->search_hit_page=it;
                    app->display_page=it;
                    app->display_found=1;

                    /* Store last searched text string */
                    strncpy(app->last_search, textptr, PHMUPDF_SEARCH_MAX_LENGTH);

                    /* Determine text boxes to highlight */
                    {
                        fz_irect hitbox;
                        fz_irect bbox;

                        hitbox=fz_empty_irect;
                        app->display_boxes=0;

                        for (jt=app->search_hit; jt<app->search_hit+app->search_hit_length; jt++)
                        {
                            bbox=phmupdf_search_bbox_charat(app->pdf_pages[it].page_text, jt);
                            if (fz_is_empty_irect(&bbox))
                            {
                                if (!fz_is_empty_irect(&hitbox))
                                {
                                    fz_rect temp_rect;

                                    fz_round_rect(&app->display_box[app->display_boxes++],
                                        fz_transform_rect(fz_rect_from_irect(&temp_rect, &hitbox),
                                        &app->pdf_pages[it].page_ctm));

                                    if (app->display_box[app->display_boxes-1].y0<smallest_y)
                                    {
                                        smallest_y=app->display_box[app->display_boxes-1].y0;
                                    }
                                    if (app->display_box[app->display_boxes-1].y1<smallest_y)
                                    {
                                        smallest_y=app->display_box[app->display_boxes-1].y1;
                                    }
                                }
                                hitbox=fz_empty_irect;
                            }
                            else
                            {
                                fz_rect temp_rect;
                                fz_rect temp_rect2;

                                fz_round_rect(&hitbox, fz_union_rect(fz_rect_from_irect(&temp_rect2, &hitbox),
                                                       fz_rect_from_irect(&temp_rect, &bbox)));
                            }
                        }

                        if (!fz_is_empty_irect(&hitbox))
                        {
                            fz_rect temp_rect;

                            fz_round_rect(&app->display_box[app->display_boxes++],
                                fz_transform_rect(fz_rect_from_irect(&temp_rect, &hitbox),
                                &app->pdf_pages[it].page_ctm));

                            if (app->display_box[app->display_boxes-1].y0<smallest_y)
                            {
                                smallest_y=app->display_box[app->display_boxes-1].y0;
                            }
                            if (app->display_box[app->display_boxes-1].y1<smallest_y)
                            {
                                smallest_y=app->display_box[app->display_boxes-1].y1;
                            }
                        }
                    }

                    /* Redraw image with highlighted found text */
                    PtDamageWidget(app->phcontent);

                    /* Adjust scrollbar settings */
                    if (app->phpagescroll)
                    {
                        int          kt;
                        unsigned int position;

                        /* Adjust scrollbar to show required page */
                        position=0;
                        for (kt=0; kt<it; kt++)
                        {
                            position+=(app->pdf_pages[kt].page_bbox_draw.y1-app->pdf_pages[kt].page_bbox_draw.y0)+1;
                            position+=PHMUPDF_GAP_BETWEEN_PAGES;
                        }
                        if (smallest_y>64)
                        {
                            position=position+smallest_y-64;
                        }
                        PtSetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, position, 0);

                        /* Update cursor in case if it points to link or text */
                        if ((app->last_x!=-1) && (app->last_y!=-1))
                        {
                            phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
                            phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
                        }
                    }

                    app->pdf_currpage=it+1;

                    return Pt_CONTINUE;
                }
                test++;
            }
        }

        /* Release page data if it is requested by user */
        if ((app->settings.fast_render==0) || (app->settings.fast_search==0))
        {
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
            if ((app->pdf_pages[it].page_list)&&(app->settings.fast_render==0))
            {
                fz_drop_display_list(app->pdfctx, app->pdf_pages[it].page_list);
                app->pdf_pages[it].page_list=NULL;
            }
            if ((app->pdf_pages[it].page)&&(app->settings.fast_render==0))
            {
                fz_free_page(app->pdfdoc, app->pdf_pages[it].page);
                app->pdf_pages[it].page=NULL;
            }
        }

        /* Clear history */
        app->search_hit=-1;
        app->search_hit_length=-1;
    }

    for (it=0; it<app->pdf_currpage; it++)
    {
        /* Update current page */
        sprintf(text, "%d", it+1);
        PtSetResource(app->nbpage, Pt_ARG_TEXT_STRING, text, 0);
        PtDamageWidget(app->nbpage);
        PtFlush();

        /* Check if page was not loaded */
        if (app->pdf_pages[it].page==NULL)
        {
            app->pdf_pages[it].page=fz_load_page(app->pdfdoc, it);
        }
        /* Check if page list was not loaded */
        if (app->pdf_pages[it].page_list==NULL)
        {
            fz_cookie cookie={0};

            cookie.incomplete_ok=1;
            app->pdf_pages[it].page_list=fz_new_display_list(app->pdfctx);
            pdf_device=fz_new_list_device(app->pdfctx, app->pdf_pages[it].page_list);
            fz_run_page(app->pdfdoc, app->pdf_pages[it].page, pdf_device, &fz_identity, &cookie);
            fz_free_device(pdf_device);
            pdf_device=NULL;
        }
        /* Check if text was not loaded */
        if (app->pdf_pages[it].page_text==NULL)
        {
            app->pdf_pages[it].page_sheet=fz_new_text_sheet(app->pdfctx);
            app->pdf_pages[it].page_text=fz_new_text_page(app->pdfctx);
            pdf_device=fz_new_text_device(app->pdfctx, app->pdf_pages[it].page_sheet, app->pdf_pages[it].page_text);
            fz_run_display_list(app->pdf_pages[it].page_list, pdf_device, &fz_identity, &fz_infinite_rect, NULL);
            fz_free_device(pdf_device);
        }

        /* Check if current page has any text */
        if ((app->pdf_pages[it].page_text==NULL)||(app->pdf_pages[it].page_sheet==NULL))
        {
            /* Clear history */
            app->search_hit=-1;
            app->search_hit_length=-1;
            continue;
        }

        /* Now we can search if text matches on this page */
        {
            int matchlen;
            int test;
            int len;
            int smallest_y=INT_MAX;

            len=phmupdf_search_text_length(app->pdf_pages[it].page_text);

            if (app->search_hit>=0)
            {
                test=app->search_hit+wc_text_length;
            }
            else
            {
                test=0;
            }

            while (test<len)
            {
                matchlen=phmupdf_search_text_match(app, wc_text, app->pdf_pages[it].page_text, test);
                if (matchlen)
                {
                    app->search_hit=test;
                    app->search_hit_length=matchlen;
                    app->search_hit_page=it;
                    app->display_page=it;
                    app->display_found=1;

                    /* Store last searched text string */
                    strncpy(app->last_search, textptr, PHMUPDF_SEARCH_MAX_LENGTH);

                    /* Determine text boxes to highlight */
                    {
                        fz_irect hitbox;
                        fz_irect bbox;

                        hitbox=fz_empty_irect;
                        app->display_boxes=0;

                        for (jt=app->search_hit; jt<app->search_hit+app->search_hit_length; jt++)
                        {
                            bbox=phmupdf_search_bbox_charat(app->pdf_pages[it].page_text, jt);
                            if (fz_is_empty_irect(&bbox))
                            {
                                if (!fz_is_empty_irect(&hitbox))
                                {
                                    fz_rect temp_rect;

                                    fz_round_rect(&app->display_box[app->display_boxes++],
                                        fz_transform_rect(fz_rect_from_irect(&temp_rect, &hitbox),
                                        &app->pdf_pages[it].page_ctm));

                                    if (app->display_box[app->display_boxes-1].y0<smallest_y)
                                    {
                                        smallest_y=app->display_box[app->display_boxes-1].y0;
                                    }
                                    if (app->display_box[app->display_boxes-1].y1<smallest_y)
                                    {
                                        smallest_y=app->display_box[app->display_boxes-1].y1;
                                    }
                                }
                                hitbox=fz_empty_irect;
                            }
                            else
                            {
                                fz_rect temp_rect;
                                fz_rect temp_rect2;

                                fz_round_rect(&hitbox, fz_union_rect(fz_rect_from_irect(&temp_rect2, &hitbox),
                                                       fz_rect_from_irect(&temp_rect, &bbox)));
                            }
                        }

                        if (!fz_is_empty_irect(&hitbox))
                        {
                            fz_rect temp_rect;

                            fz_round_rect(&app->display_box[app->display_boxes++],
                                fz_transform_rect(fz_rect_from_irect(&temp_rect, &hitbox),
                                &app->pdf_pages[it].page_ctm));
                            if (app->display_box[app->display_boxes-1].y0<smallest_y)
                            {
                                smallest_y=app->display_box[app->display_boxes-1].y0;
                            }
                            if (app->display_box[app->display_boxes-1].y1<smallest_y)
                            {
                                smallest_y=app->display_box[app->display_boxes-1].y1;
                            }
                        }
                    }

                    /* Redraw image with highlighted found text */
                    PtDamageWidget(app->phcontent);

                    /* Adjust scrollbar settings */
                    if (app->phpagescroll)
                    {
                        int          kt;
                        unsigned int position;

                        /* Adjust scrollbar to show required page */
                        position=0;
                        for (kt=0; kt<it; kt++)
                        {
                            position+=(app->pdf_pages[kt].page_bbox_draw.y1-app->pdf_pages[kt].page_bbox_draw.y0)+1;
                            position+=PHMUPDF_GAP_BETWEEN_PAGES;
                        }
                        if (smallest_y>64)
                        {
                            position=position+smallest_y-64;
                        }

                        /* Update cursor in case if it points to link or text */
                        if ((app->last_x!=-1) && (app->last_y!=-1))
                        {
                            phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
                            phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
                        }
                        PtSetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, position, 0);
                    }

                    app->pdf_currpage=it+1;

                    return Pt_CONTINUE;
                }
                test++;
            }
        }

        /* Release page data if it is requested by user */
        if ((app->settings.fast_render==0) || (app->settings.fast_search==0))
        {
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
            if ((app->pdf_pages[it].page_list)&&(app->settings.fast_render==0))
            {
                fz_drop_display_list(app->pdfctx, app->pdf_pages[it].page_list);
                app->pdf_pages[it].page_list=NULL;
            }
            if ((app->pdf_pages[it].page)&&(app->settings.fast_render==0))
            {
                fz_free_page(app->pdfdoc, app->pdf_pages[it].page);
                app->pdf_pages[it].page=NULL;
            }
        }

        /* Clear history */
        app->search_hit=-1;
        app->search_hit_length=-1;
    }

    /* Clear search history */
    app->search_hit=-1;
    app->search_hit_length=-1;
    app->display_found=0;

    PtNotice(app->phwindow, NULL, "String can't be found", NULL,
        "There is no more occurrence of string in the text", NULL, NULL, NULL,
         Pt_MODAL);

    /* Update match highlight */
    PtDamageWidget(app->phcontent);

    return Pt_CONTINUE;
}

int phmupdf_search_backward_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    char*           textptr;
    char*           textptr_utf8;
    int             it;
    int             jt;
    fz_device*      pdf_device=NULL;
    char            text[16];
    int             temp_page;
    wchar_t         wc_text[PHMUPDF_SEARCH_MAX_LENGTH];

    app=(photon_mupdf_t*)data;

    /* Get UTF8 string */
    PtGetResource(app->phsearch_text, Pt_ARG_TEXT_STRING, &textptr, 0);

    /* Convert UTF8 (multibyte) string to wide string */
    it=0;
    textptr_utf8=textptr;
    do {
        int charlen;
        int restlen;
        int result;

        utf8strlen(textptr, &restlen);
        charlen=utf8len(textptr, restlen);
        result=utf8towc(&wc_text[it], textptr, charlen);
        if (result==-1)
        {
            wc_text[it]=' ';
        }
        it++;
        textptr+=charlen;
    } while(*textptr!=0);
    wc_text[it]=0x00000000;
    textptr=textptr_utf8;

    /* Check if it will a new search, if so clear history */
    if (strcmp(textptr, app->last_search)!=0)
    {
        app->search_hit=-1;
        app->search_hit_length=-1;
    }
    if (app->search_hit_page!=app->pdf_currpage-1)
    {
        app->search_hit=-1;
        app->search_hit_length=-1;
    }

    /* Start search from previous page */
    temp_page=app->pdf_currpage-1;
    if (app->search_hit==-1)
    {
        temp_page--;
    }

    for (it=temp_page; it>=0; it--)
    {
        /* Update current page */
        sprintf(text, "%d", it+1);
        PtSetResource(app->nbpage, Pt_ARG_TEXT_STRING, text, 0);
        PtDamageWidget(app->nbpage);
        PtFlush();

        /* Check if page was not loaded */
        if (app->pdf_pages[it].page==NULL)
        {
            app->pdf_pages[it].page=fz_load_page(app->pdfdoc, it);
        }
        /* Check if page list was not loaded */
        if (app->pdf_pages[it].page_list==NULL)
        {
            fz_cookie cookie={0};

            cookie.incomplete_ok=1;
            app->pdf_pages[it].page_list=fz_new_display_list(app->pdfctx);
            pdf_device=fz_new_list_device(app->pdfctx, app->pdf_pages[it].page_list);
            fz_run_page(app->pdfdoc, app->pdf_pages[it].page, pdf_device, &fz_identity, &cookie);
            fz_free_device(pdf_device);
            pdf_device=NULL;
        }
        /* Check if text was not loaded */
        if (app->pdf_pages[it].page_text==NULL)
        {
            app->pdf_pages[it].page_sheet=fz_new_text_sheet(app->pdfctx);
            app->pdf_pages[it].page_text=fz_new_text_page(app->pdfctx);
            pdf_device=fz_new_text_device(app->pdfctx, app->pdf_pages[it].page_sheet, app->pdf_pages[it].page_text);
            fz_run_display_list(app->pdf_pages[it].page_list, pdf_device, &fz_identity, &fz_infinite_rect, NULL);
            fz_free_device(pdf_device);
        }

        /* Check if current page has any text */
        if ((app->pdf_pages[it].page_text==NULL)||(app->pdf_pages[it].page_sheet==NULL))
        {
            /* Clear history */
            app->search_hit=-1;
            app->search_hit_length=-1;
            continue;
        }

        /* Now we can search if text matches on this page */
        {
            int matchlen;
            int test;
            int len;
            int smallest_y=INT_MAX;

            len=phmupdf_search_text_length(app->pdf_pages[it].page_text);

            if (app->search_hit>=0)
            {
                test=app->search_hit-1;
            }
            else
            {
                test=len;
            }

            while (test>=0)
            {
                matchlen=phmupdf_search_text_match(app, wc_text, app->pdf_pages[it].page_text, test);
                if (matchlen)
                {
                    app->search_hit=test;
                    app->search_hit_length=matchlen;
                    app->search_hit_page=it;
                    app->display_page=it;
                    app->display_found=1;

                    /* Store last searched text string */
                    strncpy(app->last_search, textptr, PHMUPDF_SEARCH_MAX_LENGTH);

                    /* Determine text boxes to highlight */
                    {
                        fz_irect hitbox;
                        fz_irect bbox;

                        hitbox=fz_empty_irect;
                        app->display_boxes=0;

                        for (jt=app->search_hit; jt<app->search_hit+app->search_hit_length; jt++)
                        {
                            bbox=phmupdf_search_bbox_charat(app->pdf_pages[it].page_text, jt);
                            if (fz_is_empty_irect(&bbox))
                            {
                                if (!fz_is_empty_irect(&hitbox))
                                {
                                    fz_rect temp_rect;

                                    fz_round_rect(&app->display_box[app->display_boxes++],
                                        fz_transform_rect(fz_rect_from_irect(&temp_rect, &hitbox),
                                        &app->pdf_pages[it].page_ctm));
                                    if (app->display_box[app->display_boxes-1].y0<smallest_y)
                                    {
                                        smallest_y=app->display_box[app->display_boxes-1].y0;
                                    }
                                    if (app->display_box[app->display_boxes-1].y1<smallest_y)
                                    {
                                        smallest_y=app->display_box[app->display_boxes-1].y1;
                                    }
                                }
                                hitbox=fz_empty_irect;
                            }
                            else
                            {
                                fz_rect temp_rect;
                                fz_rect temp_rect2;

                                fz_round_rect(&hitbox, fz_union_rect(fz_rect_from_irect(&temp_rect2, &hitbox),
                                                       fz_rect_from_irect(&temp_rect, &bbox)));
                            }
                        }

                        if (!fz_is_empty_irect(&hitbox))
                        {
                            fz_rect temp_rect;

                            fz_round_rect(&app->display_box[app->display_boxes++],
                                fz_transform_rect(fz_rect_from_irect(&temp_rect, &hitbox),
                                &app->pdf_pages[it].page_ctm));
                            if (app->display_box[app->display_boxes-1].y0<smallest_y)
                            {
                                smallest_y=app->display_box[app->display_boxes-1].y0;
                            }
                            if (app->display_box[app->display_boxes-1].y1<smallest_y)
                            {
                                smallest_y=app->display_box[app->display_boxes-1].y1;
                            }
                        }
                    }

                    /* Redraw image with highlighted found text */
                    PtDamageWidget(app->phcontent);

                    /* Adjust scrollbar settings */
                    if (app->phpagescroll)
                    {
                        int          kt;
                        unsigned int position;

                        /* Adjust scrollbar to show required page */
                        position=0;
                        for (kt=0; kt<it; kt++)
                        {
                            position+=(app->pdf_pages[kt].page_bbox_draw.y1-app->pdf_pages[kt].page_bbox_draw.y0)+1;
                            position+=PHMUPDF_GAP_BETWEEN_PAGES;
                        }
                        if (smallest_y>64)
                        {
                            position=position+smallest_y-64;
                        }
                        PtSetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, position, 0);

                        /* Update cursor in case if it points to link or text */
                        if ((app->last_x!=-1) && (app->last_y!=-1))
                        {
                            phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
                            phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
                        }
                    }

                    app->pdf_currpage=it+1;

                    return Pt_CONTINUE;
                }
                test--;
            }
        }

        /* Release page data if it is requested by user */
        if ((app->settings.fast_render==0) || (app->settings.fast_search==0))
        {
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
            if ((app->pdf_pages[it].page_list)&&(app->settings.fast_render==0))
            {
                fz_drop_display_list(app->pdfctx, app->pdf_pages[it].page_list);
                app->pdf_pages[it].page_list=NULL;
            }
            if ((app->pdf_pages[it].page)&&(app->settings.fast_render==0))
            {
                fz_free_page(app->pdfdoc, app->pdf_pages[it].page);
                app->pdf_pages[it].page=NULL;
            }
        }

        /* Clear history */
        app->search_hit=-1;
        app->search_hit_length=-1;
    }

    /* Adjust page offset */
    if (temp_page==-1)
    {
        temp_page=0;
    }

    for (it=app->pdf_pagecount-1; it>=temp_page; it--)
    {
        /* Update current page */
        sprintf(text, "%d", it+1);
        PtSetResource(app->nbpage, Pt_ARG_TEXT_STRING, text, 0);
        PtDamageWidget(app->nbpage);
        PtFlush();

        /* Check if page was not loaded */
        if (app->pdf_pages[it].page==NULL)
        {
            app->pdf_pages[it].page=fz_load_page(app->pdfdoc, it);
        }
        /* Check if page list was not loaded */
        if (app->pdf_pages[it].page_list==NULL)
        {
            fz_cookie cookie={0};

            cookie.incomplete_ok=1;
            app->pdf_pages[it].page_list=fz_new_display_list(app->pdfctx);
            pdf_device=fz_new_list_device(app->pdfctx, app->pdf_pages[it].page_list);
            fz_run_page(app->pdfdoc, app->pdf_pages[it].page, pdf_device, &fz_identity, &cookie);
            fz_free_device(pdf_device);
            pdf_device=NULL;
        }
        /* Check if text was not loaded */
        if (app->pdf_pages[it].page_text==NULL)
        {
            app->pdf_pages[it].page_sheet=fz_new_text_sheet(app->pdfctx);
            app->pdf_pages[it].page_text=fz_new_text_page(app->pdfctx);
            pdf_device=fz_new_text_device(app->pdfctx, app->pdf_pages[it].page_sheet, app->pdf_pages[it].page_text);
            fz_run_display_list(app->pdf_pages[it].page_list, pdf_device, &fz_identity, &fz_infinite_rect, NULL);
            fz_free_device(pdf_device);
        }

        /* Check if current page has any text */
        if ((app->pdf_pages[it].page_text==NULL)||(app->pdf_pages[it].page_sheet==NULL))
        {
            /* Clear history */
            app->search_hit=-1;
            app->search_hit_length=-1;
            continue;
        }

        /* Now we can search if text matches on this page */
        {
            int matchlen;
            int test;
            int len;
            int smallest_y=INT_MAX;

            len=phmupdf_search_text_length(app->pdf_pages[it].page_text);

            if (app->search_hit>=0)
            {
                test=app->search_hit-1;
            }
            else
            {
                test=len;
            }

            while (test>=0)
            {
                matchlen=phmupdf_search_text_match(app, wc_text, app->pdf_pages[it].page_text, test);
                if (matchlen)
                {
                    app->search_hit=test;
                    app->search_hit_length=matchlen;
                    app->search_hit_page=it;
                    app->display_page=it;
                    app->display_found=1;

                    /* Store last searched text string */
                    strncpy(app->last_search, textptr, PHMUPDF_SEARCH_MAX_LENGTH);

                    /* Determine text boxes to highlight */
                    {
                        fz_irect hitbox;
                        fz_irect bbox;

                        hitbox=fz_empty_irect;
                        app->display_boxes=0;

                        for (jt=app->search_hit; jt<app->search_hit+app->search_hit_length; jt++)
                        {
                            bbox=phmupdf_search_bbox_charat(app->pdf_pages[it].page_text, jt);
                            if (fz_is_empty_irect(&bbox))
                            {
                                if (!fz_is_empty_irect(&hitbox))
                                {
                                    fz_rect temp_rect;

                                    fz_round_rect(&app->display_box[app->display_boxes++],
                                        fz_transform_rect(fz_rect_from_irect(&temp_rect, &hitbox),
                                        &app->pdf_pages[it].page_ctm));
                                    if (app->display_box[app->display_boxes-1].y0<smallest_y)
                                    {
                                        smallest_y=app->display_box[app->display_boxes-1].y0;
                                    }
                                    if (app->display_box[app->display_boxes-1].y1<smallest_y)
                                    {
                                        smallest_y=app->display_box[app->display_boxes-1].y1;
                                    }
                                }
                                hitbox=fz_empty_irect;
                            }
                            else
                            {
                                fz_rect temp_rect;
                                fz_rect temp_rect2;

                                fz_round_rect(&hitbox, fz_union_rect(fz_rect_from_irect(&temp_rect2, &hitbox),
                                                       fz_rect_from_irect(&temp_rect, &bbox)));
                            }
                        }

                        if (!fz_is_empty_irect(&hitbox))
                        {
                            fz_rect temp_rect;

                            fz_round_rect(&app->display_box[app->display_boxes++],
                                fz_transform_rect(fz_rect_from_irect(&temp_rect, &hitbox),
                                &app->pdf_pages[it].page_ctm));
                            if (app->display_box[app->display_boxes-1].y0<smallest_y)
                            {
                                smallest_y=app->display_box[app->display_boxes-1].y0;
                            }
                            if (app->display_box[app->display_boxes-1].y1<smallest_y)
                            {
                                smallest_y=app->display_box[app->display_boxes-1].y1;
                            }
                        }
                    }

                    /* Redraw image with highlighted found text */
                    PtDamageWidget(app->phcontent);

                    /* Adjust scrollbar settings */
                    if (app->phpagescroll)
                    {
                        int          kt;
                        unsigned int position;

                        /* Adjust scrollbar to show required page */
                        position=0;
                        for (kt=0; kt<it; kt++)
                        {
                            position+=(app->pdf_pages[kt].page_bbox_draw.y1-app->pdf_pages[kt].page_bbox_draw.y0)+1;
                            position+=PHMUPDF_GAP_BETWEEN_PAGES;
                        }
                        if (smallest_y>64)
                        {
                            position=position+smallest_y-64;
                        }

                        /* Update cursor in case if it points to link or text */
                        if ((app->last_x!=-1) && (app->last_y!=-1))
                        {
                            phmupdf_content_find_text(app, app->last_x, app->last_y, 0);
                            phmupdf_content_find_link(app, app->last_x, app->last_y, 0);
                        }
                        PtSetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, position, 0);
                    }

                    app->pdf_currpage=it+1;

                    return Pt_CONTINUE;
                }
                test--;
            }
        }

        /* Release page data if it is requested by user */
        if ((app->settings.fast_render==0) || (app->settings.fast_search==0))
        {
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
            if ((app->pdf_pages[it].page_list)&&(app->settings.fast_render==0))
            {
                fz_drop_display_list(app->pdfctx, app->pdf_pages[it].page_list);
                app->pdf_pages[it].page_list=NULL;
            }
            if ((app->pdf_pages[it].page)&&(app->settings.fast_render==0))
            {
                fz_free_page(app->pdfdoc, app->pdf_pages[it].page);
                app->pdf_pages[it].page=NULL;
            }
        }

        /* Clear history */
        app->search_hit=-1;
        app->search_hit_length=-1;
    }

    /* Clear search history */
    app->search_hit=-1;
    app->search_hit_length=-1;
    app->display_found=0;

    PtNotice(app->phwindow, NULL, "String can't be found", NULL,
        "There is no more occurrence of string in the text", NULL, NULL, NULL,
         Pt_MODAL);

    /* Update match highlight */
    PtDamageWidget(app->phcontent);

    return Pt_CONTINUE;
}

int phmupdf_search_close_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;

    app=(photon_mupdf_t*)data;

    PtDestroyWidget(app->phsearch_shadow);
    PtDestroyWidget(app->phsearch_pane);
    PtGiveFocus(app->phpagescroll, NULL);
    PtDamageWidget(app->phcontent);
    PtFlush();

    return Pt_CONTINUE;
}

int phmupdf_search_case_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;

    app=(photon_mupdf_t*)data;
    app->case_search=!app->case_search;

    if (app->case_search==1)
    {
        PtSetResource(widget, Pt_ARG_FILL_COLOR, PgRGB(0xA0, 0xA0, 0xA0), 0);
        PtFlush();
    }
    else
    {
        PtSetResource(widget, Pt_ARG_FILL_COLOR, PgRGB(0xD8, 0xD8, 0xD8), 0);
        PtFlush();
    }

    return Pt_CONTINUE;
}

int phmupdf_search_callback_destroyed(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    char*           textptr;

    app=(photon_mupdf_t*)data;

    PtGetResource(widget, Pt_ARG_TEXT_STRING, &textptr, 0);
    strncpy(app->last_search, textptr, PHMUPDF_SEARCH_MAX_LENGTH);

    app->phsearch_pane=NULL;
    if (app->phsearch_shadow)
    {
        PtDestroyWidget(app->phsearch_shadow);
        app->phsearch_shadow=NULL;
    }

    app->display_found=0;
    PtDamageWidget(app->phcontent);

    return Pt_CONTINUE;
}


int phmupdf_search_callback_raw(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;

    app=(photon_mupdf_t*)data;

    switch (info->event->type)
    {
         case Ph_EV_KEY:
              {
                  PhKeyEvent_t* keyevent=NULL;

                  keyevent=PhGetData(info->event);
                  if (keyevent==NULL)
                  {
                      break;
                  }

                  if ((keyevent->key_flags & Pk_KF_Key_Repeat)==Pk_KF_Key_Repeat)
                  {
                      break;
                  }

                  /* Check if key has been translated to the symbol */
                  if ((keyevent->key_flags & Pk_KF_Sym_Valid)==Pk_KF_Sym_Valid)
                  {
                      if (keyevent->key_sym==Pk_Escape)
                      {
                          PtDestroyWidget(app->phsearch_shadow);
                          PtDestroyWidget(app->phsearch_pane);
                          PtGiveFocus(app->phpagescroll, NULL);
                          PtDamageWidget(app->phcontent);
                          PtFlush();
                          return Pt_CONSUME;
                      }
                      if ((keyevent->key_sym==Pk_Return) && (keyevent->key_mods==0))
                      {
                          phmupdf_search_forward_callback(widget, data, info);
                          return Pt_CONSUME;
                      }
                      if ((keyevent->key_sym==Pk_Return) && (keyevent->key_mods==Pk_KM_Shift))
                      {
                          phmupdf_search_backward_callback(widget, data, info);
                          return Pt_CONSUME;
                      }
                      if ((keyevent->key_sym==Pk_Pg_Up) || (keyevent->key_sym==Pk_Pg_Down) ||
                          (keyevent->key_sym==Pk_Up) || (keyevent->key_sym==Pk_Down))
                      {
                          PtGiveFocus(app->phpagescroll, NULL);
                          return Pt_CONSUME;
                      }
                  }
              }
              break;
    }

    return Pt_CONTINUE;
}

int phmupdf_search_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    PtArg_t         winargs[32];
    uint32_t        winargc=0;
    PhDim_t         temp_dim;
    PhPoint_t       temp_point;
    PtCallback_t    temp_cb;
    PtRawCallback_t raw_cb;

    app=(photon_mupdf_t*)data;

    if ((app->phpagescroll==NULL) || (app->pdf_pages==NULL))
    {
        return Pt_CONTINUE;
    }

    if (app->phsearch_pane)
    {
        int start=0;
        int end=32767;

        PtTextSetSelection(app->phsearch_text, &start, &end);
        PtGiveFocus(app->phsearch_text, NULL);

        return Pt_CONTINUE;
    }

    /* Reset search history */
    app->search_hit=-1;
    app->search_hit_length=-1;

    /* Destroy previously created images if any */
    if (app->search_forward_img!=NULL)
    {
        free(app->search_forward_img);
        app->search_forward_img=NULL;
    }
    if (app->search_backward_img!=NULL)
    {
        free(app->search_backward_img);
        app->search_backward_img=NULL;
    }
    if (app->search_close_img!=NULL)
    {
        free(app->search_close_img);
        app->search_close_img=NULL;
    }
    if (app->search_case_img!=NULL)
    {
        free(app->search_case_img);
        app->search_case_img=NULL;
    }

    /* Create "Search Forward" image */
    app->search_forward_img=PhCreateImage(NULL, PH_SICON_WIDTH, PH_SICON_HEIGHT, Pg_IMAGE_DIRECT_8888, NULL, 0, 0);
    PhReleaseImage(app->search_forward_img);
    app->search_forward_img->image=(char*)search_forward_image;
    app->search_forward_img->bpl=PH_SICON_WIDTH*4;
    app->search_forward_img->flags=Ph_RELEASE_ALPHA;
    /* Enable per pixel alpha blending */
    app->search_forward_img->alpha=calloc(1, sizeof(PgAlpha_t));
    app->search_forward_img->alpha->alpha_op=Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs;

    /* Create "Search Backward" image */
    app->search_backward_img=PhCreateImage(NULL, PH_SICON_WIDTH, PH_SICON_HEIGHT, Pg_IMAGE_DIRECT_8888, NULL, 0, 0);
    PhReleaseImage(app->search_backward_img);
    app->search_backward_img->image=(char*)search_backward_image;
    app->search_backward_img->bpl=PH_SICON_WIDTH*4;
    app->search_backward_img->flags=Ph_RELEASE_ALPHA;
    /* Enable per pixel alpha blending */
    app->search_backward_img->alpha=calloc(1, sizeof(PgAlpha_t));
    app->search_backward_img->alpha->alpha_op=Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs;

    /* Create "Search Close" image */
    app->search_close_img=PhCreateImage(NULL, PH_SSICON_WIDTH, PH_SSICON_HEIGHT, Pg_IMAGE_DIRECT_8888, NULL, 0, 0);
    PhReleaseImage(app->search_close_img);
    app->search_close_img->image=(char*)search_close_image;
    app->search_close_img->bpl=PH_SSICON_WIDTH*4;
    app->search_close_img->flags=Ph_RELEASE_ALPHA;
    /* Enable per pixel alpha blending */
    app->search_close_img->alpha=calloc(1, sizeof(PgAlpha_t));
    app->search_close_img->alpha->alpha_op=Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs;

    /* Create "Search Case" image */
    app->search_case_img=PhCreateImage(NULL, PH_SSICON_WIDTH, PH_SSICON_HEIGHT, Pg_IMAGE_DIRECT_8888, NULL, 0, 0);
    PhReleaseImage(app->search_case_img);
    app->search_case_img->image=(char*)search_case_image;
    app->search_case_img->bpl=PH_SSICON_WIDTH*4;
    app->search_case_img->flags=Ph_RELEASE_ALPHA;
    /* Enable per pixel alpha blending */
    app->search_case_img->alpha=calloc(1, sizeof(PgAlpha_t));
    app->search_case_img->alpha->alpha_op=Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs;

    /* Create PtPane widget for shadow of search dialog */
    winargc=0;
    temp_point.x=app->window.dimension.w-PHMUPDF_SEARCHBAR_WIDTH-PHMUPDF_SCROLLBAR_WIDTH+1;
    temp_point.y=1;
    temp_dim.h=PHMUPDF_SEARCHBAR_HEIGHT;
    temp_dim.w=PHMUPDF_SEARCHBAR_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0x20, 0x20, 0x20), 0);

    /* Create the PtPane widget */
    app->phsearch_shadow=PtCreateWidget(PtPane, app->phcontent, winargc, winargs);
    if (app->phnavibar_shadow==NULL)
    {
        return -1;
    }

    /* Create PtPane widget for search dialog */
    winargc=0;
    temp_point.x=app->window.dimension.w-PHMUPDF_SEARCHBAR_WIDTH-PHMUPDF_SCROLLBAR_WIDTH-1;
    temp_point.y=0;
    temp_dim.h=PHMUPDF_SEARCHBAR_HEIGHT;
    temp_dim.w=PHMUPDF_SEARCHBAR_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xD8, 0xD8, 0xD8), 0);

    /* Create the PtPane widget */
    app->phsearch_pane=PtCreateWidget(PtPane, app->phcontent, winargc, winargs);
    if (app->phnavibar==NULL)
    {
        return -1;
    }

    /* Create PtText widget for search text input */
    winargc=0;
    temp_point.x=24-7;
    temp_point.y=3;
    temp_dim.h=PHMUPDF_SEARCHBAR_HEIGHT-3*2-3;
    temp_dim.w=PHMUPDF_SEARCHBAR_WIDTH-24*2;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xC0, 0xC0, 0xC0), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_MAX_LENGTH, PHMUPDF_SEARCH_MAX_LENGTH, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, app->last_search, 0);
    temp_cb.event_f=phmupdf_search_callback_destroyed;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_IS_DESTROYED, &temp_cb, 0);
    raw_cb.event_mask=Ph_EV_KEY;
    raw_cb.event_f=phmupdf_search_callback_raw;
    raw_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_RAW, &raw_cb, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Text to search", 0);

    /* Create the PtText widget */
    app->phsearch_text=PtCreateWidget(PtText, app->phsearch_pane, winargc, winargs);
    if (app->phnavibar==NULL)
    {
        return -1;
    }

    /* Set default selection of text */
    {
        int start=0;
        int end=32767;

        PtTextSetSelection(app->phsearch_text, &start, &end);
    }

    /* Create PtButton widget in the search to search backward */
    winargc=0;
    temp_point.x=4;
    temp_point.y=4;
    temp_dim.h=PH_SICON_HEIGHT;
    temp_dim.w=PH_SICON_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
         Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES | Pt_STATIC_GRADIENT);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE,
         Pt_FLAT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_IMAGE, app->search_backward_img, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE,
        Pt_HIGHLIGHTED | Pt_GETS_FOCUS | Pt_FOCUS_RENDER);
    temp_cb.event_f=phmupdf_search_backward_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Search backward", 0);

    /* Create the PtButton widget */
    app->phsearch_backward=PtCreateWidget(PtButton, app->phsearch_pane, winargc, winargs);
    if (app->phsearch_backward==NULL)
    {
        return -1;
    }

    /* Create PtButton widget in the search to search forward */
    winargc=0;
    temp_point.x=24-7+PHMUPDF_SEARCHBAR_WIDTH-24*2;
    temp_point.y=4;
    temp_dim.h=PH_SICON_HEIGHT;
    temp_dim.w=PH_SICON_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
         Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES | Pt_STATIC_GRADIENT);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE,
         Pt_FLAT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_IMAGE, app->search_forward_img, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE,
        Pt_HIGHLIGHTED | Pt_GETS_FOCUS | Pt_FOCUS_RENDER);
    temp_cb.event_f=phmupdf_search_forward_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Search forward", 0);

    /* Create the PtButton widget */
    app->phsearch_forward=PtCreateWidget(PtButton, app->phsearch_pane, winargc, winargs);
    if (app->phsearch_forward==NULL)
    {
        return -1;
    }

    /* Create PtButton widget in the search to close sub-dialog */
    winargc=0;
    temp_point.x=24-7+PHMUPDF_SEARCHBAR_WIDTH-24*2+16;
    temp_point.y=2;
    temp_dim.h=PH_SSICON_HEIGHT;
    temp_dim.w=PH_SSICON_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
         Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES | Pt_STATIC_GRADIENT);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE,
         Pt_FLAT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_IMAGE, app->search_close_img, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE,
        Pt_HIGHLIGHTED | Pt_GETS_FOCUS | Pt_FOCUS_RENDER);
    temp_cb.event_f=phmupdf_search_close_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Close dialog", 0);

    /* Create the PtButton widget */
    app->phsearch_close=PtCreateWidget(PtButton, app->phsearch_pane, winargc, winargs);
    if (app->phsearch_close==NULL)
    {
        return -1;
    }

    /* Create PtButton widget in the search to stick case sensivity */
    winargc=0;
    temp_point.x=24-7+PHMUPDF_SEARCHBAR_WIDTH-24*2+16;
    temp_point.y=18;
    temp_dim.h=PH_SSICON_HEIGHT;
    temp_dim.w=PH_SSICON_WIDTH;
    PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_FALSE,
         Pt_ALL_ETCHES | Pt_ALL_BEVELS | Pt_ALL_OUTLINES | Pt_STATIC_GRADIENT);
    PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE,
         Pt_FLAT_FILL);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_IMAGE, app->search_case_img, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE,
        Pt_HIGHLIGHTED | Pt_GETS_FOCUS | Pt_FOCUS_RENDER);
    temp_cb.event_f=phmupdf_search_case_callback;
    temp_cb.data=(void*)app;
    PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_LABEL_FLAGS, Pt_TRUE, Pt_SHOW_BALLOON);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_FILL_COLOR, PgRGB(0x40, 0x40, 0x40), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_COLOR, PgRGB(0xFF, 0xFF, 0xFF), 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_POSITION, Pt_BALLOON_TOP, 0);
    PtSetArg(&winargs[winargc++], Pt_ARG_BALLOON_TEXT, "Case sensivity", 0);

    /* Create the PtButton widget */
    app->phsearch_case=PtCreateWidget(PtButton, app->phsearch_pane, winargc, winargs);
    if (app->phsearch_case==NULL)
    {
        return -1;
    }

    PtRealizeWidget(app->phsearch_shadow);
    PtRealizeWidget(app->phsearch_pane);
    PtDamageWidget(app->phcontent);
    PtGiveFocus(app->phsearch_text, NULL);
    PtFlush();

    return Pt_CONTINUE;
}

int phmupdf_settings_callback_destroyed(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;

    app=(photon_mupdf_t*)data;

    app->phwindow_settings=NULL;
    app->phsettings_fast_search=NULL;
    app->phsettings_fast_render=NULL;
    app->phsettings_bw_render=NULL;
    app->phsettings_off_render=NULL;

    /* Unblock main window */
    PtSetResource(app->phwindow, Pt_ARG_WINDOW_STATE,
        Pt_FALSE, Ph_WM_STATE_ISBLOCKED);
    PtUnblockWindows(app->settings_bl);

    return Pt_CONTINUE;
}

int phmupdf_settings_ok_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    long*           flags;

    app=(photon_mupdf_t*)data;

    PtGetResource(app->phsettings_fast_search, Pt_ARG_FLAGS, &flags, 0);
    if (*flags & Pt_SET)
    {
        app->settings.fast_search=1;
    }
    else
    {
        app->settings.fast_search=0;
    }

    PtGetResource(app->phsettings_fast_render, Pt_ARG_FLAGS, &flags, 0);
    if (*flags & Pt_SET)
    {
        app->settings.fast_render=1;
    }
    else
    {
        app->settings.fast_render=0;
    }

    PtGetResource(app->phsettings_bw_render, Pt_ARG_FLAGS, &flags, 0);
    if (*flags & Pt_SET)
    {
        if (app->settings.bw_render==0)
        {
            phpdf_destroy_page_cache(app);
        }
        app->settings.bw_render=1;
    }
    else
    {
        if (app->settings.bw_render==1)
        {
            phpdf_destroy_page_cache(app);
        }
        app->settings.bw_render=0;
    }

    PtGetResource(app->phsettings_off_render, Pt_ARG_FLAGS, &flags, 0);
    if (*flags & Pt_SET)
    {
        if (app->settings.off_render==0)
        {
            phpdf_destroy_page_cache(app);
        }
        app->settings.off_render=1;
    }
    else
    {
        if (app->settings.off_render==1)
        {
            phpdf_destroy_page_cache(app);
        }
        app->settings.off_render=0;
    }

    PtGetResource(app->phsettings_aa_level, Pt_ARG_NUMERIC_VALUE, &flags, 0);
    if (*flags<0)
    {
        *flags=0;
    }
    if (*flags>8)
    {
        *flags=8;
    }
    if (*flags!=app->settings.aa_level)
    {
        phpdf_destroy_page_cache(app);
    }
    app->settings.aa_level=*flags;
    if (app->pdfctx!=NULL)
    {
        fz_set_aa_level(app->pdfctx, app->settings.aa_level);
    }

    if (app->phwindow_settings!=NULL)
    {
        PtDestroyWidget(app->phwindow_settings);
    }

    return Pt_CONTINUE;
}

int phmupdf_settings_cancel_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;

    app=(photon_mupdf_t*)data;
    if (app->phwindow_settings!=NULL)
    {
        PtDestroyWidget(app->phwindow_settings);
    }

    return Pt_CONTINUE;
}

int phmupdf_settings_callback_activate(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    long*           flags;

    app=(photon_mupdf_t*)data;
    PtGetResource(app->phsettings_fast_render, Pt_ARG_FLAGS, &flags, 0);
    if (*flags & Pt_SET)
    {
        PtSetResource(app->phsettings_fast_search_group, Pt_ARG_FLAGS, Pt_FALSE, Pt_BLOCKED);
        PtSetResource(app->phsettings_fast_search, Pt_ARG_COLOR, PgRGB(0, 0, 0), 0);
        PtSetResource(app->phsettings_fs_low_mem, Pt_ARG_COLOR, PgRGB(0, 0, 0), 0);
    }
    else
    {
        PtSetResource(app->phsettings_fast_search_group, Pt_ARG_FLAGS, Pt_TRUE, Pt_BLOCKED);
        PtSetResource(app->phsettings_fast_search, Pt_ARG_COLOR, PgRGB(160, 160, 160), 0);
        PtSetResource(app->phsettings_fs_low_mem, Pt_ARG_COLOR, PgRGB(160, 160, 160), 0);
    }

    PtGetResource(app->phsettings_fr_low_mem, Pt_ARG_FLAGS, &flags, 0);
    if (*flags & Pt_SET)
    {
        PtSetResource(app->phsettings_fast_search_group, Pt_ARG_FLAGS, Pt_TRUE, Pt_BLOCKED);
        PtSetResource(app->phsettings_fast_search, Pt_ARG_COLOR, PgRGB(160, 160, 160), 0);
        PtSetResource(app->phsettings_fs_low_mem, Pt_ARG_COLOR, PgRGB(160, 160, 160), 0);
    }
    else
    {
        PtSetResource(app->phsettings_fast_search_group, Pt_ARG_FLAGS, Pt_FALSE, Pt_BLOCKED);
        PtSetResource(app->phsettings_fast_search, Pt_ARG_COLOR, PgRGB(0, 0, 0), 0);
        PtSetResource(app->phsettings_fs_low_mem, Pt_ARG_COLOR, PgRGB(0, 0, 0), 0);
    }

    return Pt_CONTINUE;
}

int phmupdf_settings_callback_raw(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    switch (info->event->type)
    {
         case Ph_EV_KEY:
              {
                  PhKeyEvent_t* keyevent=NULL;

                  keyevent=PhGetData(info->event);
                  if (keyevent==NULL)
                  {
                      break;
                  }

                  if ((keyevent->key_flags & Pk_KF_Key_Repeat)==Pk_KF_Key_Repeat)
                  {
                      break;
                  }

                  /* Check if key has been translated to the symbol */
                  if ((keyevent->key_flags & Pk_KF_Sym_Valid)==Pk_KF_Sym_Valid)
                  {
                      if (keyevent->key_sym==Pk_Escape)
                      {
                          phmupdf_settings_cancel_callback(widget, data, info);
                          return Pt_CONSUME;
                      }
                      if (keyevent->key_sym==Pk_Return)
                      {
                          phmupdf_settings_ok_callback(widget, data, info);
                          return Pt_CONSUME;
                      }
                  }
              }
              break;
    }

    return Pt_CONTINUE;
}

int phmupdf_settings_callback(PtWidget_t* widget, void* data, PtCallbackInfo_t* info)
{
    photon_mupdf_t* app;
    PtArg_t         winargs[32];
    uint32_t        winargc=0;
    int32_t         status;
    PhDim_t         temp_dim;
    PhPoint_t       temp_point;
    PtCallback_t    temp_cb;
    PtCallback_t    temp_ok;
    PtCallback_t    temp_cancel;
    PtWidget_t*     temp_widget;
    PtWidget_t*     temp_widget2;
    PtRawCallback_t raw_cb;

    app=(photon_mupdf_t*)data;

    if (app->phwindow_settings==NULL)
    {
        temp_point.x=app->window.position.x+(app->window.dimension.w-400)/2;
        temp_point.y=app->window.position.y+(app->window.dimension.h-245)/2;
        temp_dim.w=385;
        temp_dim.h=210;
        PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_TITLE, "Settings for PhMuPDF", 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_FALSE,
                 Ph_WM_RENDER_MIN | Ph_WM_RENDER_MAX |
                 Ph_WM_RENDER_COLLAPSE | Ph_WM_RENDER_RESIZE);
        PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_TRUE,
                 Ph_WM_RENDER_CLOSE | Ph_WM_RENDER_MENU |
                 Ph_WM_RENDER_TITLE | Ph_WM_RENDER_MOVE |
                 Ph_WM_RENDER_BORDER);
        PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_MANAGED_FLAGS, Pt_TRUE,
                 Ph_WM_APP_DEF_MANAGED);
        PtSetArg(&winargs[winargc++], Pt_ARG_WINDOW_STATE, Pt_TRUE,
                 Ph_WM_STATE_ISFOCUS);
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xC0, 0xC0, 0xC0), 0);

        temp_cb.event_f=phmupdf_settings_callback_destroyed;
        temp_cb.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_IS_DESTROYED, &temp_cb, 0);

        raw_cb.event_mask=Ph_EV_KEY;
        raw_cb.event_f=phmupdf_settings_callback_raw;
        raw_cb.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_RAW, &raw_cb, 0);

        /* Finally create the window */
        app->phwindow_settings=PtCreateWidget(PtWindow, Pt_NO_PARENT, winargc, winargs);
        if (app->phwindow_settings==NULL)
        {
            return -1;
        }

        /* Create large pane: render optimizations */
        winargc=0;
        temp_point.x=8;
        temp_point.y=8;
        temp_dim.w=180;
        temp_dim.h=72;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xC0, 0xC0, 0xC0), 0);
        temp_widget=PtCreateWidget(PtPane, app->phwindow_settings, winargc, winargs);

        /* Create small pane: render optimizations */
        winargc=0;
        temp_point.x=8;
        temp_point.y=8;
        temp_dim.w=180;
        temp_dim.h=24;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xC0, 0xC0, 0xC0), 0);
        temp_widget2=PtCreateWidget(PtPane, app->phwindow_settings, winargc, winargs);

        /* Create small pane: render optimizations */
        winargc=0;
        temp_point.x=-2;
        temp_point.y=-2;
        temp_dim.w=202;
        temp_dim.h=24;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, " Render optimizations", 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE, Pt_REVERSE_GRADIENT);
        PtSetArg(&winargs[winargc++], Pt_ARG_HORIZONTAL_ALIGNMENT, Pt_LEFT, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE, Pt_SELECTABLE | Pt_HIGHLIGHTED |
            Pt_GETS_FOCUS | Pt_FOCUS_RENDER);
        PtCreateWidget(PtButton, temp_widget2, winargc, winargs);

        /* Create group: render optimizations */
        winargc=0;
        temp_point.x=0;
        temp_point.y=20;
        temp_dim.w=200;
        temp_dim.h=80;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_GROUP_FLAGS, Pt_TRUE, Pt_GROUP_EXCLUSIVE);
        PtSetArg(&winargs[winargc++], Pt_ARG_GROUP_ORIENTATION, Pt_GROUP_VERTICAL, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_GROUP_VERT_ALIGN, Pt_GROUP_VERT_TOP, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_GROUP_SPACING_Y, -6, 0);
        temp_widget2=PtCreateWidget(PtGroup, temp_widget, winargc, winargs);

        /* Create toggle buttons */
        winargc=0;
        temp_point.x=0;
        temp_point.y=21;
        temp_dim.w=200;
        temp_dim.h=28;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Low memory usage", 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_INDICATOR_TYPE, Pt_TOGGLE_RADIO, 0);
        if (!app->settings.fast_render)
        {
            PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_TRUE, Pt_SET);
        }
        else
        {
            PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE, Pt_SET);
        }
        raw_cb.event_mask=Ph_EV_KEY;
        raw_cb.event_f=phmupdf_settings_callback_raw;
        raw_cb.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_RAW, &raw_cb, 0);
        temp_cb.event_f=phmupdf_settings_callback_activate;
        temp_cb.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);
        app->phsettings_fr_low_mem=PtCreateWidget(PtToggleButton, temp_widget2, winargc, winargs);

        winargc=0;
        temp_point.x=0;
        temp_point.y=40;
        temp_dim.w=200;
        temp_dim.h=28;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Fast render", 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_INDICATOR_TYPE, Pt_TOGGLE_RADIO, 0);
        if (app->settings.fast_render)
        {
            PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_TRUE, Pt_SET);
        }
        else
        {
            PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE, Pt_SET);
        }
        raw_cb.event_mask=Ph_EV_KEY;
        raw_cb.event_f=phmupdf_settings_callback_raw;
        raw_cb.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_RAW, &raw_cb, 0);
        temp_cb.event_f=phmupdf_settings_callback_activate;
        temp_cb.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cb, 0);
        app->phsettings_fast_render=PtCreateWidget(PtToggleButton, temp_widget2, winargc, winargs);

        /* Create large pane: search optimizations */
        winargc=0;
        temp_point.x=196;
        temp_point.y=8;
        temp_dim.w=180;
        temp_dim.h=72;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xC0, 0xC0, 0xC0), 0);
        temp_widget=PtCreateWidget(PtPane, app->phwindow_settings, winargc, winargs);

        /* Create small pane: search optimizations */
        winargc=0;
        temp_point.x=196;
        temp_point.y=8;
        temp_dim.w=180;
        temp_dim.h=24;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_FILL_COLOR, PgRGB(0xC0, 0xC0, 0xC0), 0);
        temp_widget2=PtCreateWidget(PtPane, app->phwindow_settings, winargc, winargs);

        /* Create small pane: search optimizations */
        winargc=0;
        temp_point.x=-2;
        temp_point.y=-2;
        temp_dim.w=202;
        temp_dim.h=24;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, " Text search optimizations", 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_BASIC_FLAGS, Pt_TRUE, Pt_REVERSE_GRADIENT);
        PtSetArg(&winargs[winargc++], Pt_ARG_HORIZONTAL_ALIGNMENT, Pt_LEFT, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE, Pt_SELECTABLE | Pt_HIGHLIGHTED |
            Pt_GETS_FOCUS | Pt_FOCUS_RENDER);
        PtCreateWidget(PtButton, temp_widget2, winargc, winargs);

        /* Create group: search optimizations */
        winargc=0;
        temp_point.x=0;
        temp_point.y=20;
        temp_dim.w=200;
        temp_dim.h=80;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_GROUP_FLAGS, Pt_TRUE, Pt_GROUP_EXCLUSIVE);
        PtSetArg(&winargs[winargc++], Pt_ARG_GROUP_ORIENTATION, Pt_GROUP_VERTICAL, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_GROUP_VERT_ALIGN, Pt_GROUP_VERT_TOP, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_GROUP_SPACING_Y, -6, 0);
        temp_widget2=PtCreateWidget(PtGroup, temp_widget, winargc, winargs);
        app->phsettings_fast_search_group=temp_widget2;

        /* Create toggle buttons */
        winargc=0;
        temp_point.x=0;
        temp_point.y=21;
        temp_dim.w=200;
        temp_dim.h=28;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Low memory usage", 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_INDICATOR_TYPE, Pt_TOGGLE_RADIO, 0);
        if (!app->settings.fast_search)
        {
            PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_TRUE, Pt_SET);
        }
        else
        {
            PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE, Pt_SET);
        }
        raw_cb.event_mask=Ph_EV_KEY;
        raw_cb.event_f=phmupdf_settings_callback_raw;
        raw_cb.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_RAW, &raw_cb, 0);
        app->phsettings_fs_low_mem=PtCreateWidget(PtToggleButton, temp_widget2, winargc, winargs);

        winargc=0;
        temp_point.x=0;
        temp_point.y=40;
        temp_dim.w=200;
        temp_dim.h=28;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Fast search", 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_INDICATOR_TYPE, Pt_TOGGLE_RADIO, 0);
        if (app->settings.fast_search)
        {
            PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_TRUE, Pt_SET);
        }
        else
        {
            PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE, Pt_SET);
        }
        raw_cb.event_mask=Ph_EV_KEY;
        raw_cb.event_f=phmupdf_settings_callback_raw;
        raw_cb.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_RAW, &raw_cb, 0);
        app->phsettings_fast_search=PtCreateWidget(PtToggleButton, temp_widget2, winargc, winargs);

        if (app->settings.fast_render==0)
        {
            PtSetResource(app->phsettings_fast_search_group, Pt_ARG_FLAGS, Pt_TRUE, Pt_BLOCKED);
            PtSetResource(app->phsettings_fast_search, Pt_ARG_COLOR, PgRGB(160, 160, 160), 0);
            PtSetResource(app->phsettings_fs_low_mem, Pt_ARG_COLOR, PgRGB(160, 160, 160), 0);
        }

        winargc=0;
        temp_point.x=10;
        temp_point.y=80;
        temp_dim.w=100;
        temp_dim.h=28;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Black and white render", 0);
        if (app->settings.bw_render)
        {
            PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_TRUE, Pt_SET);
        }
        else
        {
            PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE, Pt_SET);
        }
        raw_cb.event_mask=Ph_EV_KEY;
        raw_cb.event_f=phmupdf_settings_callback_raw;
        raw_cb.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_RAW, &raw_cb, 0);
        app->phsettings_bw_render=PtCreateWidget(PtToggleButton, app->phwindow_settings, winargc, winargs);

        winargc=0;
        temp_point.x=198;
        temp_point.y=80;
        temp_dim.w=100;
        temp_dim.h=28;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Use offscreen memory", 0);
        if (app->settings.off_render)
        {
            PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_TRUE, Pt_SET);
        }
        else
        {
            PtSetArg(&winargs[winargc++], Pt_ARG_FLAGS, Pt_FALSE, Pt_SET);
        }
        raw_cb.event_mask=Ph_EV_KEY;
        raw_cb.event_f=phmupdf_settings_callback_raw;
        raw_cb.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_RAW, &raw_cb, 0);
        app->phsettings_off_render=PtCreateWidget(PtToggleButton, app->phwindow_settings, winargc, winargs);

        /* Create numeric input */
        winargc=0;
        temp_point.x=10;
        temp_point.y=104;
        temp_dim.w=12;
        temp_dim.h=12;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_NUMERIC_VALUE, app->settings.aa_level, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_NUMERIC_UPDOWN_WIDTH, 10, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_UPDOWN_INDICATOR_MARGIN, 0, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_NUMERIC_MIN, 0, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_NUMERIC_MAX, 8, 0);
        raw_cb.event_mask=Ph_EV_KEY;
        raw_cb.event_f=phmupdf_settings_callback_raw;
        raw_cb.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_RAW, &raw_cb, 0);
        temp_ok.event_f=phmupdf_settings_ok_callback;
        temp_ok.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_ok, 0);
        app->phsettings_aa_level=PtCreateWidget(PtNumericInteger, app->phwindow_settings, winargc, winargs);

        /* Create label */
        winargc=0;
        temp_point.x=44;
        temp_point.y=104;
        temp_dim.w=100;
        temp_dim.h=28;
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Anti-aliasing level", 0);
        PtCreateWidget(PtLabel, app->phwindow_settings, winargc, winargs);

        /* Create separator */
        winargc=0;
        temp_point.x=4;
        temp_point.y=132;
        temp_dim.h=4;
        temp_dim.w=377;
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_FALSE, Pt_SEP_ORIENTATION);
        PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_TRUE, Pt_SEP_HORIZONTAL);
        PtCreateWidget(PtSeparator, app->phwindow_settings, winargc, winargs);

        /* Create label */
        winargc=0;
        temp_point.x=4;
        temp_point.y=140;
        temp_dim.h=4;
        temp_dim.w=377;
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        {
            char text[128];

            sprintf(text, " Low memory cache size: %dKb", app->settings.cache_size/1024);
            PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, text, 0);
        }
        PtCreateWidget(PtLabel, app->phwindow_settings, winargc, winargs);

        /* Create separator */
        winargc=0;
        temp_point.x=4;
        temp_point.y=160;
        temp_dim.h=4;
        temp_dim.w=377;
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_FALSE, Pt_SEP_ORIENTATION);
        PtSetArg(&winargs[winargc++], Pt_ARG_SEP_FLAGS, Pt_TRUE, Pt_SEP_HORIZONTAL);
        PtCreateWidget(PtSeparator, app->phwindow_settings, winargc, winargs);

        /* Create ok button */
        winargc=0;
        temp_point.x=217;
        temp_point.y=147+28;
        temp_dim.h=24;
        temp_dim.w=76;
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Ok", 0);
        temp_ok.event_f=phmupdf_settings_ok_callback;
        temp_ok.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_ok, 0);
        PtCreateWidget(PtButton, app->phwindow_settings, winargc, winargs);

        /* Create cancel button */
        winargc=0;
        temp_point.x=301;
        temp_point.y=147+28;
        temp_dim.h=24;
        temp_dim.w=76;
        PtSetArg(&winargs[winargc++], Pt_ARG_POS, &temp_point, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_DIM, &temp_dim, 0);
        PtSetArg(&winargs[winargc++], Pt_ARG_TEXT_STRING, "Cancel", 0);
        temp_cancel.event_f=phmupdf_settings_cancel_callback;
        temp_cancel.data=(void*)app;
        PtSetArg(&winargs[winargc++], Pt_CB_ACTIVATE, &temp_cancel, 0);
        PtCreateWidget(PtButton, app->phwindow_settings, winargc, winargs);

        /* Show widget */
        status=PtRealizeWidget(app->phwindow_settings);
        if (status!=0)
        {
            PtDestroyWidget(app->phwindow_settings);
        }

        /* Block main window */
        PtSetResource(app->phwindow, Pt_ARG_WINDOW_STATE,
            Pt_TRUE, Ph_WM_STATE_ISBLOCKED);
        app->settings_bl=PtBlockWindow(app->phwindow, Ph_CURSOR_NOINPUT,
            Ph_CURSOR_DEFAULT_COLOR);

        PtFlush();
    }
    else
    {
        PtSetResource(app->phwindow_settings, Pt_ARG_WINDOW_STATE,
            Pt_TRUE, Ph_WM_STATE_ISFOCUS | Ph_WM_STATE_ISFRONT);

        PtFlush();
    }

    return Pt_CONTINUE;
}

void phmupdf_draw_content(PtWidget_t* widget, PhTile_t* damage)
{
    photon_mupdf_t* app;
    PhRect_t  raw_canvas;
    PhPoint_t position;
    PhRect_t  rect;
    PhRect_t  dst_rect;

    /* Get pointer to the application */
    PtGetResource(widget, Pt_ARG_POINTER, (void*)&app, 0);

    /* Prepare to draw images */
    PtSuperClassDraw(PtBasic, widget, damage);
    PtCalcCanvas(widget, &raw_canvas);
    PgSetTranslation(&raw_canvas.ul, Pg_RELATIVE);
    PtClipAdd(widget, &raw_canvas);

    /* Do any drawings if scrollbar is present and document is loaded */
    if ((app->phpagescroll) && (app->pdf_pages))
    {
        unsigned int* scroll_position;
        unsigned int  sc=0;
        unsigned int  offset=0;
        unsigned int  global_offset=0;
        int           it;
        int           pageno=-1;
        int           slotno=-1;

        PtGetResource(app->phpagescroll, Pt_ARG_GAUGE_VALUE, &scroll_position, 0);
        sc=*scroll_position;

        do {
            /* Find a page inside page cache, which corresponds current position */
            offset=0;
            pageno=-1;
            for (it=0; it<app->pdf_pagecount; it++)
            {
                if ((sc>=offset) &&
                    (sc<offset+(app->pdf_pages[it].page_bbox_draw.y1-app->pdf_pages[it].page_bbox_draw.y0+1)+PHMUPDF_GAP_BETWEEN_PAGES))
                {
                    pageno=it;
                    offset=sc-offset;
                    break;
                }
                offset+=(app->pdf_pages[it].page_bbox_draw.y1-app->pdf_pages[it].page_bbox_draw.y0+1)+PHMUPDF_GAP_BETWEEN_PAGES;
            }

            /* Display "pageno" page at offset "offset" */
            slotno=-1;
            for (it=0; it<PHMUPDF_RENDER_MAX_IMAGES; it++)
            {
                /* We have found requested page */
                if (app->page_image[it].pageno==pageno)
                {
                    slotno=it;
                    break;
                }
            }

            /* If free slot was not found destroy some entry */
            if (slotno==-1)
            {
                /* Load new page image */
                slotno=phpdf_find_mostdistance_page(app, pageno);
                phpdf_load_page(app, pageno, slotno);
            }

            if (app->page_image[slotno].page_image_phi)
            {
                if ((app->window.dimension.w-PHMUPDF_SCROLLBAR_WIDTH)>
                    app->page_image[slotno].page_image_phi->size.w)
                {
                    position.x=(app->window.dimension.w-PHMUPDF_SCROLLBAR_WIDTH-
                        app->page_image[slotno].page_image_phi->size.w)/2;
                    position.y=global_offset;
                }
                else
                {
                    position.x=2;
                    position.y=global_offset;
                }

                /* Cast a shadow first */
                rect.ul.x=position.x+2;
                rect.ul.y=position.y+2;
                rect.lr.x=rect.ul.x+app->page_image[slotno].page_image_phi->size.w;
                rect.lr.y=rect.ul.y+app->page_image[slotno].page_image_phi->size.h-offset;
                PgSetFillColor(PgRGB(0x20, 0x20, 0x20));
                PgDrawRect(&rect, Pg_DRAW_FILL);

                /* Draw an image */
                rect.ul.x=0;
                rect.ul.y=offset;
                rect.lr.x=app->page_image[slotno].page_image_phi->size.w;
                rect.lr.y=app->page_image[slotno].page_image_phi->size.h;
                PgDrawPhImageRectv(&position, app->page_image[slotno].page_image_phi, &rect, 0);
                PgFlush();
            }
            else
            {
                if (app->page_image[slotno].page_image_off)
                {
                    if ((app->window.dimension.w-PHMUPDF_SCROLLBAR_WIDTH)>
                        app->page_image[slotno].page_image_off->dim.w)
                    {
                        position.x=(app->window.dimension.w-PHMUPDF_SCROLLBAR_WIDTH-
                            app->page_image[slotno].page_image_off->dim.w)/2;
                        position.y=global_offset;
                    }
                    else
                    {
                        position.x=2;
                        position.y=global_offset;
                    }

                    /* Cast a shadow first */
                    rect.ul.x=position.x+2;
                    rect.ul.y=position.y+2;
                    rect.lr.x=rect.ul.x+app->page_image[slotno].page_image_off->dim.w;
                    rect.lr.y=rect.ul.y+app->page_image[slotno].page_image_off->dim.h-offset;
                    PgSetFillColor(PgRGB(0x20, 0x20, 0x20));
                    PgDrawRect(&rect, Pg_DRAW_FILL);

                    /* Draw an image */
                    rect.ul.x=0;
                    rect.ul.y=offset;
                    rect.lr.x=app->page_image[slotno].page_image_off->dim.w;
                    rect.lr.y=app->page_image[slotno].page_image_off->dim.h;
                    dst_rect.ul.x=position.x;
                    dst_rect.ul.y=position.y;
                    dst_rect.lr.x=position.x+app->page_image[slotno].page_image_off->dim.w;
                    dst_rect.lr.y=position.y+app->page_image[slotno].page_image_off->dim.h-offset;
                    PgContextBlit(app->page_image[slotno].page_image_off, &rect, PhDCGetCurrent(), &dst_rect);
                }
            }

            /* Highlight found text */
            if ((app->display_found) && (app->display_page==pageno))
            {
                int jt;

                for (jt=0; jt<app->display_boxes; jt++)
                {
                    dst_rect.ul.x=position.x+app->display_box[jt].x0;
                    dst_rect.ul.y=position.y+app->display_box[jt].y0-offset;
                    dst_rect.lr.x=position.x+app->display_box[jt].x1;
                    dst_rect.lr.y=position.y+app->display_box[jt].y1-offset;
                    PgAlphaOn();
                    PgSetAlpha(Pg_ALPHA_OP_SRC_GLOBAL | Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs, NULL, NULL, 0x60, NULL);
                    PgSetFillColor(PgRGB(0x20, 0x20, 0xA0));
                    PgDrawRect(&dst_rect, Pg_DRAW_FILL);
                    PgAlphaOff();
                }
            }

            /* Highlight selection text */
            if ((app->selection) && (app->selection_page==pageno))
            {
                int jt;

                for (jt=0; jt<app->selection_boxes; jt++)
                {
                    dst_rect.ul.x=position.x+app->selection_box[jt].x0;
                    dst_rect.ul.y=position.y+app->selection_box[jt].y0-offset;
                    dst_rect.lr.x=position.x+app->selection_box[jt].x1;
                    dst_rect.lr.y=position.y+app->selection_box[jt].y1-offset;
                    PgAlphaOn();
                    PgSetAlpha(Pg_ALPHA_OP_SRC_GLOBAL | Pg_BLEND_SRC_As | Pg_BLEND_DST_1mAs, NULL, NULL, 0x60, NULL);
                    PgSetFillColor(PgRGB(0xA0, 0xA0, 0x20));
                    PgDrawRect(&dst_rect, Pg_DRAW_FILL);
                    PgAlphaOff();
                }
            }

            global_offset+=((app->pdf_pages[pageno].page_bbox_draw.y1-app->pdf_pages[pageno].page_bbox_draw.y0+1)-offset)+PHMUPDF_GAP_BETWEEN_PAGES;
            sc+=((app->pdf_pages[pageno].page_bbox_draw.y1-app->pdf_pages[pageno].page_bbox_draw.y0+1)-offset)+PHMUPDF_GAP_BETWEEN_PAGES;
            /* No reason to draw outside of window dimensions */
            if (global_offset>=app->window.dimension.h)
            {
                break;
            }
            /* It is a last page */
            if (pageno==app->pdf_pagecount-1)
            {
                break;
            }
        } while(1);

        /* Flush all rendered graphics */
        PgFlush();
    }

    /* Restore clipping and translation */
    PtClipRemove();
    raw_canvas.ul.x*=-1;
    raw_canvas.ul.y*=-1;
    PgSetTranslation(&raw_canvas.ul, Pg_RELATIVE);
}
