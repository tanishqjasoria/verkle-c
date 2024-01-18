#include <stdio.h>
#include <iostream>
#include <x86intrin.h>

typedef unsigned long ulong;

const ulong QInvNeg = 17410672245482742751UL;
const ulong Q0 = 8429901452645165025UL;
const ulong Q1 = 18415085837358793841UL;
const ulong Q2 = 922804724659942912UL;
const ulong Q3 = 2088379214866112338UL;

struct FrE {
    union {
        struct{
            unsigned long u0;
            unsigned long u1;
            unsigned long u2;
            unsigned long u3;
        };
        char bytes[32]; // Size of the union
    };

};


//struct FrE {
//    unsigned long u0;
//    unsigned long u1;
//    unsigned long u2;
//    unsigned long u3;
//};

const FrE qElement = FrE{Q0, Q1, Q2, Q3};

const ulong R0 = 15831548891076708299UL;
const ulong R1 = 4682191799977818424UL;
const ulong R2 = 12294384630081346794UL;
const ulong R3 = 785759240370973821UL;
const FrE rSquare = FrE{R0, R1, R2, R3};

ulong BigMul(ulong a, ulong b, ulong* low)
{
    uint num1 = (uint) a;
    uint num2 = (uint) (a >> 32);
    uint num3 = (uint) b;
    uint num4 = (uint) (b >> 32);
    ulong num5 = (ulong) num1 * (ulong) num3;
    ulong num6 = (ulong) num2 * (ulong) num3 + (num5 >> 32);
    ulong num7 = (ulong) num1 * (ulong) num4 + (ulong) (uint) num6;
    *low = num7 << 32 | (ulong) (uint) num5;
    return (ulong) num2 * (ulong) num4 + (num6 >> 32) + (num7 >> 32);
}

unsigned long AddWithCarry(unsigned long a, unsigned long b, unsigned long *carry) {
    unsigned long res = a + b + *carry;
    *carry = ((a & b) | ((a | b) & ~res)) >> 63;
    return res;
}

unsigned long MAdd0(unsigned long a, unsigned long b, unsigned long c)
{
    ulong carry = 0;
    ulong lo;
    ulong hi = BigMul(a,b,&lo);
    lo = AddWithCarry(lo, c, &carry);
    hi = hi + carry;
    return hi;
}

unsigned long MAdd1(unsigned long a, unsigned long b, unsigned long c, unsigned long *lo)
{
    ulong carry = 0;
    ulong hi = BigMul(a,b,lo);
    *lo = AddWithCarry(*lo, c, &carry);
    hi = hi + carry;
    return hi;
}

unsigned long MAdd2(unsigned long a, unsigned long b, unsigned long c, unsigned long d, unsigned long *lo)
{
    ulong carry = 0;
    ulong hi = BigMul(a,b,lo);
    c = AddWithCarry(c, d, &carry);
    hi = hi + carry;
    carry = 0;
    *lo = AddWithCarry(*lo, c, &carry);
    hi = hi + carry;
    return hi;
}

unsigned long MAdd3(unsigned long a, unsigned long b, unsigned long c, unsigned long d, unsigned long e, unsigned long *lo)
{
    ulong carry = 0;
    ulong hi = BigMul(a,b,lo);
    c = AddWithCarry(c, d, &carry);
    hi = hi + carry;
    carry = 0;
    *lo = AddWithCarry(*lo, c, &carry);
    hi = hi + e + carry;
    return hi;
}

unsigned long SubtractWithBorrow(unsigned long a, unsigned long b, unsigned long *borrow) {
    unsigned long res = a - b - *borrow;
    *borrow = ((~a & b) | (~(a ^ b) & res)) >> 63;
    return res;
}

bool LessThan(const FrE& a, const FrE& b)
{
    if (a.u3 != b.u3)
        return a.u3 < b.u3;
    if (a.u2 != b.u2)
        return a.u2 < b.u2;
    if (a.u1 != b.u1)
        return a.u1 < b.u1;
    return a.u0 < b.u0;
}

FrE RightShiftByOne(const FrE& x)
{
    return FrE{
            (x.u0 >> 1) | (x.u1 << 63),
            (x.u1 >> 1) | (x.u2 << 63),
            (x.u2 >> 1) | (x.u3 << 63),
            x.u3 >> 1
    };
}

