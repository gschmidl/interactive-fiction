/*
 * Amiga MUD
 *
 * Copyright (c) 1991 by Chris Gray
 */

/*
 * books - define the books and jars in this world.
 */

private proc bookGet(thing book)status:
    thing me;
    int count;
    string volume;

    me := Me();
    count := me@p_pBookCount;
    if count = 0 then
	AddTail(me@p_pCarrying, o_kludgeBook);
    fi;
    count := count + 1;
    me@p_pBookCount := count;
    if not book@p_oVisible then
	book@p_oVisible := true;
	Print("Please remember to replace the volume when you are done.\n");
	count := me@p_pBookCombination;
	volume := book@p_oName;
	if count = 0 and volume = "Five,5;Volume" then
	    count := 5;
	elif count = 5 and volume = "Nine,9;Volume" then
	    count := 9;
	elif count = 9 and volume = "Two,2;Volume" then
	    count := 2;
	    Print("The high bookcase slides aside to reveal a doorway "
		    "heading east!\n");
	elif book@p_oEncyclopedia then
	    if count = 2 then
		Print("The high bookcase slides back into place, "
			"closing off the doorway to the east.\n");
	    fi;
	    count := 0;
	fi;
	me@p_pBookCombination := count;
    fi;
    continue
corp;

private proc bookDrop(thing book)status:

    if Here() = book@p_oReplaceLoc then
	Print("You really should replace the book on the shelf.\n");
    fi;
    dropABook();
    continue
corp;

private proc createBook(thing room; bool ency; string name, desc, text)void:
    thing book;

    book := CreateThing(nil);
    book@p_oName := name;
    if desc ~= "" then
	book@p_oDesc := desc;
    fi;
    AddTail(room@p_rContents, book);
    book@p_oReplaceLoc := room;
    book@p_oText := text;
    book@p_oVisible := false;
    book@p_oCarryable := true;
    book@p_oGetAction := bookGet;
    book@p_oDropAction := bookDrop;
    if ency then
	book@p_oEncyclopedia := true;
    fi;
corp;

private proc jarGet(thing jar)status:
    thing me;
    int count;

    me := Me();
    count := me@p_pJarCount;
    if count = 0 then
	AddTail(me@p_pCarrying, o_kludgeJar);
    fi;
    count := count + 1;
    me@p_pJarCount := count;
    if not jar@p_oVisible then
	jar@p_oVisible := true;
	Print("Please remember to replace the jar when you are done.\n");
    fi;
    continue
corp;

private proc jarLook(thing jar)status:
    int count;

    count := jar@p_oPortionCount;
    Print("The " + FormatName(jar@p_oName));
    if count = 0 then
	Print(" is empty.\n");
    elif count = 1 then
	Print(" has only one portion left.\n");
    else
	Print(" has ");
	Print(IntToString(count));
	Print(" portions left.\n");
    fi;
    continue
corp;

private proc jarDrop(thing jar)status:

    if Here() = jar@p_oReplaceLoc then
	Print("You really should replace the jar on the shelf.\n");
    fi;
    dropAJar();
    continue
corp;

private proc createJar(thing room; int count; string mixCode; string name)void:
    thing jar;

    jar := CreateThing(nil);
    jar@p_oPortionCount := count;
    jar@p_oName := name;
    AddTail(room@p_rContents, jar);
    jar@p_oReplaceLoc := room;
    jar@p_oCarryable := true;
    jar@p_oVisible := false;
    jar@p_oGetAction := jarGet;
    jar@p_oLookAction := jarLook;
    jar@p_oDropAction := jarDrop;
    jar@p_oJar := true;
    jar@p_oMixCode := mixCode;
corp;

createBook(r_study, true, "One,1;Volume",
    "Volume one covers A and B.\n",
    "...\n"
    "Belgium is a small country on the northwest coast of Europe. "
    "It is famous for it's delicious chocolates and for the city of "
    "Brussels, which is the seat of the European Economic Community...\n")$
createBook(r_study, true, "Two,2;Volume",
    "Volume two covers C to F.\n",
    "...\n"
    "England is a island nation off the coast of continental Europe. "
    "It's capital, London, is one of the world's largest cities. "
    "British tradition and history are some of the richest in the world...\n")$
createBook(r_study, true, "Three,3;Volume",
    "Volume three covers G to K.\n",
    "...\n"
    "Germans are known as some of the finest mechanics and engineers in the "
    "world. Germany is also a troublesome country, having been one of the "
    "major players in both world wars. "
    "Germany's recent re-unification makes it the single most important "
    "factor in the new Europe...\n")$
