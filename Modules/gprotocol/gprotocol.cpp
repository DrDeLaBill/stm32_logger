/* Copyright © 2024 Georgy E. All rights reserved. */

#include "gprotocol.h"


#if GPROTOCOL_BEDUG_GET || GPROTOCOL_BEDUG_SET
std::unordered_map<uint32_t, std::string> gprotocol::bedug_table;
#else
std::unordered_set<uint32_t> gprotocol::hashes;
#endif
