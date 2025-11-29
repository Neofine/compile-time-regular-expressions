	.file	"test_asm_with_decomp.cpp"
	.text
	.section	.text._ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE,"axG",@progbits,_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE,comdat
	.p2align 4
	.weak	_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE
	.type	_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE, @function
_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE:
.LFB11216:
	.cfi_startproc
	endbr64
	movl	_ZZN4ctre4simd19get_simd_capabilityEvE17cached_capability(%rip), %eax
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rdx, %r9
	movq	%rsi, %r8
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	pushq	%rbx
	.cfi_offset 3, -24
	cmpl	$-1, %eax
	je	.L109
	cmpl	$1, %eax
	jle	.L110
.L6:
	movq	%rsi, %rcx
	movzbl	2(%r9), %r9d
	subq	%rdi, %rcx
	leaq	-16(%rcx), %rax
	cmpq	$15, %rax
	jbe	.L111
	cmpq	$63, %rcx
	ja	.L21
	cmpq	$31, %rcx
	jg	.L112
.L21:
	cmpq	%rdi, %rsi
	je	.L51
	movq	%rdi, %rax
	xorl	%edx, %edx
	cmpq	$63, %rcx
	jle	.L113
.L18:
	testb	%r9b, %r9b
	jne	.L114
	movl	$97, %ecx
	vpcmpeqd	%ymm4, %ymm4, %ymm4
	vmovd	%ecx, %xmm2
	vpbroadcastb	%xmm2, %ymm2
	jmp	.L38
	.p2align 4
	.p2align 3
.L36:
	addq	$64, %rax
	addq	$64, %rdx
	cmpq	%rsi, %rax
	je	.L106
	movq	%rsi, %rcx
	subq	%rax, %rcx
	cmpq	$63, %rcx
	jle	.L115
.L38:
	vpcmpeqb	(%rax), %ymm2, %ymm0
	vpcmpeqb	32(%rax), %ymm2, %ymm1
	vpand	%ymm0, %ymm1, %ymm3
	vptest	%ymm4, %ymm3
	jb	.L36
.L27:
	vpcmpeqd	%ymm3, %ymm3, %ymm3
	vptest	%ymm3, %ymm0
	jnb	.L30
	vpmovmskb	%ymm1, %ecx
	notl	%ecx
	tzcntl	%ecx, %ecx
	addl	$32, %ecx
	movslq	%ecx, %rcx
	addq	%rcx, %rax
	addq	%rcx, %rdx
.L31:
	cmpq	%rax, %rsi
	je	.L106
	movq	%rsi, %rcx
	subq	%rax, %rcx
	cmpq	$31, %rcx
	jle	.L103
.L33:
	testb	%r9b, %r9b
	je	.L45
.L65:
	movabsq	$2314885530818453536, %rbx
	movl	$97, %ecx
	vpcmpeqd	%ymm3, %ymm3, %ymm3
	vmovq	%rbx, %xmm2
	vmovd	%ecx, %xmm1
	vpbroadcastq	%xmm2, %ymm2
	vpbroadcastb	%xmm1, %ymm1
	jmp	.L46
	.p2align 4
	.p2align 3
.L116:
	addq	$32, %rax
	addq	$32, %rdx
	cmpq	%rsi, %rax
	je	.L106
	movq	%rsi, %rcx
	subq	%rax, %rcx
	cmpq	$31, %rcx
	jle	.L103
.L46:
	vpor	(%rax), %ymm2, %ymm0
	vpcmpeqb	%ymm1, %ymm0, %ymm0
	vptest	%ymm3, %ymm0
	jb	.L116
.L41:
	vpmovmskb	%ymm0, %ecx
	notl	%ecx
	tzcntl	%ecx, %ecx
	addq	%rcx, %rax
	addq	%rcx, %rdx
	vzeroupper
.L19:
	cmpq	%rsi, %rax
	je	.L50
.L34:
	sall	$5, %r9d
	jmp	.L49
	.p2align 4
	.p2align 3
