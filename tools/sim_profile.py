#!/usr/bin/env python3
"""Fasst das Profil von mos-sim (--profile: Zyklen je Adresse) nach Funktionen zusammen."""
import subprocess, sys, collections
elf, prof = sys.argv[1], sys.argv[2]
syms = []
for line in subprocess.run(["llvm-nm", "-n", "-S", elf], capture_output=True, text=True).stdout.splitlines():
    p = line.split()
    if len(p) == 4 and p[2] in "tTwW":
        syms.append((int(p[0], 16), int(p[1], 16), p[3]))
cnt = collections.Counter(); total = 0
for line in open(prof):
    p = line.replace(":", " ").split()
    if len(p) < 2: continue
    try: a, c = int(p[0], 16), int(p[1])
    except ValueError: continue
    total += c
    name = "?"
    for s, n, nm in syms:
        if s <= a < s + n: name = nm; break
    cnt[name] += c
for nm, c in cnt.most_common(40):
    print(f"{c * 100 / total:6.2f} %  {c:>12}  {nm}")
