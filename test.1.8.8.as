	lw	0	7	Large
	lw	0	6	Neg1
lp	beq	0	7	end
	lw	7	5	ArrP
	add	6	7	7
	sw	7	5	ArrP
	beq	0	0	lp
end	halt
One	.fill	1
Neg1	.fill	-1
Large	.fill	500
ArrP	.fill	Arr
Arr	.fill	0
	