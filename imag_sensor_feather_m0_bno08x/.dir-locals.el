;;; Directory Local Variables            -*- no-byte-compile: t -*-
;;; For more information see (info "(emacs) Directory Variables")

((auto-mode-alist . (("\\.h\\'" . c++-mode)
		     ("\\.ino\\'" . c++-mode)))
 (c++-mode . (
	      ;; c++-mode coding style
	      (c-default-style . "linux")
	      (c-basic-offset . 4)
	      (indent-tabs-mode . nil)
	      (eval . (progn
			(c-set-offset 'innamespace '-)
			(c-set-offset 'inline-open '0)
			(c-set-offset 'substatement-open '0)
			(c-set-offset 'brace-list-open '0)
			(c-set-offset 'inher-cont 'c-lineup-multi-inher)
			(c-set-offset 'arglist-cont-nonempty 'c-lineup-arglist-intro-after-paren)
			(c-set-offset 'template-args-cont '+)
			))

	      ;; arduino-cli configuration
	      (eval . (arduino-cli-mode))
	      (arduino-cli-default-fqbn . "adafruit:samd:adafruit_feather_m0")
	      (arduino-cli-default-port . "/dev/cu.usbmodem124301"))))
