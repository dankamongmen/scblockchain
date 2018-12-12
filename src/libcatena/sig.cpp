#include <cstdio>
#include <iostream>
#include <openssl/pem.h>
#include <openssl/evp.h>
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

size_t Keypair::Sign(const unsigned char* in, size_t inlen, unsigned char* out, size_t outlen){
	if(!privkey){
		return 0;
	}
	EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(privkey, NULL);
	if(1 != EVP_PKEY_sign_init(ctx)){
		EVP_PKEY_CTX_free(ctx);
		throw std::runtime_error("error initializing EC ctx");
	}
	if(1 != EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha256())){
		EVP_PKEY_CTX_free(ctx);
		throw std::runtime_error("error initializing sha256 EC ctx");
	}
	size_t len = 0;
	if(1 != EVP_PKEY_sign(ctx, NULL, &len, in, inlen) || len > outlen){
		std::cerr << "len: " << len << " outlen: " << outlen << std::endl;
		EVP_PKEY_CTX_free(ctx);
		throw std::runtime_error("error getting EC outlen");
	}
	if(1 != EVP_PKEY_sign(ctx, out, &len, in, inlen)){
		EVP_PKEY_CTX_free(ctx);
		throw std::runtime_error("error EC signing");
	}
	EVP_PKEY_CTX_free(ctx);
	return len;
}

bool Keypair::Verify(const unsigned char* in, size_t inlen, const unsigned char* sig, size_t siglen){
	EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pubkey, NULL);
	if(1 != EVP_PKEY_verify_init(ctx)){
		EVP_PKEY_CTX_free(ctx);
		throw std::runtime_error("error initializing EC ctx");
	}
	if(1 != EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha256())){
		EVP_PKEY_CTX_free(ctx);
		throw std::runtime_error("error initializing sha256 EC ctx");
	}
	bool ret = false;
	if(1 != EVP_PKEY_verify(ctx, sig, siglen, in, inlen)){
		ret = true;
	};
	EVP_PKEY_CTX_free(ctx);
	return ret;
}

}
