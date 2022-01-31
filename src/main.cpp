#include <Arduino.h>
#include <WiFi.h>
#include "FLIPDOTS.h"

const char *ssid = "Skynet";
const char *password = "";
const char *ntpServer = "pool.ntp.org";

FLIPDOTS display(&Serial2);
struct tm *timeInfo;

void displayTest(void *params)
{
    byte buffer[7] = {0};
    while (true)
    {
        for (uint8_t i = 0; i < 7; i++)
        {
            buffer[i] = 1 << i;
        }
        display.write(buffer);
        digitalWrite(LED_BUILTIN, HIGH);
        vTaskDelay(1000);
        for (uint8_t i = 0; i < 7; i++)
        {
            buffer[i] = ~(1 << i);
        }
        display.write(buffer);
        digitalWrite(LED_BUILTIN, LOW);
        vTaskDelay(1000);
    }
}

void displayError()
{
    const byte[] error = {
        0b00000000,
        0b00001000,
        0b00001000,
        0b00001000,
        0b00000000,
        0b00001000,
        0b00000000};
    display.write(error);
}

void taskDisplayLoader(void *params)
{
    uint8_t type = (long)params;
    byte buffer[7] = {0};
    switch (type)
    {
    case 1:
        while (true)
        {
            buffer[4] = 0;
            buffer[2] = 0b00001000;
            display.write(buffer);
            vTaskDelay(250);
            buffer[2] = 0;
            buffer[3] = 0b00001000;
            display.write(buffer);
            vTaskDelay(250);
            buffer[3] = 0;
            buffer[4] = 0b00001000;
            display.write(buffer);
            vTaskDelay(250);
        }
    case 0:
    default:
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

bool connectWiFiAndConfigTime(TaskHandle_t *showLoaderTask)
{
    // Show loading screen 1
    xTaskCreate(taskDisplayLoader, "taskDisplayLoader", 1024, (void *)0, 1, showLoaderTask);

    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    digitalWrite(LED_BUILTIN, HIGH);
    WiFi.begin(ssid, password);
    long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 10000)
    {
        vTaskDelay(500);
    }
    if (WiFi.status() != WL_CONNECTED) // Couldn't connect to WiFi
    {
        return false;
    }

    // Show loading screen 2
    vTaskDelete(showLoaderTask);
    xTaskCreate(taskDisplayLoader, "taskDisplayLoader", 1024, (void *)1, 1, showLoaderTask);

    // Configure time
    uint8_t tryCount = 4;
    while (!getLocalTime(timeInfo) && tryCount--)
    {
        configTime(-28800, 0, ntpServer);
    }
    if (!getLocalTime(timeInfo)) // Couldn't configure time
    {
        return false;
    }

    // Cleanup
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    digitalWrite(LED_BUILTIN, LOW);
}

// the setup function runs once when you press reset or power the board
void setup()
{
    timeInfo = new tm();
    pinMode(LED_BUILTIN, OUTPUT);
    display.begin();
    // xTaskCreate(displayTest, "displayTest", 1024, NULL, 1, NULL);
    //display.write3x3char4('A', 'B', 'C', 'D');
    //xTaskCreate(taskConnectWiFiAndConfigTime, "taskConnectWiFiAndConfigTime", 1024, NULL, 1, NULL);
    TaskHandle_t showLoaderTask;
    if (!connectWiFiAndConfigTime(&showLoaderTask))
    {
        displayError();
        while (1)
            ;
    }
    char timeStr[] = "0000";
    strftime(timeStr, sizeof(timeStr), "%I%M", timeInfo);
    display.write3x3char4(timeStr[0], timeStr[1], timeStr[2], timeStr[3]);
}

// the loop function runs over and over again forever
void loop()
{
    vTaskSuspend(NULL);
}