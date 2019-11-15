/**************************************************************
 * MAX Cube library
 * Connects to a cube and parses the valve percent information
 * 
 * Romelec 2019
 *************************************************************/

#ifndef MaxCube_h
#define MaxCube_h

#include <ESP8266WiFi.h>
#include <WiFiManager.h>

#ifndef MAXCUBE_VALVE_COUNT
#define MAXCUBE_VALVE_COUNT 5
#endif

class MaxCube {
  public:
    MaxCube();
    ~MaxCube();
    void init();

    void setIP(const IPAddress& cube_ip);
    void setIP(const IPAddress& cube_ip, uint16_t cube_port);
    int update();
    int getMaxValve();

  private:
    void parseL(char* buffer, size_t length);
    void updateValve(uint32_t address, int valve);

    IPAddress _cube_ip;
    uint16_t _cube_port;
    int* _valveValues;

    /* struct L_Data - decoded from Base64 payload in L message */
    struct L_Data{
        uint8_t Submessage_Length[1];
        uint8_t RF_Address[3];
        uint8_t Unknown[1];
        uint8_t Flags[2];
        // If the submessage length is greater than 6, these fields follow
        uint8_t Valve_Position[1];
        uint8_t Temperature[1];
        // generic pointer to following data
        uint8_t next_data[1];
    };

    typedef struct valve_info
    {
        uint32_t RF_Address;
        int Valve_Position;
    }valve_info_t;

    valve_info _valves[MAXCUBE_VALVE_COUNT];

};

#endif