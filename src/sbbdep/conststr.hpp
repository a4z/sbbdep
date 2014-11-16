

#ifndef CONSTSTR_HPP_
#define CONSTSTR_HPP_



// literal class
class conststr {
    const char* p;
    std::size_t sz;
 public:
    template<std::size_t N>
    constexpr conststr(const char(&a)[N]) : p(a), sz(N-1) {}

    constexpr const char* c_str() const { return p; }

    constexpr std::size_t size() const { return sz; }
};//----------------------------------------------------------------------------



#endif /* CONSTSTR_HPP_ */
