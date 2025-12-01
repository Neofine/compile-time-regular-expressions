#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <regex>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <hs/hs.h>
#include <re2/re2.h>
#include "include/ctre.hpp"

using namespace std::chrono;

// Data generators
std::string gen_ipv4() { return "192.168.1.1"; }
std::string gen_email(size_t u) { std::string e; for(size_t i=0;i<u;i++)e+=char('a'+(i%26)); e+="@example.com"; return e; }
std::string gen_uuid() { return "550e8400-e29b-41d4-a716-446655440000"; }
std::string gen_hex_color() { return "#1a2b3c"; }
std::string gen_iso_date() { return "2024-12-01"; }
std::string gen_mac() { return "01:23:45:67:89:ab"; }
std::string gen_time() { return "12:34:56"; }
std::string gen_base64(size_t len) { const char* c="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; std::string r; for(size_t i=0;i<len;i++)r+=c[i%64]; return r; }
std::string gen_hex_string(size_t len) { const char* c="0123456789abcdef"; std::string r; for(size_t i=0;i<len;i++)r+=c[i%16]; return r; }
std::string gen_alphanumeric(size_t len) { std::string r; for(size_t i=0;i<len;i++)r+=char('a'+(i%26)); return r; }

template<ctll::fixed_string P>
double bench_ctre_simd(const std::string& in, int it) {
    std::string_view sv(in); volatile bool d=false;
    auto s=high_resolution_clock::now();
    for(int i=0;i<it;i++){auto r=ctre::match<P>(sv);if(r)d=true;}
    auto e=high_resolution_clock::now();
    (void)d; return duration_cast<nanoseconds>(e-s).count()/(double)it;
}

double bench_std(const std::string& p, const std::string& in, int it) {
    std::regex re(p); volatile bool d=false;
    auto s=high_resolution_clock::now();
    for(int i=0;i<it;i++){bool r=std::regex_match(in,re);if(r)d=true;}
    auto e=high_resolution_clock::now();
    (void)d; return duration_cast<nanoseconds>(e-s).count()/(double)it;
}

double bench_pcre2(const char* p, const std::string& in, int it) {
    int en; PCRE2_SIZE eo;
    pcre2_code* re=pcre2_compile((PCRE2_SPTR)p,PCRE2_ZERO_TERMINATED,0,&en,&eo,nullptr);
    if(!re)return -1;
    pcre2_match_data* md=pcre2_match_data_create_from_pattern(re,nullptr);
    volatile bool d=false;
    auto s=high_resolution_clock::now();
    for(int i=0;i<it;i++){
        int rc=pcre2_match(re,(PCRE2_SPTR)in.c_str(),in.length(),0,PCRE2_ANCHORED,md,nullptr);
        if(rc>=0)d=true;
    }
    auto e=high_resolution_clock::now();
    pcre2_match_data_free(md);pcre2_code_free(re);(void)d;
    return duration_cast<nanoseconds>(e-s).count()/(double)it;
}

double bench_hs(const char* p, const std::string& in, int it) {
    hs_database_t* db; hs_compile_error_t* ce;
    if(hs_compile(p,HS_FLAG_SINGLEMATCH,HS_MODE_BLOCK,nullptr,&db,&ce)!=HS_SUCCESS)return -1;
    hs_scratch_t* sc=nullptr;
    if(hs_alloc_scratch(db,&sc)!=HS_SUCCESS){hs_free_database(db);return -1;}
    volatile bool d=false;
    auto s=high_resolution_clock::now();
    for(int i=0;i<it;i++){
        int mf=0;
        hs_scan(db,in.c_str(),in.length(),0,sc,
            [](unsigned,unsigned long long,unsigned long long,unsigned,void* c)->int{*(int*)c=1;return 0;},&mf);
        if(mf)d=true;
    }
    auto e=high_resolution_clock::now();
    hs_free_scratch(sc);hs_free_database(db);(void)d;
    return duration_cast<nanoseconds>(e-s).count()/(double)it;
}

double bench_re2(const std::string& p, const std::string& in, int it) {
    RE2 re(p,RE2::Options());
    if(!re.ok())return -1;
    volatile bool d=false;
    auto s=high_resolution_clock::now();
    for(int i=0;i<it;i++){bool r=RE2::FullMatch(in,re);if(r)d=true;}
    auto e=high_resolution_clock::now();
    (void)d; return duration_cast<nanoseconds>(e-s).count()/(double)it;
}

int main() {
    std::cout<<"Pattern,Engine,Input_Size,Time_ns,Description\n";
    std::cerr<<"Running real-world patterns benchmark...\n\n";
    
    int iter = 1000000;
    
    // IPv4
    std::cerr<<"IPv4... ";
    { auto in=gen_ipv4();
      std::cout<<"ipv4,CTRE-SIMD,"<<in.length()<<","<<bench_ctre_simd<"[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+">(in,iter)<<",IP_Address\n";
      std::cout<<"ipv4,std::regex,"<<in.length()<<","<<bench_std("[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+",in,iter)<<",IP_Address\n";
      std::cout<<"ipv4,PCRE2,"<<in.length()<<","<<bench_pcre2("[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+",in,iter)<<",IP_Address\n";
      std::cout<<"ipv4,Hyperscan,"<<in.length()<<","<<bench_hs("[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+",in,iter)<<",IP_Address\n";
      std::cout<<"ipv4,RE2,"<<in.length()<<","<<bench_re2("[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+",in,iter)<<",IP_Address\n";
    } std::cerr<<"done\n";
    
    // Email (variable sizes)
    std::cerr<<"Email... ";
    for(auto u:{8,16,32,64}){ auto in=gen_email(u);
      std::cout<<"email,CTRE-SIMD,"<<in.length()<<","<<bench_ctre_simd<"[a-z]+@[a-z]+\\.[a-z]+">(in,iter/2)<<",Email\n";
      std::cout<<"email,std::regex,"<<in.length()<<","<<bench_std("[a-z]+@[a-z]+\\.[a-z]+",in,iter/2)<<",Email\n";
      std::cout<<"email,PCRE2,"<<in.length()<<","<<bench_pcre2("[a-z]+@[a-z]+\\.[a-z]+",in,iter/2)<<",Email\n";
      std::cout<<"email,Hyperscan,"<<in.length()<<","<<bench_hs("[a-z]+@[a-z]+\\.[a-z]+",in,iter/2)<<",Email\n";
      std::cout<<"email,RE2,"<<in.length()<<","<<bench_re2("[a-z]+@[a-z]+\\.[a-z]+",in,iter/2)<<",Email\n";
    } std::cerr<<"done\n";
    
    // UUID
    std::cerr<<"UUID... ";
    { auto in=gen_uuid();
      std::cout<<"uuid,CTRE-SIMD,"<<in.length()<<","<<bench_ctre_simd<"[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+">(in,iter/2)<<",UUID\n";
      std::cout<<"uuid,std::regex,"<<in.length()<<","<<bench_std("[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+",in,iter/2)<<",UUID\n";
      std::cout<<"uuid,PCRE2,"<<in.length()<<","<<bench_pcre2("[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+",in,iter/2)<<",UUID\n";
      std::cout<<"uuid,Hyperscan,"<<in.length()<<","<<bench_hs("[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+",in,iter/2)<<",UUID\n";
      std::cout<<"uuid,RE2,"<<in.length()<<","<<bench_re2("[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+",in,iter/2)<<",UUID\n";
    } std::cerr<<"done\n";
    
    // MAC
    std::cerr<<"MAC... ";
    { auto in=gen_mac();
      std::cout<<"mac,CTRE-SIMD,"<<in.length()<<","<<bench_ctre_simd<"[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+">(in,iter/2)<<",MAC_Address\n";
      std::cout<<"mac,std::regex,"<<in.length()<<","<<bench_std("[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+",in,iter/2)<<",MAC_Address\n";
      std::cout<<"mac,PCRE2,"<<in.length()<<","<<bench_pcre2("[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+",in,iter/2)<<",MAC_Address\n";
      std::cout<<"mac,Hyperscan,"<<in.length()<<","<<bench_hs("[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+",in,iter/2)<<",MAC_Address\n";
      std::cout<<"mac,RE2,"<<in.length()<<","<<bench_re2("[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+",in,iter/2)<<",MAC_Address\n";
    } std::cerr<<"done\n";
    
    // Base64 (variable sizes)
    std::cerr<<"Base64... ";
    for(auto len:{16,32,64,128,256,512}){ auto in=gen_base64(len);
      std::cout<<"base64,CTRE-SIMD,"<<in.length()<<","<<bench_ctre_simd<"[A-Za-z0-9+/]+">(in,iter/2)<<",Base64\n";
      std::cout<<"base64,std::regex,"<<in.length()<<","<<bench_std("[A-Za-z0-9+/]+",in,iter/2)<<",Base64\n";
      std::cout<<"base64,PCRE2,"<<in.length()<<","<<bench_pcre2("[A-Za-z0-9+/]+",in,iter/2)<<",Base64\n";
      std::cout<<"base64,Hyperscan,"<<in.length()<<","<<bench_hs("[A-Za-z0-9+/]+",in,iter/2)<<",Base64\n";
      std::cout<<"base64,RE2,"<<in.length()<<","<<bench_re2("[A-Za-z0-9+/]+",in,iter/2)<<",Base64\n";
    } std::cerr<<"done\n";
    
    // Hex strings (variable sizes) - common in crypto/hash validation
    std::cerr<<"Hex... ";
    for(auto len:{8,16,32,64,128,256}){ auto in=gen_hex_string(len);
      std::cout<<"hex,CTRE-SIMD,"<<in.length()<<","<<bench_ctre_simd<"[0-9a-f]+">(in,iter/2)<<",Hex_String\n";
      std::cout<<"hex,std::regex,"<<in.length()<<","<<bench_std("[0-9a-f]+",in,iter/2)<<",Hex_String\n";
      std::cout<<"hex,PCRE2,"<<in.length()<<","<<bench_pcre2("[0-9a-f]+",in,iter/2)<<",Hex_String\n";
      std::cout<<"hex,Hyperscan,"<<in.length()<<","<<bench_hs("[0-9a-f]+",in,iter/2)<<",Hex_String\n";
      std::cout<<"hex,RE2,"<<in.length()<<","<<bench_re2("[0-9a-f]+",in,iter/2)<<",Hex_String\n";
    } std::cerr<<"done\n";
    
    // Alphanumeric tokens (identifiers, usernames, etc.)
    std::cerr<<"AlphaNum... ";
    for(auto len:{8,16,32,64,128}){ auto in=gen_alphanumeric(len);
      std::cout<<"alnum,CTRE-SIMD,"<<in.length()<<","<<bench_ctre_simd<"[a-zA-Z0-9]+">(in,iter/2)<<",Identifier\n";
      std::cout<<"alnum,std::regex,"<<in.length()<<","<<bench_std("[a-zA-Z0-9]+",in,iter/2)<<",Identifier\n";
      std::cout<<"alnum,PCRE2,"<<in.length()<<","<<bench_pcre2("[a-zA-Z0-9]+",in,iter/2)<<",Identifier\n";
      std::cout<<"alnum,Hyperscan,"<<in.length()<<","<<bench_hs("[a-zA-Z0-9]+",in,iter/2)<<",Identifier\n";
      std::cout<<"alnum,RE2,"<<in.length()<<","<<bench_re2("[a-zA-Z0-9]+",in,iter/2)<<",Identifier\n";
    } std::cerr<<"done\n";
    
    std::cerr<<"\n✅ Real-world benchmark complete!\n";
    return 0;
}
