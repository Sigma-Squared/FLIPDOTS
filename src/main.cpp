#include <Arduino.h>
#include <WiFi.h>
#include <BluetoothSerial.h>
#include <Preferences.h>
#include "FLIPDOTS.h"
#include "GOL.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define DEBUG 0
#define WIFI_TIMEOUT 10000
#define BT_TIMEOUT 10000
#define NTP_CONFIGTIME_TRYCOUNT 4
#define SSID_MAXLEN 32
#define WIFI_PASS_MAXLEN 64
#define MINUTE_EDGE_CHECKPERIOD 100

#define LOADINGSCREEN_GENERIC 0
#define LOADINGSCREEN_BLUETOOTH 1
#define LOADINGSCREEN_GLIDER 2

#define CONFIGURED_VIA_BT 0
#define CONFIGURED_VIA_NVS 1

FLIPDOTS display(&Serial2);
BluetoothSerial SerialBT;
Preferences preferences;
struct tm *timeInfo;

void displayError()
{
    const byte errorDisplay[] = {
        0b00000000,
        0b00001000,
        0b00000000,
        0b00001000,
        0b00001000,
        0b00001000,
        0b00000000};
    display.write(errorDisplay);
}

bool getTime()
{
    time_t now;
    time(&now);
    localtime_r(&now, timeInfo);
    if (timeInfo->tm_year <= (2016 - 1900)) // time is not valid
        return false;
    return true;
}

