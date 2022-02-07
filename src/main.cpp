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
#define UPDATE_EVERY_SECOND 0
#define SSID_MAXLEN 32
#define WIFI_PASS_MAXLEN 64

#define LOADINGSCREEN_GENERIC 0
#define LOADINGSCREEN_BLUETOOTH 1
#define LOADINGSCREEN_GLIDER 2

#define CONFIGURED_VIA_BT 0
#define CONFIGURED_VIA_NVS 1

const char *ntpServer = "pool.ntp.org";

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

bool displayTime()
{
    time_t now;
    time(&now);
    localtime_r(&now, timeInfo);
    if (timeInfo->tm_year <= (2016 - 1900)) // time is not valid
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

uint8_t getCredentialsViaBluetoothOrNVS(char *ssid, char *password)
{
    // Attempt to receive WiFI credentials via Bluetooth
#if DEBUG
    Serial.print("Connecting Bluetooth.");
#endif
    SerialBT.begin("FLIPDOTS Clock");
    long startTime = millis();
    while (!SerialBT.connected() && (millis() - startTime) < BT_TIMEOUT)
    {
#if DEBUG
        Serial.print('.');
#endif
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
        SerialBT.println("Please enter WIFI password:");
        SerialBT.readBytesUntil('\r', password, WIFI_PASS_MAXLEN * sizeof(char));
        if (password[0] == '\n') // remove leading whitespace
            memmove(password, password + 1, WIFI_PASS_MAXLEN - 1);
        SerialBT.printf("Using WiFi \"%s\" with credential \"%s\"\n", ssid, password);
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
        return CONFIGURED_VIA_NVS;
    }
}

bool connectWiFiAndConfigTime(const char *ssid, const char *password)
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
        configTime(-28800, 0, ntpServer);
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

    xTaskCreate(taskDisplayLoader, "taskDisplayLoader", 1024, (void *)LOADINGSCREEN_BLUETOOTH, 1, &showLoaderTask);
    uint8_t configuredFrom = getCredentialsViaBluetoothOrNVS(ssid, password);
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
    if (!connectWiFiAndConfigTime(ssid, password))
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
        preferences.end();
    }
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelete(showLoaderTask);

    // short animation, then show time
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