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
template <typename Iterator> int Quit(Iterator start, Iterator end);
template <typename Iterator> int Show(Iterator start, Iterator end);
template <typename Iterator> int Outstanding(Iterator start, Iterator end);
template <typename Iterator> int FlushOutstanding(Iterator start, Iterator end);
template <typename Iterator> int TStore(Iterator start, Iterator end);
template <typename Iterator> int NewMember(Iterator start, Iterator end);
template <typename Iterator> int NoOp(Iterator start, Iterator end);
std::vector<std::string> SplitInput(const char* line) const;
};

}

#endif
