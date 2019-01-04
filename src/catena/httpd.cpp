#include <sstream>
#include <iostream>
#include <unistd.h>
#include <microhttpd.h>
#include <libcatena/utility.h>
#include <libcatena/chain.h>
#include <libcatena/hash.h>
#include "catena/favicon.h"
#include "catena/version.h"
#include "catena/httpd.h"

namespace CatenaAgent {

HTTPDServer::~HTTPDServer(){
	if(mhd){
		MHD_stop_daemon(mhd);
	}
}

constexpr char htmlhdr[] =
 "<!DOCTYPE html>"
 "<html lang=\"en\"><link rel=\"stylesheet\" type=\"text/css\" href=\"//fonts.googleapis.com/css?family=Open+Sans\" />"
 "<head>"
 "<meta name=\"google\" content=\"notranslate\">"
 "<title>catena node</title>"
 "<style>"
 "body { font-family: \"Open Sans\"; margin: 5px ; padding: 5px; background: whitesmoke; }"
 "table { table-layout: fixed; border-collapse; collapse: border-spacing: 0; width: 90%; }"
 "td { width: 10%; background: #dfdfdf; border: 1px solid transparent; transition: all 0.3s; }"
 "td+td { width: auto; }"
 "tr:nth-child(even) td { background: #f1f1f1; }"
 "tr:nth-child(odd) td { background: #fefefe; }"
 "tr td:hover { background: #00dddd; }"
 "div { margin: 1em; }"
 "span { margin: 1em; }"
 "</style>"
 "</head>";

// simple little HTML escaping for '&', '<', and '>'. might want a real one?
std::stringstream& HTTPDServer::JSONtoHTML(std::stringstream& ss, const nlohmann::json& json) const {
	std::stringstream inter;
	inter << std::setw(1) << json;
	const std::string& s = inter.str();
	for(size_t pos = 0 ; pos != s.size() ; ++pos) {
		switch(s[pos]) {
			case '&': ss << "&amp;"; break;
			case '<': ss << "&lt;"; break;
			case '>': ss << "&gt;"; break;
			default: ss << s[pos]; break;
		}
	}
	return ss;
}

std::stringstream& HTTPDServer::HTMLSysinfo(std::stringstream& ss) const {
	ss << "<h3>system</h3><table>";
	ss << "<tr><td>cxx</td><td>" << Catena::GetCompilerID() << "</td></tr>";
	ss << "<tr><td>libc</td><td>" << Catena::GetLibcID() << "</td></tr>";
	ss << "<tr><td>json</td><td>JSON for Modern C++ " <<
		NLOHMANN_JSON_VERSION_MAJOR << "." <<
		NLOHMANN_JSON_VERSION_MINOR << "." <<
		NLOHMANN_JSON_VERSION_PATCH << "</td>";
	ss << "<tr><td>crypto</td><td>" <<
		SSLeay_version(SSLEAY_VERSION) << "</td></tr>";
	ss << "</table>";
	return ss;
}

std::stringstream& HTTPDServer::HTMLMembers(std::stringstream& ss) const {
	ss << "<h3>consortium members</h3><div>";
	const auto& cmembers = chain.ConsortiumMembers();
	for(const auto& cm : cmembers){
		ss << "<a href=\"/showmember?member=" << cm.cmspec
		   << "\">" << cm.cmspec << "</a> (patients: " << cm.patients
		   << ")<pre>";
		JSONtoHTML(ss, cm.payload) << "</pre>";
	}
	ss << "</div>";
	return ss;
}

std::stringstream& HTTPDServer::HTMLChaininfo(std::stringstream& ss) const {
	ss << "<h3>chain</h3>";
	ss << "<table>";
	ss << "<tr><td>chain bytes</td><td>" << chain.Size() << "</td></tr>";
	ss << "<tr><td>blocks</td><td>" << chain.GetBlockCount() << "</td></tr>";
	ss << "<tr><td>transactions</td><td>" << chain.TXCount() << "</td></tr>";
	ss << "<tr><td>outstanding TXs</td><td>" << chain.OutstandingTXCount() << "</td></tr>";
	ss << "<tr><td>consortium members</td><td>" << chain.ConsortiumMemberCount() << "</td></tr>";
	ss << "<tr><td>lookup requests</td><td>" << chain.LookupRequestCount() << "</td></tr>";
	ss << "<tr><td>lookup authorizations</td><td>" << chain.LookupRequestCount(true) << "</td></tr>";
	ss << "<tr><td>external IDs</td><td>" << chain.ExternalLookupCount() << "</td></tr>";
	ss << "<tr><td>public keys</td><td>" << chain.PubkeyCount() << "</td></tr>";
	ss << "<tr><td>patients</td><td>" << chain.PatientCount() << "</td></tr>";
	ss << "<tr><td>status delegations</td><td>" << chain.StatusDelegationCount() << "</td></tr>";
	ss << "</table>";
	ss << "<h3>most recent block</h3>";
	BlockHTML(ss, chain.MostRecentBlockHash());
	return ss;
}

struct MHD_Response*
HTTPDServer::Favicon(struct MHD_Connection* conn __attribute__ ((unused))) const {
	auto resp = MHD_create_response_from_buffer(sizeof(FaviconBMP),
			(char*)FaviconBMP, MHD_RESPMEM_PERSISTENT);
        if(MHD_NO == MHD_add_response_header(resp, "Content-Type", "image/vnd.microsoft.icon")){
		MHD_destroy_response(resp);
		return nullptr;
	}
	return resp;
}

static std::string Hostname() {
	char hname[128] = "unknown";
	gethostname(hname, sizeof(hname));
	hname[sizeof(hname) - 1] = '\0'; // gethostname() doesn't truncate
	return hname;
}

struct MHD_Response*
HTTPDServer::Summary(struct MHD_Connection* conn __attribute__ ((unused))) const {
	std::stringstream ss;
	ss << htmlhdr;
	ss << "<body><h2>catena v" << VERSION << " on " << Hostname() << "</h2>";
	HTMLSysinfo(ss);
	HTMLChaininfo(ss);
	HTMLMembers(ss);
	ss << "<h3>other views</h3>";
	ss << "<div><span><a href=\"/show\">ledger</a></span>";
	ss << "<span><a href=\"/tstore\">truststore</a></span></div>";
	ss << "</body>";
	std::string s = ss.str();
	auto resp = MHD_create_response_from_buffer(s.size(),
			const_cast<char*>(s.c_str()), MHD_RESPMEM_MUST_COPY);
	if(resp){
		if(MHD_NO == MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html; charset=UTF-8")){
			MHD_destroy_response(resp);
			return nullptr;
		}
	}
	return resp;
}

struct MHD_Response*
HTTPDServer::Show(struct MHD_Connection* conn __attribute__ ((unused))) const {
	std::stringstream ss;
	ss << htmlhdr;
	ss << "<body><h2>catena v" << VERSION << " on " << Hostname() << "</h2>";
	auto j = InspectJSON(0, -1);
	ss << "<pre>";
	JSONtoHTML(ss, InspectJSON(0, -1)) << "</pre>";
	ss << "</body>";
	std::string s = ss.str();
	auto resp = MHD_create_response_from_buffer(s.size(),
			const_cast<char*>(s.c_str()), MHD_RESPMEM_MUST_COPY);
	if(resp){
		if(MHD_NO == MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html; charset=UTF-8")){
			MHD_destroy_response(resp);
			return nullptr;
		}
	}
	return resp;
}

struct MHD_Response*
HTTPDServer::TStore(struct MHD_Connection* conn __attribute__ ((unused))) const {
	std::stringstream ss;
	ss << htmlhdr;
	ss << "<body><h2>catena v" << VERSION << " on " << Hostname() << "</h2>";
	ss << "<pre>";
	chain.DumpTrustStore(ss);
	ss << "</pre></body>";
	std::string s = ss.str();
	auto resp = MHD_create_response_from_buffer(s.size(),
			const_cast<char*>(s.c_str()), MHD_RESPMEM_MUST_COPY);
	if(resp){
		if(MHD_NO == MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html; charset=UTF-8")){
			MHD_destroy_response(resp);
			return nullptr;
		}
	}
	return resp;
}

nlohmann::json HTTPDServer::InspectJSON(int start, int end) const {
	auto blks = chain.Inspect(start, end);
	std::vector<nlohmann::json> jblks;
	for(const auto& b : blks){
		std::vector<nlohmann::json> jtxs;
		for(const auto& tx : b.transactions){
			jtxs.emplace_back(tx.get()->JSONify());
		}
		nlohmann::json jblk;
		jblk["transactions"] = jtxs;
		jblk["version"] = b.bhdr.version;
		jblk["utc"] = b.bhdr.utc;
		jblk["bytes"] = b.bhdr.totlen;
		jblk["hash"] = Catena::hashOString(b.bhdr.hash);
		jblk["prev"] = Catena::hashOString(b.bhdr.prev);
		nlohmann::json(b.bhdr.totlen);
		jblks.emplace_back(jblk);
	}
	nlohmann::json ret(jblks);
	return ret;
}

std::stringstream& HTTPDServer::BlockHTML(std::stringstream& ss, const Catena::CatenaHash& hash) const {
	const auto blk = chain.Inspect(hash);
	ss << "<div id=\"block\">";
	ss << "hash: " << blk.bhdr.hash << "<br/>";
	ss << "prev: <a href=\"/showblock?hash=" << blk.bhdr.prev <<
		"\">" << blk.bhdr.prev << "</a><br/>";
	char timebuf[80];
	time_t utc = blk.bhdr.utc;
	ctime_r(&utc, timebuf); // FIXME there's c++ for this
	ss << "version: " << blk.bhdr.version << " bytes: " << blk.bhdr.totlen
		<< " offset: " << blk.offset << " transactions: " << blk.bhdr.txcount
		<< "<br/>" << timebuf << "<br/>" << std::endl;
	ss << "</div>";
	return ss;
}

struct MHD_Response*
HTTPDServer::ShowBlockHTML(struct MHD_Connection* conn) const {
	auto hashstr = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "hash");
	if(hashstr == nullptr){
		std::cerr << "missing required arguments" << std::endl;
		return nullptr;
	}
	MHD_Response* resp = nullptr;
	try{
		// FIXME handle genesis block manually
		auto hash = Catena::StrToCatenaHash(hashstr);
		std::stringstream ss;
		ss << htmlhdr;
		ss << "<body><h2>catena v" << VERSION << " on " << Hostname() << "</h2>";
		ss << "<h3>Block " << hash << "</h3>";
		BlockHTML(ss, hash);
		ss << "</body>";
		auto s = ss.str();
		resp = MHD_create_response_from_buffer(s.size(), const_cast<char*>(s.c_str()), MHD_RESPMEM_MUST_COPY);
	}catch(std::out_of_range& e){
		std::cerr << "bad argument (" << e.what() << ")" << std::endl;
		return nullptr;
	}catch(Catena::ConvertInputException& e){
		std::cerr << "bad argument (" << e.what() << ")" << std::endl;
		return nullptr;
	}
	return resp;
}

