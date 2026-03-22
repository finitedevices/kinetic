STEP = 1

start:
	; Set initial value in B to ASCII A
	ldb	%A

loop:
	; Output B
	lda	b
	int	$FF

	; Add STEP to B
	add	STEP
	ldb	a

	; Check if B ASCII code is at Z
	lda	b
	eq	%Z+1
	if	end

	; Loop back to start
	jump	loop

end:
	; Print newline
	lda	10
	int	$FF

stop:
	; Enter infinite loop
	jump	stop