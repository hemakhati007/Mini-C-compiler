	.def	 @feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
@feat.00 = 1
	.def	 _main;
	.scl	2;
	.type	32;
	.endef
	.text
	.globl	_main
	.align	16, 0x90
_main:                                  # @main
# BB#0:
	subl	$16, %esp
	movl	$10, 8(%esp)
	movl	$1092616192, 4(%esp)    # imm = 0x41200000
	movl	$10, (%esp)
	xorl	%eax, %eax
	addl	$16, %esp
	ret


