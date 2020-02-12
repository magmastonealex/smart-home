/* Header file generated by fdesign on Wed Feb 12 16:10:15 2020 */

#ifndef FD_main_h_
#define FD_main_h_

#include <forms.h>

#if defined __cplusplus
extern "C"
{
#endif

/* Callbacks, globals and object handlers */

void backlight_on( FL_OBJECT *, long );
void living_room_state( FL_OBJECT *, long );
void dining_room_state( FL_OBJECT *, long );
void stove_state( FL_OBJECT *, long );
void tv_on( FL_OBJECT *, long );
void tv_off( FL_OBJECT *, long );
void tv_chromecast( FL_OBJECT *, long );
void tv_switch( FL_OBJECT *, long );
void scene_night( FL_OBJECT *, long );
void kitchen_state( FL_OBJECT *, long );
void exit_button( FL_OBJECT *, long );
void scene_movies( FL_OBJECT *, long );
void curtains_open( FL_OBJECT *, long );
void curtains_close( FL_OBJECT *, long );
void rgbstrip_color( FL_OBJECT *, long );
void rgbstrip_off( FL_OBJECT *, long );


/* Forms and Objects */

typedef struct {
    FL_FORM   * main;
    void      * vdata;
    char      * cdata;
    long        ldata;
    FL_OBJECT * all_container;
    FL_OBJECT * living_room_on;
    FL_OBJECT * living_room_dim;
    FL_OBJECT * living_room_off;
    FL_OBJECT * dining_room_on;
    FL_OBJECT * stove_light_on;
    FL_OBJECT * tv_on;
    FL_OBJECT * tv_off;
    FL_OBJECT * tv_chromecast;
    FL_OBJECT * tv_switch;
    FL_OBJECT * upnext_label;
} FD_main;

FD_main * create_form_main( void );

#if defined __cplusplus
}
#endif

#endif /* FD_main_h_ */
