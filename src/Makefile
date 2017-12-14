CXX = g++
FLAGS = -Wall -Werror -Wno-sign-compare -Wno-unused-function -O2 -std=c++11 -DDATA_PATH=\"$(realpath ../Data)/\"
BUILDIR = ../build
SOURCES = common.cpp csv_reader.cpp dataframe.cpp imputation.cpp tree.cpp random_forest.cpp
OBJECTS = $(addprefix $(BUILDIR)/,$(SOURCES:cpp=o))
LIBRARY_SHORT = rf
LIBRARY = $(BUILDIR)/lib$(LIBRARY_SHORT).so
SOURCES_BIN = common_test.cpp csv_reader_test.cpp dataframe_test.cpp imputation_test.cpp tree_test.cpp train_and_test.cpp
BINARIES = $(addprefix $(BUILDIR)/,$(basename $(SOURCES_BIN)))

all: $(LIBRARY) $(BINARIES) 

define COMPILE_OBJ
$(BUILDIR)/$(1:cpp=o): $(1) $(1:cpp=h)
	$(CXX) -c $(FLAGS) -fpic $(1) -o $(BUILDIR)/$(1:cpp=o)
endef

define COMPILE_BIN
$(BUILDIR)/$(basename $(1)): $(LIBRARY) $(1)
	$(CXX) -c $(FLAGS) $(1) -o $(BUILDIR)/$(1:cpp=o)
	g++ -L$(BUILDIR) -Wl,-rpath=$(realpath $(BUILDIR)) -o $(BUILDIR)/$(basename $(1)) $(BUILDIR)/$(1:cpp=o) -l$(LIBRARY_SHORT)
endef

$(foreach source,$(SOURCES),$(eval $(call COMPILE_OBJ,$(source))))

$(LIBRARY): $(OBJECTS)
	g++ -shared -o $(LIBRARY) $(OBJECTS)

$(foreach source,$(SOURCES_BIN),$(eval $(call COMPILE_BIN,$(source))))


