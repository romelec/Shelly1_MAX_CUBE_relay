/**************************************************************
 * MAX Cube Boiler control
 * Shelly 1 firmware to control a boiler depending on the
 * ELV MAX thermostats valve position.
 * 
 * Performs a query to the cube every minute and closes the
 * connection to allow the use of other tools
 * (MAX! Remote Android app for example)
 * 
 * Romelec 2019
 * License CC BY-SA : do whatever you want but
 *  cite my name/github and publish changes
 *************************************************************/

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include <MaxCube.h>

#define UPDATE_INTERVAL 60000   // Update every 1 minute
#define UPDATE_ERROR_MAX 10     // Restart avter 10 failed updates
#define LOOP_CNT_MAX 60*12      // Restart every 12 hours

#define PIN_RELAY 4

MaxCube cube;

// Default configuration, modifiable in the web interface
static IPAddress cube_ip(192, 168, 0, 100);
const uint16_t cube_port = 62910;
static int valve_on_limit = 25;
static int valve_off_limit = 10;

//callback notifying us of the need to save config
static bool shouldSaveConfig = false;
void saveConfigCallback ()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

void setup()
{
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, LOW);

    Serial.begin(115200);
    Serial.println();

    //SPIFFS.format();  // clean FS - for testing

    Serial.println("mounting FS...");
    if (SPIFFS.begin())
    {
        Serial.println("mounted file system");
        if (SPIFFS.exists("/config.json"))
        {
            Serial.println("reading config file");
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile)
            {
                Serial.println("opened config file");
                size_t size = configFile.size();
                std::unique_ptr<char[]> buf(new char[size]);

                configFile.readBytes(buf.get(), size);
                DynamicJsonDocument doc(1024);
                DeserializationError error = deserializeJson(doc, buf.get());
                if (!error)
                {
                    cube_ip.fromString(doc["cube_ip"].as<char*>());
                    valve_on_limit = doc["on_limit"];
                    valve_off_limit = doc["off_limit"];

                    Serial.printf("From config: ip: '%s', on_limit %d, average %d\n", cube_ip.toString().c_str(),
                        valve_on_limit, valve_off_limit);
                }
                else
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

    //WiFiManager
    char tmp_string[4];
    WiFiManagerParameter cube_ip_p("cube_ip", "cube_ip", cube_ip.toString().c_str(), 16);
    itoa(valve_on_limit, tmp_string, 10);
    WiFiManagerParameter valve_on_limit_p("On limit", "On limit", tmp_string, 4);
    itoa(valve_off_limit, tmp_string, 10);
    WiFiManagerParameter valve_off_limit_p("Off limit", "Off limit", tmp_string, 4);

    WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.addParameter(&cube_ip_p);
    wifiManager.addParameter(&valve_on_limit_p);
    wifiManager.addParameter(&valve_off_limit_p);

    //wifiManager.resetSettings(); // reset settings - for testing
    wifiManager.setTimeout(120);

    // Connect to the stored wifi or creates an access point
    if (!wifiManager.autoConnect("MAXRelay", "password"))
    {
        Serial.println("failed to connect and hit timeout");
        delay(5000);
        ESP.restart();
        delay(500);
    }
    Serial.println("WIFI connected");

    // Save the custom parameters to FS
    if (shouldSaveConfig)
    {
        cube_ip.fromString(cube_ip_p.getValue());
        valve_on_limit = atoi(valve_on_limit_p.getValue());
        valve_off_limit = atoi(valve_off_limit_p.getValue());

        Serial.printf("To config: ip: '%s', single %d, average %d\n",
                cube_ip.toString().c_str(), valve_on_limit, valve_off_limit);

        DynamicJsonDocument doc(1024);
        doc["cube_ip"] = cube_ip.toString();
        doc["on_limit"] = valve_on_limit;
        doc["off_limit"] = valve_off_limit;
        serializeJson(doc, Serial);

        File configFile = SPIFFS.open("/config.json", "w");
        if (configFile)
        {
            serializeJson(doc, configFile);
            configFile.close();
        }
        else
        {
            Serial.println("failed to open config file for writing");
        }
    }

    Serial.printf("Config: ip: '%s', on_limit %d, off_limit %d\n", 
            cube_ip.toString().c_str(), valve_on_limit, valve_off_limit);

    Serial.println("local ip");
    Serial.println(WiFi.localIP());

    cube.init();
    cube.setIP(cube_ip, cube_port);
}

void loop()
{
    static int update_error_cnt = 0;
    static int loop_cnt = 0;
    if (!cube.update())
    {
        Serial.println("could not be updated");
        update_error_cnt++;
        if (update_error_cnt >= UPDATE_ERROR_MAX)
        {
            Serial.println("could not be updated -> REBOOT");
            delay(500);
            ESP.restart();
            delay(1000);
        }
        delay(UPDATE_INTERVAL);
        return;
    }
    update_error_cnt = 0;

    int valve = cube.getMaxValve();
    Serial.printf("Max valve: %d\n", valve);

    if (valve > valve_on_limit)
    {
        Serial.println("boiler ON");
        digitalWrite(PIN_RELAY, HIGH);
    }
    else if (valve < valve_off_limit)
    {
        Serial.println("boiler OFF");
        digitalWrite(PIN_RELAY, LOW);
    }
    /* Serial.printf("Free RAM: %d segmentation %d\n",
        ESP.getFreeHeap(), ESP.getHeapFragmentation());*/

    loop_cnt++;
    if (loop_cnt > LOOP_CNT_MAX)
    {
        // Not realy necessary, but restart from time to time
        Serial.println("Restarting...");
        delay(5000);
        ESP.restart();
        delay(500);
    }

    delay(UPDATE_INTERVAL);
}
