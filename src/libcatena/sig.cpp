#include <cstdio>
#include <iostream>
#include <openssl/pem.h>
#include "libcatena/sig.h"

namespace Catena {

Keypair::Keypair(const char* pubfile, const char* privfile){
	FILE* fp = fopen(pubfile, "r");
	if(!fp){
		std::cerr << "couldn't open pubkey file " << pubfile << std::endl;
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
	if(privfile){
		fp = fopen(privfile, "r");
		if(!fp){
			std::cerr << "couldn't open ec file " << privfile << std::endl;
			EVP_PKEY_free(pubkey);
			throw std::runtime_error("error opening ec file");
		}
		if(!(ec = PEM_read_ECPrivateKey(fp, NULL, NULL, NULL))){
			fclose(fp);
			EVP_PKEY_free(pubkey);
			throw std::runtime_error("error loading PEM ecfile");
		}
		fclose(fp);
		if(1 != EC_KEY_check_key(ec)){
			fclose(fp);
			EVP_PKEY_free(pubkey);
			throw std::runtime_error("error verifying PEM ecfile");
		}
		privkey = EVP_PKEY_new();
		if(1 != EVP_PKEY_assign_EC_KEY(privkey, ec)){
			EVP_PKEY_free(privkey);
			EVP_PKEY_free(pubkey);
			throw std::runtime_error("error binding EC key");
		}
	}else{
		privkey = 0;
	}
}


Keypair::~Keypair(){
	if(privkey){
		EVP_PKEY_free(privkey);
	}
	EVP_PKEY_free(pubkey);
}

}
