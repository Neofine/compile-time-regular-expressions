	.file	"test_ifconstexpr_overhead.cpp"
	.text
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB10636:
	.cfi_startproc
	endbr64
	xorl	%eax, %eax
	ret
	.cfi_endproc
.LFE10636:
	.size	main, .-main
	.section	.text._ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE,"axG",@progbits,_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE,comdat
	.p2align 4
	.weak	_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE
	.type	_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE, @function
_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE:
.LFB11313:
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
	je	.L110
	cmpl	$1, %eax
	jle	.L111
.L8:
	movq	%rsi, %rcx
	movzbl	2(%r9), %r9d
	subq	%rdi, %rcx
	leaq	-16(%rcx), %rax
	cmpq	$15, %rax
	jbe	.L112
	cmpq	$63, %rcx
	ja	.L23
	cmpq	$31, %rcx
	jg	.L113
.L23:
	cmpq	%rdi, %rsi
	je	.L53
	movq	%rdi, %rax
	xorl	%edx, %edx
	cmpq	$63, %rcx
	jle	.L114
.L20:
	testb	%r9b, %r9b
	jne	.L115
	movl	$97, %ecx
	vpcmpeqd	%ymm4, %ymm4, %ymm4
	vmovd	%ecx, %xmm2
	vpbroadcastb	%xmm2, %ymm2
	jmp	.L40
	.p2align 4
	.p2align 3
.L38:
	addq	$64, %rax
	addq	$64, %rdx
	cmpq	%rsi, %rax
	je	.L108
	movq	%rsi, %rcx
	subq	%rax, %rcx
	cmpq	$63, %rcx
	jle	.L116
.L40:
	vpcmpeqb	(%rax), %ymm2, %ymm0
	vpcmpeqb	32(%rax), %ymm2, %ymm1
	vpand	%ymm0, %ymm1, %ymm3
	vptest	%ymm4, %ymm3
	jb	.L38
.L29:
	vpcmpeqd	%ymm3, %ymm3, %ymm3
	vptest	%ymm3, %ymm0
	jnb	.L32
	vpmovmskb	%ymm1, %ecx
	notl	%ecx
	tzcntl	%ecx, %ecx
	addl	$32, %ecx
	movslq	%ecx, %rcx
	addq	%rcx, %rax
	addq	%rcx, %rdx
.L33:
	cmpq	%rax, %rsi
	je	.L108
	movq	%rsi, %rcx
	subq	%rax, %rcx
	cmpq	$31, %rcx
	jle	.L105
.L35:
	testb	%r9b, %r9b
	je	.L47
.L67:
	movabsq	$2314885530818453536, %rbx
	movl	$97, %ecx
	vpcmpeqd	%ymm3, %ymm3, %ymm3
	vmovq	%rbx, %xmm2
	vmovd	%ecx, %xmm1
	vpbroadcastq	%xmm2, %ymm2
	vpbroadcastb	%xmm1, %ymm1
	jmp	.L48
	.p2align 4
	.p2align 3
.L117:
	addq	$32, %rax
	addq	$32, %rdx
	cmpq	%rsi, %rax
	je	.L108
	movq	%rsi, %rcx
	subq	%rax, %rcx
	cmpq	$31, %rcx
	jle	.L105
.L48:
	vpor	(%rax), %ymm2, %ymm0
	vpcmpeqb	%ymm1, %ymm0, %ymm0
	vptest	%ymm3, %ymm0
	jb	.L117
.L43:
	vpmovmskb	%ymm0, %ecx
	notl	%ecx
	tzcntl	%ecx, %ecx
	addq	%rcx, %rax
	addq	%rcx, %rdx
	vzeroupper
.L21:
	cmpq	%rsi, %rax
	je	.L52
.L36:
	sall	$5, %r9d
	jmp	.L51
	.p2align 4
	.p2align 3
.L118:
	incq	%rax
	incq	%rdx
	cmpq	%rsi, %rax
	je	.L52
.L51:
	movzbl	(%rax), %ecx
	orl	%r9d, %ecx
	cmpb	$97, %cl
	je	.L118
.L78:
	movq	%rax, %r8
.L52:
	testq	%rdx, %rdx
	jne	.L102
.L53:
	movq	%rdi, %r8
