(provide 'subtilis)
(setq subtilis-highlights
      '(
	("\\(REM\\|rem\\).*$" . font-lock-comment-face)
	("[a-zA-Z]@[a-zA-Z_0-9]*\\([$%&]\\)?" . font-lock-type-face)
	("PROC[^([:space:]]+\\|FN[^([:space:]]+\\|REC[^([:space:]]+" . font-lock-function-name-face)
	("ABS\\|ACS\\|ADVAL\\|AND\\|ASC\\|ASN\\|ATN\\|BEAT\\|BEATS\\|BGET#\\|BPUT#\\|BY\\|CALL\\|CASE\\|CHR$\\|CIRCLE\\|CLG\\|CLOSE#\\|CLS\\|COLOR\\|COLOUR\\|COS\\|COUNT\\|DEF\\|DEG\\|DIM\\|DIV\\|DRAW\\|ELLIPSE\\|ELSE\\|END\\(PROC\\|IF\\|CASE\\|WHILE\\|RANGE\\|ERROR\\)?\\|EOF#\\|EOR\\|ERL\\|ERR\\|ERROR\\|EVAL\\|EXP\\|EXT#\\|FILL\\|FOR\\|GCOL\\|GET\\|GET$\\|GET$#\\|IF\\|INKEY\\|INKEY$\\|INPUT\\|INPUT#\\|INSTR\\|INT\\|LEFT$\\|LEN\\|LET\\|LINE\\|LN\\|LOCAL\\|LOG\\|MID$\\|MODE?\\|MOUSE\\|MOVE\\|NEXT\\|NOT\\|OFF?\\|ON\\|ONERROR\\|OPENIN\\|OPENOUT\\|OPENUP\\|OR(IGIN)?\\|OSCLI\\|OTHERWISE\\|PLOT\\|POINT\\|POS\\|PRINT\\|PRINT#\\|PTR#\\|QUIT\\|RAD\\|RANGE\\|RECTANGLE\\|REPEAT\\|REPORT\\|REPORT$\\|RETURN\\|RIGHTS$\\|RND\\|SGN\\|SIN\\|SOUND\\|SPC\\|SQR\\|STEP\\|STEREO\\|STOP\\|STR$\\|STRING$\\|SUM\\|SUMLEN\\|SWAP\\|SYS\\|TAB\\|TAN\\|TEMPO\\|THEN\\|TIME\\|TIME$\\|TINT\\|TO\\|TYPE\\|UNTIL\\|USR\\|VAL\\|VDU\\|VOICES\\|VPOS\\|WAIT\\|WHEN\\|WHILE\\|WIDTH\\|abs\\|acs\\|adval\\|and\\|asc\\|asn\\|atn\\|beat\\|beats\\|bget#\\|bput#\\|by\\|call\\|case\\|chr$\\|circle\\|clg\\|close#\\|cls\\|color\\|colour\\|cos\\|count\\|def\\|deg\\|dim\\|div\\|draw\\|ellipse\\|else\\|end\\(case\\|error\\|if\\|proc\\|range\\while\\)?\\|eof#\\|eor\\|erl\\|err\\|error\\|eval\\|exp\\|ext#\\|fill\\|for\\|gcol\\|get\\|get$\\|get$#\\|if\\|inkey\\|inkey$\\|input\\|input#\\|instr\\|int\\|left$\\|len\\|let\\|line\\|ln\\|local\\|log\\|mid$\\|mode?\\|mouse\\|move\\|next\\|not\\|off?\\|on\\|onerror\\|openin\\|openout\\|openup\\|or\\(igin\\)?\\|oscli\\|otherwise\\|plot\\|point\\|pos\\|print\\|print#\\|ptr#\\|quit\\|rad\\|range\\|rectangle\\|repeat\\|report\\|report$\\|return\\|rights$\\|rnd\\|sgn\\|sin\\|sound\\|spc\\|sqr\\|step\\|stereo\\|stop\\|str$\\|string$\\|sum\\|sumlen\\|swap\\|sys\\|tab\\|tan\\|tempo\\|then\\|time\\|time$\\|tint\\|to\\|type\\|until\\|usr\\|val\\|vdu\\|voices\\|vpos\\|wait\\|when\\|while\\|width" . font-lock-keyword-face)
	("TRUE\\|true\\|FALSE\\|false\\|PI\\|pi\\|[0-9]*\\(\\.[0-9]+\\)?" . font-lock-constant-face)
	("[a-zA-Z][a-zA-Z_0-9]*\\([$%&]\\)?" . font-lock-type-face)
        ))


(define-derived-mode subtilis-mode fundamental-mode "subtilis"
  "major mode for Subtilis."
  (setq font-lock-defaults '(subtilis-highlights)))







