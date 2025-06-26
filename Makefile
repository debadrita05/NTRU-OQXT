CC = g++
LD = g++

# Compilation flags
CFLAGS = -I. -I./blake3/ -w -O3 -std=c++17 -msse2 -msse -mssse3 -march=native -ffast-math -mavx2 -mfma -maes -fpermissive -fopenmp

# Linker flags
LDFLAGS = -lcryptopp -lpthread -lgmpxx -lssl -lhiredis -lredis++ -lcrypto -lntl -lgmp -lm -lrt \
  -Wl,./blake3/libblake3.so,-rpath,/sealusers/user3/redis-plus-plus/build

# Targets
ntru-oqxt-setup: rawdatautil.cpp bloom_filter.cpp AES_256GCM.c \
  ./falcon-round3/Extra/c/shake.c ./falcon-round3/Extra/c/common.c \
  ./falcon-round3/Extra/c/keygen.c ./falcon-round3/Extra/c/fft.c \
  ./falcon-round3/Extra/c/fpr.c ./falcon-round3/Extra/c/vrfy.c \
  ./falcon-round3/Extra/c/codec.c ./falcon-round3/Extra/c/sign.c \
  ./falcon-round3/Extra/c/rng.c ./blake3/blake_hash.cpp ntru-oqxt-setup.cpp
	$(CC) $(CFLAGS) -g -o ntru-oqxt-setup $^ $(LDFLAGS)

ntru-oqxt-search: rawdatautil.cpp bloom_filter.cpp AES_256GCM.c \
  ./falcon-round3/Extra/c/shake.c ./falcon-round3/Extra/c/common.c \
  ./falcon-round3/Extra/c/keygen.c ./falcon-round3/Extra/c/fft.c \
  ./falcon-round3/Extra/c/fpr.c ./falcon-round3/Extra/c/vrfy.c \
  ./falcon-round3/Extra/c/codec.c ./falcon-round3/Extra/c/sign.c \
  ./falcon-round3/Extra/c/rng.c ./blake3/blake_hash.cpp ntru-oqxt-search.cpp
	$(CC) $(CFLAGS) -g -o ntru-oqxt-search $^ $(LDFLAGS)

clean_all:
	rm -rf *.o setup *.gch oqxt_falcon_setup oqxt_falcon_search EDB_test.csv bloom_filter.dat
	@redis-cli flushall
	@redis-cli save

