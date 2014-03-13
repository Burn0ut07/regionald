#include <pebble.h>

typedef char* String;

String* string_split(String to_split, char delim);

String app_message_enum_to_string(AppMessageResult result);
