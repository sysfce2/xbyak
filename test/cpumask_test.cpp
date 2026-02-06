#define XBYAK_CPUMASK_N 8
#define XBYAK_CPUMASK_BITN 3
#include <xbyak/xbyak_util.h>
#include <cybozu/test.hpp>

using namespace Xbyak::util;

CYBOZU_TEST_AUTO(pattern)
{
	const uint32_t bitN = XBYAK_CPUMASK_BITN;
	const uint32_t bit = 1 << bitN;
	for (uint32_t i = 0; i < (1 << bit); i++) {
		CpuMask m;
		uint32_t cnt = 0;
		for (uint32_t j = 0; j < bit; j++) {
			if (i & (1 << j)) {
				cnt++;
				CYBOZU_TEST_ASSERT(m.append(j));
			}
		}
		CYBOZU_TEST_EQUAL(m.size(), cnt);
#if 0
		printf("pattern (%3u) ", i);
		for (int j = int(bit) - 1; j >= 0; j--) {
			if (i & (uint64_t(1) << j)) printf("%d ", j);
		}
		printf("\n");
		m.dump();
		m.put();
#endif
		uint32_t idx = 0;
		for (const auto& v : m) {
			CYBOZU_TEST_ASSERT(i & (1 << v));
			idx++;
		}
		CYBOZU_TEST_EQUAL(idx, cnt);
	}
}
