CXX ?= c++
BOOST_INCLUDE := $(shell if [ -d /opt/homebrew/include ]; then echo -I/opt/homebrew/include; fi)
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -pthread $(BOOST_INCLUDE)

.PHONY: all clean

all: sender_app receiver_app

sender_app: Sender/sender.cpp Sender/Frame.cpp Sender/Channel.cpp CommonFunctions/Binary.cpp CommonFunctions/SocketConnection.cpp ErrorDetection/CRC.cpp
	$(CXX) $(CXXFLAGS) Sender/sender.cpp -o sender_app

receiver_app: Receiver/receiver.cpp Sender/Frame.cpp CommonFunctions/Binary.cpp CommonFunctions/SocketConnection.cpp ErrorDetection/CRC.cpp
	$(CXX) $(CXXFLAGS) Receiver/receiver.cpp -o receiver_app

clean:
	rm -f sender_app receiver_app received_output.txt
