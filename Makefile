CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Werror -O2
LDFLAGS :=
READLINE ?= 1

ifeq ($(READLINE),1)
	CXXFLAGS += -DUSE_READLINE
	LDFLAGS += -lreadline
endif

SRCS := \
	main.cpp \
	core/Shell.cpp \
	lexer/Lexer.cpp \
	parser/Parser.cpp \
	executor/Executor.cpp \
	executor/Trace.cpp \
	builtins/Builtins.cpp \
	builtins/cd.cpp \
	builtins/echo.cpp \
	builtins/export.cpp \
	builtins/exit.cpp \
	builtins/help.cpp \
	builtins/jobs.cpp \
	builtins/fg.cpp \
	builtins/bg.cpp \
	builtins/pwd.cpp \
	builtins/unset.cpp \
	builtins/env.cpp \
	utils/Env.cpp \
	utils/Signal.cpp \
	utils/Jobs.cpp

OBJS := $(SRCS:.cpp=.o)
TARGET := mysh
UNIT_TARGET := mysh_unit

UNIT_SRCS := \
	lexer/Lexer.cpp \
	parser/Parser.cpp \
	executor/Executor.cpp \
	executor/Trace.cpp \
	builtins/Builtins.cpp \
	builtins/cd.cpp \
	builtins/echo.cpp \
	builtins/export.cpp \
	builtins/exit.cpp \
	builtins/help.cpp \
	builtins/jobs.cpp \
	builtins/fg.cpp \
	builtins/bg.cpp \
	builtins/pwd.cpp \
	builtins/unset.cpp \
	builtins/env.cpp \
	utils/Env.cpp \
	utils/Signal.cpp \
	utils/Jobs.cpp \
	tests/unit_tests.cpp

UNIT_OBJS := $(UNIT_SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(UNIT_TARGET): $(UNIT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(TARGET)

re: fclean all

test: all
	bash scripts/smoke_test.sh ./$(TARGET)

regression: all
	bash scripts/regression_test.sh ./$(TARGET)

edge: all
	bash scripts/edge_test.sh ./$(TARGET)

unit: $(UNIT_TARGET)
	./$(UNIT_TARGET)

check: test regression edge unit

benchmark: all
	bash scripts/benchmark.sh ./$(TARGET)

benchmark-compare: all
	bash scripts/benchmark_compare.sh ./$(TARGET)

benchmark-guard: benchmark
	bash scripts/benchmark_guard.sh

lint:
	bash scripts/lint.sh

coverage: CXXFLAGS += -g -O0 --coverage
coverage: LDFLAGS += --coverage
coverage: fclean all
	$(MAKE) check CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)"
	bash scripts/coverage.sh ./$(TARGET)

debug: CXXFLAGS += -g -O0
debug: all

asan: CXXFLAGS += -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
asan: LDFLAGS += -fsanitize=address,undefined
asan: fclean all

asan-check: asan
	make test regression edge

.PHONY: all clean fclean re test regression edge unit check benchmark benchmark-compare benchmark-guard lint coverage debug asan asan-check
