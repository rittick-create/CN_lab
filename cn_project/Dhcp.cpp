#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cerrno>

//headers to be loaded according to the os
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>

    using SocketHandle = SOCKET;
    using SocketLength = int;

    constexpr SocketHandle INVALID_SOCKET_HANDLE = INVALID_SOCKET;
#else
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>

    using SocketHandle = int;
    using SocketLength = socklen_t;

    constexpr SocketHandle INVALID_SOCKET_HANDLE = -1;
#endif


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

//checks packet's memory layout at compile time
//from packet start till magic cookies fixed sixe of 236 bytes
//magic cookie of 4 bytes
static_assert(offsetof(Dhcp_packet_t, magic_cookie) == 236);
static_assert(offsetof(Dhcp_packet_t, options) == 240);

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
    discover.flags=htons(0x8000);

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

// Initialize networking
//     ↓
// Create UDP socket
//     ↓
// Enable broadcast + set timeout + bind port 68
//     ↓
// Send DISCOVER to port 67
//     ↓
// Receive reply
//     ↓
// Close socket and clean up

//Printing the most recent socket error
void printSocketError(const char *operation)
{
    #ifdef _WIN32
        std::fprintf(
            stderr, "%s failed: %d\n",
            operation, WSAGetLastError());
    #else
        std::perror(operation);
    #endif

}


//Windows uses Winsock initialization
//macOS/Linux need no explicit initialization
bool initializeNetworking(){
    #ifdef _WIN32
        WSADATA  data{};
        int result = WSAStartup(MAKEWORD(2, 2), &data);
        if (result != 0) {
        std::fprintf(stderr, "WSAStartup failed: %d\n", result);
        return false;
        }
    #endif   
        return true;
}

// Close socket.
void closeClientSocket(SocketHandle sock) {
    if (sock == INVALID_SOCKET_HANDLE) {
        return;
    }

#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

// Balance successful Windows initialization.
void cleanupNetworking() {
#ifdef _WIN32
    WSACleanup();
#endif
}


//Creating and configuring the DHCP clinet's UDP socket.

SocketHandle createClientSocket(){
    SocketHandle sock = ::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);//Selects ipv4,datagram socket and udp
    
    if(sock == INVALID_SOCKET_HANDLE){
        printSocketError("socket");
        return INVALID_SOCKET_HANDLE;
    }

    //permit sending to broadcast addresses
    int enabled=1;

    //enables broadcast on sock
    if (::setsockopt(sock, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char *>(&enabled), sizeof(enabled)) != 0)
    {
        printSocketError("SO_BROADCAST");
        closeClientSocket(sock);
        return INVALID_SOCKET_HANDLE;
    }

    /*
    takes an already-created UDP socket, sets how 
    long it should wait for a reply, 
    and gives it a local port on which to receive DHCP messages.
    */
#ifdef _WIN32
    DWORD timeout = 5000;  // Milliseconds
#else
    timeval timeout{};
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
#endif

    if (::setsockopt(
            sock,
            SOL_SOCKET,
            SO_RCVTIMEO,
            reinterpret_cast<const char*>(&timeout),
            sizeof(timeout)
        ) != 0) {

        printSocketError("SO_RCVTIMEO");
        closeClientSocket(sock);
        return INVALID_SOCKET_HANDLE;
    }

    // Local endpoint: all local IPv4 addresses, client port 68.
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(68);
    local.sin_addr.s_addr = htonl(INADDR_ANY);

    if (::bind(
            sock,
            reinterpret_cast<const sockaddr*>(&local),
            sizeof(local)
        ) != 0) {

        printSocketError("bind port 68");
        closeClientSocket(sock);
        return INVALID_SOCKET_HANDLE;
    }

    return sock;

}


bool sendPacket(
    SocketHandle sock,
    const Dhcp_packet_t& packet,
    std::size_t packetLength
) {
    if (packetLength < offsetof(Dhcp_packet_t, options) ||
        packetLength > sizeof(packet)) {
        std::fprintf(stderr, "Invalid packet length\n");
        return false;
    }

    // Destination: broadcast IP, DHCP server port 67.
    sockaddr_in destination{};
    destination.sin_family = AF_INET;
    destination.sin_port = htons(67);
    destination.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    int sent = static_cast<int>(::sendto(
        sock,
        reinterpret_cast<const char*>(&packet),
        static_cast<int>(packetLength),
        0,
        reinterpret_cast<const sockaddr*>(&destination),
        sizeof(destination)
    ));

    if (sent < 0) {
        printSocketError("sendto");
        return false;
    }

    return sent == static_cast<int>(packetLength);
}

// Returns:
//   > 0 : received byte count
//     0 : timeout
//    -1 : error or rejected datagram
int receivePacket(SocketHandle sock, Dhcp_packet_t& packet) {
    // Receive the whole datagram before checking our structure's capacity.
    char buffer[65536];

    sockaddr_in source{};
    SocketLength sourceLength = sizeof(source);

    int received = static_cast<int>(::recvfrom(
        sock,
        buffer,
        sizeof(buffer),
        0,
        reinterpret_cast<sockaddr*>(&source),
        &sourceLength
    ));

    if (received < 0) {
#ifdef _WIN32
        int error = WSAGetLastError();

        if (error == WSAETIMEDOUT || error == WSAEWOULDBLOCK) {
            return 0;
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
#endif

        printSocketError("recvfrom");
        return -1;
    }

    if (source.sin_family != AF_INET ||
        ntohs(source.sin_port) != 67 ||
        received < static_cast<int>(offsetof(Dhcp_packet_t, options)) ||
        received > static_cast<int>(sizeof(packet))) {

        std::fprintf(stderr, "Rejected unexpected UDP datagram\n");
        return -1;
    }

    packet = {};
    std::memcpy(&packet, buffer, static_cast<std::size_t>(received));

    return received;
}

