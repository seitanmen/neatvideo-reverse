# musl 1.2.6 libm (W2a: alt 2 of the libm sensitivity harness)

Only the files listed below, **unmodified**, from the official release tarball. They are compiled by
`native/Makefile.libm_alt` (`make -C native libm-alt`) into `libm_alt/obj/musl.o`; the default build does
not use them. The upstream configure / Makefile were not run.

- Upstream: https://musl.libc.org/ (release 1.2.6, 2026-03-20, https://musl.libc.org/releases.html)
- Tarball: https://musl.libc.org/releases/musl-1.2.6.tar.gz
  SHA256 `d585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a`,
  SHA512 `1adad96eddb3a2eb0cacb3e363b0046568925fcdd75cf8b0503f2139df1f693d64730779ca0ce8131b7624ab2d37f4247bb1d3393c523de6e30d2b1d7732555c`
  (equal to the `sha512sums` of Alpine Linux aports `main/musl/APKBUILD`, pkgver 1.2.6, checked 2026-10-29).
  The release's PGP signature (`musl-1.2.6.tar.gz.asc`) was downloaded but **not verified** (no gpg on this machine).
- Retrieved 2026-10-29; the paths below are relative to `musl-1.2.6/` in the tarball and to this directory.
- The release page lists CVE-2026-40200 and CVE-2026-6042 for versions up to 1.2.6; they concern the C library outside
  `src/math` (none of the files below), and these files are only used for the libm comparison.
- Functions used: `pow` (pow.c + pow_data.c + exp_data.c, ARM optimized-routines), `tanf` (tanf.c + __tandf.c + __rem_pio2f.c
  + __rem_pio2_large.c), `atan2` (atan2.c + atan.c), `sin` / `cos` / `sincos` (sin.c, cos.c, sincos.c + __sin.c, __cos.c,
  __rem_pio2.c, __rem_pio2_large.c), `expf` (expf.c + exp2f_data.c, ARM optimized-routines), and the error helpers
  `__math_{oflow,uflow,xflow,invalid}[f]`. `src/internal/libm.h` and `arch/{aarch64,generic}/fp_arch.h` as in the tree.
  musl's public headers are not used: the system `<math.h>`, `<stdint.h>`, `<float.h>` are, with two stand-ins written
  for W2a in `native/libm_alt/musl_shim/` (`features.h`: the `hidden` / `weak` macros of `src/include/features.h`;
  `endian.h`: `__BYTE_ORDER`, little endian). The public names are renamed to `nv_mu_*` (`native/libm_alt/musl_rename.h`)
  and every other symbol is made local by `ld -r -exported_symbols_list native/libm_alt/musl.exp`. They call from the
  system libm only `fabs`, `floor`, `scalbn` (exact functions).
