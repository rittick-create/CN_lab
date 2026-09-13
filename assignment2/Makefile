CXX ?= c++
BOOST_INCLUDE := $(shell if [ -d /opt/homebrew/include ]; then echo -I/opt/homebrew/include; fi)
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -pthread $(BOOST_INCLUDE)

.PHONY: all clean evaluate

all: sender_app receiver_app

sender_app: Sender/sender.cpp
	$(CXX) $(CXXFLAGS) Sender/sender.cpp -o sender_app

receiver_app: Receiver/receiver.cpp
	$(CXX) $(CXXFLAGS) Receiver/receiver.cpp -o receiver_app

evaluate: all
	python3 Evaluation/evaluate.py

clean:
	rm -f sender_app receiver_app received_output.txt
	rm -f Evaluation/received_output.txt Evaluation/results.csv
