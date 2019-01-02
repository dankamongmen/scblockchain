#include <cstdlib>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <libcatena/sig.h>
#include <libcatena/chain.h>
#include <libcatena/utility.h>
#include "catena/httpd.h"
#include "catena/readline.h"

using namespace CatenaAgent;

static void usage(std::ostream& os, const char* name){
	os << "usage: " << name << " -h | options\n";
	os << " -h: print usage information\n";
	os << " -l ledger: specify ledger file\n";
	os << " -v keyfile,txspec: provide authentication material (may be used multiple times)\n";
	os << " -p port: provide HTTP service on port, 0 to disable\n";
	os << " -d: daemonize\n";
}

static const auto DEFAULT_HTTP_PORT = 8080;

int main(int argc, char **argv){
	std::vector<std::pair<std::string, Catena::TXSpec>> keys;
	std::string privkey_file; // valid iff privkey_file != nullptr
	const char* chain_file = nullptr;
	unsigned short httpd_port = DEFAULT_HTTP_PORT;
	bool daemonize = false;
	int c;
	while(-1 != (c = getopt(argc, argv, "v:l:p:hd"))){
		switch(c){
		case 'd':
			daemonize = true;
			break;
		case 'p':{
			char *end;
			auto port = strtol(optarg, &end, 0);
			if(port > 0xffffu || port < 0 || *end){
				std::cerr << "Bad value for port: " << optarg << std::endl;
				usage(std::cerr, argv[0]);
				return EXIT_FAILURE;
			}
			httpd_port = port;
			break;
		}case 'v':{
			const char* delim = strchr(optarg, ',');
			if(delim == nullptr || delim == optarg){
				std::cerr << "format: -v keyfile,txhash.txidx" << std::endl;
				usage(std::cerr, argv[0]);
				return EXIT_FAILURE;
			}
			try{
				auto tx = Catena::StrToTXSpec(delim + 1);
				keys.emplace_back(std::string(optarg, delim - optarg), tx);

			}catch(Catena::ConvertInputException& e){
				std::cerr << "format: -v keyfile,txhash.txidx (" << e.what() << ")" << std::endl;
				usage(std::cerr, argv[0]);
				return EXIT_FAILURE;
			}
			break;
		}case 'h':
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
	Catena::IgnoreSignal(SIGPIPE);
	if(chain_file == nullptr){
		std::cerr << "ledger must be specified with -l" << std::endl;
		usage(std::cerr, argv[0]);
		return EXIT_FAILURE;
	}
	std::cout << "Loading ledger from " << chain_file << std::endl;
	try{
		// FIXME we'll want to provide privkey prior to loading the
		// chain, since we need it to decode LookupAuth transactions...
		Catena::Chain chain(chain_file);
		for(auto& k : keys){
			try{
				Catena::Keypair kp(k.first);
				chain.AddPrivateKey(k.second, kp);
			}catch(Catena::KeypairException& e){
				std::cerr << e.what() << std::endl;
				return EXIT_FAILURE;
			}
		}
		std::unique_ptr<HTTPDServer> httpd;
		if(httpd_port){
			httpd = std::make_unique<HTTPDServer>(chain, httpd_port);
		}
		if(daemonize){
			// FIXME daemonize out, wait on signal
		}else{
			ReadlineUI rline(chain);
			rline.InputLoop();
		}
	}catch(std::ifstream::failure& e){
		std::cerr << chain_file << ": " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
