// #include <Arduino.h>
// #include <WiFi.h>
// #include "time.h"
// #include "httpRequest.h"
// #include <ArduinoJson.h>

// const char *ssid = "Skynet";
// const char *password = "29F4C6D5E4";

// HTTPClient http;

// const char *ntpServer = "pool.ntp.org";

// struct TzOffset
// {
//   int tz_offset;
//   int dst_offset;
// };

// TzOffset getTzOffsetByIp(String ip)
// {
//   TzOffset tzOffset;
//   tzOffset.tz_offset = 0;
//   tzOffset.dst_offset = 0;

//   String timeData = httpGet(http, "http://worldtimeapi.org/api/ip/" + ip);

//   StaticJsonDocument<512> timeDataJson;
//   auto error = deserializeJson(timeDataJson, timeData.c_str());

//   if (error)
//   {
//     Serial.printf("Failed to parse timezone data: %s", error.c_str());
//     return tzOffset;
//   }

//   tzOffset.tz_offset = timeDataJson["raw_offset"].as<int>();
//   tzOffset.dst_offset = timeDataJson["dst_offset"].as<int>();
//   return tzOffset;
// }

// void configureTimeWithNetwork()
// {
//   String publicIp = httpGet(http, "http://api.ipify.org/");
//   TzOffset tzOffset = getTzOffsetByIp(publicIp);
//   Serial.printf("Timzeone offset: %i Daylight savings offset: %i\n", tzOffset.tz_offset, tzOffset.dst_offset);

//   //init and get the time
//   configTime(tzOffset.tz_offset, tzOffset.dst_offset, ntpServer);
// }

// void printLocalTime()
// {
//   struct tm timeinfo;
//   if (!getLocalTime(&timeinfo))
//   {
//     Serial.println("Failed to obtain time. Retry network configure...");
//     configureTimeWithNetwork();
//     return;
//   }
//   Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
// }

// void setup()
// {
//   Serial.begin(115200);

//   //connect to WiFi
//   Serial.printf("Connecting to %s ", ssid);
//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED)
//   {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.print('\n');

//   configureTimeWithNetwork();
//   printLocalTime();

//   //disconnect WiFi as it's no longer needed
//   WiFi.disconnect(true);
//   WiFi.mode(WIFI_OFF);
// }

// void loop()
// {
//   delay(1000);
//   printLocalTime();
// }