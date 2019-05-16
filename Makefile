CXX_STANDARD = c++14
CXX_STANDARD_OPT = -std=$(CXX_STANDARD)
CXX_OPTIMIZE_OPT = -O2
CXX_NO_OPTIMIZE_OPT = -O0
CXX_ERRORS_OPT = -pedantic-errors
CXX_SUGGEST_OPT = -Wsuggest-attribute=pure -Wsuggest-attribute=const
CXX_WARNINGS_OPT = -Wall -Wextra -Wpedantic -Wshadow
CXX_SYMBOLS_OPT = -g
CXX_COVERAGE_OPT = -coverage
CXX_INCLUDE_OPT = -I$(MET32_PATH)/src -L$(MET32_PATH)/build/metronome32.o -I$(SRC_PATH)

# You can comment out specific portions here.
CXXFLAGS = $(CXX_STANDARD_OPT)
CXXFLAGS += $(CXX_SYMBOLS_OPT)
CXXFLAGS += $(CXX_OPTIMIZE_OPT)
#CXXFLAGS += $(CXX_NO_OPTIMIZE_OPT)
CXXFLAGS += $(CXX_ERRORS_OPT)
CXXFLAGS += $(CXX_SUGGEST_OPT)
CXXFLAGS += $(CXX_WARNINGS_OPT)
CXXFLAGS += $(CXX_INCLUDE_OPT)

LD = ld

VALG = valgrind
VALGMC = $(VALG) --tool=memcheck
VALGMCFLAGS = --track-origins=yes --expensive-definedness-checks=yes
VALGCLG = $(VALG) --tool=callgrind
VALGCLGFLAGS = --collect-jumps=yes --cache-sim=yes --branch-sim=yes \
	       --simulate-hwpref=yes --simulate-wb=yes

DOXYGEN = doxygen

RM_FILE = rm -f
RM_FOLDER = rm -rf
MKDIR = mkdir

THIS_MAKEFILE = $(abspath $(lastword $(MAKEFILE_LIST)))
MY_PATH = $(dir $(THIS_MAKEFILE))
BUILD_PATH = $(MY_PATH)build
SRC_PATH = $(MY_PATH)src
TEST_PATH = $(MY_PATH)tests
MET32_PATH = $(MY_PATH)metronome32
DOC_PATH = $(MY_PATH)doxydoc

# Compiles the object in $(BUILD_PATH)/maag32.o.
default: $(BUILD_PATH)/maag32

# Compiles the object in $(BUILD_PATH)/maag32.o AND compiles a test program.
# Then executes the test program.
test: default $(TEST_PATH)/instr.p32
	@echo Testing test program by itself
	$(BUILD_PATH)/maag32 $(TEST_PATH)/instr.p32

# Same as test except executes it in Valgrind's Memcheck.
test_memcheck: default $(TEST_PATH)/instr.p32
	@echo Testing test program with Valgrind\'s Memcheck
	$(VALGMC) $(VALGMCFLAGS) $(BUILD_PATH)/maag32 $(TEST_PATH)/instr.p32

# Same as test except executes it in Valgrind's Callgrind.
test_callgrind: default $(TEST_PATH)/instr.p32
	@echo Testing test program with Valgrind\'s Callgrind.
	$(VALGCLG) $(VALGCLGFLAGS) $(BUILD_PATH)/maag32 $(TEST_PATH)/instr.p32

# Calls both test_memcheck and test_callgrind.
test_full: test test_memcheck test_callgrind

# Runs test but with gcov coverage enabled.
coverage: CXXFLAGS += $(CXX_COVERAGE_OPT)
coverage: CXX_OPTIMIZE_OPT = $(CXX_NO_OPTIMIZE_OPT)
coverage: test

# Runs doxygen.
doc:
	$(DOXYGEN)

# Deletes the build folder.
clean:
	$(RM_FOLDER) $(BUILD_PATH)
	$(RM_FOLDER) $(DOC_PATH)
	$(MAKE) -C $(MET32_PATH) clean

.PHONY: doc default test test_memcheck test_callgrind test_full clean coverage

$(BUILD_PATH):
	$(MKDIR) $(BUILD_PATH)

$(MET32_PATH)/build/metronome32.o: $(BUILD_PATH)
	$(MAKE) -C $(MET32_PATH)

$(BUILD_PATH)/transforms.o: $(SRC_PATH)/transforms.cpp $(BUILD_PATH)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_PATH)/except.o: $(SRC_PATH)/except.cpp $(BUILD_PATH)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_PATH)/assemble.o: $(SRC_PATH)/assemble.cpp $(BUILD_PATH)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_PATH)/main.o: $(SRC_PATH)/main.cpp $(BUILD_PATH)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_PATH)/maag32: $(BUILD_PATH)/main.o \
		$(BUILD_PATH)/transforms.o \
		$(BUILD_PATH)/except.o \
		$(BUILD_PATH)/assemble.o \
		$(MET32_PATH)/build/metronome32.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@
