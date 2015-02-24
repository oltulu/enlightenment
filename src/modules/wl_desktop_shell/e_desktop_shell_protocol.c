#include <stdlib.h>
#include <stdint.h>
#include <wayland-util.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wredundant-decls"

extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_seat_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface xdg_popup_interface;
extern const struct wl_interface xdg_surface_interface;

#pragma GCC diagnostic pop

static const struct wl_interface *types[] =
{
   NULL,
   NULL,
   NULL,
   NULL,
   &xdg_surface_interface,
   &wl_surface_interface,
   &xdg_popup_interface,
   &wl_surface_interface,
   &wl_surface_interface,
   &wl_seat_interface,
   NULL,
   NULL,
   NULL,
   NULL,
   &wl_surface_interface,
   &wl_seat_interface,
   NULL,
   NULL,
   NULL,
   &wl_seat_interface,
   NULL,
   &wl_seat_interface,
   NULL,
   NULL,
   &wl_output_interface,
};

static const struct wl_message xdg_shell_requests[] =
{
   { "use_unstable_version", "i", types + 0 },
   { "get_xdg_surface", "no", types + 4 },
   { "get_xdg_popup", "nooouiiu", types + 6 },
   { "pong", "u", types + 0 },
};

static const struct wl_message xdg_shell_events[] =
{
   { "ping", "u", types + 0 },
};

WL_EXPORT const struct wl_interface xdg_shell_interface =
{
   "xdg_shell", 1,
   4, xdg_shell_requests,
   1, xdg_shell_events,
};

static const struct wl_message xdg_surface_requests[] =
{
   { "destroy", "", types + 0 },
   { "set_parent", "?o", types + 14 },
   { "set_title", "s", types + 0 },
   { "set_app_id", "s", types + 0 },
   { "show_window_menu", "ouii", types + 15 },
   { "move", "ou", types + 19 },
   { "resize", "ouu", types + 21 },
   { "ack_configure", "u", types + 0 },
   { "set_window_geometry", "iiii", types + 0 },
   { "set_maximized", "", types + 0 },
   { "unset_maximized", "", types + 0 },
   { "set_fullscreen", "?o", types + 24 },
   { "unset_fullscreen", "", types + 0 },
   { "set_minimized", "", types + 0 },
};

static const struct wl_message xdg_surface_events[] =
{
   { "configure", "iiau", types + 0 },
   { "close", "", types + 0 },
};

WL_EXPORT const struct wl_interface xdg_surface_interface =
{
   "xdg_surface", 1,
   14, xdg_surface_requests,
   2, xdg_surface_events,
};

static const struct wl_message xdg_popup_requests[] =
{
   { "destroy", "", types + 0 },
};

static const struct wl_message xdg_popup_events[] =
{
   { "popup_done", "u", types + 0 },
};

WL_EXPORT const struct wl_interface xdg_popup_interface =
{
   "xdg_popup", 1,
   1, xdg_popup_requests,
   1, xdg_popup_events,
};
