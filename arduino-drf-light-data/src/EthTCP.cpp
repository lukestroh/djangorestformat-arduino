/*
    Source file for custom Ethernet class
    Author: Luke Strohbehn
    Date: 4/17/2023
*/

#include "EthTCP.h"


Eth::Eth():
    /* Initialize class variables */
    lan_server_ip(169, 254, 235, 35),
    lan_server_port {8000},
    ard_server_port {10002},
    ard_client(),
    ard_server(ard_server_port)
{

}

void Eth::begin_ethernet() {
    /*  Initialize Ethernet shield
        DHCP version of this function, Ethernet.begin(mac),
        returns int: 1 on successful connection, 0 on failure.
        Other versions don't return anything
    */
    Ethernet.begin(MAC, IP, DNS, GATEWAY, SUBNET);

#if SERIAL_DEBUG
    // Check for hardware errors
    if (static_cast<int>(Ethernet.hardwareStatus()) == static_cast<int>(EthernetNoHardware)) {
        Serial.println(F("ERROR: Ethernet shield was not found."));
    }

    if (static_cast<int>(Ethernet.hardwareStatus()) == static_cast<int>(LinkOFF)) {
        Serial.println(F("ERROR: Ethernet cable is not connected."));
    }

    Serial.println(F("Network information:"));
    Serial.print(F("IPv4 Address: "));
    Serial.println(Ethernet.localIP());
    Serial.print(F("Gateway: "));
    Serial.println(Ethernet.gatewayIP());
    Serial.print(F("Subnet mask: "));
    Serial.println(Ethernet.subnetMask());
    Serial.print(F("DNS Server: "));
    Serial.println(Ethernet.dnsServerIP());
    Serial.println();
#endif // SERIAL_DEBUG
}


void Eth::begin_server() {
    ard_server.begin();
}

bool Eth::connect_ard_client() {
    if (!ard_client.connected()) {
        if (ard_client.connect(lan_server_ip, lan_server_port) == 1) {
#if SERIAL_DEBUG
            Serial.println(F("Connected to host.\n"));
#endif // SERIAL_DEBUG
        }
#if SERIAL_DEBUG
        else {
            Serial.println(F("Connection to host failed.\n"));
        }
#endif // SERIAL_DEBUG
    }

    // `maintain()` must be frequently called to maintain DHCP lease for the given IP address
    Ethernet.maintain();

    // Check if connected after connection attempt. Maybe just check in loop?
    if (ard_client.connected()) {
        return true;
    } else {
        return false;
    }
}

void Eth::accept_clients() {
    /* Add new clients to client list */
    EthernetClient new_client = ard_server.accept();
#if SERIAL_DEBUG
        Serial.print(F("Got new client: "));
        Serial.println(new_client);
#endif // SERIAL_DEBUG
    if (new_client) {
        for (uint8_t i=0; i<num_clients; ++i) {
            if (!lan_clients[i]) {
                lan_clients[i] = new_client;
                break;
            }
        }
    }
}

void Eth::remove_clients() {
    /* Remove clients if they disconnect */
    for (uint8_t i=0; i<num_clients; ++i) {
        if (lan_clients[i] && !lan_clients[i].connected()) {
            lan_clients[i].stop();
        }
    }
}

void Eth::read_data() { // need to make this the server, not the client
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    // Parse through client list and check for new data
    for (uint8_t i=0; i<num_clients; ++i) {
        while (lan_clients[i] && lan_clients[i].available() > 0 && newData == false) {
            rc = lan_clients[i].read();
            if (recvInProgress == true) {
                if (rc != endMarker) {
                    receivedChars[ndx] = rc;
                    ndx++;
                    if (ndx >= numChars) {
                        ndx = numChars - 1; // don't pass max index value
                    }
                } else {
                    receivedChars[ndx] = '\0'; // terminate the string
                    recvInProgress = false;
                    ndx = 0;
                    newData = true;
                }
            } else if (rc == startMarker) {
                recvInProgress = true;
            }
        }
    }
}


void Eth::send_data(char* data) {
    /* Send a char array via TCP */
    ard_client.println(data);
}

void Eth::send_data(float f) {
    /* Send a float via TCP */
    char send_buffer[13];
    dtostrf(f, 10, 10, send_buffer);
    ard_client.println(send_buffer);
}
