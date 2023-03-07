#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stack>

/// Domain represents the set of possible values for a variable.
/// Here it represents the possible days the worker can choose to work or not work.
struct Domain {
    // each array represents the 7 days of the week. 
    // array[i] == 1 means the day is available.
    int off[7] = { 1, 1, 1, 1, 1, 1, 1 };
    int  on[7] = { 1, 1, 1, 1, 1, 1, 1 };

    /// return the number of possible values
    static int count(int arr[7]) { return std::count(arr, arr + 7, 1); }
};

/// Worker represents a variable in the CSP.
struct Worker {
    std::string id, level;
    Domain domain;

    // static bool set_on_duty(Schedule& schedule, std::string worker_id, int day);
    // static bool set_off_duty(Schedule& schedule, std::string worker_id, int day);
};

/// Schedule representation
struct Schedule {
    // each array represents the 7 days of the week.
    // each element of the array is a set of workers
    // that work on that day (on[7]) or not (off[7])
    std::unordered_set<std::string> off[7]                                     = {};
    std::unordered_set<std::string> on[7]                                      = {};
    std::unordered_map<std::string, Worker> workers                            = {};
    std::unordered_map<std::string, std::unordered_set<std::string>> conflicts = {};
    
    // constraints with default values
    int min_days_off        = 2; // minimum number of days off
    int max_consec_days_off = 3; // maximum number of consecutive days off
    int min_daily_staff     = 3; // minimum number of daily staff required
    int min_daily_seniors   = 1; // minimum number of daily seniors required

    std::string to_string();

    /// return the number of days is woking or not working, given the on or off array
    static int count(std::unordered_set<std::string> arr[7], std::string id);
};

struct Constraint {
    /// Checks if the constraints are satisfied.
    static bool check                         ( Schedule& schedule, std::string worker_id, int day );
    /// Propagate the constraints and forward check the other workers new states. 
    static bool propagate                     ( Schedule& schedule, std::string worker_id, int day );
    
    static bool check_min_days_off            ( Schedule& schedule, std::string worker_id          );
    static bool check_max_consec_days_off     ( Schedule& schedule, std::string worker_id          );
    static bool check_min_daily_staff         ( Schedule& schedule, std::string worker_id, int day );
    static bool check_conflicts               ( Schedule& schedule, std::string worker_id, int day );
    static bool propagate_min_days_off        ( Schedule& schedule, std::string worker_id          );
    static bool propagate_max_consec_days_off ( Schedule& schedule, std::string worker_id          );
    static bool propagate_min_daily_staff     ( Schedule& schedule, std::string worker_id, int day );
    static bool propagate_conflicts           ( Schedule& schedule, std::string worker_id, int day );
};

/// Return the worker that have a non-zero but least number of available options in his domain.
/// When the returned worker domain is empty, it means that all the workers domain are empty too.
std::string mrv(std::unordered_map<std::string, Worker>& workers);

/// solve scheduling problem using MRV, Forward Checking, and Constriant Propagation to optimize the solution
bool scheduler(Schedule& schedule);

/// Reads the input file and creates the schedule.
/// file format:
///               worker_id level
///               worker_id level
///               ...
///               -constraint-name value1 value2 ...
///               -constraint-name value1 value2 ...
///               ...
///
/// The constraints are: 
///                 -conflict worker_id1 worker_id2 ...
///                 -min-days-off value
///                 -max-consec-days-off value
///                 -min-daily-staff value
///                 -min-daily-seniors value
Schedule load_file(std::string filename);

#endif // SCHEDULER_H