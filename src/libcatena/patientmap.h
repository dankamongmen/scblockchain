#ifndef CATENA_LIBCATENA_PATIENTMAP
#define CATENA_LIBCATENA_PATIENTMAP

// FIXME something of a misnomer -- maybe "ledger map"? Metadata for outstanding
// elements in the ledger. Contains fast lookup for essentially "everything
// which hasn't been obsoleted by a later transaction."

#include <set>
#include <map>
#include <utility>
#include <iostream>
#include <nlohmann/json.hpp>
#include <libcatena/hash.h>

namespace Catena {

using TXSpec = std::pair<CatenaHash, unsigned>;

inline std::ostream& operator<<(std::ostream& s, const TXSpec& t){
	s << t.first << "." << t.second;
	return s;
}

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

struct PatientSummary {
PatientSummary(const TXSpec& txspec) :
	patspec(txspec) {}

TXSpec patspec;
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

struct ConsortiumMemberSummary {
ConsortiumMemberSummary(const TXSpec& txspec, int pcount, const nlohmann::json& pload) :
	cmspec(txspec),
	patients(pcount),
	payload(pload) {}

TXSpec cmspec;
int patients;
nlohmann::json payload;
};

class ConsortiumMember {
public:

ConsortiumMember(const nlohmann::json& json) : payload(json) {}

int PatientCount() const {
	return patients.size();
}

void AddPatient(const TXSpec& p) {
	patients.push_back(p);
}

std::vector<PatientSummary> Patients() const {
	std::vector<PatientSummary> ret;
	for(auto it = std::begin(patients) ; it != std::end(patients); ++it){
		ret.emplace_back(*it);
	}
	return ret;
}

nlohmann::json Payload() const {
	return payload;
}

private:
std::vector<TXSpec> patients;
nlohmann::json payload;
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

int ConsortiumMemberCount() const {
	return cmembers.size();
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

void AddPatient(const TXSpec& patspec, const TXSpec& cmspec) {
	auto it = cmembers.find(cmspec);
	if(it == cmembers.end()){
		throw InvalidTXSpecException("unknown consortium member");
	}
	patients.emplace(patspec, Patient{});
	it->second.AddPatient(patspec);
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

void AddConsortiumMember(const TXSpec& cmspec, const nlohmann::json& json) {
	cmembers.emplace(cmspec, Catena::ConsortiumMember{json});
}

std::vector<ConsortiumMemberSummary> ConsortiumMembers() const {
	std::vector<ConsortiumMemberSummary> ret;
	for(auto it = std::begin(cmembers) ; it != std::end(cmembers); ++it){
		ret.emplace_back(ConsortiumMemberSummary(it->first,
				it->second.PatientCount(), it->second.Payload()));
	}
	return ret;
}

ConsortiumMemberSummary ConsortiumMember(const TXSpec& cmspec) const {
	auto it = cmembers.find(cmspec);
	if(it == cmembers.end()){
		throw InvalidTXSpecException("unknown consortium member");
	}
	return ConsortiumMemberSummary(it->first, it->second.PatientCount(),
					it->second.Payload());
}

std::vector<PatientSummary> ConsortiumPatients(const TXSpec& cmspec) const {
	auto it = cmembers.find(cmspec);
	if(it == cmembers.end()){
		throw InvalidTXSpecException("unknown consortium member");
	}
	return it->second.Patients();
}

private:
// We're using maps rather than unordered maps, but probably don't need to.
// We could just hash TXSpecs, as we already do in TrustStore. With that said,
// I see claims that std::map is usually more memory-efficient than
// unordered_map. Both guarantee validity of pointers to container data.
std::map<TXSpec, LookupRequest> lookupreqs;
std::map<TXSpec, StatusDelegation> delegations;
std::map<TXSpec, Patient> patients;
std::map<TXSpec, Catena::ConsortiumMember> cmembers;
std::set<TXSpec> extlookups;
};

}

#endif
