#ifndef CATENA_LIBCATENA_LEDGERMAP
#define CATENA_LIBCATENA_LEDGERMAP

// Metadata for outstanding elements in the ledger. Contains fast lookup for
// essentially "everything which hasn't been obsoleted by a later transaction."

#include <set>
#include <map>
#include <utility>
#include <nlohmann/json.hpp>
#include <libcatena/hash.h>

namespace Catena {

class LookupRequest {
public:
LookupRequest() = delete;

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

StatusDelegation(int stype, const TXSpec& cmspec, const TXSpec& uspec) :
	statustype(stype),
	cmspec(cmspec),
	uspec(uspec) {}

int StatusType() const {
	return statustype;
}

TXSpec USpec(void) const {
	return uspec;
}

TXSpec CMSpec(void) const {
	return cmspec;
}

private:
int statustype;
TXSpec cmspec; // ConsortiumMemberTX
TXSpec uspec; // UserTX
};

struct UserSummary {
UserSummary(const TXSpec& txspec) :
	uspec(txspec) {}

TXSpec uspec;
};

class User {
public:
nlohmann::json Status(int stype) const {
	const auto& it = statuses.find(stype);
	if(it == statuses.end()){
		throw UserStatusException("user had no such status");
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
	users(pcount),
	payload(pload) {}

TXSpec cmspec;
int users;
nlohmann::json payload;
};

class ConsortiumMember {
public:

ConsortiumMember(const nlohmann::json& json) : payload(json) {}

int UserCount() const {
	return users.size();
}

void AddUser(const TXSpec& u) {
	users.push_back(u);
}

std::vector<UserSummary> Users() const {
	std::vector<UserSummary> ret;
	for(auto it = std::begin(users) ; it != std::end(users); ++it){
		ret.emplace_back(*it);
	}
	return ret;
}

nlohmann::json Payload() const {
	return payload;
}

private:
std::vector<TXSpec> users;
nlohmann::json payload;
};

class LedgerMap {
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

int UserCount() const {
	return users.size();
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

void AddDelegation(const TXSpec& usdspec, const TXSpec& cmspec,
			const TXSpec& uspec, int stype) {
	delegations.emplace(usdspec, StatusDelegation{stype, cmspec, uspec});
}

void AddUser(const TXSpec& uspec, const TXSpec& cmspec) {
	auto it = cmembers.find(cmspec);
	if(it == cmembers.end()){
		throw InvalidTXSpecException("unknown consortium member");
	}
	users.emplace(uspec, User{});
	it->second.AddUser(uspec);
}

const User& LookupUser(const TXSpec& u) const {
	const auto& it = users.find(u);
	if(it == users.end()){
		throw InvalidTXSpecException("unknown user");
	}
	return it->second;
}

User& LookupUser(const TXSpec& u) {
	const auto& it = users.find(u);
	if(it == users.end()){
		throw InvalidTXSpecException("unknown user");
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
				it->second.UserCount(), it->second.Payload()));
	}
	return ret;
}

ConsortiumMemberSummary ConsortiumMember(const TXSpec& cmspec) const {
	auto it = cmembers.find(cmspec);
	if(it == cmembers.end()){
		throw InvalidTXSpecException("unknown consortium member");
	}
	return ConsortiumMemberSummary(it->first, it->second.UserCount(),
					it->second.Payload());
}

std::vector<UserSummary> ConsortiumUsers(const TXSpec& cmspec) const {
	auto it = cmembers.find(cmspec);
	if(it == cmembers.end()){
		throw InvalidTXSpecException("unknown consortium member");
	}
	return it->second.Users();
}

private:
// We're using maps rather than unordered maps, but probably don't need to.
// We could just hash TXSpecs, as we already do in TrustStore. With that said,
// I see claims that std::map is usually more memory-efficient than
// unordered_map. Both guarantee validity of pointers to container data.
std::map<TXSpec, LookupRequest> lookupreqs;
std::map<TXSpec, StatusDelegation> delegations;
std::map<TXSpec, User> users;
std::map<TXSpec, Catena::ConsortiumMember> cmembers;
std::set<TXSpec> extlookups;
};

}

namespace std {
// Implement std::hash<TXSpec> so it can be used as key in e.g. unordered_maps.
// Since TX hashes are already "random", use them as base (using the least
// significant bytes, since some mining schemes require leading digits),
// adding the (highly non-random, weighted towards low numbers) TX idx so that
// one block's transactions all map to different buckers.
template <>
struct hash<Catena::TXSpec>{
template <class T1, class T2>
size_t operator()(const std::pair<T1, T2>& k) const {
	// FIXME verify sizeof(sha) < KEYLEN (at compile time) (bit it will)
	size_t sha;
	memcpy(&sha, k.first.data() + k.first.size() - sizeof(sha), sizeof(sha));
	return sha + k.second;
}
};
}

#endif
