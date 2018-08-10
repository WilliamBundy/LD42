
import os
import subprocess

def process_makefile(fp="./"):
    mff = open(os.path.join(fp, "Makefile"))
    mf = mff.read()
    mfl = mf.split("\n")
    mft = mf.split()
    mff.close()

    # extract definitions
    defs = {} 
    cd = ""
    cn = 0
    for l in mfl:
        if cn == 1:
            defs[cd].append(l.strip())
            cn = l.endswith('\\')
        elif '=' in l:
            parts = l.split("=")
            cd = parts[0]
            defs[cd] = [parts[1]]
            cn = parts[1].endswith('\\')
    for d in defs.keys():
        ll = defs[d] 
        defs[d] = " ".join(ll).replace("\\", "")

    # extract flag groups
    groups = []
    cg = -1 
    mode = 0
    for t in mft:
        if mode == 0:
            if t == "#start":
                mode = 1
                cg = len(groups)
                groups.append([])
                continue
        elif mode == 1:
            if t == "#end":
                cg = -1
                mode = 0
                continue
            else:
                if t.startswith("$"):
                    t = defs[t[2:-1]]
                if t in ("\\", "clang", "cc", "gcc", "g++", "cl"):
                    continue
                groups[cg].append(t)
    return groups

def FlagsForFile(file_name, **kwargs):
    join = os.path.join
    base = os.path.dirname(__file__)
    srcdir = join(base, "src")
    libdir = join(base, "usr", "lib")
    includedir = join(base, "usr", "include")
    override = ""
    mkfg = process_makefile(base)
    if file_name.startswith("wpl"):
        override = join(base, "src", "wpl", "wpl.c")
        return {"override_filename":override,
                "flags": mkfg[0]}
    else:
        override = join(base, "src", "main.c")
        return {"override_filename":override,
                "flags": mkfg[1]}
