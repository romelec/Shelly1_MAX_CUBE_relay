/**************************************************************
 * MAX Cube library
 * Connects to a cube and parses the valve percent information
 * Done relatively quickly, not perfect but good enough !
 * 
 * Romelec 2019
 * License CC BY-SA : do whatever you want but
 *  cite my name/github and publish changes
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
        if (length && rxbuffer[length-1] == '\r')
        {
            Serial.printf("MSG (%d): ", length);
            Serial.write(rxbuffer, length);
            Serial.write('\n');
            Serial.flush();

            switch (rxbuffer[0])
            {
                case 'C':
                    parseC(rxbuffer, length);
                    break;
                case 'L':
                    parseL(rxbuffer, length);
                    running = false;
                    break;
                default:
                    break;
            };
        }
        else if (length)
        {
            Serial.printf("Invalid MSG, length %d\n", length);
        }
        
    }
    while(running && client.connected());
    Serial.println("MaxCube disconnect");
    client.stop();

    return true;
}

void MaxCube::parseC(char* buffer, size_t length)
{
    //Ignore the 9 first bytes (C:xxxxxx,) and last one (\r)
    size_t decodedLength = Base64.decodedLength(buffer+9, length-10);
    char decodedBuffer[decodedLength];
    Base64.decode(decodedBuffer, buffer+9, length-10);

    size_t parsed_length = 0;

    Serial.print("Decoded C: ");
    for(;parsed_length < 18; parsed_length++)
    {
        Serial.printf("%.2X, ", decodedBuffer[parsed_length]);
    }
    Serial.print("\n");

    struct MaxCube::C_Data_Device *C_D = (struct C_Data_Device*)&decodedBuffer[0];
    uint32_t address = (C_D->RF_Address[0] << 16) + (C_D->RF_Address[1]<< 8) + (C_D->RF_Address[2]);

    if (C_D->Device_Type[0] == 1 || C_D->Device_Type[0] == 2)
    {
        for (size_t i = 0; i < MAXCUBE_VALVE_COUNT; i++)
        {
            if (_valves[i].RF_Address == address)
            {
                Serial.printf("ALREADY address 0x%.8x, in pos %d\n", address, i);
                return;
            }
            if (_valves[i].RF_Address == 0)
            {
                Serial.printf("ADD address 0x%.8x, to pos %d\n", address, i);
                _valves[i].RF_Address = address;
                _valves[i].Valve_Position = 0;
                return;
            }
        }
    }
}

void MaxCube::parseL(char* buffer, size_t length)
{
    //Ignore the 2 first bytes (L:) and last one (\r)
    size_t decodedLength = Base64.decodedLength(buffer+2, length-3);
    char decodedBuffer[decodedLength];
    Base64.decode(decodedBuffer, buffer+2, length-3);

    size_t parsed_length = 0;

    Serial.print("Decoded L: ");
    for(;parsed_length < decodedLength; parsed_length++)
    {
        Serial.printf("%.2X, ", decodedBuffer[parsed_length]);
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
    // Udate if present
    for (size_t i = 0; i < MAXCUBE_VALVE_COUNT; i++)
    {
        if (_valves[i].RF_Address == address)
        {
            Serial.printf("UPDATE address 0x%.8x, valve %d\n", address, valve);
            _valves[i].Valve_Position = valve;
            return;
        }
    }
    Serial.printf("IGNORE address 0x%.8x, valve %d\n", address, valve);
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