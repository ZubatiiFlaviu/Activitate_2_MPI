import os
import sys
import time
import multiprocessing


def dp(formula, simboluri):
    if not formula:
        return True
    if [] in formula:
        return False

    simbol = simboluri[0]
    rest = simboluri[1:]

    formula_true = elimina_literal_negat(formula, simbol)
    if dp(formula_true, rest):
        return True

    formula_false = elimina_literal_negat(formula, -simbol)
    return dp(formula_false, rest)


def elimina_literal_negat(formula, literal):
    noua_formula = []
    for clauza in formula:
        if literal in clauza:
            continue
        noua_clauza = [l for l in clauza if l != -literal]
        noua_formula.append(noua_clauza)
    return noua_formula


def read_dimacs(path):
    formula = []
    n_vars = 0
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('c') or line.startswith('%'):
                continue
            if line.startswith('p'):
                parts = line.split()
                n_vars = int(parts[2])
                continue
            if line == '0':
                break
            lits = [int(x) for x in line.split() if x != '0']
            formula.append(lits)
    simboluri = list(range(1, n_vars + 1))
    return formula, simboluri



def _worker_dp(formula, simboluri, return_dict):
    return_dict['result'] = dp(formula, simboluri)


def dp_cu_timeout(formula, simboluri, timeout_ms):
    manager = multiprocessing.Manager()
    return_dict = manager.dict()
    p = multiprocessing.Process(target=_worker_dp, args=(formula, simboluri, return_dict))
    p.start()
    p.join(timeout_ms / 1000)
    if p.is_alive():
        p.terminate()
        p.join()
        return None  # indicate timeout
    return return_dict.get('result')


def main(cnf_dir):
    if os.path.isfile(cnf_dir) and cnf_dir.endswith('.cnf'):
        files = [cnf_dir]
    else:
        files = [os.path.join(cnf_dir, f)
                 for f in os.listdir(cnf_dir)
                 if f.endswith('.cnf')]

    total = len(files)
    sat_count = 0
    timeout_count = 0

    for fn in sorted(files):
        formula, simboluri = read_dimacs(fn)
        start = time.time()
        result = dp_cu_timeout([list(c) for c in formula], simboluri, 1000)
        duration = (time.time() - start) * 1000

        if result is None:
            status = "TIMEOUT"
            timeout_count += 1
        else:
            status = "SAT" if result else "UNSAT"
            if result:
                sat_count += 1

        print(f"{os.path.basename(fn)}: {status} ({duration:.1f} ms)")

    print(f"\nRezultat: {sat_count}/{total} instanțe SAT")
    print(f"          {timeout_count}/{total} instanțe TIMEOUT (>1000 ms)")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <cnf_directory_or_file>")
        sys.exit(1)
    main(sys.argv[1])
