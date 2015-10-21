eiger-objs = EigerCamera.o EigerInterface.o EigerDetInfoCtrlObj.o EigerSyncCtrlObj.o EigerSavingCtrlObj.o EigerStream.o EigerDecompress.o

SRCS = $(eiger-objs:.o=.cpp)


CXXFLAGS += -std=c++11 -I../include -I../../../hardware/include -I../../../common/include\
	-I../sdk/linux/EigerAPI/include \
	-I../../../third-party/Processlib/core/include \
	-Wall -pthread -fPIC -g 

all:	Eiger.o

Eiger.o: ../sdk/linux/EigerAPI/src/EigerSDK.o $(eiger-objs)
	$(LD) -o $@ -r $+

../sdk/linux/EigerAPI/src/EigerSDK.o: FORCE
	make -C ../sdk/linux/EigerAPI/src EigerSDK.o

FORCE:

clean:
	make -C ../sdk/linux/EigerAPI/src clean
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
