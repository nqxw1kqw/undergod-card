CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wno-unused-parameter -Wno-misleading-indentation -Iinclude
OUT := bin/linux/core_checks
SRCS := src/game.cpp src/quest_core.cpp src/quest_data.cpp src/quest_run.cpp src/quest_save.cpp tests/core_checks.cpp tests/run_checks.cpp

.PHONY: core
core: $(OUT)
	mkdir -p bin/linux/data
	cp data/*.json bin/linux/data/
	./$(OUT)

$(OUT): $(SRCS) include/*.hpp data/*.json
	mkdir -p bin/linux/data
	$(CXX) $(CXXFLAGS) -o $(OUT) $(SRCS)
	cp data/*.json bin/linux/data/
