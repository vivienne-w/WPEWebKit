;; Per-directory local variables for GNU Emacs 23 and later.

;; Syntax: ((MODE (VAR . VAL) ...) ...)
;; MODE is a symbol like `c-mode', or `nil' for all modes.

((c-mode
  (indent-tabs-mode . nil)
  (c-basic-offset . 4))
 (c++-mode
  (indent-tabs-mode . nil)
  (c-basic-offset . 4))
 (java-mode
  (indent-tabs-mode . nil)
  (c-basic-offset . 4))
 (change-log-mode
  (indent-tabs-mode . nil))
 (nil
  (fill-column . 160)
;  (ccls-executable . "/home/calvaris/.bin/webkit-ccls")
;  (ccls-initialization-options . (:compilationDatabaseDirectory "/app/webkit/WebKitBuild/Release"
;                                  :cache (:directory "/app/webkit/.ccls-cache")))
  (compile-command . "~/Igalia/Metrological/scripts/build-wpe.sh")))
