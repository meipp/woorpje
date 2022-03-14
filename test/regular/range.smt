(set-logic QF_S)

(declare-fun A () String)

(assert (= A A))

(assert (str.in_re (str.++  A)
    (re.union 
        (re.range "/" "=")
        (str.to_re "0")
    )
))