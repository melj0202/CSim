// CSim headless test runner (CTest target: CSimTests / MockBackend for compat).

#include <cstdio>

int runMockBackendTests();
int runRendererE2ETests();
int runRuleSetTests();
int runCellContextTests();
int runCanvasDomainTests();
int runUITokenTests();

int main()
{
	std::printf("CSimTests — headless suite\n");

	int failures = 0;
	failures += runMockBackendTests();
	failures += runRendererE2ETests();
	failures += runRuleSetTests();
	failures += runCellContextTests();
	failures += runCanvasDomainTests();
	failures += runUITokenTests();

	std::printf("\n==== %s (%d failure(s) total) ====\n",
		failures == 0 ? "ALL PASSED" : "FAILED",
		failures);
	return failures == 0 ? 0 : 1;
}
