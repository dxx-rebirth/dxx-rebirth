/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/


#include <cctype>
#include <cstdint>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hash.h"

namespace dcx {

bool hashtable::compare_t::operator()(const char *l, const char *r) const
{
	for (;; ++l, ++r)
	{
		uint_fast32_t ll = tolower(static_cast<unsigned>(*l)), lr = tolower(static_cast<unsigned>(*r));
		if (ll < lr)
			return true;
		if (lr < ll || !ll)
			return false;
	}
}

int hashtable_search(hashtable *ht, const char *key)
{
	auto i = ht->m.find(key);
	if (i != ht->m.end())
		return i->second;
	return -1;
}

void hashtable_insert(hashtable *ht, const char *key, int value)
{
	ht->m.insert(std::make_pair(key, value));
}

}