struct MHD_Response*
HTTPDServer::ShowMemberHTML(struct MHD_Connection* conn) const {
	auto cmspecstr = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "member");
	if(cmspecstr == nullptr){
		std::cerr << "missing required arguments" << std::endl;
		return nullptr;
	}
	MHD_Response* resp = nullptr;
	try{
		auto cmspec = Catena::TXSpec::StrToTXSpec(cmspecstr);
		std::stringstream ss;
		ss << htmlhdr;
		ss << "<body><h2>catena v" << VERSION << " on " << Hostname() << "</h2>";
		const auto& cmember = chain.ConsortiumMember(cmspec);
		const auto& patients = chain.ConsortiumPatients(cmspec);
		ss << "<h3>Consortium member " << cmspecstr << "</h3><pre>";
		JSONtoHTML(ss, cmember.payload) << "</pre>";
		ss << "<h3>Enrolled patients: " << patients.size() << "</h3>";
		for(const auto& p : patients){
			// FIXME be more general in the future, but for the
			// 2018-01 demo, just link to status 0
			ss << "<a href=\"/showpstatus?stype=0&patient=" << p.patspec << "\">" << p.patspec <<
				"</a><br/>";
		}
		ss << "</body>";
		auto s = ss.str();
		resp = MHD_create_response_from_buffer(s.size(), const_cast<char*>(s.c_str()), MHD_RESPMEM_MUST_COPY);
	}catch(Catena::InvalidTXSpecException& e){
		std::cerr << "bad txspec (" << e.what() << ")" << std::endl;
		return nullptr; // FIXME return error
	}catch(Catena::ConvertInputException& e){
		std::cerr << "bad argument (" << e.what() << ")" << std::endl;
		return nullptr;
	}
	return resp;
}

