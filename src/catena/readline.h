#ifndef CATENA_CATENA_READLINE
#define CATENA_CATENA_READLINE

#include <libcatena/chain.h>

namespace Catena {

class ReadlineUI {
public:
ReadlineUI() = delete;
ReadlineUI(Catena::Chain&);
~ReadlineUI() = default;
void InputLoop();

private:
bool cancelled;
Catena::Chain& chain;
template <typename Iterator> int HandleQuit(Iterator start, Iterator end);
template <typename Iterator> int HandleShow(Iterator start, Iterator end);
template <typename Iterator> int HandleOutstanding(Iterator start, Iterator end);
template <typename Iterator> int HandleTStore(Iterator start, Iterator end);
template <typename Iterator> int HandleNewMember(Iterator start, Iterator end);
template <typename Iterator> int HandleNoOp(Iterator start, Iterator end);
std::vector<std::string> SplitInput(const char* line) const;
};

}

#endif
