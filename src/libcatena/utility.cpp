#include <climits>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <capnp/common.h>
#include <nlohmann/json.hpp>
#include <libcatena/utility.h>

namespace Catena {

class RAIIfd {
public:
RAIIfd() = delete;
RAIIfd(int fd) : fd(fd) {}
~RAIIfd(){
  if(fd >= 0){
    close(fd); // FIXME check for error especially on written file
    fd = -1;
  }
}
private:
int fd;
};

std::unique_ptr<unsigned char[]>
ReadBinaryFile(const std::string& fname, size_t *len){
	int fd = open(fname.c_str(), O_RDONLY | O_CLOEXEC);
	if(fd < 0){
		throw std::ifstream::failure("couldn't open file");
	}
  RAIIfd rfd(fd);
	struct stat stats;
	if(fstat(fd, &stats)){
		throw std::ifstream::failure("couldn't fstat file");
	}
	if((stats.st_mode & S_IFMT) != S_IFREG){
		throw std::ifstream::failure("not a regular file");
	}
	auto flen = stats.st_size;
	if((*len = flen) == 0){
		return nullptr;
	}
	std::unique_ptr<unsigned char[]> memblock;
	memblock = std::make_unique<unsigned char[]>(*len);
	auto r = read(fd, memblock.get(), *len);
	if(r < 0 || r != flen){
		throw std::ifstream::failure("error reading file");
	}
	return memblock;
}

// FIXME move to mmap() could make this single-copy. exposing the mmap()ed data
// via RAII-managed structure could make this zero-copy. two right now.
std::unique_ptr<unsigned char[]>
ReadBinaryBlob(const std::string& fname, off_t offset, size_t len){
	int fd = open(fname.c_str(), O_RDONLY | O_CLOEXEC);
	if(fd < 0){
		throw std::ifstream::failure("couldn't open file");
	}
  RAIIfd rfd(fd);
  auto loff = lseek(fd, offset, SEEK_SET);
  if(loff != offset){
		throw std::ifstream::failure("couldn't lseek file");
  }
	std::unique_ptr<unsigned char[]> memblock = std::make_unique<unsigned char[]>(len);
	auto r = read(fd, memblock.get(), len);
	if(r < 0 || static_cast<size_t>(r) != len){
		throw std::ifstream::failure("error reading file");
	}
	return memblock;
}

std::vector<std::string> SplitInput(const char* line) {
	std::vector<std::string> tokens;
	std::vector<char> token;
	bool quoted = false;
	bool escaped = false;
	int offset = 0;
	char c;
	while( (c = line[offset]) ){
		if(c == '\\' && !escaped){
			escaped = true;
		}else if(escaped){
			token.push_back(c);
			escaped = false;
		}else if(quoted){
			if(c == '\''){
				quoted = false;
			}else{
				token.push_back(c);
			}
		}else if(isspace(c)){
			if(token.size()){
				tokens.emplace_back(std::string(token.begin(), token.end()));
				token.clear();
			}
		}else if(c == '\''){
			quoted = true;
		}else{
			token.push_back(c);
		}
		++offset;
	}
	if(token.size()){
		tokens.emplace_back(std::string(token.begin(), token.end()));
	}
	if(quoted){
		throw SplitInputException("unterminated single quote");
	}
	return tokens;
}

long StrToLong(const std::string& str, long min, long max){
	const char* s = str.c_str();
	char* e;
	errno = 0;
	auto ret = strtol(s, &e, 10);
	if((ret == LONG_MIN || ret == LONG_MAX) && errno == ERANGE){
		throw ConvertInputException("overflowed parsing number " + str);
	}
	if(ret < min){
		throw ConvertInputException("extracted value was too low " + str);
	}
	if(ret > max){
		throw ConvertInputException("extracted value was too high " + str);
	}
	if(e == s){
		throw ConvertInputException("value was empty");
	}
	if(*e){
		throw ConvertInputException("garbage in numeric value " + str);
	}
	return ret;
}

void IgnoreSignal(int signum) {
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	if(sigaction(signum, &sa, NULL)){
		throw std::runtime_error("couldn't ignore signal");
	}
}

int tls_cert_verify(int preverify_ok, X509_STORE_CTX* x509_ctx){
	if(!preverify_ok){
		int e = X509_STORE_CTX_get_error(x509_ctx);
		std::cerr << "cert verification error: " << X509_verify_cert_error_string(e) << std::endl;
	}
	X509* cert = X509_STORE_CTX_get_current_cert(x509_ctx);
	if(cert){
		X509_NAME* certsub = X509_get_subject_name(cert);
		char *cn = X509_NAME_oneline(certsub, nullptr, 0);
		if(cn){
			std::cout << "server cert: " << cn << std::endl;
			free(cn);
		}
	}
	return preverify_ok;
}

std::string X509CN(X509_NAME* xname) {
	int lastpos = -1;
	lastpos = X509_NAME_get_index_by_NID(xname, NID_commonName, lastpos);
	if(lastpos == -1){
		throw NetworkException("no subject common name in cert");
	}
	X509_NAME_ENTRY* e = X509_NAME_get_entry(xname, lastpos);
	auto asn1 = X509_NAME_ENTRY_get_data(e);
	auto str = ASN1_STRING_get0_data(asn1);
	std::string ret(reinterpret_cast<const char*>(str));
	return ret;
}

void FDSetNonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if(flags < 0){
		throw SystemException("couldn't get fd flags");
	}
	flags |= O_NONBLOCK;
	if(0 != fcntl(fd, F_SETFL, flags)){
		throw SystemException("couldn't set fd flags");
	}
}

std::string GetCompilerID(){
#if defined(__GNUC__) && !defined(__clang__)
	return std::string("GNU C++ ") + __VERSION__;
#elif defined(__clang__)
	return __VERSION__;
#else
#error "couldn't determine c++ compiler"
#endif
}

std::string GetCapnProtoID() {
  return std::string("Cap'n Proto ") +
    std::to_string(CAPNP_VERSION_MAJOR) + '.' +
    std::to_string(CAPNP_VERSION_MINOR) + '.' +
    std::to_string(CAPNP_VERSION_MICRO);
}

std::string GetLibjsonID() {
  return std::string("JSON for Modern C++ ") +
		std::to_string(NLOHMANN_JSON_VERSION_MAJOR) + "." +
		std::to_string(NLOHMANN_JSON_VERSION_MINOR) + "." +
		std::to_string(NLOHMANN_JSON_VERSION_PATCH);
}

EVP_PKEY* ExtractPublicKey(const EC_KEY* ec) {
	auto group = EC_KEY_get0_group(ec);
	auto pub = EC_POINT_new(group);
	auto bn = EC_KEY_get0_private_key(ec);
	if(1 != EC_POINT_mul(group, pub, bn, NULL, NULL, NULL)){
		EC_POINT_free(pub);
		throw KeypairException("error extracting public key");
	}
	auto ec2 = EC_KEY_new_by_curve_name(NID_secp256k1);
	EC_KEY_set_public_key(ec2, pub);
	if(1 != EC_KEY_check_key(ec2)){
    EC_KEY_free(ec2);
		EC_POINT_free(pub);
		throw KeypairException("error verifying PEM public key");
	}
	auto pubkey = EVP_PKEY_new();
	if(1 != EVP_PKEY_assign_EC_KEY(pubkey, ec2)){
    EC_KEY_free(ec2);
		EC_POINT_free(pub);
		EVP_PKEY_free(pubkey);
		throw KeypairException("error binding EC pubkey");
	}
	EC_POINT_free(pub);
  return pubkey;
}

}
