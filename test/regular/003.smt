(set-logic QF_S)

(declare-fun A () String)

(assert (= A A))

(assert (str.in_re (str.++ A "de")  (re.* (str.to_re "ced") ) ) )
