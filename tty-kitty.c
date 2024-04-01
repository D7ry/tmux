
#include <sys/types.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <ctype.h>
#include <limits.h>
#include <resolv.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "tmux.h"

#define REVERT_DIAMBIGUATE_ESCAPE_CODES 1

/*
 *
 * modifiers
shift     0b1         (1)
alt       0b10        (2)
ctrl      0b100       (4)
super     0b1000      (8)
hyper     0b10000     (16)
meta      0b100000    (32)
caps_lock 0b1000000   (64)
num_lock  0b10000000  (128) 

*/
#define MOD_CTRL 5

int
kitty_disambiguate_escape_codes(struct tty *tty, const char *buf, size_t len, size_t *size, key_code *key)
{
    // buf starts with \033[ and ends with a character u

    // look for u
    int u_index = -1;
    int semilcolon_index = -1;
    // start from 2 because we know the first two characters are \033[
    for (int i = 2; i < len; i++) {
        if (buf[i] == ';') {
            semilcolon_index = i;
        }
        if (buf[i] == 'u') {
            u_index = i;
            break;
        }
    }
    if (u_index == -1 || semilcolon_index == -1) {
        return 1;
    }

    if (u_index == semilcolon_index) {
        return 1;
    }

    *size = u_index + 1;

    int modifier = 0;
    int modifier_size = u_index - semilcolon_index - 1;
    char modifier_str [4];
    memcpy(modifier_str , buf + semilcolon_index + 1,modifier_size);
    modifier_str[modifier_size] = '\0';
    modifier = atoi(modifier_str);

    if (REVERT_DIAMBIGUATE_ESCAPE_CODES) {
        // only get the part before the semicolon
        char keycode_str[16];
        int key_size = semilcolon_index - 2;
        memcpy(keycode_str, buf + 2, key_size);
        keycode_str[key_size] = '\0';
        int keycode = atoi(keycode_str);
        //
        switch (modifier) {
            case MOD_CTRL:
                // regular terminal ctrl keycodes
                if ((keycode >= 'a' && keycode <= 'z') || (keycode >= 'A' && keycode <= 'Z')) {
                    keycode = keycode - 'a' + 1;
                }
                break;
        }

        
        *key = keycode;
    } else {
    }

    return 0;
}

int
tty_keys_kitty(struct tty *tty, const char *buf, size_t len, size_t *size, key_code *key)
{
    log1("[tty_keys_kitty] keys are (%.*s) (%llu)", (int)len, buf, *key);
    
    // Check kitty features TODO: actually check for kitty

	/* First two bytes are always \033[. */
	if (buf[0] != '\033')
		return (-1);
	if (len == 1)
		return (1);
	if (buf[1] != '[')
		return (-1);
	if (len == 2)
		return (1);

    switch (kitty_disambiguate_escape_codes(tty, buf, len, size, key)) {
        case 0: // valid
            return 0;
        case -1: // not valid
            break;
        case 1: // partial, return partial
            return 1;
    }

    return -1;
}

