(set-logic QF_S)
(declare-fun A () String)
(assert (str.in_re A
    (re.union      
        ((_ re.loop 4 5) re.union (str.to_re "a") (str.to_re "b") )
    )
))