#include <pebble.h>

#include "strings.h"

String* string_split(String to_split, char delim) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "to_split: %s | delim: %c", to_split, delim);
    int split_count = 0;
    
    for (char* char_ref = to_split; *char_ref != '\0'; char_ref++) {
        if (*char_ref == delim) {
            split_count++;
        }
    }
    // size of array is the amount of times delim is seen plus one for final element and one for NULL terminator
    String* parts = (String*) malloc((split_count + 2) * sizeof(String));

    int part_size = 0;
    int part_index = 0;
    String copy_from = to_split;
    for (char* char_ref = to_split; *char_ref != '\0'; char_ref++) {
        if (*char_ref == delim) {
            parts[part_index] = (String) malloc((part_size + 1) * sizeof(char));
            strncpy(parts[part_index], copy_from, part_size);
            parts[part_index][part_size] = '\0';

            copy_from = char_ref + 1;
            part_size = 0;
            part_index++;
        } else if (*(char_ref+1) == '\0') {
            part_size++;
            parts[part_index] = (String) malloc((part_size + 1) * sizeof(char));
            strncpy(parts[part_index], copy_from, part_size);
            parts[part_index][part_size] = '\0';
        } else {
            part_size++;
        }
    }
    
    parts[split_count+1] = NULL;

    return parts;
}

String app_message_enum_to_string(AppMessageResult result) {
    switch (result) {
        case APP_MSG_OK: return "APP_MSG_OK";
        case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
        case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
        case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
        case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
        case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
        case APP_MSG_BUSY: return "APP_MSG_BUSY";
        case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
        case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
        case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
        case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
        case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
        case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
        case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
        default: return "UNKNOWN ERROR";
    }
}
