#include "CSMA.cpp"
#include <filesystem>
#include <iomanip>

struct Row {
    string scheme;
    int n, seed;
    double p;
    Metrics metrics;
};

// Starts one independent child process for each station using fork().
// Runs the shared channel in the parent, then waits for the stations to finish.
// Returns the measured results and stops remaining children if an error occurs.
Metrics experiment(int n, string scheme, double p, int frames, int seed) {
    int port = 0;
    int listener = listenChannel(port);
    vector<pid_t> children;
    try {
        for (int station = 0; station < n; station++) {
            pid_t child = fork();
            check(child >= 0, "Could not start station process");
            if (child == 0) {
                close(listener);
                try {
                    runStation(port, station, scheme, frames, p, seed);
                    _exit(0);
                } catch (exception& error) {
                    cerr << error.what() << '\n';
                    _exit(1);
                }
            }
            children.push_back(child);
        }
        ofstream deliveries("output/received.csv");
        check(bool(deliveries), "Could not open delivery file");
        Metrics metrics = runChannel(listener, n, scheme, frames, deliveries);
        for (pid_t child : children) {
            int status;
            check(waitpid(child, &status, 0) == child && WIFEXITED(status)
                  && WEXITSTATUS(status) == 0, "Station failed");
        }
        return metrics;
    } catch (...) {
        for (pid_t child : children) {
            // Reap only children that are still ours; avoid signalling a reused PID.
            int status;
            if (waitpid(child, &status, WNOHANG) == 0) {
                kill(child, SIGTERM);
                waitpid(child, &status, 0);
            }
        }
        throw;
    }
}

// Writes the supplied experiment results to a CSV file in output/.
// Each row includes the settings, collisions, delay, throughput, and efficiency.
void saveCSV(string name, const vector<Row>& rows) {
    ofstream file("output/" + name + "_results.csv");
    file << "scheme,N,p,seed,frames,slots,collisions,delay,throughput,efficiency\n";
    for (const Row& r : rows) {
        Metrics m = r.metrics;
        file << r.scheme << ',' << r.n << ',' << r.p << ',' << r.seed << ','
             << m.received << ',' << m.slots << ',' << m.collisions << ','
             << m.delay << ',' << m.throughput << ',' << m.efficiency << '\n';
    }
}

// Selects one measurement from an experiment row for plotting.
// Metric 0 means collisions, 1 means average delay, and 2 means throughput.
double value(const Row& row, int metric) {
    if (metric == 0) return row.metrics.collisions;
    if (metric == 1) return row.metrics.delay;
    return row.metrics.throughput;
}

// Creates an SVG line chart for one metric against p or the station count N.
// Averages measurements at each point and draws a separate line for each scheme.
// Saves the chart in output/ so it can be opened in a browser.
void plot(string axis, const vector<Row>& rows, const vector<double>& xs, int metric) {
    vector<string> names = {"collisions", "delay", "throughput"};
    vector<string> units = {"Collision events", "Delay (slots/frame)", "Throughput (frames/slot)"};
    vector<string> schemes = {"non", "one", "p", "cd"};
    vector<string> labels = {"Non-persistent", "1-persistent", "p-persistent", "CSMA/CD"};
    vector<string> colors = {"#2563eb", "#dc2626", "#16a34a", "#9333ea"};
    double ymax = 1;
    for (const Row& row : rows) ymax = max(ymax, value(row, metric) * 1.1);
    if (metric == 2) ymax = 0.275; // Maximum useful throughput is 1/4.
    auto x = [&](double v) { return 80 + 480 * (v - xs.front()) / (xs.back() - xs.front()); };
    auto y = [&](double v) { return 310 - 240 * v / ymax; };
    ofstream svg("output/" + axis + "_" + names[metric] + ".svg");
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"760\" height=\"390\" viewBox=\"0 0 760 390\">"
        << "<rect width=\"760\" height=\"390\" fill=\"white\"/>"
        << "<g font-family=\"Arial\" font-size=\"13\" fill=\"#222\">"
        << "<text x=\"80\" y=\"30\" font-size=\"18\">" << names[metric] << " vs " << axis
        << " (mean of 3 runs)</text>";
    svg << fixed << setprecision(3);
    for (int i = 0; i <= 5; i++) {
        double v = ymax * i / 5;
        svg << "<path d=\"M80 " << y(v) << " H560\" stroke=\"#ddd\"/>"
            << "<text x=\"72\" y=\"" << y(v) + 4 << "\" text-anchor=\"end\">" << v << "</text>";
    }
    for (double v : xs) svg << "<text x=\"" << x(v) << "\" y=\"332\" text-anchor=\"middle\">" << v << "</text>";
    svg << "<path d=\"M80 70 V310 H560\" fill=\"none\" stroke=\"#222\"/>"
        << "<text x=\"300\" y=\"365\">" << axis << "</text>"
        << "<text transform=\"translate(18 270) rotate(-90)\">" << units[metric] << "</text>";
    for (int s = 0; s < 4; s++) {
        string points;
        for (double v : xs) {
            double sum = 0;
            int count = 0;
            for (const Row& row : rows) {
                double coordinate = axis == "p" ? row.p : row.n;
                if (row.scheme == schemes[s] && coordinate == v) {
                    sum += value(row, metric);
                    count++;
                }
            }
            if (count) {
                ostringstream point;
                point << x(v) << ',' << y(sum / count) << ' ';
                points += point.str();
                svg << "<circle cx=\"" << x(v) << "\" cy=\"" << y(sum / count)
                    << "\" r=\"3\" fill=\"" << colors[s] << "\"/>";
            }
        }
        if (!points.empty()) svg << "<polyline points=\"" << points << "\" fill=\"none\" stroke=\""
            << colors[s] << "\" stroke-width=\"2\"/><text x=\"585\" y=\"" << 85 + s * 25
            << "\" fill=\"" << colors[s] << "\">" << labels[s] << "</text>";
    }
    svg << "</g></svg>\n";
}

