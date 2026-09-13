#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using boost::asio::ip::tcp;

// Send all frames to the receiver and read its true/false result.
bool sendFramesToReceiver(vector<string>& transmittedFrames,
                          string receiverAddress,
                          string receiverPort,
                          bool& receiverResult) {
    try {
        boost::asio::io_context ioContext;
        tcp::resolver resolver(ioContext);
        tcp::socket socket(ioContext);

        tcp::resolver::results_type endpoints = resolver.resolve(
            receiverAddress, receiverPort
        );
        boost::asio::connect(socket, endpoints);

        // First send the number of frames, then send every frame.
        string message = to_string(transmittedFrames.size()) + "\n";
        int totalFrames = transmittedFrames.size();

        for (int i = 0; i < totalFrames; i++) {
            message += transmittedFrames[i] + "\n";
        }

        boost::asio::write(socket, boost::asio::buffer(message));

        // Wait for the receiver's result.
        boost::asio::streambuf responseBuffer;
        boost::asio::read_until(socket, responseBuffer, '\n');

        istream responseStream(&responseBuffer);
        string response;
        getline(responseStream, response);

        if (response == "true") {
            receiverResult = true;
        }
        else if (response == "false") {
            receiverResult = false;
        }
        else {
            cerr << "Error: invalid response from receiver\n";
            return false;
        }

        return true;
    }
    catch (exception& error) {
        cerr << "Socket error: " << error.what() << '\n';
        return false;
    }
}
