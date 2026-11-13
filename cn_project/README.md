# macOS/Linux compilation:
```
g++ -std=c++17 Dhcp.cpp
sudo ./a.out
```

# Windows compilation with MinGW needs two libraries:
```
g++ -std=c++17 Dhcp.cpp -lws2_32 -liphlpapi
```