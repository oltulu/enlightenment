#include "private.h"

#define MUSIC_CONTROL_DOMAIN "module.music_control"

static E_Module *music_control_mod = NULL;

static Eina_Bool was_playing_before_lock = EINA_FALSE;

static const char _e_music_control_Name[] = "Music controller";

const Player music_player_players[] =
{
   {"gmusicbrowser", "org.mpris.MediaPlayer2.gmusicbrowser"},
   {"Banshee", "org.mpris.MediaPlayer2.banshee"},
   {"Audacious", "org.mpris.MediaPlayer2.audacious"},
   {"VLC", "org.mpris.MediaPlayer2.vlc"},
   {"BMP", "org.mpris.MediaPlayer2.bmp"},
   {"XMMS2", "org.mpris.MediaPlayer2.xmms2"},
   {NULL, NULL}
};

Eina_Bool
_desklock_cb(void *data, int type, void *ev)
{
   E_Music_Control_Module_Context *ctxt;
   E_Event_Desklock *event;

   ctxt = data;
   event = ev;

   /* Lock with music on. Pause it */
   if (event->on && ctxt->playing)
     {
        media_player2_player_play_pause_call(ctxt->mpris2_player);
        was_playing_before_lock = EINA_TRUE;
        return ECORE_CALLBACK_DONE;
     }

   /* Lock without music. Keep music off as state */
   if (event->on && (!ctxt->playing))
     {
        was_playing_before_lock = EINA_FALSE;
        return ECORE_CALLBACK_DONE;
     }

   /* Unlock with music pause and playing before lock. Turn it back on */
   if ((!event->on) && (!ctxt->playing) && was_playing_before_lock)
     media_player2_player_play_pause_call(ctxt->mpris2_player);

   return ECORE_CALLBACK_DONE;
}

static void
_music_control(E_Object *obj, const char *params)
{
   E_Music_Control_Module_Context *ctxt;
   EINA_SAFETY_ON_NULL_RETURN(music_control_mod->data);
   ctxt = music_control_mod->data;
   if (!strcmp(params, "play"))
     media_player2_player_play_pause_call(ctxt->mpris2_player);
   else if (!strcmp(params, "next"))
     media_player2_player_next_call(ctxt->mpris2_player);
   else if (!strcmp(params, "previous"))
     media_player2_player_previous_call(ctxt->mpris2_player);
}

#define ACTION_NEXT "next_music"
#define ACTION_NEXT_NAME "Next Music"
#define ACTION_PLAY_PAUSE "playpause_music"
#define ACTION_PLAY_PAUSE_NAME "Play/Pause Music"
#define ACTION_PREVIOUS "previous_music"
#define ACTION_PREVIOUS_NAME "Previous Music"

static void
_actions_register(E_Music_Control_Module_Context *ctxt)
{
   E_Action *action;
   if (ctxt->actions_set) return;

   action = e_action_add(ACTION_NEXT);
   action->func.go = _music_control;
   e_action_predef_name_set(_e_music_control_Name, ACTION_NEXT_NAME,
                            ACTION_NEXT, "next", NULL, 0);

   action = e_action_add(ACTION_PLAY_PAUSE);
   action->func.go = _music_control;
   e_action_predef_name_set(_e_music_control_Name, ACTION_PLAY_PAUSE_NAME,
                            ACTION_PLAY_PAUSE, "play", NULL, 0);

   action = e_action_add(ACTION_PREVIOUS);
   action->func.go = _music_control;
   e_action_predef_name_set(_e_music_control_Name, ACTION_PREVIOUS_NAME,
                            ACTION_PREVIOUS, "previous", NULL, 0);

   ctxt->actions_set = EINA_TRUE;
}

static void
_actions_unregister(E_Music_Control_Module_Context *ctxt)
{
   if (!ctxt->actions_set) return;
   e_action_predef_name_del(ACTION_NEXT_NAME, ACTION_NEXT);
   e_action_del(ACTION_NEXT);
   e_action_predef_name_del(ACTION_PLAY_PAUSE_NAME, ACTION_PLAY_PAUSE);
   e_action_del(ACTION_PLAY_PAUSE);
   e_action_predef_name_del(ACTION_PREVIOUS_NAME, ACTION_PREVIOUS);
   e_action_del(ACTION_PREVIOUS);

   ctxt->actions_set = EINA_FALSE;
}

/* Gadcon Api Functions */
static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   E_Music_Control_Instance *inst;
   E_Music_Control_Module_Context *ctxt;

   EINA_SAFETY_ON_NULL_RETURN_VAL(music_control_mod, NULL);
   ctxt = music_control_mod->data;

   inst = calloc(1, sizeof(E_Music_Control_Instance));
   inst->ctxt = ctxt;
   inst->gadget = edje_object_add(gc->evas);
   e_theme_edje_object_set(inst->gadget, "base/theme/modules/music-control",
                           "modules/music-control/main");

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->gadget);
   inst->gcc->data = inst;

   evas_object_event_callback_add(inst->gadget, EVAS_CALLBACK_MOUSE_DOWN, music_control_mouse_down_cb, inst);

   ctxt->instances = eina_list_append(ctxt->instances, inst);
   _actions_register(ctxt);

   return inst->gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   E_Music_Control_Instance *inst;
   E_Music_Control_Module_Context *ctxt;

   EINA_SAFETY_ON_NULL_RETURN(music_control_mod);

   ctxt = music_control_mod->data;
   inst = gcc->data;

   evas_object_del(inst->gadget);
   if (inst->popup) music_control_popup_del(inst);
   ctxt->instances = eina_list_remove(ctxt->instances, inst);
   if (!ctxt->instances)
     _actions_unregister(ctxt);

   free(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class)
{
   return _e_music_control_Name;
}