extern "C" {

    bool AddOverflow(const FrE& a, const FrE& b, FrE* c)
    {
        FrE p{};
        unsigned long carry = 0;
        p.u0 = AddWithCarry(a.u0, b.u0, &carry);
        p.u1 = AddWithCarry(a.u1, b.u1, &carry);
        p.u2 = AddWithCarry(a.u2, b.u2, &carry);
        p.u3 = AddWithCarry(a.u3, b.u3, &carry);
        *c = p;
        return carry != 0;
    }

    bool SubtractUnderflow(const FrE& a, const FrE& b, FrE* c)
    {
        FrE p{};
        unsigned long borrow = 0;
        p.u0 = SubtractWithBorrow(a.u0, b.u0, &borrow);
        p.u1 = SubtractWithBorrow(a.u1, b.u1, &borrow);
        p.u2 = SubtractWithBorrow(a.u2, b.u2, &borrow);
        p.u3 = SubtractWithBorrow(a.u3, b.u3, &borrow);
        *c = p;
        return borrow != 0;
    }

    FrE AddMod(const FrE& a, const FrE& b)
    {
        FrE c{};
        AddOverflow(a,b,&c);

        if(!LessThan(c, qElement))
        {
            SubtractUnderflow(c,qElement,&c);
        }
        return c;
    }

    FrE SubMod(const FrE& a, const FrE& b)
    {
        FrE c{};
        if (SubtractUnderflow(a, b, &c))
        {
            AddOverflow(qElement, c, &c);
        }
        return c;
    }

    FrE MultiplyMod(const FrE& x, const FrE& y)
    {
        ulong t[4] = {0,0,0,0};
        ulong c[3] = {0,0,0};
        ulong z[4] = {0,0,0,0};

        // round 0
        c[1] = BigMul(x.u0, y.u0, & c[0]);
        ulong m = c[0] * QInvNeg;
        c[2] = MAdd0(m, Q0, c[0]);
        c[1] = MAdd1(x.u0, y.u1, c[1], & c[0]);
        c[2] = MAdd2(m, Q1, c[2], c[0], & t[0]);
        c[1] = MAdd1(x.u0, y.u2, c[1], & c[0]);
        c[2] = MAdd2(m, Q2, c[2], c[0], & t[1]);
        c[1] = MAdd1(x.u0, y.u3, c[1], & c[0]);
        t[3] = MAdd3(m, Q3, c[0], c[2], c[1], & t[2]);

        // round 1
        c[1] = MAdd1(x.u1, y.u0, t[0], & c[0]);
        m = c[0] * QInvNeg;
        c[2] = MAdd0(m, Q0, c[0]);
        c[1] = MAdd2(x.u1, y.u1, c[1], t[1], & c[0]);
        c[2] = MAdd2(m, Q1, c[2], c[0], & t[0]);
        c[1] = MAdd2(x.u1, y.u2, c[1], t[2], & c[0]);
        c[2] = MAdd2(m, Q2, c[2], c[0], & t[1]);
        c[1] = MAdd2(x.u1, y.u3, c[1], t[3], & c[0]);
        t[3] = MAdd3(m, Q3, c[0], c[2], c[1], & t[2]);

        // round 2
        c[1] = MAdd1(x.u2, y.u0, t[0], & c[0]);
        m = c[0] * QInvNeg;
        c[2] = MAdd0(m, Q0, c[0]);
        c[1] = MAdd2(x.u2, y.u1, c[1], t[1], & c[0]);
        c[2] = MAdd2(m, Q1, c[2], c[0], & t[0]);
        c[1] = MAdd2(x.u2, y.u2, c[1], t[2], & c[0]);
        c[2] = MAdd2(m, Q2, c[2], c[0], & t[1]);
        c[1] = MAdd2(x.u2, y.u3, c[1], t[3], & c[0]);
        t[3] = MAdd3(m, Q3, c[0], c[2], c[1], & t[2]);

        // round 3
        c[1] = MAdd1(x.u3, y.u0, t[0], & c[0]);
        m = c[0] * QInvNeg;
        c[2] = MAdd0(m, Q0, c[0]);
        c[1] = MAdd2(x.u3, y.u1, c[1], t[1], & c[0]);
        c[2] = MAdd2(m, Q1, c[2], c[0], & z[0]);
        c[1] = MAdd2(x.u3, y.u2, c[1], t[2], & c[0]);
        c[2] = MAdd2(m, Q2, c[2], c[0], & z[1]);
        c[1] = MAdd2(x.u3, y.u3, c[1], t[3], & c[0]);
        z[3] = MAdd3(m, Q3, c[0], c[2], c[1], & z[2]);


        FrE res = {z[0],z[1],z[2],z[3]};
        if (LessThan(qElement, res)) SubtractUnderflow(res, qElement, & res);
        return res;
    }

    FrE Inverse(const FrE& x)
    {
        // initialize u = q
        FrE u = qElement;
        // initialize s = r^2
        FrE s = rSquare;
        FrE r = {0,0,0,0};
        FrE v = x;


        while (true)
        {
            while ((v.u0 & 1) == 0)
            {
                v = RightShiftByOne(v);
                if ((s.u0 & 1) == 1) AddOverflow(s, qElement, & s);

                s = RightShiftByOne(s);
            }

            while ((u.u0 & 1) == 0)
            {
                u = RightShiftByOne(u);
                if ((r.u0 & 1) == 1) AddOverflow(r, qElement, & r);

                r = RightShiftByOne(r);
            }

            if (!LessThan(v, u))
            {
                SubtractUnderflow(v, u, & v);
                s = SubMod(s, r);
            }
            else
            {
                SubtractUnderflow(u, v, & u);
                r = SubMod(r, s);
            }


            if (u.u0 == 1 && (u.u3 | u.u2 | u.u1) == 0)
            {
                return r;
            }

            if (v.u0 == 1 && (v.u3 | v.u2 | v.u1) == 0)
            {
                return s;
            }
        }
    }
}