struct MHD_Response*
HTTPDServer::PstatusHTML(struct MHD_Connection* conn) const {
	auto patspecstr = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "patient");
	auto stypestr = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "stype");
	if(patspecstr == nullptr || stypestr == nullptr){
		std::cerr << "missing required arguments in /showpstatus" << std::endl;
		return nullptr;
	}
	MHD_Response* resp = nullptr;
	try{
		auto patspec = Catena::TXSpec::StrToTXSpec(patspecstr);
		auto stype = Catena::StrToLong(stypestr, 0, LONG_MAX);
		auto json = chain.PatientStatus(patspec, stype).dump();
		std::stringstream ss;
		ss << htmlhdr;
		ss << "<body><h2>catena v" << VERSION << " on " << Hostname() << "</h2>";
		ss << "<h3>Patient " << patspecstr << "</h3>";
		ss << "<span>" << stype << "</span><span>" << json << "</span>";
		ss << "</body>";
		auto s = ss.str();
		resp = MHD_create_response_from_buffer(s.size(), const_cast<char*>(s.c_str()), MHD_RESPMEM_MUST_COPY);
	}catch(Catena::InvalidTXSpecException& e){
		std::cerr << "bad txspec (" << e.what() << ")" << std::endl;
		return nullptr; // FIXME return error
	}catch(Catena::PatientStatusException& e){
		std::cerr << "invalid lookup (" << e.what() << ")" << std::endl;
		return nullptr;
	}catch(Catena::ConvertInputException& e){
		std::cerr << "bad argument (" << e.what() << ")" << std::endl;
		return nullptr;
	}
	if(resp){
		if(MHD_NO == MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html; charset=UTF-8")){
			MHD_destroy_response(resp);
			return nullptr;
		}
	}
	return resp;
}

