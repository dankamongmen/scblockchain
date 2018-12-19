#include <term.h>
#include <unistd.h>
#include <ncurses.h>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <libcatena/tx.h>
#include <libcatena/chain.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <catena/readline.h>

namespace Catena {

ReadlineUI::ReadlineUI(Catena::Chain& chain) :
	cancelled(false),
	chain(chain) {
	// disable tab-completion and restore standard behavior
	rl_bind_key('\t', rl_insert);
}

template <typename Iterator>
int ReadlineUI::HandleQuit(Iterator start, Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	cancelled = true;
	return 0;
}

template <typename Iterator>
int ReadlineUI::HandleShow(Iterator start, Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	std::cout << chain << std::flush;
	return 0;
}

template <typename Iterator>
int ReadlineUI::HandleOutstanding(Iterator start, Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	chain.DumpOutstanding(std::cout) << std::flush;
	return 0;
}

template <typename Iterator>
int ReadlineUI::HandleTStore(Iterator start, Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	chain.DumpTrustStore(std::cout) << std::flush;
	return 0;
}

template <typename Iterator>
int ReadlineUI::HandleNoOp(Iterator start, Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	chain.AddNoOp();
	return 0;
}

template <typename Iterator>
int ReadlineUI::HandleNewMember(Iterator start, Iterator end){
	while(start != end){
		++start;
	}
	// FIXME construct new TX
	chain.AddConsortiumMember(/*FIXME*/);
	return 0;
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
		int (Catena::ReadlineUI::* fxn)(std::vector<std::string>::iterator,
						std::vector<std::string>::iterator);
		const char* help;
	} cmdtable[] = {
		{ .cmd = "quit", .fxn = &ReadlineUI::HandleQuit, .help = "exit catena", },
		{ .cmd = "show", .fxn = &ReadlineUI::HandleShow, .help = "show blocks", },
		{ .cmd = "outstanding", .fxn = &ReadlineUI::HandleOutstanding, .help = "show outstanding transactions", },
		{ .cmd = "tstore", .fxn = &ReadlineUI::HandleTStore, .help = "dump trust store (key info)", },
		{ .cmd = "noop", .fxn = &ReadlineUI::HandleNoOp, .help = "create new NoOp transaction", },
		{ .cmd = "member", .fxn = &ReadlineUI::HandleNewMember, .help = "create new ConsortiumMember transaction", },
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
