#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <ctre.hpp>

using namespace std::chrono;

std::string gen_ipv4() { return "192.168.1.1"; }
std::string gen_email(size_t u) { std::string e; for(size_t i=0;i<u;i++)e+=char('a'+(i%26)); e+="@example.com"; return e; }
std::string gen_uuid() { return "550e8400-e29b-41d4-a716-446655440000"; }
std::string gen_mac() { return "01:23:45:67:89:ab"; }
std::string gen_base64(size_t len) { const char* c="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; std::string r; for(size_t i=0;i<len;i++)r+=c[i%64]; return r; }
std::string gen_hex_string(size_t len) { const char* c="0123456789abcdef"; std::string r; for(size_t i=0;i<len;i++)r+=c[i%16]; return r; }
std::string gen_alphanumeric(size_t len) { std::string r; for(size_t i=0;i<len;i++)r+=char('a'+(i%26)); return r; }

template<ctll::fixed_string P>
double b(const std::string& in,int it){
    std::string_view sv(in);volatile bool d=false;
    auto s=high_resolution_clock::now();
    for(int i=0;i<it;i++){auto r=ctre::match<P>(sv);if(r)d=true;}
    auto e=high_resolution_clock::now();
    (void)d;return duration_cast<nanoseconds>(e-s).count()/(double)it;
}

int main(){
    std::cout<<"Pattern,Engine,Input_Size,Time_ns,Description\n";
    int iter=1000000;
    
    {auto in=gen_ipv4();std::cout<<"ipv4,CTRE,"<<in.length()<<","<<b<"[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+">(in,iter)<<",IP_Address\n";}
    for(auto u:{8,16,32,64}){auto in=gen_email(u);std::cout<<"email,CTRE,"<<in.length()<<","<<b<"[a-z]+@[a-z]+\\.[a-z]+">(in,iter/2)<<",Email\n";}
    {auto in=gen_uuid();std::cout<<"uuid,CTRE,"<<in.length()<<","<<b<"[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+">(in,iter/2)<<",UUID\n";}
    {auto in=gen_mac();std::cout<<"mac,CTRE,"<<in.length()<<","<<b<"[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+">(in,iter/2)<<",MAC_Address\n";}
    for(auto len:{16,32,64,128,256,512}){auto in=gen_base64(len);std::cout<<"base64,CTRE,"<<in.length()<<","<<b<"[A-Za-z0-9+/]+">(in,iter/2)<<",Base64\n";}
    for(auto len:{8,16,32,64,128,256}){auto in=gen_hex_string(len);std::cout<<"hex,CTRE,"<<in.length()<<","<<b<"[0-9a-f]+">(in,iter/2)<<",Hex_String\n";}
    for(auto len:{8,16,32,64,128}){auto in=gen_alphanumeric(len);std::cout<<"alnum,CTRE,"<<in.length()<<","<<b<"[a-zA-Z0-9]+">(in,iter/2)<<",Identifier\n";}
    
    return 0;
}
