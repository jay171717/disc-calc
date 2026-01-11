#include <cstdio>
#include <vector>
#include <algorithm>
#include <string>
#include <emscripten/emscripten.h>

using namespace std;

typedef unsigned long long ull;
typedef __int128_t int128; 

struct Window { ull start, length; };
struct MegaClock { vector<Window> windows; ull period; int id1, id2; };

string format128(int128 n) {
    if (n == 0) return "0";
    string s = "";
    while (n > 0) { s += (char)('0' + (n % 10)); n /= 10; }
    reverse(s.begin(), s.end());
    return s;
}

ull gcd(ull a, ull b) { while (b) { a %= b; swap(a, b); } return a; }
ull lcm(ull a, ull b) { return (a == 0 || b == 0) ? 0 : a * (b / gcd(a, b)); }

int128 extended_gcd(int128 a, int128 b, int128 &x, int128 &y) {
    if (b == 0) { x = 1; y = 0; return a; }
    int128 x1, y1, g = extended_gcd(b, a % b, x1, y1);
    x = y1; y = x1 - y1 * (a / b); return g;
}

ull find_actual_first_overlap(ull s1, ull p1, ull l1, ull s2, ull p2, ull l2) {
    ull earliest = 0xFFFFFFFFFFFFFFFFULL;
    for (ull i = 0; i < l1; ++i) {
        for (ull j = 0; j < l2; ++j) {
            int128 n1 = p1, n2 = p2, a1 = s1 + i, a2 = s2 + j, x, y;
            int128 g = extended_gcd(n1, n2, x, y);
            if ((a2 - a1) % g == 0) {
                int128 res = (x * (a2 - a1) / g) % (n2 / g);
                if (res < 0) res += n2 / g;
                ull sync_time = (ull)(a1 + res * n1);
                if (sync_time < earliest) earliest = sync_time;
            }
        }
    }
    return earliest;
}

MegaClock merge_optimized(MegaClock A, MegaClock B) {
    MegaClock res; res.period = lcm(A.period, B.period); res.id1 = A.id1; res.id2 = B.id2;
    printf("Merging Groups (%d-%d) and (%d-%d). New Cycle: %llu\n", A.id1, A.id2, B.id1, B.id2, res.period);
    ull total_j = res.period / A.period;
    for (const auto& wa : A.windows) {
        for (ull ji = 0; ji < total_j; ++ji) {
            ull ca_s = wa.start + (ji * A.period), ca_e = ca_s + wa.length - 1;
            ull st_jB = (ca_s / B.period) * B.period; if (st_jB > 0) st_jB -= B.period;
            for (ull jB = st_jB; jB <= ca_e && jB < res.period; jB += B.period) {
                for (const auto& wb : B.windows) {
                    ull cb_s = wb.start + jB, cb_e = cb_s + wb.length - 1;
                    ull is = max(ca_s, cb_s), ie = min(ca_e, cb_e);
                    if (is <= ie) res.windows.push_back({is, ie - is + 1});
                }
            }
        }
    }
    sort(res.windows.begin(), res.windows.end(), [](const Window& a, const Window& b) { return a.start < b.start; });
    return res;
}

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void solve(int n, ull* X, ull* Y, ull* O) {
        vector<MegaClock> active;
        for (int i = 0; i < n; i++) {
            MegaClock c; c.period = X[i] + Y[i]; c.id1 = i+1; c.id2 = i+1;
            c.windows.push_back({O[i] + X[i], Y[i]}); active.push_back(c);
        }
        while (active.size() > 2) {
            vector<MegaClock> next;
            for (size_t i = 0; i < active.size(); i += 2) {
                if (i + 1 < active.size()) next.push_back(merge_optimized(active[i], active[i+1]));
                else next.push_back(active[i]);
            }
            active = next;
        }
        if (active.size() == 2) {
            ull min_t = 0xFFFFFFFFFFFFFFFFULL;
            for (const auto& wa : active[0].windows) {
                for (const auto& wb : active[1].windows) {
                    ull r = find_actual_first_overlap(wa.start, active[0].period, wa.length, wb.start, active[1].period, wb.length);
                    if (r < min_t) min_t = r;
                }
            }
            printf("\n>>> ABSOLUTE FIRST TICK: %s <<<\n", format128((int128)min_t).c_str());
            printf(">>> Time: %.4f years\n", (double)min_t / (20.0 * 3600 * 24 * 365.25));
        }
    }
}