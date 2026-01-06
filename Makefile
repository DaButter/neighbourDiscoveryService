CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

OBJDIR = obj
SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:src/%.cpp=$(OBJDIR)/%.o)

OUT = neighbor_discovery_service

all: $(OUT)

$(OBJDIR):
	mkdir -p $@

$(OBJDIR)/%.o: src/%.cpp | $(OBJDIR)
	@echo "Compiling $<"
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

$(OUT): $(OBJ)
	$(CXX) $(OBJ) -o $@
	@echo "Build complete: $@"

clean:
	rm -rf $(OBJDIR) $(OUT)
	@echo "Cleaned up."

help:
	@echo "Makefile targets:"
	@echo "  all   - Build the project"
	@echo "  clean - Remove build artifacts"
	@echo "  help  - Shows this"