.L117:
	incq	%rax
	incq	%rdx
	cmpq	%rsi, %rax
	je	.L50
.L49:
	movzbl	(%rax), %ecx
	orl	%r9d, %ecx
	cmpb	$97, %cl
	je	.L117
.L76:
	movq	%rax, %r8
.L50:
	testq	%rdx, %rdx
	jne	.L100
.L51:
	movq	%rdi, %r8
.L100:
	movq	-8(%rbp), %rbx
	movq	%r8, %rax
	leave
	.cfi_remember_state
	.cfi_def_cfa 7, 8
	ret
	.p2align 4
	.p2align 3
.L110:
	.cfi_restore_state
	jne	.L51
.L63:
	movzbl	2(%r9), %r9d
	cmpq	%rdi, %rsi
	je	.L51
	movq	%rsi, %rax
	movl	$0, %edx
	subq	%rdi, %rax
	cmpq	$15, %rax
	movq	%rdi, %rax
	jle	.L57
	testb	%r9b, %r9b
	jne	.L75
	movl	$97, %ecx
	vmovd	%ecx, %xmm1
	vpbroadcastb	%xmm1, %xmm1
	jmp	.L60
	.p2align 4
	.p2align 3
.L118:
	addq	$16, %rax
	addq	$16, %rdx
	cmpq	%rsi, %rax
	je	.L50
	movq	%rsi, %rcx
	subq	%rax, %rcx
	cmpq	$15, %rcx
	jle	.L57
.L60:
	vpcmpeqb	(%rax), %xmm1, %xmm0
	vpmovmskb	%xmm0, %ecx
	cmpl	$65535, %ecx
	je	.L118
.L54:
	notl	%ecx
	tzcntl	%ecx, %ecx
	addq	%rcx, %rax
	addq	%rcx, %rdx
	cmpq	%rsi, %rax
	je	.L50
.L57:
	sall	$5, %r9d
	jmp	.L61
	.p2align 4
	.p2align 3
.L119:
	incq	%rax
	incq	%rdx
	cmpq	%rsi, %rax
	je	.L50
.L61:
	movzbl	(%rax), %ecx
	orl	%r9d, %ecx
	cmpb	$97, %cl
	je	.L119
	jmp	.L76
	.p2align 4
	.p2align 3
.L114:
	movabsq	$2314885530818453536, %rbx
	movl	$97, %ecx
	vpcmpeqd	%ymm5, %ymm5, %ymm5
	vmovq	%rbx, %xmm3
	vmovd	%ecx, %xmm2
	vpbroadcastq	%xmm3, %ymm3
	vpbroadcastb	%xmm2, %ymm2
	jmp	.L35
	.p2align 4
	.p2align 3
.L120:
	addq	$64, %rax
	addq	$64, %rdx
	cmpq	%rsi, %rax
	je	.L106
	movq	%rsi, %rcx
	subq	%rax, %rcx
	cmpq	$63, %rcx
	jle	.L26
.L35:
	vpor	(%rax), %ymm3, %ymm0
	vpor	32(%rax), %ymm3, %ymm1
	vpcmpeqb	%ymm2, %ymm0, %ymm0
	vpcmpeqb	%ymm2, %ymm1, %ymm1
	vpand	%ymm1, %ymm0, %ymm4
	vptest	%ymm5, %ymm4
	jb	.L120
	jmp	.L27
	.p2align 4
	.p2align 3
.L111:
	vmovdqu	(%rdi), %xmm0
	testb	%r9b, %r9b
	je	.L12
	movabsq	$2314885530818453536, %rax
	vmovq	%rax, %xmm1
	vpunpcklqdq	%xmm1, %xmm1, %xmm1
	vpor	%xmm1, %xmm0, %xmm0
