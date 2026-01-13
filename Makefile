CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

OBJDIR = obj

SERVICE_SRC = $(filter-out src/cli.cpp, $(wildcard src/**/*.cpp src/*.cpp))
SERVICE_OBJ = $(SERVICE_SRC:src/%.cpp=$(OBJDIR)/%.o)

CLI_SRC = src/cli.cpp
CLI_OBJ = $(OBJDIR)/utils/utils.o

SERVICE = neighbor_discovery_service
CLI = neighbor_discovery_cli

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

.PHONY: all clean