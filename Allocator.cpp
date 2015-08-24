//__gnu_cxx::new_allocator< typename > Class Template Reference
//https://gcc.gnu.org/onlinedocs/gcc-4.9.0/libstdc++/api/a00057.html
/**
 processor	: 0
 vendor_id	: AuthenticAMD
 cpu family	: 16
 model		: 6
 model name	: AMD Athlon(tm) II X2 270 Processor
 stepping	: 3
 microcode	: 0x10000c8
 cpu MHz		: 2000.000
 cache size	: 1024 KB
 ...
 processor	: 1
 vendor_id	: AuthenticAMD
 cpu family	: 16
 model		: 6
 model name	: AMD Athlon(tm) II X2 270 Processor
 stepping	: 3
 microcode	: 0x10000c8
 cpu MHz		: 800.000
 cache size	: 1024 KB
 ...
 Linux debian 3.14-2-686-pae #1 SMP Debian 3.14.15-2 (2014-08-09) i686 GNU/Linux
 ...
 gcc (Debian 4.9.1-12) 4.9.1
 Copyright (C) 2014 Free Software Foundation, Inc.
 This is free software; see the source for copying conditions.  There is NO
 warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
...
java@debian:~/java/eclipse$ ldd /usr/lib/i386-linux-gnu/libstdc++.so.6.0.20
	linux-gate.so.1 (0xb7733000)
	libm.so.6 => /lib/i386-linux-gnu/i686/cmov/libm.so.6 (0xb75da000)
	libc.so.6 => /lib/i386-linux-gnu/i686/cmov/libc.so.6 (0xb742f000)
	/lib/ld-linux.so.2 (0xb7734000)
	libgcc_s.so.1 => /lib/i386-linux-gnu/libgcc_s.so.1 (0xb7411000)

 */
#include <iostream>
using namespace std;
using namespace __gnu_cxx;

class RequiredAllocation
{
public:
	RequiredAllocation ();
	~RequiredAllocation ();
	std::basic_string<char> s = "hello world!\n";
};

RequiredAllocation::RequiredAllocation ()
{
	cout << "RequiredAllocation::RequiredAllocation()" << endl;
}
RequiredAllocation::~RequiredAllocation ()
{
	cout << "RequiredAllocation::~RequiredAllocation()" << endl;
}





void alloc(__gnu_cxx ::new_allocator<RequiredAllocation>* all, unsigned int size, void* pt, RequiredAllocation* t){
	try
		{
			all->allocate (size, pt);
			cout << all->max_size () << endl;
			for (auto& e : t->s)
				{
					cout << e;
				}
		}
	catch (std::bad_alloc& e)
		{
			cout << e.what () << endl;
		}
}

int
main ()
{

	__gnu_cxx ::new_allocator<RequiredAllocation> *all =
			new __gnu_cxx ::new_allocator<RequiredAllocation> ();

	RequiredAllocation t;
	void* pt = &t;

	/**
	 * What happens when new can find no store to allocate? By default, the allocator throws a stan-
	 * dard-library bad_alloc exception (for an alternative, see §11.2.4.1)
	 * @C Bjarne Stroustrup  The C++ Programming language
	 */
	unsigned int size = 1073741824;
	alloc(all, size, &pt, &t);

	size = 1;
	alloc(all, size, &pt, &t);

	return 0;
}