bool displayTime()
{
    if (!getTime())
    {
        displayError();
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
    case LOADINGSCREEN_BLUETOOTH:
    {
        byte buffer[7] = {0};
        byte frame[7] = {
            0b00000000,
            0b00001000,
            0b00001100,
            0b00001000,
            0b00001100,
            0b00001000,
            0b00000000};
        while (true)
        {
            display.write(frame);
            vTaskDelay(250);
            display.write(buffer);
            vTaskDelay(250);
        };
    }
    case LOADINGSCREEN_GLIDER:
    {
        byte buffer[7] = {
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00110000,
            0b01010000,
            0b00010000};
        while (true)
        {
            display.write(buffer);
            GOL(buffer);
            vTaskDelay(250);
        }
    }
    case LOADINGSCREEN_GENERIC:
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
    // Initial delay lines up the task so it executes exactly when the next minute changes. Then, it runs periodically every minute.
    // The initial delay is very important, if it's off by an amount, every single update will be off by the same amount.
    // It's also important the the task initally fires AFTER the minute has changed, otherwise it wil read the time milliseconds
    // before it's about to change, resulting in the clock being always a minute behind.
    getTime();
    uint8_t oldMinute = timeInfo->tm_min;
    // wait for minute to change
    while (timeInfo->tm_min == oldMinute)
    {
        vTaskDelay(MINUTE_EDGE_CHECKPERIOD);
        getTime();
    }

    // we are now synchronized with minute change, run every 60s
    TickType_t lastWakeTime = xTaskGetTickCount();
    while (true)
    {
        displayTime();
        vTaskDelayUntil(&lastWakeTime, 60000);
    }
}

void cleanInput(char *input)
{
    // remove leading and trailing whitespace
    uint len = strlen(input);
    uint start = 0;
    while (isspace(input[start++]))
        ;
    memmove(input, input + start - 1, len - start + 2);
    for (uint end = len - 1; isspace(input[end]); end--)
    {
        input[end] = '\0';
    }
}

uint8_t getCredentialsViaBluetoothOrNVS(char *ssid, char *password, int *gmtOffset, int *daylightOffset)
{
    // Attempt to receive WiFI credentials via Bluetooth
#if DEBUG
    Serial.print("Connecting Bluetooth.");
#endif
    SerialBT.begin("FLIPDOTS Clock");
    long startTime = millis();
    while (!SerialBT.connected() && (millis() - startTime) < BT_TIMEOUT)
    {
        delay(500);
    }
    if (SerialBT.connected()) // Configure via Bluetooth
    {
#if DEBUG
        Serial.println("Connected");
#endif
        SerialBT.setTimeout(100000);

        SerialBT.println("Please enter WIFI SSID:");
        SerialBT.readBytesUntil('\r', ssid, SSID_MAXLEN * sizeof(char));
        cleanInput(ssid);

        SerialBT.println("Please enter WIFI password:");
        SerialBT.readBytesUntil('\r', password, WIFI_PASS_MAXLEN * sizeof(char));
        cleanInput(password);

        char gmtOffsetStr[10] = "";
        SerialBT.println("Please enter GMT offset in seconds:");
        SerialBT.readBytesUntil('\r', gmtOffsetStr, sizeof(gmtOffsetStr));
        cleanInput(gmtOffsetStr);
        *gmtOffset = atoi(gmtOffsetStr);

        char daylightOffsetStr[10] = "";
        SerialBT.println("Please enter daylight offset in seconds:");
        SerialBT.readBytesUntil('\r', daylightOffsetStr, sizeof(gmtOffsetStr));
        cleanInput(daylightOffsetStr);
        *daylightOffset = atoi(daylightOffsetStr);

        SerialBT.printf("Using WiFi \"%s\" with credential \"%s\", GMT offset %i, daylight offset %i. Disconnecting.\n", ssid, password, *gmtOffset, *daylightOffset);
        delay(500);
        SerialBT.end();
        return CONFIGURED_VIA_BT;
    }
    else // Didn't connect, configure via stored information on disk instead
    {
#if DEBUG
        Serial.println("Didn't sync via Bluetooth. Loading credentials from NVS instead.");
#endif
        preferences.getBytes("ssid", ssid, SSID_MAXLEN * sizeof(char));
        preferences.getBytes("password", password, WIFI_PASS_MAXLEN * sizeof(char));
        *gmtOffset = preferences.getInt("gmtOffset", 0);
        *daylightOffset = preferences.getInt("daylightOffset", 0);
        return CONFIGURED_VIA_NVS;
    }
}

bool connectWiFiAndConfigTime(const char *ssid, const char *password, int gmtOffset, int daylightOffset)
{
    // Connect to WiFi
    WiFi.mode(WIFI_STA);
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
        return false;
    }
#if DEBUG
    Serial.println("Connected.");
#endif

    // Configure time
#if DEBUG
    Serial.print("Configuring time...");
#endif
    uint8_t tryCount = NTP_CONFIGTIME_TRYCOUNT;
    while (!getLocalTime(timeInfo) && tryCount--)
    {
        configTime(gmtOffset, daylightOffset, "pool.ntp.org");
    }
    if (!getLocalTime(timeInfo)) // Couldn't configure time
    {
#if DEBUG
        Serial.println("Failed");
#endif
        return false;
    }
#if DEBUG
    Serial.println("Done.");
#endif
    // Cleanup
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
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
    preferences.begin("flipdots", false);
    TaskHandle_t showLoaderTask;
    char ssid[SSID_MAXLEN] = "";
    char password[WIFI_PASS_MAXLEN] = "";
    int gmtOffset = 0;
    int daylightOffset = 0;

    xTaskCreate(taskDisplayLoader, "taskDisplayLoader", 1024, (void *)LOADINGSCREEN_BLUETOOTH, 1, &showLoaderTask);
    uint8_t configuredFrom = getCredentialsViaBluetoothOrNVS(ssid, password, &gmtOffset, &daylightOffset);
    vTaskDelete(showLoaderTask);

    if (strlen(ssid) == 0 || strlen(password) == 0) // No credentials
    {
#if DEBUG
        Serial.println("Failed to load WiFi credentials from anywhere.");
#endif
        displayError();
        while (1)
            ;
    }

    // Show loading screen & connect Wifi & configure time
    xTaskCreate(taskDisplayLoader, "taskDisplayLoader", 1024, (void *)LOADINGSCREEN_GLIDER, 1, &showLoaderTask);
    digitalWrite(LED_BUILTIN, HIGH);
#if DEBUG
    Serial.printf("Using WiFi \"%s\" with credential \"%s\"\n", ssid, password);
#endif
    if (!connectWiFiAndConfigTime(ssid, password, gmtOffset, daylightOffset))
    {
        vTaskDelete(showLoaderTask);
        digitalWrite(LED_BUILTIN, LOW);
        displayError();
        while (1)
            ;
    }
    // WiFi success, save credentials in non-volatile storage for next boot (unless loaded from non-volatile storage)
    if (configuredFrom != CONFIGURED_VIA_NVS)
    {
        preferences.putBytes("ssid", (const char *)ssid, strlen(ssid));
        preferences.putBytes("password", (const char *)password, strlen(password));
        preferences.putInt("gmtOffset", gmtOffset);
        preferences.putInt("daylightOffset", daylightOffset);
    }
    preferences.end();
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelete(showLoaderTask);

    // short animation, then show time
    display.setInverted(true);
    display.clear();
    display.setInverted(false);
    delay(250);
    displayTime();
    xTaskCreate(taskUpdateClock, "taskUpdateClock", 2048, NULL, 1, NULL);
}

void loop()
{
    vTaskSuspend(NULL);
}