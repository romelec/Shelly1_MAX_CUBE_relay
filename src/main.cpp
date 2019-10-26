#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#define THERMOSTAT_NAME_LENGTH 11U
#define THERMOSTAT_MAX 10U

//define your default values here, if there are different values in config.json, they are overwritten.
int valve_single_limit = 50;
int valve_average_limit = 25;
char valve_single_limit_s[4];
char valve_average_limit_s[4];
char thermostats[THERMOSTAT_MAX][THERMOSTAT_NAME_LENGTH];


//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback ()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println();

    itoa(valve_single_limit, valve_single_limit_s, 10);
    itoa(valve_average_limit, valve_average_limit_s, 10);

    //clean FS, for testing
    //SPIFFS.format();

    //read configuration from FS json
    Serial.println("mounting FS...");

    if (SPIFFS.begin())
    {
        Serial.println("mounted file system");
        if (SPIFFS.exists("/config.json"))
        {
            //file exists, reading and loading
            Serial.println("reading config file");
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile)
            {
                Serial.println("opened config file");
                size_t size = configFile.size();
                // Allocate a buffer to store contents of the file.
                std::unique_ptr<char[]> buf(new char[size]);

                configFile.readBytes(buf.get(), size);
                DynamicJsonDocument doc(1024);
                DeserializationError error = deserializeJson(doc, buf.get());
                if (!error)
                {
                    Serial.println("\nparsed json");
                    strcpy(valve_single_limit_s, doc["min_single"]);
                    strcpy(valve_average_limit_s, doc["min_average"]);
                    valve_single_limit = atoi(valve_single_limit_s);
                    valve_average_limit = atoi(valve_average_limit_s);

                    strcpy(thermostats[0], doc["thermostat_0"]);
                    strcpy(thermostats[0], doc["thermostat_0"]);
                    strcpy(thermostats[1], doc["thermostat_1"]);
                    strcpy(thermostats[2], doc["thermostat_2"]);
                    strcpy(thermostats[3], doc["thermostat_3"]);
                    strcpy(thermostats[4], doc["thermostat_4"]);
                    strcpy(thermostats[5], doc["thermostat_5"]);
                    strcpy(thermostats[6], doc["thermostat_6"]);
                    strcpy(thermostats[7], doc["thermostat_7"]);
                    strcpy(thermostats[8], doc["thermostat_8"]);
                    strcpy(thermostats[9], doc["thermostat_9"]);
                }else
                {
                    Serial.println("failed to load json config");
                }
                configFile.close();
            }
        }
    }
    else
    {
        Serial.println("failed to mount FS");
    }
    //end read

    // The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    // id/name placeholder/prompt default length
    WiFiManagerParameter valve_single_limit_p(
        "Single valve", "Single valve", valve_single_limit_s, 4);
    WiFiManagerParameter valve_average_limit_p(
        "Average valve", "Average valve", valve_average_limit_s, 4);

    WiFiManagerParameter thermostat_0(
        "Thermostat 0", "Thermostat 0", thermostats[0], THERMOSTAT_NAME_LENGTH);
    WiFiManagerParameter thermostat_1(
        "Thermostat 1", "Thermostat 1", thermostats[1], THERMOSTAT_NAME_LENGTH);
    WiFiManagerParameter thermostat_2(
        "Thermostat 2", "Thermostat 2", thermostats[2], THERMOSTAT_NAME_LENGTH);
    WiFiManagerParameter thermostat_3(
        "Thermostat 3", "Thermostat 3", thermostats[3], THERMOSTAT_NAME_LENGTH);
    WiFiManagerParameter thermostat_4(
        "Thermostat 4", "Thermostat 4", thermostats[4], THERMOSTAT_NAME_LENGTH);
    WiFiManagerParameter thermostat_5(
        "Thermostat 5", "Thermostat 5", thermostats[5], THERMOSTAT_NAME_LENGTH);
    WiFiManagerParameter thermostat_6(
        "Thermostat 6", "Thermostat 6", thermostats[6], THERMOSTAT_NAME_LENGTH);
    WiFiManagerParameter thermostat_7(
        "Thermostat 7", "Thermostat 7", thermostats[7], THERMOSTAT_NAME_LENGTH);
    WiFiManagerParameter thermostat_8(
        "Thermostat 8", "Thermostat 8", thermostats[8], THERMOSTAT_NAME_LENGTH);
    WiFiManagerParameter thermostat_9(
        "Thermostat 9", "Thermostat 9", thermostats[9], THERMOSTAT_NAME_LENGTH);

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    //add all your parameters here
    wifiManager.addParameter(&valve_single_limit_p);
    wifiManager.addParameter(&valve_average_limit_p);
    wifiManager.addParameter(&thermostat_0);
    wifiManager.addParameter(&thermostat_1);
    wifiManager.addParameter(&thermostat_2);
    wifiManager.addParameter(&thermostat_3);
    wifiManager.addParameter(&thermostat_4);
    wifiManager.addParameter(&thermostat_5);
    wifiManager.addParameter(&thermostat_6);
    wifiManager.addParameter(&thermostat_7);
    wifiManager.addParameter(&thermostat_8);
    wifiManager.addParameter(&thermostat_9);

    //reset settings - for testing
    //wifiManager.resetSettings();

    //set minimu quality of signal so it ignores AP's under that quality
    //defaults to 8%
    //wifiManager.setMinimumSignalQuality();
    
    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    wifiManager.setTimeout(120);

    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "MAXRelay"
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect("MAXRelay", "password"))
    {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...)");

    //save the custom parameters to FS
    if (shouldSaveConfig)
    {
        Serial.println("saving config");

        //read updated parameters
        strcpy(valve_single_limit_s, valve_single_limit_p.getValue());
        strcpy(valve_average_limit_s, valve_average_limit_p.getValue());
        valve_single_limit = atoi(valve_single_limit_s);
        valve_average_limit = atoi(valve_average_limit_s);

        strcpy(thermostats[0], thermostat_0.getValue());
        strcpy(thermostats[1], thermostat_1.getValue());
        strcpy(thermostats[2], thermostat_2.getValue());
        strcpy(thermostats[3], thermostat_3.getValue());
        strcpy(thermostats[4], thermostat_4.getValue());
        strcpy(thermostats[5], thermostat_5.getValue());
        strcpy(thermostats[6], thermostat_6.getValue());
        strcpy(thermostats[7], thermostat_7.getValue());
        strcpy(thermostats[8], thermostat_8.getValue());
        strcpy(thermostats[9], thermostat_9.getValue());

        DynamicJsonDocument doc(1024);
        doc["min_single"] = valve_single_limit_s;
        doc["min_average"] = valve_average_limit_s;
        doc["thermostat_0"] = thermostats[0];
        doc["thermostat_1"] = thermostats[1];
        doc["thermostat_2"] = thermostats[2];
        doc["thermostat_3"] = thermostats[3];
        doc["thermostat_4"] = thermostats[4];
        doc["thermostat_5"] = thermostats[5];
        doc["thermostat_6"] = thermostats[6];
        doc["thermostat_7"] = thermostats[7];
        doc["thermostat_8"] = thermostats[8];
        doc["thermostat_9"] = thermostats[9];

        File configFile = SPIFFS.open("/config.json", "w");
        if (!configFile) {
        Serial.println("failed to open config file for writing");
        }

        serializeJson(doc, Serial);
        serializeJson(doc, configFile);
        configFile.close();
        //end save
    }

    Serial.println("local ip");
    Serial.println(WiFi.localIP());

}

void loop() {
  // put your main code here, to run repeatedly:


}
