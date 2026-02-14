#include <ctre.hpp>
#include <cassert>
#include <string_view>

int main() {
    using namespace std::string_view_literals;
    
    // ===== BASIC LITERAL MATCHING (Tests 1-50) =====
    assert(ctre::match<"a">("a"));
    assert(!ctre::match<"a">("b"));
    assert(ctre::match<"abc">("abc"));
    assert(!ctre::match<"abc">("abd"));
    assert(ctre::match<"hello">("hello"));
    assert(!ctre::match<"hello">("Hello"));
    assert(ctre::match<"world">("world"));
    assert(ctre::match<"">(""));
    assert(!ctre::match<"x">(""));
    assert(!ctre::match<"">("x"));
    
    assert(ctre::match<"test">("test"));
    assert(ctre::match<"regex">("regex"));
    assert(ctre::match<"pattern">("pattern"));
    assert(ctre::match<"compile">("compile"));
    assert(ctre::match<"time">("time"));
    assert(ctre::match<"simd">("simd"));
    assert(ctre::match<"avx2">("avx2"));
    assert(ctre::match<"sse">("sse"));
    assert(ctre::match<"fast">("fast"));
    assert(ctre::match<"speed">("speed"));
    
    assert(ctre::match<"1">("1"));
    assert(ctre::match<"12">("12"));
    assert(ctre::match<"123">("123"));
    assert(ctre::match<"1234">("1234"));
    assert(ctre::match<"12345">("12345"));
    assert(ctre::match<"123456">("123456"));
    assert(ctre::match<"1234567">("1234567"));
    assert(ctre::match<"12345678">("12345678"));
    assert(ctre::match<"123456789">("123456789"));
    assert(ctre::match<"1234567890">("1234567890"));
    
    assert(ctre::match<"a">("a"));
    assert(ctre::match<"ab">("ab"));
    assert(ctre::match<"abc">("abc"));
    assert(ctre::match<"abcd">("abcd"));
    assert(ctre::match<"abcde">("abcde"));
    assert(ctre::match<"abcdef">("abcdef"));
    assert(ctre::match<"abcdefg">("abcdefg"));
    assert(ctre::match<"abcdefgh">("abcdefgh"));
    assert(ctre::match<"abcdefghi">("abcdefghi"));
    assert(ctre::match<"abcdefghij">("abcdefghij"));
    
    assert(ctre::match<"z">("z"));
    assert(ctre::match<"zy">("zy"));
    assert(ctre::match<"zyx">("zyx"));
    assert(ctre::match<"zyxw">("zyxw"));
    assert(ctre::match<"zyxwv">("zyxwv"));
    assert(ctre::match<"zyxwvu">("zyxwvu"));
    assert(ctre::match<"zyxwvut">("zyxwvut"));
    assert(ctre::match<"zyxwvuts">("zyxwvuts"));
    assert(ctre::match<"zyxwvutsr">("zyxwvutsr"));
    assert(ctre::match<"zyxwvutsrq">("zyxwvutsrq"));
    
    // ===== SEARCH OPERATIONS (Tests 51-100) =====
    assert(ctre::search<"a">("a"));
    assert(ctre::search<"a">("ba"));
    assert(ctre::search<"a">("abc"));
    assert(ctre::search<"abc">("xabcy"));
    assert(ctre::search<"test">("this is a test"));
    assert(!ctre::search<"xyz">("abc"));
    assert(ctre::search<"b">("abc"));
    assert(ctre::search<"c">("abc"));
    assert(ctre::search<"bc">("abc"));
    assert(ctre::search<"ab">("abc"));
    
    assert(ctre::search<"find">("please find me"));
    assert(ctre::search<"needle">("needle in haystack"));
    assert(ctre::search<"pattern">("the pattern is here"));
    assert(ctre::search<"substring">("find substring now"));
    assert(ctre::search<"word">("word boundary"));
    assert(ctre::search<"x">("xyz"));
    assert(ctre::search<"y">("xyz"));
    assert(ctre::search<"z">("xyz"));
    assert(ctre::search<"xy">("xyz"));
    assert(ctre::search<"yz">("xyz"));
    
    assert(ctre::search<"1">("a1b"));
    assert(ctre::search<"2">("a2b"));
    assert(ctre::search<"3">("a3b"));
    assert(ctre::search<"4">("a4b"));
    assert(ctre::search<"5">("a5b"));
    assert(ctre::search<"6">("a6b"));
    assert(ctre::search<"7">("a7b"));
    assert(ctre::search<"8">("a8b"));
    assert(ctre::search<"9">("a9b"));
    assert(ctre::search<"0">("a0b"));
    
    assert(ctre::search<"start">("start middle end"));
    assert(ctre::search<"middle">("start middle end"));
    assert(ctre::search<"end">("start middle end"));
    assert(!ctre::search<"nothere">("start middle end"));
    assert(ctre::search<" ">("a b"));
    assert(ctre::search<"  ">("a  b"));
    assert(ctre::search<"   ">("a   b"));
    assert(ctre::search<"ab">("aabb"));
    assert(ctre::search<"bb">("aabb"));
    assert(ctre::search<"aa">("aabb"));
    
    assert(ctre::search<"first">("first second third"));
    assert(ctre::search<"second">("first second third"));
    assert(ctre::search<"third">("first second third"));
    assert(ctre::search<"hello">("say hello world"));
    assert(ctre::search<"world">("say hello world"));
    assert(ctre::search<"say">("say hello world"));
    assert(ctre::search<"quick">("the quick brown"));
    assert(ctre::search<"brown">("the quick brown"));
    assert(ctre::search<"the">("the quick brown"));
    assert(ctre::search<"jump">("jump over"));
    
    // ===== DOT WILDCARD (Tests 101-150) =====
    assert(ctre::match<".">("a"));
    assert(ctre::match<".">("b"));
    assert(ctre::match<".">("1"));
    assert(ctre::match<".">("@"));
    assert(!ctre::match<".">(""));
    assert(!ctre::match<".">("ab"));
    assert(ctre::match<"..">("ab"));
    assert(ctre::match<"...">("abc"));
    assert(ctre::match<"....">("abcd"));
    assert(ctre::match<".....">("abcde"));
    
    assert(ctre::match<"a.c">("abc"));
    assert(ctre::match<"a.c">("a1c"));
    assert(ctre::match<"a.c">("a@c"));
    assert(!ctre::match<"a.c">("ac"));
    assert(!ctre::match<"a.c">("abbc"));
    assert(ctre::match<".....">("12345"));
    assert(ctre::match<".....">("!@#$%"));
    assert(ctre::match<"a.b.c">("a1b2c"));
    assert(ctre::match<"x.y.z">("x@y#z"));
    assert(ctre::match<"..x..">("aaxbb"));
    
    assert(ctre::search<".">("a"));
    assert(ctre::search<".">("abc"));
    assert(ctre::search<".x.">("axb"));
    assert(ctre::search<"a..">("abc"));
    assert(ctre::search<"..c">("abc"));
    assert(ctre::match<".........">("123456789"));
    assert(ctre::match<"..........">("1234567890"));
    assert(ctre::match<"............">("abcdefghijkl"));
    assert(ctre::match<"...............">("123456789012345"));
    assert(ctre::match<"................">("1234567890123456"));
    
    assert(ctre::match<".">("x"));
    assert(ctre::match<".">("y"));
    assert(ctre::match<".">("z"));
    assert(ctre::match<"..">("xy"));
    assert(ctre::match<"..">("yz"));
    assert(ctre::match<"...">("xyz"));
    assert(ctre::match<"......">("abcdef"));
    assert(ctre::match<".......">("abcdefg"));
    assert(ctre::match<"........">("abcdefgh"));
    assert(ctre::match<".........">("abcdefghi"));
    
    assert(ctre::match<".a.">("bac"));
    assert(ctre::match<".b.">("xby"));
    assert(ctre::match<".c.">("1c2"));
    assert(ctre::match<"..a">("xya"));
    assert(ctre::match<"a..">("abc"));
    assert(ctre::match<".ab">("xab"));
    assert(ctre::match<"ab.">("abx"));
    assert(ctre::match<"..ab">("xyab"));
    assert(ctre::match<"ab..">("abxy"));
    assert(ctre::match<"......">("!@#$%^"));
    
    // ===== CHARACTER CLASSES [a-z] (Tests 151-200) =====
    assert(ctre::match<"[a-z]">("a"));
    assert(ctre::match<"[a-z]">("z"));
    assert(ctre::match<"[a-z]">("m"));
    assert(!ctre::match<"[a-z]">("A"));
    assert(!ctre::match<"[a-z]">("1"));
    assert(!ctre::match<"[a-z]">("@"));
    assert(ctre::match<"[a-z][a-z]">("ab"));
    assert(ctre::match<"[a-z][a-z][a-z]">("abc"));
    assert(ctre::match<"[a-z][a-z][a-z][a-z]">("abcd"));
    assert(ctre::match<"[a-z][a-z][a-z][a-z][a-z]">("abcde"));
    
    assert(ctre::match<"[A-Z]">("A"));
    assert(ctre::match<"[A-Z]">("Z"));
    assert(ctre::match<"[A-Z]">("M"));
    assert(!ctre::match<"[A-Z]">("a"));
    assert(!ctre::match<"[A-Z]">("1"));
    assert(ctre::match<"[0-9]">("0"));
    assert(ctre::match<"[0-9]">("5"));
    assert(ctre::match<"[0-9]">("9"));
    assert(!ctre::match<"[0-9]">("a"));
    assert(!ctre::match<"[0-9]">("A"));
    
    assert(ctre::match<"[abc]">("a"));
    assert(ctre::match<"[abc]">("b"));
    assert(ctre::match<"[abc]">("c"));
    assert(!ctre::match<"[abc]">("d"));
    assert(!ctre::match<"[abc]">("x"));
    assert(ctre::match<"[xyz]">("x"));
    assert(ctre::match<"[xyz]">("y"));
    assert(ctre::match<"[xyz]">("z"));
    assert(!ctre::match<"[xyz]">("a"));
    assert(!ctre::match<"[xyz]">("w"));
    
    assert(ctre::match<"[a-z][0-9]">("a1"));
    assert(ctre::match<"[a-z][0-9]">("z9"));
    assert(!ctre::match<"[a-z][0-9]">("1a"));
    assert(!ctre::match<"[a-z][0-9]">("aa"));
    assert(ctre::match<"[0-9][a-z]">("1a"));
    assert(ctre::match<"[0-9][a-z]">("9z"));
    assert(!ctre::match<"[0-9][a-z]">("a1"));
    assert(!ctre::match<"[0-9][a-z]">("11"));
    assert(ctre::match<"[a-zA-Z]">("a"));
    assert(ctre::match<"[a-zA-Z]">("Z"));
    
    assert(ctre::match<"[0-9][0-9]">("12"));
    assert(ctre::match<"[0-9][0-9]">("99"));
    assert(ctre::match<"[0-9][0-9]">("00"));
    assert(ctre::match<"[0-9][0-9][0-9]">("123"));
    assert(ctre::match<"[0-9][0-9][0-9][0-9]">("1234"));
    assert(ctre::match<"[a-z][a-z][a-z]">("xyz"));
    assert(ctre::match<"[a-z][a-z][a-z][a-z]">("test"));
    assert(ctre::match<"[a-zA-Z0-9]">("a"));
    assert(ctre::match<"[a-zA-Z0-9]">("Z"));
    assert(ctre::match<"[a-zA-Z0-9]">("5"));
    
    // ===== NEGATED CHARACTER CLASSES (Tests 201-250) =====
    assert(ctre::match<"[^a]">("b"));
    assert(ctre::match<"[^a]">("c"));
    assert(!ctre::match<"[^a]">("a"));
    assert(ctre::match<"[^0-9]">("a"));
    assert(!ctre::match<"[^0-9]">("5"));
    assert(ctre::match<"[^a-z]">("A"));
    assert(ctre::match<"[^a-z]">("1"));
    assert(!ctre::match<"[^a-z]">("a"));
    assert(ctre::match<"[^A-Z]">("a"));
    assert(!ctre::match<"[^A-Z]">("A"));
    
    assert(ctre::match<"[^abc]">("d"));
    assert(ctre::match<"[^abc]">("x"));
    assert(!ctre::match<"[^abc]">("a"));
    assert(!ctre::match<"[^abc]">("b"));
    assert(!ctre::match<"[^abc]">("c"));
    assert(ctre::match<"[^xyz]">("a"));
    assert(!ctre::match<"[^xyz]">("x"));
    assert(ctre::match<"[^0-9][^0-9]">("ab"));
    assert(!ctre::match<"[^0-9][^0-9]">("a1"));
    assert(!ctre::match<"[^0-9][^0-9]">("12"));
    
    assert(ctre::match<"[^a-z][0-9]">("A1"));
    assert(ctre::match<"[^a-z][0-9]">("11"));
    assert(!ctre::match<"[^a-z][0-9]">("a1"));
    assert(ctre::match<"[^A-Z][a-z]">("aa"));
    assert(ctre::match<"[^A-Z][a-z]">("1a"));
    assert(!ctre::match<"[^A-Z][a-z]">("Aa"));
    assert(ctre::match<"[^0-9]">("!"));
    assert(ctre::match<"[^0-9]">("@"));
    assert(ctre::match<"[^0-9]">("#"));
    assert(ctre::match<"[^0-9]">("$"));
    
    assert(ctre::match<"[^a]">("1"));
    assert(ctre::match<"[^a]">("@"));
    assert(ctre::match<"[^1]">("a"));
    assert(ctre::match<"[^1]">("2"));
    assert(!ctre::match<"[^1]">("1"));
    assert(ctre::match<"[^abc][^xyz]">("de"));
    assert(ctre::match<"[^abc][^xyz]">("d1"));
    assert(!ctre::match<"[^abc][^xyz]">("ax"));
    assert(!ctre::match<"[^abc][^xyz]">("ay"));
    assert(!ctre::match<"[^abc][^xyz]">("bx"));
    
    assert(ctre::match<"[^a-z]">("9"));
    assert(ctre::match<"[^a-z]">("-"));
    assert(ctre::match<"[^a-z]">("_"));
    assert(ctre::match<"[^0-9a-z]">("A"));
    assert(ctre::match<"[^0-9a-z]">("Z"));
    assert(!ctre::match<"[^0-9a-z]">("a"));
    assert(!ctre::match<"[^0-9a-z]">("5"));
    assert(ctre::match<"[^aeiou]">("b"));
    assert(ctre::match<"[^aeiou]">("k"));
    assert(!ctre::match<"[^aeiou]">("a"));
    
    // ===== QUANTIFIERS: * (Tests 251-300) =====
    assert(ctre::match<"a*">(""));
    assert(ctre::match<"a*">("a"));
    assert(ctre::match<"a*">("aa"));
    assert(ctre::match<"a*">("aaa"));
    assert(ctre::match<"a*">("aaaa"));
    assert(ctre::match<"a*">("aaaaa"));
    assert(!ctre::match<"a*">("b"));
    assert(!ctre::match<"a*">("ab"));
    assert(ctre::match<"ab*">("a"));
    assert(ctre::match<"ab*">("ab"));
    
    assert(ctre::match<"ab*">("abb"));
    assert(ctre::match<"ab*">("abbb"));
    assert(ctre::match<"ab*">("abbbb"));
    assert(!ctre::match<"ab*">("b"));
    assert(!ctre::match<"ab*">("aab"));
    assert(ctre::match<"a*b">("b"));
    assert(ctre::match<"a*b">("ab"));
    assert(ctre::match<"a*b">("aab"));
    assert(ctre::match<"a*b">("aaab"));
    assert(!ctre::match<"a*b">("a"));
    
    assert(ctre::match<"[0-9]*">(""));
    assert(ctre::match<"[0-9]*">("1"));
    assert(ctre::match<"[0-9]*">("12"));
    assert(ctre::match<"[0-9]*">("123"));
    assert(ctre::match<"[0-9]*">("1234"));
    assert(ctre::match<"[0-9]*">("12345"));
    assert(!ctre::match<"[0-9]*">("a"));
    assert(!ctre::match<"[0-9]*">("1a"));
    assert(ctre::match<"[a-z]*">(""));
    assert(ctre::match<"[a-z]*">("a"));
    
    assert(ctre::match<"[a-z]*">("abc"));
    assert(ctre::match<"[a-z]*">("abcdef"));
    assert(!ctre::match<"[a-z]*">("abc1"));
    assert(ctre::match<"a*b*">(""));
    assert(ctre::match<"a*b*">("a"));
    assert(ctre::match<"a*b*">("b"));
    assert(ctre::match<"a*b*">("ab"));
    assert(ctre::match<"a*b*">("aab"));
    assert(ctre::match<"a*b*">("abb"));
    assert(ctre::match<"a*b*">("aabb"));
    
    assert(ctre::match<"x*">(""));
    assert(ctre::match<"x*">("x"));
    assert(ctre::match<"x*">("xx"));
    assert(ctre::match<"x*">("xxx"));
    assert(ctre::match<"x*y*">(""));
    assert(ctre::match<"x*y*">("xy"));
    assert(ctre::match<"x*y*">("xxy"));
    assert(ctre::match<"x*y*">("xyy"));
    assert(ctre::match<"x*y*">("xxyy"));
    assert(ctre::match<".*">("anything"));
    
    // ===== QUANTIFIERS: + (Tests 301-350) =====
    assert(!ctre::match<"a+">("")); 
    assert(ctre::match<"a+">("a"));
    assert(ctre::match<"a+">("aa"));
    assert(ctre::match<"a+">("aaa"));
    assert(ctre::match<"a+">("aaaa"));
    assert(ctre::match<"a+">("aaaaa"));
    assert(!ctre::match<"a+">("b"));
    assert(!ctre::match<"a+">("ba"));
    assert(ctre::match<"ab+">("ab"));
    assert(ctre::match<"ab+">("abb"));
    
    assert(ctre::match<"ab+">("abbb"));
    assert(ctre::match<"ab+">("abbbb"));
    assert(!ctre::match<"ab+">("a"));
    assert(!ctre::match<"ab+">("b"));
    assert(ctre::match<"a+b">("ab"));
    assert(ctre::match<"a+b">("aab"));
    assert(ctre::match<"a+b">("aaab"));
    assert(!ctre::match<"a+b">("b"));
    assert(!ctre::match<"a+b">("abb"));
    assert(ctre::match<"[0-9]+">("1"));
    
    assert(ctre::match<"[0-9]+">("12"));
    assert(ctre::match<"[0-9]+">("123"));
    assert(ctre::match<"[0-9]+">("1234"));
    assert(ctre::match<"[0-9]+">("12345"));
    assert(!ctre::match<"[0-9]+">(""));
    assert(!ctre::match<"[0-9]+">("a"));
    assert(ctre::match<"[a-z]+">("a"));
    assert(ctre::match<"[a-z]+">("abc"));
    assert(ctre::match<"[a-z]+">("abcdef"));
    assert(!ctre::match<"[a-z]+">(""));
    
    assert(!ctre::match<"[a-z]+">("123"));
    assert(ctre::match<"a+b+">("ab"));
    assert(ctre::match<"a+b+">("aab"));
    assert(ctre::match<"a+b+">("abb"));
    assert(ctre::match<"a+b+">("aabb"));
    assert(!ctre::match<"a+b+">("a"));
    assert(!ctre::match<"a+b+">("b"));
    assert(ctre::match<"x+">("x"));
    assert(ctre::match<"x+">("xx"));
    assert(ctre::match<"x+">("xxx"));
    
    assert(ctre::match<"x+y+">("xy"));
    assert(ctre::match<"x+y+">("xxy"));
    assert(ctre::match<"x+y+">("xyy"));
    assert(ctre::match<"x+y+">("xxyy"));
    assert(!ctre::match<"x+y+">("x"));
    assert(!ctre::match<"x+y+">("y"));
    assert(ctre::match<".+">("a"));
    assert(ctre::match<".+">("ab"));
    assert(ctre::match<".+">("abc"));
    assert(!ctre::match<".+">(""));
    
    // ===== QUANTIFIERS: ? (Tests 351-400) =====
    assert(ctre::match<"a?">(""));
    assert(ctre::match<"a?">("a"));
    assert(!ctre::match<"a?">("aa"));
    assert(!ctre::match<"a?">("b"));
    assert(ctre::match<"ab?">("a"));
    assert(ctre::match<"ab?">("ab"));
    assert(!ctre::match<"ab?">("abb"));
    assert(!ctre::match<"ab?">("b"));
    assert(ctre::match<"a?b">("b"));
    assert(ctre::match<"a?b">("ab"));
    
    assert(!ctre::match<"a?b">("aab"));
    assert(!ctre::match<"a?b">("a"));
    assert(ctre::match<"[0-9]?">(""));
    assert(ctre::match<"[0-9]?">("1"));
    assert(!ctre::match<"[0-9]?">("12"));
    assert(!ctre::match<"[0-9]?">("a"));
    assert(ctre::match<"[a-z]?">(""));
    assert(ctre::match<"[a-z]?">("a"));
    assert(!ctre::match<"[a-z]?">("ab"));
    assert(!ctre::match<"[a-z]?">("1"));
    
    assert(ctre::match<"a?b?">(""));
    assert(ctre::match<"a?b?">("a"));
    assert(ctre::match<"a?b?">("b"));
    assert(ctre::match<"a?b?">("ab"));
    assert(!ctre::match<"a?b?">("aa"));
    assert(!ctre::match<"a?b?">("bb"));
    assert(!ctre::match<"a?b?">("aab"));
    assert(!ctre::match<"a?b?">("abb"));
    assert(ctre::match<"x?">(""));
    assert(ctre::match<"x?">("x"));
    
    assert(!ctre::match<"x?">("xx"));
    assert(ctre::match<"x?y?">(""));
    assert(ctre::match<"x?y?">("x"));
    assert(ctre::match<"x?y?">("y"));
    assert(ctre::match<"x?y?">("xy"));
    assert(!ctre::match<"x?y?">("xx"));
    assert(!ctre::match<"x?y?">("yy"));
    assert(ctre::match<".?">(""));
    assert(ctre::match<".?">("a"));
    assert(!ctre::match<".?">("ab"));
    
    assert(ctre::match<"colou?r">("color"));
    assert(ctre::match<"colou?r">("colour"));
    assert(!ctre::match<"colou?r">("colouur"));
    assert(ctre::match<"https?">("http"));
    assert(ctre::match<"https?">("https"));
    assert(!ctre::match<"https?">("httpss"));
    assert(ctre::match<"files?">("file"));
    assert(ctre::match<"files?">("files"));
    assert(!ctre::match<"files?">("filess"));
    assert(ctre::match<"items?">("item"));
    
    // ===== QUANTIFIERS: {n} (Tests 401-450) =====
    assert(ctre::match<"a{3}">("aaa"));
    assert(!ctre::match<"a{3}">("aa"));
    assert(!ctre::match<"a{3}">("aaaa"));
    assert(!ctre::match<"a{3}">("a"));
    assert(!ctre::match<"a{3}">(""));
    assert(ctre::match<"a{1}">("a"));
    assert(!ctre::match<"a{1}">("aa"));
    assert(ctre::match<"a{2}">("aa"));
    assert(!ctre::match<"a{2}">("a"));
    assert(ctre::match<"a{4}">("aaaa"));
    
    assert(ctre::match<"a{5}">("aaaaa"));
    assert(ctre::match<"[0-9]{3}">("123"));
    assert(!ctre::match<"[0-9]{3}">("12"));
    assert(!ctre::match<"[0-9]{3}">("1234"));
    assert(ctre::match<"[0-9]{4}">("1234"));
    assert(ctre::match<"[0-9]{5}">("12345"));
    assert(ctre::match<"[a-z]{3}">("abc"));
    assert(!ctre::match<"[a-z]{3}">("ab"));
    assert(!ctre::match<"[a-z]{3}">("abcd"));
    assert(ctre::match<"[a-z]{4}">("abcd"));
    
    assert(ctre::match<"x{2}">("xx"));
    assert(!ctre::match<"x{2}">("x"));
    assert(!ctre::match<"x{2}">("xxx"));
    assert(ctre::match<"x{3}">("xxx"));
    assert(ctre::match<".{3}">("abc"));
    assert(ctre::match<".{3}">("123"));
    assert(ctre::match<".{3}">("!@#"));
    assert(!ctre::match<".{3}">("ab"));
    assert(!ctre::match<".{3}">("abcd"));
    assert(ctre::match<".{5}">("12345"));
    
    assert(ctre::match<"a{1}b{1}">("ab"));
    assert(!ctre::match<"a{1}b{1}">("aab"));
    assert(!ctre::match<"a{1}b{1}">("abb"));
    assert(ctre::match<"a{2}b{2}">("aabb"));
    assert(!ctre::match<"a{2}b{2}">("ab"));
    assert(!ctre::match<"a{2}b{2}">("aaabbb"));
    assert(ctre::match<"[0-9]{2}[a-z]{2}">("12ab"));
    assert(!ctre::match<"[0-9]{2}[a-z]{2}">("1ab"));
    assert(!ctre::match<"[0-9]{2}[a-z]{2}">("12a"));
    assert(ctre::match<"[0-9]{3}[a-z]{3}">("123abc"));
    
    assert(ctre::match<"a{6}">("aaaaaa"));
    assert(ctre::match<"a{7}">("aaaaaaa"));
    assert(ctre::match<"a{8}">("aaaaaaaa"));
    assert(ctre::match<"a{9}">("aaaaaaaaa"));
    assert(ctre::match<"a{10}">("aaaaaaaaaa"));
    assert(ctre::match<"[0-9]{6}">("123456"));
    assert(ctre::match<"[0-9]{7}">("1234567"));
    assert(ctre::match<"[0-9]{8}">("12345678"));
    assert(ctre::match<"[0-9]{9}">("123456789"));
    assert(ctre::match<"[0-9]{10}">("1234567890"));
    
    // ===== QUANTIFIERS: {n,m} (Tests 451-500) =====
    assert(ctre::match<"a{2,4}">("aa"));
    assert(ctre::match<"a{2,4}">("aaa"));
    assert(ctre::match<"a{2,4}">("aaaa"));
    assert(!ctre::match<"a{2,4}">("a"));
    assert(!ctre::match<"a{2,4}">("aaaaa"));
    assert(ctre::match<"a{1,3}">("a"));
    assert(ctre::match<"a{1,3}">("aa"));
    assert(ctre::match<"a{1,3}">("aaa"));
    assert(!ctre::match<"a{1,3}">(""));
    assert(!ctre::match<"a{1,3}">("aaaa"));
    
    assert(ctre::match<"[0-9]{2,5}">("12"));
    assert(ctre::match<"[0-9]{2,5}">("123"));
    assert(ctre::match<"[0-9]{2,5}">("1234"));
    assert(ctre::match<"[0-9]{2,5}">("12345"));
    assert(!ctre::match<"[0-9]{2,5}">("1"));
    assert(!ctre::match<"[0-9]{2,5}">("123456"));
    assert(ctre::match<"[a-z]{1,4}">("a"));
    assert(ctre::match<"[a-z]{1,4}">("ab"));
    assert(ctre::match<"[a-z]{1,4}">("abc"));
    assert(ctre::match<"[a-z]{1,4}">("abcd"));
    
    assert(!ctre::match<"[a-z]{1,4}">("abcde"));
    assert(ctre::match<"x{0,2}">(""));
    assert(ctre::match<"x{0,2}">("x"));
    assert(ctre::match<"x{0,2}">("xx"));
    assert(!ctre::match<"x{0,2}">("xxx"));
    assert(ctre::match<".{1,5}">("a"));
    assert(ctre::match<".{1,5}">("ab"));
    assert(ctre::match<".{1,5}">("abc"));
    assert(ctre::match<".{1,5}">("abcd"));
    assert(ctre::match<".{1,5}">("abcde"));
    
    assert(!ctre::match<".{1,5}">("abcdef"));
    assert(!ctre::match<".{1,5}">(""));
    assert(ctre::match<"a{2,3}b{2,3}">("aabb"));
    assert(ctre::match<"a{2,3}b{2,3}">("aaabb"));
    assert(ctre::match<"a{2,3}b{2,3}">("aabbb"));
    assert(ctre::match<"a{2,3}b{2,3}">("aaabbb"));
    assert(!ctre::match<"a{2,3}b{2,3}">("ab"));
    assert(!ctre::match<"a{2,3}b{2,3}">("aaaabbbb"));
    assert(ctre::match<"[0-9]{3,6}">("123"));
    assert(ctre::match<"[0-9]{3,6}">("1234"));
    
    assert(ctre::match<"[0-9]{3,6}">("12345"));
    assert(ctre::match<"[0-9]{3,6}">("123456"));
    assert(!ctre::match<"[0-9]{3,6}">("12"));
    assert(!ctre::match<"[0-9]{3,6}">("1234567"));
    assert(ctre::match<"a{1,2}">("a"));
    assert(ctre::match<"a{1,2}">("aa"));
    assert(!ctre::match<"a{1,2}">("aaa"));
    assert(ctre::match<"b{2,4}">("bb"));
    assert(ctre::match<"b{2,4}">("bbb"));
    assert(ctre::match<"b{2,4}">("bbbb"));
    
    // ===== ANCHORS: ^ START (Tests 501-550) =====
    assert(ctre::match<"^a">("a"));
    assert(!ctre::match<"^a">("ba"));
    assert(ctre::match<"^abc">("abc"));
    assert(!ctre::match<"^abc">("xabc"));
    assert(ctre::match<"^[0-9]">("1"));
    assert(!ctre::match<"^[0-9]">("a1"));
    assert(ctre::match<"^test">("test"));
    assert(!ctre::match<"^test">("atest"));
    assert(ctre::match<"^hello">("hello"));
    assert(!ctre::match<"^hello">("xhello"));
    
    assert(ctre::match<"^.">("a"));
    assert(ctre::match<"^.">("1"));
    assert(!ctre::match<"^.">(""));
    assert(ctre::match<"^a+">("a"));
    assert(ctre::match<"^a+">("aa"));
    assert(!ctre::match<"^a+">("ba"));
    assert(ctre::match<"^[a-z]+">("abc"));
    assert(!ctre::match<"^[a-z]+">("1abc"));
    assert(ctre::match<"^[0-9]+">("123"));
    assert(!ctre::match<"^[0-9]+">("a123"));
    
    assert(ctre::match<"^a*">(""));
    assert(ctre::match<"^a*">("a"));
    assert(ctre::match<"^a*">("aa"));
    assert(!ctre::match<"^a*">("ba"));
    assert(ctre::match<"^abc.*">("abc"));
    assert(ctre::match<"^abc.*">("abcdef"));
    assert(!ctre::match<"^abc.*">("xabc"));
    assert(ctre::match<"^x">("x"));
    assert(!ctre::match<"^x">("xy"));
    assert(!ctre::match<"^x">("yx"));
    
    assert(ctre::match<"^start">("start"));
    assert(ctre::match<"^start.*">("starting"));
    assert(!ctre::match<"^start">("restart"));
    assert(ctre::match<"^begin">("begin"));
    assert(!ctre::match<"^begin">("abegin"));
    assert(ctre::match<"^first">("first"));
    assert(!ctre::match<"^first">("afirst"));
    assert(ctre::match<"^prefix">("prefix"));
    assert(!ctre::match<"^prefix">("xprefix"));
    assert(ctre::match<"^initial">("initial"));
    
    assert(!ctre::match<"^initial">("xinitial"));
    assert(ctre::match<"^[a-z]">("a"));
    assert(!ctre::match<"^[a-z]">("1a"));
    assert(ctre::match<"^[A-Z]">("A"));
    assert(!ctre::match<"^[A-Z]">("aA"));
    assert(ctre::match<"^.">("x"));
    assert(ctre::match<"^.+">("abc"));
    assert(ctre::match<"^.*">(""));
    assert(ctre::match<"^.*">("anything"));
    assert(ctre::match<"^a{3}">("aaa"));
    
    // ===== ANCHORS: $ END (Tests 551-600) =====
    assert(ctre::match<"a$">("a"));
    assert(!ctre::match<"a$">("ab"));
    assert(ctre::match<"abc$">("abc"));
    assert(!ctre::match<"abc$">("abcx"));
    assert(ctre::match<"[0-9]$">("1"));
    assert(!ctre::match<"[0-9]$">("1a"));
    assert(ctre::match<"test$">("test"));
    assert(!ctre::match<"test$">("testa"));
    assert(ctre::match<"hello$">("hello"));
    assert(!ctre::match<"hello$">("hellox"));
    
    assert(ctre::match<".$">("a"));
    assert(ctre::match<".$">("1"));
    assert(!ctre::match<".$">("ab"));
    assert(ctre::match<"a+$">("a"));
    assert(ctre::match<"a+$">("aa"));
    assert(!ctre::match<"a+$">("aab"));
    assert(ctre::match<"[a-z]+$">("abc"));
    assert(!ctre::match<"[a-z]+$">("abc1"));
    assert(ctre::match<"[0-9]+$">("123"));
    assert(!ctre::match<"[0-9]+$">("123a"));
    
    assert(ctre::match<"a*$">(""));
    assert(ctre::match<"a*$">("a"));
    assert(ctre::match<"a*$">("aa"));
    assert(!ctre::match<"a*$">("aab"));
    assert(ctre::match<".*abc$">("abc"));
    assert(ctre::match<".*abc$">("xyzabc"));
    assert(!ctre::match<".*abc$">("abcx"));
    assert(ctre::match<"x$">("x"));
    assert(ctre::match<".*x$">("yx"));
    assert(!ctre::match<"x$">("xy"));
    
    assert(ctre::match<"end$">("end"));
    assert(ctre::match<".*end$">("backend"));
    assert(!ctre::match<"end$">("ending"));
    assert(ctre::match<"finish$">("finish"));
    assert(!ctre::match<"finish$">("finishx"));
    assert(ctre::match<"last$">("last"));
    assert(!ctre::match<"last$">("lastx"));
    assert(ctre::match<"suffix$">("suffix"));
    assert(!ctre::match<"suffix$">("suffixx"));
    assert(ctre::match<"final$">("final"));
    
    assert(!ctre::match<"final$">("finalx"));
    assert(ctre::match<"[a-z]$">("a"));
    assert(!ctre::match<"[a-z]$">("a1"));
    assert(ctre::match<"[A-Z]$">("A"));
    assert(!ctre::match<"[A-Z]$">("Aa"));
    assert(ctre::match<".$">("x"));
    assert(ctre::match<".+$">("abc"));
    assert(ctre::match<".*$">(""));
    assert(ctre::match<".*$">("anything"));
    assert(ctre::match<"a{3}$">("aaa"));
    
    // ===== ANCHORS: ^$ BOTH (Tests 601-650) =====
    assert(ctre::match<"^a$">("a"));
    assert(!ctre::match<"^a$">("ab"));
    assert(!ctre::match<"^a$">("ba"));
    assert(!ctre::match<"^a$">("aa"));
    assert(ctre::match<"^abc$">("abc"));
    assert(!ctre::match<"^abc$">("xabc"));
    assert(!ctre::match<"^abc$">("abcx"));
    assert(!ctre::match<"^abc$">("xabcx"));
    assert(ctre::match<"^test$">("test"));
    assert(!ctre::match<"^test$">("atest"));
    
    assert(!ctre::match<"^test$">("testa"));
    assert(ctre::match<"^[0-9]$">("1"));
    assert(!ctre::match<"^[0-9]$">("12"));
    assert(!ctre::match<"^[0-9]$">("a1"));
    assert(ctre::match<"^[a-z]+$">("abc"));
    assert(!ctre::match<"^[a-z]+$">("1abc"));
    assert(!ctre::match<"^[a-z]+$">("abc1"));
    assert(ctre::match<"^[0-9]+$">("123"));
    assert(!ctre::match<"^[0-9]+$">("a123"));
    assert(!ctre::match<"^[0-9]+$">("123a"));
    
    assert(ctre::match<"^.$">("a"));
    assert(!ctre::match<"^.$">("ab"));
    assert(!ctre::match<"^.$">(""));
    assert(ctre::match<"^.+$">("a"));
    assert(ctre::match<"^.+$">("abc"));
    assert(!ctre::match<"^.+$">(""));
    assert(ctre::match<"^.*$">(""));
    assert(ctre::match<"^.*$">("abc"));
    assert(ctre::match<"^a*$">(""));
    assert(ctre::match<"^a*$">("aaa"));
    
    assert(!ctre::match<"^a*$">("aab"));
    assert(!ctre::match<"^a*$">("baa"));
    assert(ctre::match<"^x$">("x"));
    assert(!ctre::match<"^x$">("xx"));
    assert(ctre::match<"^hello$">("hello"));
    assert(!ctre::match<"^hello$">("xhello"));
    assert(!ctre::match<"^hello$">("hellox"));
    assert(ctre::match<"^world$">("world"));
    assert(ctre::match<"^exact$">("exact"));
    assert(!ctre::match<"^exact$">("notexact"));
    
    assert(ctre::match<"^[a-z]{3}$">("abc"));
    assert(!ctre::match<"^[a-z]{3}$">("ab"));
    assert(!ctre::match<"^[a-z]{3}$">("abcd"));
    assert(ctre::match<"^[0-9]{4}$">("1234"));
    assert(!ctre::match<"^[0-9]{4}$">("123"));
    assert(!ctre::match<"^[0-9]{4}$">("12345"));
    assert(ctre::match<"^a{2,4}$">("aa"));
    assert(ctre::match<"^a{2,4}$">("aaa"));
    assert(ctre::match<"^a{2,4}$">("aaaa"));
    assert(!ctre::match<"^a{2,4}$">("a"));
    
    // ===== ALTERNATION | (Tests 651-700) =====
    assert(ctre::match<"a|b">("a"));
    assert(ctre::match<"a|b">("b"));
    assert(!ctre::match<"a|b">("c"));
    assert(!ctre::match<"a|b">("ab"));
    assert(ctre::match<"cat|dog">("cat"));
    assert(ctre::match<"cat|dog">("dog"));
    assert(!ctre::match<"cat|dog">("bird"));
    assert(ctre::match<"red|blue">("red"));
    assert(ctre::match<"red|blue">("blue"));
    assert(!ctre::match<"red|blue">("green"));
    
    assert(ctre::match<"yes|no">("yes"));
    assert(ctre::match<"yes|no">("no"));
    assert(!ctre::match<"yes|no">("maybe"));
    assert(ctre::match<"on|off">("on"));
    assert(ctre::match<"on|off">("off"));
    assert(!ctre::match<"on|off">("half"));
    assert(ctre::match<"[0-9]|[a-z]">("1"));
    assert(ctre::match<"[0-9]|[a-z]">("a"));
    assert(!ctre::match<"[0-9]|[a-z]">("A"));
    assert(ctre::match<"x|y|z">("x"));
    
    assert(ctre::match<"x|y|z">("y"));
    assert(ctre::match<"x|y|z">("z"));
    assert(!ctre::match<"x|y|z">("w"));
    assert(ctre::match<"one|two|three">("one"));
    assert(ctre::match<"one|two|three">("two"));
    assert(ctre::match<"one|two|three">("three"));
    assert(!ctre::match<"one|two|three">("four"));
    assert(ctre::match<"abc|def">("abc"));
    assert(ctre::match<"abc|def">("def"));
    assert(!ctre::match<"abc|def">("ghi"));
    
    assert(ctre::match<"hello|world">("hello"));
    assert(ctre::match<"hello|world">("world"));
    assert(!ctre::match<"hello|world">("goodbye"));
    assert(ctre::match<"start|end">("start"));
    assert(ctre::match<"start|end">("end"));
    assert(!ctre::match<"start|end">("middle"));
    assert(ctre::match<"true|false">("true"));
    assert(ctre::match<"true|false">("false"));
    assert(!ctre::match<"true|false">("null"));
    assert(ctre::match<"left|right">("left"));
    
    assert(ctre::match<"left|right">("right"));
    assert(!ctre::match<"left|right">("center"));
    assert(ctre::match<"up|down">("up"));
    assert(ctre::match<"up|down">("down"));
    assert(!ctre::match<"up|down">("side"));
    assert(ctre::match<"alpha|beta|gamma">("alpha"));
    assert(ctre::match<"alpha|beta|gamma">("beta"));
    assert(ctre::match<"alpha|beta|gamma">("gamma"));
    assert(!ctre::match<"alpha|beta|gamma">("delta"));
    assert(ctre::match<"foo|bar|baz">("foo"));
    
    // ===== GROUPS: (abc) (Tests 701-750) =====
    assert(ctre::match<"(a)">("a"));
    assert(ctre::match<"(abc)">("abc"));
    assert(!ctre::match<"(abc)">("ab"));
    assert(ctre::match<"(a)(b)">("ab"));
    assert(!ctre::match<"(a)(b)">("a"));
    assert(!ctre::match<"(a)(b)">("b"));
    assert(ctre::match<"(abc)(def)">("abcdef"));
    assert(!ctre::match<"(abc)(def)">("abc"));
    assert(!ctre::match<"(abc)(def)">("def"));
    assert(ctre::match<"(a)+">("a"));
    
    assert(ctre::match<"(a)+">("aa"));
    assert(ctre::match<"(a)+">("aaa"));
    assert(!ctre::match<"(a)+">(""));
    assert(ctre::match<"(ab)+">("ab"));
    assert(ctre::match<"(ab)+">("abab"));
    assert(ctre::match<"(ab)+">("ababab"));
    assert(!ctre::match<"(ab)+">("a"));
    assert(!ctre::match<"(ab)+">("aba"));
    assert(ctre::match<"(abc)+">("abc"));
    assert(ctre::match<"(abc)+">("abcabc"));
    
    assert(!ctre::match<"(abc)+">("abcab"));
    assert(ctre::match<"(a)*">(""));
    assert(ctre::match<"(a)*">("a"));
    assert(ctre::match<"(a)*">("aa"));
    assert(ctre::match<"(ab)*">(""));
    assert(ctre::match<"(ab)*">("ab"));
    assert(ctre::match<"(ab)*">("abab"));
    assert(!ctre::match<"(ab)*">("a"));
    assert(!ctre::match<"(ab)*">("aba"));
    assert(ctre::match<"(xyz)*">(""));
    
    assert(ctre::match<"(xyz)*">("xyz"));
    assert(ctre::match<"(xyz)*">("xyzxyz"));
    assert(!ctre::match<"(xyz)*">("xy"));
    assert(ctre::match<"(a)?">(""));
    assert(ctre::match<"(a)?">("a"));
    assert(!ctre::match<"(a)?">("aa"));
    assert(ctre::match<"(ab)?">(""));
    assert(ctre::match<"(ab)?">("ab"));
    assert(!ctre::match<"(ab)?">("abab"));
    assert(ctre::match<"(test)?">(""));
    
    assert(ctre::match<"(test)?">("test"));
    assert(!ctre::match<"(test)?">("testtest"));
    assert(ctre::match<"(a){3}">("aaa"));
    assert(!ctre::match<"(a){3}">("aa"));
    assert(!ctre::match<"(a){3}">("aaaa"));
    assert(ctre::match<"(ab){2}">("abab"));
    assert(!ctre::match<"(ab){2}">("ab"));
    assert(!ctre::match<"(ab){2}">("ababab"));
    assert(ctre::match<"(abc){2}">("abcabc"));
    assert(!ctre::match<"(abc){2}">("abc"));
    
    // ===== ESCAPE SEQUENCES (Tests 751-800) =====
    assert(ctre::match<R"(\d)">("0"));
    assert(ctre::match<R"(\d)">("5"));
    assert(ctre::match<R"(\d)">("9"));
    assert(!ctre::match<R"(\d)">("a"));
    assert(!ctre::match<R"(\d)">("A"));
    assert(ctre::match<R"(\d\d)">("12"));
    assert(ctre::match<R"(\d\d\d)">("123"));
    assert(ctre::match<R"(\d+)">("123"));
    assert(ctre::match<R"(\d+)">("456789"));
    assert(!ctre::match<R"(\d+)">("abc"));
    
    assert(ctre::match<R"(\w)">("a"));
    assert(ctre::match<R"(\w)">("A"));
    assert(ctre::match<R"(\w)">("0"));
    assert(ctre::match<R"(\w)">("_"));
    assert(!ctre::match<R"(\w)">("@"));
    assert(ctre::match<R"(\w+)">("abc123"));
    assert(ctre::match<R"(\w+)">("test_var"));
    assert(!ctre::match<R"(\w+)">("test-var"));
    assert(ctre::match<R"(\w\w\w)">("abc"));
    assert(ctre::match<R"(\w\w\w)">("A1_"));
    
    assert(ctre::match<R"(\s)">(" "));
    assert(ctre::match<R"(\s)">("\t"));
    assert(ctre::match<R"(\s)">("\n"));
    assert(!ctre::match<R"(\s)">("a"));
    assert(!ctre::match<R"(\s)">("1"));
    assert(ctre::match<R"(\s+)">("   "));
    assert(ctre::match<R"(\s+)">(" \t\n"));
    assert(ctre::match<R"(\s*)">(""));
    assert(ctre::match<R"(\s*)">("  "));
    assert(ctre::match<R"(a\s+b)">("a b"));
    
    assert(ctre::match<R"(a\s+b)">("a  b"));
    assert(ctre::match<R"(a\s+b)">("a\tb"));
    assert(!ctre::match<R"(a\s+b)">("ab"));
    assert(ctre::match<R"(\D)">("a"));
    assert(ctre::match<R"(\D)">("Z"));
    assert(ctre::match<R"(\D)">("@"));
    assert(!ctre::match<R"(\D)">("0"));
    assert(!ctre::match<R"(\D)">("9"));
    assert(ctre::match<R"(\D+)">("abc"));
    assert(!ctre::match<R"(\D+)">("123"));
    
    assert(ctre::match<R"(\W)">("@"));
    assert(ctre::match<R"(\W)">("!"));
    assert(ctre::match<R"(\W)">(" "));
    assert(!ctre::match<R"(\W)">("a"));
    assert(!ctre::match<R"(\W)">("1"));
    assert(!ctre::match<R"(\W)">("_"));
    assert(ctre::match<R"(\S)">("a"));
    assert(ctre::match<R"(\S)">("1"));
    assert(!ctre::match<R"(\S)">(" "));
    assert(!ctre::match<R"(\S)">("\t"));
    
    // ===== MIXED PATTERNS (Tests 801-850) =====
    assert(ctre::match<R"([a-z]+\d+)">("abc123"));
    assert(ctre::match<R"([a-z]+\d+)">("test9"));
    assert(!ctre::match<R"([a-z]+\d+)">("abc"));
    assert(!ctre::match<R"([a-z]+\d+)">("123"));
    assert(ctre::match<R"(\d+[a-z]+)">("123abc"));
    assert(!ctre::match<R"(\d+[a-z]+)">("123"));
    assert(!ctre::match<R"(\d+[a-z]+)">("abc"));
    assert(ctre::match<R"([A-Z][a-z]+)">("Hello"));
    assert(ctre::match<R"([A-Z][a-z]+)">("World"));
    assert(!ctre::match<R"([A-Z][a-z]+)">("hello"));
    
    assert(!ctre::match<R"([A-Z][a-z]+)">("HELLO"));
    assert(ctre::match<R"(\w+@\w+)">("user@domain"));
    assert(!ctre::match<R"(\w+@\w+)">("userdomain"));
    assert(ctre::match<R"(\d{3}-\d{3}-\d{4})">("123-456-7890"));
    assert(!ctre::match<R"(\d{3}-\d{3}-\d{4})">("1234567890"));
    assert(ctre::match<R"([a-z]{2,5}\d{1,3})">("ab1"));
    assert(ctre::match<R"([a-z]{2,5}\d{1,3})">("hello99"));
    assert(!ctre::match<R"([a-z]{2,5}\d{1,3})">("a1"));
    assert(!ctre::match<R"([a-z]{2,5}\d{1,3})">("abcdef1"));
    assert(ctre::match<"^[a-z]+$">("lowercase"));
    
    assert(!ctre::match<"^[a-z]+$">("Lowercase"));
    assert(ctre::match<"^[A-Z]+$">("UPPERCASE"));
    assert(!ctre::match<"^[A-Z]+$">("Uppercase"));
    assert(ctre::match<R"(^\d+$)">("12345"));
    assert(!ctre::match<R"(^\d+$)">("123a45"));
    assert(ctre::match<R"(^\w+$)">("valid_name"));
    assert(!ctre::match<R"(^\w+$)">("invalid-name"));
    assert(ctre::match<"^[a-zA-Z0-9]+$">("Alpha123"));
    assert(!ctre::match<"^[a-zA-Z0-9]+$">("Alpha-123"));
    assert(ctre::match<R"(^[a-z]*\d*$)">("abc123"));
    
    assert(ctre::match<R"(^[a-z]*\d*$)">("abc"));
    assert(ctre::match<R"(^[a-z]*\d*$)">("123"));
    assert(ctre::match<R"(^[a-z]*\d*$)">(""));
    assert(ctre::match<"(ab|cd)+">("ab"));
    assert(ctre::match<"(ab|cd)+">("cd"));
    assert(ctre::match<"(ab|cd)+">("abcd"));
    assert(ctre::match<"(ab|cd)+">("cdab"));
    assert(!ctre::match<"(ab|cd)+">("ac"));
    assert(ctre::match<"(red|blue|green)">("red"));
    assert(ctre::match<"(red|blue|green)">("blue"));
    
    // ===== LONG STRING PATTERNS (Tests 851-900) =====
    assert(ctre::match<"abcdefghijklmnop">("abcdefghijklmnop"));
    assert(!ctre::match<"abcdefghijklmnop">("abcdefghijklmno"));
    assert(ctre::match<"123456789012345678901234567890">("123456789012345678901234567890"));
    assert(!ctre::match<"123456789012345678901234567890">("12345678901234567890123456789"));
    assert(ctre::match<"[a-z]{20}">("abcdefghijklmnopqrst"));
    assert(!ctre::match<"[a-z]{20}">("abcdefghijklmnopqrs"));
    assert(ctre::match<"[0-9]{15}">("123456789012345"));
    assert(!ctre::match<"[0-9]{15}">("12345678901234"));
    assert(ctre::match<".{25}">("1234567890123456789012345"));
    assert(!ctre::match<".{25}">("123456789012345678901234"));
    
    assert(ctre::match<"a{30}">("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    assert(!ctre::match<"a{30}">("aaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    assert(ctre::match<"(abc){10}">("abcabcabcabcabcabcabcabcabcabc"));
    assert(!ctre::match<"(abc){10}">("abcabcabcabcabcabcabcabcabcab"));
    assert(ctre::match<"[a-z]+[0-9]+[a-z]+">("abc123def"));
    assert(!ctre::match<"[a-z]+[0-9]+[a-z]+">("abc123"));
    assert(ctre::match<R"(\d{3}\.\d{3}\.\d{3}\.\d{3})">("192.168.001.001"));
    assert(!ctre::match<R"(\d{3}\.\d{3}\.\d{3}\.\d{3})">("192.168.1.1"));
    assert(ctre::search<"needle">("haystack needle haystack"));
    assert(!ctre::search<"needle">("haystack haystack"));
    
    assert(ctre::search<"pattern">("this is a pattern in text"));
    assert(!ctre::search<"pattern">("this is text"));
    assert(ctre::search<"[0-9]+">("text123more"));
    assert(ctre::search<"[0-9]+">("text 456 more"));
    assert(!ctre::search<"[0-9]+">("text more"));
    assert(ctre::search<"test">("this is a test case"));
    assert(ctre::search<"case">("this is a test case"));
    assert(!ctre::search<"missing">("this is a test case"));
    assert(ctre::match<"a{15,20}">("aaaaaaaaaaaaaaa"));
    assert(ctre::match<"a{15,20}">("aaaaaaaaaaaaaaaaaaaa"));
    
    assert(!ctre::match<"a{15,20}">("aaaaaaaaaaaaa"));
    assert(!ctre::match<"a{15,20}">("aaaaaaaaaaaaaaaaaaaaa"));
    assert(ctre::match<"[a-z]{10,15}">("abcdefghij"));
    assert(ctre::match<"[a-z]{10,15}">("abcdefghijklmno"));
    assert(!ctre::match<"[a-z]{10,15}">("abcdefghi"));
    assert(!ctre::match<"[a-z]{10,15}">("abcdefghijklmnop"));
    assert(ctre::match<R"(\d{5,10})">("12345"));
    assert(ctre::match<R"(\d{5,10})">("1234567890"));
    assert(!ctre::match<R"(\d{5,10})">("1234"));
    assert(!ctre::match<R"(\d{5,10})">("12345678901"));
    
    assert(ctre::search<"ab+">("aaabbbccc"));
    assert(ctre::search<"b+">("aaabbbccc"));
    assert(ctre::search<"c+">("aaabbbccc"));
    assert(!ctre::search<"d+">("aaabbbccc"));
    assert(ctre::search<"[aeiou]">("test"));
    assert(!ctre::search<"[aeiou]">("xyz"));
    assert(ctre::search<R"(\d)">("test1"));
    assert(!ctre::search<R"(\d)">("test"));
    assert(ctre::match<"(hello|world)+">("hello"));
    assert(ctre::match<"(hello|world)+">("helloworld"));
    
    // ===== EDGE CASES & BOUNDARY CONDITIONS (Tests 901-950) =====
    assert(ctre::match<"">(""));
    assert(!ctre::match<"">(" "));
    assert(ctre::match<"a?">(""));
    assert(ctre::match<"a*">(""));
    assert(ctre::match<"(a)*">(""));
    assert(ctre::match<"(ab)*">(""));
    assert(ctre::match<"[a-z]*">(""));
    assert(ctre::match<R"(\d*)">(""));
    assert(ctre::match<R"(\w*)">(""));
    assert(ctre::match<".*">(""));
    
    assert(ctre::match<"^$">(""));
    assert(!ctre::match<"^$">("a"));
    assert(ctre::match<"^.*$">(""));
    assert(ctre::match<"^.*$">("anything"));
    assert(ctre::match<"^a*$">(""));
    assert(ctre::match<"^a*$">("aaa"));
    assert(ctre::match<"x{0,0}">(""));
    assert(!ctre::match<"x{1,1}">(""));
    assert(ctre::match<"a{0,3}">(""));
    assert(ctre::match<"a{0,3}">("a"));
    
    assert(ctre::match<"a{0,3}">("aa"));
    assert(ctre::match<"a{0,3}">("aaa"));
    assert(!ctre::match<"a{0,3}">("aaaa"));
    assert(ctre::match<"a{1}">("a"));
    assert(!ctre::match<"a{1}">("aa"));
    assert(ctre::match<"a{1,1}">("a"));
    assert(!ctre::match<"a{1,1}">("aa"));
    assert(ctre::match<".{0,0}">(""));
    assert(!ctre::match<".{1,1}">(""));
    assert(ctre::match<"[a-z]{0,0}">(""));
    
    assert(!ctre::match<"[a-z]{1,1}">(""));
    assert(ctre::search<"">("anything"));
    assert(ctre::search<"a*">("bbb"));
    assert(ctre::search<"x*">("yyy"));
    assert(ctre::match<"a{0,5}">("a"));
    assert(ctre::match<"a{0,5}">("aa"));
    assert(ctre::match<"a{0,5}">("aaa"));
    assert(ctre::match<"a{0,5}">("aaaa"));
    assert(ctre::match<"a{0,5}">("aaaaa"));
    assert(ctre::match<"a{0,5}">(""));
    assert(!ctre::match<"a{0,5}">("aaaaaa"));
    
    assert(ctre::match<"[0-9]{0,5}">(""));
    assert(ctre::match<"[0-9]{0,5}">("1"));
    assert(ctre::match<"[0-9]{0,5}">("12345"));
    assert(!ctre::match<"[0-9]{0,5}">("123456"));
    assert(ctre::match<".*test.*">("test"));
    assert(ctre::match<".*test.*">("this test here"));
    assert(!ctre::match<".*test.*">("no such word"));
    assert(ctre::match<"(a|b|c)">("a"));
    assert(ctre::match<"(a|b|c)">("b"));
    assert(ctre::match<"((a))">("a"));
    
    // ===== SPECIAL CHARACTER TESTS (Tests 951-1000) =====
    assert(ctre::match<R"(\.)">("."));
    assert(!ctre::match<R"(\.)">("a"));
    assert(ctre::match<R"(\+)">("+"));
    assert(!ctre::match<R"(\+)">("a"));
    assert(ctre::match<R"(\*)">("*"));
    assert(!ctre::match<R"(\*)">("a"));
    assert(ctre::match<R"(\?)">("?"));
    assert(!ctre::match<R"(\?)">("a"));
    assert(ctre::match<R"(\|)">("|"));
    assert(!ctre::match<R"(\|)">("a"));
    
    assert(ctre::match<R"(\()">("("));
    assert(!ctre::match<R"(\()">("a"));
    assert(ctre::match<R"(\))">(")"));
    assert(!ctre::match<R"(\))">("a"));
    assert(ctre::match<R"(\[)">("["));
    assert(!ctre::match<R"(\[)">("a"));
    assert(ctre::match<R"(\])">("]"));
    assert(!ctre::match<R"(\])">("a"));
    assert(ctre::match<R"(\{)">("{"));
    assert(!ctre::match<R"(\{)">("a"));
    
    assert(ctre::match<R"(\})">("}"));
    assert(!ctre::match<R"(\})">("a"));
    assert(ctre::match<R"(\\)">(R"(\)"));
    assert(!ctre::match<R"(\\)">("a"));
    assert(ctre::match<R"(\^)">("^"));
    assert(!ctre::match<R"(\^)">("a"));
    assert(ctre::match<R"(\$)">("$"));
    assert(!ctre::match<R"(\$)">("a"));
    assert(ctre::match<"[ ]">(" "));
    assert(!ctre::match<"[ ]">("a"));
    
    assert(ctre::match<"[!]">("!"));
    assert(ctre::match<"[@]">("@"));
    assert(ctre::match<"[#]">("#"));
    assert(ctre::match<"[$]">("$"));
    assert(ctre::match<"[%]">("%"));
    assert(ctre::match<"[&]">("&"));
    assert(ctre::match<"[']">("'"));
    assert(ctre::match<"[\"]">("\""));
    assert(ctre::match<"[,]">(","));
    assert(ctre::match<"[;]">(";"));
    
    assert(ctre::match<"[:]">(":"));
    assert(ctre::match<"[<]">("<"));
    assert(ctre::match<"[>]">(">"));
    assert(ctre::match<"[=]">("="));
    assert(ctre::match<"[/]">("/"));
    assert(ctre::match<"[\\-]">("-"));
    assert(ctre::match<"[_]">("_"));
    assert(ctre::match<"[~]">("~"));
    assert(ctre::match<"[`]">("`"));
    assert(ctre::match<"[a-zA-Z0-9]">("a"));
    
    assert(ctre::match<"[a-zA-Z0-9]">("Z"));
    assert(ctre::match<"[a-zA-Z0-9]">("0"));
    assert(!ctre::match<"[a-zA-Z0-9]">("@"));
    assert(ctre::match<"[^a-zA-Z0-9]">("@"));
    assert(!ctre::match<"[^a-zA-Z0-9]">("a"));
    assert(ctre::match<"[!@#$%]">("!"));
    assert(ctre::match<"[!@#$%]">("@"));
    assert(ctre::match<"[!@#$%]">("#"));
    assert(ctre::match<"[!@#$%]">("$"));
    assert(ctre::match<"[!@#$%]">("%"));
    
    return 0;
}
