# Constraint Satisfaction Ploblem: Scheduling

CSP technology can be used to solve scheduling problems in everyday life that need to satisfy various constraints.
Here We present an example of a company `Weekly Staff Scheduling`.

## Constraints

- `Minimum Days Off`: Even robots need to recharge their batteries once in a while!
- `Maximum Consecutive Days Off`: Too much of a good thing can be bad, even days off!
- `Minimum Daily Staff`:  We don't want our employees to feel lonely at work!
- `Minimum Daily Senior Staff`: Don't leave the newbies hanging, they need someone to hold their hand!
- `Conflit`: Some employees can never get along!

## Usage

### Compile && Run
```bash
$ g++ main.cpp scheduler.h  scheduler.cpp -o main
$ ./main <input_file> [-o <output_file>]
                      [-min-days-off <value>]
                      [-max-consec-days-off <value>] 
                      [-min-daily-staff <value>]
                      [-min-daily-seniors <value>] 
                      [-conflict <worker_id> <worker_id> ...]
```

- Input file format:

```
worker_id level
worker_id level
...
-constraint-name value1 value2 ...
-constraint-name value1 value2 ...
...
```

`level` should be senior or junior

- Conflict

Note that the constraints can either be declared in the input file or as argument when running the program.

All the constraints except `-conflict` support only one value.

`-conflict 1 2 3` <==> `-conflict 1 2` and `-conflict 1 3` and `-conflict 2 3` 

`-conflict` means two or more people cannot work together the same day.

### Example

- Input file

```
1 senior
2 junior
3 junior
4 senior
5 junior
-conflict 2 4
-min-days-off 2
-max-consec-days-off 3
-min-daily-staff 3
-min-daily-seniors 1
```

- Result

```
Min days off: 2
Max consec days off: 3
Min daily staff: 3
Min daily seniors: 1
Conflicts: 
4: 2 
2: 4 

4 4 x 4 4 x 4 
x x 2 x x 2 x 
5 5 5 x 5 x 5 
3 x x 3 3 3 3 
1 1 1 1 x 1 x 

Duration: 1 ms
```