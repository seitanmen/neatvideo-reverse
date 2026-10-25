# Every harness binary that loads the genuine bundle must include nvh_guard.h and call
# nvh_guard_preload() before dlopen (Agents.md §5). Only `nvh` is built here.
nvh: nvh.c nvh_guard.h
	clang -O1 -arch arm64 -Wall -framework CoreFoundation -o nvh nvh.c

.PHONY: check
check: nvh
	./nvh selftest; test $$? -eq 3 && echo "selftest ok (exit 3)"
