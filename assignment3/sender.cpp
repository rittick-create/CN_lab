#include "CSMA.cpp"
// Reads the station number, CSMA scheme, and optional settings from the command line.
// Checks the arguments and starts one station; prints an error if it fails.
int main(int argc, char* argv[]) {
    try {
        check(argc >= 3 && argc <= 9, "Usage: ./sender_app <station> <non|one|p|cd> [frames=5] [p=0.5] [port=8080] [seed=1] [destination=other station for 0/1] [message] OR ./sender_app <station> <scheme> -1");
        // A lone -1 after the scheme means channel-only delivery, not a frame count.
        bool channelOnly = argc == 4 && string(argv[3]) == "-1";
        int station = stoi(argv[1]);
        int frames = argc > 3 && !channelOnly ? stoi(argv[3]) : 5;
        string scheme = argv[2];
        double p = argc > 4 ? stod(argv[4]) : 0.5;
        int port = argc > 5 ? stoi(argv[5]) : 8080;
        int seed = argc > 6 ? stoi(argv[6]) : 1;
        int destination = -1;
        // The short commands let stations 0 and 1 exchange messages automatically.
        if (station == 0) destination = 1;
        if (station == 1) destination = 0;
        if (argc > 7) destination = stoi(argv[7]);
        if (channelOnly) destination = -1;
        string data = argc > 8 ? argv[8] : "Hello from station " + to_string(station);
        check(destination >= -1 && data.size() < 800 && data.find_first_of("\r\n") == string::npos, "Invalid destination or message");
        check(station >= 0 && frames > 0 && p > 0 && p <= 1 && port > 0 && port <= 65535
              && validScheme(scheme), "Invalid arguments");
        runStation(port, station, scheme, frames, p, seed, destination, data);
    } catch (exception& error) {
        cerr << error.what() << '\n';
        return 1;
    }
}
