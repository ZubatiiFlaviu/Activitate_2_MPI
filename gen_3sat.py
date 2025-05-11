import random
import os

def gen_instance(n_vars, m_clauses, fname):
    with open(fname, 'w') as f:
        f.write(f"p cnf {n_vars} {m_clauses}\n")
        for _ in range(m_clauses):
            clause = set()
            while len(clause) < 3:
                lit = random.randint(1, n_vars) * random.choice([-1,1])
                clause.add(lit)
            f.write(" ".join(str(l) for l in clause) + " 0\n")

if __name__ == "__main__":
    os.makedirs("synthetic", exist_ok=True)
    for i in range(1, 101):
        gen_instance(50, 218, f"synthetic/uf50_instance_{i:03d}.cnf")
    print("Generated 100 synthetic 3-SAT instances în directorul ./synthetic/")