struct MHD_Response*
HTTPDServer::PstatusJSON(struct MHD_Connection* conn) const {
	auto patspecstr = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "patient");
	auto stypestr = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "stype");
	if(patspecstr == nullptr || stypestr == nullptr){
		std::cerr << "missing required arguments in /pstatus" << std::endl;
		return nullptr;
	}
	MHD_Response* resp = nullptr;
	try{
		auto patspec = Catena::TXSpec::StrToTXSpec(patspecstr);
		auto stype = Catena::StrToLong(stypestr, 0, LONG_MAX);
		auto json = chain.PatientStatus(patspec, stype).dump();
		resp = MHD_create_response_from_buffer(json.size(), const_cast<char*>(json.c_str()), MHD_RESPMEM_MUST_COPY);
	}catch(Catena::InvalidTXSpecException& e){
		std::cerr << "bad txspec (" << e.what() << ")" << std::endl;
		return MHD_NO; // FIXME return error response
	}catch(Catena::ConvertInputException& e){
		std::cerr << "bad argument (" << e.what() << ")" << std::endl;
		return MHD_NO; // FIXME return error response
	}
	if(resp){
		if(MHD_NO == MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json")){
			MHD_destroy_response(resp);
			return nullptr;
		}
	}
	return resp;
}

struct MHD_Response*
HTTPDServer::Inspect(struct MHD_Connection* conn) const {
	auto sstart = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "begin");
	auto sstop = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "end");
	long start = 0;
	if(sstart){
		char* e = const_cast<char*>(sstart);
		start = strtol(sstart, &e, 10);
		if(start < 0 || start == LONG_MAX || *e){
			return nullptr; // FIXME return 400 with explanation
		}
	}
	long end = -1;
	if(sstop){
		char* e = const_cast<char*>(sstart);
		end = strtol(sstop, &e, 10);
		if(end < -1 || end == LONG_MAX || *e){ // allow explicit -1
			return nullptr;
		}
	}
	auto res = InspectJSON(start, end).dump();
	auto resp = MHD_create_response_from_buffer(res.size(), const_cast<char*>(res.c_str()), MHD_RESPMEM_MUST_COPY);
	if(resp){
		if(MHD_NO == MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json")){
			MHD_destroy_response(resp);
			return nullptr;
		}
	}
	return resp;
}

