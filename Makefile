CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

OBJDIR = obj
CLIOBJDIR = obj/cli

SERVICE_SRC = $(filter-out src/cli/%, $(wildcard src/*.cpp))
SERVICE_OBJ = $(SERVICE_SRC:src/%.cpp=$(OBJDIR)/%.o)

IPC_SRC = src/cli/ipc.cpp
IPC_OBJ = $(CLIOBJDIR)/ipc.o

CLI_SRC = src/cli/cli.cpp

SERVICE = neighbor_discovery_service
CLI = neighbor_discovery_cli

all: $(SERVICE) $(CLI)

$(OBJDIR):
	mkdir -p $@

$(CLIOBJDIR):
	mkdir -p $@

$(OBJDIR)/%.o: src/%.cpp | $(OBJDIR)
	@echo "Compiling $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(CLIOBJDIR)/ipc.o: src/cli/ipc.cpp | $(CLIOBJDIR)
	@echo "Compiling $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SERVICE): $(SERVICE_OBJ) $(IPC_OBJ)
	@echo "Linking $@"
	$(CXX) $(SERVICE_OBJ) $(IPC_OBJ) -o $@
	@echo "Build complete: $@"

$(CLI): $(CLI_SRC) | $(OBJDIR)
	@echo "Building $@"
	$(CXX) $(CXXFLAGS) $(CLI_SRC) -o $@
	@echo "Build complete: $@"

clean:
	rm -rf $(OBJDIR) $(SERVICE) $(CLI)
	@echo "Cleaned up."

help:
	@echo "Makefile targets:"
	@echo "  all     - Build service and CLI"
	@echo "  clean   - Remove build artifacts"
	@echo "  help    - Show this message"

.PHONY: all clean help