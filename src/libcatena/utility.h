#ifndef CATENA_LIBCATENA_UTILITY
#define CATENA_LIBCATENA_UTILITY

#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <unistd.h>
#include <openssl/x509.h>
#if defined(__GLIBC__) && !defined(__UCLIBC__)
#include <gnu/libc-version.h>
#endif
#include <libcatena/exceptions.h>
#include <libcatena/tls.h>

namespace Catena {

// Simple extractor of network byte order unsigned integers, safe for all
// alignment restrictions.
// FIXME rewrite as template<bytes> atop std::array
inline unsigned long nbo_to_ulong(const unsigned char* data, int bytes){
	unsigned long ret = 0;
	while(bytes-- > 0){
		ret *= 0x100;
		ret += *data++;
	}
	return ret;
}

// Write an unsigned integer into the specified bytes using network byte order,
// zeroing out any unused leading bytes. Returns the memory following the
// written bytes.
// FIXME throw exception if hbo won't fit in bytes
// FIXME rewrite as template<bytes> atop std::array
inline unsigned char* ulong_to_nbo(unsigned long hbo, unsigned char* data, int bytes){
	int offset = bytes;
	while(offset-- > 0){
		data[offset] = hbo % 0x100;
		hbo >>= 8;
	}
	return data + bytes;
}

inline std::ostream& HexOutput(std::ostream& s, const unsigned char* data, size_t len){
	std::ios state(NULL);
	state.copyfmt(s);
	s << std::hex;
	for(size_t i = 0 ; i < len ; ++i){
		s << std::setfill('0') << std::setw(2) << (int)data[i];
	}
	s.copyfmt(state);
	return s;
}

template<size_t SIZE>
std::ostream& HexOutput(std::ostream& s, const std::array<unsigned char, SIZE>& data){
	std::ios state(NULL);
	state.copyfmt(s);
	s << std::hex;
	for(auto& e : data){
		s << std::setfill('0') << std::setw(2) << (int)e;
	}
	s.copyfmt(state);
	return s;
}

// Returns nullptr for a 0-byte file (setting *len to 0). Throws exceptions on
// inability to open file or read error. We use POSIX C I/O here because it's
// difficult to e.g. verify that a file is not a directory using C++ ifstream.
// Poor performance and memory characteristics for large files.
std::unique_ptr<unsigned char[]>
ReadBinaryFile(const std::string& fname, size_t *len);

// Returns nullptr if the specified region was not available in the file. Throws
// exceptions on inability to open file or read error. Copies the data; we might
// want to rewrite this in a zero-copy fashion.
std::unique_ptr<unsigned char[]>
ReadBinaryBlob(const std::string& fname, off_t offset, size_t len);

// Split a line into whitespace-delimited tokens, supporting simple quoting
// using single quotes, plus escaping using backslash.
std::vector<std::string> SplitInput(const char* line);

// Extract a long int from s, ensuring that it is the entirety of s (save any
// leading whitespace and sign; see strtol(3)), that it is greater than or
// equal to min, and that it is less than or equal to max. Throws
// ConvertInputException on any error.
long StrToLong(const std::string& s, long min, long max);

inline int ASCHexToVal(char nibble) {
	if(nibble >= 'a' && nibble <= 'f'){
		return nibble - 'a' + 10;
	}else if(nibble >= 'A' && nibble <= 'F'){
		return nibble - 'A' + 10;
	}else if(nibble >= '0' && nibble <= '9'){
		return nibble - '0';
	}
	throw ConvertInputException("bad hex digit: " + std::to_string(nibble));
}

// Extract a SIZE-byte binary blob from s, which ought be SIZE*2 ASCII hex
// chars decoding to SIZE raw bytes.
template<size_t SIZE>
void StrToBlob(const std::string& s, std::array<unsigned char, SIZE>& out) {
	if(s.length() < SIZE * 2){
		throw ConvertInputException("too short for input: " + s);
	}
	for(size_t i = 0 ; i < SIZE ; ++i){
		out[i] = ASCHexToVal(s[i * 2]) * 16 + ASCHexToVal(s[i * 2 + 1]);
	}
}

void IgnoreSignal(int signum);

int tls_cert_verify(int preverify_ok, X509_STORE_CTX* x509_ctx);

std::string X509CN(X509_NAME* xname);

inline std::string X509SubjectCN(const X509* cert) {
	auto xname = X509_get_subject_name(cert);
	return X509CN(xname);
}

inline std::string X509IssuerCN(const X509* cert) {
	auto xname = X509_get_issuer_name(cert);
	return X509CN(xname);
}

inline TLSName X509NetworkName(const X509* cert) {
	auto subcn = X509SubjectCN(cert);
	auto isscn = X509IssuerCN(cert);
	return std::make_pair(isscn, subcn);
}

inline TLSName SSLPeerName(SSL* s) {
  if(X509_V_OK != SSL_get_verify_result(s)){
    throw NetworkException("ssl verify failure");
  }
  auto x509 = SSL_get_peer_certificate(s); // FIXME RAII
  if(x509 == nullptr){
    throw NetworkException("error getting peer cert");
  }
  TLSName ret;
  try{
    ret = X509NetworkName(x509);
  }catch(...){
    X509_free(x509);
    throw;
  }
  X509_free(x509);
  return ret;
}

void FDSetNonblocking(int fd);

std::string GetCompilerID();
std::string GetLibjsonID();
std::string GetCapnProtoID();

inline std::string Hostname() {
	char hname[256] = "unknown"; // eh, from SuSv2
	gethostname(hname, sizeof(hname));
	hname[sizeof(hname) - 1] = '\0'; // gethostname() doesn't truncate
	return hname;
}

}

#endif
