#include "CSMA.cpp"
#include <filesystem>
// Reads the station count, CSMA scheme, frame count, and port from the command line.
// Starts the shared channel, saves received frame numbers, and prints the final metrics.
int main(int argc, char* argv[]) {
    try {
        check(argc >= 3 && argc <= 7, "Usage: ./receiver_app <stations> <non|one|p|cd> [frames=5] [port=8080] [transmission=4] [propagation=1]");
        int n = stoi(argv[1]), frames = argc > 3 ? stoi(argv[3]) : 5;
        int port = argc > 4 ? stoi(argv[4]) : 8080;
        string scheme = argv[2];
        int transmission = argc > 5 ? stoi(argv[5]) : 4;
        int propagation = argc > 6 ? stoi(argv[6]) : 1;
        check(transmission > 0 && transmission <= 10000 && propagation > 0 && propagation <= 10000, "Times must be 1 to 10000 slots");
        check(n > 0 && n <= 128 && frames > 0 && port > 0 && port <= 65535
              && validScheme(scheme), "Invalid arguments");
        filesystem::create_directory("output");
        ofstream deliveries("output/received.csv");
        check(bool(deliveries), "Could not open output file");
        int listener = listenChannel(port);
        cout << "Channel ready on port " << port << endl;
        printMetrics(runChannel(listener, n, scheme, frames, deliveries, transmission, propagation));
    } catch (exception& error) {
        cerr << error.what() << '\n';
        return 1;
    }
}
