#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <chrono>
#include <stack>
#include <optional>

using namespace std;
namespace fs = std::filesystem;

using Clauza = vector<int>;
using Formula = vector<Clauza>;

enum class Assign { UNDEF = 0, TRUE = 1, FALSE = -1 };

struct Solver {
    Formula clauses;
    int num_vars;
    vector<Assign> assigns;
    vector<int> level;
    int decision_level = 0;
    unordered_map<int, vector<int>> watches;
    vector<int> trail;
    vector<int> trail_limits;

    Solver(const Formula& f, int n) : clauses(f), num_vars(n) {
        assigns.resize(n+1, Assign::UNDEF);
        level.resize(n+1, -1);
        init_watches();
    }

    void init_watches() {
        for (int i = 0; i < clauses.size(); ++i) {
            const auto& c = clauses[i];
            if (c.size() >= 2) {
                watches[c[0]].push_back(i);
                watches[c[1]].push_back(i);
            }
        }
    }

    bool pick_branch_var(int& lit) {
        for (int v = 1; v <= num_vars; ++v) {
            if (assigns[v] == Assign::UNDEF) {
                lit = v;
                return true;
            }
        }
        return false;
    }

    void enqueue(int lit) {
        int var = abs(lit);
        assigns[var] = lit > 0 ? Assign::TRUE : Assign::FALSE;
        level[var] = decision_level;
        trail.push_back(lit);
    }

    optional<pair<Clauza,int>> propagate() {
        size_t qi = 0;
        while (qi < trail.size()) {
            int lit = trail[qi++];
            int neg = -lit;
            auto ws = watches[neg];
            for (int ci : ws) {
                auto& c = clauses[ci];
                if (!move_watch(ci, c, neg)) {
                    return make_pair(c, neg);
                }
            }
        }
        return nullopt;
    }

    bool move_watch(int ci, Clauza& c, int lit) {
        for (int l : c) {
            if (l == lit) continue;
            int var = abs(l);
            if (assigns[var] == Assign::UNDEF || (assigns[var] == Assign::TRUE && l > 0) || (assigns[var] == Assign::FALSE && l < 0)) {
                watches[l].push_back(ci);
                auto& ws = watches[lit];
                ws.erase(remove(ws.begin(), ws.end(), ci), ws.end());
                return true;
            }
        }
        int unassigned = 0, count = 0;
        for (int l : c) {
            int v = abs(l);
            if (assigns[v] == Assign::UNDEF) {
                unassigned = l;
                ++count;
            } else if ((assigns[v] == Assign::TRUE && l > 0) || (assigns[v] == Assign::FALSE && l < 0)) {
                return true; 
            }
        }
        if (count == 1) {
            enqueue(unassigned);
            return true;
        }
        return false;
    }

    int analyze_conflict(const Clauza& conflict_clause) {
        if (decision_level == 0) return -1;
        return decision_level - 1;
    }

    bool solve() {
        if (auto confl = propagate()) return false;
        while (true) {
            int lit;
            if (!pick_branch_var(lit)) return true;
            decision_level++;
            trail_limits.push_back(trail.size());
            enqueue(lit);
            while (true) {
                auto conflict = propagate();
                if (!conflict) break;
                int back_level = analyze_conflict(conflict->first);
                if (back_level < 0) return false;
                backtrack(back_level);
            }
        }
    }

    void backtrack(int lvl) {
        int limit = lvl == 0 ? 0 : trail_limits[lvl-1];
        for (int i = trail.size()-1; i >= limit; --i) {
            int lit = trail[i];
            assigns[abs(lit)] = Assign::UNDEF;
            level[abs(lit)] = -1;
        }
        trail.resize(limit);
        trail_limits.resize(lvl);
        decision_level = lvl;
    }
};

Formula read_dimacs(const string& filename, int& out_num_vars) {
    Formula formula;
    ifstream in(filename);
    string line;
    while (getline(in, line)) {
        if (line.empty() || line[0] == 'c') continue;
        if (line[0] == 'p') {
            istringstream iss(line);
            string tmp;
            int n_vars, n_clauses;
            iss >> tmp >> tmp >> n_vars >> n_clauses;
            out_num_vars = n_vars;
        } else {
            istringstream iss(line);
            Clauza cl;
            int lit;
            while (iss >> lit && lit != 0)
                cl.push_back(lit);
            formula.push_back(cl);
        }
    }
    return formula;
}

vector<string> list_cnf_files(const string& dir) {
    vector<string> files;
    for (auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".cnf")
            files.push_back(entry.path().string());
    }
    return files;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <cnf_directory>" << endl;
        return 1;
    }
    string dir = argv[1];
    auto files = list_cnf_files(dir);
    int total = files.size(), solved = 0;
    for (const auto& f : files) {
        int n_vars = 0;
        Formula formula = read_dimacs(f, n_vars);
        Solver solver(formula, n_vars);

        auto t1 = chrono::high_resolution_clock::now();
        bool sat = solver.solve();
        auto t2 = chrono::high_resolution_clock::now();
        auto duration_ms = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();

        cout << f << ": " << (sat ? "SAT" : "UNSAT") << " (" << duration_ms << " ms)" << endl;
        if (sat) ++solved;
    }
    cout << "Rezultat: " << solved << "/" << total << " instanțe SAT" << endl;
    return 0;
}