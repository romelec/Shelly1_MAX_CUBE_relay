/**************************************************************
 * MAX Cube library
 * Connects to a cube and parses the valve percent information
 * Done relatively quickly, not perfect but good enough !
 * 
 * TODO: add the cube IP discovery ?
 * https://github.com/Bouni/max-cube-protocol/blob/master/Cube_Discovery.md
 * 
 * Romelec 2019
 * License CC BY-SA : do whatever you want but
 *  cite my name/github and publish changes
 * 
 * Protocol information from https://github.com/Bouni/max-cube-protocol
 * struct C_Data_Device L_Data from Costin Popescu https://github.com/pacostiro/MAX-cube-ctl
 * 
 *************************************************************/

/* Copyright (c) 2015, Costin Popescu
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MaxCube_h
#define MaxCube_h

#include <ESP8266WiFi.h>
#include <WiFiManager.h>

// Change this if needed
#define MAXCUBE_VALVE_COUNT 5

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
    void parseC(char* buffer, size_t length);
    void parseL(char* buffer, size_t length);
    void updateValve(uint32_t address, int valve);

    IPAddress _cube_ip;
    uint16_t _cube_port;
    int* _valveValues;

    /* union C_Data_Device - decoded from Base64 payload in C message */
    struct C_Data_Device {
        uint8_t Data_Length[1];
        uint8_t RF_Address[3];
        uint8_t Device_Type[1];
        uint8_t Room_ID[1];
        uint8_t Firmware_version[1];
        uint8_t Test_Result[1];
        char Serial_Number[10];
    };

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