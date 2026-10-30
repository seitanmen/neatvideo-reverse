"""LZMA SDK 9.20 encoder (LzmaEnc.c / LzFind.c, Igor Pavlov, public domain) as the genuine's FUN_009b41c4 /
FUN_009d61f4 run it, for the three LZMA sizes of the BuildNoiseModel profile (+0x18 / +0x1c / +0x20).

    plane_int16(plane)  -> bytes   FUN_009f0554: float32 -> int16 of rint(x * 16) (ties to even), saturated,
                                   NaN -> 0 (row major, no row padding: W * H * 2 bytes)
    compress(src)       -> (props 5 bytes, stream bytes)   LzmaCompress(level 5, dict = len rounded up to 4 KiB
                                   (max 128 MiB), lc 3, lp 0, pb 2, fb 32, 1 thread, no end mark)
    blob_size(src)      -> int     FUN_009d61f4: 16 + stream length, or -1 when the stream does not fit the
                                   output buffer (len + 0x80 bytes; SZ_ERROR_OUTPUT_EOF)
    blob(src)           -> bytes or None   the 16-byte header [len u32][stream len u32][props 5][3 bytes] + stream;
                                   the 3 bytes after the props are not written by the genuine (heap garbage), 0 here

Only the configuration the genuine uses is ported: binary-tree match finder with 4 hash bytes (Bt4, cut value 32),
the optimal parser (GetOptimum, algo 1), single thread, the whole input in memory (direct input) and a dictionary
at least as large as the input (so the cyclic buffer never wraps and the match finder never normalises).
Checked against the SDK 9.20 C code (analysis/noise/lzma_oracle) and the genuine's blobs (`nvh profile`
writes out.bin.lzma0/1/2); see analysis/noise/lzma_oracle/verify_lzma.py.  Observed: SDK 9.20 and 16.04 give the
genuine's streams, 19.00, 21.07, 23.01, 24.09 and 26.04 other ones (17.x / 18.x not tried).
Pure Python: about 10 us per input byte on noisy planes (M2 Max), less on very repetitive data."""
import numpy as np

F32 = np.float32

kNumBitModelTotalBits = 11
kBitModelTotal = 1 << kNumBitModelTotalBits
kNumMoveBits = 5
kProbInitValue = kBitModelTotal >> 1
kNumMoveReducingBits = 4
kNumBitPriceShiftBits = 4
kTopValue = 1 << 24
LZMA_NUM_REPS = 4
kNumOpts = 1 << 12
kNumLenToPosStates = 4
kNumPosSlotBits = 6
kNumAlignBits = 4
kAlignTableSize = 1 << kNumAlignBits
kAlignMask = kAlignTableSize - 1
kStartPosModelIndex = 4
kEndPosModelIndex = 14
kNumFullDistances = 1 << (kEndPosModelIndex >> 1)
kLenNumLowBits = 3
kLenNumLowSymbols = 1 << kLenNumLowBits
kLenNumMidBits = 3
kLenNumMidSymbols = 1 << kLenNumMidBits
kLenNumHighBits = 8
kLenNumHighSymbols = 1 << kLenNumHighBits
LZMA_MATCH_LEN_MIN = 2
LZMA_MATCH_LEN_MAX = LZMA_MATCH_LEN_MIN + kLenNumLowSymbols + kLenNumMidSymbols + kLenNumHighSymbols - 1
kNumStates = 12
kInfinityPrice = 1 << 30
kNumLogBits = 9 + 8 // 2                                         # 9 + sizeof(size_t) / 2 on the 64-bit genuine
kDicLogSizeMaxCompress = (kNumLogBits - 1) * 2 + 7
M32 = 0xFFFFFFFF

kLiteralNextStates = (0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 4, 5)
kMatchNextStates = (7, 7, 7, 7, 7, 7, 7, 10, 10, 10, 10, 10)
kRepNextStates = (8, 8, 8, 8, 8, 8, 8, 11, 11, 11, 11, 11)
kShortRepNextStates = (9, 9, 9, 9, 9, 9, 9, 11, 11, 11, 11, 11)