struct PostState {
	std::string response;
};

int HTTPDServer::MemberTXReq(struct PostState* ps, const char* upload) const {
	nlohmann::json json;
	try{
		json = nlohmann::json::parse(upload);
		auto pubkey = json.find("pubkey");
		auto payload = json.find("payload");
		auto regspec = json.find("regspec");
		if(pubkey == json.end() || payload == json.end() || regspec == json.end()){
			std::cerr << "missing necessary elements" << std::endl;
			return MHD_NO;
		}
		if(!(*pubkey).is_string()){
			std::cerr << "pubkey was not a string" << std::endl;
			return MHD_NO;
		}
		if(!(*payload).is_object()){
			std::cerr << "payload was invalid JSON" << std::endl;
			return MHD_NO;
		}
		if(!(*regspec).is_string()){
			std::cerr << "regspec was not a string" << std::endl;
			return MHD_NO;
		}
		auto kstr = (*pubkey).get<std::string>();
		auto rspecstr = (*regspec).get<std::string>();
		try{
			auto regkl = Catena::TXSpec::StrToTXSpec(rspecstr);
			chain.AddConsortiumMember(regkl,
					reinterpret_cast<const unsigned char*>(kstr.c_str()),
					kstr.size(), *payload);
		}catch(Catena::ConvertInputException& e){
			std::cerr << "bad argument (" << e.what() << ")" << std::endl;
			return MHD_NO; // FIXME return error response
		}catch(Catena::SigningException& e){
			std::cerr << "couldn't sign transaction (" << e.what() << ")" << std::endl;
			return MHD_NO; // FIXME return error response
		}
	}catch(nlohmann::detail::parse_error& e){
		std::cerr << "error extracting JSON from " << upload << std::endl;
		return MHD_NO;
	}
	ps->response = "{}";
	return MHD_YES;
}

int HTTPDServer::ExternalLookupTXReq(struct PostState* ps, const char* upload) const {
	nlohmann::json json;
	try{
		json = nlohmann::json::parse(upload);
		auto pubkey = json.find("pubkey");
		auto lookuptype = json.find("lookuptype");
		auto payload = json.find("payload");
		auto regspec = json.find("regspec");
		if(pubkey == json.end() || lookuptype == json.end() ||
				payload == json.end() || regspec == json.end()){
			std::cerr << "missing necessary elements from ExternalLookupTXRequest" << std::endl;
			return MHD_NO;
		}
		if(!(*pubkey).is_string()){
			std::cerr << "pubkey was not a string" << std::endl;
			return MHD_NO;
		}
		if(!(*lookuptype).is_number()){
			std::cerr << "lookuptype was not a number" << std::endl;
			return MHD_NO;
		}
		if(!(*payload).is_string()){
			std::cerr << "payload was not a string" << std::endl;
			return MHD_NO;
		}
		if(!(*regspec).is_string()){
			std::cerr << "regspec was not a string" << std::endl;
			return MHD_NO;
		}
		auto kstr = (*pubkey).get<std::string>();
		auto pstr = (*payload).get<std::string>();
		auto rspecstr = (*regspec).get<std::string>();
		auto ltype = (*lookuptype).get<int>();
		try{
			auto regkl = Catena::TXSpec::StrToTXSpec(rspecstr);
			chain.AddExternalLookup(regkl,
					reinterpret_cast<const unsigned char*>(kstr.c_str()),
					kstr.size(), pstr, ltype);
		}catch(Catena::ConvertInputException& e){
			std::cerr << "bad argument (" << e.what() << ")" << std::endl;
			return MHD_NO; // FIXME return error response
		}catch(Catena::SigningException& e){
			std::cerr << "couldn't sign transaction (" << e.what() << ")" << std::endl;
			return MHD_NO; // FIXME return error response
		}
	}catch(nlohmann::detail::parse_error& e){
		std::cerr << "error extracting JSON from " << upload << std::endl;
		return MHD_NO;
	}
	ps->response = "{}";
	return MHD_YES;
}

