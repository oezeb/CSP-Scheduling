#include "scheduler.h"

/// Return the worker that have a non-zero but least number of available options in his domain.
/// When the returned worker domain is empty, it means that all the workers domain are empty too.
std::string mrv(std::unordered_map<std::string, Worker>& workers) { // minimun remaining values
    std::string mrv_id;
    for (auto& p : workers) {
        if (mrv_id.empty()) mrv_id = p.first;
        else {
            int rv_count = Domain::count(p.second.domain.on) + Domain::count(p.second.domain.off);
            int mrv_count = Domain::count(workers[mrv_id].domain.on) + Domain::count(workers[mrv_id].domain.off);

            if (mrv_count == 0 || (rv_count != 0 && rv_count < mrv_count))
                mrv_id = p.first;
        }
    }
    return mrv_id;
}

/// solve scheduling problem using MRV, Forward Checking, and Constriant Propagation to optimize the solution
bool scheduler(Schedule& schedule) {
    if (schedule.workers.empty())
        return true;

    // get the worker with the least number of available options (MRV)
    std::string worker_id = mrv(schedule.workers);
    Worker& worker = schedule.workers[worker_id];

    int on_count = Domain::count(worker.domain.on);
    int off_count = Domain::count(worker.domain.off);

    // all workers domain are empty, found a solution
    if (on_count == 0 && off_count == 0)
        return true;

    // find an assignment available options for the worker
    for (int i = 0; i < 7; i++) {
        if (worker.domain.on[i] == 1) { // worker can choose to be on duty
            // Saving worker current state. We already know that worker.domain.on[i] == 1
            int off = worker.domain.off[i];

            // Try to put the worker on duty
            schedule.on[i].insert(worker_id);
            worker.domain.on[i] = 0;
            worker.domain.off[i] = 0;

            // Forward check the current state of the worker still satisfies the constraints
            if (Constraint::check(schedule, worker_id, i)) {
                // Propagate the changes to the other workers.
                // The propagate function will forward check the new state of the other workers
                // And return true if their new states are still valid
                // Therefore, we use a copy of the schedule to propagate the changes.
                // So when propagate returns false, there will be no need to rollback the changes
                Schedule new_schedule = schedule;
                if(Constraint::propagate(new_schedule, worker_id, i)) {
                    // Passed the propagation check, recursively call the function
                    if (scheduler(new_schedule)) {
                        schedule = new_schedule;
                        return true;
                    }
                }
            }

            // The assignment failed, so rollback the changes
            schedule.on[i].erase(worker_id);
            worker.domain.on[i] = 1;
            worker.domain.off[i] = off;
        }


        if (worker.domain.off[i] == 1) { // worker can choose to be off duty
            // Same as above
            int on = worker.domain.on[i];

            schedule.off[i].insert(worker_id);
            worker.domain.on[i] = 0;
            worker.domain.off[i] = 0;

            if (Constraint::check(schedule, worker_id, i)) {
                Schedule new_schedule = schedule;
                if(Constraint::propagate(new_schedule, worker_id, i)) {
                    if (scheduler(new_schedule)) {
                        schedule = new_schedule;
                        return true;
                    }
                }
            }

            schedule.off[i].erase(worker_id);
            worker.domain.on[i] = on;
            worker.domain.off[i] = 1;
        }
    }
    return false;
}

/*------------------------------------------------------- Schedule -------------------------------------------------------*/

/// print the schedule
std::string Schedule::to_string() {
    std::string s = "";
    for (auto& p : workers) {
        auto& id = p.first;
        for (int i = 0; i < 7; i++) {
            if (on[i].count(id))
                s += id + " ";
            else if (off[i].count(id))
                s += "x ";
            else
                s += "- ";
        }
        s += "\n";
    }
    return s;
}

/// Return the number of days that the worker is on or off duty,
/// given the schedule for on or off duty
int Schedule::count(std::unordered_set<std::string> arr[7], std::string id) {
    int count = 0;
    for (int i = 0; i < 7; i++)
        if (arr[i].count(id))
            count++;
    return count;
}

/*------------------------------------------------------- Constraint -------------------------------------------------------*/

/// Checks if the constraints are satisfied.
bool Constraint::check(Schedule& schedule, std::string worker_id, int day) {
    return check_min_days_off(schedule, worker_id) &&
              check_max_consec_days_off(schedule, worker_id) &&
                check_min_daily_staff(schedule, worker_id, day) && // check both min daily staff and min daily senior staff
                        check_conflicts(schedule, worker_id, day);
}

/// Propagate the constraints and forward check the other workers new states.
bool Constraint::propagate(Schedule& schedule, std::string worker_id, int day) {
    return propagate_min_days_off(schedule, worker_id) &&
                propagate_max_consec_days_off(schedule, worker_id) &&
                    propagate_min_daily_staff(schedule, worker_id, day) &&
                            propagate_conflicts(schedule, worker_id, day);
}

