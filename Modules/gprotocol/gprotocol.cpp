/* Copyright © 2024 Georgy E. All rights reserved. */

#include "gprotocol.h"


#if GPROTOCOL_BEDUG
std::unordered_map<uint32_t, std::string> gprotocol::debug_table;
#else
std::unordered_set<uint32_t> gprotocol::hashes;
#endif
