.PHONY: default all run clean header

PATTERN := ABCD|DEFGH|EFGHI|A{4,}
FILE := input.txt
REPEAT := 1000

PATTERN_HASH := $(shell echo "$(PATTERN)" | md5 || echo "$(PATTERN)" | md5sum | cut -d' ' -f1)

LIBRARIES := $(sort baseline boost ctre pcre pcre-jit re2 std xpressive srell)

default: all

CXXFLAGS := -std=c++2a -I../../include -I/opt/homebrew/opt/re2/include -I/opt/homebrew/Cellar/abseil/20240722.1/include -I/opt/homebrew/opt/boost/include -I/opt/homebrew/opt/pcre2/include -I. -Wno-error -Wno-sign-conversion -Wno-conversion
LDFLAGS :=


ctre_CXXFLAGS := -Wno-error -Wno-sign-conversion -Wno-conversion
ctre_LDFLAGS := 
ctre_NAME := CTRE

boost_CXXFLAGS := -I/opt/homebrew/opt/boost/include -Wno-error -Wno-sign-conversion -Wno-conversion
boost_LDFLAGS := -L/opt/homebrew/opt/boost/lib -lboost_regex
boost_NAME := boost::regex

pcre_CXXFLAGS := -Wno-error -Wno-sign-conversion -Wno-conversion -I/opt/homebrew/opt/pcre2/include
pcre_LDFLAGS := -L/opt/homebrew/opt/pcre2/lib -lpcre2-8
pcre_NAME := PCRE2

pcre-jit_CXXFLAGS := -Wno-error -Wno-sign-conversion -Wno-conversion -I/opt/homebrew/opt/pcre2/include
pcre-jit_LDFLAGS := -L/opt/homebrew/opt/pcre2/lib -lpcre2-8
pcre-jit_NAME := PCRE2 (jit)

re2_CXXFLAGS := -Wno-error -Wno-sign-conversion -Wno-conversion -I/opt/homebrew/opt/re2/include -I/opt/homebrew/Cellar/abseil/20240722.1/include
re2_LDFLAGS := -L/opt/homebrew/opt/re2/lib -lre2
re2_NAME := RE2

std_CXXFLAGS := -Wno-error -Wno-sign-conversion -Wno-conversion
std_NAME := std::regex

xpressive_CXXFLAGS := -Wno-error -Wno-sign-conversion -Wno-conversion
xpressive_NAME := boost::xpressive

srell_CXXFLAGS := -Wno-error -Wno-sign-conversion -Wno-conversion -I./srell_include
srell_NAME := SRELL

all: $(LIBRARIES)

$(PATTERN_HASH).tmp:
	@rm -f *.tmp
	@touch $@

pattern.hpp: $(PATTERN_HASH).tmp
	@echo "#define PATTERN \"$(PATTERN)\"" > pattern.hpp

$(LIBRARIES:%=%.o): %.o: %.cpp common.hpp $(PATTERN_HASH).tmp pattern.hpp
	$(CXX) -O3 $(CXXFLAGS) ${${@:%.o=%}_CXXFLAGS} -c $< -o $@

$(LIBRARIES): %: %.o common.hpp $(PATTERN_HASH).tmp pattern.hpp
	$(CXX) -O3 $(LDFLAGS) ${${@}_LDFLAGS} $< -o $@

header.csv:
	echo "library;pattern;duration" > header.csv
	echo "library;pattern;duration" > result.csv

$(LIBRARIES:%=%.run): %.run: % header.csv
	for i in $$(seq 1 $(REPEAT)); do ./$< $(FILE) benchmark "${${<}_NAME}" 2>/dev/null; done | tail -1 >>result.csv

run: $(LIBRARIES:%=%.run)
	
node:
	node node-v8.js path=${FILE} pattern="${PATTERN}"

jsc:
	jsc jsc.js -- path=${FILE} pattern="${PATTERN}"

result.csv: header.csv $(LIBRARIES:%=%.run)
	touch result.csv

clean:
	rm -f $(LIBRARIES) *.o *.tmp header.csv result.csv pattern.hpp
	