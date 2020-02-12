#include <forms.h>
#include <stdlib.h>
#include <stdio.h>
#include <MQTTClient.h>
#include <pthread.h>
#include <signal.h>
#include "main.h"
#include <curl/curl.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define PAYLOAD "HelloWorld"
#define TOPIC "ping"

MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
FD_main *mainForm;
pthread_mutex_t formMutex = PTHREAD_MUTEX_INITIALIZER;

volatile time_t turnedOnAtTime = 0;

FL_FORM *dialogForm = NULL;
char* dialogText;

FL_OBJECT *upnextText = NULL;

typedef struct {
    char * ptr;
    size_t len;
    size_t max;
} fake_string;

fake_string * upnextRespString;

static void dismiss_cb(FL_OBJECT *obj, long data)
{
    if(dialogForm) {
        fl_hide_form(dialogForm);
        dialogForm = NULL;
        free(dialogText);
    }
}

void do_redraw_dialog(int x, void* data) {
    if(dialogForm) {
        fl_redraw_form(dialogForm);
    }
}
void showDialog(char* message, int messageLength) {
    if(dialogForm != NULL) {
        dismiss_cb(NULL, 0);
    }

    dialogText = (char*) malloc(messageLength + 1);
    memcpy(dialogText, message, messageLength);
    dialogText[messageLength] = 0;
    printf("Showing dialog w/ text: %s\n", dialogText);
    dialogForm = fl_bgn_form(FL_UP_BOX, 700, 400);
    FL_OBJECT* buttonObj;
    buttonObj = fl_add_text(FL_NORMAL_TEXT, 30, 30, 707, 100, dialogText);
    fl_set_object_lsize( buttonObj, FL_HUGE_SIZE );
    buttonObj = fl_add_button(FL_NORMAL_BUTTON, 280, 290, 150, 80, "Dismiss");
    fl_set_object_lsize( buttonObj, FL_LARGE_SIZE );
    fl_set_object_callback(buttonObj, dismiss_cb, 0);
    
    fl_end_form();
    //L:D_N:dialog_ID:formtest
    fl_show_form(dialogForm, FL_PLACE_CENTERFREE, FL_FULLBORDER, "L:D_N:dialog_ID:formtester");
    fl_redraw_form(dialogForm);
    fl_add_timeout(500, do_redraw_dialog, NULL);
}


void daemonise() {
    // Fork, allowing the parent process to terminate.
    pid_t pid = fork();
    if (pid == -1) {
        _exit(1);
    } else if (pid != 0) {
        _exit(0);
    }

    // Start a new session for the daemon.
    if (setsid()==-1) {
        _exit(1);
    }

    // Fork again, allowing the parent process to terminate.
    signal(SIGHUP,SIG_IGN);
    pid=fork();
    if (pid == -1) {
        _exit(1);
    } else if (pid != 0) {
        _exit(0);
    }

    // Set the current working directory to the root directory.
    if (chdir("/") == -1) {
        _exit(1);
    }

    // Set the user file creation mask to zero.
    umask(0);

    // Close then reopen standard file descriptors.
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    if (open("/dev/null",O_RDONLY) == -1) {
        _exit(1);
    }
    if (open("/dev/null",O_WRONLY) == -1) {
        _exit(1);
    }
    if (open("/dev/null",O_RDWR) == -1) {
        _exit(1);
    }
}

void do_curl_post(const char* url, const char* jsonstr) {
    CURL *curl = curl_easy_init();
    if(curl) {
        CURLcode res;
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "charsets: utf-8");
        headers = curl_slist_append(headers, "Authorization: Bearer " BEARER_TOKEN);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonstr);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(jsonstr));

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, fake_string *s)
{
  size_t copy_len;
  size_t new_len = s->len + size*nmemb;
  copy_len = size*nmemb;
  if(new_len > s->max) {
    copy_len = s->max - (s->len + size*nmemb) - 1;
  }

  memcpy(s->ptr+s->len, ptr, copy_len);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}

void do_curl_post_retval(const char* url, const char* jsonstr, fake_string* retval) {
    CURL *curl = curl_easy_init();
    if(curl) {
        CURLcode res;
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "charsets: utf-8");
        headers = curl_slist_append(headers, "Authorization: Bearer "BEARER_TOKEN);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonstr);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(jsonstr));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, retval);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
}

