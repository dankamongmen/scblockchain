#ifndef CATENA_CATENA_READLINE
#define CATENA_CATENA_READLINE

#include <libcatena/chain.h>

namespace Catena {

class ReadlineUI {
public:
ReadlineUI();
~ReadlineUI() = default;
void InputLoop(Catena::Chain&);

private:
bool cancelled;
void HandleQuit(Catena::Chain&);
void HandleShow(Catena::Chain&);
void HandleTStore(Catena::Chain&);
void HandleNewMember(Catena::Chain&);
void HandleNoOp(Catena::Chain&);
};

}

#endif
