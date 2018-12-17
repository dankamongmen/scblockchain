#include <cstring>
#include <cstdlib>
#include <iostream>
#include <libcatena/chain.h>
#include <catena/readline.h>
#include <readline/history.h>
#include <readline/readline.h>

namespace Catena {

ReadlineUI::ReadlineUI():
	cancelled(false) {
	// disable tab-completion and restore standard behavior
	rl_bind_key('\t', rl_insert);
}

void ReadlineUI::HandleQuit(Catena::Chain& chain __attribute__ ((unused))){
	cancelled = true;
}

void ReadlineUI::HandleShow(Catena::Chain& chain){
	std::cout << chain << std::flush;
}

void ReadlineUI::InputLoop(Catena::Chain& chain){
	const struct {
		const char* cmd;
		void (Catena::ReadlineUI::* fxn)(Catena::Chain&);
		const char* help;
	} cmdtable[] = {
		{ .cmd = "quit", .fxn = &ReadlineUI::HandleQuit, .help = "exit catena", },
		{ .cmd = "show", .fxn = &ReadlineUI::HandleShow, .help = "show blocks", },
		{ .cmd = nullptr, .fxn = nullptr, .help = nullptr, },
	}, *c;
	char* line;
	while(!cancelled){
		line = readline("catena] ");
		if(line == nullptr){
			break;
		}
		add_history(line);
		for(c = cmdtable ; c->cmd ; ++c){
			if(strcmp(line, c->cmd) == 0){
				(*this.*(c->fxn))(chain);
				break;
			}
		}
		if(c->cmd == nullptr && strcmp(line, "help")){
			std::cerr << "unknown command: " << line << std::endl;
		}else if(c->cmd == nullptr){
			for(c = cmdtable ; c->cmd ; ++c){
				std::cout << c->cmd << ": " << c->help << '\n';
			}
			std::cout << "help: list commands" << std::endl;
		}
		free(line);

	}
}

}
