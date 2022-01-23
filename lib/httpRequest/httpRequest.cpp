#include "httpRequest.h"

String httpGet(HTTPClient &http, const char *url)
{
    http.begin(url);

    int responseCode = http.GET();
    String payload = "";
    if (responseCode < 200 || responseCode >= 300)
    {
        Serial.printf("%d on HTTP request: to %s\n", responseCode, url);
        return "";
    }
    payload = http.getString();
    http.end();
    return payload;
}

String httpGet(HTTPClient &http, String url)
{
    return httpGet(http, url.c_str());
}