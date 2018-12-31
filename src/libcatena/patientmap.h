#ifndef CATENA_LIBCATENA_PATIENTMAP
#define CATENA_LIBCATENA_PATIENTMAP

#include <set>
#include <map>
#include <iostream>
#include <libcatena/hash.h>

namespace Catena {

using TXSpec = std::pair<CatenaHash, unsigned>;

class InvalidTXSpecException : public std::runtime_error {
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

class StatusDelegation {
public:
StatusDelegation() :
 statustype(0) {}

StatusDelegation(const TXSpec& cmspec, const TXSpec& patspec) :
	statustype(0),
	cmspec(cmspec),
	patspec(patspec) {}

int StatusType() const {
	return statustype;
}

TXSpec PatSpec(void) const {
	return patspec;
}

TXSpec CMSpec(void) const {
	return cmspec;
}

private:
int statustype;
TXSpec cmspec; // ConsortiumMemberTX
TXSpec patspec; // PatientTX
};

class PatientMap {
public:

// Total number of LookupAuthReq transactions in the ledger
int LookupRequestCount() const {
	return lookupreqs.size();
}

// Number of LookupAuthReqs that (authorized==true) have a corresponding
// LookupAuth, or (authorized==false) do not.
int LookupRequestCount(bool authorized) const {
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

int ExternalLookupCount() const {
	return extlookups.size();
}

int StatusDelegationCount() const {
	return delegations.size();
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

StatusDelegation& LookupDelegation(const TXSpec& psd) {
	const auto& it = delegations.find(psd);
	if(it == delegations.end()){
		throw InvalidTXSpecException("unknown status delegation");
	}
	return it->second;
}

void AddDelegation(const TXSpec& psdspec, const TXSpec& cmspec, const TXSpec& patspec) {
	delegations.emplace(psdspec, StatusDelegation{cmspec, patspec});
}

private:
std::map<TXSpec, LookupRequest> lookupreqs;
std::map<TXSpec, StatusDelegation> delegations;
std::set<TXSpec> extlookups;
};

}

#endif
