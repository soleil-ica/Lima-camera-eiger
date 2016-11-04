sdk-objs = CurlLoop.o Requests.o

SRCS = $(sdk-objs:.o=.cpp)

JSON_INCLUDES = $(shell pkg-config --cflags jsoncpp)

CXXFLAGS += -std=c++11 -I../include -Wall -DCOMPILATION_WITH_CURL \
	$(JSON_INCLUDES) \
	-pthread -fPIC -g

all:	EigerSDK.o

EigerSDK.o:	$(sdk-objs)
	$(LD) -o $@ -r $+

clean:
	rm -f *.o *.P

%.o : %.cpp
	$(COMPILE.cpp) -MD $(CXXFLAGS) -o $@ $<
	@cp $*.d $*.P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	-e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	rm -f $*.d


-include $(SRCS:.cpp=.P)



.PHONY: check-syntax
check-syntax:
	$(CXX) -Wall -Wextra -fsyntax-only $(CXXFLAGS) $(CHK_SOURCES)
