CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wno-unused-parameter -Wno-misleading-indentation -Iinclude
OUT := bin/linux/core_checks
SRCS := src/game.cpp src/game_order.cpp src/quest_core.cpp src/quest_turn_stars.cpp src/quest_battle.cpp src/quest_world.cpp src/quest_storage.cpp src/quest_meta.cpp src/quest_data.cpp src/quest_run.cpp src/quest_save.cpp tests/core_checks.cpp tests/run_checks.cpp tests/world_checks.cpp tests/tiger_checks.cpp tests/star_checks.cpp

.PHONY: core
core: $(OUT)
	mkdir -p bin/linux/data
	cp data/*.json bin/linux/data/
	./$(OUT)

$(OUT): $(SRCS) include/*.hpp data/*.json
	mkdir -p bin/linux/data
	$(CXX) $(CXXFLAGS) -o $(OUT) $(SRCS)
	cp data/*.json bin/linux/data/