// int 
// tty_kitty_keys_next(struct tty *tty)
// {
//     
//     log_open("here");
// 	struct client		*c = tty->client;
// 	struct timeval		 tv;
// 	const char		*buf;
// 	size_t			 len, size;
// 	cc_t			 bspace;
// 	int			 delay, expired = 0, n;
// 	key_code		 key = KEYC_UNKNOWN;
// 	struct mouse_event	 m = { 0 };
// 	struct key_event	*event;
//
// 	/* Get key buffer. */
// 	buf = EVBUFFER_DATA(tty->in);
// 	len = EVBUFFER_LENGTH(tty->in);
// 	if (len == 0)
// 		return (0);
//
//
// 	/* Is this a clipboard response? */
//     //TODO: not sure if we need this
// 	switch (tty_keys_clipboard(tty, buf, len, &size)) {
// 	case 0:		/* yes */
// 		key = KEYC_UNKNOWN;
// 		goto complete_key;
// 	case -1:	/* no, or not valid */
// 		break;
// 	case 1:		/* partial */
// 		goto partial_key;
// 	}
//
// 	/* Is this a primary device attributes response? */
// 	switch (tty_keys_device_or_keyboard_protocol_attributes(tty, buf, len, &size)) {
// 	case 0:		/* yes */
// 		key = KEYC_UNKNOWN;
// 		goto complete_key;
// 	case -1:	/* no, or not valid */
// 		break;
// 	case 1:		/* partial */
// 		goto partial_key;
// 	}
//
// 	/* Is this a secondary device attributes response? */
// 	switch (tty_keys_device_attributes2(tty, buf, len, &size)) {
// 	case 0:		/* yes */
// 		key = KEYC_UNKNOWN;
// 		goto complete_key;
// 	case -1:	/* no, or not valid */
// 		break;
// 	case 1:		/* partial */
// 		goto partial_key;
// 	}
//
// 	/* Is this an extended device attributes response? */
// 	switch (tty_keys_extended_device_attributes(tty, buf, len, &size)) {
// 	case 0:		/* yes */
// 		key = KEYC_UNKNOWN;
// 		goto complete_key;
// 	case -1:	/* no, or not valid */
// 		break;
// 	case 1:		/* partial */
// 		goto partial_key;
// 	}
//
// 	/* Is this a colours response? */
// 	switch (tty_keys_colours(tty, buf, len, &size)) {
// 	case 0:		/* yes */
// 		key = KEYC_UNKNOWN;
// 		goto complete_key;
// 	case -1:	/* no, or not valid */
// 		break;
// 	case 1:		/* partial */
// 		goto partial_key;
// 	}
//
//
// first_key:
//     log1("[tty_kitty_keys_next] keys are (%.*s)", (int)len, buf);
//     
// 	/* First two bytes are always \033[. */
// 	if (buf[0] != '\033')
//         goto first_key_temp_done;
// 	if (len == 1)
//         goto first_key_temp_done;
// 	if (buf[1] != '[')
//         goto first_key_temp_done;
// 	if (len == 2)
//         goto first_key_temp_done;
//
//     /* Look for ; character */
//     int keycode_begin = 2;
//     int keycode_end = keycode_begin;
//     while (keycode_end < len && buf[keycode_end] != ';') {
//         keycode_end++;
//     }
//     if (keycode_end == len) {
//         goto first_key_temp_done;
//     }
//
//     /* Get the key code */
//     char keycode_str[16];
//     memcpy(keycode_str, buf + keycode_begin, keycode_end - keycode_begin);
//     keycode_str[keycode_end - keycode_begin] = '\0';
//     int keycode = atoi(keycode_str);
//     log1("[tty_kitty_keys_next] keycode: %d", keycode);
//     if (keycode >= 0 && keycode < 256) {
//         log1("[tty_kitty_keys_next] key: %c", (char)keycode);
//         key = (key_code)keycode;
//     }
//     
//     if (keycode_end == len) {
//         goto first_key_temp_done;
//     }
//
// first_key_temp_done:
//     size = len; // read the whole thing lmao TODO: not read the whole thing
//     goto complete_key;
//
// partial_key:
// 	log_debug("%s: partial key %.*s", c->name, (int)len, buf);
//
// 	log1("%s: partial key %.*s", c->name, (int)len, buf);
// 	/* If timer is going, check for expiration. */
// 	if (tty->flags & TTY_TIMER) {
// 		if (evtimer_initialized(&tty->key_timer) &&
// 		    !evtimer_pending(&tty->key_timer, NULL)) {
// 			expired = 1;
// 			goto first_key; // deal with the key as it is complete
// 		}
// 		return (0);
// 	}
//
// 	/* Get the time period. */
// 	delay = options_get_number(global_options, "escape-time");
// 	if (delay == 0)
// 		delay = 1;
// 	tv.tv_sec = delay / 1000;
// 	tv.tv_usec = (delay % 1000) * 1000L;
//
// 	/* Start the timer. */
// 	if (event_initialized(&tty->key_timer))
// 		evtimer_del(&tty->key_timer);
// 	evtimer_set(&tty->key_timer, tty_keys_callback, tty);
// 	evtimer_add(&tty->key_timer, &tv);
//
// 	tty->flags |= TTY_TIMER;
// 	return (0);
//
// complete_key:
// 	/* Remove data from buffer. */
// 	evbuffer_drain(tty->in, size);
//
// 	/* Remove key timer. */
// 	if (event_initialized(&tty->key_timer))
// 		evtimer_del(&tty->key_timer);
// 	tty->flags &= ~TTY_TIMER;
//
// 	/* Check for focus events. */
// 	if (key == KEYC_FOCUS_OUT) {
// 		c->flags &= ~CLIENT_FOCUSED;
// 		window_update_focus(c->session->curw->window);
// 		notify_client("client-focus-out", c);
// 	} else if (key == KEYC_FOCUS_IN) {
// 		c->flags |= CLIENT_FOCUSED;
// 		notify_client("client-focus-in", c);
// 		window_update_focus(c->session->curw->window);
// 	}
//
// 	/* Fire the key. */
// 	if (key != KEYC_UNKNOWN) {
// 		event = xmalloc(sizeof *event);
// 		event->key = key;
// 		memcpy(&event->m, &m, sizeof event->m);
// 		if (!server_client_handle_key(c, event))
// 			free(event);
// 	}
//
// 	return (1);
// }



