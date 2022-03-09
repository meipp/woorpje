(set-logic QF_S)

(declare-fun A () String)

(assert (= A A))

(assert (str.in_re (str.++  A )

        (re.union
            (str.to_re "b")
            (str.to_re "c")
        )
    
) )
