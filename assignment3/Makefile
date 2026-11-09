CXX ?= c++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic

.PHONY: all clean
all: sender_app receiver_app evaluate_app

sender_app: sender.cpp CSMA.cpp
	$(CXX) $(CXXFLAGS) sender.cpp -o $@
receiver_app: receiver.cpp CSMA.cpp
	$(CXX) $(CXXFLAGS) receiver.cpp -o $@
evaluate_app: evaluate.cpp CSMA.cpp
	$(CXX) $(CXXFLAGS) evaluate.cpp -o $@
clean:
	rm -f sender_app receiver_app evaluate_app
