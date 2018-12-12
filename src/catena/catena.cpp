#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include "libcatena/sig.h"

static void usage(std::ostream& os, const char* name){
	os << "usage: " << name << " [ -h ] [ -u pubkey -v privkey ]" << std::endl;
}

int main(int argc, char **argv){
	const char* privkey_file = NULL;
	const char* pubkey_file = NULL;
	int c;
	while(-1 != (c = getopt(argc, argv, "u:v:h"))){
		switch(c){
		case 'u':
			pubkey_file = optarg;
			break;
		case 'v':
			privkey_file = optarg;
			break;
		case 'h':
			usage(std::cout, argv[0]);
			return EXIT_SUCCESS;
		default:
			usage(std::cerr, argv[0]);
			return EXIT_FAILURE;
		}
	}
	if(pubkey_file || privkey_file){
		if(!pubkey_file || !privkey_file){
			std::cerr << "-u must be provided with -v" << std::endl;
			usage(std::cerr, argv[0]);
			return EXIT_FAILURE;
		}
		Catena::Keypair kp(pubkey_file, privkey_file);
	}
	return EXIT_SUCCESS;
}
