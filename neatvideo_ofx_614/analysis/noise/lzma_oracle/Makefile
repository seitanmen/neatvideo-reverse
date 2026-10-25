# C oracle of the genuine's LZMA call (FUN_009d61f4 -> LzmaCompress), built from the LZMA SDK (public domain).
# It does not load the genuine bundle.  lzo_920 is the reference; lzo_1604 is built for the version comparison.
SDK920 = sdk920/LzmaLib.c sdk920/LzmaEnc.c sdk920/LzmaDec.c sdk920/LzFind.c sdk920/Alloc.c
SDK1604 = sdk1604/LzmaLib.c sdk1604/LzmaEnc.c sdk1604/LzmaDec.c sdk1604/LzFind.c sdk1604/Alloc.c

all: lzo_920 lzo_1604

lzo_920: lzo.c $(SDK920)
	clang -O2 -w -D_7ZIP_ST -Isdk920 -o $@ lzo.c $(SDK920)

lzo_1604: lzo.c $(SDK1604)
	clang -O2 -w -D_7ZIP_ST -Isdk1604 -o $@ lzo.c $(SDK1604)
