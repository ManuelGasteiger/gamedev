#pragma once
#include <cstdio>
#include <cstdlib>
#include <string>

#define CHECK(cond, msg) do { if (!(cond)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); std::exit(1); } } while (0)
