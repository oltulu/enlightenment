#define E_COMP_WL
#include "e.h"

#define COMPOSITOR_VERSION 3

#define E_COMP_WL_PIXMAP_CHECK \
   if (e_pixmap_type_get(ec->pixmap) != E_PIXMAP_TYPE_WL) return

/* local functions */
static void 
_e_comp_wl_log_cb_print(const char *format, va_list args)
{
   INF("WL: ", format, args);
}

static void 
_e_comp_wl_compositor_cb_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
   E_Comp *comp;
   struct wl_resource *res;

   if (!(comp = data)) return;

   if (!(res = 
         wl_resource_create(client, &wl_compositor_interface, 
                            MIN(version, COMPOSITOR_VERSION), id)))
     {
        ERR("Could not create compositor resource: %m");
        wl_client_post_no_memory(client);
        return;
     }
}

static void 
_e_comp_wl_compositor_cb_del(E_Comp *comp)
{
   E_Comp_Data *cdata;

   /* get existing compositor data */
   if (!(cdata = comp->wl_comp_data)) return;

   /* free allocated data structure */
   free(cdata);
}

static Eina_Bool 
_e_comp_wl_compositor_create(void)
{
   E_Comp *comp;
   const char *name;

   /* check for existing compositor. create if needed */
   if (!(comp = e_comp_get(NULL)))
     {
        comp = e_comp_new();
        comp->comp_type = E_PIXMAP_TYPE_WL;
        E_OBJECT_DEL_SET(comp, _e_comp_wl_compositor_cb_del);
     }

   /* create new compositor data */
   cdata = E_NEW(E_Comp_Data, 1);

   /* set wayland log handler */
   wl_log_set_handler_server(_e_comp_wl_log_cb_print);

   /* try to create a wayland display */
   if (!(cdata->wl.disp = wl_display_create()))
     {
        ERR("Could not create a Wayland display: %m");
        goto disp_err;
     }

   /* try to setup wayland socket */
   if (!(name = wl_display_add_socket_auto(cdata->wl.disp)))
     {
        ERR("Could not create Wayland display socket: %m");
        goto sock_err;
     }

   /* set wayland display environment variable */
   e_env_set("WAYLAND_DISPLAY", name);

   /* initialize compositor signals */
   wl_signal_init(&cdata->signals.surface.create);
   wl_signal_init(&cdata->signals.surface.activate);
   wl_signal_init(&cdata->signals.surface.kill);

   /* try to add compositor to wayland globals */
   if (!wl_global_create(cdata->wl.disp, &wl_compositor_interface, 
                         COMPOSITOR_VERSION, comp, 
                         _e_comp_wl_compositor_cb_bind))
     {
        ERR("Could not add compositor to wayland globals: %m");
        goto comp_global_err;
     }

   /* initialize shm mechanism */
   wl_display_init_shm(cdata->wl.disp);

   return EINA_TRUE;

comp_global_err:
   e_env_unset("WAYLAND_DISPLAY");
sock_err:
   wl_display_destroy(cdata->wl.disp);
disp_err:
   free(cdata);
   return EINA_FALSE;
}

/* public functions */
EAPI Eina_Bool 
e_comp_wl_init(void)
{
   /* set gl available if we have ecore_evas support */
   if (ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_WAYLAND_EGL) || 
       ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_OPENGL_DRM))
     e_comp_gl_set(EINA_TRUE);

   /* try to create a wayland compositor */
   if (!_e_comp_wl_compositor_create())
     {
        e_error_message_show(_("Enlightenment cannot create a Wayland Compositor!\n"));
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

EAPI struct wl_signal 
e_comp_wl_surface_create_signal_get(E_Comp *comp)
{
   return comp->wl_comp_data->signals.surface.create;
}

/* internal functions */
EINTERN void 
e_comp_wl_shutdown(void)
{

}

EINTERN struct wl_resource *
e_comp_wl_surface_create(struct wl_client *client, int version, uint32_t id)
{
   return NULL;
}
