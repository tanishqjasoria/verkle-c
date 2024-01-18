#ifndef FRE_LIBRARY_H
#define FRE_LIBRARY_H

void hello();

#endif //FRE_LIBRARY_H

extern "C" {
    bool AddOverflow(const FrE& a, const FrE& b, FrE* c);
    bool SubtractUnderflow(const FrE& a, const FrE& b, FrE* c)
    FrE AddMod(const FrE& a, const FrE& b, const FrE& qElement)
    FrE SubMod(const FrE& a, const FrE& b, const FrE& qElement)
    FrE MultiplyMod(const FrE& x, const FrE& y)
}