bool Constraint::check_min_days_off(Schedule& schedule, std::string worker_id) {
    // The minimum days off satisfied if sum of the remaining options for the worker to be off duty (domain)
    // and the actual off duty (schedule) days is greater than or equal to the minimum days off
    return Domain::count(schedule.workers[worker_id].domain.off) +
                Schedule::count(schedule.off, worker_id) >=
                    schedule.min_days_off;
}

bool Constraint::check_max_consec_days_off(Schedule& schedule, std::string worker_id) {
    // count the number of consecutive days that the worker is off duty
    int count = 0;
    for (int i = 0; i < 7; i++) {
        if (schedule.off[i].count(worker_id) > 0) count++;
        else count = 0;

        if (count >= schedule.max_consec_days_off) // exceeds the maximum consecutive days off
            return false;
    }
    return true;
}

bool Constraint::check_min_daily_staff(Schedule& schedule, std::string worker_id, int day) {
    int workers_count = 0;
    int seniors_count = 0;
    for (auto& p : schedule.workers) {
        auto& w = p.second;
        // count the worker already in the schedule and those who still have options to be on duty
        if (w.domain.on[day] || schedule.on[day].count(w.id) > 0) {
            if (w.level == "senior")
                seniors_count++;
            workers_count++;
        }
    }
    return workers_count >= schedule.min_daily_staff && 
        seniors_count >= schedule.min_daily_seniors;
}

bool Constraint::check_conflicts(Schedule& schedule, std::string worker_id, int day) {
    if (schedule.on[day].count(worker_id) > 0) { // current worker is on duty
        // check if any other worker with whom he has conflicts is on duty too.
        for (auto& id : schedule.conflicts[worker_id])
            if (schedule.on[day].count(id) > 0)
                return false;
    }
    return true;
}

bool Constraint::propagate_min_days_off(Schedule& schedule, std::string worker_id) {
    Worker& worker = schedule.workers[worker_id];
    // count the number of days the worker can be or is off duty
    int count = Domain::count(worker.domain.off) + 
                Schedule::count(schedule.off, worker_id);

    if (count == schedule.min_days_off) { 
        // This worker has already reached the minimum
        // Therefore, for any remaining options to be off duty in his domain, he should be off duty.
        for (int i = 0; i < 7; i++) {
            if (worker.domain.off[i] == 1) { // available off duty option
                if (worker.domain.on[i] == 1) { // available on duty option
                    worker.domain.on[i] = 0; // remove the on duty option

                    // forward check if he can be off duty
                    worker.domain.off[i] = 0;
                    schedule.off[i].insert(worker.id);
                    if (Constraint::check(schedule, worker.id, i) == false)
                        return false;
                    worker.domain.off[i] = 1;
                    schedule.off[i].erase(worker.id);
                }
            }
        }
    }
    return true;
}

bool Constraint::propagate_max_consec_days_off(Schedule& schedule, std::string worker_id) {
    // Here we check that all remaining options to be off will not violate the maximum consecutive days off
    // If it doesn't we propagate the constraint otherwise return false
    Worker& worker = schedule.workers[worker_id];

    // Use an array to record status of the worker in a week
    // 0: unknown, 1: off duty (schedule), 2: not off duty but still have the option (domain)
    int arr[7] = {0};
    for (int i = 0; i < 7; i++) {
        if (schedule.off[i].count(worker_id) > 0)
            arr[i] = 1;
        if (worker.domain.off[i] == 1)
            arr[i] = 2;
    }

    // for a given day if the worker has the off duty option,
    // check when assigned off duty, the number of consecutive days off is still within the limit
    for (int k = 0; k < 7; k++) {
        if (arr[k] == 2) { // can be off duty for the kth day
            // count the number of already off days arround the kth day  
            int i = k -1, j = k + 1;
            while (i >= 0 && arr[i] == 1)
                i--;
            while (j < 7 && arr[j] == 1)
                j++;
                    
            // So, when assigned off duty in the kth day, all [i+1, j-1] days are off.
            // if those days number already reaches the limit, then the ith and jth days should be on duty
            int count = j - i - 1;
            if (count + 1 >= schedule.max_consec_days_off) { // exceeds the maximum consecutive days off
                if (i >= 0) {
                    if (worker.domain.off[i] == 1) {
                        worker.domain.off[i] = 0; // remove the off duty option

                        // forward check if he can be on duty
                        int on = worker.domain.on[i];

                        worker.domain.on[i] = 0;
                        schedule.on[i].insert(worker.id);
                        if (Constraint::check(schedule, worker.id, i) == false)
                            return false;
                        worker.domain.on[i] = on;
                        schedule.on[i].erase(worker.id);
                    }
                }

                if (j < 7) {
                    if (worker.domain.off[j] == 1) {
                        worker.domain.off[j] = 0; // remove the off duty option

                        int tmp = worker.domain.on[j];
                        worker.domain.on[j] = 0;
                        schedule.on[j].insert(worker.id);
                        if (Constraint::check(schedule, worker.id, j) == false)
                            return false;
                        worker.domain.on[j] = tmp;
                        schedule.on[j].erase(worker.id);
                    }
                }
            }
        }
    }
    return true;
}