def _prob_prices():
    """LzmaEnc_InitPriceTables."""
    out = [0] * (kBitModelTotal >> kNumMoveReducingBits)
    for i in range((1 << kNumMoveReducingBits) // 2, kBitModelTotal, 1 << kNumMoveReducingBits):
        w, bit_count = i, 0
        for _ in range(kNumBitPriceShiftBits):
            w = w * w
            bit_count <<= 1
            while w >= 1 << 16:
                w >>= 1
                bit_count += 1
        out[i >> kNumMoveReducingBits] = (kNumBitModelTotalBits << kNumBitPriceShiftBits) - 15 - bit_count
    return out


def _fast_pos():
    """LzmaEnc_FastPosInit."""
    g = [0] * (1 << kNumLogBits)
    g[1] = 1
    c = 2
    for slot in range(2, kNumLogBits * 2):
        for _ in range(1 << ((slot >> 1) - 1)):
            g[c] = slot
            c += 1
    return g


def _crc_table():
    """MatchFinder_Construct."""
    t = []
    for i in range(256):
        r = i
        for _ in range(8):
            r = (r >> 1) ^ (0xEDB88320 & ~((r & 1) - 1) & M32)
        t.append(r)
    return t


PROB_PRICES = _prob_prices()
FAST_POS = _fast_pos()
CRC = _crc_table()


def _pos_slot2(pos):
    """GetPosSlot2 (BSR2_RET with g_FastPos)."""
    i = 6 + ((kNumLogBits - 1) & ((0 - (((((1 << (kNumLogBits + 6)) - 1) - pos) & M32) >> 31)) & M32))
    return FAST_POS[pos >> i] + i * 2


def _pos_slot(pos):
    return FAST_POS[pos] if pos < kNumFullDistances else _pos_slot2(pos)


def _len_to_pos_state(ln):
    return ln - 2 if ln < kNumLenToPosStates + 1 else kNumLenToPosStates - 1


def dict_size_for(n):
    """FUN_009d61f4: (n + 0xfff) & ~0xfff, or 128 MiB when n + 0xfff >= 2^27."""
    d = (n + 0xfff) & 0xfffff000
    if (n + 0xfff) >> 0x1b:
        d = 0x8000000
    return d


def props_bytes(dict_size, lc=3, lp=0, pb=2):
    """LzmaEnc_WriteProperties."""
    d = dict_size
    for i in range(11, 31):
        if d <= 2 << i:
            d = 2 << i
            break
        if d <= 3 << i:
            d = 3 << i
            break
    return bytes([(pb * 5 + lp) * 9 + lc]) + (d & M32).to_bytes(4, "little")


class _Encoder:
    def __init__(self, src, dict_size, lc=3, lp=0, pb=2, fb=32, mc=None):
        n = len(src)
        if n > dict_size:
            raise NotImplementedError("input larger than the dictionary (cyclic buffer wrap is not ported)")
        self.buf = src
        self.n = n
        self.lc, self.lp, self.pb = lc, lp, pb
        self.lpMask = (1 << lp) - 1
        self.pbMask = (1 << pb) - 1
        self.numFastBytes = max(5, min(fb, LZMA_MATCH_LEN_MAX))
        self.cutValue = mc if mc is not None else (16 + (fb >> 1))  # btMode: >> 0
        # match finder (MatchFinder_Create / MatchFinder_Init with direct input)
        hs = dict_size - 1
        hs |= hs >> 1
        hs |= hs >> 2
        hs |= hs >> 4
        hs |= hs >> 8
        hs >>= 1
        hs |= 0xFFFF
        if hs > 1 << 24:
            hs >>= 1
        self.hashMask = hs
        self.hash2 = [0] * (1 << 10)
        self.hash3 = [0] * (1 << 16)
        self.hash4 = [0] * (hs + 1)
        self.cbs = dict_size + 1                                # cyclicBufferSize
        self.son = [0] * (2 * (n + 1))
        self.mfi = 0                                            # buffer index = cyclicBufferPos; pos = cbs + mfi
        self.matchMaxLen = self.numFastBytes
        # encoder state (LzmaEnc_Init)
        i = 0
        while i < kDicLogSizeMaxCompress and dict_size > (1 << i):
            i += 1
        self.distTableSize = i * 2
        self.state = 0
        self.reps = [0, 0, 0, 0]
        self.isMatch = [[kProbInitValue] * 16 for _ in range(kNumStates)]
        self.isRep0Long = [[kProbInitValue] * 16 for _ in range(kNumStates)]
        self.isRep = [kProbInitValue] * kNumStates
        self.isRepG0 = [kProbInitValue] * kNumStates
        self.isRepG1 = [kProbInitValue] * kNumStates
        self.isRepG2 = [kProbInitValue] * kNumStates
        self.litProbs = [kProbInitValue] * (0x300 << (lc + lp))
        self.posSlotEncoder = [[kProbInitValue] * (1 << kNumPosSlotBits) for _ in range(kNumLenToPosStates)]
        self.posEncoders = [kProbInitValue] * (kNumFullDistances - kEndPosModelIndex)
        self.posAlignEncoder = [kProbInitValue] * (1 << kNumAlignBits)
        self.lenEnc = _LenPriceEnc()
        self.repLenEnc = _LenPriceEnc()
        self.optimumEndIndex = 0
        self.optimumCurrentIndex = 0
        self.additionalOffset = 0
        self.longestMatchLength = 0
        self.numPairs = 0
        self.numAvail = 0
        self.matches = [0] * (LZMA_MATCH_LEN_MAX * 2 + 2 + 1)
        self.posSlotPrices = [[0] * (kDicLogSizeMaxCompress * 2 + 2) for _ in range(kNumLenToPosStates)]
        self.distancesPrices = [[0] * kNumFullDistances for _ in range(kNumLenToPosStates)]
        self.alignPrices = [0] * kAlignTableSize
        self.alignPriceCount = 0
        self.matchPriceCount = 0
        # COptimal opt[kNumOpts] as parallel lists
        self.o_price = [0] * kNumOpts
        self.o_state = [0] * kNumOpts
        self.o_prev1IsChar = [False] * kNumOpts
        self.o_prev2 = [False] * kNumOpts
        self.o_posPrev2 = [0] * kNumOpts
        self.o_backPrev2 = [0] * kNumOpts
        self.o_posPrev = [0] * kNumOpts
        self.o_backPrev = [0] * kNumOpts
        self.o_backs = [[0, 0, 0, 0] for _ in range(kNumOpts)]
        # range encoder (RangeEnc_Init)
        self.low = 0
        self.range = M32
        self.cacheSize = 1
        self.cache = 0
        self.out = bytearray()
        # LzmaEnc_InitPrices
        self.fill_distances_prices()
        self.fill_align_prices()
        self.lenEnc.tableSize = self.repLenEnc.tableSize = self.numFastBytes + 1 - LZMA_MATCH_LEN_MIN
        for ps in range(1 << pb):
            self.lenEnc.update_table(ps)
            self.repLenEnc.update_table(ps)

    # ---- range encoder
    def shift_low(self):
        low = self.low
        if (low & M32) < 0xFF000000 or (low >> 32) != 0:
            temp = self.cache
            out = self.out
            while True:
                out.append((temp + (low >> 32)) & 0xFF)
                temp = 0xFF
                self.cacheSize -= 1
                if self.cacheSize == 0:
                    break
            self.cache = (low & M32) >> 24
        self.cacheSize += 1
        self.low = (low & 0x00FFFFFF) << 8

    def encode_bit(self, probs, idx, bit):
        ttt = probs[idx]
        bound = (self.range >> kNumBitModelTotalBits) * ttt
        if bit == 0:
            self.range = bound
            probs[idx] = ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)
        else:
            self.low += bound
            self.range -= bound
            probs[idx] = ttt - (ttt >> kNumMoveBits)
        if self.range < kTopValue:
            self.range = (self.range << 8) & M32
            self.shift_low()

    def encode_direct_bits(self, value, num_bits):
        while num_bits:
            num_bits -= 1
            self.range >>= 1
            if (value >> num_bits) & 1:
                self.low += self.range
            if self.range < kTopValue:
                self.range = (self.range << 8) & M32
                self.shift_low()

    def lit_encode(self, base, symbol):
        probs = self.litProbs
        symbol |= 0x100
        while True:
            self.encode_bit(probs, base + (symbol >> 8), (symbol >> 7) & 1)
            symbol <<= 1
            if symbol >= 0x10000:
                break

    def lit_encode_matched(self, base, symbol, match_byte):
        probs = self.litProbs
        offs = 0x100
        symbol |= 0x100
        while True:
            match_byte <<= 1
            self.encode_bit(probs, base + offs + (match_byte & offs) + (symbol >> 8), (symbol >> 7) & 1)
            symbol <<= 1
            offs &= ~(match_byte ^ symbol)
            if symbol >= 0x10000:
                break

    def rc_tree_encode(self, probs, num_bits, symbol):
        m = 1
        for i in range(num_bits - 1, -1, -1):
            bit = (symbol >> i) & 1
            self.encode_bit(probs, m, bit)
            m = (m << 1) | bit

    def rc_tree_reverse_encode(self, probs, base, num_bits, symbol):
        m = 1
        for _ in range(num_bits):
            bit = symbol & 1
            self.encode_bit(probs, base + m, bit)
            m = (m << 1) | bit
            symbol >>= 1

    def len_encode(self, le, symbol, pos_state):
        """LenEnc_Encode2 (updatePrice: the optimal parser)."""
        p = le.probs
        if symbol < kLenNumLowSymbols:
            self.encode_bit(p, 0, 0)
            self._tree(p, 2 + (pos_state << kLenNumLowBits), kLenNumLowBits, symbol)
        else:
            self.encode_bit(p, 0, 1)
            if symbol < kLenNumLowSymbols + kLenNumMidSymbols:
                self.encode_bit(p, 1, 0)
                self._tree(p, 2 + 128 + (pos_state << kLenNumMidBits), kLenNumMidBits, symbol - kLenNumLowSymbols)
            else:
                self.encode_bit(p, 1, 1)
                self._tree(p, 2 + 256, kLenNumHighBits, symbol - kLenNumLowSymbols - kLenNumMidSymbols)
        le.counters[pos_state] -= 1
        if le.counters[pos_state] == 0:
            le.update_table(pos_state)

    def _tree(self, probs, base, num_bits, symbol):
        m = 1
        for i in range(num_bits - 1, -1, -1):
            bit = (symbol >> i) & 1
            self.encode_bit(probs, base + m, bit)
            m = (m << 1) | bit

    def flush_data(self):
        for _ in range(5):
            self.shift_low()

    # ---- prices
    def fill_align_prices(self):
        pp = PROB_PRICES
        probs = self.posAlignEncoder
        for i in range(kAlignTableSize):
            self.alignPrices[i] = _rc_tree_reverse_price(probs, 0, kNumAlignBits, i, pp)
        self.alignPriceCount = 0

    def fill_distances_prices(self):
        pp = PROB_PRICES
        temp = [0] * kNumFullDistances
        for i in range(kStartPosModelIndex, kNumFullDistances):
            slot = FAST_POS[i]
            footer = (slot >> 1) - 1
            base = (2 | (slot & 1)) << footer
            temp[i] = _rc_tree_reverse_price(self.posEncoders, base - slot - 1, footer, i - base, pp)
        for lps in range(kNumLenToPosStates):
            enc = self.posSlotEncoder[lps]
            psp = self.posSlotPrices[lps]
            for slot in range(self.distTableSize):
                psp[slot] = _rc_tree_price(enc, kNumPosSlotBits, slot, pp)
            for slot in range(kEndPosModelIndex, self.distTableSize):
                psp[slot] += (((slot >> 1) - 1) - kNumAlignBits) << kNumBitPriceShiftBits
            dp = self.distancesPrices[lps]
            for i in range(kStartPosModelIndex):
                dp[i] = psp[i]
            for i in range(kStartPosModelIndex, kNumFullDistances):
                dp[i] = psp[FAST_POS[i]] + temp[i]
        self.matchPriceCount = 0

    def lit_price(self, base, symbol):
        probs, pp = self.litProbs, PROB_PRICES
        price = 0
        symbol |= 0x100
        while symbol < 0x10000:
            prob = probs[base + (symbol >> 8)]
            price += pp[(prob ^ (2047 if (symbol >> 7) & 1 else 0)) >> 4]
            symbol <<= 1
        return price

    def lit_price_matched(self, base, symbol, match_byte):
        probs, pp = self.litProbs, PROB_PRICES
        price = 0
        offs = 0x100
        symbol |= 0x100
        while symbol < 0x10000:
            match_byte <<= 1
            prob = probs[base + offs + (match_byte & offs) + (symbol >> 8)]
            price += pp[(prob ^ (2047 if (symbol >> 7) & 1 else 0)) >> 4]
            symbol <<= 1
            offs &= ~(match_byte ^ symbol)
        return price

    def lit_base(self, position, prev_byte):
        """LIT_PROBS."""
        return (((position & self.lpMask) << self.lc) + (prev_byte >> (8 - self.lc))) * 0x300

    def rep_len1_price(self, state, pos_state):
        pp = PROB_PRICES
        return pp[self.isRepG0[state] >> 4] + pp[self.isRep0Long[state][pos_state] >> 4]

    def pure_rep_price(self, rep_index, state, pos_state):
        pp = PROB_PRICES
        if rep_index == 0:
            return pp[self.isRepG0[state] >> 4] + pp[(self.isRep0Long[state][pos_state] ^ 2047) >> 4]
        price = pp[(self.isRepG0[state] ^ 2047) >> 4]
        if rep_index == 1:
            return price + pp[self.isRepG1[state] >> 4]
        price += pp[(self.isRepG1[state] ^ 2047) >> 4]
        return price + pp[(self.isRepG2[state] ^ (2047 if rep_index - 2 else 0)) >> 4]

    def rep_price(self, rep_index, ln, state, pos_state):
        return self.repLenEnc.prices[pos_state][ln - LZMA_MATCH_LEN_MIN] + self.pure_rep_price(rep_index, state,
                                                                                              pos_state)

    # ---- match finder (Bt4)
    def _len_limit(self):
        a = self.n - self.mfi
        return a if a < self.matchMaxLen else self.matchMaxLen

    def get_matches(self, d):
        """Bt4_MatchFinder_GetMatches: writes (len, dist - 1) pairs to d, returns their count * 2."""
        i = self.mfi
        lenLimit = self._len_limit()
        if lenLimit < 4:
            self.mfi = i + 1
            return 0
        buf = self.buf
        cbs = self.cbs
        pos = cbs + i
        c0 = buf[i]
        temp = CRC[c0] ^ buf[i + 1]
        h2 = temp & 1023
        t3 = temp ^ (buf[i + 2] << 8)
        h3 = t3 & 0xFFFF
        hv = (t3 ^ (CRC[buf[i + 3]] << 5)) & self.hashMask
        delta2 = pos - self.hash2[h2]
        delta3 = pos - self.hash3[h3]
        curMatch = self.hash4[hv]
        self.hash2[h2] = self.hash3[h3] = self.hash4[hv] = pos
        maxLen = 1
        off = 0
        if delta2 < cbs and buf[i - delta2] == c0:
            d[0] = maxLen = 2
            d[1] = delta2 - 1
            off = 2
        if delta2 != delta3 and delta3 < cbs and buf[i - delta3] == c0:
            maxLen = 3
            d[off + 1] = delta3 - 1
            off += 2
            delta2 = delta3
        if off != 0:
            while maxLen != lenLimit and buf[i + maxLen - delta2] == buf[i + maxLen]:
                maxLen += 1
            d[off - 2] = maxLen
            if maxLen == lenLimit:
                self._skip_spec(lenLimit, curMatch, pos, i)
                self.mfi = i + 1
                return off
        if maxLen < 3:
            maxLen = 3
        off = self._get_matches_spec1(lenLimit, curMatch, pos, i, d, off, maxLen)
        self.mfi = i + 1
        return off

    def _get_matches_spec1(self, lenLimit, curMatch, pos, i, d, off, maxLen):
        """GetMatchesSpec1."""
        buf, son, cbs = self.buf, self.son, self.cbs
        cut = self.cutValue
        ptr0 = (i << 1) + 1
        ptr1 = i << 1
        len0 = len1 = 0
        while True:
            delta = pos - curMatch
            if cut == 0 or delta >= cbs:
                son[ptr0] = son[ptr1] = 0
                return off
            cut -= 1
            pb = i - delta
            pair = pb << 1
            ln = len0 if len0 < len1 else len1
            if buf[pb + ln] == buf[i + ln]:
                ln += 1
                if ln != lenLimit and buf[pb + ln] == buf[i + ln]:
                    ln += 1
                    while ln != lenLimit and buf[pb + ln] == buf[i + ln]:
                        ln += 1
                if maxLen < ln:
                    maxLen = ln
                    d[off] = ln
                    d[off + 1] = delta - 1
                    off += 2
                    if ln == lenLimit:
                        son[ptr1] = son[pair]
                        son[ptr0] = son[pair + 1]
                        return off
            if buf[pb + ln] < buf[i + ln]:
                son[ptr1] = curMatch
                ptr1 = pair + 1
                curMatch = son[ptr1]
                len1 = ln
            else:
                son[ptr0] = curMatch
                ptr0 = pair
                curMatch = son[ptr0]
                len0 = ln

    def _skip_spec(self, lenLimit, curMatch, pos, i):
        """SkipMatchesSpec."""
        buf, son, cbs = self.buf, self.son, self.cbs
        cut = self.cutValue
        ptr0 = (i << 1) + 1
        ptr1 = i << 1
        len0 = len1 = 0
        while True:
            delta = pos - curMatch
            if cut == 0 or delta >= cbs:
                son[ptr0] = son[ptr1] = 0
                return
            cut -= 1
            pb = i - delta
            pair = pb << 1
            ln = len0 if len0 < len1 else len1
            if buf[pb + ln] == buf[i + ln]:
                ln += 1
                while ln != lenLimit and buf[pb + ln] == buf[i + ln]:
                    ln += 1
                if ln == lenLimit:
                    son[ptr1] = son[pair]
                    son[ptr0] = son[pair + 1]
                    return
            if buf[pb + ln] < buf[i + ln]:
                son[ptr1] = curMatch
                ptr1 = pair + 1
                curMatch = son[ptr1]
                len1 = ln
            else:
                son[ptr0] = curMatch
                ptr0 = pair
                curMatch = son[ptr0]
                len0 = ln

    def skip(self, num):
        """Bt4_MatchFinder_Skip."""
        buf = self.buf
        while True:
            i = self.mfi
            lenLimit = self._len_limit()
            if lenLimit >= 4:
                pos = self.cbs + i
                temp = CRC[buf[i]] ^ buf[i + 1]
                t3 = temp ^ (buf[i + 2] << 8)
                hv = (t3 ^ (CRC[buf[i + 3]] << 5)) & self.hashMask
                curMatch = self.hash4[hv]
                self.hash2[temp & 1023] = self.hash3[t3 & 0xFFFF] = pos
                self.hash4[hv] = pos
                self._skip_spec(lenLimit, curMatch, pos, i)
            self.mfi = i + 1
            num -= 1
            if num == 0:
                return

    def move_pos(self, num):
        if num != 0:
            self.additionalOffset += num
            self.skip(num)

    def read_match_distances(self):
        """ReadMatchDistances -> (lenRes, numPairs)."""
        self.numAvail = self.n - self.mfi
        d = self.matches
        num_pairs = self.get_matches(d)
        len_res = 0
        if num_pairs > 0:
            len_res = d[num_pairs - 2]
            if len_res == self.numFastBytes:
                pby = self.mfi - 1
                distance = d[num_pairs - 1] + 1
                num_avail = min(self.numAvail, LZMA_MATCH_LEN_MAX)
                buf = self.buf
                while len_res < num_avail and buf[pby + len_res] == buf[pby - distance + len_res]:
                    len_res += 1
        self.additionalOffset += 1
        return len_res, num_pairs

    # ---- optimal parser
    def backward(self, cur):
        """Backward -> (len, back)."""
        o_posPrev, o_backPrev = self.o_posPrev, self.o_backPrev
        o_prev1IsChar = self.o_prev1IsChar
        posMem = o_posPrev[cur]
        backMem = o_backPrev[cur]
        self.optimumEndIndex = cur
        while True:
            if o_prev1IsChar[cur]:
                o_backPrev[posMem] = M32
                o_prev1IsChar[posMem] = False
                o_posPrev[posMem] = posMem - 1
                if self.o_prev2[cur]:
                    o_prev1IsChar[posMem - 1] = False
                    o_posPrev[posMem - 1] = self.o_posPrev2[cur]
                    o_backPrev[posMem - 1] = self.o_backPrev2[cur]
            posPrev = posMem
            backCur = backMem
            backMem = o_backPrev[posPrev]
            posMem = o_posPrev[posPrev]
            o_backPrev[posPrev] = backCur
            o_posPrev[posPrev] = cur
            cur = posPrev
            if cur == 0:
                break
        self.optimumCurrentIndex = o_posPrev[0]
        return self.optimumCurrentIndex, o_backPrev[0]

    def get_optimum(self, position):
        """GetOptimum -> (len, back)."""
        o_price, o_posPrev, o_backPrev = self.o_price, self.o_posPrev, self.o_backPrev
        o_prev1IsChar, o_prev2 = self.o_prev1IsChar, self.o_prev2
        o_state, o_backs = self.o_state, self.o_backs
        pp = PROB_PRICES
        buf = self.buf
        if self.optimumEndIndex != self.optimumCurrentIndex:
            c = self.optimumCurrentIndex
            len_res = o_posPrev[c] - c
            back = o_backPrev[c]
            self.optimumCurrentIndex = o_posPrev[c]
            return len_res, back
        self.optimumCurrentIndex = self.optimumEndIndex = 0
        if self.additionalOffset == 0:
            mainLen, numPairs = self.read_match_distances()
        else:
            mainLen = self.longestMatchLength
            numPairs = self.numPairs
        numAvail = self.numAvail
        if numAvail < 2:
            return 1, M32
        if numAvail > LZMA_MATCH_LEN_MAX:
            numAvail = LZMA_MATCH_LEN_MAX
        data = self.mfi - 1
        reps = list(self.reps)
        repLens = [0, 0, 0, 0]
        repMaxIndex = 0
        for i in range(LZMA_NUM_REPS):
            d2 = data - (reps[i] + 1)
            if buf[data] != buf[d2] or buf[data + 1] != buf[d2 + 1]:
                repLens[i] = 0
                continue
            lenTest = 2
            while lenTest < numAvail and buf[data + lenTest] == buf[d2 + lenTest]:
                lenTest += 1
            repLens[i] = lenTest
            if lenTest > repLens[repMaxIndex]:
                repMaxIndex = i
        if repLens[repMaxIndex] >= self.numFastBytes:
            len_res = repLens[repMaxIndex]
            self.move_pos(len_res - 1)
            return len_res, repMaxIndex
        matches = self.matches
        if mainLen >= self.numFastBytes:
            back = matches[numPairs - 1] + LZMA_NUM_REPS
            self.move_pos(mainLen - 1)
            return mainLen, back
        curByte = buf[data]
        matchByte = buf[data - (reps[0] + 1)]
        if mainLen < 2 and curByte != matchByte and repLens[repMaxIndex] < 2:
            return 1, M32
        state = self.state
        o_state[0] = state
        posState = position & self.pbMask
        isMatch_s = self.isMatch[state]
        base = self.lit_base(position, buf[data - 1])
        o_price[1] = pp[isMatch_s[posState] >> 4] + (
            self.lit_price_matched(base, curByte, matchByte) if state >= 7 else self.lit_price(base, curByte))
        o_backPrev[1] = M32
        o_prev1IsChar[1] = False
        matchPrice = pp[(isMatch_s[posState] ^ 2047) >> 4]
        repMatchPrice = matchPrice + pp[(self.isRep[state] ^ 2047) >> 4]
        if matchByte == curByte:
            shortRepPrice = repMatchPrice + self.rep_len1_price(state, posState)
            if shortRepPrice < o_price[1]:
                o_price[1] = shortRepPrice
                o_backPrev[1] = 0
                o_prev1IsChar[1] = False
        lenEnd = mainLen if mainLen >= repLens[repMaxIndex] else repLens[repMaxIndex]
        if lenEnd < 2:
            return 1, o_backPrev[1]
        o_posPrev[1] = 0
        o_backs[0][:] = reps
        ln = lenEnd
        while True:
            o_price[ln] = kInfinityPrice
            ln -= 1
            if ln < 2:
                break
        repPrices = self.repLenEnc.prices[posState]
        for i in range(LZMA_NUM_REPS):
            repLen = repLens[i]
            if repLen < 2:
                continue
            price = repMatchPrice + self.pure_rep_price(i, state, posState)
            while True:
                curAndLenPrice = price + repPrices[repLen - 2]
                if curAndLenPrice < o_price[repLen]:
                    o_price[repLen] = curAndLenPrice
                    o_posPrev[repLen] = 0
                    o_backPrev[repLen] = i
                    o_prev1IsChar[repLen] = False
                repLen -= 1
                if repLen < 2:
                    break
        normalMatchPrice = matchPrice + pp[self.isRep[state] >> 4]
        ln = repLens[0] + 1 if repLens[0] >= 2 else 2
        if ln <= mainLen:
            lenPrices = self.lenEnc.prices[posState]
            offs = 0
            while ln > matches[offs]:
                offs += 2
            while True:
                distance = matches[offs + 1]
                curAndLenPrice = normalMatchPrice + lenPrices[ln - LZMA_MATCH_LEN_MIN]
                lps = _len_to_pos_state(ln)
                if distance < kNumFullDistances:
                    curAndLenPrice += self.distancesPrices[lps][distance]
                else:
                    curAndLenPrice += self.alignPrices[distance & kAlignMask] + \
                        self.posSlotPrices[lps][_pos_slot2(distance)]
                if curAndLenPrice < o_price[ln]:
                    o_price[ln] = curAndLenPrice
                    o_posPrev[ln] = 0
                    o_backPrev[ln] = distance + LZMA_NUM_REPS
                    o_prev1IsChar[ln] = False
                if ln == matches[offs]:
                    offs += 2
                    if offs == numPairs:
                        break
                ln += 1

        cur = 0
        numFastBytes = self.numFastBytes
        while True:
            cur += 1
            if cur == lenEnd:
                return self.backward(cur)
            newLen, numPairs = self.read_match_distances()
            if newLen >= numFastBytes:
                self.numPairs = numPairs
                self.longestMatchLength = newLen
                return self.backward(cur)
            position += 1
            posPrev = o_posPrev[cur]
            if o_prev1IsChar[cur]:
                posPrev -= 1
                if o_prev2[cur]:
                    state = o_state[self.o_posPrev2[cur]]
                    if self.o_backPrev2[cur] < LZMA_NUM_REPS:
                        state = kRepNextStates[state]
                    else:
                        state = kMatchNextStates[state]
                else:
                    state = o_state[posPrev]
                state = kLiteralNextStates[state]
            else:
                state = o_state[posPrev]
            if posPrev == cur - 1:
                if o_backPrev[cur] == 0:                         # IsShortRep
                    state = kShortRepNextStates[state]
                else:
                    state = kLiteralNextStates[state]
            else:
                if o_prev1IsChar[cur] and o_prev2[cur]:
                    posPrev = self.o_posPrev2[cur]
                    pos = self.o_backPrev2[cur]
                    state = kRepNextStates[state]
                else:
                    pos = o_backPrev[cur]
                    if pos < LZMA_NUM_REPS:
                        state = kRepNextStates[state]
                    else:
                        state = kMatchNextStates[state]
                pb_ = o_backs[posPrev]
                if pos < LZMA_NUM_REPS:
                    reps[0] = pb_[pos]
                    i = 1
                    while i <= pos:
                        reps[i] = pb_[i - 1]
                        i += 1
                    while i < LZMA_NUM_REPS:
                        reps[i] = pb_[i]
                        i += 1
                else:
                    reps[0] = pos - LZMA_NUM_REPS
                    reps[1] = pb_[0]
                    reps[2] = pb_[1]
                    reps[3] = pb_[2]
            o_state[cur] = state
            o_backs[cur][:] = reps
            curPrice = o_price[cur]
            nextIsChar = False
            data = self.mfi - 1
            curByte = buf[data]
            matchByte = buf[data - (reps[0] + 1)]
            posState = position & self.pbMask
            isMatch_s = self.isMatch[state]
            curAnd1Price = curPrice + pp[isMatch_s[posState] >> 4]
            base = self.lit_base(position, buf[data - 1])
            curAnd1Price += self.lit_price_matched(base, curByte, matchByte) if state >= 7 else \
                self.lit_price(base, curByte)
            nxt = cur + 1
            if curAnd1Price < o_price[nxt]:
                o_price[nxt] = curAnd1Price
                o_posPrev[nxt] = cur
                o_backPrev[nxt] = M32
                o_prev1IsChar[nxt] = False
                nextIsChar = True
            matchPrice = curPrice + pp[(isMatch_s[posState] ^ 2047) >> 4]
            repMatchPrice = matchPrice + pp[(self.isRep[state] ^ 2047) >> 4]
            if matchByte == curByte and not (o_posPrev[nxt] < cur and o_backPrev[nxt] == 0):
                shortRepPrice = repMatchPrice + self.rep_len1_price(state, posState)
                if shortRepPrice <= o_price[nxt]:
                    o_price[nxt] = shortRepPrice
                    o_posPrev[nxt] = cur
                    o_backPrev[nxt] = 0
                    o_prev1IsChar[nxt] = False
                    nextIsChar = True
            numAvailFull = self.numAvail
            temp = kNumOpts - 1 - cur
            if temp < numAvailFull:
                numAvailFull = temp
            if numAvailFull < 2:
                continue
            numAvail = numAvailFull if numAvailFull <= numFastBytes else numFastBytes

            if not nextIsChar and matchByte != curByte:
                # try Literal + rep0
                d2 = data - (reps[0] + 1)
                limit = numFastBytes + 1
                if limit > numAvailFull:
                    limit = numAvailFull
                temp = 1
                while temp < limit and buf[data + temp] == buf[d2 + temp]:
                    temp += 1
                lenTest2 = temp - 1
                if lenTest2 >= 2:
                    state2 = kLiteralNextStates[state]
                    posStateNext = (position + 1) & self.pbMask
                    nextRepMatchPrice = curAnd1Price + pp[(self.isMatch[state2][posStateNext] ^ 2047) >> 4] + \
                        pp[(self.isRep[state2] ^ 2047) >> 4]
                    offset = cur + 1 + lenTest2
                    while lenEnd < offset:
                        lenEnd += 1
                        o_price[lenEnd] = kInfinityPrice
                    curAndLenPrice = nextRepMatchPrice + self.rep_price(0, lenTest2, state2, posStateNext)
                    if curAndLenPrice < o_price[offset]:
                        o_price[offset] = curAndLenPrice
                        o_posPrev[offset] = cur + 1
                        o_backPrev[offset] = 0
                        o_prev1IsChar[offset] = True
                        o_prev2[offset] = False

            startLen = 2
            repPrices = self.repLenEnc.prices[posState]
            for repIndex in range(LZMA_NUM_REPS):
                d2 = data - (reps[repIndex] + 1)
                if buf[data] != buf[d2] or buf[data + 1] != buf[d2 + 1]:
                    continue
                lenTest = 2
                while lenTest < numAvail and buf[data + lenTest] == buf[d2 + lenTest]:
                    lenTest += 1
                while lenEnd < cur + lenTest:
                    lenEnd += 1
                    o_price[lenEnd] = kInfinityPrice
                lenTestTemp = lenTest
                price = repMatchPrice + self.pure_rep_price(repIndex, state, posState)
                while True:
                    curAndLenPrice = price + repPrices[lenTest - 2]
                    k = cur + lenTest
                    if curAndLenPrice < o_price[k]:
                        o_price[k] = curAndLenPrice
                        o_posPrev[k] = cur
                        o_backPrev[k] = repIndex
                        o_prev1IsChar[k] = False
                    lenTest -= 1
                    if lenTest < 2:
                        break
                lenTest = lenTestTemp
                if repIndex == 0:
                    startLen = lenTest + 1
                lenTest2 = lenTest + 1
                limit = lenTest2 + numFastBytes
                if limit > numAvailFull:
                    limit = numAvailFull
                while lenTest2 < limit and buf[data + lenTest2] == buf[d2 + lenTest2]:
                    lenTest2 += 1
                lenTest2 -= lenTest + 1
                if lenTest2 >= 2:
                    state2 = kRepNextStates[state]
                    posStateNext = (position + lenTest) & self.pbMask
                    curAndLenCharPrice = price + repPrices[lenTest - 2] + \
                        pp[self.isMatch[state2][posStateNext] >> 4] + \
                        self.lit_price_matched(self.lit_base(position + lenTest, buf[data + lenTest - 1]),
                                               buf[data + lenTest], buf[d2 + lenTest])
                    state2 = kLiteralNextStates[state2]
                    posStateNext = (position + lenTest + 1) & self.pbMask
                    nextRepMatchPrice = curAndLenCharPrice + pp[(self.isMatch[state2][posStateNext] ^ 2047) >> 4] + \
                        pp[(self.isRep[state2] ^ 2047) >> 4]
                    offset = cur + lenTest + 1 + lenTest2
                    while lenEnd < offset:
                        lenEnd += 1
                        o_price[lenEnd] = kInfinityPrice
                    curAndLenPrice = nextRepMatchPrice + self.rep_price(0, lenTest2, state2, posStateNext)
                    if curAndLenPrice < o_price[offset]:
                        o_price[offset] = curAndLenPrice
                        o_posPrev[offset] = cur + lenTest + 1
                        o_backPrev[offset] = 0
                        o_prev1IsChar[offset] = True
                        o_prev2[offset] = True
                        self.o_posPrev2[offset] = cur
                        self.o_backPrev2[offset] = repIndex

            if newLen > numAvail:
                newLen = numAvail
                numPairs = 0
                while newLen > matches[numPairs]:
                    numPairs += 2
                matches[numPairs] = newLen
                numPairs += 2
            if newLen >= startLen:
                normalMatchPrice = matchPrice + pp[self.isRep[state] >> 4]
                lenPrices = self.lenEnc.prices[posState]
                while lenEnd < cur + newLen:
                    lenEnd += 1
                    o_price[lenEnd] = kInfinityPrice
                offs = 0
                while startLen > matches[offs]:
                    offs += 2
                curBack = matches[offs + 1]
                posSlot = _pos_slot2(curBack)
                lenTest = startLen
                while True:
                    curAndLenPrice = normalMatchPrice + lenPrices[lenTest - LZMA_MATCH_LEN_MIN]
                    lps = _len_to_pos_state(lenTest)
                    if curBack < kNumFullDistances:
                        curAndLenPrice += self.distancesPrices[lps][curBack]
                    else:
                        curAndLenPrice += self.posSlotPrices[lps][posSlot] + self.alignPrices[curBack & kAlignMask]
                    k = cur + lenTest
                    if curAndLenPrice < o_price[k]:
                        o_price[k] = curAndLenPrice
                        o_posPrev[k] = cur
                        o_backPrev[k] = curBack + LZMA_NUM_REPS
                        o_prev1IsChar[k] = False
                    if lenTest == matches[offs]:
                        # try Match + Literal + Rep0
                        d2 = data - (curBack + 1)
                        lenTest2 = lenTest + 1
                        limit = lenTest2 + numFastBytes
                        if limit > numAvailFull:
                            limit = numAvailFull
                        while lenTest2 < limit and buf[data + lenTest2] == buf[d2 + lenTest2]:
                            lenTest2 += 1
                        lenTest2 -= lenTest + 1
                        if lenTest2 >= 2:
                            state2 = kMatchNextStates[state]
                            posStateNext = (position + lenTest) & self.pbMask
                            curAndLenCharPrice = curAndLenPrice + pp[self.isMatch[state2][posStateNext] >> 4] + \
                                self.lit_price_matched(self.lit_base(position + lenTest, buf[data + lenTest - 1]),
                                                       buf[data + lenTest], buf[d2 + lenTest])
                            state2 = kLiteralNextStates[state2]
                            posStateNext = (posStateNext + 1) & self.pbMask
                            nextRepMatchPrice = curAndLenCharPrice + \
                                pp[(self.isMatch[state2][posStateNext] ^ 2047) >> 4] + \
                                pp[(self.isRep[state2] ^ 2047) >> 4]
                            offset = cur + lenTest + 1 + lenTest2
                            while lenEnd < offset:
                                lenEnd += 1
                                o_price[lenEnd] = kInfinityPrice
                            curAndLenPrice = nextRepMatchPrice + self.rep_price(0, lenTest2, state2, posStateNext)
                            if curAndLenPrice < o_price[offset]:
                                o_price[offset] = curAndLenPrice
                                o_posPrev[offset] = cur + lenTest + 1
                                o_backPrev[offset] = 0
                                o_prev1IsChar[offset] = True
                                o_prev2[offset] = True
                                self.o_posPrev2[offset] = cur
                                self.o_backPrev2[offset] = curBack + LZMA_NUM_REPS
                        offs += 2
                        if offs == numPairs:
                            break
                        curBack = matches[offs + 1]
                        if curBack >= kNumFullDistances:
                            posSlot = _pos_slot2(curBack)
                    lenTest += 1

    # ---- LzmaEnc_CodeOneBlock (no limits; the 32 KiB block returns of LzmaEnc_Encode2 do not change the output)
    def encode(self):
        nowPos32 = 0
        if self.n == 0:
            self.flush_data()
            return bytes(self.out)
        self.read_match_distances()
        self.encode_bit(self.isMatch[self.state], 0, 0)
        self.state = kLiteralNextStates[self.state]
        cur_byte = self.buf[self.mfi - self.additionalOffset]
        self.lit_encode(0, cur_byte)
        self.additionalOffset -= 1
        nowPos32 += 1
        buf = self.buf
        if self.n - self.mfi != 0:
            while True:
                ln, pos = self.get_optimum(nowPos32)
                posState = nowPos32 & self.pbMask
                st = self.state
                if ln == 1 and pos == M32:
                    self.encode_bit(self.isMatch[st], posState, 0)
                    data = self.mfi - self.additionalOffset
                    cur_byte = buf[data]
                    base = self.lit_base(nowPos32, buf[data - 1])
                    if st < 7:
                        self.lit_encode(base, cur_byte)
                    else:
                        self.lit_encode_matched(base, cur_byte, buf[data - self.reps[0] - 1])
                    self.state = kLiteralNextStates[st]
                else:
                    self.encode_bit(self.isMatch[st], posState, 1)
                    if pos < LZMA_NUM_REPS:
                        self.encode_bit(self.isRep, st, 1)
                        if pos == 0:
                            self.encode_bit(self.isRepG0, st, 0)
                            self.encode_bit(self.isRep0Long[st], posState, 0 if ln == 1 else 1)
                        else:
                            reps = self.reps
                            distance = reps[pos]
                            self.encode_bit(self.isRepG0, st, 1)
                            if pos == 1:
                                self.encode_bit(self.isRepG1, st, 0)
                            else:
                                self.encode_bit(self.isRepG1, st, 1)
                                self.encode_bit(self.isRepG2, st, pos - 2)
                                if pos == 3:
                                    reps[3] = reps[2]
                                reps[2] = reps[1]
                            reps[1] = reps[0]
                            reps[0] = distance
                        if ln == 1:
                            self.state = kShortRepNextStates[st]
                        else:
                            self.len_encode(self.repLenEnc, ln - LZMA_MATCH_LEN_MIN, posState)
                            self.state = kRepNextStates[st]
                    else:
                        self.encode_bit(self.isRep, st, 0)
                        self.state = kMatchNextStates[st]
                        self.len_encode(self.lenEnc, ln - LZMA_MATCH_LEN_MIN, posState)
                        pos -= LZMA_NUM_REPS
                        posSlot = _pos_slot(pos)
                        self.rc_tree_encode(self.posSlotEncoder[_len_to_pos_state(ln)], kNumPosSlotBits, posSlot)
                        if posSlot >= kStartPosModelIndex:
                            footerBits = (posSlot >> 1) - 1
                            base = (2 | (posSlot & 1)) << footerBits
                            posReduced = pos - base
                            if posSlot < kEndPosModelIndex:
                                self.rc_tree_reverse_encode(self.posEncoders, base - posSlot - 1, footerBits,
                                                            posReduced)
                            else:
                                self.encode_direct_bits(posReduced >> kNumAlignBits, footerBits - kNumAlignBits)
                                self.rc_tree_reverse_encode(self.posAlignEncoder, 0, kNumAlignBits,
                                                            posReduced & kAlignMask)
                                self.alignPriceCount += 1
                        reps = self.reps
                        reps[3] = reps[2]
                        reps[2] = reps[1]
                        reps[1] = reps[0]
                        reps[0] = pos
                        self.matchPriceCount += 1
                self.additionalOffset -= ln
                nowPos32 += ln
                if self.additionalOffset == 0:
                    if self.matchPriceCount >= 1 << 7:
                        self.fill_distances_prices()
                    if self.alignPriceCount >= kAlignTableSize:
                        self.fill_align_prices()
                    if self.n - self.mfi == 0:
                        break
        self.flush_data()
        return bytes(self.out)


