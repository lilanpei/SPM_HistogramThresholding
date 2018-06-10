# 
# FF_ROOT     pointing to the FastFlow root directory (i.e.
#             the one containing the ff directory).
ifndef FF_ROOT
FF_ROOT		= $(HOME)/fastflow
endif

CXX		= g++ -std=c++11
INCLUDES	= -I $(FF_ROOT) 

LDFLAGS 	= -lpthread -lX11
OPTFLAGS	= -O3

TARGETS		= 	Sequential \
			Threads \
			Farm \
                        Pipeline \
                        Farm_Pipeline


.PHONY: all clean cleanall
.SUFFIXES: .cpp 


%: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS)

all		: $(TARGETS)

clean		: 
	rm -f $(TARGETS)
cleanall	: clean
	\rm -f *.o *~
