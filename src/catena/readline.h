#ifndef CATENA_CATENA_READLINE
#define CATENA_CATENA_READLINE

namespace Catena {

class ReadlineUI {
public:
ReadlineUI();
~ReadlineUI() = default;
void InputLoop();

private:
bool cancelled;
void HandleQuit();
};

}

#endif