createBook(r_study, true, "Four,4;Volume",
    "Volume four covers L and M.\n",
    "...\n"
    "Lebanon, home of the famed Cedars of Lebanon (long since cut down), "
    "is a troubled country. Located on the eastern end of the "
    "Mediterranean Sea, Lebanon, and its capital, Beirut, have known strife "
    "throughout their history...\n")$
createBook(r_study, true, "Five,5;Volume",
    "Volume five covers N and O.\n",
    "...\n"
    "Oman is the south-east tip of the Arabian peninsula. Rich in oil, Oman "
    "is a significant player in today's world affairs...\n")$
createBook(r_study, true, "Six,6;Volume",
    "Volume six covers P to R.\n",
    "...\n"
    "Rumania, located in south-eastern Europe, is prominent lately for its "
    "turbulent politics, having thrown off the yoke of communism for the "
    "confusion and insecurity of a democracy. A perpetual feud with "
    "Hungary keeps things interesting...\n")$
createBook(r_study, true, "Seven,7;Volume",
    "Volume seven covers S.\n",
    "...\n"
    "Switzerland, known for its stance as a neutral country, is home to "
    "many banks offering unique international services. The city of Geneva "
    "is the site of many international conferences, and is often a symbol "
    "of peace. The storied Swiss mountain, 'the Matterhorn' is one of the "
    "most intimidating in Europe...\n")$
createBook(r_study, true, "Eight,8;Volume",
    "Volume eight covers T to V.\n",
    "...\n"
    "Uganda is located in east-central Africa surrounded by Sudan, Kenya, "
    "Tanzania, Rwanda, and Zaire. "
    "Its long-time leader, Idi Amin Dada, is known for his atrocities against "
    "members of tribes other than his own...\n")$
createBook(r_study, true, "Nine,9;Volume",
    "Volume nine covers W to Z.\n",
    "...\n"
    "Xanadu, located on the northern end of the Arctic ocean, is kept at a "
    "near-tropical temperature by its many hot springs and volcanoes. "
    "Difficult to reach even in modern times, this remote country has "
    "nevertheless contributed much to modern culture. "
    "Conquered in the mid 13th century by Kublai Khan, Xanada has since "
    "maintained its oriental influences...\n")$
createBook(r_study, true, "Ten,10;Volume",
    "Volume ten is the index.\n",
    "This volume has thousands of names, all sorted in alphabetical order, "
    "with indications of where the entries can be found in the main "
    "volumes of the encyclopedia.\n")$

createBook(r_study, false, "Mechanics;Quantum",
    "This is a pretty imposing looking volume - read at your own risk.\n",
    "... This is the Schrodinger equation of the density operator. Although "
    "formally similar to the Heisenberg equation (VIII.40) from which it "
    "differs only in the sign of the commutator, it should not be confused "
    "with the latter. The quantities which occur in it are operators of the "
    "Schrodinger \"representation\". ...\n")$
createBook(r_study, false, "Mechanics;Classical",
    "One look at this book convinces you that mechanics is not as simple "
    "as you always though it was.\n",
    "... A number of interesting consequences follow from Eqs. (8-79) and "
    "(8-80). If L[x] and L[y] are constants of the motion, so that their "
    "Poisson brackets with H vanish, it follows then from Poisson's theorem "
    "that L[z] = [L[x],L[y]] is also a constant of the motion. If any two "
    "components of the angular momentum are constant, the total angular "
    "momentum vector is conserved. ...\n")$
createBook(r_study, false, "Tools;Software",
    "This book is more readable than the others - fewer funny symbols.\n",
    "... We emphasize that these \"features\" are 'ad hoc' decisions made "
    "as we implemented the pattern builder. A number of curious situations "
    "turned out to be unspecified, as is often the case, and had to be "
    "resolved during coding. We chose to complete the specification in what "
    "appeared to be the most convenient way for the user. ...\n")$
createBook(r_study, false, "Calculus;Advanced",
    "This innocent-looking volume is stained with the tears of frustration.\n",
    "... For the Taylor polynomial to be a good approximation to 'f', "
    "uniformly on an inverval 'I', the remainder, 'R[n]' must be uniformly "
    "small there. The importance of the theorem lies in the fact that by "
    "having a formula for 'R[n]', we are thereby able to estimate its size. "
    "For example, let us find a polynomial which approximates 'e^x' on the "
    "interval [-1,1] accurately to within .005. ...\n")$
