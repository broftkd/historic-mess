#include "cpudefs.h"
void op_c000(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	BYTE dst = regs.d[dstreg].B.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}
void op_c010(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}
void op_c018(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	regs.a[srcreg] += areg_byteinc[srcreg];
{	BYTE dst = regs.d[dstreg].B.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}}
void op_c020(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}}
void op_c028(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}
void op_c030(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}}
void op_c038(ULONG opcode) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}
void op_c039(ULONG opcode) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}
void op_c03a(ULONG opcode) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}}
void op_c03b(ULONG opcode) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	BYTE src = get_byte(srca);
{	BYTE dst = regs.d[dstreg].B.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}}
void op_c03c(ULONG opcode) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = nextiword();
{	BYTE dst = regs.d[dstreg].B.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	regs.d[dstreg].B.l = src;
}}}}
void op_c040(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	WORD dst = regs.d[dstreg].W.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}
void op_c050(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}
void op_c058(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	WORD dst = regs.d[dstreg].W.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}}
void op_c060(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}}
void op_c068(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}
void op_c070(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}}
void op_c078(ULONG opcode) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}
void op_c079(ULONG opcode) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}
void op_c07a(ULONG opcode) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}}
void op_c07b(ULONG opcode) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}}
void op_c07c(ULONG opcode) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	WORD dst = regs.d[dstreg].W.l;
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	regs.d[dstreg].W.l = src;
}}}}
void op_c080(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = regs.d[srcreg].D;
{	LONG dst = regs.d[dstreg].D;
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}
void op_c090(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}
void op_c098(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	regs.a[srcreg] += 4;
{	LONG dst = regs.d[dstreg].D;
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}}
void op_c0a0(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 4;
{	CPTR srca = regs.a[srcreg];
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}}
void op_c0a8(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}
void op_c0b0(ULONG opcode) /* AND */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}}
void op_c0b8(ULONG opcode) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}
void op_c0b9(ULONG opcode) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}
void op_c0ba(ULONG opcode) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}}
void op_c0bb(ULONG opcode) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	LONG src = get_long(srca);
{	LONG dst = regs.d[dstreg].D;
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}}
void op_c0bc(ULONG opcode) /* AND */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	LONG src = nextilong();
{	LONG dst = regs.d[dstreg].D;
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	regs.d[dstreg].D = (src);
}}}}
void op_c0c0(ULONG opcode) /* MULU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}
void op_c0d0(ULONG opcode) /* MULU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}
void op_c0d8(ULONG opcode) /* MULU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}}
void op_c0e0(ULONG opcode) /* MULU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}}
void op_c0e8(ULONG opcode) /* MULU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}
void op_c0f0(ULONG opcode) /* MULU */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}}
void op_c0f8(ULONG opcode) /* MULU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}
void op_c0f9(ULONG opcode) /* MULU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}
void op_c0fa(ULONG opcode) /* MULU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}}
void op_c0fb(ULONG opcode) /* MULU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}}
void op_c0fc(ULONG opcode) /* MULU */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (ULONG)(UWORD)dst * (ULONG)(UWORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}
void op_c100(ULONG opcode) /* ABCD */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	BYTE dst = regs.d[dstreg].B.l;
{	UWORD newv_lo = (src & 0xF) + (dst & 0xF) + (regs.x ? 1 : 0);
	UWORD newv_hi = (src & 0xF0) + (dst & 0xF0);
	UWORD newv;
	if (newv_lo > 9) { newv_lo +=6; }
	newv = newv_hi + newv_lo;	CFLG = regs.x = (newv & 0x1F0) > 0x90;
	if (CFLG) newv += 0x60;
	if (((BYTE)(newv)) != 0) ZFLG = 0;
	NFLG = ((BYTE)(newv)) < 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	regs.d[dstreg].B.l = newv;
}}}}}}
void op_c108(ULONG opcode) /* ABCD */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= areg_byteinc[srcreg];
{	CPTR srca = regs.a[srcreg];
	BYTE src = get_byte(srca);
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	UWORD newv_lo = (src & 0xF) + (dst & 0xF) + (regs.x ? 1 : 0);
	UWORD newv_hi = (src & 0xF0) + (dst & 0xF0);
	UWORD newv;
	if (newv_lo > 9) { newv_lo +=6; }
	newv = newv_hi + newv_lo;	CFLG = regs.x = (newv & 0x1F0) > 0x90;
	if (CFLG) newv += 0x60;
	if (((BYTE)(newv)) != 0) ZFLG = 0;
	NFLG = ((BYTE)(newv)) < 0;
{	int flgs = ((BYTE)(src)) < 0;
	int flgo = ((BYTE)(dst)) < 0;
	int flgn = ((BYTE)(newv)) < 0;
	VFLG = (flgs != flgo) && (flgn != flgo);
	put_byte(dsta,newv);
}}}}}}}}
void op_c110(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	put_byte(dsta,src);
}}}}
void op_c118(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
{	regs.a[dstreg] += areg_byteinc[dstreg];
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	put_byte(dsta,src);
}}}}}
void op_c120(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	regs.a[dstreg] -= areg_byteinc[dstreg];
{	CPTR dsta = regs.a[dstreg];
	BYTE dst = get_byte(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	put_byte(dsta,src);
}}}}}
void op_c128(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	put_byte(dsta,src);
}}}}
void op_c130(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	BYTE dst = get_byte(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	put_byte(dsta,src);
}}}}}
void op_c138(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = (LONG)(WORD)nextiword();
	BYTE dst = get_byte(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	put_byte(dsta,src);
}}}}
void op_c139(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	BYTE src = regs.d[srcreg].B.l;
{	CPTR dsta = nextilong();
	BYTE dst = get_byte(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((BYTE)(src)) == 0;
	NFLG = ((BYTE)(src)) < 0;
	put_byte(dsta,src);
}}}}
void op_c140(ULONG opcode) /* EXG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	LONG dst = regs.d[dstreg].D;
	regs.d[srcreg].D = (dst);
	regs.d[dstreg].D = (src);
}}}}
void op_c148(ULONG opcode) /* EXG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.a[srcreg];
{	LONG dst = regs.a[dstreg];
	regs.a[srcreg] = (dst);
	regs.a[dstreg] = (src);
}}}}
void op_c150(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_c158(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
{	regs.a[dstreg] += 2;
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_c160(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	regs.a[dstreg] -= 2;
{	CPTR dsta = regs.a[dstreg];
	WORD dst = get_word(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_c168(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_c170(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	WORD dst = get_word(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}}
void op_c178(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = (LONG)(WORD)nextiword();
	WORD dst = get_word(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_c179(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	WORD src = regs.d[srcreg].W.l;
{	CPTR dsta = nextilong();
	WORD dst = get_word(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((WORD)(src)) == 0;
	NFLG = ((WORD)(src)) < 0;
	put_word(dsta,src);
}}}}
void op_c188(ULONG opcode) /* EXG */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	LONG dst = regs.a[dstreg];
	regs.d[srcreg].D = (dst);
	regs.a[dstreg] = (src);
}}}}
void op_c190(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_c198(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
{	regs.a[dstreg] += 4;
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_c1a0(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	regs.a[dstreg] -= 4;
{	CPTR dsta = regs.a[dstreg];
	LONG dst = get_long(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_c1a8(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = regs.a[dstreg] + (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_c1b0(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
	ULONG dstreg = opcode & 7;
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = get_disp_ea(regs.a[dstreg]);
{	LONG dst = get_long(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}}
void op_c1b8(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = (LONG)(WORD)nextiword();
	LONG dst = get_long(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_c1b9(ULONG opcode) /* AND */
{
	ULONG srcreg = ((opcode >> 9) & 7);
{{	LONG src = regs.d[srcreg].D;
{	CPTR dsta = nextilong();
	LONG dst = get_long(dsta);
	src &= dst;
	CLEARVC;
	ZFLG = ((LONG)(src)) == 0;
	NFLG = ((LONG)(src)) < 0;
	put_long(dsta,src);
}}}}
void op_c1c0(ULONG opcode) /* MULS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = regs.d[srcreg].W.l;
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}
void op_c1d0(ULONG opcode) /* MULS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}
void op_c1d8(ULONG opcode) /* MULS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	regs.a[srcreg] += 2;
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}}
void op_c1e0(ULONG opcode) /* MULS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	regs.a[srcreg] -= 2;
{	CPTR srca = regs.a[srcreg];
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}}
void op_c1e8(ULONG opcode) /* MULS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = regs.a[srcreg] + (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}
void op_c1f0(ULONG opcode) /* MULS */
{
	ULONG srcreg = (opcode & 7);
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(regs.a[srcreg]);
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}}
void op_c1f8(ULONG opcode) /* MULS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = (LONG)(WORD)nextiword();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}
void op_c1f9(ULONG opcode) /* MULS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = nextilong();
	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}
void op_c1fa(ULONG opcode) /* MULS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = m68k_getpc();
	srca += (LONG)(WORD)nextiword();
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}}
void op_c1fb(ULONG opcode) /* MULS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	CPTR srca = get_disp_ea(m68k_getpc());
{	WORD src = get_word(srca);
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}}
void op_c1fc(ULONG opcode) /* MULS */
{
	ULONG dstreg = (opcode >> 9) & 7;
{{	WORD src = nextiword();
{	WORD dst = regs.d[dstreg].W.l;
{	ULONG newv = (LONG)(WORD)dst * (LONG)(WORD)src;
	CLEARVC;
	ZFLG = ((LONG)(newv)) == 0;
	NFLG = ((LONG)(newv)) < 0;
	regs.d[dstreg].D = (newv);
}}}}}