void update_upcoming(int x, void* data) {
    upnextRespString->len = 0;
    upnextRespString->ptr[0] = '\0';
    do_curl_post_retval("http://10.102.40.20:9123/api/template", "{\"template\":\"{{ states('input_text.calendarevents') }}\"}", upnextRespString);

    for (size_t i = 0; i < upnextRespString->len-1; ++i)
    {
        if (upnextRespString->ptr[i] == '\\' && upnextRespString->ptr[i+1] == 'n') {

            upnextRespString->ptr[i] = ' ';
            upnextRespString->ptr[i+1] = '\n';
        }
    }
    fl_set_object_label(upnextText, upnextRespString->ptr);
    fl_add_timeout(300000, update_upcoming, NULL);
}

void open_cover(const char* jsonstr) {
    do_curl_post("http://10.102.40.20:9123/api/services/cover/open_cover", jsonstr);
}

void close_cover(const char* jsonstr) {
    do_curl_post("http://10.102.40.20:9123/api/services/cover/close_cover", jsonstr);
}

void start_scening(const char* jsonstr) {
    do_curl_post("http://10.102.40.20:9123/api/services/scene/turn_on", jsonstr);
}

void light_on(const char* jsonstr) {
    do_curl_post("http://10.102.40.20:9123/api/services/light/turn_on", jsonstr);
}

void light_off(const char* jsonstr) {
    do_curl_post("http://10.102.40.20:9123/api/services/light/turn_off", jsonstr);
}

void light_toggle(const char* jsonstr) {
    do_curl_post("http://10.102.40.20:9123/api/services/light/toggle", jsonstr);
}

void switch_toggle(const char* jsonstr) {
    do_curl_post("http://10.102.40.20:9123/api/services/switch/toggle", jsonstr);
}

void mp_on(const char* jsonstr) {
    do_curl_post("http://10.102.40.20:9123/api/services/media_player/turn_on", jsonstr);
}

void mp_off(const char* jsonstr) {
    do_curl_post("http://10.102.40.20:9123/api/services/media_player/turn_off", jsonstr);
}

void inpbool_on(const char* jsonstr) {
    do_curl_post("http://10.102.40.20:9123/api/services/input_boolean/turn_on", jsonstr);
}

void turnoff_backlight() {
      const char* onfull = "0";
      #ifdef __arm__
        int fd = open("/sys/class/backlight/max77696-bl/brightness", O_WRONLY);
        if (fd > 0) {
            write (fd, onfull, 1);
            close(fd);
            printf("turnedoff\n");
        }
      #else 
        printf("turnedoff\n");
      #endif
      
}

void check_turnoff_backlight(int x, void* data) {
    //fl_remove_timeout(x);
    printf("%lu %lu\n", (turnedOnAtTime + 9), time(NULL));
    if((turnedOnAtTime + 9) < time(NULL)) {
        turnoff_backlight();
    }

}

void turnon_backlight() {
      const char* onfull = "600";
      #ifdef __arm__
        int fd = open("/sys/class/backlight/max77696-bl/brightness", O_WRONLY);
        if (fd > 0) {
            int wrote = write (fd, onfull, 3);
            if(wrote < 0) {
                perror("error writing: ");
            }
            close(fd);
            turnedOnAtTime = time(NULL);
            printf("turnedon: %d\n", wrote);
        }
      #else
        turnedOnAtTime = time(NULL);
        
        printf("turnedon\n");
      #endif
    fl_add_timeout(10000, check_turnoff_backlight, NULL);
}


/**
 * Xforms callbacks
 * */
void living_room_state( FL_OBJECT *obj, long val ) {
    
    const char* fullstr = "{\"entity_id\": \"scene.living_room_on_full\"}";
    const char* dimstr = "{\"entity_id\": \"scene.living_room_on_dim\"}";
    const char* offstr = "{\"entity_id\": \"scene.living_room_off\"}";

    if (val == 0) {
        start_scening(offstr);
    }
    else if (val == 1) {
        start_scening(dimstr);
    }
    else {
        start_scening(fullstr);
    }
}

void rgbstrip_color( FL_OBJECT *obj, long val ) {
    char* offstr;
    switch(val) {
	case 3:
	   offstr = "{\"entity_id\": \"light.extended_color_light_1\", \"brightness\": 255, \"rgb_color\": [0,0,255]}";
	   break;
	case 2:
	   offstr = "{\"entity_id\": \"light.extended_color_light_1\", \"brightness\": 255, \"rgb_color\": [0,255,0]}";
	   break;
	case 1:
	   offstr = "{\"entity_id\": \"light.extended_color_light_1\", \"brightness\": 255, \"rgb_color\": [255,0,0]}";
	   break;
	case 0:
	default:
	   offstr = "{\"entity_id\": \"light.extended_color_light_1\", \"brightness\": 255, \"color_temp\": 500}";
    }
    light_on(offstr);
}