.L102:
	movq	-8(%rbp), %rbx
	movq	%r8, %rax
	leave
	.cfi_remember_state
	.cfi_def_cfa 7, 8
	ret
	.p2align 4
	.p2align 3
.L111:
	.cfi_restore_state
	jne	.L53
.L65:
	movzbl	2(%r9), %r9d
	cmpq	%rdi, %rsi
	je	.L53
	movq	%rsi, %rax
	movl	$0, %edx
	subq	%rdi, %rax
	cmpq	$15, %rax
	movq	%rdi, %rax
	jle	.L59
	testb	%r9b, %r9b
	jne	.L77
	movl	$97, %ecx
	vmovd	%ecx, %xmm1
	vpbroadcastb	%xmm1, %xmm1
	jmp	.L62
	.p2align 4
	.p2align 3
.L119:
	addq	$16, %rax
	addq	$16, %rdx
	cmpq	%rsi, %rax
	je	.L52
	movq	%rsi, %rcx
	subq	%rax, %rcx
	cmpq	$15, %rcx
	jle	.L59
.L62:
	vpcmpeqb	(%rax), %xmm1, %xmm0
	vpmovmskb	%xmm0, %ecx
	cmpl	$65535, %ecx
	je	.L119
.L56:
	notl	%ecx
	tzcntl	%ecx, %ecx
	addq	%rcx, %rax
	addq	%rcx, %rdx
	cmpq	%rsi, %rax
	je	.L52
.L59:
	sall	$5, %r9d
	jmp	.L63
	.p2align 4
	.p2align 3
.L120:
	incq	%rax
	incq	%rdx
	cmpq	%rsi, %rax
	je	.L52
.L63:
	movzbl	(%rax), %ecx
	orl	%r9d, %ecx
	cmpb	$97, %cl
	je	.L120
	jmp	.L78
	.p2align 4
	.p2align 3
.L115:
	movabsq	$2314885530818453536, %rbx
	movl	$97, %ecx
	vpcmpeqd	%ymm5, %ymm5, %ymm5
	vmovq	%rbx, %xmm3
	vmovd	%ecx, %xmm2
	vpbroadcastq	%xmm3, %ymm3
	vpbroadcastb	%xmm2, %ymm2
	jmp	.L37
	.p2align 4
	.p2align 3
.L121:
	addq	$64, %rax
	addq	$64, %rdx
	cmpq	%rsi, %rax
	je	.L108
	movq	%rsi, %rcx
	subq	%rax, %rcx
	cmpq	$63, %rcx
	jle	.L28
.L37:
	vpor	(%rax), %ymm3, %ymm0
	vpor	32(%rax), %ymm3, %ymm1
	vpcmpeqb	%ymm2, %ymm0, %ymm0
	vpcmpeqb	%ymm2, %ymm1, %ymm1
	vpand	%ymm1, %ymm0, %ymm4
	vptest	%ymm5, %ymm4
	jb	.L121
	jmp	.L29
	.p2align 4
	.p2align 3
.L112:
	vmovdqu	(%rdi), %xmm0
	testb	%r9b, %r9b
	je	.L14
	movabsq	$2314885530818453536, %rax
	vmovq	%rax, %xmm1
	vpunpcklqdq	%xmm1, %xmm1, %xmm1
	vpor	%xmm1, %xmm0, %xmm0
.L14:
	movl	$97, %eax
	vpcmpeqd	%xmm2, %xmm2, %xmm2
	vmovd	%eax, %xmm1
	vpbroadcastb	%xmm1, %xmm1
	vpcmpeqb	%xmm1, %xmm0, %xmm0
	vptest	%xmm2, %xmm0
	jnb	.L16
	leaq	16(%rdi), %rax
	cmpq	%rsi, %rax
	jnb	.L68
	movq	%rsi, %rcx
	movl	$16, %edx
	subq	%rax, %rcx
	cmpq	$63, %rcx
	ja	.L79
	cmpq	$31, %rcx
	jg	.L18
.L79:
	cmpq	$63, %rcx
	jg	.L20
	jmp	.L21
	.p2align 4
	.p2align 3
.L113:
	movq	%rdi, %rax
	xorl	%edx, %edx
