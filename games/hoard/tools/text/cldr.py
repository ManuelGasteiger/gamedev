"""Writes data/text/<lang>/locale.json from CLDR via Babel: number symbols, grouping, plural rules.
Usage: python3 tools/text/cldr.py en de"""
import json, sys
import babel
from babel import Locale
from babel.numbers import format_decimal, parse_pattern

def locale_json(lang):
    loc = Locale.parse(lang)
    sym = loc.number_symbols["latn"]
    pattern = parse_pattern(loc.decimal_formats[None].pattern)
    min_grouping = 1 if sym["group"] in format_decimal(1000, locale=lang) else 2
    return {"lang": lang, "source": f"CLDR via Babel {babel.__version__}", "group": sym["group"], "decimal": sym["decimal"],
            "grouping": pattern.grouping[0], "min_grouping": min_grouping, "plural": dict(loc.plural_form.rules)}

if __name__ == "__main__":
    for lang in sys.argv[1:]:
        path = f"data/text/{lang}/locale.json"
        with open(path, "w", encoding="utf-8") as f:
            json.dump(locale_json(lang), f, ensure_ascii=False, indent=1)
            f.write("\n")
        print(path)
