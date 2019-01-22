#ifndef CATENA_CATENA_READLINE
#define CATENA_CATENA_READLINE

#include <libcatena/chain.h>

namespace CatenaAgent {

class ReadlineUI {
public:
ReadlineUI() = delete;
ReadlineUI(Catena::Chain&);
~ReadlineUI() = default;
void InputLoop();

private:
bool cancelled;
Catena::Chain& chain;
template <typename Iterator> int Quit(const Iterator start, const Iterator end);
template <typename Iterator> int Summary(const Iterator start, const Iterator end);
template <typename Iterator> int Show(const Iterator start, const Iterator end);
template <typename Iterator> int Inspect(const Iterator start, const Iterator end);
template <typename Iterator> int Outstanding(const Iterator start, const Iterator end);
template <typename Iterator> int FlushOutstanding(const Iterator start, const Iterator end);
template <typename Iterator> int CommitOutstanding(const Iterator start, const Iterator end);
template <typename Iterator> int TStore(const Iterator start, const Iterator end);
template <typename Iterator> int NewMember(const Iterator start, const Iterator end);
template <typename Iterator> int GetMembers(const Iterator start, const Iterator end);
template <typename Iterator> int NewUser(const Iterator start, const Iterator end);
template <typename Iterator> int NewExternalLookup(const Iterator start, const Iterator end);
template <typename Iterator> int NewLookupAuth(const Iterator start, const Iterator end);
template <typename Iterator> int NewLookupAuthReq(const Iterator start, const Iterator end);
template <typename Iterator> int NewUserStatus(const Iterator start, const Iterator end);
template <typename Iterator> int GetUserStatus(const Iterator start, const Iterator end);
template <typename Iterator> int NewUserStatusDelegation(const Iterator start, const Iterator end);
template <typename Iterator> int Peers(const Iterator start, const Iterator end);
template <typename Iterator> int Conns(const Iterator start, const Iterator end);

std::ostream& MemberSummary(std::ostream& s, const Catena::ConsortiumMemberSummary& cm) const;
};

}

#endif
