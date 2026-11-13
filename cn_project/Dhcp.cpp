#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <chrono>
#include <random>
#include <string>
#include <vector>
#include <cctype>

//headers to be loaded according to the os
#ifdef _WIN32
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0600
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    #ifdef _MSC_VER
        #pragma comment(lib, "Ws2_32.lib")
        #pragma comment(lib, "Iphlpapi.lib")
    #endif

    using SocketHandle = SOCKET;
    using SocketLength = int;

    constexpr SocketHandle INVALID_SOCKET_HANDLE = INVALID_SOCKET;
#else
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <ifaddrs.h>
    #include <net/if.h>
    #ifdef __APPLE__
        #include <net/if_dl.h>
    #elif defined(__linux__)
        #include <netpacket/packet.h>
    #endif

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
constexpr std::size_t DHCP_MIN_PACKET_LEN = 300; // BOOTP compatibility padding

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

// DHCP option codes=> follows [code,length ,data]
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
static_assert(sizeof(Dhcp_packet_t) >= DHCP_MIN_PACKET_LEN);

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

//DHCPDISCOVER
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

    // Option 55: Request subnet mask, gateway, DNS, lease time, and server ID.
    discover.options[index++] = DHCP_OPTION_PARAM_REQ_LIST;
    discover.options[index++] = 5;  // Five option codes
    discover.options[index++] = DHCP_OPTION_SUBNET_MASK;
    discover.options[index++] = DHCP_OPTION_ROUTER;
    discover.options[index++] = DHCP_OPTION_DNS_SERVER;
    discover.options[index++] = DHCP_OPTION_LEASE_TIME;
    discover.options[index++] = DHCP_OPTION_SERVER_ID;

    // Option 61: use the same client identifier as the earlier working version.
    discover.options[index++] = DHCP_OPTION_CLIENT_ID;
    discover.options[index++] = 7;  // Ethernet type plus six MAC bytes
    discover.options[index++] = 1;
    std::memcpy(discover.options + index, discover.chaddr, 6);
    index += 6;

    // END has no length or data bytes.
    discover.options[index++] = DHCP_OPTION_END;

    // PAD bytes after END keep short messages compatible with BOOTP receivers.
    while (offsetof(Dhcp_packet_t, options) + index < DHCP_MIN_PACKET_LEN) {
        discover.options[index++] = DHCP_OPTION_PAD;
    }

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
        int error = errno;
        std::perror(operation);
        #ifdef __APPLE__
        if (error == EHOSTUNREACH || error == EACCES) {
            std::fprintf(stderr, "If the route exists, check Local Network access for the "
                         "terminal/app in System Settings > Privacy & Security > Local Network.\n");
        }
        #else
        (void)error;
        #endif
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


struct ClientInterface {
    std::string name;
    std::string systemName;
    uint8_t mac[6]{};
    unsigned index{};
    bool macRedacted{};
};

void printMac(const uint8_t* mac) {
    std::printf("%02x:%02x:%02x:%02x:%02x:%02x",
                unsigned(mac[0]), unsigned(mac[1]), unsigned(mac[2]),
                unsigned(mac[3]), unsigned(mac[4]), unsigned(mac[5]));
}

// Read the current OS address, including a Private Wi-Fi Address when enabled.
std::vector<ClientInterface> listClientInterfaces() {
    std::vector<ClientInterface> interfaces;
#ifdef _WIN32
    std::vector<unsigned char> storage(15000);
    ULONG result = ERROR_BUFFER_OVERFLOW;
    for (int attempt = 0; attempt < 3; ++attempt) {
        ULONG size = static_cast<ULONG>(storage.size());
        auto* adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(storage.data());
        result = GetAdaptersAddresses(AF_INET, 0, nullptr, adapters, &size);
        if (result == ERROR_BUFFER_OVERFLOW) {
            storage.resize(size);
            continue;
        }
        if (result != NO_ERROR) break;
        for (auto* adapter = adapters; adapter; adapter = adapter->Next) {
            if (adapter->OperStatus != IfOperStatusUp ||
                adapter->PhysicalAddressLength != 6 || adapter->IfIndex == 0) continue;
            char friendlyName[512]{};
            if (adapter->FriendlyName) {
                WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, -1,
                                    friendlyName, sizeof(friendlyName), nullptr, nullptr);
            }
            ClientInterface interface;
            interface.systemName = adapter->AdapterName;
            interface.name = friendlyName[0] ? friendlyName : adapter->AdapterName;
            interface.index = adapter->IfIndex;
            std::memcpy(interface.mac, adapter->PhysicalAddress, 6);
            interfaces.push_back(interface);
        }
        break;
    }
    if (result != NO_ERROR) {
        std::fprintf(stderr, "GetAdaptersAddresses failed: %lu\n", result);
    }