.L18:
	vmovdqu	(%rax), %ymm0
	testb	%r9b, %r9b
	je	.L24
	movabsq	$2314885530818453536, %rbx
	vmovq	%rbx, %xmm1
	vpbroadcastq	%xmm1, %ymm1
	vpor	%ymm1, %ymm0, %ymm0
.L24:
	movl	$97, %ecx
	vpcmpeqd	%ymm2, %ymm2, %ymm2
	vmovd	%ecx, %xmm1
	vpbroadcastb	%xmm1, %ymm1
	vpcmpeqb	%ymm1, %ymm0, %ymm0
	vptest	%ymm2, %ymm0
	jnb	.L26
	addq	$32, %rax
	cmpq	%rsi, %rax
	jnb	.L71
	movq	%rsi, %rcx
	addq	$32, %rdx
	subq	%rax, %rcx
	cmpq	$63, %rcx
	jg	.L20
	cmpq	$31, %rcx
	jg	.L35
	vzeroupper
	jmp	.L21
	.p2align 4
	.p2align 3
.L108:
	vzeroupper
	jmp	.L52
	.p2align 4
	.p2align 3
.L116:
	cmpq	$31, %rcx
	jle	.L105
.L47:
	movl	$97, %ecx
	vpcmpeqd	%ymm2, %ymm2, %ymm2
	vmovd	%ecx, %xmm1
	vpbroadcastb	%xmm1, %ymm1
	jmp	.L49
	.p2align 4
	.p2align 3
.L122:
	addq	$32, %rax
	addq	$32, %rdx
	cmpq	%rax, %rsi
	je	.L108
	movq	%rsi, %rcx
	subq	%rax, %rcx
	cmpq	$31, %rcx
	jle	.L105
.L49:
	vpcmpeqb	(%rax), %ymm1, %ymm0
	vptest	%ymm2, %ymm0
	jb	.L122
	jmp	.L43
	.p2align 4
	.p2align 3
.L28:
	cmpq	$31, %rcx
	jg	.L67
	.p2align 4
	.p2align 3
.L105:
	vzeroupper
	jmp	.L36
	.p2align 4
	.p2align 3
.L110:
	cmpb	$0, _ZZN4ctre4simd8has_avx2EvE8detected(%rip)
	je	.L5
	movzbl	_ZZN4ctre4simd8has_avx2EvE6result(%rip), %eax
.L6:
	testb	%al, %al
	je	.L7
	movl	$2, _ZZN4ctre4simd19get_simd_capabilityEvE17cached_capability(%rip)
	jmp	.L8
	.p2align 4
	.p2align 3
.L77:
	movabsq	$2314885530818453536, %rbx
	movl	$97, %ecx
	vmovq	%rbx, %xmm2
	vmovd	%ecx, %xmm1
	vpunpcklqdq	%xmm2, %xmm2, %xmm2
	vpbroadcastb	%xmm1, %xmm1
	jmp	.L61
	.p2align 4
	.p2align 3
.L123:
	addq	$16, %rax
	addq	$16, %rdx
	cmpq	%rsi, %rax
	je	.L52
	movq	%rsi, %rcx
	subq	%rax, %rcx
	cmpq	$15, %rcx
	jle	.L59
.L61:
	vpor	(%rax), %xmm2, %xmm0
	vpcmpeqb	%xmm1, %xmm0, %xmm0
	vpmovmskb	%xmm0, %ecx
	cmpl	$65535, %ecx
	je	.L123
	jmp	.L56
	.p2align 4
	.p2align 3
.L16:
	vpmovmskb	%xmm0, %edx
	notl	%edx
	tzcntl	%edx, %edx
	leaq	(%rdi,%rdx), %r8
	jmp	.L52
	.p2align 4
	.p2align 3
.L32:
	vpmovmskb	%ymm0, %ecx
	notl	%ecx
	tzcntl	%ecx, %ecx
	addq	%rcx, %rax
	addq	%rcx, %rdx
	jmp	.L33
	.p2align 4
	.p2align 3
.L26:
	vpmovmskb	%ymm0, %ecx
	notl	%ecx
	tzcntl	%ecx, %ecx
	addq	%rcx, %rdx
	leaq	(%rax,%rcx), %r8
	vzeroupper
	jmp	.L52
	.p2align 4
	.p2align 3
