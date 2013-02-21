#include "e.h"

/* local subsystem functions */

static Eina_Bool
_e_popup_autoclose_deskafter_show_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   E_Event_Desk_After_Show *ev = event;
   E_Gadcon_Popup *pop = data;

   if (!pop) return ECORE_CALLBACK_RENEW;
   if (!pop->win->visible) return ECORE_CALLBACK_RENEW;
   if (!e_gadcon_client_visible_get(pop->gcc, ev->desk))
     e_object_del(E_OBJECT(pop));

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_popup_autoclose_border_fullscreen_cb(void *data, int type EINA_UNUSED, void *event)
{
   E_Event_Border_Fullscreen *ev = event;
   E_Gadcon_Popup *pop = data;

   if (!pop) return ECORE_CALLBACK_RENEW;
   if (!pop->win->visible) return ECORE_CALLBACK_RENEW;
   if (!ev->border->fullscreen) return ECORE_CALLBACK_RENEW;
   if (e_gadcon_client_visible_get(pop->gcc, ev->border->desk))
     e_object_del(E_OBJECT(pop));
   return ECORE_CALLBACK_RENEW;
}

static void
_e_gadcon_popup_locked_set(E_Gadcon_Popup *pop, Eina_Bool locked)
{
   if (!pop->gcc)
     return;

   e_gadcon_locked_set(pop->gcc->gadcon, locked);
   pop->gadcon_was_locked = locked;
}

static void
_e_gadcon_popup_free(E_Gadcon_Popup *pop)
{
   E_FN_DEL(ecore_event_handler_del, pop->autoclose_handlers[0]);
   E_FN_DEL(ecore_event_handler_del, pop->autoclose_handlers[1]);

   if (pop->gadcon_was_locked)
     _e_gadcon_popup_locked_set(pop, 0);
   pop->gcc = NULL;
   evas_object_del(pop->content);
   if (pop->win)
     E_OBJECT_DEL_SET(pop->win, NULL);
   E_FN_DEL(e_object_del, pop->win);
   free(pop);
}

static void
_e_gadcon_popup_del_cb(void *obj)
{
   E_Gadcon_Popup *pop;

   pop = e_object_data_get(obj);
   pop->win = NULL;
   e_object_del(E_OBJECT(pop));
}

static void
_e_gadcon_popup_position(E_Gadcon_Popup *pop)
{
   Evas_Coord gx = 0, gy = 0, gw, gh, zw, zh, zx, zy, px, py;

   /* Popup positioning */
   e_gadcon_client_geometry_get(pop->gcc, &gx, &gy, &gw, &gh);
   zx = pop->win->zone->x;
   zy = pop->win->zone->y;
   zw = pop->win->zone->w;
   zh = pop->win->zone->h;
   switch (pop->gcc->gadcon->orient)
     {
      case E_GADCON_ORIENT_CORNER_RT:
      case E_GADCON_ORIENT_CORNER_RB:
      case E_GADCON_ORIENT_RIGHT:
        px = gx - pop->w;
        py = gy;
        if (py + pop->h >= (zy + zh))
          py = gy + gh - pop->h;
        px = MIN(zx + zw - gw - pop->w - 3, px);
        break;

      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_CORNER_LB:
        px = gx + gw;
        py = gy;
        if (py + pop->h >= (zy + zh))
          py = gy + gh - pop->h;
        px = MIN(zx + zw - gw - pop->w + 3, px);
        break;

      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
        py = gy + gh;
        px = (gx + (gw / 2)) - (pop->w / 2);
        if ((px + pop->w) >= (zx + zw))
          px = gx + gw - pop->w;
        else if (px < zx)
          px = zx;
        py = MIN(zy + zh - gh - pop->h + 3, py);
        break;

      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
        py = gy - pop->h;
        px = (gx + (gw / 2)) - (pop->w / 2);
        if ((px + pop->w) >= (zx + zw))
          px = gx + gw - pop->w;
        else if (px < zx)
          px = zx;
        py = MIN(zy + zh - gh - pop->h - 3, py);
        break;

      case E_GADCON_ORIENT_FLOAT:
        px = (gx + (gw / 2)) - (pop->w / 2);
        if (gy >= (zy + (zh / 2)))
          py = gy - pop->h;
        else
          py = gy + gh;
        if ((px + pop->w) >= (zx + zw))
          px = gx + gw - pop->w;
        else if (px < zx)
          px = zx;
        break;

      default:
        e_popup_move(pop->win, 50, 50);
        return;
     }
   if (px - zx < 0)
     px = zx;
   if (py - zy < 0)
     py = zy;
   e_popup_move(pop->win, px - zx, py - zy);

   if (pop->gadcon_lock && (!pop->gadcon_was_locked))
     _e_gadcon_popup_locked_set(pop, 1);
}