bool Constraint::propagate_min_daily_staff(Schedule& schedule, std::string worker_id, int day) {
    int workers_count = 0;
    int seniors_count = 0;
    for (auto& p : schedule.workers) {
        auto& w = p.second;
        // count the worker already in the schedule and those who still have options to be on duty
        if (w.domain.on[day] || schedule.on[day].count(w.id) > 0) {
            if (w.level == "senior")
                seniors_count++;
            workers_count++;
        }
    }

    if (workers_count == schedule.min_daily_staff) { // already reached the minimum
        // Therefore, all workers who still have the on duty option should be on duty (remove the off duty option)
        for (auto& p : schedule.workers) {
            if (p.second.domain.on[day] == 1) { // available on duty option
                if (p.second.domain.off[day] == 1) { // available off duty option
                    p.second.domain.off[day] = 0; // remove the off duty option

                    // forward check if he can be on duty
                    p.second.domain.on[day] = 0;
                    schedule.on[day].insert(p.first);
                    if (Constraint::check(schedule, p.first, day) == false)
                        return false;
                    p.second.domain.on[day] = 1;
                    schedule.on[day].erase(p.first);
                }
            }
        }
    }
    // use else because if the minimum daily staff is already reached, 
    // remaining seniors will automatically be on duty
    else if (seniors_count == schedule.min_daily_seniors) {
        // Same as above, but for seniors
        for (auto& p : schedule.workers) {
            if (p.second.level == "senior" && p.second.domain.on[day] == 1) {
                if (p.second.domain.off[day] == 1) {
                    p.second.domain.off[day] = 0;

                    p.second.domain.on[day] = 0;
                    schedule.on[day].insert(p.first);
                    if (Constraint::check(schedule, p.first, day) == false)
                        return false;
                    p.second.domain.on[day] = 1;
                    schedule.on[day].erase(p.first);
                }
            }
        }
    }
    return true;
}

bool Constraint::propagate_conflicts(Schedule& schedule, std::string worker_id, int day) {
    if (schedule.on[day].count(worker_id)) { // current worker is on duty for the given day
        for (auto& id : schedule.conflicts[worker_id]) {
            auto& w = schedule.workers[id];
            if (w.domain.on[day] == 1) { // available on duty option
                w.domain.on[day] = 0; // remove the on duty option, because it conflicts with the current worker
            
                // check conflicts for all days
                // the other days changes will be rolled back in the end, but the current day will be changed
                // on[7] and off[7] will save the current status
                int on[7] = {0};
                int off[7] = {0};
                for (int i = 0; i < 7; i++) {
                    on[i] = w.domain.on[i];
                    off[i] = w.domain.off[i];

                    if (schedule.on[i].count(worker_id)) { // current worker is on duty for the ith day
                        // set the conflict worker to off duty
                        w.domain.on[i] = 0;
                        w.domain.off[i] = 0;
                        schedule.off[i].insert(w.id);
                    }
                }

                // forward checking
                if (Constraint::check(schedule, w.id, day) == false)
                    return false;
                
                // roll back the changes
                for (int i = 0; i < 7; i++) {
                    if (schedule.on[i].count(worker_id)) {
                        w.domain.on[i] = on[i];
                        w.domain.off[i] = off[i];
                        schedule.off[i].erase(w.id);
                    }
                }
            }
        }
    }
    return true;
}

/*---------------------------------------------------------------------------------------------------------------------*/

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
Schedule load_file(std::string filename) {
    Schedule schedule;
    std::ifstream in(filename);
    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string s;
        ss >> s;
        if (s.empty()) continue;

        if (s[0] == '-') {
            // constraint
            if (s == "-min-days-off") {
                ss >> schedule.min_days_off;
            } else if (s == "-max-consec-days-off") {
                ss >> schedule.max_consec_days_off;
            } else if (s == "-min-daily-staff") {
                ss >> schedule.min_daily_staff;
            } else if (s == "-min-daily-seniors") {
                ss >>schedule. min_daily_seniors;
            }
            else if (s == "-conflict") {
                std::vector<std::string> ids;
                while (ss >> s)
                    ids.push_back(s);
                
                for (auto& id : ids) {
                    schedule.conflicts[id].insert(ids.begin(), ids.end());
                    schedule.conflicts[id].erase(id);
                }
            }
        } 
        else {
            // worker
            Worker worker;
            worker.id = s;
            ss >> worker.level;
            schedule.workers[s] = worker;
        }
    }
    return schedule;
}