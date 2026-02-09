#ifndef OQXT_FALCON_H
#define OQXT_FALCON_H


#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <set>
#include <map>
#include <bitset>
#include <random>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <cmath>
#include <cstdio>
#include <unistd.h>
#include <cstdlib>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bits/stdc++.h>
#include <NTL/ZZ_pX.h>
#include <NTL/ZZ_p.h>


#include "crypto++/cryptlib.h"
#include "crypto++/sha3.h"
#include "crypto++/hex.h"
#include "crypto++/files.h"
#include "crypto++/filters.h"



#include "./blake3/blake3.h" 
#include "./blake3/blake_hash.h"

#include </usr/local/include/sw/redis++/redis++.h>
#include </usr/local/include/sw/redis++/connection.h>
#include </usr/local/include/sw/redis++/redis_cluster.h>


#include "size_parameters.h"
#include "rawdatautil.h"
#include "bloom_filter.h"
#include "utils.h"
#include "AES_256GCM.h"
#include "../../NTRU-OQXT/falcon-round3/Extra/c/falcon.h"
#include "../../NTRU-OQXT/falcon-round3/Extra/c/inner.h"

using namespace std;
using namespace std::chrono;
using namespace sw::redis;
using namespace CryptoPP;
using namespace NTL;

extern sw::redis::ConnectionOptions connection_options;
extern sw::redis::ConnectionPoolOptions pool_options;


int Sys_Init();
int Sys_Clear();

int TSet_SetUp();


int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *aad,int aad_len, unsigned char *key, unsigned char *iv,
	        unsigned char *ciphertext, unsigned char *tag);


int HASH(unsigned char *msg, unsigned char *digest);
int BLOOM_HASH(unsigned char *msg, unsigned char *digest);



int MGDB_QUERY(unsigned char *RES, unsigned char *BIDX, unsigned char *JIDX, unsigned char *LBL);


int SHA3_HASH(blake3_hasher *hasher,unsigned char *msg, unsigned char *digest);
int SHA3_HASH_K(blake3_hasher *hasher,unsigned char *msg, unsigned char *digest);

std::string HexToStr(int *hexarr, int len);
std::string HexToStr(unsigned char *hexarr,int len=16);

unsigned int BFIdxConv(unsigned char *hex_arr, unsigned int n_bits);

static void mk_rand_poly_oqxt(prng *p, fpr *f, unsigned logn);
std::string HexToStr(int *hexarr, int len);
std::string HexToStr(unsigned char *hexarr,int len=16);


#endif