// Runs a single experiment using defaults or settings supplied on the command line.
// With --sweep, compares all schemes and saves CSV results plus six SVG plots.
int main(int argc, char* argv[]) {
    try {
        filesystem::create_directory("output");
        if (argc == 2 && string(argv[1]) == "--sweep") {
            vector<Row> pRows, nRows;
            vector<double> ps = {0.1, 0.25, 0.5, 0.75, 1};
            double bestP = 0.1, bestThroughput = -1;
            for (double p : ps) {
                double total = 0;
                for (int seed = 1; seed <= 3; seed++) {
                    Metrics m = experiment(4, "p", p, 5, seed);
                    pRows.push_back({"p", 4, seed, p, m});
                    total += m.throughput;
                }
                if (total > bestThroughput) { bestThroughput = total; bestP = p; }
                cout << "p = " << p << " done" << endl;
            }
            for (int n : {2, 4, 8, 12}) {
                for (string scheme : {"non", "one", "p", "cd"}) {
                    for (int seed = 1; seed <= 3; seed++)
                        nRows.push_back({scheme, n, seed, bestP, experiment(n, scheme, bestP, 5, seed)});
                }
                cout << "N = " << n << " done" << endl;
            }
            saveCSV("p", pRows);
            saveCSV("N", nRows);
            for (int metric = 0; metric < 3; metric++) {
                plot("p", pRows, ps, metric);
                plot("N", nRows, {2, 4, 8, 12}, metric);
            }
            ofstream("output/best_p.txt") << "Best tested p by mean throughput at N=4: " << bestP << '\n';
            cout << "Best tested p: " << bestP << "; results saved in output/\n";
        } else {
            check(argc == 1 || (argc >= 3 && argc <= 6),
                  "Usage: ./evaluate_app [<stations> <non|one|p|cd> [frames=5] [p=0.5] [seed=1]] OR --sweep");
            int n = argc > 1 ? stoi(argv[1]) : 4;
            string scheme = argc > 2 ? argv[2] : "cd";
            int frames = argc > 3 ? stoi(argv[3]) : 5;
            double p = argc > 4 ? stod(argv[4]) : 0.5;
            int seed = argc > 5 ? stoi(argv[5]) : 1;
            check(n > 0 && n <= 128 && frames > 0 && p > 0 && p <= 1 && validScheme(scheme), "Invalid arguments");
            Metrics m = experiment(n, scheme, p, frames, seed);
            printMetrics(m);
            saveCSV("run", {{scheme, n, seed, p, m}});
        }
    } catch (exception& error) {
        cerr << error.what() << '\n';
        return 1;
    }
}
