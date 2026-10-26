"""Player-facing English left in sim/: string literals of two or more words. Lines that build developer errors
(err, error, fail, fprintf, CHECK) and the renderer in sim/text are skipped. Exit 1 and list the hits.
Usage: python3 tools/check_literals.py"""
import pathlib, re, sys

LITERAL = re.compile(r'"((?:[^"\\]|\\.)*)"')
WORDS = re.compile(r"[A-Za-z]{2,}[a-z]* +[A-Za-z]{2,}")
DEV = re.compile(r"\berr\b|\berror\b|fail\w*\(|problem\(|problems->|fprintf|CHECK\(|#include")
SKIP = ("sim/ages/json.h", "sim/tests/", "sim/runner/", "sim/text/")   # sim/text holds no player text

def hits(root):
    for path in sorted(pathlib.Path(root, "sim").rglob("*")):
        rel = path.relative_to(root).as_posix()
        if path.suffix not in (".cpp", ".h") or rel.startswith(SKIP): continue
        for n, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            code = line.split("//")[0] if '"' not in line.split("//")[0][-1:] else line
            if DEV.search(code): continue
            for lit in LITERAL.findall(code.split("// ")[0]):
                if WORDS.search(lit): yield f"{rel}:{n}: \"{lit}\""

if __name__ == "__main__":
    found = list(hits(pathlib.Path(__file__).resolve().parent.parent))
    print("\n".join(found))
    sys.exit(1 if found else 0)
