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
template <typename Iterator> int NewPatient(const Iterator start, const Iterator end);
template <typename Iterator> int NewExternalLookup(const Iterator start, const Iterator end);
template <typename Iterator> int NewLookupAuth(const Iterator start, const Iterator end);
template <typename Iterator> int NewLookupAuthReq(const Iterator start, const Iterator end);
template <typename Iterator> int NewNoOp(const Iterator start, const Iterator end);
template <typename Iterator> int NewPatientStatus(const Iterator start, const Iterator end);
template <typename Iterator> int GetPatientStatus(const Iterator start, const Iterator end);
template <typename Iterator> int NewPatientStatusDelegation(const Iterator start, const Iterator end);
};

}

#endif
