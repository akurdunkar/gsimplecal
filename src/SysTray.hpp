#ifndef SYSTRAY_HPP
#define SYSTRAY_HPP

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <gtk/gtk.h>

class MainWindow;

class SysTray
{
public:
    SysTray();
    ~SysTray();

    MainWindow* getWindow();
    void toggle();

private:
    GtkStatusIcon* tray_icon;
    MainWindow* window;
    guint timer_id;

    void showWindow();
    void hideWindow();

    static void onActivate(GtkStatusIcon* icon, gpointer data);
    static void onPopupMenu(GtkStatusIcon* icon, guint button,
                            guint activate_time, gpointer data);
    static void onQuit(GtkMenuItem* item, gpointer data);
    static gboolean onFocusOut(GtkWidget* widget, GdkEvent* event,
                               gpointer data);
    static gboolean onMouseLeave(GtkWidget* widget, GdkEventCrossing* event,
                                 gpointer data);
    static gboolean onTimeHandler(gpointer data);
    static void onWindowDestroy(GtkWidget* widget, gpointer data);
};

#pragma GCC diagnostic pop

#endif
