#include <Arduino.h>           // In-built
#include <esp_task_wdt.h>      // In-built
#include "freertos/FreeRTOS.h" // In-built
#include "freertos/task.h"     // In-built
#include "epd_driver.h"        // https://github.com/Xinyuan-LilyGO/LilyGo-EPD47
#include "esp_adc_cal.h"       // In-built
#include <HTTPSRedirect.h>     // In-built

// #include <WiFiClientSecure.h>

#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include <HTTPClient.h>  // In-built

#include <WiFi.h> // In-built
#include <SPI.h>  // In-built
#include <time.h> // In-built

#include "owm_credentials.h"
#include "forecast_record.h"
#include "lang_nl.h"

#define SCREEN_WIDTH EPD_WIDTH
#define SCREEN_HEIGHT EPD_HEIGHT

//################  VERSION  ##################################################
String version = "2.7 / 4.7in"; // Programme version, see change log at end
//################ VARIABLES ##################################################

enum alignment { LEFT, RIGHT, CENTER };
#define White 0xFF
#define LightGrey 0xBB
#define Grey 0x88
#define DarkGrey 0x44
#define Black 0x00

#define autoscale_on true
#define autoscale_off false
#define barchart_on true
#define barchart_off false

boolean LargeIcon = true;
boolean SmallIcon = false;
#define Large 20 // For icon drawing
#define Small 10 // For icon drawing
String Time_str            = "--:--:--";
String Date_str            = "-- --- ----";
String internal_server_str = "000.000.000.000";
int    wifi_signal, CurrentHour = 0, CurrentMin = 0, CurrentSec = 0, EventCnt = 0, vref = 1100;
//################ PROGRAM VARIABLES and OBJECTS ##########################################
#define max_readings 24 // Limited to 3-days here, but could go to 5-days = 40 as the data is issued

Forecast_record_type WxConditions[1];
Forecast_record_type WxForecast[max_readings];

float pressure_readings[max_readings]    = {0};
float temperature_readings[max_readings] = {0};
float humidity_readings[max_readings]    = {0};
float rain_readings[max_readings]        = {0};
float snow_readings[max_readings]        = {0};

long SleepDuration = 1; // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour
int  WakeupHour    = 8;  // Wakeup after 07:00 to save battery power
int  SleepHour     = 23; // Sleep  after 23:00 to save battery power
long StartTime     = 0;
long SleepTimer    = 0;
long Delta         = 30; // ESP32 rtc speed compensation, prevents display at xx:59:yy and then xx:00:yy (one minute later) to save power

// fonts
#include <opensans8b.h>
#include <opensans10b.h>
#include <opensans12.h>
#include <opensans12b.h>
#include <opensans18b.h>
#include <opensans14.h>
#include <opensans14b.h>
#include <opensans24b.h>
#include "moon.h"
#include "sunrise.h"
#include "sunset.h"
#include "uvi.h"

GFXfont  currentFont;
uint8_t* framebuffer;

void BeginSleep() {
    epd_poweroff_all();
    UpdateLocalTime();
    SleepTimer = (SleepDuration * 60 - ((CurrentMin % SleepDuration) * 60 + CurrentSec)) + Delta; // Some ESP32 have a RTC that is too fast to maintain accurate time, so add an offset
    esp_sleep_enable_timer_wakeup(SleepTimer * 1000000LL);                                        // in Secs, 1000000LL converts to Secs as unit = 1uSec
    Serial.println("Awake for : " + String((millis() - StartTime) / 1000.0, 3) + "-secs");
    Serial.println("Entering " + String(SleepTimer) + " (secs) of sleep time");
    Serial.println("Starting deep-sleep period...");
    esp_deep_sleep_start(); // Sleep for e.g. 30 minutes
}

boolean SetupTime() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, "time.nist.gov"); //(gmtOffset_sec, daylightOffset_sec, ntpServer)
    setenv("TZ", Timezone, 1);                                                 // setenv()adds the "TZ" variable to the environment with a value TimeZone, only used if set to 1, 0 means no change
    tzset();                                                                   // Set the TZ environment variable
    delay(100);
    return UpdateLocalTime();
}

