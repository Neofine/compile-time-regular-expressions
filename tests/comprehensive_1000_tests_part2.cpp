#include <ctre.hpp>
#include <cassert>
#include <string_view>
#include <string>

int main() {
    using namespace std::string_view_literals;
    
    // ===== ESCAPE SEQUENCES: \d \w \s (Tests 1-200) =====
    
    // \d - digit [0-9]
    assert(ctre::match<"\\d">("0"));
    assert(ctre::match<"\\d">("5"));
    assert(ctre::match<"\\d">("9"));
    assert(!ctre::match<"\\d">("a"));
    assert(!ctre::match<"\\d">(" "));
    
    assert(ctre::match<"\\d\\d">("12"));
    assert(ctre::match<"\\d\\d\\d">("999"));
    assert(!ctre::match<"\\d\\d">("1a"));
    assert(!ctre::match<"\\d\\d">("a1"));
    
    assert(ctre::match<"\\d+">("123"));
    assert(ctre::match<"\\d+">("0"));
    assert(ctre::match<"\\d+">("9876543210"));
    assert(!ctre::match<"\\d+">(""));
    assert(!ctre::match<"\\d+">("abc"));
    
    assert(ctre::match<"\\d*">(""));
    assert(ctre::match<"\\d*">("123"));
    assert(ctre::match<"\\d*">("0"));
    
    assert(ctre::match<"\\d?">(""));
    assert(ctre::match<"\\d?">("5"));
    assert(!ctre::match<"\\d?">("55"));
    
    assert(ctre::match<"\\d{3}">("123"));
    assert(!ctre::match<"\\d{3}">("12"));
    assert(!ctre::match<"\\d{3}">("1234"));
    
    assert(ctre::match<"\\d{2,4}">("12"));
    assert(ctre::match<"\\d{2,4}">("123"));
    assert(ctre::match<"\\d{2,4}">("1234"));
    assert(!ctre::match<"\\d{2,4}">("1"));
    assert(!ctre::match<"\\d{2,4}">("12345"));
    
    // \D - non-digit
    assert(!ctre::match<"\\D">("0"));
    assert(!ctre::match<"\\D">("9"));
    assert(ctre::match<"\\D">("a"));
    assert(ctre::match<"\\D">(" "));
    assert(ctre::match<"\\D">("@"));
    
    assert(ctre::match<"\\D+">("abc"));
    assert(ctre::match<"\\D+">("   "));
    assert(!ctre::match<"\\D+">("123"));
    
    assert(ctre::match<"\\D*">(""));
    assert(ctre::match<"\\D*">("hello"));
    
    assert(ctre::match<"\\D{3}">("abc"));
    assert(!ctre::match<"\\D{3}">("ab1"));
    
    // \w - word character [a-zA-Z0-9_]
    assert(ctre::match<"\\w">("a"));
    assert(ctre::match<"\\w">("Z"));
    assert(ctre::match<"\\w">("0"));
    assert(ctre::match<"\\w">("_"));
    assert(!ctre::match<"\\w">(" "));
    assert(!ctre::match<"\\w">("@"));
    
    assert(ctre::match<"\\w+">("word"));
    assert(ctre::match<"\\w+">("test_123"));
    assert(ctre::match<"\\w+">("_underscore"));
    assert(!ctre::match<"\\w+">("hello world"));
    
    assert(ctre::match<"\\w*">(""));
    assert(ctre::match<"\\w*">("abc123"));
    
    assert(ctre::match<"\\w{5}">("hello"));
    assert(ctre::match<"\\w{5}">("test1"));
    assert(ctre::match<"\\w{5}">("_____"));
    
    // \W - non-word character
    assert(!ctre::match<"\\W">("a"));
    assert(!ctre::match<"\\W">("0"));
    assert(!ctre::match<"\\W">("_"));
    assert(ctre::match<"\\W">(" "));
    assert(ctre::match<"\\W">("@"));
    assert(ctre::match<"\\W">("!"));
    
    assert(ctre::match<"\\W+">("   "));
    assert(ctre::match<"\\W+">("!!!"));
    assert(ctre::match<"\\W+">("@#$"));
    assert(!ctre::match<"\\W+">("word"));
    
    // \s - whitespace
    assert(ctre::match<"\\s">(" "));
    assert(ctre::match<"\\s">("\t"));
    assert(ctre::match<"\\s">("\n"));
    assert(ctre::match<"\\s">("\r"));
    assert(!ctre::match<"\\s">("a"));
    assert(!ctre::match<"\\s">("1"));
    
    assert(ctre::match<"\\s+">("   "));
    assert(ctre::match<"\\s+">("  \t  "));
    assert(ctre::match<"\\s+">("\n\n\n"));
    
    assert(ctre::match<"\\s*">(""));
    assert(ctre::match<"\\s*">("   "));
    
    assert(ctre::match<"\\s{3}">("   "));
    assert(ctre::match<"\\s{5}">("     "));
    
    // \S - non-whitespace
    assert(!ctre::match<"\\S">(" "));
    assert(!ctre::match<"\\S">("\t"));
    assert(ctre::match<"\\S">("a"));
    assert(ctre::match<"\\S">("1"));
    assert(ctre::match<"\\S">("@"));
    
    assert(ctre::match<"\\S+">("word"));
    assert(ctre::match<"\\S+">("test123"));
    assert(!ctre::match<"\\S+">("hello world"));
    
    // Combined escape sequences
    assert(ctre::match<"\\w\\d">("a1"));
    assert(ctre::match<"\\w\\d">("Z9"));
    assert(!ctre::match<"\\w\\d">("ab"));
    
    assert(ctre::match<"\\d\\w">("1a"));
    assert(ctre::match<"\\d\\w">("9Z"));
    
    assert(ctre::match<"\\w\\s\\w">("a b"));
    assert(ctre::match<"\\w\\s\\w">("1\t2"));
    
    assert(ctre::match<"\\w+\\s+\\w+">("hello world"));
    assert(ctre::match<"\\w+\\s+\\w+">("test  123"));
    
    assert(ctre::match<"\\d+\\.\\d+">("123.456"));
    assert(ctre::match<"\\d+\\.\\d+">("0.0"));
    
    // Practical patterns with escapes
    assert(ctre::match<"\\w+@\\w+\\.\\w+">("user@domain.com"));
    assert(ctre::match<"\\w+@\\w+\\.\\w+">("test@example.org"));
    
    assert(ctre::match<"\\d{3}-\\d{3}-\\d{4}">("123-456-7890"));
    assert(ctre::match<"\\d{3}-\\d{4}">("555-1234"));
    
    assert(ctre::match<"\\d+[a-z]+">("123abc"));
    assert(ctre::match<"[a-z]+\\d+">("abc123"));
    
    assert(ctre::match<"^\\d+$">("123"));
    assert(!ctre::match<"^\\d+$">("123a"));
    
    assert(ctre::match<"^\\w+$">("test"));
    assert(!ctre::match<"^\\w+$">("test "));
    
    assert(ctre::match<"^\\s+$">("   "));
    assert(!ctre::match<"^\\s+$">("  a "));
    
    // More complex escape combinations
    assert(ctre::match<"\\w+\\(\\d+\\)">("func(123)"));
    assert(ctre::match<"\\[\\d+\\]"> ("[42]"));
    assert(ctre::match<"\\d+\\s*\\+\\s*\\d+">("5 + 3"));
    assert(ctre::match<"\\d+\\s*\\+\\s*\\d+">("5+3"));
    
    assert(ctre::match<"\\w+:\\s*\\d+">("age: 25"));
    assert(ctre::match<"\\w+:\\s*\\d+">("count:42"));
    
    // Negations with escapes
    assert(ctre::match<"\\D\\D\\D">("abc"));
    assert(!ctre::match<"\\D\\D\\D">("ab1"));
    
    assert(ctre::match<"\\W\\W\\W">("@#$"));
    assert(!ctre::match<"\\W\\W\\W">("@#a"));
    
    assert(ctre::match<"\\S\\S\\S">("abc"));
    assert(!ctre::match<"\\S\\S\\S">("ab "));
    
    // ===== LAZY QUANTIFIERS (Tests 201-300) =====
    
    // Lazy star *?
    assert(ctre::match<"a*?">(""));
    assert(ctre::search<"a*?b">("b"));
    assert(ctre::search<"a*?b">("ab"));
    assert(ctre::search<"a*?b">("aaab"));
    
    assert(ctre::match<"x.*?y">("xy"));
    assert(ctre::match<"x.*?y">("xabcy"));
    
    // Lazy plus +?
    assert(ctre::match<"a+?">("a"));
    assert(ctre::search<"a+?b">("ab"));
    assert(ctre::search<"a+?b">("aaab"));
    assert(!ctre::search<"a+?b">("b"));
    
    assert(!ctre::match<"x.+?y">("xy"));  // .+? needs at least 1 char
    assert(ctre::match<"x.+?y">("xabcy"));
    
    // Lazy question ??
    assert(ctre::match<"a??">(""));
    assert(ctre::search<"a??b">("b"));
    assert(ctre::search<"a??b">("ab"));
    
    // Lazy range {n,m}?
    assert(ctre::match<"a{2,4}?">("aa"));
    assert(ctre::search<"a{2,4}?b">("aab"));
    assert(ctre::search<"a{2,4}?b">("aaab"));
    assert(ctre::search<"a{2,4}?b">("aaaab"));
    
    assert(ctre::match<"a{1,3}?">("a"));
    assert(ctre::match<"a{2,5}?">("aa"));
    
    // Lazy with different patterns
    assert(ctre::search<"<.*?>">("< html>"));
    assert(ctre::search<"<.+?>">("< html>"));
    
    assert(ctre::search<"[a-z]*?x">("abcx"));
    assert(ctre::search<"[0-9]+?5">("12345"));
    
    assert(ctre::search<"\\d+?5">("12345"));
    assert(ctre::search<"\\w*?t">("test"));
    assert(ctre::search<"\\s*?\\w">("   a"));
    
    // Lazy in complex patterns
    assert(ctre::search<"a.*?b.*?c">("aXbYc"));
    assert(ctre::search<"a.+?b.+?c">("aXbYc"));
    
    // Multiple lazy quantifiers
    assert(ctre::search<"a*?b*?c">("abc"));
    assert(ctre::search<"a*?b*?c">("c"));
    
    // Greedy followed by lazy
    assert(ctre::search<"a+b*?">("aaabbb"));
    assert(ctre::search<"a*?b+">("aaabbb"));
    
    // Lazy with anchors
    assert(ctre::match<"^a*?$">(""));
    assert(ctre::match<"^a*?$">("aaa"));
    assert(ctre::match<"^a+?$">("a"));
    
    // Lazy alternation
    assert(ctre::match<"(a|b)*?">(""));
    assert(ctre::match<"(a|b)+?">("a"));
    
    // ===== WORD BOUNDARIES \b \B (Tests 301-400) =====
    
    // Basic \b tests
    assert(ctre::search<"\\bword\\b">("word"));
    assert(ctre::search<"\\bword\\b">("a word here"));
    assert(ctre::search<"\\bword\\b">("word."));
    assert(!ctre::search<"\\bword\\b">("sword"));
    assert(!ctre::search<"\\bword\\b">("words"));
    
    assert(ctre::search<"\\btest\\b">("test"));
    assert(ctre::search<"\\btest\\b">("test "));
    assert(ctre::search<"\\btest\\b">(" test"));
    assert(ctre::search<"\\btest\\b">(" test "));
    assert(!ctre::search<"\\btest\\b">("testing"));
    
    assert(ctre::search<"\\bcat\\b">("cat"));
    assert(ctre::search<"\\bcat\\b">("the cat sat"));
    assert(ctre::search<"\\bcat\\b">("cat!"));
    assert(!ctre::search<"\\bcat\\b">("concatenate"));
    assert(!ctre::search<"\\bcat\\b">("scat"));
    
    // Start word boundary
    assert(ctre::search<"\\bthe">("the"));
    assert(ctre::search<"\\bthe">("the end"));
    assert(!ctre::search<"\\bthe">("other"));
    
    assert(ctre::search<"\\bhello">("hello"));
    assert(ctre::search<"\\bhello">("hello world"));
    assert(!ctre::search<"\\bhello">("xhello"));
    
    // End word boundary
    assert(ctre::search<"ing\\b">("testing"));
    assert(ctre::search<"ing\\b">("running away"));
    assert(!ctre::search<"ing\\b">("finger"));
    
    assert(ctre::search<"end\\b">("the end"));
    assert(ctre::search<"end\\b">("append"));
    assert(!ctre::search<"end\\b">("endless"));
    
    // Multiple word boundaries
    assert(ctre::search<"\\bword1\\b.*\\bword2\\b">("word1 and word2"));
    assert(ctre::search<"\\bfirst\\b.*\\bsecond\\b">("first then second"));
    assert(!ctre::search<"\\bword1\\b.*\\bword2\\b">("word1word2"));
    
    // Non-word boundary \B - NOW FIXED!
    assert(ctre::search<"\\Btest">("atest"));
    assert(!ctre::search<"\\Btest">("a test"));
    assert(!ctre::search<"\\Btest">("test"));
    
    assert(ctre::search<"test\\B">("testa"));
    assert(!ctre::search<"test\\B">("test a"));
    assert(!ctre::search<"test\\B">("test"));
    
    assert(ctre::search<"\\Bcat\\B">("concatenate"));
    assert(!ctre::search<"\\Bcat\\B">("cat"));
    assert(!ctre::search<"\\Bcat\\B">("the cat"));
    
    // Word boundary with character classes
    assert(ctre::search<"\\b[a-z]+\\b">("word"));
    assert(ctre::search<"\\b[a-z]+\\b">("a word here"));
    assert(ctre::search<"\\b[0-9]+\\b">("123"));
    assert(ctre::search<"\\b[0-9]+\\b">("test 456 here"));
    
    // Word boundary with quantifiers
    assert(ctre::search<"\\ba+\\b">("aaa"));
    assert(ctre::search<"\\ba+\\b">("test aaa here"));
    assert(!ctre::search<"\\ba+\\b">("baaab"));
    
    assert(ctre::search<"\\b\\d+\\b">("123"));
    assert(ctre::search<"\\b\\d+\\b">("test 999 here"));
    assert(!ctre::search<"\\b\\d+\\b">("a123b"));
    
    // Word boundary with special chars
    assert(ctre::search<"\\btest\\b">("test."));
    assert(ctre::search<"\\btest\\b">("test!"));
    assert(ctre::search<"\\btest\\b">("test?"));
    assert(ctre::search<"\\btest\\b">("test,"));
    
    // Word boundary with underscores (underscore is word char)
    assert(!ctre::search<"\\bword\\b">("_word"));
    assert(!ctre::search<"\\bword\\b">("word_"));
    assert(ctre::search<"\\b\\w+\\b">("_word_"));
    
    // Multiple words
    assert(ctre::search<"\\bone\\b">("one two three"));
    assert(ctre::search<"\\btwo\\b">("one two three"));
    assert(ctre::search<"\\bthree\\b">("one two three"));
    
    // Word boundary with exact lengths
    assert(ctre::search<"\\b\\w{4}\\b">("test"));
    assert(ctre::search<"\\b\\w{4}\\b">("word here"));
    assert(ctre::search<"\\b\\w{5}\\b">("hello world"));
    
    // ===== LOOKAHEAD & LOOKBEHIND (Tests 401-500) =====
    
    // Positive lookahead (?=...)
    assert(ctre::search<"a(?=b)">("ab"));
    assert(!ctre::search<"a(?=b)">("ac"));
    assert(!ctre::search<"a(?=b)">("a"));
    
    assert(ctre::search<"test(?=ing)">("testing"));
    assert(!ctre::search<"test(?=ing)">("tested"));
    
    assert(ctre::search<"\\d(?=\\d)">("123"));
    assert(ctre::search<"\\d(?=\\d)">("12"));
    assert(!ctre::search<"\\d(?=\\d)">("1"));
    
    // Negative lookahead (?!...)
    assert(ctre::search<"a(?!b)">("ac"));
    assert(!ctre::search<"a(?!b)">("ab"));
    
    assert(ctre::search<"test(?!ing)">("tested"));
    assert(!ctre::search<"test(?!ing)">("testing"));
    
    assert(ctre::search<"\\d(?!\\d)">("1a"));
    assert(ctre::search<"\\d(?!\\d)">("12"));  // Matches the '2'
    
    // Positive lookahead with longer patterns
    assert(ctre::search<"foo(?=bar)">("foobar"));
    assert(!ctre::search<"foo(?=bar)">("foobaz"));
    
    assert(ctre::search<"hello(?= world)">("hello world"));
    assert(!ctre::search<"hello(?= world)">("hello there"));
    
    // Negative lookahead with longer patterns
    assert(ctre::search<"foo(?!bar)">("foobaz"));
    assert(!ctre::search<"foo(?!bar)">("foobar"));
    
    assert(ctre::search<"hello(?! world)">("hello there"));
    assert(!ctre::search<"hello(?! world)">("hello world"));
    
    // Lookahead with quantifiers
    assert(ctre::search<"a+(?=b)">("aaab"));
    assert(!ctre::search<"a+(?=b)">("aaac"));
    
    assert(ctre::search<"\\w+(?=\\d)">("test123"));
    assert(!ctre::search<"\\w+(?=\\d)">("testxyz"));
    
    // Lookahead with character classes
    assert(ctre::search<"[a-z](?=[0-9])">("a1"));
    assert(!ctre::search<"[a-z](?=[0-9])">("ab"));
    
    assert(ctre::search<"[0-9](?=[a-z])">("1a"));
    assert(!ctre::search<"[0-9](?=[a-z])">("12"));
    
    // Positive lookbehind (?<=...)
    assert(ctre::search<"(?<=@)\\w+">("@user"));
    assert(!ctre::search<"(?<=@)\\w+">("user"));
    
    assert(ctre::search<"(?<=test)ing">("testing"));
    assert(!ctre::search<"(?<=test)ing">("ing"));
    
    assert(ctre::search<"(?<=\\d)\\w">("1a"));
    assert(!ctre::search<"(?<=\\d)\\w">("ab"));
    
    // Negative lookbehind (?<!...)
    assert(ctre::search<"(?<!@)\\w+">("user"));
    assert(ctre::search<"(?<!@)\\w+">("@user"));  // Matches 'ser'
    
    assert(ctre::search<"(?<!test)ing">("running"));
    assert(!ctre::search<"(?<!test)ing">("testing"));
    
    assert(ctre::search<"(?<!\\d)\\w">("ab"));
    assert(ctre::search<"(?<!\\d)\\w">("1a"));  // Matches '1' (not preceded by digit)
    
    // Lookbehind with longer patterns
    assert(ctre::search<"(?<=foo)bar">("foobar"));
    assert(!ctre::search<"(?<=foo)bar">("bazbar"));
    
    assert(ctre::search<"(?<=hello )world">("hello world"));
    assert(!ctre::search<"(?<=hello )world">("world"));
    
    // Combined lookahead and lookbehind
    assert(ctre::search<"(?<=@)\\w+(?=\\.)">("@user."));
    assert(!ctre::search<"(?<=@)\\w+(?=\\.)">("@user"));
    assert(!ctre::search<"(?<=@)\\w+(?=\\.)">("user."));
    
    // Lookaround with alternation
    assert(ctre::search<"test(?=ing|ed)">("testing"));
    assert(ctre::search<"test(?=ing|ed)">("tested"));
    assert(!ctre::search<"test(?=ing|ed)">("tester"));
    
    // ===== CAPTURE GROUPS (Tests 501-700) =====
    
    // Basic captures
    auto r1 = ctre::match<"(a)">("a");
    assert(r1);
    assert(r1.get<1>().to_view() == "a");
    
    auto r2 = ctre::match<"(abc)">("abc");
    assert(r2);
    assert(r2.get<1>().to_view() == "abc");
    
    auto r3 = ctre::match<"(a)(b)">("ab");
    assert(r3);
    assert(r3.get<1>().to_view() == "a");
    assert(r3.get<2>().to_view() == "b");
    
    auto r4 = ctre::match<"(a)(b)(c)">("abc");
    assert(r4);
    assert(r4.get<1>().to_view() == "a");
    assert(r4.get<2>().to_view() == "b");
    assert(r4.get<3>().to_view() == "c");
    
    auto r5 = ctre::match<"([0-9]+)">("123");
    assert(r5);
    assert(r5.get<1>().to_view() == "123");
    
    auto r6 = ctre::match<"([a-z]+)">("abc");
    assert(r6);
    assert(r6.get<1>().to_view() == "abc");
    
    auto r7 = ctre::match<"([a-z]+)([0-9]+)">("abc123");
    assert(r7);
    assert(r7.get<1>().to_view() == "abc");
    assert(r7.get<2>().to_view() == "123");
    
    auto r8 = ctre::match<"(.*)">("anything");
    assert(r8);
    assert(r8.get<1>().to_view() == "anything");
    
    auto r9 = ctre::match<"(.+)">("test");
    assert(r9);
    assert(r9.get<1>().to_view() == "test");
    
    auto r10 = ctre::match<"(.?)">("x");
    assert(r10);
    assert(r10.get<1>().to_view() == "x");
    
    // Nested captures
    auto r11 = ctre::match<"((a))">("a");
    assert(r11);
    assert(r11.get<1>().to_view() == "a");
    assert(r11.get<2>().to_view() == "a");
    
    auto r12 = ctre::match<"((a)(b))">("ab");
    assert(r12);
    assert(r12.get<1>().to_view() == "ab");
    assert(r12.get<2>().to_view() == "a");
    assert(r12.get<3>().to_view() == "b");
    
    // Captures with quantifiers
    auto r13 = ctre::match<"(a)+">("a");
    assert(r13);
    assert(r13.get<1>().to_view() == "a");
    
    auto r14 = ctre::match<"(a)+">("aa");
    assert(r14);
    assert(r14.get<1>().to_view() == "a"); // Last match
    
    auto r15 = ctre::match<"(ab)+">("ab");
    assert(r15);
    assert(r15.get<1>().to_view() == "ab");
    
    auto r16 = ctre::match<"(ab)+">("abab");
    assert(r16);
    assert(r16.get<1>().to_view() == "ab"); // Last match
    
    auto r17 = ctre::match<"(a){3}">("aaa");
    assert(r17);
    assert(r17.get<1>().to_view() == "a");
    
    auto r18 = ctre::match<"(ab){2}">("abab");
    assert(r18);
    assert(r18.get<1>().to_view() == "ab");
    
    // Captures with alternation
    auto r19 = ctre::match<"(a|b)">("a");
    assert(r19);
    assert(r19.get<1>().to_view() == "a");
    
    auto r20 = ctre::match<"(a|b)">("b");
    assert(r20);
    assert(r20.get<1>().to_view() == "b");
    
    auto r21 = ctre::match<"(cat|dog)">("cat");
    assert(r21);
    assert(r21.get<1>().to_view() == "cat");
    
    auto r22 = ctre::match<"(cat|dog)">("dog");
    assert(r22);
    assert(r22.get<1>().to_view() == "dog");
    
    // Captures at start/end
    auto r23 = ctre::match<"^(a)">("a");
    assert(r23);
    assert(r23.get<1>().to_view() == "a");
    
    auto r24 = ctre::match<"(a)$">("a");
    assert(r24);
    assert(r24.get<1>().to_view() == "a");
    
    auto r25 = ctre::match<"^(a)$">("a");
    assert(r25);
    assert(r25.get<1>().to_view() == "a");
    
    // Multiple captures with different patterns
    auto r26 = ctre::match<"([a-z]+)@([a-z]+)">("user@domain");
    assert(r26);
    assert(r26.get<1>().to_view() == "user");
    assert(r26.get<2>().to_view() == "domain");
    
    auto r27 = ctre::match<"([0-9]{3})-([0-9]{3})-([0-9]{4})">("123-456-7890");
    assert(r27);
    assert(r27.get<1>().to_view() == "123");
    assert(r27.get<2>().to_view() == "456");
    assert(r27.get<3>().to_view() == "7890");
    
    auto r28 = ctre::match<"([a-z]+):([0-9]+)">("host:8080");
    assert(r28);
    assert(r28.get<1>().to_view() == "host");
    assert(r28.get<2>().to_view() == "8080");
    
    // Empty captures
    auto r29 = ctre::match<"(a?)">(""); 
    assert(r29);
    assert(r29.get<1>().to_view() == "");
    
    auto r30 = ctre::match<"(a*)">("");
    assert(r30);
    assert(r30.get<1>().to_view() == "");
    
    // Captures with dots
    auto r31 = ctre::match<"(.)">("x");
    assert(r31);
    assert(r31.get<1>().to_view() == "x");
    
    auto r32 = ctre::match<"(..)">("xy");
    assert(r32);
    assert(r32.get<1>().to_view() == "xy");
    
    auto r33 = ctre::match<"(...)">("abc");
    assert(r33);
    assert(r33.get<1>().to_view() == "abc");
    
    // Four captures
    auto r34 = ctre::match<"(a)(b)(c)(d)">("abcd");
    assert(r34);
    assert(r34.get<1>().to_view() == "a");
    assert(r34.get<2>().to_view() == "b");
    assert(r34.get<3>().to_view() == "c");
    assert(r34.get<4>().to_view() == "d");
    
    // Five captures
    auto r35 = ctre::match<"(a)(b)(c)(d)(e)">("abcde");
    assert(r35);
    assert(r35.get<1>().to_view() == "a");
    assert(r35.get<2>().to_view() == "b");
    assert(r35.get<3>().to_view() == "c");
    assert(r35.get<4>().to_view() == "d");
    assert(r35.get<5>().to_view() == "e");
    
    // Captures with escape sequences
    auto r36 = ctre::match<"(\\d+)">("123");
    assert(r36);
    assert(r36.get<1>().to_view() == "123");
    
    auto r37 = ctre::match<"(\\w+)">("test");
    assert(r37);
    assert(r37.get<1>().to_view() == "test");
    
    auto r38 = ctre::match<"(\\s+)">("   ");
    assert(r38);
    assert(r38.get<1>().to_view() == "   ");
    
    auto r39 = ctre::match<"(\\w+)@(\\w+)\\.(\\w+)">("user@example.com");
    assert(r39);
    assert(r39.get<1>().to_view() == "user");
    assert(r39.get<2>().to_view() == "example");
    assert(r39.get<3>().to_view() == "com");
    
    auto r40 = ctre::match<"(\\d{2})/(\\d{2})/(\\d{4})">("12/31/2024");
    assert(r40);
    assert(r40.get<1>().to_view() == "12");
    assert(r40.get<2>().to_view() == "31");
    assert(r40.get<3>().to_view() == "2024");
    
    // Captures with word boundaries
    auto r41 = ctre::search<"\\b(\\w+)\\b">("word");
    assert(r41);
    assert(r41.get<1>().to_view() == "word");
    
    auto r42 = ctre::search<"\\b(test)\\b">("a test here");
    assert(r42);
    assert(r42.get<1>().to_view() == "test");
    
    // Captures in search
    auto r43 = ctre::search<"(test)">("this is a test");
    assert(r43);
    assert(r43.get<1>().to_view() == "test");
    
    auto r44 = ctre::search<"([0-9]+)">("abc123def");
    assert(r44);
    assert(r44.get<1>().to_view() == "123");
    
    auto r45 = ctre::search<"([a-z]+)">("123abc456");
    assert(r45);
    assert(r45.get<1>().to_view() == "abc");
    
    // Capture optional patterns
    auto r46 = ctre::match<"(a)?(b)">("b");
    assert(r46);
    assert(!r46.get<1>());
    assert(r46.get<2>().to_view() == "b");
    
    auto r47 = ctre::match<"(a)?(b)">("ab");
    assert(r47);
    assert(r47.get<1>().to_view() == "a");
    assert(r47.get<2>().to_view() == "b");
    
    // ===== SIMD BOUNDARY TESTS (Tests 701-900) =====
    
    // Use std::string to generate exact-length strings
    std::string s16_a(16, 'a');
    std::string s15_a(15, 'a');
    std::string s17_a(17, 'a');
    std::string s32_a(32, 'a');
    std::string s31_a(31, 'a');
    std::string s33_a(33, 'a');
    std::string s64_a(64, 'a');
    
    // Strings at SIMD boundaries (16 bytes)
    assert(ctre::match<"[a-z]{16}">("abcdefghijklmnop"));  // Exactly 16
    assert(!ctre::match<"[a-z]{16}">("abcdefghijklmno"));   // 15
    assert(!ctre::match<"[a-z]{16}">("abcdefghijklmnopq")); // 17
    
    assert(ctre::match<"[0-9]{16}">("1234567890123456"));  // Exactly 16
    assert(!ctre::match<"[0-9]{16}">("123456789012345"));   // 15
    
    assert(ctre::match<".{16}">(s16_a));
    assert(ctre::match<".{16}">("1234567890abcdef"));
    
    assert(ctre::match<"a{16}">(s16_a));
    assert(!ctre::match<"a{16}">(s15_a));
    
    // Strings at 32 bytes
    assert(ctre::match<"[a-z]{32}">("abcdefghijklmnopqrstuvwxyzabcdef"));  // 32
    assert(!ctre::match<"[a-z]{32}">("abcdefghijklmnopqrstuvwxyzabcde"));  // 31
    
    assert(ctre::match<"[0-9]{32}">("12345678901234567890123456789012")); // 32
    assert(!ctre::match<"[0-9]{32}">("1234567890123456789012345678901")); // 31
    
    assert(ctre::match<".{32}">(s32_a));
    assert(ctre::match<"a{32}">(s32_a));
    
    // Repetitions crossing 16-byte boundary
    assert(ctre::match<"a{15}b">(s15_a + "b"));
    assert(ctre::match<"a{16}b">(s16_a + "b"));
    assert(ctre::match<"a{17}b">(s17_a + "b"));
    
    // Repetitions crossing 32-byte boundary
    assert(ctre::match<"a{31}b">(s31_a + "b"));
    assert(ctre::match<"a{32}b">(s32_a + "b"));
    assert(ctre::match<"a{33}b">(s33_a + "b"));
    
    // Mixed patterns around boundaries
    assert(ctre::match<"[0-9]{15}[a-z]">("123456789012345a"));
    assert(ctre::match<"[0-9]{16}[a-z]">("1234567890123456a"));
    assert(ctre::match<"[0-9]{31}[a-z]">("1234567890123456789012345678901a"));
    assert(ctre::match<"[0-9]{32}[a-z]">("12345678901234567890123456789012a"));
    
    // Character class changes at boundaries
    assert(ctre::match<"[a-z]{15}[0-9]">("abcdefghijklmno1"));
    assert(ctre::match<"[a-z]{16}[0-9]">("abcdefghijklmnop1"));
    assert(ctre::match<"[a-z]{31}[0-9]">("abcdefghijklmnopqrstuvwxyzabcde1"));
    assert(ctre::match<"[a-z]{32}[0-9]">("abcdefghijklmnopqrstuvwxyzabcdef1"));
    
    // Dots around boundaries
    assert(ctre::match<".{15}x">("123456789012345x"));
    assert(ctre::match<".{16}x">("1234567890123456x"));
    assert(ctre::match<".{17}x">("12345678901234567x"));
    assert(ctre::match<".{31}x">("1234567890123456789012345678901x"));
    assert(ctre::match<".{32}x">("12345678901234567890123456789012x"));
    
    // Plus quantifier around boundaries
    assert(ctre::match<"a+">("aaaaaaaaaaaaaaaa")); // 16 a's
    assert(ctre::match<"a+">("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")); // 32 a's
    assert(ctre::match<"a+">("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")); // 64 a's
    
    assert(ctre::match<"[0-9]+">("1234567890123456")); // 16 digits
    assert(ctre::match<"[0-9]+">("12345678901234567890123456789012")); // 32 digits
    
    assert(ctre::match<"[a-z]+">("abcdefghijklmnop")); // 16 letters
    assert(ctre::match<"[a-z]+">("abcdefghijklmnopqrstuvwxyzabcdef")); // 32 letters
    
    // Star quantifier around boundaries
    assert(ctre::match<"a*">("aaaaaaaaaaaaaaaa"));
    assert(ctre::match<"a*">("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    assert(ctre::match<"[0-9]*">("1234567890123456"));
    assert(ctre::match<"[0-9]*">("12345678901234567890123456789012"));
    
    // Exact 64 bytes
    assert(ctre::match<".{64}">("1234567890123456789012345678901234567890123456789012345678901234"));
    assert(ctre::match<"a{64}">("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    assert(ctre::match<"[a-z]{64}">("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijkl"));
    assert(ctre::match<"[0-9]{64}">("1234567890123456789012345678901234567890123456789012345678901234"));
    
    // Patterns longer than 64
    std::string long_a(100, 'a');
    assert(ctre::match<"a{100}">(long_a));
    
    std::string long_any(100, 'x');
    assert(ctre::match<".{100}">(long_any));
    
    // Captures across SIMD boundaries
    auto rs1 = ctre::match<"(.{16})">("1234567890123456");
    assert(rs1);
    assert(rs1.get<1>().to_view() == "1234567890123456");
    
    auto rs2 = ctre::match<"(.{32})">("12345678901234567890123456789012");
    assert(rs2);
    assert(rs2.get<1>().to_view() == "12345678901234567890123456789012");
    
    auto rs3 = ctre::match<"(.{64})">("1234567890123456789012345678901234567890123456789012345678901234");
    assert(rs3);
    assert(rs3.get<1>().to_view() == "1234567890123456789012345678901234567890123456789012345678901234");
    
    // Alternation at boundaries
    assert(ctre::match<"(a{16}|b{16})">("aaaaaaaaaaaaaaaa"));
    assert(ctre::match<"(a{16}|b{16})">("bbbbbbbbbbbbbbbb"));
    assert(ctre::match<"([a-z]{32}|[0-9]{32})">("abcdefghijklmnopqrstuvwxyzabcdef"));
    assert(ctre::match<"([a-z]{32}|[0-9]{32})">("12345678901234567890123456789012"));
    
    // ===== COMPLEX COMBINATIONS (Tests 901-1000) =====
    
    // Everything combined
    auto rc1 = ctre::match<"^([a-z]{2,5})-(\\d{3})-([A-Z])$">("test-123-X");
    assert(rc1);
    assert(rc1.get<1>().to_view() == "test");
    assert(rc1.get<2>().to_view() == "123");
    assert(rc1.get<3>().to_view() == "X");
    
    // Email-like pattern
    assert(ctre::match<"\\w+@\\w+\\.\\w+">("user@example.com"));
    assert(ctre::match<"\\w+@\\w+\\.\\w+">("test@domain.org"));
    assert(!ctre::match<"\\w+@\\w+\\.\\w+">("invalid.email"));
    
    // Phone patterns
    assert(ctre::match<"\\d{3}-\\d{3}-\\d{4}">("123-456-7890"));
    assert(ctre::match<"\\(\\d{3}\\)\\s?\\d{3}-\\d{4}">("(123) 456-7890"));
    assert(ctre::match<"\\(\\d{3}\\)\\s?\\d{3}-\\d{4}">("(123)456-7890"));
    
    // Date patterns
    assert(ctre::match<"\\d{2}/\\d{2}/\\d{4}">("12/31/2024"));
    assert(ctre::match<"\\d{4}-\\d{2}-\\d{2}">("2024-12-31"));
    
    // Time patterns
    assert(ctre::match<"\\d{2}:\\d{2}">("14:30"));
    assert(ctre::match<"\\d{2}:\\d{2}:\\d{2}">("14:30:45"));
    
    // IP address (simplified)
    assert(ctre::match<"\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}">("192.168.1.1"));
    assert(ctre::match<"\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}">("127.0.0.1"));
    
    // Variable name patterns
    assert(ctre::match<"[a-zA-Z_][a-zA-Z0-9_]*">("variable"));
    assert(ctre::match<"[a-zA-Z_][a-zA-Z0-9_]*">("_private"));
    assert(!ctre::match<"[a-zA-Z_][a-zA-Z0-9_]*">("2invalid"));
    
    // Lazy with word boundaries
    assert(ctre::search<"\\b\\w+?\\b">("test"));
    assert(ctre::search<"\\b.+?\\b">("word"));
    
    // Lookahead with captures
    auto rc2 = ctre::search<"(\\w+)(?=@)">("user@");
    assert(rc2);
    assert(rc2.get<1>().to_view() == "user");
    
    // Lookbehind with captures
    auto rc3 = ctre::search<"(?<=@)(\\w+)">("@domain");
    assert(rc3);
    assert(rc3.get<1>().to_view() == "domain");
    
    // Multiple features together
    assert(ctre::search<"\\b\\w+(?=\\s)">("hello world"));
    assert(ctre::search<"(?<=\\s)\\w+\\b">("hello world"));
    
    // Lazy with alternation
    assert(ctre::match<"(a|b)*?c">("aaabbbcccababcc") || ctre::search<"(a|b)*?c">("aaabbbcccababc"));
    
    // Success marker
    assert(true);
    
    return 0;
}
