#include <cstdio>
#include <iostream>
#include <iterator>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include "libcatena/sig.h"

namespace Catena {

Keypair::Keypair(const char* pubfile, const char* privfile){
	std::string pubstr = pubfile;
	FILE* fp = fopen(pubfile, "r");
	if(!fp){
		throw KeypairException("error opening pubkey file " + pubstr);
	}
	EC_KEY* ec = PEM_read_EC_PUBKEY(fp, NULL, NULL, NULL);
	if(!ec){
		fclose(fp);
		throw KeypairException("error loading PEM pubkey " + pubstr);
	}
	fclose(fp);
	if(1 != EC_KEY_check_key(ec)){
		throw KeypairException("error verifying PEM pubkey " + pubstr);
	}
	pubkey = EVP_PKEY_new();
	if(1 != EVP_PKEY_assign_EC_KEY(pubkey, ec)){
		EVP_PKEY_free(pubkey);
		throw KeypairException("error binding EC pubkey " + pubstr);
	}
	if(privfile){
		std::string privstr = privfile;
		fp = fopen(privfile, "r");
		if(!fp){
			EVP_PKEY_free(pubkey);
			throw KeypairException("error opening ec file " + privstr);
		}
		if(!(ec = PEM_read_ECPrivateKey(fp, NULL, NULL, NULL))){
			fclose(fp);
			EVP_PKEY_free(pubkey);
			throw KeypairException("error loading PEM privkey " + privstr);
		}
		fclose(fp);
		if(1 != EC_KEY_check_key(ec)){
			fclose(fp);
			EVP_PKEY_free(pubkey);
			throw KeypairException("error verifying PEM privkey " + privstr);
		}
		privkey = EVP_PKEY_new();
		if(1 != EVP_PKEY_assign_EC_KEY(privkey, ec)){
			EVP_PKEY_free(privkey);
			EVP_PKEY_free(pubkey);
			throw KeypairException("error binding EC privkey " + privstr);
		}
	}else{
		privkey = 0;
	}
}

Keypair::Keypair(const unsigned char* pubblob, size_t len){
	BIO* bio = BIO_new_mem_buf(pubblob, len);
	EC_KEY* ec = PEM_read_bio_EC_PUBKEY(bio, NULL, NULL, NULL);
	BIO_free(bio);
	if(!ec){
		throw KeypairException("error extracting PEM pubkey");
	}
	if(1 != EC_KEY_check_key(ec)){
		throw KeypairException("error verifying PEM pubkey ");
	}
	pubkey = EVP_PKEY_new();
	if(1 != EVP_PKEY_assign_EC_KEY(pubkey, ec)){
		EVP_PKEY_free(pubkey);
		throw KeypairException("error binding PEM pubkey ");
	}
	privkey = 0;
}

Keypair::~Keypair(){
	if(privkey){
		EVP_PKEY_free(privkey);
	}
	EVP_PKEY_free(pubkey);
}

size_t Keypair::Sign(const unsigned char* in, size_t inlen, unsigned char* out, size_t outlen) const {
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

std::pair<std::unique_ptr<unsigned char[]>, size_t>
Keypair::Sign(const unsigned char* in, size_t inlen) const {
	if(!privkey){
		return std::make_pair(nullptr, 0);
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
	if(1 != EVP_PKEY_sign(ctx, NULL, &len, in, inlen)){
		EVP_PKEY_CTX_free(ctx);
		throw std::runtime_error("error getting EC outlen");
	}
	std::unique_ptr<unsigned char[]> ret(new unsigned char[len]);
	if(1 != EVP_PKEY_sign(ctx, ret.get(), &len, in, inlen)){
		EVP_PKEY_CTX_free(ctx);
		throw std::runtime_error("error EC signing");
	}
	EVP_PKEY_CTX_free(ctx);
	return std::make_pair(std::move(ret), len);
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

std::ostream& Keypair::PrintPublicKey(std::ostream& s, const EVP_PKEY* evp){
	s << ' ';
	switch(EVP_PKEY_base_id(evp)){
		case EVP_PKEY_DSA: s << "DSA"; break;
		case EVP_PKEY_DH: s << "DH"; break;
		case EVP_PKEY_EC: s << "ECDSA"; break;
		case EVP_PKEY_RSA: s << "RSA"; break;
		default: s << "Unknown type"; break;
	}
	// FIXME RAII-wrap this so it's not lost during exceptions
	BIO* bio = BIO_new(BIO_s_mem());
	auto r = EVP_PKEY_print_public(bio, evp, 1, NULL);
	if(r <= 0){
		BIO_free(bio);
		return s;
	}
	char* buf;
	long len;
	if((len = BIO_get_mem_data(bio, &buf)) <= 0){
		BIO_free(bio);
		return s;
	}
	std::copy(buf, buf + len, std::ostream_iterator<char>(s, ""));
	BIO_free(bio);
	return s;
}

}
