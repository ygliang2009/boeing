#define CATCH_CONFIG_MAIN
#include "../catch/catch.hpp"
#include "../message/Header.h"
#include <cstdio>

TEST_CASE( "boeHeader.headerEncode" ) {
	BoeHeader boeHeader;
	boeHeader.version = 2;
	boeHeader.mid = 3;
        boeHeader.uid = 10; 
	char *s = (char *)malloc(100 * sizeof(char));
        memset(s, 100, '\0');
	boeHeader.headerEncode(s);
        int64_t encode_num;
        int8_t *encode_p = (int8_t *)&encode_num;
	*encode_p = s[0];
	encode_p++;
	*encode_p = s[1];
	encode_p++;
	*(uint32_t *)encode_p = *(uint32_t *)&s[2];
        int64_t encode_res = 0xa0302;
	REQUIRE( encode_num == encode_res );
}

TEST_CASE( "boeHeader.headerDecode" ) {
	BoeHeader boeHeader;
	boeHeader.version = 2;
	boeHeader.mid = 3;
        boeHeader.uid = 10; 
	char *s = (char *)malloc(100 * sizeof(char));
        memset(s, 100, '\0');
        s[0] = 1;
        s[1] = 5;
        s[2] = 1;
        s[3] = 1;
	s[4] = 0;
	s[5] = 0;
	boeHeader.headerDecode(s);
	REQUIRE(boeHeader.version == 1);
	REQUIRE(boeHeader.mid == 5);
	REQUIRE(boeHeader.uid == 257);
	REQUIRE(boeHeader.header_size == 6);
}
