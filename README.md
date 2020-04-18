# Shelly1_MAX_CUBE_relay
Boiler control for MAX heating system on a Shelly 1 relay

Very easy to use, on the first power up connect to the wifi "MAXRelay" with the password "password" (Can be changed on wifiManager.autoConnect).
Select your wifi SSID, enter the password, and put the MAX cube IP address.

It is also possible to change the valve position limits that controls the boiler:
- Boiler ON if ONE valve > "On limit"
- Boiler OFF if ALL valves < "Off limit"

Then restart it and every minute it will check the valve percentage and control the relay accordingly.

It never stays connected to the Cube, meaning you can still use other applications (change the settings, ...)

Be careful when programming/debugging, it is not isolated from the mains so make sure to disconnect it when working on it.
