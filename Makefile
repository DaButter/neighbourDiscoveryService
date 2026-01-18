CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Isrc

OBJDIR = obj

SERVICE_SRC = $(filter-out src/cli.cpp, $(wildcard src/**/*.cpp src/*.cpp))
SERVICE_OBJ = $(SERVICE_SRC:src/%.cpp=$(OBJDIR)/%.o)

CLI_SRC = src/cli.cpp
CLI_OBJ = $(OBJDIR)/utils/utils.o

SERVICE = ndisc_svc
CLI = ndisc_cli

all: $(SERVICE) $(CLI)

$(OBJDIR):
	mkdir -p $@

$(OBJDIR)/%.o: src/%.cpp | $(OBJDIR)
	@mkdir -p $(dir $@)
	@echo "Compiling $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SERVICE): $(SERVICE_OBJ)
	@echo "Linking $@"
	$(CXX) $(SERVICE_OBJ) -o $@

$(CLI): $(CLI_SRC) $(CLI_OBJ)
	@echo "Building $@"
	$(CXX) $(CXXFLAGS) $(CLI_SRC) $(CLI_OBJ) -o $@

clean:
	rm -rf $(OBJDIR) $(SERVICE) $(CLI)

.PHONY: all clean help

help:
	@echo "Makefile targets:"
	@echo "  all    - Build service and CLI"
	@echo "  clean  - Remove build artifacts"
	@echo "  help   - Show this help message"