#else
    ifaddrs* addresses = nullptr;
    if (getifaddrs(&addresses) != 0) {
        std::perror("getifaddrs");
        return interfaces;
    }
    for (auto* entry = addresses; entry; entry = entry->ifa_next) {
        if (!entry->ifa_addr || !(entry->ifa_flags & IFF_UP) ||
            (entry->ifa_flags & IFF_LOOPBACK)) continue;
        const void* mac = nullptr;
#ifdef __APPLE__
        if (entry->ifa_addr->sa_family == AF_LINK) {
            const auto* link = reinterpret_cast<const sockaddr_dl*>(entry->ifa_addr);
            if (link->sdl_alen == 6) mac = LLADDR(link);
        }
#elif defined(__linux__)
        if (entry->ifa_addr->sa_family == AF_PACKET) {
            const auto* link = reinterpret_cast<const sockaddr_ll*>(entry->ifa_addr);
            if (link->sll_halen == 6) mac = link->sll_addr;
        }
#endif
        if (!mac) continue;
        ClientInterface interface;
        interface.name = entry->ifa_name;
        interface.systemName = entry->ifa_name;
        interface.index = if_nametoindex(entry->ifa_name);
        std::memcpy(interface.mac, mac, 6);
#ifdef __APPLE__
        const uint8_t redacted[] = {2, 0, 0, 0, 0, 0};
        interface.macRedacted = std::memcmp(interface.mac, redacted, 6) == 0;
#endif
        if (interface.index != 0) interfaces.push_back(interface);
    }
    freeifaddrs(addresses);
#endif
    return interfaces;
}

bool selectClientInterface(const char* name, const uint8_t* suppliedMac,
                           ClientInterface& selected) {
    auto interfaces = listClientInterfaces();
    for (const auto& interface : interfaces) {
        if (name && interface.name != name && interface.systemName != name) continue;
        if (interface.macRedacted) {
            if (!name || !suppliedMac) continue;
            selected = interface;
            std::memcpy(selected.mac, suppliedMac, 6);
            std::printf("MAC lookup is hidden by macOS. Using the supplied MAC for %s; "
                        "verify it with ifconfig and an Ethernet-header capture.\n", name);
        } else {
            if (suppliedMac && std::memcmp(interface.mac, suppliedMac, 6) != 0) continue;
            selected = interface;
        }
        std::printf("Using interface %s, MAC ", selected.name.c_str());
        printMac(selected.mac);
        std::printf("\n");
        return true;
    }
    std::fprintf(stderr, "No active interface matches the supplied name/MAC. "
                 "A Private Wi-Fi Address may have changed.\n");
    for (const auto& interface : interfaces) {
        std::printf("Available: %s, MAC ", interface.name.c_str());
        if (interface.macRedacted) std::printf("<hidden by macOS>");
        else printMac(interface.mac);
        std::printf("\n");
    }
    std::fprintf(stderr, "Use --interface <name>. If macOS hides its MAC, also supply "
                 "the current MAC printed by /sbin/ifconfig <name>.\n");
    return false;
}

// Creating and configuring the DHCP client's UDP socket.

SocketHandle createClientSocket(const ClientInterface& interface){
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


#ifdef __APPLE__
    if (::setsockopt(sock, IPPROTO_IP, IP_BOUND_IF,
                     &interface.index, sizeof(interface.index)) != 0) {
        printSocketError("IP_BOUND_IF");
        closeClientSocket(sock);
        return INVALID_SOCKET_HANDLE;
    }
#elif defined(__linux__)
    if (::setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, interface.systemName.c_str(),
                     static_cast<socklen_t>(interface.systemName.size() + 1)) != 0) {
        printSocketError("SO_BINDTODEVICE");
        closeClientSocket(sock);
        return INVALID_SOCKET_HANDLE;
    }
#else
    (void)interface; // Windows chooses this adapter per packet using IP_PKTINFO.
#endif

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
    std::size_t packetLength,
    unsigned interfaceIndex = 0
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

