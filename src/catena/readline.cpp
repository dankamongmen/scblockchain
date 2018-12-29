#include <term.h>
#include <unistd.h>
#include <ncurses.h>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <libcatena/tx.h>
#include <nlohmann/json.hpp>
#include <libcatena/chain.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <libcatena/utility.h>
#include <catena/readline.h>

namespace CatenaAgent {

ReadlineUI::ReadlineUI(Catena::Chain& chain) :
	cancelled(false),
	chain(chain) { }

template <typename Iterator>
int ReadlineUI::Quit(const Iterator start, const Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	cancelled = true;
	return 0;
}

template <typename Iterator>
int ReadlineUI::Show(const Iterator start, const Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	std::cout << chain << std::flush;
	return 0;
}

template <typename Iterator>
int ReadlineUI::Inspect(const Iterator start, const Iterator end){
	int b1, b2;
	if(start + 2 < end){
		std::cerr << "command requires at most one argument" << std::endl;
		return -1;
	}else if(start == end){
		b1 = 0;
		b2 = -1;
	}else{
		try{
			b1 = Catena::StrToLong(start[0], 0, LONG_MAX);
			if(start + 1 < end){
				b2 = Catena::StrToLong(start[1], -1, LONG_MAX);
			}else{
				b2 = -1;
			}
		}catch(Catena::ConvertInputException& e){
			std::cerr << "couldn't parse argument (" << e.what() << ")" << std::endl;
			return -1;
		}
	}
	auto det = chain.Inspect(b1, b2);
	for(const auto& d : det){
		std::cout << d;
	}
	return 0;
}

template <typename Iterator>
int ReadlineUI::Outstanding(const Iterator start, const Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	chain.DumpOutstanding(std::cout) << std::flush;
	return 0;
}

template <typename Iterator>
int ReadlineUI::CommitOutstanding(const Iterator start, const Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	try{
		chain.CommitOutstanding();
	}catch(std::exception& e){
		std::cerr << "Error committing: " << e.what();
		return -1;
	}
	return 0;
}

template <typename Iterator>
int ReadlineUI::FlushOutstanding(const Iterator start, const Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	chain.FlushOutstanding();
	return 0;
}

template <typename Iterator>
int ReadlineUI::TStore(const Iterator start, const Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	chain.DumpTrustStore(std::cout) << std::flush;
	return 0;
}

template <typename Iterator>
int ReadlineUI::NewNoOp(const Iterator start, const Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	chain.AddNoOp();
	return 0;
}

template <typename Iterator>
int ReadlineUI::NewMember(const Iterator start, const Iterator end){
	if(end - start != 2){
		std::cerr << "command requires two arguments: public key file, JSON payload" << std::endl;
		return -1;
	}
	const auto& keyfile = start[0];
	const auto& json = start[1];
	try{
		auto payload = nlohmann::json::parse(json);
		try{
			size_t plen;
			auto pkey = Catena::ReadBinaryFile(keyfile, &plen);
			chain.AddConsortiumMember(pkey.get(), plen, payload);
			return 0;
		}catch(std::ifstream::failure& e){
			std::cerr << "couldn't read a public key from " << keyfile << std::endl;
		}catch(Catena::SigningException& e){
			std::cerr << "couldn't sign transaction (" << e.what() << ")" << std::endl;
		}
	}catch(nlohmann::detail::parse_error& e){
		std::cerr << "couldn't parse JSON from '" << json << "'" << std::endl;
	}
	return -1;
}

template <typename Iterator>
int ReadlineUI::NewPatient(const Iterator start, const Iterator end){
	(void)start;
	(void)end;
	return -1;
}

template <typename Iterator>
int ReadlineUI::NewLookupAuthReq(const Iterator start, const Iterator end){
	(void)start;
	(void)end;
	return -1;
}

template <typename Iterator>
int ReadlineUI::NewLookupAuth(const Iterator start, const Iterator end){
	(void)start;
	(void)end;
	return -1;
}

template <typename Iterator>
int ReadlineUI::NewPatientStatus(const Iterator start, const Iterator end){
	(void)start;
	(void)end;
	return -1;
}

template <typename Iterator>
int ReadlineUI::GetPatientStatus(const Iterator start, const Iterator end){
	(void)start;
	(void)end;
	return -1;
}

template <typename Iterator>
int ReadlineUI::NewExternalLookup(const Iterator start, const Iterator end){
	if(end - start != 3){
		std::cerr << "command requires three arguments: lookup type, public key file, external ID" << std::endl;
		return -1;
	}
	long ltype;
	try{
		ltype = Catena::StrToLong(start[0], 0, 65535);
	}catch(Catena::ConvertInputException& e){
		std::cerr << "couldn't parse lookup type from " << start[0] << " (" << e.what() << ")" << std::endl;
		return -1;
	}
	const auto& keyfile = start[1];
	const auto& extid = start[2];
	try{
		size_t plen;
		auto pkey = Catena::ReadBinaryFile(keyfile, &plen);
		chain.AddExternalLookup(pkey.get(), plen, extid, ltype);
		return 0;
	}catch(std::ifstream::failure& e){
		std::cerr << "couldn't read a public key from " << keyfile << std::endl;
	}catch(Catena::SigningException& e){
		std::cerr << "couldn't sign transaction (" << e.what() << ")" << std::endl;
	}
	return -1;
}

#define RL_START "\x01" // RL_PROMPT_START_IGNORE
#define RL_END "\x02"   // RL_PROMPT_END_IGNORE

void ReadlineUI::InputLoop(){
	const struct {
		const std::string cmd;
		int (ReadlineUI::* fxn)(std::vector<std::string>::iterator,
						std::vector<std::string>::iterator);
		const char* help;
	} cmdtable[] = {
		{ .cmd = "quit", .fxn = &ReadlineUI::Quit, .help = "exit catena", },
		{ .cmd = "show", .fxn = &ReadlineUI::Show, .help = "show blocks", },
		{ .cmd = "inspect", .fxn = &ReadlineUI::Inspect, .help = "detailed view of a range of blocks", },
		{ .cmd = "outstanding", .fxn = &ReadlineUI::Outstanding, .help = "show outstanding transactions", },
		{ .cmd = "commit", .fxn = &ReadlineUI::CommitOutstanding, "commit outstanding transactions to ledger", },
		{ .cmd = "flush", .fxn = &ReadlineUI::FlushOutstanding, "flush (drop) outstanding transactions", },
		{ .cmd = "tstore", .fxn = &ReadlineUI::TStore, .help = "dump trust store (key info)", },
		{ .cmd = "noop", .fxn = &ReadlineUI::NewNoOp, .help = "create new NoOp transaction", },
		{ .cmd = "member", .fxn = &ReadlineUI::NewMember, .help = "create new ConsortiumMember transaction", },
		{ .cmd = "exlookup", .fxn = &ReadlineUI::NewExternalLookup, .help = "create new ExternalLookup transaction", },
		{ .cmd = "lauthreq", .fxn = &ReadlineUI::NewLookupAuthReq, .help = "create new LookupAuthorizationRequest transaction", },
		{ .cmd = "lauth", .fxn = &ReadlineUI::NewLookupAuth, .help = "create new LookupAuthorization transaction", },
		{ .cmd = "patient", .fxn = &ReadlineUI::NewPatient, .help = "create new Patient transaction", },
		{ .cmd = "pstatus", .fxn = &ReadlineUI::NewPatientStatus, .help = "create new PatientStatus transaction", },
		{ .cmd = "getpstatus", .fxn = &ReadlineUI::GetPatientStatus, .help = "look up a patient's status", },
		{ .cmd = "", .fxn = nullptr, .help = "", },
	}, *c;
	char* line;
	while(!cancelled){
		line = readline(RL_START "\033[0;35m" RL_END
				"[" RL_START "\033[0;36m" RL_END
				"catena" RL_START "\033[0;35m" RL_END
				"] " RL_START "\033[1;37m" RL_END);
		if(line == nullptr){
			break;
		}
		std::vector<std::string> tokens;
		try{
			tokens = Catena::SplitInput(line);
		}catch(Catena::SplitInputException& e){
			std::cerr << "error tokenizing inputs: " << e.what() << std::endl;
			add_history(line);
			continue;
		}
		if(tokens.size() == 0){
			continue;
		}
		add_history(line);
		for(c = cmdtable ; c->fxn ; ++c){
			if(c->cmd == tokens[0]){
				(*this.*(c->fxn))(tokens.begin() + 1, tokens.end());
				break;
			}
		}
		if(c->fxn == nullptr && tokens[0] != "help"){
			std::cerr << "unknown command: " << tokens[0] << std::endl;
		}else if(c->fxn == nullptr){ // display help
			for(c = cmdtable ; c->fxn ; ++c){
				std::cout << c->cmd << ": " << c->help << '\n';
			}
			std::cout << "help: list commands" << std::endl;
		}
		free(line);
	}
	if(!cancelled){ // got an EOF, probably
		std::cout << std::endl;
	}
}

}
