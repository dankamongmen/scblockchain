#ifndef CATENA_LIBCATENA_PATIENTMAP
#define CATENA_LIBCATENA_PATIENTMAP

#include <set>
#include <map>
#include <iostream>
#include <libcatena/hash.h>

namespace Catena {

using TXSpec = std::pair<CatenaHash, unsigned>;

class InvalidTXSpecException : std::runtime_error {
public:
InvalidTXSpecException() : std::runtime_error("bad transaction spec"){}
InvalidTXSpecException(const std::string& s) : std::runtime_error(s){}
};

class LookupRequest {
public:
LookupRequest() :
 authorized(false) {}

LookupRequest(const TXSpec& elspec, const TXSpec& cmspec) :
	authorized(false),
	elspec(elspec),
	cmspec(cmspec) {}

bool IsAuthorized() const {
	return authorized;
}

void Authorize() {
	authorized = true; // FIXME check for existing auth?
}

TXSpec ELSpec(void) const {
	return elspec;
}

TXSpec CMSpec(void) const {
	return cmspec;
}

private:
bool authorized; // Have we seen a LookupAuthTX?
TXSpec elspec; // ExternalLookupTX
TXSpec cmspec; // ConsortiumMemberTX
};

class PatientMap {
public:

// Total number of LookupAuthReq transactions in the ledger
unsigned LookupRequestCount() const {
	return lookupreqs.size();
}

// Number of LookupAuthReqs that (authorized==true) have a corresponding
// LookupAuth, or (authorized==false) do not.
unsigned LookupRequestCount(bool authorized) const {
	auto authed = std::accumulate(lookupreqs.begin(), lookupreqs.end(), 0,
			[](int total, const decltype(lookupreqs)::value_type& lr){
				return total + lr.second.IsAuthorized();
			});
	if(authorized){
		return authed;
	}else{
		return LookupRequestCount() - authed;
	}
}

LookupRequest& LookupReq(const TXSpec& lar) {
	const auto& it = lookupreqs.find(lar);
	if(it == lookupreqs.end()){
		throw InvalidTXSpecException("unknown lookup auth req");
	}
	return it->second;
}

void AddLookupReq(const TXSpec& larspec, const TXSpec& elspec, const TXSpec& cmspec) {
	lookupreqs.emplace(larspec, LookupRequest{elspec, cmspec});
}

void AddExtLookup(const TXSpec& elspec) {
	extlookups.insert(elspec);
}

private:
std::map<TXSpec, LookupRequest> lookupreqs;
std::set<TXSpec> extlookups;
};

}

#endif
