#include <cstdlib>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <libcatena/rpc.h>
#include <libcatena/sig.h>
#include <libcatena/chain.h>
#include <libcatena/utility.h>
#include "catena/readline.h"
#include "catena/httpd.h"

using namespace CatenaAgent;

static constexpr auto DEFAULT_HTTP_PORT = 8080;
static constexpr auto DEFAULT_RPC_PORT = 0; // default to no incoming RPC

static void usage(std::ostream& os, const char* name, int exitcode)
	__attribute__ ((noreturn));

static void usage(std::ostream& os, const char* name, int exitcode){
	os << "usage: " << name << " -h | options\n";
	os << " -l ledger: specify ledger file\n";
	os << " -k keyfile,txspec: provide authentication material (may be used multiple times)\n";
	os << " -p port: HTTP service port, 0 to disable, default: " << DEFAULT_HTTP_PORT << "\n";
	os << " -r port: RPC service port, 0 to disable, default: " << DEFAULT_RPC_PORT << "\n";
	os << " -C certchain: certificate chain for RPC authentication\n";
	os << " -P peerfile: file containing initial RPC peers\n";
  os << " -A addrs: comma-delimited list of addresses to advertise\n";
	os << " -v keyfile: file containing PEM key for RPC authentication\n";
	os << " -h: print usage information\n";
	os << " -d: daemonize\n";
	os << std::flush;
	exit(exitcode);
}

int main(int argc, char **argv){
	std::vector<std::pair<std::string, Catena::TXSpec>> keys;
	unsigned short httpd_port = DEFAULT_HTTP_PORT;
  std::vector<std::string> addresses;
	const char* ledger_file = nullptr;
	const char* chain_file = nullptr;
	const char* peer_file = nullptr;
	const char* key_file = nullptr;
	auto rpc_port = DEFAULT_RPC_PORT;
	bool daemonize = false;
	int c;
	while(-1 != (c = getopt(argc, argv, "A:P:C:k:l:p:r:v:hd"))){
		switch(c){
		case 'd':
			daemonize = true;
			break;
		case 'p':{
			try{
				httpd_port = Catena::StrToLong(optarg, 0, 65535);
			}catch(Catena::ConvertInputException& e){
				std::cerr << "bad value for HTTP port: " << e.what() << std::endl;
				usage(std::cerr, argv[0], EXIT_FAILURE);
			}
			break;
		}case 'r':{
			try{
				rpc_port = Catena::StrToLong(optarg, 0, 65535);
			}catch(Catena::ConvertInputException& e){
				std::cerr << "bad value for RPC port: " << e.what() << std::endl;
				usage(std::cerr, argv[0], EXIT_FAILURE);
			}
			break;
    }case 'A':{ // may be provided multiple times
      std::stringstream ss(optarg);
      while(ss.good()){
        std::string substr;
        getline(ss, substr, ',');
        addresses.push_back(substr);
      }
      break;
		}case 'v':{
			if(key_file){
				std::cerr << "key file may only be specified once" << std::endl;
				usage(std::cerr, argv[0], EXIT_FAILURE);
			}
			key_file = optarg;
			break;
		}case 'P':{
			if(peer_file){
				std::cerr << "peer file may only be specified once" << std::endl;
				usage(std::cerr, argv[0], EXIT_FAILURE);
			}
			peer_file = optarg;
			break;
		}case 'C':{
			if(chain_file){
				std::cerr << "cert chain file may only be specified once" << std::endl;
				usage(std::cerr, argv[0], EXIT_FAILURE);
			}
			chain_file = optarg;
			break;
		}case 'k':{
			const char* delim = strchr(optarg, ',');
			if(delim == nullptr || delim == optarg){
				std::cerr << "format: -k keyfile,txhash.txidx" << std::endl;
				usage(std::cerr, argv[0], EXIT_FAILURE);
			}
			try{
				auto tx = Catena::TXSpec::StrToTXSpec(delim + 1);
				keys.emplace_back(std::string(optarg, delim - optarg), tx);

			}catch(Catena::ConvertInputException& e){
				std::cerr << "format: -k keyfile,txhash.txidx (" << e.what() << ")" << std::endl;
				usage(std::cerr, argv[0], EXIT_FAILURE);
			}
			break;
		}case 'l':
			ledger_file = optarg;
			break;
		case 'h':
			usage(std::cout, argv[0], EXIT_SUCCESS);
		default:
			usage(std::cout, argv[0], EXIT_FAILURE);
		}
	}
	if(argv[optind]){
		std::cerr << "unexpected arguments" << std::endl;
		usage(std::cerr, argv[0], EXIT_FAILURE);
	}
	Catena::IgnoreSignal(SIGPIPE);
	if(ledger_file == nullptr){
		std::cerr << "ledger must be specified with -l" << std::endl;
		usage(std::cerr, argv[0], EXIT_FAILURE);
	}
	if(rpc_port && (chain_file == nullptr || key_file == nullptr)){
		std::cerr << "-C and -v must be specified for RPC" << std::endl;
		usage(std::cerr, argv[0], EXIT_FAILURE);
	}else if(rpc_port == 0 && (chain_file || peer_file || key_file)){
		std::cerr << "warning: -C/-P/-v have no meaning without -r" << std::endl;
		// not a fatal error
	}else if(rpc_port == 0 && addresses.size()){
    std::cerr << "error: -A cannot be used without -r" << std::endl;
  }
	try{
		std::cout << "Loading ledger from " << ledger_file << std::endl;
		// FIXME we'll want to provide privkey prior to loading the
		// chain, since we need it to decode LookupAuth transactions...
		Catena::Chain chain(ledger_file);
		for(auto& k : keys){
			try{
				std::cout << "Loading private key from " << k.first << std::endl;
				Catena::Keypair kp(k.first);
				chain.AddPrivateKey(k.second, kp);
			}catch(Catena::KeypairException& e){
				std::cerr << e.what() << std::endl;
				return EXIT_FAILURE;
			}
		}
		// These don't have defauult constructors, so declare pointers
		// to them, and only set those pointers when we need them.
		std::unique_ptr<HTTPDServer> httpd;
		std::unique_ptr<Catena::RPCService> rpcd;
		if(rpc_port){
			std::cout << "Enabling RPC on port " << rpc_port << std::endl;
      const Catena::RPCServiceOptions opts = {
        .port = rpc_port,
        .chainfile = chain_file,
        .keyfile = key_file,
        .addresses = addresses,
      };
			chain.EnableRPC(opts);
			if(peer_file){
				chain.AddPeers(peer_file);
			}
		}
		if(httpd_port){
			std::cout << "Enabling HTTP on port " << httpd_port << std::endl;
			httpd = std::make_unique<HTTPDServer>(chain, httpd_port);
		}
		if(daemonize){
			std::cout << "Daemonizing..." << std::endl;
			return EXIT_FAILURE; // FIXME daemonize, wait on signal
		}else{
			ReadlineUI rline(chain);
			rline.InputLoop();
		}
		return EXIT_SUCCESS;
	}catch(Catena::NetworkException& e){
		std::cerr << e.what() << std::endl;
	}catch(Catena::ConvertInputException& e){
		std::cerr << e.what() << std::endl;
	}catch(Catena::BlockHeaderException& e){
		std::cerr << ledger_file << ": " << e.what() << std::endl;
	}catch(Catena::BlockValidationException& e){
		std::cerr << ledger_file << ": " << e.what() << std::endl;
	}catch(std::ifstream::failure& e){
		std::cerr << ledger_file << ": " << e.what() << std::endl;
	}
	return EXIT_FAILURE;
}