#ifdef _WIN32
    // IP_PKTINFO chooses the adapter for this broadcast on Windows.
    WSABUF buffer{};
    buffer.buf = const_cast<char*>(reinterpret_cast<const char*>(&packet));
    buffer.len = static_cast<ULONG>(packetLength);
    alignas(WSACMSGHDR) char control[WSA_CMSG_SPACE(sizeof(IN_PKTINFO))]{};
    WSAMSG message{};
    message.name = reinterpret_cast<sockaddr*>(&destination);
    message.namelen = sizeof(destination);
    message.lpBuffers = &buffer;
    message.dwBufferCount = 1;
    message.Control.buf = control;
    message.Control.len = sizeof(control);
    auto* header = WSA_CMSG_FIRSTHDR(&message);
    header->cmsg_level = IPPROTO_IP;
    header->cmsg_type = IP_PKTINFO;
    header->cmsg_len = WSA_CMSG_LEN(sizeof(IN_PKTINFO));
    auto* info = reinterpret_cast<IN_PKTINFO*>(WSA_CMSG_DATA(header));
    info->ipi_ifindex = interfaceIndex;
    DWORD bytesSent = 0;
    if (WSASendMsg(sock, &message, 0, &bytesSent, nullptr, nullptr) != 0) {
        printSocketError("WSASendMsg");
        return false;
    }
    int sent = static_cast<int>(bytesSent);
#else
    (void)interfaceIndex; // The socket is already bound to the selected device.
    int sent = static_cast<int>(::sendto(
        sock,
        reinterpret_cast<const char*>(&packet),
        static_cast<int>(packetLength),
        0,
        reinterpret_cast<const sockaddr*>(&destination),
        sizeof(destination)
    ));
#endif

    if (sent < 0) {
        printSocketError("sendto");
        return false;
    }

    return sent == static_cast<int>(packetLength);
}

// Returns:
//   > 0 : received byte count
//     0 : timeout
//    -1 : socket error
//    -2 : rejected datagram (the caller can keep waiting)
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

        std::fprintf(stderr, "Ignoring UDP datagram: %d bytes, source port %u "
                     "(expected port 67 and 240..%zu bytes).\n",
                     received, unsigned(ntohs(source.sin_port)), sizeof(packet));
        return -2;
    }

    packet = {};
    std::memcpy(&packet, buffer, static_cast<std::size_t>(received));

    std::printf("Received %d bytes from %s:%u. Checking DHCP reply...\n",
                received, inet_ntoa(source.sin_addr), unsigned(ntohs(source.sin_port)));

    return received;
}

//iterprets the received bytes
// validate the header -> walk through options -> extract values ->save the result at END.
bool parseReply(const Dhcp_packet_t &packet,std::size_t receivedLength,const ClientContext& client,ParsedReply& reply){

    //check the size
    constexpr std::size_t headerLength = offsetof(Dhcp_packet_t,options);
    if(receivedLength<headerLength || receivedLength>sizeof(packet))return false;

    //check that this is a reply for our Ethernet client
    if (packet.op != BOOTREPLY ||
        packet.htype != 1 ||
        packet.hlen != sizeof(client.mac) ||
        ntohl(packet.xid) != client.transactionId ||
        ntohl(packet.magic_cookie) != DHCP_MAGIC_COOKIE ||
        std::memcmp(packet.chaddr, client.mac, sizeof(client.mac)) != 0) {
        return false;
    }

    ParsedReply result{};
    result.offeredIp=packet.yiaddr;

    const std::size_t optionsLength = receivedLength - headerLength;
    std::size_t index=0;

    while(index<optionsLength){
        uint8_t code = packet.options[index++];
        if (code == DHCP_OPTION_END) {

            bool knownType =result.messageType == DHCPOFFER ||result.messageType == DHCPACK ||result.messageType == DHCPNAK;

            if (!knownType || result.serverIp == 0) return false;
            if (result.messageType != DHCPNAK &&(result.offeredIp == 0 || result.leaseSeconds == 0))return false;

            reply=result;
            return true;

        }

        if (code == DHCP_OPTION_PAD) {
            continue;
        }

        if (index >= optionsLength) {
            return false;  // Missing length byte
        }

        uint8_t length = packet.options[index++];
        if (length > optionsLength - index) {
            return false;  // Data extends beyond received bytes
        }

        const uint8_t* data = packet.options + index;

        switch (code) {
            case DHCP_OPTION_MSG_TYPE:
                if (length != 1) return false;
                result.messageType = data[0];
                break;

            case DHCP_OPTION_SERVER_ID:
                if (length != 4) return false;
                std::memcpy(&result.serverIp, data, 4);
                break;

            case DHCP_OPTION_LEASE_TIME: {
                if (length != 4) return false;
                uint32_t lease{};
                std::memcpy(&lease, data, 4);
                result.leaseSeconds = ntohl(lease);
                break;
            }
        }

        index += length;
    }

    return false;
}