uint8_t StartWiFi() {
    Serial.print("\r\nConnecting to: ");
    Serial.println(String(ssid));
    IPAddress dns(8, 8, 8, 8); // Google DNS
    WiFi.disconnect();
    WiFi.mode(WIFI_STA); // switch off AP
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);
    unsigned long start = millis();
    uint8_t       connectionStatus;
    bool          AttemptConnection = true;
    while (AttemptConnection) {
        connectionStatus = WiFi.status();
        if (millis() > start + 15000) { // Wait 15-secs maximum
            AttemptConnection = false;
        }
        if (connectionStatus == WL_CONNECTED || connectionStatus == WL_CONNECT_FAILED) {
            AttemptConnection = false;
        }
        delay(50);
    }
    if (connectionStatus == WL_CONNECTED) {
        wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!

        internal_server_str = WiFi.localIP().toString();

        Serial.println("WiFi connected at: " + WiFi.localIP().toString());
        DisplayGeneralInfoSection();
    } else
        Serial.println("WiFi connection *** FAILED ***");
    return connectionStatus;
}

void StopWiFi() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi switched Off");
}

void InitialiseSystem() {
    StartTime = millis();
    Serial.begin(115200);
    while (!Serial)
        ;
    Serial.println(String(__FILE__) + "\nStarting...");
    epd_init();
    framebuffer = (uint8_t*)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
    if (!framebuffer)
        Serial.println("Memory alloc failed!");
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
}

void loop() {
    // Nothing to do here
}

void setup() {

    InitialiseSystem();
    if (StartWiFi() == WL_CONNECTED && SetupTime() == true) {
        bool WakeUp = false;
        if (WakeupHour > SleepHour)
            WakeUp = (CurrentHour >= WakeupHour || CurrentHour <= SleepHour);
        else
            WakeUp = (CurrentHour >= WakeupHour && CurrentHour <= SleepHour);
        if (WakeUp) {
            byte       Attempts   = 1;
            bool       RxWeather  = false;
            bool       RxForecast = false;
            WiFiClient client;

            // wifi client object
            while ((RxWeather == false || RxForecast == false) && Attempts <= 2) { // Try up-to 2 time for Weather and Forecast data
                create_new_agenda("I_AM_AWSOME");

                // if (RxWeather == false)
                //     RxWeather = obtainWeatherData(client, "onecall");
                // if (RxForecast == false)
                //     RxForecast = obtainWeatherData(client, "forecast");
                Attempts++;
            }

            // Serial.println("Received all weather data...");
            if (RxWeather && RxForecast) { // Only if received both Weather or Forecast proceed
                StopWiFi();                // Reduces power consumption
                epd_poweron();             // Switch on EPD display
                epd_clear();               // Clear the screen
                DisplayWeather();          // Display the weather data
                edp_update();              // Update the display to show the information
                epd_poweroff_all();        // Switch off all power to EPD
            }
        }
    } else {
        epd_poweron();      // Switch on EPD display
        epd_clear();        // Clear the screen
        DisplayWeather();   // Display the weather data
        edp_update();       // Update the display to show the information
        epd_poweroff_all(); // Switch off all power to EPD
    }
    BeginSleep();
}

