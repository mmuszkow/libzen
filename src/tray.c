/*
 * Name        : tray.c
 * Author      : Maciej Muszkowski
 * Version     : 0.0.0.6
 * Copyright   : GPL
 * Description : GTK tray icon application showing charging progress
 */


#include <gtk/gtk.h>
#include "libzen.h"

static GtkStatusIcon *create_tray_icon() {
    GtkStatusIcon *tray_icon;

    tray_icon = gtk_status_icon_new();
    gtk_status_icon_set_visible(tray_icon, FALSE);

    return tray_icon;
}

static GdkPixbuf* create_progress_pixbuf(int val) {
    GdkPixbuf* pixbuf;
    cairo_surface_t *surface;
    cairo_t *cr;

    /* Create surface to draw */
    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, 16, 16);
    cr = cairo_create(surface);
    
    /* Draw progressbar */
    val = (val / 100.0f) * 16;
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
    cairo_rectangle(cr, 0, 0, 16, 16);
    cairo_fill(cr);
    cairo_set_source_rgb(cr, 0.672, 0.965, 0.117);
    cairo_rectangle(cr, 0, 0, val, 16);
    cairo_fill(cr);

    pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, 16, 16);

    /* Free cairo */
    cairo_surface_destroy(surface);
    cairo_destroy(cr);

    return pixbuf;
}

static gboolean update_status(gpointer user_data) {
    GtkStatusIcon *tray_icon;
    char battBuff[16];
    int  level = -1;

    tray_icon = (GtkStatusIcon*)user_data;

    usb_dev_handle* hdev = init_zen(0, 0);
    if(hdev) {
        int iter = 0;
        while((level = read_batt_level(hdev)) == ZEN_ERROR) {
            if(iter++ > 10) /* Max 10 tries */
            break;
        }
        deinit_zen(hdev);

        if(level == ZEN_ERROR) {
            /* Set error icon */
            gtk_status_icon_set_tooltip_text(tray_icon, "Error");
            gtk_status_icon_set_from_icon_name(tray_icon, GTK_STOCK_DIALOG_ERROR);
        } else {
            /* Set progress */
            if(level != 100) {
                sprintf(battBuff,"Charging: %d%%",level);
                gtk_status_icon_set_tooltip_text(tray_icon, battBuff);
            } else
                gtk_status_icon_set_tooltip_text(tray_icon, "Fully charged");

            gtk_status_icon_set_from_pixbuf(tray_icon, create_progress_pixbuf(level));
        }
        gtk_status_icon_set_visible(tray_icon, TRUE);
    } else {
        /* Hide icon */
        gtk_status_icon_set_visible(tray_icon, FALSE);
    }

    return 1;
}


int main(int argc, char **argv) {
    GtkStatusIcon *tray_icon;

    /* Initialise */
    gtk_init(&argc, &argv);
    tray_icon = create_tray_icon();

    /* Start timer */
    update_status(tray_icon);
    g_timeout_add_seconds(10, update_status, tray_icon);

    gtk_main();

    return 0;
}