.L12:
	movl	$97, %eax
	vpcmpeqd	%xmm2, %xmm2, %xmm2
	vmovd	%eax, %xmm1
	vpbroadcastb	%xmm1, %xmm1
	vpcmpeqb	%xmm1, %xmm0, %xmm0
	vptest	%xmm2, %xmm0
	jnb	.L14
	leaq	16(%rdi), %rax
	cmpq	%rsi, %rax
	jnb	.L66
	movq	%rsi, %rcx
	movl	$16, %edx
	subq	%rax, %rcx
	cmpq	$63, %rcx
	ja	.L77
	cmpq	$31, %rcx
	jg	.L16
.L77:
	cmpq	$63, %rcx
	jg	.L18
	jmp	.L19
	.p2align 4
	.p2align 3
.L112:
	movq	%rdi, %rax
	xorl	%edx, %edx
.L16:
	vmovdqu	(%rax), %ymm0
	testb	%r9b, %r9b
	je	.L22
	movabsq	$2314885530818453536, %rbx
	vmovq	%rbx, %xmm1
	vpbroadcastq	%xmm1, %ymm1
	vpor	%ymm1, %ymm0, %ymm0
.L22:
	movl	$97, %ecx
	vpcmpeqd	%ymm2, %ymm2, %ymm2
	vmovd	%ecx, %xmm1
	vpbroadcastb	%xmm1, %ymm1
	vpcmpeqb	%ymm1, %ymm0, %ymm0
	vptest	%ymm2, %ymm0
	jnb	.L24
	addq	$32, %rax
	cmpq	%rsi, %rax
	jnb	.L69
	movq	%rsi, %rcx
	addq	$32, %rdx
	subq	%rax, %rcx
	cmpq	$63, %rcx
	jg	.L18
	cmpq	$31, %rcx
	jg	.L33
	vzeroupper
	jmp	.L19
	.p2align 4
	.p2align 3
.L106:
	vzeroupper
	jmp	.L50
	.p2align 4
	.p2align 3
.L115:
	cmpq	$31, %rcx
	jle	.L103
.L45:
	movl	$97, %ecx
	vpcmpeqd	%ymm2, %ymm2, %ymm2
	vmovd	%ecx, %xmm1
	vpbroadcastb	%xmm1, %ymm1
	jmp	.L47
	.p2align 4
	.p2align 3
.L121:
	addq	$32, %rax
	addq	$32, %rdx
	cmpq	%rax, %rsi
	je	.L106
	movq	%rsi, %rcx
	subq	%rax, %rcx
	cmpq	$31, %rcx
	jle	.L103
.L47:
	vpcmpeqb	(%rax), %ymm1, %ymm0
	vptest	%ymm2, %ymm0
	jb	.L121
	jmp	.L41
	.p2align 4
	.p2align 3
.L26:
	cmpq	$31, %rcx
	jg	.L65
	.p2align 4
	.p2align 3
.L103:
	vzeroupper
	jmp	.L34
	.p2align 4
	.p2align 3
.L109:
	cmpb	$0, _ZZN4ctre4simd8has_avx2EvE8detected(%rip)
	je	.L3
	movzbl	_ZZN4ctre4simd8has_avx2EvE6result(%rip), %eax
.L4:
	testb	%al, %al
	je	.L5
	movl	$2, _ZZN4ctre4simd19get_simd_capabilityEvE17cached_capability(%rip)
	jmp	.L6
	.p2align 4
	.p2align 3
.L75:
	movabsq	$2314885530818453536, %rbx
	movl	$97, %ecx
	vmovq	%rbx, %xmm2
	vmovd	%ecx, %xmm1
	vpunpcklqdq	%xmm2, %xmm2, %xmm2
	vpbroadcastb	%xmm1, %xmm1
	jmp	.L59
	.p2align 4
	.p2align 3
.L122:
	addq	$16, %rax
	addq	$16, %rdx
	cmpq	%rsi, %rax
	je	.L50
	movq	%rsi, %rcx
	subq	%rax, %rcx
	cmpq	$15, %rcx
	jle	.L57
.L59:
	vpor	(%rax), %xmm2, %xmm0
	vpcmpeqb	%xmm1, %xmm0, %xmm0
	vpmovmskb	%xmm0, %ecx
	cmpl	$65535, %ecx
	je	.L122
	jmp	.L54
	.p2align 4
	.p2align 3
