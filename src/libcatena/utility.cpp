#include <climits>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
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

}