int HTTPDServer::PatientTXReq(struct PostState* ps, const char* upload) const {
	nlohmann::json json;
	Catena::SymmetricKey symkey;
	try{
		json = nlohmann::json::parse(upload);
		auto pubkey = json.find("pubkey");
		auto payload = json.find("payload");
		auto regspec = json.find("regspec");
		if(pubkey == json.end() || payload == json.end() || regspec == json.end()){
			std::cerr << "missing necessary elements" << std::endl;
			return MHD_NO;
		}
		if(!(*pubkey).is_string()){
			std::cerr << "pubkey was not a string" << std::endl;
			return MHD_NO;
		}
		if(!(*payload).is_object()){
			std::cerr << "payload was invalid JSON" << std::endl;
			return MHD_NO;
		}
		if(!(*regspec).is_string()){
			std::cerr << "regspec was not a string" << std::endl;
			return MHD_NO;
		}
		auto kstr = (*pubkey).get<std::string>();
		auto rspecstr = (*regspec).get<std::string>();
		symkey = Catena::Keypair::CreateSymmetricKey();
		try{
			auto regkl = Catena::TXSpec::StrToTXSpec(rspecstr);
			chain.AddPatient(regkl,
					reinterpret_cast<const unsigned char*>(kstr.c_str()),
					kstr.size(), symkey, *payload);
		}catch(Catena::ConvertInputException& e){
			std::cerr << "bad argument (" << e.what() << ")" << std::endl;
			return MHD_NO; // FIXME return error response
		}catch(Catena::SigningException& e){
			std::cerr << "couldn't sign transaction (" << e.what() << ")" << std::endl;
			return MHD_NO; // FIXME return error response
		}
	}catch(Catena::KeypairException& e){
		std::cerr << e.what() << std::endl;
		return MHD_NO;
	}catch(nlohmann::detail::parse_error& e){
		std::cerr << "error extracting JSON from " << upload << std::endl;
		return MHD_NO;
	}
	nlohmann::json j;
	std::stringstream symstr;
	Catena::HexOutput(symstr, symkey.data(), symkey.size());
	j["symkey"] = symstr.str();
	ps->response = j.dump();
	return MHD_YES;
}

int HTTPDServer::LookupAuthReqTXReq(struct PostState* ps, const char* upload) const {
	(void)ps;
	(void)upload;
	return MHD_NO;
}

int HTTPDServer::LookupAuthTXReq(struct PostState* ps, const char* upload) const {
	try{
		auto json = nlohmann::json::parse(upload);
		auto refspec = json.find("refspec");
		auto patspec = json.find("patspec");
		auto symspec = json.find("symkey");
		if(refspec == json.end() || patspec == json.end()){
			std::cerr << "missing necessary elements" << std::endl;
			return MHD_NO;
		}
		if(!(*refspec).is_string()){
			std::cerr << "refspec was not a string" << std::endl;
			return MHD_NO;
		}
		if(!(*patspec).is_string()){
			std::cerr << "patspec was not a string" << std::endl;
			return MHD_NO;
		}
		if(!(*symspec).is_string()){
			std::cerr << "symkey was not a string" << std::endl;
			return MHD_NO;
		}
		auto patspecstr = (*patspec).get<std::string>();
		auto refspecstr = (*refspec).get<std::string>();
		try{
			Catena::SymmetricKey symkey;
			Catena::StrToBlob((*symspec).get<std::string>(), symkey);
			auto patkl = Catena::TXSpec::StrToTXSpec(patspecstr);
			auto refkl = Catena::TXSpec::StrToTXSpec(refspecstr);
			chain.AddLookupAuth(refkl, patkl, symkey);
		}catch(Catena::ConvertInputException& e){
			std::cerr << "bad argument (" << e.what() << ")" << std::endl;
			return MHD_NO; // FIXME return error response
		}catch(Catena::SigningException& e){
			std::cerr << "couldn't sign transaction (" << e.what() << ")" << std::endl;
			return MHD_NO; // FIXME return error response
		}
	}catch(nlohmann::detail::parse_error& e){
		std::cerr << "error extracting JSON from " << upload << std::endl;
		return MHD_NO;
	}
	ps->response = "{}";
	return MHD_NO;
}