static char tmpbuf[1024]; /* general purpose buffer, just use immediately */

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas)
{
   Evas_Object *o;
   EINA_SAFETY_ON_NULL_RETURN_VAL(music_control_mod, NULL);
   snprintf(tmpbuf, sizeof(tmpbuf), "%s/e-module-music-control.edj",
            e_module_dir_get(music_control_mod));
   o = edje_object_add(evas);
   edje_object_file_set(o, tmpbuf, "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class)
{
   E_Music_Control_Module_Context *ctxt;
   EINA_SAFETY_ON_NULL_RETURN_VAL(music_control_mod, NULL);
   ctxt = music_control_mod->data;

   snprintf(tmpbuf, sizeof(tmpbuf), "music-control.%d",
            eina_list_count(ctxt->instances));
   return tmpbuf;
}

static const E_Gadcon_Client_Class _gc_class =
{
   GADCON_CLIENT_CLASS_VERSION, "music-control",
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
      e_gadcon_site_is_not_toolbar
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, _e_music_control_Name };

static void
cb_playback_status_get(void *data, EDBus_Pending *p, const char *propname, EDBus_Proxy *proxy, EDBus_Error_Info *error_info, const char *value)
{
   E_Music_Control_Module_Context *ctxt = data;

   if (error_info)
     {
        ERR("%s %s", error_info->error, error_info->message);
        return;
     }

   if (!strcmp(value, "Playing"))
     ctxt->playing = EINA_TRUE;
   else
     ctxt->playing = EINA_FALSE;
   music_control_state_update_all(ctxt);
}

static void
prop_changed(void *data, EDBus_Proxy *proxy, void *event_info)
{
   EDBus_Proxy_Event_Property_Changed *event = event_info;
   E_Music_Control_Module_Context *ctxt = data;

   if (!strcmp(event->name, "PlaybackStatus"))
     {
        const Eina_Value *value = event->value;
        const char *status;

        eina_value_get(value, &status);
        if (!strcmp(status, "Playing"))
          ctxt->playing = EINA_TRUE;
        else
          ctxt->playing = EINA_FALSE;
        music_control_state_update_all(ctxt);
     }
}

Eina_Bool
music_control_dbus_init(E_Music_Control_Module_Context *ctxt, const char *bus)
{
   edbus_init();
   ctxt->conn = edbus_connection_get(EDBUS_CONNECTION_TYPE_SESSION);
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctxt->conn, EINA_FALSE);

   ctxt->mrpis2 = mpris_media_player2_proxy_get(ctxt->conn, bus, NULL);
   ctxt->mpris2_player = media_player2_player_proxy_get(ctxt->conn, bus, NULL);
   media_player2_player_playback_status_propget(ctxt->mpris2_player, cb_playback_status_get, ctxt);
   edbus_proxy_event_callback_add(ctxt->mpris2_player, EDBUS_PROXY_EVENT_PROPERTY_CHANGED,
                                  prop_changed, ctxt);
   return EINA_TRUE;
}

EAPI void *
e_modapi_init(E_Module *m)
{
   E_Music_Control_Module_Context *ctxt;

   ctxt = calloc(1, sizeof(E_Music_Control_Module_Context));
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctxt, NULL);
   music_control_mod = m;

   ctxt->conf_edd = E_CONFIG_DD_NEW("music_control_config", Music_Control_Config);
   #undef T
   #undef D
   #define T Music_Control_Config
   #define D ctxt->conf_edd
   E_CONFIG_VAL(D, T, player_selected, INT);
   E_CONFIG_VAL(D, T, pause_on_desklock, INT);
   ctxt->config = e_config_domain_load(MUSIC_CONTROL_DOMAIN, ctxt->conf_edd);
   if (!ctxt->config)
     ctxt->config = calloc(1, sizeof(Music_Control_Config));

   if (!music_control_dbus_init(ctxt, music_player_players[ctxt->config->player_selected].dbus_name))
     goto error_dbus_bus_get;
   music_control_mod = m;

   e_gadcon_provider_register(&_gc_class);

   if (ctxt->config->pause_on_desklock)
     desklock_handler = ecore_event_handler_add(E_EVENT_DESKLOCK, _desklock_cb, ctxt);

   return ctxt;

error_dbus_bus_get:
   free(ctxt);
   return NULL;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   E_Music_Control_Module_Context *ctxt;
   EINA_SAFETY_ON_NULL_RETURN_VAL(music_control_mod, 0);
   ctxt = music_control_mod->data;

   free(ctxt->config);
   E_CONFIG_DD_FREE(ctxt->conf_edd);

   E_FREE_FUNC(desklock_handler, ecore_event_handler_del);

   media_player2_player_proxy_unref(ctxt->mpris2_player);
   mpris_media_player2_proxy_unref(ctxt->mrpis2);
   edbus_connection_unref(ctxt->conn);
   edbus_shutdown();

   e_gadcon_provider_unregister(&_gc_class);

   if (eina_list_count(ctxt->instances))
     ERR("Live instances.");

   free(ctxt);
   music_control_mod = NULL;

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   E_Music_Control_Module_Context *ctxt = m->data;
   e_config_domain_save(MUSIC_CONTROL_DOMAIN, ctxt->conf_edd, ctxt->config);
   return 1;
}
