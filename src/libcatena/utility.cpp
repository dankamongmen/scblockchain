#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libcatena/utility.h>

namespace Catena {

std::unique_ptr<unsigned char[]>
ReadBinaryFile(const std::string& fname, size_t *len){
	// FIXME this should be wrapped in an RAII class
	int fd = open(fname.c_str(), O_RDONLY | O_CLOEXEC);
	if(fd < 0){
		throw std::ifstream::failure("couldn't open file");
	}
	struct stat stats;
	if(fstat(fd, &stats)){
		close(fd);
		throw std::ifstream::failure("couldn't fstat file");
	}
	if((stats.st_mode & S_IFMT) != S_IFREG){
		close(fd);
		throw std::ifstream::failure("not a regular file");
	}
	auto flen = stats.st_size;
	if((*len = flen) == 0){
		close(fd);
		return nullptr;
	}
	// FIXME could throw, leaking file descriptor!
	std::unique_ptr<unsigned char[]> memblock(new unsigned char[*len]);
	auto r = read(fd, memblock.get(), *len);
	close(fd);
	if(r < 0 || r != flen){
		throw std::ifstream::failure("error reading file");
	}
	return memblock;
}

}