- Flags: `-std=c99 -O2 -ffp-contract=off -fno-fast-math -fno-builtin -fno-strict-aliasing -frounding-math` (the upstream
  `-std=c99 -fno-builtin -frounding-math -fno-strict-aliasing` plus ours; `-ffreestanding -nostdinc` are not used because
  the host headers replace musl's) and `-Wno-unused-but-set-variable` (one upstream warning, libm.h:175).
  clang defines `__FP_FAST_FMA` on arm64, so pow.c takes its `__builtin_fma` path as a musl build for aarch64 would.

| SHA256 | file |
|---|---|
| `b870108ec5e7790e9f9919064f1b9421d62d5f9b0e6c230c6adf7ea2da62e97b` | `COPYRIGHT` |
| `4a2018637e5cf7dad14b85245589bbe11d7b37c75de49bfdc22c8c4a3217019b` | `arch/aarch64/fp_arch.h` |
| `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` | `arch/generic/fp_arch.h` |
| `d5a11757541471030d8f4bf7a39a29de35732ecbb291b2f45bf03296be4f25a2` | `src/internal/libm.h` |
| `3f0f774aab60a18d7ef935ffdf121bcda17cbadabff14096c614da22e31a42c1` | `src/math/__cos.c` |
| `b56440ed59fa1e1aaaf6bf8b672c8bb71eaf844457906f7773da1d67d7ef013b` | `src/math/__math_invalid.c` |
| `74669de53bc4b250183cfa262a8a03ee8106f33a83e3aca6b217bafc7363b83c` | `src/math/__math_oflow.c` |
| `69414934bb798f5261e2d80f4b878e2bbe4dfa6025adbd06586195ba5554b04a` | `src/math/__math_oflowf.c` |
| `9a841197bbe9ade9bbd684f8bc65424a97c481d37dabb9fa1790bafe7959034a` | `src/math/__math_uflow.c` |
| `2371ed08fc95e48833a67122b5ef538ce7fc87f40fc7a4df451d71e1787d1d42` | `src/math/__math_uflowf.c` |
| `5feafe10347636884a071a5096f912d5ca4a0939844804ca09b88241405648ad` | `src/math/__math_xflow.c` |
| `38da031cfed0ba90e68a14dfbc57404ebcb827c9ac34648161c7d11cb212c9ba` | `src/math/__math_xflowf.c` |
| `938329e0939cd83ee2185a52154b41242f618d4a6df1b08395f344da33d4f1e6` | `src/math/__rem_pio2.c` |
| `b97f19fb5951c83a0c10506fab35646caefde13bd03100ad2af55ef00af0b9fb` | `src/math/__rem_pio2_large.c` |
| `b73a0219ba4a34e39ed904b5008126ff1b12dfa1a4a380941363c8db5088d756` | `src/math/__rem_pio2f.c` |
| `f3a37ef9bcfb8a2f9c809d8c7f1aa2c17c8c3319ad7fb2fe7c0a6284ea8859d2` | `src/math/__sin.c` |
| `60455f3cc0ac743f3eaf0017a3a4f0adbab37c0ee30d29397852d22904c40444` | `src/math/__tandf.c` |
| `1cffb67a65456ae0bc5f07a94df04a4f18ebc1ac0bffe3219250c4e0555ee01d` | `src/math/atan.c` |
| `a5f94b09b10b4e72bb15d94ae403c9dfd27d7ad881ecd729000004429acfca63` | `src/math/atan2.c` |
| `3a74e054057d636fa494bdd76a53a40db5cee618264f6ab31cb0538ba63c6bda` | `src/math/cos.c` |
| `3da38c471cc5eb8dc1c2e283c6c00a36b1d353ba5c8ca1e82d95180948e26bd5` | `src/math/exp2f_data.c` |
| `fb52b690bef63df2e8996f2d2b3fd77f9ea08795863e73afdc882df04460e850` | `src/math/exp2f_data.h` |
| `c55dff64e369fe816cd8ce367064989bb119f85783599680d1a329031c660689` | `src/math/exp_data.c` |
| `df77fca837edd02b260dd09a2914202775909ae67a059218ae38cfab104e6d16` | `src/math/exp_data.h` |
| `225123b9b3a47a083c06f2580c6b73d5cadae5cec545f9e928f7041ff837b4b4` | `src/math/expf.c` |
| `b06f0a08d74fcff8cced7afa2c0e47e301e3e8a19c0de5becdec28cd7b014027` | `src/math/pow.c` |
| `2052ec913f9b4a2170a98a6fcbcd7694f5dec220036b00c36106a2b4b65a4c55` | `src/math/pow_data.c` |
| `5014345a7958adcf6ff835b5e96afa99dd5a98c40dfe968442288e002b8e01a3` | `src/math/pow_data.h` |
| `24e53a9d85f196f66919cc19247be5480bb607ef38844b1a24bc47c4fe4974ef` | `src/math/sin.c` |
| `2b88cfba380b8d5aa8f8f6b9f055aee91fe112de9f967a4484807afcb5405d9d` | `src/math/sincos.c` |
| `38b39aaf03d33341e606b67b3743ee5fa436b7e8338af155994d4b794b366003` | `src/math/tanf.c` |

## License (COPYRIGHT of the release, full text)

````
musl as a whole is licensed under the following standard MIT license:

----------------------------------------------------------------------
Copyright © 2005-2020 Rich Felker, et al.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
----------------------------------------------------------------------

Authors/contributors include:

A. Wilcox
Ada Worcester
Alex Dowad
Alex Suykov
Alexander Monakov
Andre McCurdy
Andrew Kelley
Anthony G. Basile
Aric Belsito
Arvid Picciani
Bartosz Brachaczek
Benjamin Peterson
Bobby Bingham
Boris Brezillon
Brent Cook
Chris Spiegel
Clément Vasseur
Daniel Micay
Daniel Sabogal
Daurnimator
David Carlier
David Edelsohn
Denys Vlasenko
Dmitry Ivanov
Dmitry V. Levin
Drew DeVault
Emil Renner Berthing
Fangrui Song
Felix Fietkau
Felix Janda
Gianluca Anzolin
Hauke Mehrtens
He X
Hiltjo Posthuma
Isaac Dunham
Jaydeep Patil
Jens Gustedt
Jeremy Huntwork
Jo-Philipp Wich
Joakim Sindholt
John Spencer
Julien Ramseier
Justin Cormack
Kaarle Ritvanen
Khem Raj
Kylie McClain
Leah Neukirchen
Luca Barbato
Luka Perkov
Lynn Ochs
M Farkas-Dyck (Strake)
Mahesh Bodapati
Markus Wichmann
Masanori Ogino
Michael Clark
Michael Forney
Mikhail Kremnyov
Natanael Copa
Nicholas J. Kain
orc
Pascal Cuoq
Patrick Oppenlander
Petr Hosek
Petr Skocik
Pierre Carrier
Reini Urban
Rich Felker
Richard Pennington
Ryan Fairfax
Samuel Holland
Segev Finer
Shiz
sin
Solar Designer
Stefan Kristiansson
Stefan O'Rear
Szabolcs Nagy
Timo Teräs
Trutz Behn
Will Dietz
William Haddon
William Pitcock

Portions of this software are derived from third-party works licensed
under terms compatible with the above MIT license:

The TRE regular expression implementation (src/regex/reg* and
src/regex/tre*) is Copyright © 2001-2008 Ville Laurikari and licensed
under a 2-clause BSD license (license text in the source files). The
included version has been heavily modified by Rich Felker in 2012, in
the interests of size, simplicity, and namespace cleanliness.

Much of the math library code (src/math/* and src/complex/*) is
Copyright © 1993,2004 Sun Microsystems or
Copyright © 2003-2011 David Schultz or
Copyright © 2003-2009 Steven G. Kargl or
Copyright © 2003-2009 Bruce D. Evans or
Copyright © 2008 Stephen L. Moshier or
Copyright © 2017-2018 Arm Limited
and labelled as such in comments in the individual source files. All
have been licensed under extremely permissive terms.

The ARM memcpy code (src/string/arm/memcpy.S) is Copyright © 2008
The Android Open Source Project and is licensed under a two-clause BSD
license. It was taken from Bionic libc, used on Android.

The AArch64 memcpy and memset code (src/string/aarch64/*) are
Copyright © 1999-2019, Arm Limited.

The implementation of DES for crypt (src/crypt/crypt_des.c) is
Copyright © 1994 David Burren. It is licensed under a BSD license.

The implementation of blowfish crypt (src/crypt/crypt_blowfish.c) was
originally written by Solar Designer and placed into the public
domain. The code also comes with a fallback permissive license for use
in jurisdictions that may not recognize the public domain.

The smoothsort implementation (src/stdlib/qsort.c) is Copyright © 2011
Lynn Ochs and is licensed under an MIT-style license.

The x86_64 port was written by Nicholas J. Kain and is licensed under
the standard MIT terms.

The mips and microblaze ports were originally written by Richard
Pennington for use in the ellcc project. The original code was adapted
by Rich Felker for build system and code conventions during upstream
integration. It is licensed under the standard MIT terms.

The mips64 port was contributed by Imagination Technologies and is
licensed under the standard MIT terms.

The powerpc port was also originally written by Richard Pennington,
and later supplemented and integrated by John Spencer. It is licensed
under the standard MIT terms.

All other files which have no copyright comments are original works
produced specifically for use as part of this library, written either
by Rich Felker, the main author of the library, or by one or more
contibutors listed above. Details on authorship of individual files
can be found in the git version control history of the project. The
omission of copyright and license comments in each file is in the
interest of source tree size.

In addition, permission is hereby granted for all public header files
(include/* and arch/*/bits/*) and crt files intended to be linked into
applications (crt/*, ldso/dlstart.c, and arch/*/crt_arch.h) to omit
the copyright notice and permission notice otherwise required by the
license, and to use these files without any requirement of
attribution. These files include substantial contributions from:

Bobby Bingham
John Spencer
Nicholas J. Kain
Rich Felker
Richard Pennington
Stefan Kristiansson
Szabolcs Nagy

all of whom have explicitly granted such permission.

This file previously contained text expressing a belief that most of
the files covered by the above exception were sufficiently trivial not
to be subject to copyright, resulting in confusion over whether it
negated the permissions granted in the license. In the spirit of
permissive licensing, and of not having licensing issues being an
obstacle to adoption, that text has been removed.
````
