#include <boost/asio.hpp>
#include <chrono>
#include <string>
#include <thread>

using namespace std;
using boost::asio::ip::tcp;

// Adds newline-delimited messages and timeouts to a TCP socket.
class SocketConnection {
private:
    tcp::socket& socket;
    string pendingInput;

public:
    SocketConnection(tcp::socket& connectedSocket) : socket(connectedSocket) {
        pendingInput = "";
    }

    bool sendLine(string line) {
        boost::system::error_code error;
        socket.non_blocking(false, error);

        if (error) {
            return false;
        }

        line += '\n';
        boost::asio::write(socket, boost::asio::buffer(line), error);
        return !error;
    }

    // Wait for one complete line for at most timeoutMilliseconds.
    bool receiveLine(string& line, int timeoutMilliseconds) {
        size_t newlinePosition = pendingInput.find('\n');

        if (newlinePosition != string::npos) {
            line = pendingInput.substr(0, newlinePosition);
            pendingInput.erase(0, newlinePosition + 1);
            return true;
        }

        boost::system::error_code error;
        socket.non_blocking(true, error);

        if (error) {
            return false;
        }

        chrono::steady_clock::time_point deadline =
            chrono::steady_clock::now() +
            chrono::milliseconds(timeoutMilliseconds);

        while (chrono::steady_clock::now() < deadline) {
            char buffer[2048];
            size_t bytesRead = socket.read_some(
                boost::asio::buffer(buffer), error
            );

            if (!error) {
                pendingInput.append(buffer, bytesRead);
                newlinePosition = pendingInput.find('\n');

                if (newlinePosition != string::npos) {
                    line = pendingInput.substr(0, newlinePosition);
                    pendingInput.erase(0, newlinePosition + 1);
                    return true;
                }
            }
            else if (error == boost::asio::error::would_block ||
                     error == boost::asio::error::try_again) {
                error.clear();
                this_thread::sleep_for(chrono::milliseconds(2));
            }
            else {
                return false;
            }
        }

        return false;
    }
};
