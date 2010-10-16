# source file for testing assembler capabilities

.text

.ifdef ASMCAPA_MMX_TEST
   emms
.endif

.ifdef ASMCAPA_SSE_TEST
   maskmovq %mm3, %mm1
.endif
