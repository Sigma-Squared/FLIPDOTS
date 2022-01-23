#include <HTTPClient.h>
#include <ArduinoJSON.h>

String httpGet(HTTPClient &http, const char *url);
String httpGet(HTTPClient &http, String url);