/*
bool DecodeWeather(WiFiClient& json, String Type) {
    Serial.print(F("\nCreating object...and "));
    DynamicJsonDocument  doc(64 * 1024);                     // allocate the JsonDocument
    DeserializationError error = deserializeJson(doc, json); // Deserialize the JSON document
    if (error) {                                             // Test if parsing succeeds.
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return false;
    }
    // convert it to a JsonObject
    JsonObject root = doc.as<JsonObject>();
    Serial.println(" Decoding " + Type + " data");
    if (Type == "onecall") {
        // All Serial.println statements are for diagnostic purposes and some are not required, remove if not needed with //
        WxConditions[0].High      = -50;                    // Minimum forecast low
        WxConditions[0].Low       = 50;                     // Maximum Forecast High
        WxConditions[0].FTimezone = doc["timezone_offset"]; // "0"
        JsonObject current        = doc["current"];
        WxConditions[0].Sunrise   = current["sunrise"];
        Serial.println("SRis: " + String(WxConditions[0].Sunrise));
        WxConditions[0].Sunset = current["sunset"];
        Serial.println("SSet: " + String(WxConditions[0].Sunset));
        WxConditions[0].Temperature = current["temp"];
        Serial.println("Temp: " + String(WxConditions[0].Temperature));
        WxConditions[0].FeelsLike = current["feels_like"];
        Serial.println("FLik: " + String(WxConditions[0].FeelsLike));
        WxConditions[0].Pressure = current["pressure"];
        Serial.println("Pres: " + String(WxConditions[0].Pressure));
        WxConditions[0].Humidity = current["humidity"];
        Serial.println("Humi: " + String(WxConditions[0].Humidity));
        WxConditions[0].DewPoint = current["dew_point"];
        Serial.println("DPoi: " + String(WxConditions[0].DewPoint));
        WxConditions[0].UVI = current["uvi"];
        Serial.println("UVin: " + String(WxConditions[0].UVI));
        WxConditions[0].Cloudcover = current["clouds"];
        Serial.println("CCov: " + String(WxConditions[0].Cloudcover));
        WxConditions[0].Visibility = current["visibility"];
        Serial.println("Visi: " + String(WxConditions[0].Visibility));
        WxConditions[0].Windspeed = current["wind_speed"];
        Serial.println("WSpd: " + String(WxConditions[0].Windspeed));
        WxConditions[0].Winddir = current["wind_deg"];
        Serial.println("WDir: " + String(WxConditions[0].Winddir));
        JsonObject current_weather = current["weather"][0];
        String     Description     = current_weather["description"]; // "scattered clouds"
        String     Icon            = current_weather["icon"];        // "01n"
        WxConditions[0].Forecast0  = Description;
        Serial.println("Fore: " + String(WxConditions[0].Forecast0));
        WxConditions[0].Icon = Icon;
        Serial.println("Icon: " + String(WxConditions[0].Icon));
    }
    if (Type == "forecast") {
        // Serial.println(json);
        Serial.print(F("\nReceiving Forecast period - ")); //------------------------------------------------
        JsonArray list = root["list"];
        for (byte r = 0; r < max_readings; r++) {
            Serial.println("\nPeriod-" + String(r) + "--------------");
            WxForecast[r].Dt          = list[r]["dt"].as<int>();
            WxForecast[r].Temperature = list[r]["main"]["temp"].as<float>();
            Serial.println("Temp: " + String(WxForecast[r].Temperature));
            WxForecast[r].Low = list[r]["main"]["temp_min"].as<float>();
            Serial.println("TLow: " + String(WxForecast[r].Low));
            WxForecast[r].High = list[r]["main"]["temp_max"].as<float>();
            Serial.println("THig: " + String(WxForecast[r].High));
            WxForecast[r].Pressure = list[r]["main"]["pressure"].as<float>();
            Serial.println("Pres: " + String(WxForecast[r].Pressure));
            WxForecast[r].Humidity = list[r]["main"]["humidity"].as<float>();
            Serial.println("Humi: " + String(WxForecast[r].Humidity));
            WxForecast[r].Icon = list[r]["weather"][0]["icon"].as<char*>();
            Serial.println("Icon: " + String(WxForecast[r].Icon));
            WxForecast[r].Rainfall = list[r]["rain"]["3h"].as<float>();
            Serial.println("Rain: " + String(WxForecast[r].Rainfall));
            WxForecast[r].Snowfall = list[r]["snow"]["3h"].as<float>();
            Serial.println("Snow: " + String(WxForecast[r].Snowfall));
            if (r < 8) { // Check next 3 x 8 Hours = 1 day
                if (WxForecast[r].High > WxConditions[0].High)
                    WxConditions[0].High = WxForecast[r].High; // Get Highest temperature for next 24Hrs
                if (WxForecast[r].Low < WxConditions[0].Low)
                    WxConditions[0].Low = WxForecast[r].Low; // Get Lowest  temperature for next 24Hrs
            }
        }
        //------------------------------------------
        float pressure_trend  = WxForecast[0].Pressure - WxForecast[2].Pressure; // Measure pressure slope between ~now and later
        pressure_trend        = ((int)(pressure_trend * 10)) / 10.0;             // Remove any small variations less than 0.1
        WxConditions[0].Trend = "=";
        if (pressure_trend > 0)
            WxConditions[0].Trend = "+";
        if (pressure_trend < 0)
            WxConditions[0].Trend = "-";
        if (pressure_trend == 0)
            WxConditions[0].Trend = "0";

    }
    return true;
}
*/
//#########################################################################################
String ConvertUnixTime(int unix_time) {
    // Returns either '21:12  ' or ' 09:12pm' depending on Units mode
    time_t     tm     = unix_time;
    struct tm* now_tm = localtime(&tm);
    char       output[40];
    if (Units == "M") {
        strftime(output, sizeof(output), "%H:%M %d/%m/%y", now_tm);
    } else {
        strftime(output, sizeof(output), "%I:%M%P %m/%d/%y", now_tm);
    }
    return output;
}
//#########################################################################################