static void
_e_gadcon_popup_size_recalc(E_Gadcon_Popup *pop, Evas_Object *obj)
{
   Evas_Coord w = 0, h = 0;

   e_widget_size_min_get(obj, &w, &h);
   if ((!w) || (!h)) evas_object_size_hint_min_get(obj, &w, &h);
   if ((!w) || (!h))
     {
        edje_object_size_min_get(obj, &w, &h);
        edje_object_size_min_restricted_calc(obj, &w, &h, w, h);
     }
   edje_extern_object_min_size_set(obj, w, h);
   edje_object_size_min_calc(pop->o_bg, &pop->w, &pop->h);
   e_popup_resize(pop->win, pop->w, pop->h);

   if (pop->win->visible)
     _e_gadcon_popup_position(pop);
}

static void
_e_gadcon_popup_changed_size_hints_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   E_Gadcon_Popup *pop;

   pop = data;
   _e_gadcon_popup_size_recalc(pop, obj);
}

/* externally accessible functions */

EAPI E_Gadcon_Popup *
e_gadcon_popup_new(E_Gadcon_Client *gcc)
{
   E_Gadcon_Popup *pop;
   Evas_Object *o;
   E_Zone *zone;

   pop = E_OBJECT_ALLOC(E_Gadcon_Popup, E_GADCON_POPUP_TYPE, _e_gadcon_popup_free);
   if (!pop) return NULL;
   zone = e_gadcon_client_zone_get(gcc);
   pop->win = e_popup_new(zone, 0, 0, 0, 0);
   e_object_data_set(E_OBJECT(pop->win), pop);
   E_OBJECT_DEL_SET(pop->win, _e_gadcon_popup_del_cb);

   o = edje_object_add(pop->win->evas);
   e_theme_edje_object_set(o, "base/theme/gadman", "e/gadman/popup");
   pop->o_bg = o;

   pop->gcc = gcc;
   pop->gadcon_lock = 1;
   pop->gadcon_was_locked = 0;

   return pop;
}

EAPI void
e_gadcon_popup_content_set(E_Gadcon_Popup *pop, Evas_Object *o)
{
   Evas_Object *old_o;

   if (!pop) return;
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_GADCON_POPUP_TYPE);

   old_o = edje_object_part_swallow_get(pop->o_bg, "e.swallow.content");
   if (old_o != o)
     {
        if (old_o)
          {
             edje_object_part_unswallow(pop->o_bg, old_o);
             evas_object_del(old_o);
          }
        edje_object_part_swallow(pop->o_bg, "e.swallow.content", o);
        evas_object_event_callback_add(o, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _e_gadcon_popup_changed_size_hints_cb, pop);
     }
   pop->content = o;
   _e_gadcon_popup_size_recalc(pop, o);
}

EAPI void
e_gadcon_popup_show(E_Gadcon_Popup *pop)
{
   if (!pop) return;
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_GADCON_POPUP_TYPE);

   if (pop->win->visible) return;

   _e_gadcon_popup_position(pop);
   pop->autoclose_handlers[0] = ecore_event_handler_add(E_EVENT_DESK_AFTER_SHOW, _e_popup_autoclose_deskafter_show_cb, NULL);
   pop->autoclose_handlers[1] = ecore_event_handler_add(E_EVENT_BORDER_FULLSCREEN, _e_popup_autoclose_border_fullscreen_cb, NULL);
   e_popup_content_set(pop->win, pop->o_bg);
   e_popup_show(pop->win);
}

EAPI void
e_gadcon_popup_hide(E_Gadcon_Popup *pop)
{
   if (!pop) return;
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_GADCON_POPUP_TYPE);
   if (pop->pinned) return;
   e_popup_hide(pop->win);
   if (pop->gadcon_was_locked)
     _e_gadcon_popup_locked_set(pop, 0);
}

EAPI void
e_gadcon_popup_toggle_pinned(E_Gadcon_Popup *pop)
{
   if (!pop) return;
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_GADCON_POPUP_TYPE);

   if (pop->pinned)
     {
        pop->pinned = 0;
        edje_object_signal_emit(pop->o_bg, "e,state,unpinned", "e");
     }
   else
     {
        pop->pinned = 1;
        edje_object_signal_emit(pop->o_bg, "e,state,pinned", "e");
     }
}

EAPI void
e_gadcon_popup_lock_set(E_Gadcon_Popup *pop, Eina_Bool setting)
{
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_GADCON_POPUP_TYPE);

   setting = !!setting;
   if (pop->gadcon_lock == setting) return;
   pop->gadcon_lock = setting;

   if (setting != pop->gadcon_was_locked)
     _e_gadcon_popup_locked_set(pop, setting);
}