class _LenPriceEnc:
    """CLenPriceEnc: probs = [choice, choice2, low[16 << 3], mid[16 << 3], high[256]]."""

    def __init__(self):
        self.probs = [kProbInitValue] * (2 + 128 + 128 + 256)
        self.prices = [[0] * (kLenNumLowSymbols + kLenNumMidSymbols + kLenNumHighSymbols) for _ in range(16)]
        self.tableSize = 0
        self.counters = [0] * 16

    def update_table(self, pos_state):
        """LenPriceEnc_UpdateTable (LenEnc_SetPrices)."""
        p, pp = self.probs, PROB_PRICES
        prices = self.prices[pos_state]
        n = self.tableSize
        a0 = pp[p[0] >> 4]
        a1 = pp[(p[0] ^ 2047) >> 4]
        b0 = a1 + pp[p[1] >> 4]
        b1 = a1 + pp[(p[1] ^ 2047) >> 4]
        for i in range(n):
            if i < kLenNumLowSymbols:
                prices[i] = a0 + _rc_tree_price_at(p, 2 + (pos_state << kLenNumLowBits), kLenNumLowBits, i, pp)
            elif i < kLenNumLowSymbols + kLenNumMidSymbols:
                prices[i] = b0 + _rc_tree_price_at(p, 2 + 128 + (pos_state << kLenNumMidBits), kLenNumMidBits,
                                                   i - kLenNumLowSymbols, pp)
            else:
                prices[i] = b1 + _rc_tree_price_at(p, 2 + 256, kLenNumHighBits,
                                                   i - kLenNumLowSymbols - kLenNumMidSymbols, pp)
        self.counters[pos_state] = n


