# LilyGo-EPD-4-7

The LilyGO is an 4.7" e-paper screen. For this project the ESP32 will be located infront of a meetingroom.
Currently, Open Weather Map is being used to show the weather on the LilyGo EPD 4.7" display.

# Google scrips
To actually create google scripts go to the folder `google_scrips/`  and change the agenda names if needed.
Publish the scrips and make sure both of them are selected:
* **Execture as user**: yourself
* **Who has access**: everyone


Each script will have its own implementation ID, add them to the own_credentials, or update.

If the `get_web_app_token` does not exist in the the `own_credentials.h` file add the token for: `String get_web_app_token = "<tImplementatie-ID-token->";` and the line itself.


# Compiling and flashing

Edit `owm_credentials.h` and enter OWM API key as well as the location for which you want to display the weather data

You need Platformio to compile (on VScode)


C:\Users<Local User>.platformio\packages\framework-

#define CONFIG_MBEDTLS_SSL_MAX_CONTENT_LEN 16384
to -->
#define CONFIG_MBEDTLS_SSL_MAX_CONTENT_LEN 32768