int HTTPDServer::PatientDelegationTXReq(struct PostState* ps, const char* upload) const {
	(void)ps;
	(void)upload;
	return MHD_NO;
}

int HTTPDServer::PatientStatusTXReq(struct PostState* ps, const char* upload) const {
	try{
		auto json = nlohmann::json::parse(upload);
		auto payload = json.find("payload");
		auto psdspec = json.find("psdspec");
		if(payload == json.end() || psdspec == json.end()){
			std::cerr << "missing necessary elements from NewPatientStatusTX" << std::endl;
			return MHD_NO;
		}
		if(!(*payload).is_object()){
			std::cerr << "payload was invalid JSON" << std::endl;
			return MHD_NO;
		}
		if(!(*psdspec).is_string()){
			std::cerr << "psdspec was not a string" << std::endl;
			return MHD_NO;
		}
		auto psdspecstr = (*psdspec).get<std::string>();
		try{
			auto psdkl = Catena::TXSpec::StrToTXSpec(psdspecstr);
			chain.AddPatientStatus(psdkl, *payload);
		}catch(Catena::ConvertInputException& e){
			std::cerr << "bad argument (" << e.what() << ")" << std::endl;
			return MHD_NO; // FIXME return error response
		}catch(Catena::SigningException& e){
			std::cerr << "couldn't sign transaction (" << e.what() << ")" << std::endl;
			return MHD_NO; // FIXME return error response
		}
	}catch(nlohmann::detail::parse_error& e){
		std::cerr << "error extracting JSON from " << upload << std::endl;
		return MHD_NO;
	}
	ps->response = "{}";
	return MHD_NO;
}

int HTTPDServer::HandlePost(struct MHD_Connection* conn, const char* url,
				const char* upload_data, size_t* upload_len,
				void** conn_cls){
	const struct {
		const char *uri;
		int (HTTPDServer::*fxn)(struct PostState*, const char*) const;
	} cmds[] = {
		{ "/member", &HTTPDServer::MemberTXReq, },
		{ "/exlookup", &HTTPDServer::ExternalLookupTXReq, },
		{ "/lauthreq", &HTTPDServer::LookupAuthReqTXReq, },
		{ "/lauth", &HTTPDServer::LookupAuthTXReq, },
		{ "/patient", &HTTPDServer::PatientTXReq, },
		{ "/delpstatus", &HTTPDServer::PatientDelegationTXReq, },
		{ "/pstatus", &HTTPDServer::PatientStatusTXReq, },
		{ nullptr, nullptr },
	},* cmd;
	int retcode = MHD_HTTP_OK; // FIXME callbacks need be able to set retcode
	struct MHD_Response* resp = nullptr;
	for(cmd = cmds ; cmd->uri ; ++cmd){
		if(strcmp(cmd->uri, url) == 0){
			break;
		}
	}
	if(!cmd->uri){
		retcode = MHD_HTTP_NOT_FOUND;
		char buf[1] = {0};
		std::cerr << "no POST service provided at " << url << ", returning 404" << std::endl;
		resp = MHD_create_response_from_buffer(0, buf, MHD_RESPMEM_MUST_COPY);
		auto ret = MHD_queue_response(conn, retcode, resp);
		MHD_destroy_response(resp);
		return ret;
	}
	auto mpp = static_cast<struct PostState*>(*conn_cls);
	if(mpp == nullptr){
		*conn_cls = new PostState;
		return MHD_YES;
	}
	if(upload_len && *upload_len){
		auto ret = (this->*(cmd->fxn))(mpp, upload_data);
		if(ret == MHD_NO){
			std::cerr << "error handling " << url << std::endl;
		}
		*upload_len = 0;
		return ret;
	}
	resp = MHD_create_response_from_buffer(mpp->response.size(),
			const_cast<char*>(mpp->response.c_str()),
			MHD_RESPMEM_MUST_COPY);
	if(resp == nullptr){
		return MHD_NO;
	}
	auto ret = MHD_queue_response(conn, retcode, resp);
	MHD_destroy_response(resp);
	return ret;
}