def _rc_tree_price(probs, num_bits, symbol, pp):
    return _rc_tree_price_at(probs, 0, num_bits, symbol, pp)


def _rc_tree_price_at(probs, base, num_bits, symbol, pp):
    """RcTree_GetPrice."""
    price = 0
    symbol |= 1 << num_bits
    while symbol != 1:
        price += pp[(probs[base + (symbol >> 1)] ^ (2047 if symbol & 1 else 0)) >> 4]
        symbol >>= 1
    return price


def _rc_tree_reverse_price(probs, base, num_bits, symbol, pp):
    """RcTree_ReverseGetPrice."""
    price = 0
    m = 1
    for _ in range(num_bits):
        bit = symbol & 1
        symbol >>= 1
        price += pp[(probs[base + m] ^ (2047 if bit else 0)) >> 4]
        m = (m << 1) | bit
    return price


def compress(src, dict_size=None):
    """LzmaCompress(level 5, dict_size, lc 3, lp 0, pb 2, fb 32, 1 thread) -> (props, stream)."""
    src = bytes(src)
    if dict_size is None:
        dict_size = dict_size_for(len(src))
    return props_bytes(dict_size), _Encoder(src, dict_size).encode()


def blob(src):
    """FUN_009d61f4 into a buffer of len + 0x90 bytes: the 16-byte header + stream, or None on overflow."""
    src = bytes(src)
    props, stream = compress(src)
    if len(stream) > len(src) + 0x90 - 0x10:
        return None
    return len(src).to_bytes(4, "little") + len(stream).to_bytes(4, "little") + props + b"\0\0\0" + stream


def blob_size(src):
    """FUN_009d61f4's return value: 16 + stream length, or -1 (SZ_ERROR_OUTPUT_EOF)."""
    b = blob(src)
    return -1 if b is None else len(b)


def plane_int16(plane):
    """FUN_009f0554 (float -> 16-bit image type): rint(x * 16) with ties to even, saturated to int16, NaN -> 0.
    The NEON part (fcvtns + sqxtn) and the scalar tail (fmin 2047.9375, < -2048 -> -32768, frintn, fcvtzs) agree."""
    with np.errstate(over="ignore", invalid="ignore"):
        x = np.asarray(plane, F32) * F32(16.0)
        r = np.rint(np.nan_to_num(x, nan=0.0, posinf=32767.0, neginf=-32768.0))
    return np.clip(r, -32768, 32767).astype("<i2").tobytes()
