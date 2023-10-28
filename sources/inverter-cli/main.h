#pragma once

#include <atomic>

extern std::atomic_bool ups_data_changed;
extern std::atomic_bool ups_status_changed;
extern std::atomic_bool ups_qpiws_changed;
extern std::atomic_bool ups_qmod_changed;
extern std::atomic_bool ups_qpiri_changed;
extern std::atomic_bool ups_qpigs_changed;
