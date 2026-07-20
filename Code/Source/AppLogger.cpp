#include <nierika_dsp/nierika_dsp.h>

nutils::Logger& nutils::AppLogger::get()
{
    return Logger::getInstance("Nierika");
}
