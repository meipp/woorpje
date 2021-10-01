(set-logic QF_S)

(declare-fun A () String)

(assert (= A A))

(assert (str.in_re (str.++ "c" A)
    (re.union
        (str.to_re "bb")
        (str.to_re "a")
    )
) )
