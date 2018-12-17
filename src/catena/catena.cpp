#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <libcatena/sig.h>
#include <libcatena/chain.h>
#include "catena/readline.h"

static void usage(std::ostream& os, const char* name){
	os << "usage: " << name << " -h | options\n";
	os << "\t-h: print usage information\n";
	os << "\t-l ledger: specify ledger file\n";
	os << "\t-u pubkey -v privkey: provide authentication material\n";
	os << "\t-d: daemonize\n";
	os << std::endl;
}

int main(int argc, char **argv){
	const char* privkey_file = nullptr;
	const char* pubkey_file = nullptr;
	const char* chain_file = nullptr;
	bool daemonize = false;
	int c;
	while(-1 != (c = getopt(argc, argv, "u:v:l:hd"))){
		switch(c){
		case 'd':
			daemonize = true;
			break;
		case 'u':
			pubkey_file = optarg;
			break;
		case 'v':
			privkey_file = optarg;
			break;
		case 'h':
			usage(std::cout, argv[0]);
			return EXIT_SUCCESS;
		case 'l':
			chain_file = optarg;
			break;
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
	if(chain_file == nullptr){
		std::cerr << "ledger must be specified with -l" << std::endl;
		usage(std::cerr, argv[0]);
		return EXIT_FAILURE;
	}
	std::cout << "Loading ledger from " << chain_file << std::endl;
	Catena::Chain chain(chain_file);
	// FIXME start up web server for HTTP API
	if(daemonize){
		// FIXME daemonize out
	}else{
		Catena::ReadlineUI rline;
		rline.InputLoop();
	}
	return EXIT_SUCCESS;
}