.L7:
	cmpb	$0, _ZZN4ctre4simd9has_sse42EvE8detected(%rip)
	je	.L9
	movzbl	_ZZN4ctre4simd9has_sse42EvE6result(%rip), %eax
.L10:
	testb	%al, %al
	je	.L124
	movl	$1, _ZZN4ctre4simd19get_simd_capabilityEvE17cached_capability(%rip)
	jmp	.L65
	.p2align 4
	.p2align 3
.L5:
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
	jmp	.L6
	.p2align 4
	.p2align 3
.L68:
	movq	%rax, %r8
	jmp	.L102
.L71:
	movq	%rax, %r8
	vzeroupper
	jmp	.L102
.L9:
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
	jmp	.L10
.L124:
	movl	$0, _ZZN4ctre4simd19get_simd_capabilityEvE17cached_capability(%rip)
	jmp	.L53
.L114:
	cmpq	$31, %rcx
	jg	.L35
	jmp	.L36
	.cfi_endproc
.LFE11313:
	.size	_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE, .-_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE
	.text
	.p2align 4
	.globl	_Z14match_standardRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE
	.type	_Z14match_standardRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, @function
_Z14match_standardRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE:
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
	jg	.L138
.L126:
	xorl	%eax, %eax
	cmpq	%rbp, %rbx
	je	.L125
	xorl	%eax, %eax
	.p2align 4
	.p2align 3
.L131:
	cmpb	$97, (%rbx)
	jne	.L130
	incq	%rbx
	incq	%rax
	cmpq	%rbx, %rbp
	jne	.L131
	testq	%rax, %rax
	setne	%al
.L125:
	movq	8(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L139
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
.L130:
	.cfi_restore_state
	testq	%rax, %rax
	setne	%al
	cmpq	%rbx, %rbp
	sete	%dl
	andl	%edx, %eax
	jmp	.L125
	.p2align 4
	.p2align 3
.L138:
	leaq	4(%rsp), %r12
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	movq	%r12, %rdx
	call	_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE
	cmpq	%rax, %rbx
	je	.L127
	cmpq	%rax, %rbp
	sete	%al
	jmp	.L125
	.p2align 4
	.p2align 3
.L127:
	movq	%r12, %rdx
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	call	_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE
	cmpq	%rax, %rbx
	je	.L126
	cmpq	%rax, %rbp
	sete	%al
	jmp	.L125
.L139:
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE10473:
	.size	_Z14match_standardRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, .-_Z14match_standardRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE
	.p2align 4
	.globl	_Z16match_with_checkRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE
	.type	_Z16match_with_checkRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, @function
_Z16match_with_checkRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE:
.LFB10542:
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
	jg	.L153
.L141:
	xorl	%eax, %eax
	cmpq	%rbp, %rbx
	je	.L140
	xorl	%eax, %eax
	.p2align 4
	.p2align 3
.L146:
	cmpb	$97, (%rbx)
	jne	.L145
	incq	%rbx
	incq	%rax
	cmpq	%rbx, %rbp
	jne	.L146
	testq	%rax, %rax
	setne	%al
.L140:
	movq	8(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L154
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
.L145:
	.cfi_restore_state
	testq	%rax, %rax
	setne	%al
	cmpq	%rbx, %rbp
	sete	%dl
	andl	%edx, %eax
	jmp	.L140
	.p2align 4
	.p2align 3
.L153:
	leaq	4(%rsp), %r12
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	movq	%r12, %rdx
	call	_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE
	cmpq	%rax, %rbx
	je	.L142
	cmpq	%rax, %rbp
	sete	%al
	jmp	.L140
	.p2align 4
	.p2align 3
.L142:
	movq	%r12, %rdx
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	call	_ZN4ctre4simd25match_pattern_repeat_simdINS_9characterILc97EEELm1ELm0EN9__gnu_cxx17__normal_iteratorIPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEESE_EET2_SF_T3_RKNS_5flagsE
	cmpq	%rax, %rbx
	je	.L141
	cmpq	%rax, %rbp
	sete	%al
	jmp	.L140
.L154:
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE10542:
	.size	_Z16match_with_checkRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, .-_Z16match_with_checkRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE
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
