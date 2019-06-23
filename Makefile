CC=g++
BSV_FLAGS=-std=c++1z -DHAVE_CONFIG_H -Ibitcoin -Ibitcoin/src/config  -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -ICommon -Ibitcoin/src -Ibitcoin/src/obj  -DBOOST_SP_USE_STD_ATOMIC -DBOOST_AC_USE_STD_ATOMIC -pthread -I/usr/include -Ibitcoin/src/leveldb/include -I/bitcoin/src/leveldb/helpers/memenv -Ibitcoin/src/secp256k1/include -Ibitcoin/src/univalue/include -DHAVE_BUILD_INFO -D__STDC_FORMAT_MACROS -DPERFMON -Wstack-protector -fstack-protector-all -fpermissive -fPIC -g -O0 -DPERFMON 

TARGET=Hello_compiler

SRC=Hello/Hello.cpp \
HelloDll/parser.cpp \
HelloDll/AST.cpp \
HelloDll/Hello_compiler.cpp \
HelloDll/HelloDll.cpp \
bitcoin/src/script/script.cpp \
bitcoin/src/utilstrencodings.cpp

OBJ=$(SRC:.cpp=.o)

LIBOBJ=$(LIBSRC:.cpp=.o)

$(TARGET) : $(OBJS)
	$(CC) $(BSV_FLAGS) $(SRC) -o $@ 

clean:
	rm -f $(TARGET) $(TARGETLIB) $(OBJ)

