(set-logic QF_S)
(declare-fun A () String)
(assert (str.in_re A
    (re.union
        (re.* (str.to_re ""))
    )
)
(check-sat)