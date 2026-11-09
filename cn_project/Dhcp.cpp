#include <cstdint>
#include <cstddef>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>

using namespace std;


// Buffer sizes
constexpr std::size_t DHCP_CHADDR_LEN = 16;
constexpr std::size_t DHCP_SNAME_LEN = 64;
constexpr std::size_t DHCP_FILE_LEN = 128;
constexpr std::size_t DHCP_OPTIONS_BUF_LEN = 312; // Chosen buffer capacity

// Operation codes
constexpr uint8_t BOOTREQUEST = 1;
constexpr uint8_t BOOTREPLY = 2;

// DHCP message types
constexpr uint8_t DHCPDISCOVER = 1;
constexpr uint8_t DHCPOFFER = 2;
constexpr uint8_t DHCPREQUEST = 3;
constexpr uint8_t DHCPDECLINE = 4;
constexpr uint8_t DHCPACK = 5;
constexpr uint8_t DHCPNAK = 6;
constexpr uint8_t DHCPRELEASE = 7;
constexpr uint8_t DHCPINFORM = 8;

// DHCP option codes
constexpr uint8_t DHCP_OPTION_PAD = 0;
constexpr uint8_t DHCP_OPTION_SUBNET_MASK = 1;
constexpr uint8_t DHCP_OPTION_ROUTER = 3;
constexpr uint8_t DHCP_OPTION_DNS_SERVER = 6;
constexpr uint8_t DHCP_OPTION_HOSTNAME = 12;
constexpr uint8_t DHCP_OPTION_DOMAIN_NAME = 15;
constexpr uint8_t DHCP_OPTION_IP_TTL = 23;
constexpr uint8_t DHCP_OPTION_MTU = 26;
constexpr uint8_t DHCP_OPTION_BROADCAST_ADDR = 28;
constexpr uint8_t DHCP_OPTION_STATIC_ROUTE = 33;
constexpr uint8_t DHCP_OPTION_TCP_TTL = 37;
constexpr uint8_t DHCP_OPTION_NTP = 42;
constexpr uint8_t DHCP_OPTION_NETBIOS_NAME_SRV = 44;
constexpr uint8_t DHCP_OPTION_NETBIOS_NODE_TYPE = 46;
constexpr uint8_t DHCP_OPTION_REQUESTED_IP = 50;
constexpr uint8_t DHCP_OPTION_LEASE_TIME = 51;
constexpr uint8_t DHCP_OPTION_MSG_TYPE = 53;
constexpr uint8_t DHCP_OPTION_SERVER_ID = 54;
constexpr uint8_t DHCP_OPTION_PARAM_REQ_LIST = 55;
constexpr uint8_t DHCP_RENEWAL_TIME = 58;
constexpr uint8_t DHCP_REBINDING_TIME = 59;
constexpr uint8_t DHCP_OPTION_CLIENT_ID = 61;
constexpr uint8_t DHCP_OPTION_LDAP = 95;
constexpr uint8_t DHCP_OPTION_DISABLE_IPV4 = 108;
constexpr uint8_t DHCP_OPTION_CAPTIVE_PORTAL = 114;
constexpr uint8_t DHCP_OPTION_DOMAIN_SEARCH = 119;
constexpr uint8_t DHCP_OPTION_CLASSLESS_STATIC_ROUTE = 121;
constexpr uint8_t DHCP_OPTION_END = 255;

// DHCP magic cookie
constexpr uint32_t DHCP_MAGIC_COOKIE = 0x63825363;

//structure of the DHCP message
struct Dhcp_packet_t
{
    //fixed header is of 236 bytes
    uint8_t op;       // 1 = client request, 2 = server reply
    uint8_t htype;    // Hardware type: 1 = Ethernet
    uint8_t hlen;     // Hardware address length: 6 for Ethernet
    uint8_t hops;     // Used by relay agents

    uint32_t xid;     // Transaction ID
    uint16_t secs;    // Seconds since the process began
    uint16_t flags;   // Includes the broadcast flag

    uint32_t ciaddr;  // Client's current IP address the client already has and can use
    uint32_t yiaddr;  // Offered/assigned client IP address by the server/router
    uint32_t siaddr;  // Next bootstrap server IP address
    uint32_t giaddr;  // Relay agent IP address

    uint8_t chaddr[DHCP_CHADDR_LEN];  // Client hardware address
    char sname[DHCP_SNAME_LEN];      // Optional server hostname
    char file[DHCP_FILE_LEN];        // Optional boot filename

