#if !defined(SHIPPING)
#include "..\Content\ContentLoader.h"
#include "..\Components\Script.h"
#include <thread>

bool engine_initialize()
{
    bool result{ nidhog::content::load_game() };
    return result;
}

void engine_update()
{
    nidhog::script::update(10.f);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void engine_shutdown()
{
    nidhog::content::unload_game();
}
#endif // !defined(SHIPPING)