HTTPSRedirect* client_httpdirect = nullptr;
// used to store the values of free stack and heap
// before the HTTPSRedirect object is instantiated
// so that they can be written to Google sheets
// upon instantiation
unsigned int free_heap_before = 0;
unsigned int free_stack_before = 0;

bool create_new_agenda(const String& agenda_value) {

    // * /macros/s/{API key}/exec?title={agenda_value}

    free_heap_before = ESP.getFreeHeap();
    free_stack_before = ESP.getFreePsram();
    Serial.printf("Free heap: %u\n", free_heap_before);
    Serial.printf("Free stack: %u\n", free_stack_before);


    delay(100);
    // Use HTTPSRedirect class to create a new TLS connection
    client_httpdirect = new HTTPSRedirect(443);
    client_httpdirect->setPrintResponseBody(true);
    client_httpdirect->setInsecure();
    client_httpdirect->setContentTypeHeader("application/json");

    Serial.print("Connecting to ");
    Serial.println(host_google);

    // Try to connect for a maximum of 5 times
    bool flag = false;
    for (int i = 0; i < 5; i++) {
        int retval = client_httpdirect->connect(host_google, 443);
        if (retval == 1) {
            flag = true;
            break;
        } else
            Serial.println("Connection failed. Retrying...");
    }

    if (!flag) {
        Serial.print("Could not connect to server: ");
        Serial.println(host_google);
        Serial.println("Exiting...");
        delete client_httpdirect;
        return false;
    }


    Serial.println("Connected to Google");


    // * Fetch Google Calendar events

    String uri = String("/macros/s/") + web_app_token + String("/exec") + String("?title=") + agenda_value;
    Serial.println(host_google + uri);

    while(1){
        client_httpdirect->GET(uri, host_google, true);
    }

    String calendarData = client_httpdirect->getResponseBody();
    Serial.print("Calendar Data---> "); Serial.println(calendarData); Serial.println("");

    // delete HTTPSRedirect object
    delete client_httpdirect;
    client_httpdirect = nullptr;

    return true;
}

void DisplayWeather() { // 4.7" e-paper display is 960x540 resolution

    DisplayGeneralInfoSection(); // Top line of the display

    String time        = "04:00 - 16:00";
    String organisator = "Martijn van Wezel";
    String titel       = "End of the Month - Internal IT";
    DisplayTimeBox_current(50, 100, time, organisator, titel);

    time           = "16:00 - 16:30";
    String details = "Booked";
    DisplayTimeBox_comming(50, 200, time, details);

    time    = "16:30 - 24:00";
    details = "Available";
    DisplayTimeBox_comming(50, 260, time, details);

    // * Weather stuff
    // DisplayMainWeatherSection(450, 300);           // Centre section of display for Location, temperatur
    // DisplayForecastSection(450, 350); // 3hr forecast boxes
}

