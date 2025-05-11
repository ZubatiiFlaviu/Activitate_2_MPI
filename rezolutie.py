import os
import sys
import time
import multiprocessing


def rezolutie(clauze):
    while True:
        noi_clauze = []
        for i in range(len(clauze)):
            for j in range(i + 1, len(clauze)):
                clauze_noi = rezolva_clauze(clauze[i], clauze[j])
                for cl in clauze_noi:
                    if cl == []:
                        return True
                    if cl not in clauze and cl not in noi_clauze:
                        noi_clauze.append(cl)
        if not noi_clauze:
            return False
        clauze.extend(noi_clauze)


def rezolva_clauze(clauza1, clauza2):
    rezultat = []
    for lit1 in clauza1:
        for lit2 in clauza2:
            if lit1 == -lit2:
                noua = set(clauza1 + clauza2)
                noua.discard(lit1)
                noua.discard(lit2)
                rezultat.append(sorted(noua))
    return rezultat


def read_dimacs(path):
    formula = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            # skip comments and delimiters
            if not line or line.startswith('c') or line.startswith('%'):
                continue
            # skip problem line
            if line.startswith('p'):
                continue
            # parse clause literals ignoring terminal 0
            parts = line.split()
            lits = []
            for x in parts:
                if x == '0':
                    break
                try:
                    lits.append(int(x))
                except ValueError:
                    # ignore any non-integer tokens
                    continue
            if lits:
                formula.append(lits)
    return formula


def _worker_resolutie(clauze, return_dict):
    return_dict['result'] = rezolutie(clauze)


def rezolutie_cu_timeout(clauze, timeout_ms):
    manager = multiprocessing.Manager()
    return_dict = manager.dict()
    p = multiprocessing.Process(target=_worker_resolutie, args=(clauze, return_dict))
    p.start()
    p.join(timeout_ms / 1000)

    if p.is_alive():
        p.terminate()
        p.join()
        return "TIMEOUT", timeout_ms

    return return_dict.get('result', False), None


def main(cnf_dir):
    # Determine files to process
    if os.path.isfile(cnf_dir) and cnf_dir.endswith('.cnf'):
        files = [cnf_dir]
    else:
        files = [os.path.join(cnf_dir, f)
                 for f in os.listdir(cnf_dir)
                 if f.endswith('.cnf')]

    total = len(files)
    unsat_count = 0
    timeout_count = 0

    for fn in sorted(files):
        clauze = read_dimacs(fn)
        start = time.time()
        result, timeout_used = rezolutie_cu_timeout([list(c) for c in clauze], 1000)
        duration = (time.time() - start) * 1000

        if result == "TIMEOUT":
            status = "TIMEOUT"
            timeout_count += 1
        else:
            status = "UNSAT" if result else "SAT"
            if result:
                unsat_count += 1

        print(f"{os.path.basename(fn)}: {status} ({duration:.1f} ms)")

    print(f"\nRezultat: {unsat_count}/{total} instanțe UNSAT (contradicție)")
    print(f"          {timeout_count}/{total} instanțe TIMEOUT (>1000 ms)")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <cnf_directory_or_file>")
        sys.exit(1)
    main(sys.argv[1])