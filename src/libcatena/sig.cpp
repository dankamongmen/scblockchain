#include <cstdio>
#include <iostream>
#include <openssl/pem.h>
#include "libcatena/sig.h"

namespace Catena {

Keypair::Keypair(const char* pubfile){
	FILE* fp = fopen(pubfile, "r");
	if(!fp){
		std::cerr << "couldn't open file " << pubfile << std::endl;
		throw std::runtime_error("error opening pubkey file");
	}
	EC_KEY* ec = PEM_read_EC_PUBKEY(fp, NULL, NULL, NULL);
	if(!ec){
		fclose(fp);
		throw std::runtime_error("error loading PEM keyfile");
	}
	fclose(fp);
	pubkey = EVP_PKEY_new();
	if(1 != EVP_PKEY_assign_EC_KEY(pubkey, ec)){
		EVP_PKEY_free(pubkey);
		throw std::runtime_error("error binding EC pubkey");
	}
}


Keypair::~Keypair(){
	EVP_PKEY_free(pubkey);
}

}
