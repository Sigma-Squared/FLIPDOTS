#include <Arduino.h>
#include <WiFi.h>
#include "FLIPDOTS.h"
#include "GOL.h"

#define DEBUG 1
#define WIFI_TIMEOUT 10000
#define NTP_CONFIGTIME_TRYCOUNT 4
#define UPDATE_EVERY_SECOND 0

const char *ssid = "Skynet";
const char *password = "";
const char *ntpServer = "pool.ntp.org";

FLIPDOTS display(&Serial2);
struct tm *timeInfo;

void displayError()
{
    const byte errorDisplay[] = {
        0b00000000,
        0b00001000,
        0b00001000,
        0b00001000,
        0b00000000,
        0b00001000,
        0b00000000};
    display.write(errorDisplay);
}

bool displayTime()
{
    time_t now;
    time(&now);
    localtime_r(&now, timeInfo);
    if (timeInfo->tm_year <= (2016 - 1900))
    {
        return false;
    }
    char timeStr[] = "0000";
    strftime(timeStr, sizeof(timeStr), "%I%M", timeInfo);
#if DEBUG
    Serial.printf("Time: %s\n", timeStr);
#endif
    display.write3x3char4(timeStr);
    return true;
}

void taskDisplayLoader(void *params)
{
    uint8_t type = (long)params;
    switch (type)
    {
    case 1:
    {
        byte buffer[7] = {0};
        const byte frames[][3] = {
            {0b010, 0b010, 0b000},
            {0b100, 0b010, 0b000},
            {0b000, 0b110, 0b000},
            {0b000, 0b010, 0b100},
            {0b000, 0b010, 0b010},
            {0b000, 0b010, 0b001},
            {0b000, 0b011, 0b000},
            {0b001, 0b010, 0b000},
        };
        for (uint8_t i = 0;; i++)
        {
            uint8_t f = i % 8;
            buffer[2] = frames[f][0] << 2;
            buffer[3] = frames[f][1] << 2;
            buffer[4] = frames[f][2] << 2;
            display.write(buffer);
            vTaskDelay(125);
        }
    }
    case 2:
    {
        byte buffer[7] = {0};
        while (true)
        {
            display.write(buffer);
            GOL(buffer);
            vTaskDelay(250);
        }
    }
    case 0:
    default:
    {
        byte buffer[7] = {0};
        while (true)
        {
            buffer[3] = 0b00010000;
            display.write(buffer);
            vTaskDelay(250);
            buffer[3] = buffer[3] >> 1;
            display.write(buffer);
            vTaskDelay(250);
            buffer[3] = buffer[3] >> 1;
            display.write(buffer);
            vTaskDelay(250);
        }
    }
    }
}

void taskUpdateClock(void *params)
{
    auto initialDelay = (uint32_t)params;
#if DEBUG
    Serial.printf("Initial delay: %ims\n", initialDelay);
#endif
    vTaskDelay(initialDelay);

    TickType_t lastWakeTime = xTaskGetTickCount();
    while (true)
    {
        displayTime();
        vTaskDelayUntil(&lastWakeTime, UPDATE_EVERY_SECOND ? 1000 : 60000);
    }
}

bool connectWiFiAndConfigTime()
{
    // Show loading screen 1
    TaskHandle_t showLoaderTask;
    xTaskCreate(taskDisplayLoader, "taskDisplayLoader", 1024, (void *)2, 1, &showLoaderTask);

    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    digitalWrite(LED_BUILTIN, HIGH);
#if DEBUG
    Serial.print("Connecting Wifi...");
#endif
    WiFi.begin(ssid, password);
    long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_TIMEOUT)
    {
        delay(500);
    }
    if (WiFi.status() != WL_CONNECTED) // Couldn't connect to WiFi
    {
#if DEBUG
        Serial.println("Failed.");
#endif
        vTaskDelete(showLoaderTask);
        return false;
    }
#if DEBUG
    Serial.println("Connected.");
#endif

    // Show loading screen 2
    //vTaskDelete(showLoaderTask);
    //xTaskCreate(taskDisplayLoader, "taskDisplayLoader", 1024, (void *)1, 1, &showLoaderTask);

    // Configure time
#if DEBUG
    Serial.print("Configuring time...");
#endif
    uint8_t tryCount = NTP_CONFIGTIME_TRYCOUNT;
    while (!getLocalTime(timeInfo) && tryCount--)
    {
        configTime(-28800, 0, ntpServer);
    }
    if (!getLocalTime(timeInfo)) // Couldn't configure time
    {
        vTaskDelete(showLoaderTask);
#if DEBUG
        Serial.println("Failed");
#endif
        return false;
    }
#if DEBUG
    Serial.println("Done.");
#endif
    // Cleanup
    vTaskDelete(showLoaderTask);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    digitalWrite(LED_BUILTIN, LOW);
    return true;
}

void setup()
{
#if DEBUG
    Serial.begin(115200);
    Serial.setDebugOutput(true);
#endif
    timeInfo = new tm();
    pinMode(LED_BUILTIN, OUTPUT);
    display.begin();

    if (!connectWiFiAndConfigTime())
    {
        displayError();
        while (1)
            ;
    }

    display.setInverted(true);
    display.clear();
    display.setInverted(false);
    delay(250);

#if UPDATE_EVERY_SECOND
    uint32_t initialDelay = 0;
#else
    displayTime();
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint32_t msOffset = tv_now.tv_usec / 1000; // adjust for edge variance;
    uint32_t initialDelay = (59 - timeInfo->tm_sec) * 1000 + msOffset;
#endif
    xTaskCreate(taskUpdateClock, "taskUpdateClock", 2048, (void *)initialDelay, 1, NULL);
}

void loop()
{
    vTaskSuspend(NULL);
}