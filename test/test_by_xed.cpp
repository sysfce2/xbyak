#include <stdio.h>
#include <xbyak/xbyak.h>

struct Code : Xbyak::CodeGenerator {
	Code()
		 : Xbyak::CodeGenerator(4096*8)
	{
#include "tmp.cpp"
	}
};

int main()
    try
{
    Code c;
	FILE *fp = fopen("bin", "wb");
	if (fp) {
		fwrite(c.getCode(), 1, c.getSize(), fp);
		fclose(fp);
	}
} catch (std::exception& e) {
    printf("ERR %s\n", e.what());
	return 1;
}
