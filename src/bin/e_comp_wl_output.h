#ifdef E_TYPEDEFS
#else
# ifndef E_COMP_WL_OUTPUT_H
#  define E_COMP_WL_OUTPUT_H

Eina_Bool e_comp_wl_output_init(void);
void e_comp_wl_output_shutdown(void);
Wayland_Output *e_comp_wl_output_get(void);

# endif
#endif