    //4-byte magic cookie to recognize the options and then options(variable)
    uint32_t magic_cookie;                       // DHCP options marker
    uint8_t options[DHCP_OPTIONS_BUF_LEN];        // DHCP options
};

//Strore information across the exchange
/*
    Before DISOCVER => Set MAC and transaction ID
    After receiving OFFER => Save offered IP AND server IP
    When building REQUEST => Reading the saved values
*/
struct ClientContext {
    uint8_t mac[6]{};
    uint32_t transactionId{};  // Host byte order

    uint32_t offeredIp{};     // The address received in an OFFER
    uint32_t serverIp{};      // The selected server's ip address from option 54
};

//Build the outgoing packets
void fillClientHeader(Dhcp_packet_t& packet,const ClientContext& client)
{
    packet = {}; //All fields are zero

    //htonl=>Converts 32-bit integer from host to network byte order
    //htons=>Converts 16-bit integer from host to network byte order
    
    packet.op = BOOTREQUEST;
    packet.htype = 1; //Ethernet
    packet.hlen = 6; //MAC add. length
    packet.hops = 0;

    packet.xid = htonl(client.transactionId);

    packet.secs = 0;
    packet.flags = htons(0x8000); //Request broadcast reply

    std::memcpy(packet.chaddr, client.mac, sizeof(client.mac));

    packet.magic_cookie = htonl(DHCP_MAGIC_COOKIE);
};

/*
    Uses fillDiscover to create the packet
    Write the options into packet.options
    Tracks how many option bytes were written
    Returns the actual packet lenght
*/
std::size_t buildDiscover(Dhcp_packet_t& packet,const ClientContext& client)
{
    fillClientHeader(packet, client);

    std::size_t index = 0;

    // Option 53: DHCP message type
    packet.options[index++] = DHCP_OPTION_MSG_TYPE;
    packet.options[index++] = 1;  // Data length
    packet.options[index++] = DHCPDISCOVER;

    // Option 55: Parameters we want the server to provide
    packet.options[index++] = DHCP_OPTION_PARAM_REQ_LIST;
    packet.options[index++] = 5;  // Five option codes
    packet.options[index++] = DHCP_OPTION_SUBNET_MASK;
    packet.options[index++] = DHCP_OPTION_ROUTER;
    packet.options[index++] = DHCP_OPTION_DNS_SERVER;
    packet.options[index++] = DHCP_OPTION_LEASE_TIME;
    packet.options[index++] = DHCP_OPTION_SERVER_ID;

    // Option 61: Client identifier = Ethernet type + MAC
    packet.options[index++] = DHCP_OPTION_CLIENT_ID;
    packet.options[index++] = 7;  // One type byte + six MAC bytes
    packet.options[index++] = 1;  // Ethernet

    std::memcpy(packet.options + index,client.mac,sizeof(client.mac));
    index += sizeof(client.mac);

    // End of options: no length byte follows
    packet.options[index++] = DHCP_OPTION_END;

    return offsetof(Dhcp_packet_t, options) + index;
};

/*
    DHCPRequest using the OFFER info extracted by the parse Reply
*/
std::size_t buildRequest(Dhcp_packet_t& packet,const ClientContext& client)
{
    fillClientHeader(packet,client);

    std::size_t index=0;

    // Option 53: DHCP message type
    packet.options[index++] = DHCP_OPTION_MSG_TYPE;
    packet.options[index++] = 1;
    packet.options[index++] = DHCPREQUEST;

    // Option 50: Requested IP address
    packet.options[index++] = DHCP_OPTION_REQUESTED_IP;
    packet.options[index++] = 4;

    std::memcpy(packet.options+index,&client.offeredIp,sizeof(client.offeredIp));
    index+=sizeof(client.offeredIp);

    // Option 54: Selected DHCP server
    packet.options[index++] = DHCP_OPTION_SERVER_ID;
    packet.options[index++] = 4;

    std::memcpy(
        packet.options + index,
        &client.serverIp,
        sizeof(client.serverIp)
    );
    index += sizeof(client.serverIp);

    // Option 55: Same parameter request list as DISCOVER
    packet.options[index++] = DHCP_OPTION_PARAM_REQ_LIST;
    packet.options[index++] = 5;
    packet.options[index++] = DHCP_OPTION_SUBNET_MASK;
    packet.options[index++] = DHCP_OPTION_ROUTER;
    packet.options[index++] = DHCP_OPTION_DNS_SERVER;
    packet.options[index++] = DHCP_OPTION_LEASE_TIME;
    packet.options[index++] = DHCP_OPTION_SERVER_ID;

    // Option 61: Same client identifier as DISCOVER
    packet.options[index++] = DHCP_OPTION_CLIENT_ID;
    packet.options[index++] = 7;
    packet.options[index++] = 1;  // Ethernet

    std::memcpy(
        packet.options + index,
        client.mac,
        sizeof(client.mac)
    );
    index += sizeof(client.mac);

    // End of options
    packet.options[index++] = DHCP_OPTION_END;

    return offsetof(Dhcp_packet_t, options) + index;
};