void DisplayGeneralInfoSection() {
    setFont(OpenSans10B);

    drawString(5, 2, "http://" + internal_server_str, LEFT);
    setFont(OpenSans8B);
    drawString(250, 2, Date_str + "  @   " + Time_str, LEFT);

    // * Wi-Fi signal strength and Battery voltage
    DisplayStatusSection(550, 20, wifi_signal);

    fillRect(10, 25, 960 - 20, 4, Black);
}



void DisplayTimeBox_current(int x, int y, String time, String organisator, String titel) {

    setFont(OpenSans18B);

    drawString(x, y, time, LEFT);
    y = y + 30;

    // setFont(OpenSans12B);

    // drawString(x, y, "Organisator: ", LEFT);
    // setFont(OpenSans12);
    // drawString(x+200, y, organisator.substring(0, 20), LEFT);
    setFont(OpenSans14);
    drawString(x, y, organisator.substring(0, 20), LEFT);

    x = x + 450;

    setFont(OpenSans12);

    uint8_t counter   = 0;
    uint8_t maxlength = 80;
    while (titel.length() >= counter) {

        if (counter + maxlength <= 180) {
            drawString(x + 20, y + 5, titel.substring(counter, counter + maxlength), LEFT);
        } else {
            String endline = "...";
            drawString(x + 20, y + 5, titel.substring(counter, counter + maxlength - 3) + endline, LEFT);
        }

        counter = counter + maxlength;
        y       = y + 35;
    }
}

void DisplayTimeBox_comming(int x, int y, String time, String details) {

    setFont(OpenSans12B);

    drawString(x, y, time, LEFT);

    y = y + 25;
    setFont(OpenSans8B);

    if (details.length() <= 80) {
        drawString(x, y, details, LEFT);
    } else {
        String endline = "...";
        drawString(x, y, details.substring(0, 80 - 3) + endline, LEFT);
    }
}

void DrawSegment(int x, int y, int o1, int o2, int o3, int o4, int o11, int o12, int o13, int o14) {
    drawLine(x + o1, y + o2, x + o3, y + o4, Black);
    drawLine(x + o11, y + o12, x + o13, y + o14, Black);
}

void DisplayStatusSection(int x, int y, int rssi) {
    setFont(OpenSans8B);
    DrawRSSI(x + 335, y, rssi);
    DrawBattery(x + 150, y);
}

void DrawRSSI(int x, int y, int rssi) {
    int WIFIsignal = 0;
    int xpos       = 1;
    for (int _rssi = -100; _rssi <= rssi; _rssi = _rssi + 20) {
        if (_rssi <= -20)
            WIFIsignal = 30; //            <-20dbm displays 5-bars
        if (_rssi <= -40)
            WIFIsignal = 24; //  -40dbm to  -21dbm displays 4-bars
        if (_rssi <= -60)
            WIFIsignal = 18; //  -60dbm to  -41dbm displays 3-bars
        if (_rssi <= -80)
            WIFIsignal = 12; //  -80dbm to  -61dbm displays 2-bars
        if (_rssi <= -100)
            WIFIsignal = 6; // -100dbm to  -81dbm displays 1-bar
        fillRect(x + xpos * 8, y - WIFIsignal, 6, WIFIsignal, Black);
        xpos++;
    }
}