// FIXME all the places where we just plainly return MHD_NO, we ought try
//  queueing an actual MHD_HTTP_INTERNAL_SERVER_ERROR.
// cls is this pointer
int HTTPDServer::Handler(void* cls, struct MHD_Connection* conn, const char* url,
				const char* method, const char* version,
				const char* upload_data, size_t* upload_len,
				void** conn_cls){
	HTTPDServer* this_ = reinterpret_cast<HTTPDServer*>(cls);
	(void)version;
	(void)conn_cls;
	if(strcmp(method, MHD_HTTP_METHOD_POST) == 0){
		return this_->HandlePost(conn, url, upload_data, upload_len, conn_cls);
	}
	if(strcmp(method, MHD_HTTP_METHOD_GET)){
		return MHD_NO;
	}
	int retcode = MHD_HTTP_OK; // FIXME callbacks need be able to set retcode
	const struct {
		const char *uri;
		struct MHD_Response* (HTTPDServer::*fxn)(struct MHD_Connection*) const;
	} cmds[] = {
		{ "/", &HTTPDServer::Summary, },
		{ "/favicon.ico", &HTTPDServer::Favicon, },
		{ "/show", &HTTPDServer::Show, },
		{ "/tstore", &HTTPDServer::TStore, },
		{ "/inspect", &HTTPDServer::Inspect, },
		{ "/pstatus", &HTTPDServer::PstatusJSON, },
		{ "/showpstatus", &HTTPDServer::PstatusHTML, },
		{ "/showmember", &HTTPDServer::ShowMemberHTML, },
		{ "/showblock", &HTTPDServer::ShowBlockHTML, },
		{ nullptr, nullptr },
	},* cmd;
	struct MHD_Response* resp = nullptr;
	for(cmd = cmds ; cmd->uri ; ++cmd){
		if(strcmp(cmd->uri, url) == 0){
			resp = (*reinterpret_cast<HTTPDServer*>(cls).*(cmd->fxn))(conn);
			break;
		}
	}
	if(!cmd->uri){
		retcode = MHD_HTTP_NOT_FOUND;
		char buf[1] = {0};
		resp = MHD_create_response_from_buffer(0, buf, MHD_RESPMEM_MUST_COPY);
		std::cerr << "no GET service provided at " << url << ", returning 404" << std::endl;
	}
	if(resp == nullptr){
		std::cerr << "couldn't create HTTP response, sending error" << std::endl;
		char buf[1] = {0};
		resp = MHD_create_response_from_buffer(0, buf, MHD_RESPMEM_MUST_COPY);
		if(resp == nullptr){
			return MHD_NO;
		}
		retcode = MHD_HTTP_INTERNAL_SERVER_ERROR;
	}
	auto ret = MHD_queue_response(conn, retcode, resp);
	MHD_destroy_response(resp);
	if(ret == MHD_NO){
		std::cerr << "couldn't queue HTTP response" << std::endl;
		return MHD_NO;
	}
	return ret;
}

HTTPDServer::HTTPDServer(Catena::Chain& chain, unsigned port) :
  chain(chain) {
	const unsigned flags = MHD_USE_SELECT_INTERNALLY | MHD_USE_EPOLL_LINUX_ONLY;
	mhd = MHD_start_daemon(flags, port, NULL, NULL, Handler, this,
				MHD_OPTION_END);
	if(mhd == nullptr){
		throw HTTPException();
	}
}

}
