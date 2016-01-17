/*
* Copyright (c) 2011-2012 Research In Motion Limited.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <assert.h>
#include <bps/bps.h>
#include <bps/event.h>
#include <bps/navigator.h>
#include <bps/screen.h>
#include <fcntl.h>
#include <screen/screen.h>
#include <bps/dialog.h>     //Include for displaying a dialog box
#include <stdio.h>
#include <stdlib.h>



static bool shutdown;

static void
handle_screen_event(bps_event_t *event)
{
    int screen_val;

    screen_event_t screen_event = screen_event_get_event(event);
    screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_TYPE, &screen_val);

    switch (screen_val) {
    case SCREEN_EVENT_MTOUCH_TOUCH:
        fprintf(stderr,"Touch event");
        break;
    case SCREEN_EVENT_MTOUCH_MOVE:
        fprintf(stderr,"Move event");
        break;
    case SCREEN_EVENT_MTOUCH_RELEASE:
        fprintf(stderr,"Release event");
        break;
    default:
        break;
    }
    fprintf(stderr,"\n");
}

static void
handle_navigator_event(bps_event_t *event) {
    switch (bps_event_get_code(event)) {
    case NAVIGATOR_SWIPE_DOWN:
        fprintf(stderr,"Swipe down event");
        break;
    case NAVIGATOR_EXIT:
        fprintf(stderr,"Exit event");
        shutdown = true;
        break;
    default:
        break;
    }
    fprintf(stderr,"\n");
}

static void
handle_event()
{
    int rc, domain;

    bps_event_t *event = NULL;
    rc = bps_get_event(&event, -1);
    assert(rc == BPS_SUCCESS);
    if (event) {
        domain = bps_event_get_domain(event);
        if (domain == navigator_get_domain()) {
            handle_navigator_event(event);
        } else if (domain == screen_get_domain()) {
            handle_screen_event(event);
        }
    }
}

/**
 * Show an alert dialog that has two buttons: a "Cancel" button, and a "Submit" button.
 */
static void
show_alert()
{
    if (alert_dialog) {
        return;
    }

    if (dialog_create_alert(&alert_dialog) != BPS_SUCCESS) {
        fprintf(stderr, "Failed to create alert dialog.");
        return;
    }

    if (dialog_set_alert_message_text(alert_dialog, "Hello Tobias, Daniel en Olivier!") != BPS_SUCCESS) {
        fprintf(stderr, "Failed to set alert dialog message text.");
        dialog_destroy(alert_dialog);
        alert_dialog = 0;
        return;
    }

    /*
     * Create a context to attach to the cancel button
     */
    char* cancel_button_context = "Annuleren";

    /*
     * Use a button label defined in bps/dialog.h. Attach a context to the button.
     */
    if (dialog_add_button(alert_dialog, DIALOG_CANCEL_LABEL, true, cancel_button_context, true) != BPS_SUCCESS) {
        fprintf(stderr, "Failed to add button to alert dialog.");
        dialog_destroy(alert_dialog);
        alert_dialog = 0;
        return;
    }

    /*
     * Use a button label of our own. Don't attach a context to the button.
     */
    if (dialog_add_button(alert_dialog, "Ga maar door", true, 0, true) != BPS_SUCCESS) {
        fprintf(stderr, "Failed to add button to alert dialog.");
        dialog_destroy(alert_dialog);
        alert_dialog = 0;
        return;
    }

    if (dialog_show(alert_dialog) != BPS_SUCCESS) {
        fprintf(stderr, "Failed to show alert dialog.");
        dialog_destroy(alert_dialog);
        alert_dialog = 0;
        return;
    }
}

/**
 * Handle a dialog response.
 */
static void
handle_dialog_response(bps_event_t *event)
{
    /*
     * Double check that the event is valid
     */
    if (event == NULL) {
        return;
    }

    int selectedIndex = dialog_event_get_selected_index(event);
    const char* label = dialog_event_get_selected_label(event);
    const char* context = dialog_event_get_selected_context(event);

    char output[1024];
    snprintf(output, 1024, "Selected Index: %d, Label: %s, Context: %s\n",
            selectedIndex, label?label:"n/a", context?(char*)context:"n/a");
    fprintf(stderr, output);

    dialog_destroy(alert_dialog);
    alert_dialog = 0;
}




int
main(int argc, char **argv)
{
    const int usage = SCREEN_USAGE_NATIVE;

    screen_context_t screen_ctx;
    screen_window_t screen_win;
    screen_buffer_t screen_buf = NULL;
    int rect[4] = { 0, 0, 0, 0 };

    /* Setup the window */
    screen_create_context(&screen_ctx, 0);
    screen_create_window(&screen_win, screen_ctx);
    screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_USAGE, &usage);
    screen_create_window_buffers(screen_win, 1);

    screen_get_window_property_pv(screen_win, SCREEN_PROPERTY_RENDER_BUFFERS, (void **)&screen_buf);
    screen_get_window_property_iv(screen_win, SCREEN_PROPERTY_BUFFER_SIZE, rect+2);

    /* Fill the screen buffer with blue */
    int attribs[] = { SCREEN_BLIT_COLOR, 0xff0000ff, SCREEN_BLIT_END };
    screen_fill(screen_ctx, screen_buf, attribs);
    screen_post_window(screen_win, screen_buf, 1, rect, 0);

    /* Signal bps library that navigator and screen events will be requested */
    /*
       * Before we can listen for events from the BlackBerry Tablet OS platform
       * services, we need to initialize the BPS infrastructure
    */
    bps_initialize();
    screen_request_events(screen_ctx);
    /*
        * Once the BPS infrastructure has been initialized we can register for events from the various BlackBerry Tablet OS platform services. The
        * Navigator service manages and delivers application life cycle and visibility events. For this sample, we request Navigator events so that we can track when
        * the system is terminating the application (NAVIGATOR_EXIT event), and as a convenient way to trigger the display of a dialog (NAVIGATOR_SWIPE_DOWN).
        * We request dialog events so we can be notified when the dialog service responds to our requests/queries.
        */
    navigator_request_events(0);
    dialog_request_events(0);    //dialog events are requested so that we can be notified.

    /*
      * start the application with a dialog.
      */
     show_alert();


    while (!shutdown) {
        /* Handle user input */
        handle_event();
    }

    /* Clean up */
    screen_stop_events(screen_ctx);
    bps_shutdown();
    screen_destroy_window(screen_win);
    screen_destroy_context(screen_ctx);
    return 0;
}

