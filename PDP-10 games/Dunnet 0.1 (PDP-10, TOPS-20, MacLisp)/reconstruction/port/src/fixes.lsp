;;; fixes.lsp - corrections loaded after foo.lsp unless --no-fixes is given.
;;; Each function is the original with the marked change; numbers are
;;; octal, as in foo.lsp.

;; FIX: "take all" in a room holding something that cannot be taken (the
;; silicon and the symbol in the solar room) looped forever, because the
;; loop waited for the room to become empty.  It now takes each movable
;; object once.
(defun take ()

  (setq obj (disp 2 cmd))

  (cond (
	 (eq obj nil)
	 (princ verb) (princ " what?"))
	(
	 (eq obj 'all)
	 (setq cmd '(foo bar))
	 (cond (
		(eq (disp room items) 0)
		(princ "I found nothing.") (crlf))
	       (t
		(do ((l (disp room items) (cdr l))            ; FIX
		     (found nil))
		    ((atom l)
		     (cond ((not found) (princ "I found nothing.") (crlf))))
		  (cond ((not (greaterp (car l) 200))
			 (setq found t)
			 (princ (disp (car l) objshd))
			 (princ "  ")
			 (setq all (car l))
			 (take) (crlf)))))))
	 (t
  (setq objnum (objref obj))
  (cond (
	 (not (eq all nil))
	 (setq objnum all)
	 (setq all nil)))
  (cond
   ((member objnum inventory) (princ "You already have it, wedge!")
				  (setq foo 1))
   ((atom (disp room items)) (princ "I can't see anything here, let alone a ") (princ obj))


   ((member objnum (disp room items))
    (cond (
	   (greaterp objnum 200)
	   (cond (
		  (eq objnum 201) (princ "The piece of silicon is stuck to the wall"))
		 (
		  (eq objnum 202) (princ "You can't remove a piece of the floor!"))))
	  (t
	   (princ "Taken.")
	   (cond
	    ((eq breaking-beam objnum)
	     (setq breaking-beam nil)
	     (setq beam 'shining) (crlf)
	     (cond ((eq objnum 3)
		    (princ "The beam now hits the floor and the passage disappears"))
		   (t
		    (princ "The beam once again hits the floor")))))
	   (cond
	    ((not (eq (car (assq objnum treasure-items)) nil))
	     (cond (
		    (not (member objnum scored))
		    (setq scored (add objnum scored))
		    (setq score (+ score (cdr (assq objnum treasure-items))))))))
	   (rid objnum) (setq inventory (add objnum inventory)))))
   (t
    (princ "I can't see that here."))))))

;; FIX: the third case set F instead of FOO, so the console could find the
;; imp up although the chaosnet connection had been dropped.
(defun console ()
  (cond (
	 (= room 6)
	 (cond (
		(eq inventory nil)
		(setq foo nil))
	       ((member 1 inventory)
		(setq foo T))
	       (t
		(setq foo nil)))                             ; FIX: was F
	 (readch)
	 (cond (
		(greaterp room 7)
		(setq sys 'dungeon))
	       (t
		(setq sys 'sally)))
	 (tops20 sys foo))))

;; FIX: READCHR does not exist; READCH reads the character after the
;; interrupt character ~ ("~c" leaves the dungeon for the computer room).
(defun conn-term ()
  (cond (
	 (not (member room light-rooms))
	 (setq chr (readch))                                 ; FIX: was READCHR
	 (cond (
		(and (not (eq chr '/c)) (not (eq chr '/C)))
		(princ ""))
	       (t
		(crlf) (crlf)
		(princ "Connection closed.")
		(setq room 6)
		(crlf)
		(princ "Some force throws you off the console"))))))

;; FIX: in a room without objects (its entry is 0, not a list), examining
;; something not carried called MEMBER on 0, which is an error.
(defun examin ()
  (let (objnum obj)
    (setq obj (disp 2 cmd))
    (setq objnum (objref obj))
    (cond (
	   (and (not (member objnum inventory))
		(or (atom (disp room items))                 ; FIX
		    (not (member objnum (disp room items)))))
	   (princ "I don't see that here."))
	  (t
	   (cond (
		  (eq objnum 202)
		  (cond (
			 (eq beam 'broken)
			 (princ "The insciption says: The shovel has more than one purpose"))
			(t
			 (princ "It is too bright in here."))))
		  (t
		   (princ "I don't see anything peculiar about it.")))))))

;; FIX: LIST-WORDS took the line apart by dropping the first and last
;; characters of its EXPLODE, which only works when MacLisp prints the line
;; between |bars| (lowercase letters, spaces).  A single word in capitals
;; ("N") was an error.  Now letters and digits form the words and anything
;; else separates them (octal character codes: 60-71 digits, 101-132 and
;; 141-172 letters, 40 space).
(defun list-words (line)
  (readlist
   (cons '/(
	 (nconc
	  (mapcar (function (lambda (ch)
			      (cond ((or (and (> ch 57) (< ch 72))
					 (and (> ch 100) (< ch 133))
					 (and (> ch 140) (< ch 173)))
				     ch)
				    (t 40))))
		  (exploden line))
	  (list '/))))))

;; FIX: the DIRECTORY command left FILES unset (an error) when nothing was
;; carried, and a stale count after the inventory had shrunk.  The function
;; is otherwise unchanged.
(defun tops20 (sys imp)
  
  (setq comm nil)
  (terpri) (princ "Tops-20 Command Processor 0(a1)")
  (terpri) (princ "[Command RETURN defined]")
    (do ((foo ()))
      ((eq comm 'return))
      
      (let-comred "@"
			     (return (prog1 
				      (setq comm (comred 'command)
			       )
	
    (cond (
	   (eq comm 'directory)
	   (comred-force-guideword "Of files")
	   (comred 'confirm)
	   (princ
"
ps:<luser>
")
	   (setq files 0)                                    ; FIX
	   (do ((foo 1 (1+ foo)))
	       ((= foo (1+ (count inventory))) ())
	       (princ (nth (1- (nth (1- foo) inventory)) objshd))
	       (princ ".obj.1")
	       (setq files foo)
	     (crlf))
	   (crlf) (princ "Total of ") (princ files) (princ " files"))

(
           (eq comm 'ftp)
	   (cond (
		  (eq sys 'bally)
		  (princ "Ftp not available on Mit-Sally")
		  (princ ", sorry.") (crlf))
		 (t
		  (princ "MIT-") (princ sys)
		  (princ " FTP service") (crlf)
		  (let-comred "*" (return (prog1 
				      (setq fomm (comred 'ftp))
	   (cond (
		  (eq fomm 'connect)
		  (comred-force-guideword "To host")
		  (setq concmd (comred 'ftpcon))
		  (cond (
			 (eq concmd sys)
			 (princ "Cannot connect to your own system, silly"))
			(t
			 (princ "Connection opened. Assuming Object-file-transfer")
			 (setq connected t)
			 (princ "-> ") (princ concmd) (princ " ftp service"))))
		 (
		  (eq fomm 'login)
		  (comred-force-guideword "Username")
		  (setq name (comred 'text-string))
		  (comred-force-guideword "Password")
		  (setq pass (comred 'text-string))
		  (cond (
			 (neq name 'luser)
			 (princ "-> Does not match directory or username"))
			(t
			 (cond (
			  (neq pass 'resul)
			  (princ "-> ?Invalid Password"))
			       (t
				(Princ "-> User LUSER Logged in")
				(setq login t))))))
		 (					
		  (eq fomm 'send)
		  (comred-force-guideword "From local file")
		  (defspec files nil "File name" "File not found")
		  (do ((foo 1 (1+ foo)))
		      ((eq foo (1+ (count inventory))))
		    (setq ob (nth (1- (nth (1- foo) inventory)) objshd))
		    (setq barf (implode (nconc (exploden ob)
					       (exploden ".obj"))))
		    (comspec-add-items 'files (list barf) t))
		  (setq file (comred 'files))
		  (setq ob (cdr (plib:sassoc file filest)))
		  (setq frob room)
		  (setq room (cdr (assq concmd hostroom)))
		  (place (objref ob))
		  (setq room frob)))))))))

	   ((eq comm 'information)
	   (comred-force-guideword "About")
	   (setq sinfo (comred 'info))
	   (comred 'confirm)
	   (cond (
		  (eq sinfo 'directory)
		  (princ 
"Name Ps:<Luser>
Password - RESUL
Working disk storage page limit - 0
Perminant disk storage page limit - 0
Chaos-net-user
Absolute-arpanet-sockets
Account default set for LOGIN players
Last login - Never logged in
"))
		 (
		  (eq sinfo 'arpanet)
		  (princ "%No arpanet"))
		 (
		  (eq sinfo 'chaosnet)
		  (cond (
			(not (eq imp t))
			(princ "The imp is down."))
			(t
		  (princ "The imp is up"))))))

	  ((or (eq comm 'tn) (eq comm 'telnet))
	   (comred 'confirm)
	   (setq cmd nil)
	   (do ((loop-var 0 ()))
	       ((eq cmd 'exit) ())
	  (setq telnet-mode
		(let-comred "TELNET>"
			    (setq cmd (comred 'tnet))
			    (cond (
				   (and (not (eq cmd nil)) 
					(not (eq cmd 'exit)))

			    (comred-force-guideword "Network")
			    (setq ccom (comred 'connect))
			    (cond (
				   (eq ccom 'arpanet)
				    (comred-force-guideword "host")
				    (setq foo (comred 'arpa))
				    (comred 'confirm)
				    (princ "Trying...")
				    (sleep 2) (princ "Refused"))
				  (t
				   (comred-force-guideword "host")
				   (setq foo (comred 'chaos))
				   (comred 'confirm)
				   (cond ((eq foo 'dungeon)
					 (princ "Trying...")
					 (sleep 2)
					 (cond (
						(eq imp t)
						(princ "open")
     			(setq room 7) (setq cmd 'exit) (setq comm 'return)
			(setq state looking) (terpri))
					       (t
						(princ "no chaosnet"))))
					 (t
					  (princ "Trying...")
					  (sleep 2)
					  (princ "Refused")))))))))))))))))