boolean UpdateLocalTime() {
    struct tm timeinfo;
    char      time_output[30], day_output[30 + 13], update_time[30];
    while (!getLocalTime(&timeinfo, 5000)) { // Wait for 5-sec for time to synchronise
        Serial.println("Failed to obtain time");
        return false;
    }
    CurrentHour = timeinfo.tm_hour;
    CurrentMin  = timeinfo.tm_min;
    CurrentSec  = timeinfo.tm_sec;
    // See http://www.cplusplus.com/reference/ctime/strftime/
    Serial.println(&timeinfo, "%a %b %d %Y   %H:%M:%S"); // Displays: Saturday, June 24 2017 14:05:49
    if (Units == "M") {
        sprintf(day_output, "Last updated:%s, %02u %s %04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900);
        strftime(update_time, sizeof(update_time), "%H:%M:%S", &timeinfo); // Creates: '@ 14:05:49'   and change from 30 to 8 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        sprintf(time_output, "%s", update_time);
    } else {
        strftime(day_output, sizeof(day_output), "%a %b-%d-%Y", &timeinfo); // Creates  'Sat May-31-2019'
        strftime(update_time, sizeof(update_time), "%r", &timeinfo);        // Creates: '@ 02:05:49pm'
        sprintf(time_output, "%s", update_time);
    }
    Date_str = day_output;
    Time_str = time_output;
    return true;
}

void DrawBattery(int x, int y) {
    uint8_t                       percentage = 100;
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t           val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
        vref = adc_chars.vref;
    }
    float voltage = analogRead(36) / 4096.0 * 6.566 * (vref / 1000.0);
    if (voltage > 1) { // Only display if there is a valid reading
        Serial.println("\nVoltage = " + String(voltage));
        percentage = 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) - 656689.7123 * voltage + 632041.7303;
        if (voltage >= 4.20)
            percentage = 100;
        if (voltage <= 3.20)
            percentage = 0; // orig 3.5
        drawRect(x + 25, y - 14, 40, 15, Black);
        fillRect(x + 65, y - 10, 4, 7, Black);
        fillRect(x + 27, y - 12, 36 * percentage / 100.0, 11, Black);
        drawString(x + 85, y - 14, String(percentage) + "%  " + String(voltage, 1) + "v", LEFT);
    }
}

