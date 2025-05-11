#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <chrono>

using namespace std;
namespace fs = std::filesystem;

using Clauza = vector<int>;
using Formula = vector<Clauza>;

bool contine_clauza_vid(const Formula& formula) {
    for (const auto& clauza : formula)
        if (clauza.empty())
            return true;
    return false;
}

Formula aplica_literal(const Formula& formula, int literal) {
    Formula rezultat;
    for (const auto& clauza : formula) {
        if (find(clauza.begin(), clauza.end(), literal) != clauza.end())
            continue;

        Clauza noua_clauza;
        for (int lit : clauza) {
            if (lit != -literal)
                noua_clauza.push_back(lit);
        }
        rezultat.push_back(noua_clauza);
    }
    return rezultat;
}

bool dpll(Formula formula, vector<int>& solutie) {
    if (formula.empty())
        return true;

    if (contine_clauza_vid(formula))
        return false;

    int variabila = formula[0][0];

    {
        Formula f_true = aplica_literal(formula, variabila);
        if (dpll(f_true, solutie)) {
            solutie.push_back(variabila);
            return true;
        }
    }
    {
        Formula f_false = aplica_literal(formula, -variabila);
        if (dpll(f_false, solutie)) {
            solutie.push_back(-variabila);
            return true;
        }
    }

    return false;
}

// Citește o instanță DIMACS CNF și returnează formula și numărul de variabile
Formula read_dimacs(const string& filename, int& out_num_vars) {
    Formula formula;
    ifstream in(filename);
    string line;
    while (getline(in, line)) {
        if (line.empty() || line[0] == 'c')
            continue;
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

// Listează toate fișierele .cnf dintr-un director
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
        vector<int> sol;

        auto t1 = chrono::high_resolution_clock::now();
        bool sat = dpll(formula, sol);
        auto t2 = chrono::high_resolution_clock::now();
        auto duration_ms = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();

        cout << f << ": "
             << (sat ? "SAT" : "UNSAT")
             << " (" << duration_ms << " ms)" << endl;
        if (sat) ++solved;
    }
    cout << "Rezultat: " << solved << "/" << total << " instanțe SAT" << endl;
    return 0;
}