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


# Microsoft Office 365 calendar -> Google calendar

## Overview

One-way sync from a Microsoft Office 365 Outlook calendar to a Google calendar, handling new, updated, and deleted events.

The script connects to the Microsoft API using the [O365 package](https://github.com/O365/python-o365#calendar) and connects to the Google API using its [Python client](https://developers.google.com/calendar/api/quickstart/python). Familiarize yourself with their documentation as you may need to enable APIs or create credentials per their instructions before you begin. See also the Google Calendar API [reference](https://developers.google.com/calendar/v3/reference/events).

## Setup

  - Create `config.py` (you can adapt [`config_sample.py`](config_sample.py)) to hold your personal configuration details, include your Microsoft `client_id` and `client_secret` and your Google calendar ID.
      - **Create a new Google calendar just for this application, or else your existing events will be deleted!**
      - Create Google credentials for this application (see overview section above) and save as `credentials/google_credentials.json`.
      - Get the Microsoft `client_id` and `client_secret` by following the O365 [instructions](https://github.com/O365/python-o365#authentication) on how to authenticate on behalf of a user.
  - Run `pip install --upgrade -r requirements.txt` to install the [required Python dependencies](requirements.txt).
  - In the [credentials folder](credentials), run [`python quickstart.py`](credentials/quickstart.py) to create a Google API access token.
  - Microsoft API access token is created interactively via URL on first run, then permanently stored. It expires in 90 days *if* you don't run the script within that time.
  - On your server, set up a cron job to run [`outlook_to_google.py`](outlook_to_google.py) (using [run.sh](run.sh)) every 15 minutes (or however often you need).
  - The script will check Microsoft for calendar events and compare them to the calendar events it saved (in events_ts.json) during the previous run.

## extra docs

** The script will delete all events on this **Google calendar** and then add some Microsoft calendar events to the Google calendar.**




### STEP 1 - Google credentials for OAuth 2.0:
-> Create a project in tthe google cloud platform, don't know if you really really need one but google sugest it.
-> to go the API'S & Services -> Credentials
-> Created an OAuth client ID, first time user? then register
-> Then create a OAuth client id:
    1) web application
    2) name it to something usefull
    3) Create, BUT download the JSON file and place it under creadentials folder with the name `google_credentials.json`
-> Run the [`python quickstart.py`](credentials/quickstart.py) in the `credentials/` folder to create a Google API access token.

### STEP 2 - Outlook credentials
-> Follow the following link: [instructions O365](https://github.com/O365/python-o365#authentication)
-> Click on `Authenticate on behalf of a user (public):`
-> The tutorial tells the following:
    * Create a new APP registration [on this page](https://portal.azure.com/#blade/Microsoft_AAD_RegisteredApps/ApplicationsListBlade)
    * Name it with a good name: `Accounts in any organizational directory (Any Azure AD directory - Multitenant)`
    * Select for platform `web`, without link
    * Copy the `config_sample.py` to the new file `config.py`
    * `outlook_client_id` is the **application (client) ID**
    * `outlook_client_tenant` is the **Directory (tenant) ID**
    * Create a secret under the `Certificated & secrets` -> `new client secret` -> give it some name and when it expires.
    * Copy the secret value from above and place in the config under `outlook_client_secret`.



### STEP 3 - Combine le stuff
-> change in the config the `google_calendar_id` to your calendar id (settings button when you hover over the agenda in google agenda, settings-and-share-something, then `Agenda-ID` is your agendaa)
-> TODO Finish this writing..

redirect url opgezet en onder clients een token aangemaakt