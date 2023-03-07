#include <iostream>
#include <fstream>
#include <chrono>

#include "scheduler.h"

using namespace std;

int main(int argc, char* argv[]) {
    string output_file = "";
    Schedule schedule;

    try {
        string filename = argv[1];
        schedule = load_file(filename);

        for (int i = 2; i < argc; i++) {
            string arg = argv[i];
            if (arg == "-o") {
                output_file = argv[++i];
            }
            else if (arg == "-min-days-off") {
                schedule.min_days_off = stoi(argv[++i]);
            }
            else if (arg == "-max-consec-days-off") {
                schedule.max_consec_days_off = stoi(argv[++i]);
            }
            else if (arg == "-min-daily-staff") {
                schedule.min_daily_staff = stoi(argv[++i]);
            }
            else if (arg == "-min-daily-seniors") {
                schedule.min_daily_seniors = stoi(argv[++i]);
            }
            else if (arg == "-conflict") {
                vector<string> ids;
                while (++i < argc && argv[i][0] != '-') {
                    ids.push_back(argv[i]);
                }
                --i;
                for (auto& id : ids) {
                    schedule.conflicts[id].insert(ids.begin(), ids.end());
                    schedule.conflicts[id].erase(id);
                }
            }
        }
    }
    catch (const exception& e) {
        cout << e.what() << endl;
        cout << "Usage: " << endl
             << "$ g++ main.cpp scheduler.h  scheduler.cpp -o main" << endl
             << "$ ./main <input_file> [-o <output_file>] [-min-days-off <value>]" << endl
             << "                   [-max-consec-days-off <value>] [-min-daily-staff <value>]" << endl
             << "                   [-min-daily-seniors <value>] [-conflict <worker_id> <worker_id> ...]" << endl
             << endl;
        return 1;
    }

    // print constraints
    cout << "Min days off: " << schedule.min_days_off << endl;
    cout << "Max consec days off: " << schedule.max_consec_days_off << endl;
    cout << "Min daily staff: " << schedule.min_daily_staff << endl;
    cout << "Min daily seniors: " << schedule.min_daily_seniors << endl;
    cout << "Conflicts: " << endl;
    for (auto& pair : schedule.conflicts) {
        cout << pair.first << ": ";
        for (auto& id : pair.second) {
            cout << id << " ";
        }
        cout << endl;
    }
    cout << endl;

    ofstream fout = ofstream(output_file);
    ostream& out = output_file == "" ? cout : fout;

    auto start_time = chrono::high_resolution_clock::now();
    bool success = scheduler(schedule);
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();

    if (success)
        out << schedule.to_string();
    else
        out << "No solution found." << endl;

    cout << duration << "ms" << endl;
    return 0;
}