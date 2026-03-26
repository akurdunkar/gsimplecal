// Suppress GtkStatusIcon deprecation warnings in GTK3
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "SysTray.hpp"
#include "MainWindow.hpp"
#include "Config.hpp"
#include "config.h"


SysTray::SysTray()
{
    window = NULL;
    timer_id = 0;

    Config* config = Config::getInstance();
    if (g_file_test(config->systray_icon.c_str(), G_FILE_TEST_EXISTS)) {
        tray_icon = gtk_status_icon_new_from_file(config->systray_icon.c_str());
    } else {
        tray_icon = gtk_status_icon_new_from_icon_name(config->systray_icon.c_str());
    }
    gtk_status_icon_set_tooltip_text(tray_icon, "gsimplecal");
    gtk_status_icon_set_visible(tray_icon, TRUE);

    g_signal_connect(tray_icon, "activate",
                     G_CALLBACK(SysTray::onActivate), this);
    g_signal_connect(tray_icon, "popup-menu",
                     G_CALLBACK(SysTray::onPopupMenu), this);
}

SysTray::~SysTray()
{
    hideWindow();
    if (window) {
        delete window;
        window = NULL;
    }
    g_object_unref(tray_icon);
}

MainWindow* SysTray::getWindow()
{
    return window;
}

void SysTray::toggle()
{
    if (!window) {
        // First time: create the window and connect signals
        window = new MainWindow();

        Config* config = Config::getInstance();
        if (config->close_on_unfocus) {
            g_signal_connect(window->getWindow(), "focus-out-event",
                             G_CALLBACK(SysTray::onFocusOut), this);
        }
        if (config->close_on_mouseleave) {
            g_signal_connect(window->getWindow(), "leave-notify-event",
                             G_CALLBACK(SysTray::onMouseLeave), this);
        }
        g_signal_connect(window->getWindow(), "destroy",
                         G_CALLBACK(SysTray::onWindowDestroy), this);

        // Start timezone timer if needed
        if (config->show_timezones) {
            unsigned int interval = 1000;
            if (config->clock_format.find("%S") == std::string::npos) {
                interval = 30 * 1000;
            }
            timer_id = g_timeout_add(interval,
                                     (GSourceFunc)SysTray::onTimeHandler, this);
        }
        // Window is already shown by MainWindow constructor
        return;
    }

    if (gtk_widget_get_visible(GTK_WIDGET(window->getWindow()))) {
        hideWindow();
    } else {
        showWindow();
    }
}

void SysTray::showWindow()
{
    if (!window) {
        return;
    }
    // Reposition window near the cursor
    Config* config = Config::getInstance();
    GtkWidget* w = GTK_WIDGET(window->getWindow());
    gtk_window_set_position(GTK_WINDOW(w), config->mainwindow_position);
    gtk_widget_show(w);

    gint xpos, ypos;
    gtk_window_get_position(GTK_WINDOW(w), &xpos, &ypos);
    gtk_window_move(GTK_WINDOW(w),
                    config->mainwindow_xoffset + xpos,
                    config->mainwindow_yoffset + ypos);

    gtk_window_present(GTK_WINDOW(w));
}

void SysTray::hideWindow()
{
    if (!window) {
        return;
    }
    gtk_widget_hide(GTK_WIDGET(window->getWindow()));
}

void SysTray::onActivate(GtkStatusIcon* icon, gpointer data)
{
    static_cast<SysTray*>(data)->toggle();
}

void SysTray::onPopupMenu(GtkStatusIcon* icon, guint button,
                           guint activate_time, gpointer data)
{
    GtkWidget* menu = gtk_menu_new();
    GtkWidget* quit_item = gtk_menu_item_new_with_label("Quit");
    g_signal_connect(quit_item, "activate",
                     G_CALLBACK(SysTray::onQuit), data);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit_item);
    gtk_widget_show_all(menu);

#if HAVE_GTK_BOX_NEW
    gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
#else
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
                   gtk_status_icon_position_menu, icon,
                   button, activate_time);
#endif
}

void SysTray::onQuit(GtkMenuItem* item, gpointer data)
{
    gtk_main_quit();
}

gboolean SysTray::onFocusOut(GtkWidget* widget, GdkEvent* event,
                              gpointer data)
{
    static_cast<SysTray*>(data)->hideWindow();
    return TRUE;
}

gboolean SysTray::onMouseLeave(GtkWidget* widget, GdkEventCrossing* event,
                                gpointer data)
{
    GdkRectangle allocation;
    gtk_widget_get_allocation(widget, &allocation);
    if (event->x <= allocation.x ||
        event->x >= allocation.x + allocation.width ||
        event->y <= allocation.y ||
        event->y >= allocation.y + allocation.height) {
        static_cast<SysTray*>(data)->hideWindow();
    }
    return TRUE;
}

gboolean SysTray::onTimeHandler(gpointer data)
{
    SysTray* self = static_cast<SysTray*>(data);
    if (self->window) {
        self->window->updateTime();
    }
    return TRUE;
}

void SysTray::onWindowDestroy(GtkWidget* widget, gpointer data)
{
    // Safety handler: if the WM force-destroys the window,
    // NULL out our pointer so next toggle creates a fresh one.
    SysTray* self = static_cast<SysTray*>(data);
    if (self->timer_id) {
        g_source_remove(self->timer_id);
        self->timer_id = 0;
    }
    self->window = NULL;
}

#pragma GCC diagnostic pop
