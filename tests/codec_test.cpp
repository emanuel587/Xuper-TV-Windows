#include "codec.hpp"
#include <cassert>
#include <iostream>
#include <string>
int main() {
    const std::string key="AAECAwQFBgcICQoLDA0ODxAREhMUFRYX";
    const std::string plain="{\"hello\":\"Switch\"}";
    // Generated independently with Java's Cipher DESede/ECB/PKCS5Padding.
    const std::string golden="323654342b6875754853534f714a7a416f34677463673449685447572f694441";
    std::string encoded,decoded;
    assert(xs::encode_request(plain,key,encoded));assert(encoded==golden);
    assert(xs::encode_request(plain,key+"GBka",encoded));assert(encoded==golden);
    assert(xs::decode_response_data(golden,key,decoded));assert(decoded==plain);
    for(const std::string& input:{std::string(),std::string("12345678"),std::string("texto en espa\xc3\xb1ol"),std::string(4096,'a')}) {
        assert(xs::encode_request(input,key,encoded));
        assert(xs::decode_response_data(encoded,key,decoded));assert(decoded==input);
    }
    for(const char* invalid:{"", "f", "zz", "0000", "41414141"})assert(!xs::decode_response_data(invalid,key,decoded));
    assert(!xs::encode_request(plain,"bad",encoded));
    std::string prepared;
    assert(xs::prepare_password("test-password","test-salt","0123456789abcdef",prepared));
    assert(prepared=="6a12c47951d60026663e8598d53cd3bbc7K9hoi6vxbILr1diHv57A==");
    assert(xs::account_type("demo@example.invalid")=="2");
    assert(xs::account_type("12345678")=="1");
    assert(xs::account_type("Usuario123")=="6");
    assert(!xs::prepare_password("test-password","", "0123456789abcdef",prepared));
    std::cout<<"codec: Java vector, round trips and malformed inputs passed\n";
}
