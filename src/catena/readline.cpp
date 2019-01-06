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
#include <libcatena/truststore.h>
#include <catena/readline.h>

#define ANSI_WHITE "\033[1;37m"
#define ANSI_DGREY "\033[1;30m"
#define ANSI_GREY "\033[0;37m"
#define ANSI_GREEN "\033[1;32m"

namespace Catena {

template <typename Iterator> std::ostream&
DumpTransactions(std::ostream& s, const Iterator begin, const Iterator end){
	s << ANSI_GREY;
	char prevfill = s.fill('0');
	int i = 0;
	while(begin + i != end){
		s << std::setw(5) << i << " " << begin[i].get() << "\n";
		++i;
	}
	s.fill(prevfill);
	return s;
}

std::ostream& operator<<(std::ostream& stream, const Block& b){
	return DumpTransactions(stream, b.transactions.begin(), b.transactions.end());
}

std::ostream& operator<<(std::ostream& stream, const BlockHeader& bh){
	stream << ANSI_WHITE;
	char prevfill = stream.fill('0');
	stream << std::setw(8) << bh.txidx <<  " v" << bh.version << " transactions: " << bh.txcount <<
		" bytes: " << bh.totlen << " ";
	stream.fill(prevfill);
	char buf[80];
	time_t btime = bh.utc;
	if(ctime_r(&btime, buf)){
		stream << buf; // has its own newline
	}else{
		stream << "\n";
	}
	stream << ANSI_GREY " hash: " << bh.hash << ANSI_DGREY "\n prev: " << bh.prev << ANSI_WHITE "\n";
	return stream;
}

std::ostream& operator<<(std::ostream& stream, const BlockDetail& b){
	stream << b.bhdr;
	DumpTransactions(stream, b.transactions.begin(), b.transactions.end());
	return stream;
}

std::ostream& operator<<(std::ostream& stream, const Blocks& blocks){
	for(auto& h : blocks.headers){
		stream << h;
	}
	return stream;
}

std::ostream& operator<<(std::ostream& stream, const Chain& chain){
	stream << chain.blocks;
	return stream;
}

}

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
int ReadlineUI::Summary(const Iterator start, const Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	std::cout << "chain bytes: " << chain.Size() << "\n";
	std::cout << "consortium members: " << chain.ConsortiumMemberCount() << "\n";
	std::cout << "lookup requests: " << chain.LookupRequestCount() << "\n";
	std::cout << "users: " << chain.UserCount() << "\n";
	std::cout << "status delegations: " << chain.StatusDelegationCount() << "\n";
	std::cout << std::flush;
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
	for(auto it = det.begin() ; it != det.end() ; ++it){
		std::cout << *it;
		if(std::next(it) != det.end()){
			std::cout << "\n";
		}
	}
	std::cout << std::flush;
	return 0;
}

