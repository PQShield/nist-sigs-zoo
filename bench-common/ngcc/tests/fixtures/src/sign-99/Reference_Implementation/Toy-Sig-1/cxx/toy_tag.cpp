/* C++ copy of the toy keyed mix, used by the TOY_CXX row: same output, so the KAT still
   matches. std::vector and std::invalid_argument come from the C++ runtime, so the .so
   only loads if it was linked against it. Tests only. */
#include <cstring>
#include <stdexcept>
#include <vector>

extern "C" void toy_tag_cxx(const unsigned char *key, const unsigned char *m, unsigned long long mlen,
                            unsigned char *tag, unsigned n) {
    if (n == 0) throw std::invalid_argument("toy: empty key");
    std::vector<unsigned char> t(key, key + n);
    for (unsigned long long j = 0; j < mlen; j++)
        t[j % n] = (unsigned char)(t[j % n] * 31u + m[j] + 1u);
    std::memcpy(tag, t.data(), n);
}