.L14:
	vpmovmskb	%xmm0, %edx
	notl	%edx
	tzcntl	%edx, %edx
	leaq	(%rdi,%rdx), %r8
	jmp	.L50
	.p2align 4
	.p2align 3
.L30:
	vpmovmskb	%ymm0, %ecx
	notl	%ecx
	tzcntl	%ecx, %ecx
	addq	%rcx, %rax
	addq	%rcx, %rdx
	jmp	.L31
	.p2align 4
	.p2align 3
.L24:
	vpmovmskb	%ymm0, %ecx
	notl	%ecx
	tzcntl	%ecx, %ecx
	addq	%rcx, %rdx
	leaq	(%rax,%rcx), %r8
	vzeroupper
	jmp	.L50
	.p2align 4
	.p2align 3
.L5:
	cmpb	$0, _ZZN4ctre4simd9has_sse42EvE8detected(%rip)
	je	.L7
	movzbl	_ZZN4ctre4simd9has_sse42EvE6result(%rip), %eax
.L8:
	testb	%al, %al
	je	.L123
	movl	$1, _ZZN4ctre4simd19get_simd_capabilityEvE17cached_capability(%rip)
	jmp	.L63
	.p2align 4
	.p2align 3
.L3:
	movl	$7, %eax
	xorl	%ecx, %ecx
#APP
# 56 "include/ctre/simd_detection.hpp" 1
	cpuid
# 0 "" 2
#NO_APP
	shrl	$5, %ebx
	movb	$1, _ZZN4ctre4simd8has_avx2EvE8detected(%rip)
	movl	%ebx, %eax
	andl	$1, %eax
	movb	%al, _ZZN4ctre4simd8has_avx2EvE6result(%rip)
	jmp	.L4
	.p2align 4
	.p2align 3
.L66:
	movq	%rax, %r8
	jmp	.L100
.L69:
	movq	%rax, %r8
	vzeroupper
	jmp	.L100
.L7:
	movl	$1, %eax
#APP
# 93 "include/ctre/simd_detection.hpp" 1
	cpuid
# 0 "" 2
#NO_APP
	movl	%ecx, %eax
	movb	$1, _ZZN4ctre4simd9has_sse42EvE8detected(%rip)
	shrl	$20, %eax
	andl	$1, %eax
	movb	%al, _ZZN4ctre4simd9has_sse42EvE6result(%rip)
	jmp	.L8
.L123:
	movl	$0, _ZZN4ctre4simd19get_simd_capabilityEvE17cached_capability(%rip)
	jmp	.L51
.L113:
	cmpq	$31, %rcx
	jg	.L33
	jmp	.L34
	.cfi_endproc
.LFE11216:
	.size	_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE, .-_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE
	.text
	.p2align 4
	.globl	_Z10test_matchRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE
	.type	_Z10test_matchRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, @function
_Z10test_matchRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE:
.LFB10473:
	.cfi_startproc
	endbr64
	pushq	%r12
	.cfi_def_cfa_offset 16
	.cfi_offset 12, -16
	pushq	%rbp
	.cfi_def_cfa_offset 24
	.cfi_offset 6, -24
	pushq	%rbx
	.cfi_def_cfa_offset 32
	.cfi_offset 3, -32
	subq	$16, %rsp
	.cfi_def_cfa_offset 48
	movq	(%rdi), %rbx
	movq	%fs:40, %rax
	movq	%rax, 8(%rsp)
	xorl	%eax, %eax
	movq	8(%rdi), %rax
	movw	$0, 4(%rsp)
	movb	$0, 6(%rsp)
	leaq	(%rbx,%rax), %rbp
	cmpq	$15, %rax
	jg	.L137
.L125:
	xorl	%eax, %eax
	cmpq	%rbp, %rbx
	je	.L124
	xorl	%eax, %eax
	.p2align 4
	.p2align 3
