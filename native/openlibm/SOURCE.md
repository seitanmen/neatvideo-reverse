# openlibm 0.8.8 (W2a: alt 1 of the libm sensitivity harness)

Only the files listed below, **unmodified**, from the official release tarball. They are compiled by
`native/Makefile.libm_alt` (`make -C native libm-alt`) into `libm_alt/obj/openlibm.o`; the default build does
not use them. The upstream Makefile / Make.inc / CMakeLists were not run.

- Upstream: https://github.com/JuliaMath/openlibm (tag `v0.8.8`, GitHub release published 2026-09-02)
- Tarball: https://github.com/JuliaMath/openlibm/archive/refs/tags/v0.8.8.tar.gz
  SHA256 `721bab18ff37160fd9e903eb211ff26fdee38c3d0041d556b07d76089c435d17`
  (equal to the `sha256` of Homebrew's `Formula/o/openlibm.rb` for the same URL, checked 2026-10-29)
- Retrieved 2026-10-29; the paths below are relative to `openlibm-0.8.8/` in the tarball and to this directory.
- Functions used: `pow` (e_pow.c), `tanf` (s_tanf.c + k_tanf.c + e_rem_pio2f.c + k_rem_pio2.c), `atan2` (e_atan2.c + s_atan.c),
  `sin` / `cos` / `sincos` (s_sin.c, s_cos.c, s_sincos.c + k_sin.c, k_cos.c, e_rem_pio2.c, k_rem_pio2.c), `expf` (e_expf.c).
  The public names are renamed to `nv_ol_*` by the forced include `native/libm_alt/openlibm_rename.h` and every other
  symbol is made local by `ld -r -exported_symbols_list native/libm_alt/openlibm.exp`. They call from the system libm
  only `fabs`, `floor`, `scalbn`, `sqrt` (exact functions).
- Flags: `-std=c99 -O2 -ffp-contract=off -fno-fast-math -fno-builtin -fno-strict-aliasing -D__BSD_VISIBLE -DASSEMBLER`
  (the upstream `-std=c99 -fno-builtin -fno-strict-aliasing -D__BSD_VISIBLE -DASSEMBLER` plus ours) and
  `-Wno-sign-compare` (one upstream warning, e_pow.c:134).

| SHA256 | file |
|---|---|
| `b1843fbf5b03f519a5f0a44fce751bdd1022ae7148614923f9d6293e17a18b17` | `LICENSE.md` |
| `fe9cd6b24c166532b4c5a13cf98f3dbab66c3f93f6c68e4cc0dae09795cc19cd` | `include/openlibm_complex.h` |
| `3967372af64f76527304d752755b4ccaa2c23f8797e92413899111553df26931` | `include/openlibm_defs.h` |
| `0330f443aadc9016e6722912f83f1512fea7da9e533d09e8ea890c94879b964f` | `include/openlibm_math.h` |
| `1f38b5ed74fb2b64bf217856c7bae8be3ac4c7c8ff8e2bbc488dff41326b432c` | `src/aarch64_fpmath.h` |
| `7b03076c9b5e0701bb7d040ed498e4c2ffc499d98bdc6fb2eb13610369d9a168` | `src/amd64_fpmath.h` |
| `0735fe65ceaee64ac4eb45576682b7d64b410838ae33253b9b7c775f55ccbc93` | `src/cdefs-compat.h` |
| `56780ad4087137c9e745b12fe9fd1b87bc36b8c85ea8d40b0c7fec316491ed52` | `src/e_atan2.c` |
| `f661c4664b285c0c2516024a7d1e3f671e9e4675333f1f97b6b434fa78bec1ba` | `src/e_expf.c` |
| `13c580f60a6e2f48ef36ff1e9c46cdd394c9cd9c0fceb18f2526404fcb949ec7` | `src/e_pow.c` |
| `70218a9f96dde3f3045521600330d9c46040212c617a1bfd539cc1e766e6660b` | `src/e_rem_pio2.c` |
| `f9b7bfdf4ccd060abfaf1e49737824eb6c3d49fb50320872fb687bf40728f1e0` | `src/e_rem_pio2f.c` |
| `49bce4b53605cf4886fa6d4abe7e5b18cb5d61a235d9a0f203d131cc770c731b` | `src/fpmath.h` |
| `3d7fce5fa51a8469bf69ff3757109986872f889b73efecd2a9787f0584797cb3` | `src/k_cos.c` |
| `ade789bf2edce95bcbe7582caf9978c4b0759891c1f5e4fca9278bd4715de2a8` | `src/k_rem_pio2.c` |
| `1c9ab096c98e3f589aeb7a14619fb82e4b63e82f6ee9be8349af8c96de2a3e7a` | `src/k_sin.c` |
| `a9c3887f7ecc96b069e564408e8eb482fe6b181641639175d7a3fb017bef4e8d` | `src/k_tanf.c` |
| `7be165cf6b3e18967db4fba46fff1f59e5164fb93947b08b4ebc6407d5125445` | `src/math_private.h` |
| `c27fb884f372de93426727e6674f592bf29c21b2c72463a6ce4bc47f57c7ba2c` | `src/math_private_openbsd.h` |
| `2e987a843fa83c0a3d96dbca9a041a917b4c50449bdb3605142bca1697ff9075` | `src/s_atan.c` |
| `e3e86b502f3629cd6cb14f83585266b43e2595646ad62fbbbebd0a349c34458c` | `src/s_cos.c` |
| `c6a867fea444a062615ea521c6f307531baf35d03b039615cb7fcdaf3775e6ee` | `src/s_sin.c` |
| `3a872d76c10007b8b2646a18a282fc7c9af3e634a1bb23866c7152827d44de7d` | `src/s_sincos.c` |
| `a7506a558bae10eaca5e8c2e17de1349f5399b74a07075a93e4a1e4f5858d18d` | `src/s_tanf.c` |
| `9160de0e95c492f7ae07a2f5b3f3a32b199b21e40aee2ccf3e4900af7f9c116c` | `src/types-compat.h` |

## License (LICENSE.md of the release, full text)

````
## OpenLibm

OpenLibm contains code that is covered by various licenses.

The OpenLibm code derives from the FreeBSD msun and OpenBSD libm
implementations, which in turn derives from FDLIBM 5.3. As a result, it
has a number of fixes and updates that have accumulated over the years
in msun, and also optimized assembly versions of many functions. These
improvements are provided under the BSD and ISC licenses. The msun
library also includes work placed under the public domain, which is
noted in the individual files. Further work on making a standalone
OpenLibm library from msun, as part of the Julia project is covered
under the MIT license. The test files, test-double.c and test-float.c
are under the LGPL.

## Parts copyrighted by the Julia project (MIT License)

>       Copyright (c) 2011-14 The Julia Project.
>       https://github.com/JuliaMath/openlibm/graphs/contributors
>
>       Permission is hereby granted, free of charge, to any person obtaining
>       a copy of this software and associated documentation files (the
>       "Software"), to deal in the Software without restriction, including
>       without limitation the rights to use, copy, modify, merge, publish,
>       distribute, sublicense, and/or sell copies of the Software, and to
>       permit persons to whom the Software is furnished to do so, subject to
>       the following conditions:
>
>       The above copyright notice and this permission notice shall be
>       included in all copies or substantial portions of the Software.
>
>       THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
>       EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
>       MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
>       NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
>       LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
>       OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
>       WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## Parts copyrighted by Stephen L. Moshier (ISC License)

> Copyright (c) 2008 Stephen L. Moshier <steve@moshier.net>
>
> Permission to use, copy, modify, and distribute this software for any
> purpose with or without fee is hereby granted, provided that the above
> copyright notice and this permission notice appear in all copies.
>
> THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
> WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
> MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
> ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
> WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
> ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
> OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

## FREEBSD MSUN (FreeBSD/2-clause BSD/Simplified BSD License)

>       Copyright 1992-2011 The FreeBSD Project. All rights reserved.
>
>       Redistribution and use in source and binary forms, with or without
>       modification, are permitted provided that the following conditions are
>       met:
>
>       1. Redistributions of source code must retain the above copyright
>       notice, this list of conditions and the following disclaimer.
>
>       2. Redistributions in binary form must reproduce the above copyright
>       notice, this list of conditions and the following disclaimer in the
>       documentation and/or other materials provided with the distribution.
>       THIS SOFTWARE IS PROVIDED BY THE FREEBSD PROJECT ``AS IS'' AND ANY
>       EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
>       IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
>       PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE FREEBSD PROJECT OR
>       CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
>       EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
>       PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
>       PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
>       LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
>       NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
>       SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
>
>       The views and conclusions contained in the software and documentation
>       are those of the authors and should not be interpreted as representing
>       official policies, either expressed or implied, of the FreeBSD
>       Project.

## FDLIBM

>      Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
>
>      Developed at SunPro, a Sun Microsystems, Inc. business.
>      Permission to use, copy, modify, and distribute this
>      software is freely granted, provided that this notice
>      is preserved.

## Tests

>   Copyright (C) 1997, 1999 Free Software Foundation, Inc.
>   This file is part of the GNU C Library.
>   Contributed by Andreas Jaeger <aj@suse.de>, 1997.
>
>   The GNU C Library is free software; you can redistribute it and/or
>   modify it under the terms of the GNU Lesser General Public
>   License as published by the Free Software Foundation; either
>   version 2.1 of the License, or (at your option) any later version.
>
>   The GNU C Library is distributed in the hope that it will be useful,
>   but WITHOUT ANY WARRANTY; without even the implied warranty of
>   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
>   Lesser General Public License for more details.
>
>   You should have received a copy of the GNU Lesser General Public
>   License along with the GNU C Library; if not, write to the Free
>   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
>   02111-1307 USA.
````