// //Handle UDP communication
// int createClientSocket();

// bool sendPacket(
//     int socketFd,
//     const Dhcp_packet_t& packet,
//     std::size_t packetLength
// );

// ssize_t receivePacket(
//     int socketFd,
//     Dhcp_packet_t& packet
// );


//Parse and validate incoming packets
struct ParsedReply {
    uint8_t messageType{};
    uint32_t offeredIp{};     // Network byte order
    uint32_t serverIp{};      // Network byte order
    uint32_t leaseSeconds{};  // Host byte order
};

bool parseReply(const Dhcp_packet_t& packet,std::size_t receivedLength,const ClientContext& client,ParsedReply& reply)
{
    constexpr std::size_t optionsOffset = offsetof(Dhcp_packet_t, options);

    // Check length before accessing packet fields.
    if (receivedLength < optionsOffset ||receivedLength > sizeof(packet)) {return false;}

    // Validate the fixed header and client identity.
    if (packet.op != BOOTREPLY ||
        packet.htype != 1 ||
        packet.hlen != sizeof(client.mac) ||
        ntohl(packet.xid) != client.transactionId ||
        ntohl(packet.magic_cookie) != DHCP_MAGIC_COOKIE ||
        std::memcmp(packet.chaddr, client.mac,
        sizeof(client.mac)) != 0){return false;}

    ParsedReply parsed{};
    parsed.offeredIp = packet.yiaddr;

    const std::size_t optionsLength = receivedLength - optionsOffset;
    std::size_t index = 0;

    bool foundMessageType = false;
    bool foundServerId = false;
    bool foundEnd = false;

    while (index < optionsLength) {
        uint8_t code = packet.options[index++];

        if (code == DHCP_OPTION_END) {
            foundEnd = true;
            break;
        }

        if (code == DHCP_OPTION_PAD) {
            continue;
        }

        // Every other option needs a length byte.
        if (index >= optionsLength) {
            return false;
        }

        uint8_t length = packet.options[index++];

        // Ensure the option's data fits in the received bytes.
        if (length > optionsLength - index) {
            return false;
        }

        const uint8_t* data = packet.options + index;

        switch (code) {
            case DHCP_OPTION_MSG_TYPE:
                if (length != 1 || foundMessageType) {
                    return false;
                }

                parsed.messageType = data[0];
                foundMessageType = true;
                break;

            case DHCP_OPTION_SERVER_ID:
                if (length != 4 || foundServerId) {
                    return false;
                }

                std::memcpy(&parsed.serverIp, data, 4);
                foundServerId = true;
                break;

            case DHCP_OPTION_LEASE_TIME: {
                if (length != 4) {
                    return false;
                }

                uint32_t networkLease{};
                std::memcpy(&networkLease, data, 4);
                parsed.leaseSeconds = ntohl(networkLease);
                break;
            }

            default:
                // Skip options this parser doesn't extract.
                break;
        }

        index += length;
    }

    if (!foundEnd || !foundMessageType || !foundServerId) {
        return false;
    }

    // This client handles these three server message types.
    if (parsed.messageType != DHCPOFFER &&
        parsed.messageType != DHCPACK &&
        parsed.messageType != DHCPNAK) {
        return false;
    }

    // Only update the output after successful parsing.
    reply = parsed;
    return true;
}


// //Printing
// void printPacketHeader(const Dhcp_packet_t& packet);
// void printReply(const ParsedReply& reply);


int main() {

    ClientContext client{};
    client.transactionId=0x12345678;
    const uint8_t mac[6] ={0x00, 0x0C, 0x29, 0x12, 0x34, 0x56};
    std::memcpy(client.mac, mac, sizeof(mac));

    Dhcp_packet_t discover{};
    fillClientHeader(discover, client);

    // Prepare client MAC and transaction ID.
    // Create the socket.
    // Build and send DISCOVER.
    // Wait for a valid OFFER.
    // Save its offered IP and server identifier.
    // Build and send REQUEST.
    // Wait for a matching ACK or NAK.
    // Print the result and close the socket.
}