template <typename Iterator>
int ReadlineUI::Outstanding(const Iterator start, const Iterator end){
	if(start != end){
		std::cerr << "command does not accept arguments" << std::endl;
		return -1;
	}
	std::cout << chain.OutstandingTXs();
	const auto p = chain.SerializeOutstanding();
	Catena::HexOutput(std::cout, p.first.get(), p.second) << std::endl;
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
		std::cerr << "Error committing: " << e.what() << std::endl;
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

std::ostream& ReadlineUI::MemberSummary(std::ostream& s, const Catena::ConsortiumMemberSummary& cm) const {
	s << cm.cmspec << " (" << cm.users << " users) ";
	s << ANSI_GREY << std::setw(1) << cm.payload << ANSI_WHITE << std::endl;
	return s;
}

template <typename Iterator>
int ReadlineUI::GetMembers(const Iterator start, const Iterator end) {
	if(end - 1 > start){
		std::cerr << "command requires at most one argument: member TXSpec" << std::endl;
		return -1;
	}
	if(end != start){
		try {
			const auto& txspec = Catena::TXSpec::StrToTXSpec(start[0]);
			const auto& cm = chain.ConsortiumMember(txspec);
			MemberSummary(std::cout, cm);
			const auto& users = chain.ConsortiumUsers(txspec);
			std::cout << ANSI_GREY;
			for(const auto& u : users){
				std::cout << u.uspec << "\n";
			}
			return 0;
		}catch(Catena::ConvertInputException& e){
			std::cerr << "couldn't extract txspec (" << e.what() << ")" << std::endl;
		}catch(Catena::InvalidTXSpecException& e){
			std::cerr << "bad txpsec (" << e.what() << ")" << std::endl;
		}
	}
	const auto& cmembers = chain.ConsortiumMembers();
	for(const auto& cm : cmembers){
		MemberSummary(std::cout, cm);
	}
	return 0;
}

template <typename Iterator>
int ReadlineUI::NewMember(const Iterator start, const Iterator end){
	if(end - start != 3){
		std::cerr << "3 arguments required: signing key TXSpec, public key file, JSON payload" << std::endl;
		return -1;
	}
	const auto& keyfile = start[1];
	const auto& json = start[2];
	try{
		const auto& txspec = Catena::TXSpec::StrToTXSpec(start[0]);
		auto payload = nlohmann::json::parse(json);
		size_t plen;
		auto pkey = Catena::ReadBinaryFile(keyfile, &plen);
		chain.AddConsortiumMember(txspec, pkey.get(), plen, payload);
		return 0;
	}catch(std::ifstream::failure& e){
		std::cerr << "couldn't read a public key from " << keyfile << std::endl;
	}catch(Catena::SigningException& e){
		std::cerr << "couldn't sign transaction (" << e.what() << ")" << std::endl;
	}catch(nlohmann::detail::parse_error& e){
		std::cerr << "couldn't parse JSON from '" << json << "'" << std::endl;
	}catch(Catena::ConvertInputException& e){
		std::cerr << "couldn't extract hashspec (" << e.what() << ")" << std::endl;
	}
	return -1;
}

template <typename Iterator>
int ReadlineUI::NewUser(const Iterator start, const Iterator end){
	if(end - start != 4){
		std::cerr << "4 arguments required: ConsortiumMember spec, public key file, symmetric key file, JSON payload" << std::endl;
		return -1;
	}
	const auto& json = start[3];
	try{
		auto cmspec = Catena::TXSpec::StrToTXSpec(start[0]);
		size_t plen, symlen;
		auto pkey = Catena::ReadBinaryFile(start[1], &plen);
		auto symkey = Catena::ReadBinaryFile(start[2], &symlen);
		Catena::SymmetricKey skey;
		if(symlen != skey.size()){
			std::cerr << "invalid " << symlen << "b symmetric key at " << start[2] << std::endl;
			return -1;
		}
		memcpy(skey.data(), symkey.get(), skey.size());
		auto payload = nlohmann::json::parse(json);
		chain.AddUser(cmspec, pkey.get(), plen, skey, payload);
		return 0;
	}catch(std::ifstream::failure& e){
		std::cerr << "couldn't read a key (" << e.what() << ")" << std::endl;
	}catch(Catena::SigningException& e){
		std::cerr << "couldn't sign transaction (" << e.what() << ")" << std::endl;
	}catch(nlohmann::detail::parse_error& e){
		std::cerr << "couldn't parse JSON from '" << json << "'" << std::endl;
	}
	return -1;
}

template <typename Iterator>
int ReadlineUI::NewLookupAuthReq(const Iterator start, const Iterator end){
	if(start + 3 != end){
		std::cerr << "3 arguments required: ConsortiumMember spec, ExternalLookup spec, JSON payload" << std::endl;
		return -1;
	}
	const auto& json = start[2];
	try{
		auto cmspec = Catena::TXSpec::StrToTXSpec(start[0]);
		auto elspec = Catena::TXSpec::StrToTXSpec(start[1]);
		auto payload = nlohmann::json::parse(json);
		chain.AddLookupAuthReq(cmspec, elspec, payload);
		return 0;
	}catch(Catena::SigningException& e){
		std::cerr << "couldn't sign transaction (" << e.what() << ")" << std::endl;
	}catch(Catena::ConvertInputException& e){
		std::cerr << "couldn't extract hashspec (" << e.what() << ")" << std::endl;
	}catch(nlohmann::detail::parse_error& e){
		std::cerr << "couldn't parse JSON from '" << json << "'" << std::endl;
	}
	return -1;
}

template <typename Iterator>
int ReadlineUI::NewLookupAuth(const Iterator start, const Iterator end){
	// FIXME make symmetric key file optional for anonymous passthrough
	if(start + 3 != end){
		std::cerr << "3 arguments required: LookupAuthReq spec, User spec, symmetric key file" << std::endl;
		return -1;
	}
	try{
		auto larspec = Catena::TXSpec::StrToTXSpec(start[0]);
		auto uspec = Catena::TXSpec::StrToTXSpec(start[1]);
		size_t symlen;
		auto symkey = Catena::ReadBinaryFile(start[2], &symlen);
		Catena::SymmetricKey skey;
		if(symlen != skey.size()){
			std::cerr << "invalid " << symlen << "b symmetric key at " << start[2] << std::endl;
			return -1;
		}
		memcpy(skey.data(), symkey.get(), skey.size());
		chain.AddLookupAuth(larspec, uspec, skey);
		return 0;
	}catch(std::ifstream::failure& e){
		std::cerr << "couldn't read symmetric key (" << e.what() << ")" << std::endl;
	}catch(Catena::SigningException& e){
		std::cerr << "couldn't sign transaction (" << e.what() << ")" << std::endl;
	}catch(Catena::ConvertInputException& e){
		std::cerr << "couldn't extract hashspec (" << e.what() << ")" << std::endl;
	}
	return -1;
}

template <typename Iterator>
int ReadlineUI::NewUserStatus(const Iterator start, const Iterator end){
	if(start + 2 != end){
		std::cerr << "2 arguments required: UserStatusDelegation spec, JSON payload" << std::endl;
		return -1;
	}
	const auto& json = start[1];
	try{
		auto usdspec = Catena::TXSpec::StrToTXSpec(start[0]);
		auto payload = nlohmann::json::parse(json);
		chain.AddUserStatus(usdspec, payload);
		return 0;
	}catch(Catena::InvalidTXSpecException& e){
		std::cerr << "bad UserStatusDelegation (" << e.what() << ")" << std::endl;
	}catch(Catena::ConvertInputException& e){
		std::cerr << "couldn't extract TXspec (" << e.what() << ")" << std::endl;
	}catch(Catena::SigningException& e){
		std::cerr << "couldn't sign transaction (" << e.what() << ")" << std::endl;
	}catch(nlohmann::detail::parse_error& e){
		std::cerr << "couldn't parse JSON from '" << json << "'" << std::endl;
	}
	return -1;
}

template <typename Iterator>
int ReadlineUI::GetUserStatus(const Iterator start, const Iterator end){
	if(start + 2 != end){
		std::cerr << "2 arguments required: User spec, status type" << std::endl;
		return -1;
	}
	try{
		auto uspec = Catena::TXSpec::StrToTXSpec(start[0]);
		auto stype = Catena::StrToLong(start[1], 0, LONG_MAX);
		auto json = chain.UserStatus(uspec, stype);
		std::cout << json.dump() << "\n";
		return 0;
	}catch(Catena::UserStatusException& e){
		std::cerr << "couldn't get status (" << e.what() << ")" << std::endl;
	}catch(Catena::ConvertInputException& e){
		std::cerr << "couldn't extract TXspec (" << e.what() << ")" << std::endl;
	}
	return -1;
}

template <typename Iterator>
int ReadlineUI::NewUserStatusDelegation(const Iterator start, const Iterator end){
	if(start + 4 != end){
		std::cerr << "4 arguments required: User spec, ConsortiumMember spec, status type, JSON payload" << std::endl;
		return -1;
	}
	const auto& json = start[3];
	try{
		auto uspec = Catena::TXSpec::StrToTXSpec(start[0]);
		auto cmspec = Catena::TXSpec::StrToTXSpec(start[1]);
		auto stype = Catena::StrToLong(start[2], 0, LONG_MAX);
		auto payload = nlohmann::json::parse(json);
		chain.AddUserStatusDelegation(cmspec, uspec, stype, payload);
		return 0;
	}catch(Catena::ConvertInputException& e){
		std::cerr << "bad argument (" << e.what() << ")" << std::endl;
	}catch(nlohmann::detail::parse_error& e){
		std::cerr << "couldn't parse JSON from '" << json << "'" << std::endl;
	}catch(Catena::SigningException& e){
		std::cerr << "couldn't sign transaction (" << e.what() << ")" << std::endl;
	}
	return -1;
}

template <typename Iterator>
int ReadlineUI::NewExternalLookup(const Iterator start, const Iterator end){
	if(end - start != 4){
		std::cerr << "4 arguments required: signing key spec, lookup type, public key file, external ID" << std::endl;
		return -1;
	}
	long ltype;
	try{
		ltype = Catena::StrToLong(start[1], 0, 65535);
	}catch(Catena::ConvertInputException& e){
		std::cerr << "couldn't parse lookup type from " << start[1] << " (" << e.what() << ")" << std::endl;
		return -1;
	}
	const auto& keyfile = start[2];
	const auto& extid = start[3];
	try{
		size_t plen;
		auto pkey = Catena::ReadBinaryFile(keyfile, &plen);
		const auto& signspec = Catena::TXSpec::StrToTXSpec(start[0]);
		chain.AddExternalLookup(signspec, pkey.get(), plen, extid, ltype);
		return 0;
	}catch(std::ifstream::failure& e){
		std::cerr << "couldn't read a public key from " << keyfile << std::endl;
	}catch(Catena::SigningException& e){
		std::cerr << "couldn't sign transaction (" << e.what() << ")" << std::endl;
	}catch(Catena::ConvertInputException& e){
		std::cerr << "argument error (" << e.what() << ")" << std::endl;
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
		{ .cmd = "summary", .fxn = &ReadlineUI::Summary, .help = "summarize the ledger", },
		{ .cmd = "inspect", .fxn = &ReadlineUI::Inspect, .help = "detailed view of a range of blocks", },
		{ .cmd = "outstanding", .fxn = &ReadlineUI::Outstanding, .help = "show outstanding transactions", },
		{ .cmd = "commit", .fxn = &ReadlineUI::CommitOutstanding, "commit outstanding transactions to ledger", },
		{ .cmd = "flush", .fxn = &ReadlineUI::FlushOutstanding, "flush (drop) outstanding transactions", },
		{ .cmd = "tstore", .fxn = &ReadlineUI::TStore, .help = "dump trust store (key info)", },
		{ .cmd = "member", .fxn = &ReadlineUI::NewMember, .help = "create new ConsortiumMember transaction", },
		{ .cmd = "getmembers", .fxn = &ReadlineUI::GetMembers, .help = "list consortium members, or one with detail", },
		{ .cmd = "exlookup", .fxn = &ReadlineUI::NewExternalLookup, .help = "create new ExternalLookup transaction", },
		{ .cmd = "lauthreq", .fxn = &ReadlineUI::NewLookupAuthReq, .help = "create new LookupAuthorizationRequest transaction", },
		{ .cmd = "lauth", .fxn = &ReadlineUI::NewLookupAuth, .help = "create new LookupAuthorization transaction", },
		{ .cmd = "patient", .fxn = &ReadlineUI::NewUser, .help = "create new User transaction", },
		{ .cmd = "delustatus", .fxn = &ReadlineUI::NewUserStatusDelegation, .help = "create new UserStatusDelegation transaction", },
		{ .cmd = "ustatus", .fxn = &ReadlineUI::NewUserStatus, .help = "create new UserStatus transaction", },
		{ .cmd = "getustatus", .fxn = &ReadlineUI::GetUserStatus, .help = "look up a patient's status", },
		{ .cmd = "", .fxn = nullptr, .help = "", },
	}, *c;
	char* line;
	while(!cancelled){
		line = readline(RL_START "\033[0;35m" RL_END
				"[" RL_START "\033[0;36m" RL_END
				"catena" RL_START "\033[0;35m" RL_END
				"] " RL_START ANSI_WHITE RL_END);
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
				std::cout << c->cmd << ANSI_GREY " " << c->help << ANSI_WHITE "\n";
			}
			std::cout << "help" ANSI_GREY ": list commands" ANSI_WHITE << std::endl;
		}
		free(line);
	}
	if(!cancelled){ // got an EOF, probably
		std::cout << std::endl;
	}
}

}
