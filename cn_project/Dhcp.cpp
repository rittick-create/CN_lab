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

//Parse and validate incoming packets
struct ParsedReply {
    uint8_t messageType{};
    uint32_t offeredIp{};     // Network byte order
    uint32_t serverIp{};      // Network byte order
    uint32_t leaseSeconds{};  // Host byte order
};

//creating the dhcpDiscover packet
Dhcp_packet_t dhcpDiscover(Dhcp_packet_t discover,const ClientContext& client){
    
    discover={};//reset 

    discover.op=BOOTREQUEST;
    discover.htype=1;//ethernet
    discover.hlen=6;//mac address length
    discover.hops=0;

    discover.xid=htonl(client.transactionId);
    discover.secs=0;
    discover.flags=htons(0x8000);;

    std::memcpy(discover.chaddr,client.mac,sizeof(client.mac));

    

    return discover;

}

//adding options
std::size_t addDiscoverOptions(Dhcp_packet_t& discover) {
    discover.magic_cookie = htonl(DHCP_MAGIC_COOKIE);

    std::size_t index = 0;

    // Option 53: Identify this message as DHCPDISCOVER.
    discover.options[index++] = DHCP_OPTION_MSG_TYPE;
    discover.options[index++] = 1;  // Data length
    discover.options[index++] = DHCPDISCOVER;

    // Option 55: Request subnet mask, gateway, and DNS.
    discover.options[index++] = DHCP_OPTION_PARAM_REQ_LIST;
    discover.options[index++] = 3;  // Three option codes
    discover.options[index++] = DHCP_OPTION_SUBNET_MASK;
    discover.options[index++] = DHCP_OPTION_ROUTER;
    discover.options[index++] = DHCP_OPTION_DNS_SERVER;

    // END has no length or data bytes.
    discover.options[index++] = DHCP_OPTION_END;

    return offsetof(Dhcp_packet_t, options) + index;
}

