#include <iostream>
#include <openssl/rand.h>
#include <libcatena/truststore.h>
#include <libcatena/utility.h>

namespace Catena {

// AES-256 and AES-128 both use a 128-bit (16 byte) IV
static constexpr size_t IVSIZE = 16;

std::ostream& operator<<(std::ostream& s, const TrustStore& ts){
	for(const auto& k : ts.keys){
		const KeyLookup& kl = k.first;
		const Keypair& kp = k.second;
		if(kp.HasPrivateKey()){
			s << "(*) ";
		}
		HexOutput(s, kl.first) << "." << kl.second << "\n";
		s << kp;
	}
	return s;
}

const KeyLookup& TrustStore::GetLookup(const Keypair& kp){
	for(const auto& k : keys){
		if(k.second == kp){
			return k.first;
		}
	}
	throw std::out_of_range("no such public key");
}

void TrustStore::addKey(const Keypair* kp, const KeyLookup& kidx){
	auto it = keys.find(kidx);
	if(it != keys.end()){
		it->second = *kp;
	}else{
		keys.insert({kidx, *kp});
	}
}

std::pair<std::unique_ptr<unsigned char[]>, size_t>
TrustStore::Sign(const unsigned char* in, size_t inlen, const KeyLookup& signer) const {
	const auto& it = keys.find(signer);
	if(it == keys.end()){
		throw SigningException("no such entry in truststore");
	}
	return it->second.Sign(in, inlen);
}

// Returned ciphertext includes 128 bits of random AES IV, so size >= IVSIZE
std::pair<std::unique_ptr<unsigned char[]>, size_t>
TrustStore::Encrypt(const void* in, size_t len, const SymmetricKey& key) const {
	std::pair<std::unique_ptr<unsigned char[]>, size_t> ret;
	if(in == nullptr){
		throw EncryptException("null plaintext");
	}
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new(); // FIXME RAII?
	if(ctx == nullptr){
		throw EncryptException("couldn't allocate EVP CTX");
	}
	int elen;
	auto maxlen = IVSIZE;
	// 0 bytes go to 16, which is less than the blocksize, but who cares,
	// we're at most wasting one block
	maxlen += (len / key.size() + 1) * key.size();
	ret.first = std::make_unique<unsigned char[]>(maxlen);
	if(1 != RAND_bytes(ret.first.get(), IVSIZE)){
		EVP_CIPHER_CTX_free(ctx);
		throw EncryptException("couldn't get random IV");
	}
	// AES-256 and AES-128 both use 128 bits of IV
	unsigned char* ctext = ret.first.get() + IVSIZE;
	if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), ret.first.get())){
		EVP_CIPHER_CTX_free(ctx);
		throw EncryptException("couldn't encrypt with AES EVP");
	}
	if(1 != EVP_EncryptUpdate(ctx, ctext, &elen,
			reinterpret_cast<const unsigned char*>(in), len)){
		EVP_CIPHER_CTX_free(ctx);
		throw EncryptException("couldn't update AES encryption");
	}
	ret.second = IVSIZE + elen;
	if(1 != EVP_EncryptFinal_ex(ctx, ctext + elen, &elen)){
		EVP_CIPHER_CTX_free(ctx);
		throw EncryptException("couldn't finalize AES encryption");
	}
	ret.second += elen;
	EVP_CIPHER_CTX_free(ctx);
	return ret;
}

// Only the plaintext is returned. AES IV and ciphertext are provided.
std::pair<std::unique_ptr<unsigned char[]>, size_t>
TrustStore::Decrypt(const void* in, size_t len, const SymmetricKey& key) const {
	std::pair<std::unique_ptr<unsigned char[]>, size_t> ret;
	if(len <= IVSIZE){
		throw EncryptException("not enough data to decrypt");
	}
	const unsigned char* iv = reinterpret_cast<const unsigned char*>(in);
	const unsigned char* ctext = iv + IVSIZE;
	const size_t clen = len - IVSIZE;
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new(); // FIXME RAII?
	if(ctx == nullptr){
		throw EncryptException("couldn't allocate EVP CTX");
	}
	if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv)){
		EVP_CIPHER_CTX_free(ctx);
		throw EncryptException("couldn't decrypt with AES EVP");
	}
	ret.first = std::make_unique<unsigned char[]>((clen / key.size() + 1) * key.size());
	int elen;
	if(1 != EVP_DecryptUpdate(ctx, ret.first.get(), &elen, ctext, clen)){
		EVP_CIPHER_CTX_free(ctx);
		throw EncryptException("couldn't update AES decryption");
	}
	ret.second = elen;
	int dlen = 0;
	if(1 != EVP_DecryptFinal_ex(ctx, ret.first.get() + elen, &dlen)){
		EVP_CIPHER_CTX_free(ctx);
		throw EncryptException("couldn't finalize AES decryption");
	}
	ret.second += dlen;
	EVP_CIPHER_CTX_free(ctx);
	return ret;
}

SymmetricKey
TrustStore::DeriveSymmetricKey(const KeyLookup& k1, const KeyLookup& k2) const {
	auto kit1 = keys.find(k1);
	auto kit2 = keys.find(k2);
	if(kit1 == keys.end() || kit2 == keys.end()){
		throw SigningException("key not found for derivation");
	}
	return kit1->second.DeriveSymmetricKey(kit2->second);
}

}