/* (C) D L BIRD
    This function will draw a graph on a ePaper/TFT/LCD display using data from an array containing data to be graphed.
    The variable 'max_readings' determines the maximum number of data elements for each array. Call it with the following parametric data:
    x_pos-the x axis top-left position of the graph
    y_pos-the y-axis top-left position of the graph, e.g. 100, 200 would draw the graph 100 pixels along and 200 pixels down from the top-left of the screen
    width-the width of the graph in pixels
    height-height of the graph in pixels
    Y1_Max-sets the scale of plotted data, for example 5000 would scale all data to a Y-axis of 5000 maximum
    data_array1 is parsed by value, externally they can be called anything else, e.g. within the routine it is called data_array1, but externally could be temperature_readings
    auto_scale-a logical value (TRUE or FALSE) that switches the Y-axis autoscale On or Off
    barchart_on-a logical value (TRUE or FALSE) that switches the drawing mode between barhcart and line graph
    barchart_colour-a sets the title and graph plotting colour
    If called with Y!_Max value of 500 and the data never goes above 500, then autoscale will retain a 0-500 Y scale, if on, the scale increases/decreases to match the data.
    auto_scale_margin, e.g. if set to 1000 then autoscale increments the scale by 1000 steps.
*/
void DrawGraph(int x_pos, int y_pos, int gwidth, int gheight, float Y1Min, float Y1Max, String title, float DataArray[], int readings, boolean auto_scale, boolean barchart_mode) {
#define auto_scale_margin 0 // Sets the autoscale increment, so axis steps up fter a change of e.g. 3
#define y_minor_axis 5      // 5 y-axis division markers
    setFont(OpenSans10B);
    int   maxYscale = -10000;
    int   minYscale = 10000;
    int   last_x, last_y;
    float x2, y2;
    if (auto_scale == true) {
        for (int i = 1; i < readings; i++) {
            if (DataArray[i] >= maxYscale)
                maxYscale = DataArray[i];
            if (DataArray[i] <= minYscale)
                minYscale = DataArray[i];
        }
        maxYscale = round(maxYscale + auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Max
        Y1Max     = round(maxYscale + 0.5);
        if (minYscale != 0)
            minYscale = round(minYscale - auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Min
        Y1Min = round(minYscale);
    }
    // Draw the graph
    last_x = x_pos + 1;
    last_y = y_pos + (Y1Max - constrain(DataArray[1], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight;
    drawRect(x_pos, y_pos, gwidth + 3, gheight + 2, Grey);
    drawString(x_pos - 20 + gwidth / 2, y_pos - 28, title, CENTER);
    for (int gx = 0; gx < readings; gx++) {
        x2 = x_pos + gx * gwidth / (readings - 1) - 1; // max_readings is the global variable that sets the maximum data that can be plotted
        y2 = y_pos + (Y1Max - constrain(DataArray[gx], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight + 1;
        if (barchart_mode) {
            fillRect(last_x + 2, y2, (gwidth / readings) - 1, y_pos + gheight - y2 + 2, Black);
        } else {
            drawLine(last_x, last_y - 1, x2, y2 - 1, Black); // Two lines for hi-res display
            drawLine(last_x, last_y, x2, y2, Black);
        }
        last_x = x2;
        last_y = y2;
    }
    // Draw the Y-axis scale
#define number_of_dashes 20
    for (int spacing = 0; spacing <= y_minor_axis; spacing++) {
        for (int j = 0; j < number_of_dashes; j++) { // Draw dashed graph grid lines
            if (spacing < y_minor_axis)
                drawFastHLine((x_pos + 3 + j * gwidth / number_of_dashes), y_pos + (gheight * spacing / y_minor_axis), gwidth / (2 * number_of_dashes), Grey);
        }
        if ((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing) < 5 || title == TXT_PRESSURE_IN) {
            drawString(x_pos - 10, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 1), RIGHT);
        } else {
            if (Y1Min < 1 && Y1Max < 10) {
                drawString(x_pos - 3, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 1), RIGHT);
            } else {
                drawString(x_pos - 7, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 0), RIGHT);
            }
        }
    }
    for (int i = 0; i < 3; i++) {
        drawString(20 + x_pos + gwidth / 3 * i, y_pos + gheight + 10, String(i) + "d", LEFT);
        if (i < 2)
            drawFastVLine(x_pos + gwidth / 3 * i + gwidth / 3, y_pos, gheight, LightGrey);
    }
}

void drawString(int x, int y, String text, alignment align) {
    char* data = const_cast<char*>(text.c_str());
    int   x1, y1; // the bounds of x,y and w and h of the variable 'text' in pixels.
    int   w, h;
    int   xx = x, yy = y;
    get_text_bounds(&currentFont, data, &xx, &yy, &x1, &y1, &w, &h, NULL);
    if (align == RIGHT)
        x = x - w;
    if (align == CENTER)
        x = x - w / 2;
    int cursor_y = y + h;
    write_string(&currentFont, data, &x, &cursor_y, framebuffer);
}

void fillCircle(int x, int y, int r, uint8_t color) {
    epd_fill_circle(x, y, r, color, framebuffer);
}

void drawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color) {
    epd_draw_hline(x0, y0, length, color, framebuffer);
}

void drawFastVLine(int16_t x0, int16_t y0, int length, uint16_t color) {
    epd_draw_vline(x0, y0, length, color, framebuffer);
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    epd_write_line(x0, y0, x1, y1, color, framebuffer);
}

void drawCircle(int x0, int y0, int r, uint8_t color) {
    epd_draw_circle(x0, y0, r, color, framebuffer);
}

void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    epd_draw_rect(x, y, w, h, color, framebuffer);
}

void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    epd_fill_rect(x, y, w, h, color, framebuffer);
}

void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    epd_fill_triangle(x0, y0, x1, y1, x2, y2, color, framebuffer);
}

void drawPixel(int x, int y, uint8_t color) {
    epd_draw_pixel(x, y, color, framebuffer);
}

void setFont(GFXfont const& font) {
    currentFont = font;
}

void edp_update() {
    epd_draw_grayscale_image(epd_full_screen(), framebuffer); // Update the screen
}
/*
   1071 lines of code 03-03-2021
*/
