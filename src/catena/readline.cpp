#include <term.h>
#include <unistd.h>
#include <ncurses.h>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <json.hpp>
#include <libcatena/tx.h>
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
int ReadlineUI::Quit(Iterator start, Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	cancelled = true;
	return 0;
}

template <typename Iterator>
int ReadlineUI::Show(Iterator start, Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	std::cout << chain << std::flush;
	return 0;
}

template <typename Iterator>
int ReadlineUI::Inspect(Iterator start, Iterator end){
	if(start + 1 < end){
		std::cerr << "command requires at most one argument" << std::endl;
		return -1;
	}
	// FIXME inspect range, display result
	return 0;
}

template <typename Iterator>
int ReadlineUI::Outstanding(Iterator start, Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	chain.DumpOutstanding(std::cout) << std::flush;
	return 0;
}

template <typename Iterator>
int ReadlineUI::CommitOutstanding(Iterator start, Iterator end){
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
int ReadlineUI::FlushOutstanding(Iterator start, Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	chain.FlushOutstanding();
	return 0;
}

template <typename Iterator>
int ReadlineUI::TStore(Iterator start, Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	chain.DumpTrustStore(std::cout) << std::flush;
	return 0;
}

template <typename Iterator>
int ReadlineUI::NoOp(Iterator start, Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	chain.AddNoOp();
	return 0;
}

template <typename Iterator>
int ReadlineUI::NewMember(Iterator start, Iterator end){
	if(end - start != 2){
		std::cerr << "command requires two arguments, a public key file and a JSON payload" << std::endl;
		return -1;
	}
	size_t plen;
	try{
		auto payload = nlohmann::json::parse(start[1]);
		try{
			auto pkey = Catena::ReadBinaryFile(start[0], &plen);
			chain.AddConsortiumMember(pkey.get(), plen, payload);
			return 0;
		}catch(std::ifstream::failure& e){
			std::cerr << "couldn't read a public key from " << start[0] << std::endl;
		}
	}catch(nlohmann::detail::parse_error &e){
		std::cerr << "couldn't parse JSON from '" << start[1] << "'" << std::endl;
	}
	return -1;
}

std::vector<std::string> ReadlineUI::SplitInput(const char* line) const {
	std::vector<std::string> tokens;
	int soffset = -1, offset = 0;
	char c;
	while( (c = line[offset]) ){
		if(isspace(c)){
			if(soffset != -1){
				tokens.emplace_back(line + soffset, offset - soffset);
				soffset = -1;
			}
		}else if(soffset == -1){
			soffset = offset;
		}
		++offset;
	}
	if(soffset != -1){
		tokens.emplace_back(line + soffset, offset - soffset);
	}
	return tokens;
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
		{ .cmd = "noop", .fxn = &ReadlineUI::NoOp, .help = "create new NoOp transaction", },
		{ .cmd = "member", .fxn = &ReadlineUI::NewMember, .help = "create new ConsortiumMember transaction", },
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
		auto tokens = SplitInput(line);
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
