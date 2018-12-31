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

class PatientStatusException : public std::runtime_error {
public:
PatientStatusException() : std::runtime_error("bad patient status"){}
PatientStatusException(const std::string& s) : std::runtime_error(s){}
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
StatusDelegation() = delete;

StatusDelegation(int stype, const TXSpec& cmspec, const TXSpec& patspec) :
	statustype(stype),
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

class Patient {
public:
nlohmann::json Status(int stype) const {
	const auto& it = statuses.find(stype);
	if(it == statuses.end()){
		throw PatientStatusException("patient had no such status");
	}
	return it->second;
}

void SetStatus(int stype, const nlohmann::json& status) {
	const auto& it = statuses.find(stype);
	if(it == statuses.end()){
		statuses.insert({stype, status});
	}else{
		it->second = status;
	}
}

private:
std::map<int, nlohmann::json> statuses;
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

int PatientCount() const {
	return patients.size();
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

void AddDelegation(const TXSpec& psdspec, const TXSpec& cmspec,
			const TXSpec& patspec, int stype) {
	delegations.emplace(psdspec, StatusDelegation{stype, cmspec, patspec});
}

void AddPatient(const TXSpec& patspec) {
	patients.emplace(patspec, Patient{});
}

const Patient& LookupPatient(const TXSpec& pat) const {
	const auto& it = patients.find(pat);
	if(it == patients.end()){
		throw InvalidTXSpecException("unknown patient");
	}
	return it->second;
}

Patient& LookupPatient(const TXSpec& pat) {
	const auto& it = patients.find(pat);
	if(it == patients.end()){
		throw InvalidTXSpecException("unknown patient");
	}
	return it->second;
}

private:
std::map<TXSpec, LookupRequest> lookupreqs;
std::map<TXSpec, StatusDelegation> delegations;
std::map<TXSpec, Patient> patients;
std::set<TXSpec> extlookups;
};

}

#endif