void rgbstrip_off( FL_OBJECT *obj, long val ) {
    const char* offstr = "{\"entity_id\": \"light.extended_color_light_1\"}";
    light_off(offstr);
}

void dining_room_state( FL_OBJECT *obj, long val ) {
    const char* fullstr = "{\"entity_id\": \"light.dining_room\", \"brightness\":255}";
    const char* offstr = "{\"entity_id\": \"light.dining_room\"}";
    light_toggle(offstr);
}

void scene_night( FL_OBJECT *obj, long val ) {
    const char* offstr = "{\"entity_id\": \"scene.night_time\"}";
    start_scening(offstr);
}

void scene_movies( FL_OBJECT *obj, long val ) {
    const char* offstr = "{\"entity_id\": \"scene.living_room_movies\"}";
    start_scening(offstr);
}

void curtains_open( FL_OBJECT *obj, long val ) {
    const char* offstr = "{\"entity_id\": \"cover.living_room_curtains\"}";
    open_cover(offstr);
}

void curtains_close( FL_OBJECT *obj, long val ) {
    const char* offstr = "{\"entity_id\": \"cover.living_room_curtains\"}";
    close_cover(offstr);
}


void kitchen_state( FL_OBJECT *obj, long val ) {
    const char* offstr = "{\"entity_id\": \"switch.leviton_dz15s1bz_decora_smart_switch_switch\"}";
    switch_toggle(offstr);
}

void stove_state( FL_OBJECT *obj, long val ) {
    const char* fullstr = "{\"entity_id\": \"light.kitchen\", \"brightness\":255}";
    const char* offstr = "{\"entity_id\": \"light.kitchen\"}";
    light_toggle(offstr);
}

void tv_on( FL_OBJECT *obj, long val ) {
    const char* mpstr = "{\"entity_id\": \"media_player.tv\"}";
    mp_on(mpstr);
}
void tv_off( FL_OBJECT *obj, long val ) {
    const char* mpstr = "{\"entity_id\": \"media_player.tv\"}";
    mp_off(mpstr);
}

void tv_chromecast( FL_OBJECT *obj, long val ) {
    inpbool_on("{\"entity_id\": \"input_boolean.switch_chromecast\"}");
}
void tv_switch( FL_OBJECT *obj, long val ) {
    inpbool_on("{\"entity_id\": \"input_boolean.switch_switch\"}");
}

void exit_button( FL_OBJECT *obj, long val ) {
    exit(0);
}

void backlight_on( FL_OBJECT * obj, long val) {
    turnon_backlight();
}

void do_exit(int x, void* data) {
    exit(0);
}

int main(int argc, char **argv)
{
    #ifdef __arm__
    daemonise();
    #endif

    upnextRespString = malloc(sizeof(fake_string));
    upnextRespString->ptr = malloc(1025);
    upnextRespString->max = 1024;
    upnextRespString->len = 0;


    XInitThreads();
    //FL_FORM *form;
    fl_initialize(&argc, argv, 0, 0, 0);
    
    //FL_IOPT fl_cntl;
    //fl_cntl.buttonFontSize = 24;
    //fl_cntl.labelFontSize = 24;
    //fl_set_defaults(FL_PDButtonFontSize | FL_PDLabelFontSize, &fl_cntl);

    FD_main *mainForm = create_form_main();

    upnextText = mainForm->upnext_label;
    //form = fl_bgn_form(FL_UP_BOX, 230, 100);
    //fl_add_button(FL_NORMAL_BUTTON, 20, 20, 190, 60, "Hello world");
    //fl_add_slider(FL_VERT_NICE_SLIDER, 0, 0, 30, 190, "BRGHT");
    //fl_end_form();
    pthread_mutex_lock(&formMutex);
    
    fl_show_form(mainForm->main, FL_PLACE_CENTER, FL_FULLBORDER, "L:A_N:application_ID:formtester");


    //pthread_mutex_unlock(&formMutex);
    update_upcoming(0, NULL);
    while(1) {
	   fl_do_forms();
    }

    fl_hide_form(mainForm->main);
    fl_finish();
    free(upnextRespString->ptr);
    free(upnextRespString);
    return 0;
}