createBook(r_study, false, "Peasants",
    "This slim anthropology book seems a bit out of place here.\n",
    "... 'Prebendal domain' over land differs from patrimonial domain in that "
    "it is not heritable, but granted to officials who draw tribute from the "
    "peasantry in their capacity as servants of the state. Such domains are "
    "not lineage domains, then; rather they represent grants of income - "
    "'prebends' - in return for the exercise of a particular office. ...\n")$

createBook(r_hiddenStudy, false, "Necronomicon",
    "The Necronomicon is a deep, wierdly glowing black. It's cover is "
    "incised with a circle containing signs of the zodiac.\n",
    "... And the conjuration of the Three must be made, thus: \n"
    "  ISS MASS SSARATI SHA MUSHI LIPSHURU RUXISHA LIMNUT!\n"
    "  IZIZANIMMA ILANI RABUTI SHIMA YA DABABI!\n"
    "  DINI DINA ALAKTI LIMDA!\n"
    "  ALSI KU NUSHI ILANI MUSHITI!\n"
    "  IA MASS SSARATI ISS MASS SSARATI BA IDS MASS SSARATU!\n"
    "...\n")$
createBook(r_hiddenStudy, false, "Magica;Ars.Ars",
    "Ars Magica is blood red, with burning yellow lettering. The combination "
    "seems to make the eyes shy away.\n",
    "... As the apprentice will soon discover, great care must be taken to "
    "ensure that formulae are followed exactly. Changes in quantity can "
    "result in uncontrolled spells. Ingredients must be of good quality - "
    "bad ingredients can turn a spell around. More leeway exists in the "
    "verbal component, so long as errors do not result in another active "
    "spell - an apprentice who accidentally conjures a demon is not likely "
    "to repeat the mistake! ...\n")$
createBook(r_hiddenStudy, false, "Dead;Book,of,the.ANI;Papyrus,of.Papyrus",
    "The Book of the Dead, gleaned from the papyrus of ANI, is an old volume "
    "bound in sandy-coloured leather. It's spine is filled with "
    "hieroglyphics.\n",
    "... The skies lower, the stars tremble, the Archers quake, the bones "
    "of Akeru-gods tremble, and those who are with them are struck dumb when "
    "they see Unas rising up as a soul, in the form of the god who liveth "
    "upon his fathers and who maketh to be his food his mothers. Unas is the "
    "lord of wisdom, and his mother knoweth not his name. ...\n")$
createBook(r_hiddenStudy, false, "Weatherwax;Wisdom,of.Wisdom",
    "The Wisdom of Weatherwax is a slim book, with an ugly green "
    "cover and harsh, crabbed lettering done in a faded brown ink.\n",
    "...\n"
    "Once people lose their respect, it means trouble.\n\n"
    "Trouble is, just because things are obvious doesn't mean they're true.\n"
    "\n"
    "You'd have to be a fool to be born a king.\n"
    "...\n")$

createJar(r_hiddenStudy, 2, "f",
    "jar;fillet,of,fenny,snake.snake;fillet,of,fenny,jar.fillet")$
createJar(r_hiddenStudy, 2, "e",
    "jar;eye,of,newt.newt;eye,of,jar.eye")$
createJar(r_hiddenStudy, 5, "t",
    "jar;toe,of,frog.frog;toe,of,jar.toe")$
createJar(r_hiddenStudy, 7, "w",
    "jar;wool,of,bat.bat;wool,of,jar.wool")$
createJar(r_hiddenStudy, 2, "u",
    "jar;tongue,of,dog.dog;tongue,of,jar.tongue")$
createJar(r_hiddenStudy, 3, "d",
    "jar;scale,of,dragon.dragon;scale,of,jar.scale")$
createJar(r_hiddenStudy, 2, "w",
    "jar;tooth,of,wolf.jar;teeth,of,wolf.wolf;tooth,teeth,of,jar.tooth,teeth")$
createJar(r_hiddenStudy, 3, "r",
    "jar;root,of,hemlock.hemlock;root,of,jar.root")$
createJar(r_hiddenStudy, 3, "g",
    "jar;gall,of,goat.goat;gall,of,jar.gall")$
createJar(r_hiddenStudy, 5, "s",
    "jar;slips,of,yew.yew;slips,of,jar.slip")$
