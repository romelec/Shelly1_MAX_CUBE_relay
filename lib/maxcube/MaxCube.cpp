/**************************************************************
 * MAX Cube library
 * Connects to a cube and parses the valve percent information
 * 
 * Romelec 2019
 *************************************************************/
#include "MaxCube.h"
#include "Base64.h"

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

MaxCube::MaxCube()
{
    //Nothing for now
}

MaxCube::~MaxCube()
{
    //Nothing for now
}

void MaxCube::init()
{
    for (size_t i = 0; i < MAXCUBE_VALVE_COUNT; i++)
    {
        _valves[i].RF_Address = 0;
        _valves[i].Valve_Position = 0;
    }
}

void MaxCube::setIP(const IPAddress& cube_ip)
{
    _cube_ip = cube_ip;
}
void MaxCube::setIP(const IPAddress& cube_ip, uint16_t cube_port)
{
    _cube_ip = cube_ip;
    _cube_port = cube_port;
}

int MaxCube::update()
{
    WiFiClient client;

    if(!client.connect(_cube_ip, _cube_port))
    {
        Serial.println("Connect Failed");
        return false;
    }

    client.setTimeout(2);
    char rxbuffer[2000];
    size_t length;

    bool running = true;
    do
    {
        length = client.readBytesUntil('\n', rxbuffer, sizeof(rxbuffer));
        if (length)
        {
            Serial.printf("MSG (%d): ", length);
            Serial.write(rxbuffer, length);
            Serial.write('\n');
            Serial.flush();

            switch (rxbuffer[0])
            {
                case 'L':
                    parseL(rxbuffer, length);
                    running = false;
                    break;
                default:
                    break;
            };
        }
    }
    while(running && client.connected());
    Serial.println("MaxCube disconnect");
    client.stop();

    return true;
}

void MaxCube::parseL(char* buffer, size_t length)
{
    //Ignore the 2 first bytes (L:) and last one (\r)
    size_t decodedLength = Base64.decodedLength(buffer+2, length-3);
    char decodedBuffer[decodedLength];
    Base64.decode(decodedBuffer, buffer+2, length-3);

    size_t parsed_length = 0;

    Serial.print("Decoded: ");
    for(;parsed_length < decodedLength; parsed_length++)
    {
        Serial.printf("0x%.2x, ", decodedBuffer[parsed_length]);
    }
    Serial.print("\n");

    parsed_length = 0;
    while (parsed_length < decodedLength)
    {
        struct MaxCube::L_Data *L_D = (struct L_Data*)&decodedBuffer[parsed_length];
        parsed_length = parsed_length + (size_t)L_D->Submessage_Length[0] + 1;

        if ((int)L_D->Submessage_Length[0] > 6)
        {
            uint32_t address = (L_D->RF_Address[0] << 16) + (L_D->RF_Address[1]<< 8) + (L_D->RF_Address[2]);
            int valve = (int)L_D->Valve_Position[0];
            updateValve(address,valve);
        }
    };
}

void MaxCube::updateValve(uint32_t address, int valve)
{
    // 1st update if already present
    for (size_t i = 0; i < MAXCUBE_VALVE_COUNT; i++)
    {
        if (_valves[i].RF_Address == address)
        {
            Serial.printf("UPDATE address 0x%.8x, valve %d\n", address, valve);
            _valves[i].Valve_Position = valve;
            return;
        }
    }

    // Thermostats have the valve value fixed at 4, ignore this value
    if (valve == 4)
    {
        Serial.printf("IGNORE address 0x%.8x, valve %d\n", address, valve);
        return;
    }

    // 2nd add it in an empty slot
    for (size_t i = 0; i < MAXCUBE_VALVE_COUNT; i++)
    {
        if (_valves[i].RF_Address == 0)
        {
            Serial.printf("ADD address 0x%.8x, valve %d\n", address, valve);
            _valves[i].RF_Address = address;
            _valves[i].Valve_Position = valve;
            return;
        }
    }
}

int MaxCube::getMaxValve()
{
    int valve = 0;
    for (size_t i = 0; i < MAXCUBE_VALVE_COUNT; i++)
    {
        if (_valves[i].RF_Address != 0)
        {
            valve = MAX(_valves[i].Valve_Position, valve);
        }
    }
    return valve;
}