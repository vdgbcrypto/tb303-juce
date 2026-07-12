// Definitive aliasing test via real FFT. Render a HIGH note square (where
// harmonics alias) naive vs polyBlep. Measure energy in the band where only
// ALIASES of a high note live. Less alias energy = better band-limiting.
#include <cmath>
#include <cstdio>
#include <vector>
#include <complex>
#include <cstdlib>
static const double PI = 3.14159265358979323846;
static double gSR = 44100.0;
typedef std::complex<double> Cx;

double pb(double t, double dt) {
    if (t < dt) { double u = t / dt; return u + u - u * u - 1.0; }
    else if (t > 1 - dt) { double u = (t - 1) / dt; return u * u + u + u + 1.0; }
    return 0.0;
}
void fft(std::vector<Cx>& a) {
    int n = (int)a.size();
    for (int i = 1, j = 0; i < n; i++) {
        int b = n >> 1; for (; j & b; b >>= 1) j ^= b; j ^= b;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (int len = 2; len <= n; len <<= 1) {
        double ang = -2 * PI / len; Cx wlen(cos(ang), sin(ang));
        for (int i = 0; i < n; i += len) {
            Cx w(1);
            for (int k = 0; k < len / 2; k++) {
                Cx u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v; a[i + k + len / 2] = u - v; w *= wlen;
            }
        }
    }
}
std::vector<double> render(bool blep, double freq, int N) {
    std::vector<double> o(N); double ph = 0, dt = freq / gSR;
    for (int n = 0; n < N; n++) {
        double t = ph / (2 * PI); double sq = (t < 0.5) ? 1.0 : -1.0;
        if (blep) { double tp = t + 0.5; if (tp >= 1) tp -= 1; sq = sq + pb(t, dt) - pb(tp, dt); }
        o[n] = sq; ph += dt * 2 * PI; if (ph > 2 * PI) ph -= 2 * PI;
    }
    return o;
}
double bandEnergy(const std::vector<Cx>& F, int lo, int hi) {
    double s = 0; for (int k = lo; k <= hi; k++) s += std::abs(F[k]); return s;
}
int main() {
    int N = 4096;
    int fbin = (int)floor(9000.0 / gSR * N);
    double freq = fbin * (double)gSR / N;
    auto naive = render(false, freq, N), bl = render(true, freq, N);
    std::vector<Cx> fa(N), fb(N);
    for (int n = 0; n < N; n++) { fa[n] = naive[n]; fb[n] = bl[n]; }
    fft(fa); fft(fb);
    int lo = (int)(11025.0 / gSR * N), hi = (int)(22050.0 / gSR * N);
    double ne = bandEnergy(fa, lo, hi), be = bandEnergy(fb, lo, hi);
    printf("f0=%.0f Hz  alias-band [11k,22k]: naive=%.1f  blep=%.1f  reduction=%.1f%%\n",
           freq, ne, be, (1 - be / (ne + 1e-9)) * 100.0);
    bool ok = true; for (double v : bl) if (!std::isfinite(v)) ok = false;
    printf("BLEP output finite: %s\n", ok ? "yes" : "NO (BUG)");
    return 0;
}