.L130:
	cmpb	$97, (%rbx)
	jne	.L129
	incq	%rbx
	incq	%rax
	cmpq	%rbx, %rbp
	jne	.L130
	testq	%rax, %rax
	setne	%al
.L124:
	movq	8(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L138
	addq	$16, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 32
	popq	%rbx
	.cfi_def_cfa_offset 24
	popq	%rbp
	.cfi_def_cfa_offset 16
	popq	%r12
	.cfi_def_cfa_offset 8
	ret
	.p2align 4
	.p2align 3
.L129:
	.cfi_restore_state
	testq	%rax, %rax
	setne	%al
	cmpq	%rbx, %rbp
	sete	%dl
	andl	%edx, %eax
	jmp	.L124
	.p2align 4
	.p2align 3
.L137:
	leaq	4(%rsp), %r12
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	movq	%r12, %rdx
	call	_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE
	cmpq	%rax, %rbx
	je	.L126
	cmpq	%rax, %rbp
	sete	%al
	jmp	.L124
	.p2align 4
	.p2align 3
.L126:
	movq	%r12, %rdx
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	call	_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE
	cmpq	%rax, %rbx
	je	.L125
	cmpq	%rax, %rbp
	sete	%al
	jmp	.L124
.L138:
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE10473:
	.size	_Z10test_matchRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, .-_Z10test_matchRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE
	.weak	_ZZN4ctre4simd19get_simd_capabilityEvE17cached_capability
	.section	.data._ZZN4ctre4simd19get_simd_capabilityEvE17cached_capability,"awG",@progbits,_ZZN4ctre4simd19get_simd_capabilityEvE17cached_capability,comdat
	.align 4
	.type	_ZZN4ctre4simd19get_simd_capabilityEvE17cached_capability, @gnu_unique_object
	.size	_ZZN4ctre4simd19get_simd_capabilityEvE17cached_capability, 4
_ZZN4ctre4simd19get_simd_capabilityEvE17cached_capability:
	.long	-1
	.weak	_ZZN4ctre4simd9has_sse42EvE6result
	.section	.bss._ZZN4ctre4simd9has_sse42EvE6result,"awG",@nobits,_ZZN4ctre4simd9has_sse42EvE6result,comdat
	.type	_ZZN4ctre4simd9has_sse42EvE6result, @gnu_unique_object
	.size	_ZZN4ctre4simd9has_sse42EvE6result, 1
_ZZN4ctre4simd9has_sse42EvE6result:
	.zero	1
	.weak	_ZZN4ctre4simd9has_sse42EvE8detected
	.section	.bss._ZZN4ctre4simd9has_sse42EvE8detected,"awG",@nobits,_ZZN4ctre4simd9has_sse42EvE8detected,comdat
	.type	_ZZN4ctre4simd9has_sse42EvE8detected, @gnu_unique_object
	.size	_ZZN4ctre4simd9has_sse42EvE8detected, 1
_ZZN4ctre4simd9has_sse42EvE8detected:
	.zero	1
	.weak	_ZZN4ctre4simd8has_avx2EvE6result
	.section	.bss._ZZN4ctre4simd8has_avx2EvE6result,"awG",@nobits,_ZZN4ctre4simd8has_avx2EvE6result,comdat
	.type	_ZZN4ctre4simd8has_avx2EvE6result, @gnu_unique_object
	.size	_ZZN4ctre4simd8has_avx2EvE6result, 1
_ZZN4ctre4simd8has_avx2EvE6result:
	.zero	1
	.weak	_ZZN4ctre4simd8has_avx2EvE8detected
	.section	.bss._ZZN4ctre4simd8has_avx2EvE8detected,"awG",@nobits,_ZZN4ctre4simd8has_avx2EvE8detected,comdat
	.type	_ZZN4ctre4simd8has_avx2EvE8detected, @gnu_unique_object
	.size	_ZZN4ctre4simd8has_avx2EvE8detected, 1
_ZZN4ctre4simd8has_avx2EvE8detected:
	.zero	1
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
