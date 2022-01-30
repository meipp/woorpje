(set-logic QF_S)
(declare-fun A () String)
(assert (str.in_re (str.++ "tewczkvs" (str.++ A "setutdorumiyeqj")) (re.++ (re.++ (re.* (re.union (re.union (re.++ (str.to_re "tewczkvssetut") (str.to_re "dorum")) (re.* (str.to_re ""))) (re.union (re.++ (str.to_re "zx") (str.to_re "mffq")) (str.to_re "ekvbvytzoqagbppaqva")))) (re.* (re.* (re.* (re.* (str.to_re "w")))))) (str.to_re "iyeqj"))))
(check-sat)