//print the  parsed msg
void printReply(const ParsedReply& reply) {
    std::printf("-------------------Printing the parsed Reply msg---------------------\n");
    in_addr address{};

    address.s_addr = reply.serverIp;
    std::printf("Server IP: %s\n", inet_ntoa(address));

    address.s_addr = reply.offeredIp;
    std::printf("Client IP: %s\n", inet_ntoa(address));

    std::printf("Message type: %u\n", unsigned(reply.messageType));
    std::printf("Lease: %u seconds\n", unsigned(reply.leaseSeconds));
}

//DHCPREQUEST
std::size_t dhcpRequest(Dhcp_packet_t &request , const ClientContext &client){
    request = {};

    //header 
    request.op=BOOTREQUEST;
    request.htype=1;//ethernet
    request.hlen=6;//mac address length
    request.hops=0;

    request.xid=htonl(client.transactionId);
    request.secs=0;
    request.flags=htons(0x8000);

    std::memcpy(request.chaddr,client.mac,sizeof(client.mac));

    //options
    request.magic_cookie = htonl(DHCP_MAGIC_COOKIE);

    std::size_t index = 0;

    // Option 53: Identify this message as DHCPrequest.
    request.options[index++] = DHCP_OPTION_MSG_TYPE;
    request.options[index++] = 1;  // Data length
    request.options[index++] = DHCPREQUEST;

    // Option 50: Requested IP address
    request.options[index++] = DHCP_OPTION_REQUESTED_IP;
    request.options[index++] = 4;
    std::memcpy(request.options + index, &client.offeredIp, 4);
    index += 4;

    // Option 54: Selected DHCP server
    request.options[index++] = DHCP_OPTION_SERVER_ID;
    request.options[index++] = 4;

    std::memcpy(request.options + index, &client.serverIp, 4);
    index += 4;


    // Option 55: Same parameters requested in DISCOVER
    request.options[index++] = DHCP_OPTION_PARAM_REQ_LIST;
    request.options[index++] = 5;
    request.options[index++] = DHCP_OPTION_SUBNET_MASK;
    request.options[index++] = DHCP_OPTION_ROUTER;
    request.options[index++] = DHCP_OPTION_DNS_SERVER;
    request.options[index++] = DHCP_OPTION_LEASE_TIME;
    request.options[index++] = DHCP_OPTION_SERVER_ID;

    // Preserve the client identifier used in DISCOVER.
    request.options[index++] = DHCP_OPTION_CLIENT_ID;
    request.options[index++] = 7;
    request.options[index++] = 1;
    std::memcpy(request.options + index, client.mac, sizeof(client.mac));
    index += sizeof(client.mac);

    request.options[index++] = DHCP_OPTION_END;

    while (offsetof(Dhcp_packet_t, options) + index < DHCP_MIN_PACKET_LEN) {
        request.options[index++] = DHCP_OPTION_PAD;
    }


    return offsetof(Dhcp_packet_t, options) + index;

}


// Initialize -> Prepare client -> Create socket
//     -> Send DISCOVER -> Accept OFFER -> Save addresses
//     -> Send REQUEST -> Handle ACK/NAK -> Cleanup

void printUsage(const char* executable) {
    std::printf("Run %s without arguments for interactive setup.\n", executable);
    std::printf("Usage: %s --interface <name> [--dry-run]\n", executable);
    std::printf("   or: %s <current-MAC> [--interface <name>] [--dry-run]\n", executable);
    std::printf("Examples: --interface en0 (macOS), --interface wlan0 (Linux),\n");
    std::printf("          --interface \"Wi-Fi\" (Windows). Use your actual device name.\n");
    std::printf("If macOS hides the interface MAC, supply both its name and current MAC.\n");
    std::printf("--dry-run prints DISCOVER bytes without binding port 68 or sending.\n");
}

