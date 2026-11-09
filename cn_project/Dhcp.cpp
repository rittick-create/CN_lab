#include <cstdint>
#include <cstddef>

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