int main(int argc, char* argv[]) {
    const char* interfaceName = nullptr;
    const char* macText = nullptr;
    bool dryRun = false;
    char interfaceInput[512]{};
    char macInput[64]{};

    // Running ./a.out needs no flags: ask for the required connection details.
    if (argc == 1) {
        auto interfaces = listClientInterfaces();
        std::printf("Available interfaces:");
        for (const auto& interface : interfaces) {
            std::printf(" %s", interface.name.c_str());
        }
        std::printf("\nNetwork interface (for example en0): ");
        std::fflush(stdout);
        if (!std::fgets(interfaceInput, sizeof(interfaceInput), stdin)) return 1;
        interfaceInput[std::strcspn(interfaceInput, "\r\n")] = '\0';
        if (!interfaceInput[0]) return 1;
        interfaceName = interfaceInput;

        for (const auto& interface : interfaces) {
            if ((interface.name == interfaceName || interface.systemName == interfaceName) &&
                interface.macRedacted) {
                std::printf("macOS hides this interface's MAC. Find its current ether address\n"
                            "with /sbin/ifconfig %s in another terminal.\nClient MAC: ", interfaceName);
                std::fflush(stdout);
                if (!std::fgets(macInput, sizeof(macInput), stdin)) return 1;
                macInput[std::strcspn(macInput, "\r\n")] = '\0';
                if (!macInput[0]) return 1;
                macText = macInput;
                break;
            }
        }
    }

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        if (std::strcmp(argv[i], "--dry-run") == 0) {
            dryRun = true;
        } else if (std::strcmp(argv[i], "--interface") == 0 && i + 1 < argc &&
                   argv[i + 1][0] != '-') {
            interfaceName = argv[++i];
        } else if (argv[i][0] != '-' && !macText) {
            macText = argv[i];
        } else {
            std::fprintf(stderr, "Unknown or incomplete argument: %s\n", argv[i]);
            printUsage(argv[0]);
            return 1;
        }
    }
    if (!interfaceName && !macText) {
        printUsage(argv[0]);
        return argc == 1 ? 0 : 1;
    }

    uint8_t suppliedMac[6]{};
    if (macText) {
        if (std::strlen(macText) != 17) {
            std::fprintf(stderr, "Invalid MAC: use six hexadecimal pairs separated by colons.\n");
            return 1;
        }
        for (std::size_t i = 0; i < 6; ++i) {
            std::size_t offset = i * 3;
            if (!std::isxdigit(static_cast<unsigned char>(macText[offset])) ||
                !std::isxdigit(static_cast<unsigned char>(macText[offset + 1])) ||
                (i < 5 && macText[offset + 2] != ':')) {
                std::fprintf(stderr, "Invalid MAC format.\n");
                return 1;
            }
            unsigned value = 0;
            std::sscanf(macText + offset, "%2x", &value);
            suppliedMac[i] = static_cast<uint8_t>(value);
        }
        const uint8_t zeroMac[6]{};
        const uint8_t redactedMac[6] = {2, 0, 0, 0, 0, 0};
        if ((suppliedMac[0] & 1) || std::memcmp(suppliedMac, zeroMac, 6) == 0 ||
            std::memcmp(suppliedMac, redactedMac, 6) == 0) {
            std::fprintf(stderr, "Use the actual unicast MAC, not a broadcast, zero, "
                         "or macOS redaction placeholder address.\n");
            return 1;
        }
    }

    if (!initializeNetworking()) return 1;
    ClientInterface interface;
    if (!selectClientInterface(interfaceName, macText ? suppliedMac : nullptr, interface)) {
        cleanupNetworking();
        return 1;
    }
    ClientContext client{};
    std::memcpy(client.mac, interface.mac, sizeof(client.mac));
    std::random_device random;
    std::mt19937 generator(random());
    client.transactionId = std::uniform_int_distribution<uint32_t>(1, UINT32_MAX)(generator);

    //Build and send DISCOVER.
    Dhcp_packet_t discover{};
    discover = dhcpDiscover(discover, client);
    std::size_t length = addDiscoverOptions(discover);
    if (dryRun) {
        std::printf("DISCOVER: %zu bytes, XID 0x%08x (not sent).\nOptions (hex):",
                    length, unsigned(client.transactionId));
        for (std::size_t i = 0; i < length - offsetof(Dhcp_packet_t, options); ++i) {
            std::printf(" %02x", unsigned(discover.options[i]));
        }
        std::printf("\n");
        cleanupNetworking();
        return 0;
    }

    SocketHandle sock = createClientSocket(interface);
    if (sock == INVALID_SOCKET_HANDLE) {
        cleanupNetworking();
        return 1;
    }
    if (!sendPacket(sock, discover, length, interface.index)) {
        closeClientSocket(sock);
        cleanupNetworking();
        return 1;
    }
    std::printf("Sent DHCPDISCOVER (%zu bytes, XID 0x%08x). Waiting for OFFER...\n",
                length, unsigned(client.transactionId));

    //Select the first valid OFFER. Invalid replies do not restart the deadline.
    using Clock = std::chrono::steady_clock;
    auto started = Clock::now();
    auto deadline = started + std::chrono::seconds(15);
    int discoverAttempts = 1;
    ParsedReply offer{};
    bool haveOffer = false;
    while (Clock::now() < deadline) {
        Dhcp_packet_t response{};
        int received = receivePacket(sock, response);
        if (received == -2) continue;
        if (received < 0) break;
        if (received == 0) {
            std::printf("No UDP reply arrived during the five-second receive wait.\n");
            if (discoverAttempts >= 3 || Clock::now() >= deadline) break;
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - started).count();
            discover.secs = htons(static_cast<uint16_t>(elapsed));
            if (!sendPacket(sock, discover, length, interface.index)) break;
            std::printf("Resent DHCPDISCOVER (attempt %d/3, same transaction ID).\n",
                        ++discoverAttempts);
            continue;
        }
        if (!parseReply(response, static_cast<std::size_t>(received), client, offer)) {
            std::printf("Ignoring reply: op=%u, XID=0x%08x (expected 0x%08x), "
                        "cookie %s, MAC %s; header/options validation failed.\n",
                        unsigned(response.op), unsigned(ntohl(response.xid)),
                        unsigned(client.transactionId),
                        ntohl(response.magic_cookie) == DHCP_MAGIC_COOKIE ? "matches" : "differs",
                        std::memcmp(response.chaddr, client.mac, sizeof(client.mac)) == 0
                            ? "matches" : "differs");
            continue;
        }
        if (offer.messageType != DHCPOFFER) {
            std::printf("Ignoring message type %u while waiting for OFFER.\n",
                        unsigned(offer.messageType));
            continue;
        }
        haveOffer = true;
        break;
    }
    if (!haveOffer) {
        std::fprintf(stderr, "No valid OFFER received.\n");
        closeClientSocket(sock);
        cleanupNetworking();
        return 1;
    }
    printReply(offer);
    client.offeredIp = offer.offeredIp;
    client.serverIp = offer.serverIp;

    // Build and send REQUEST with the same client identity and XID.
    Dhcp_packet_t request{};
    length = dhcpRequest(request, client);
    request.secs = discover.secs; // Preserve the selected DISCOVER's value for relays.
    if (!sendPacket(sock, request, length, interface.index)) {
        closeClientSocket(sock);
        cleanupNetworking();
        return 1;
    }
    std::printf("Sent DHCPREQUEST (%zu bytes). Waiting for ACK/NAK...\n", length);

    //Accept only a matching ACK/NAK from the selected server.
    deadline = Clock::now() + std::chrono::seconds(10);
    bool acknowledged = false;
    while (Clock::now() < deadline) {
        Dhcp_packet_t response{};
        ParsedReply reply{};
        int received = receivePacket(sock, response);
        if (received == -2) continue;
        if (received < 0) break;
        if (received == 0) {
            std::printf("Receive timed out while waiting for ACK/NAK.\n");
            continue;
        }
        if (!parseReply(response, static_cast<std::size_t>(received), client, reply) ||
            reply.serverIp != client.serverIp) continue;

        if (reply.messageType == DHCPNAK) {
            std::fprintf(stderr, "DHCPNAK: server rejected the request.\n");
            break;
        }
        if (reply.messageType != DHCPACK || reply.offeredIp != client.offeredIp) continue;

        printReply(reply);
        std::printf("Lease acknowledged. This program has not changed the OS network settings.\n");
        acknowledged = true;
        break;
    }
    if (!acknowledged) std::fprintf(stderr, "No lease accepted.\n");

    //Release local resources on either success or failure.
    closeClientSocket(sock);
    cleanupNetworking();
    return acknowledged ? 0 : 1;
}
