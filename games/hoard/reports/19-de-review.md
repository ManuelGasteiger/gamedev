# German seeds for a native read

All 30 seed scenarios, every string: English (persons male), then German with every person male (m) and every person female (f); items take the same gender (Aschenkelch m, Aschenschale f). Names are sex-neutral test names. Rendered by the core (`python3 tools/text/review.py --bin build/hoard-sim docs/scenarios/seed-*.json`). Seeds 1 to 20 are the 2026-10-24 draft with key and token fixes; seeds 21 to 30 and the six reveal lines are new. Nothing here is native-reviewed yet.

Mark a defect inline with **DE:** and the corrected German, or a note.

## scribe_among_hostages

**headline**  
EN: A scribe among the hostages  
m: Ein Schreiber unter den Geiseln  
f: Eine Schreiberin unter den Geiseln

**body**  
EN: Aldric of Kell has spent the days since the taking writing on the cave wall with a burnt stick: the names of the dead at Ashford, the year, the weather. Asked why, he says someone should keep the record, and that nobody else here can read.  
m: Aldric von Kell verbringt die Tage seit der Gefangennahme damit, mit einem verbrannten Stock an die Höhlenwand zu schreiben: die Namen der Toten von Ashford, das Jahr, das Wetter. Nach dem Grund gefragt, sagt er, jemand müsse Buch führen, und sonst könne hier niemand lesen.  
f: Aldric von Kell verbringt die Tage seit der Gefangennahme damit, mit einem verbrannten Stock an die Höhlenwand zu schreiben: die Namen der Toten von Ashford, das Jahr, das Wetter. Nach dem Grund gefragt, sagt sie, jemand müsse Buch führen, und sonst könne hier niemand lesen.

**because**  
EN: You took Aldric at Ashford 40 years ago; Kell keeps a library, so its people are lettered.  
m: Du hast Aldric vor 40 Jahren bei Ashford gefangen genommen; Kell besitzt eine Bibliothek, also können die Leute dort lesen.

**variant.scribe.body**  
EN: Marta of Kell has covered the cave wall in charcoal: the names of the dead at Ashford, the year, the weather. He offers, without being asked, to keep your chronicle. Nobody else here can read.  
m: Marta von Kell hat die Höhlenwand mit Kohle bedeckt: die Namen der Toten von Ashford, das Jahr, das Wetter. Er bietet ungefragt an, deine Chronik zu führen. Sonst kann hier niemand lesen.  
f: Marta von Kell hat die Höhlenwand mit Kohle bedeckt: die Namen der Toten von Ashford, das Jahr, das Wetter. Sie bietet ungefragt an, deine Chronik zu führen. Sonst kann hier niemand lesen.

**option.keep**  
EN: Keep the scribe as your historian  
m: Den Schreiber als deinen Chronisten behalten  
f: Die Schreiberin als deine Chronistin behalten

**option.release**  
EN: Send the scribe home  
m: Den Schreiber nach Hause schicken  
f: Die Schreiberin nach Hause schicken

**option.kill**  
EN: Silence the record  
m: Die Aufzeichnung zum Schweigen bringen

**option.ignore**  
EN: Let the writing stay on the wall  
m: Die Schrift an der Wand lassen

**reveal.keep**  
EN: From now on the briefing tells you why, not only what, while this one lives.  
m: Von nun an erklärt dir der Bericht beim Erwachen auch das Warum, solange dieser Mensch lebt.


## burned_library_scribes

**headline**  
EN: The scribes of Kell  
m: Die Schreiber von Kell

**body**  
EN: Marta stands at the ledge with a satchel of ash. 40 years after you burned the library at Ashford, Kell has no record of its own kings, and the young are taught from memory by the old. The envoy asks for what you took, or for someone to teach them letters again.  
m: Marta steht mit einer Tasche voll Asche auf dem Felsvorsprung. Seit du vor 40 Jahren die Bibliothek bei Ashford verbrannt hast, besitzt Kell keine Aufzeichnung der eigenen Könige mehr, und die Jungen lernen alles aus dem Gedächtnis der Alten. Der Gesandte bittet um das, was du genommen hast, oder darum, dass jemand die Schrift wieder lehrt.  
f: Marta steht mit einer Tasche voll Asche auf dem Felsvorsprung. Seit du vor 40 Jahren die Bibliothek bei Ashford verbrannt hast, besitzt Kell keine Aufzeichnung der eigenen Könige mehr, und die Jungen lernen alles aus dem Gedächtnis der Alten. Die Gesandte bittet um das, was du genommen hast, oder darum, dass jemand die Schrift wieder lehrt.

**because**  
EN: Kell lost its written memory at Ashford; what it knows of you is now told, not read.  
m: Kell verlor bei Ashford das geschriebene Gedächtnis; was man dort von dir weiß, wird jetzt erzählt und nicht mehr gelesen.

**variant.sage.body**  
EN: Aldric, the last sage of Kell, has sent Marta with a satchel of ash from Ashford. The hold has no record of its own kings now. The sage asks for the books you took, or for letters to be taught again.  
m: Aldric, der letzte Weise von Kell, hat Marta mit einer Tasche voll Asche aus Ashford geschickt. Die Feste besitzt keine Aufzeichnung ihrer eigenen Könige mehr. Er bittet um die Bücher, die du genommen hast, oder darum, dass die Schrift wieder gelehrt wird.  
f: Aldric, die letzte Weise von Kell, hat Marta mit einer Tasche voll Asche aus Ashford geschickt. Die Feste besitzt keine Aufzeichnung ihrer eigenen Könige mehr. Sie bittet um die Bücher, die du genommen hast, oder darum, dass die Schrift wieder gelehrt wird.

**option.books**  
EN: Give back the books  
m: Die Bücher zurückgeben

**option.teach**  
EN: Teach them letters  
m: Ihnen die Schrift beibringen

**option.eat**  
EN: Eat the envoy  
m: Den Gesandten fressen  
f: Die Gesandte fressen

**option.ignore**  
EN: Send them away  
m: Die Gesandtschaft wegschicken


## hero_at_dawn

**headline**  
EN: Aldric comes alone  
m: Aldric kommt allein

**body**  
EN: Aldric of Kell stands on the lair's border at first light with a banner planted in the scree and no army behind it. He was a child at Ashford when it burned, 40 years ago, and has sworn on that fire. The order waits a day's march back. This one wants it settled face to face.  
m: Aldric von Kell steht im ersten Licht an der Grenze deiner Höhle, ein Banner im Geröll, kein Heer im Rücken. Er war als Kind bei Ashford, als es brannte, vor 40 Jahren, und hat auf dieses Feuer geschworen. Der Orden wartet einen Tagesmarsch entfernt. Er will es von Angesicht zu Angesicht klären.  
f: Aldric von Kell steht im ersten Licht an der Grenze deiner Höhle, ein Banner im Geröll, kein Heer im Rücken. Sie war als Kind bei Ashford, als es brannte, vor 40 Jahren, und hat auf dieses Feuer geschworen. Der Orden wartet einen Tagesmarsch entfernt. Sie will es von Angesicht zu Angesicht klären.

**because**  
EN: The burning of Ashford is written in Kell's chronicle; Aldric is sworn to that memory.  
m: Der Brand von Ashford steht in der Chronik von Kell; Aldric ist diesem Gedächtnis verpflichtet.

**option.fight**  
EN: Meet the challenge  
m: Die Herausforderung annehmen

**option.haunt**  
EN: Send a nightmare instead  
m: Stattdessen einen Albtraum schicken

**option.demand**  
EN: Demand the order be disbanded  
m: Die Auflösung des Ordens fordern

**option.ignore**  
EN: Let the banner stand  
m: Das Banner stehen lassen


## princess_returns

**headline**  
EN: A letter in a hand you taught  
m: Ein Brief in einer Schrift, die du gelehrt hast

**body**  
EN: A kestrel drops a letter on the ledge. Aldric writes from Kell's throne that the priests have begun asking where the new king learned his craft, and that two of them have gone to the archive. He asks whether you would like the priests to find something, or nothing.  
m: Ein Turmfalke lässt einen Brief auf den Felsvorsprung fallen. Aldric schreibt vom Thron von Kell, dass die Priester zu fragen begonnen haben, wo der neue König sein Handwerk gelernt hat, und dass zwei von ihnen ins Archiv gegangen sind. Er fragt, ob die Priester etwas finden sollen oder nichts.  
f: Ein Turmfalke lässt einen Brief auf den Felsvorsprung fallen. Aldric schreibt vom Thron von Kell, dass die Priester zu fragen begonnen haben, wo die neue Königin ihr Handwerk gelernt hat, und dass zwei von ihnen ins Archiv gegangen sind. Sie fragt, ob die Priester etwas finden sollen oder nichts.

**because**  
EN: You released Aldric 40 years ago, schooled in intrigue and touched by your magic; a devout court notices.  
m: Du hast Aldric vor 40 Jahren freigelassen, in Ränken geschult und von deiner Magie berührt; ein frommer Hof wird aufmerksam.

**option.reveal**  
EN: Expose the priests' own plot  
m: Das eigene Komplott der Priester enthüllen

**option.spy**  
EN: Walk the city yourself, unseen  
m: Selbst und ungesehen durch die Stadt gehen

**option.haunt**  
EN: Warn him to burn the archive  
m: Ihm raten, das Archiv zu verbrennen  
f: Ihr raten, das Archiv zu verbrennen

**option.gold**  
EN: Send gold to the keeper of the archive  
m: Den Hütern des Archivs Gold schicken

**option.ignore**  
EN: Let him manage his own court  
m: Ihm den Hof selbst überlassen  
f: Ihr den Hof selbst überlassen


## inquisition_opens

**headline**  
EN: The inquisition of Kell  
m: Die Inquisition von Kell

**body**  
EN: Aldric has the court's mages in chains and its archive under seal. The charge is dark craft at the throne; the proof is thin and the fires are not. The mage guild's work has stopped. A court that burns its own scholars is weaker for a generation, and it knows who it blames.  
m: Aldric hat die Magier des Hofes in Ketten und sein Archiv unter Siegel. Der Vorwurf lautet auf dunkle Künste am Thron; der Beweis ist dünn, die Feuer sind es nicht. Die Arbeit der Magiergilde ruht. Ein Hof, der die eigenen Gelehrten verbrennt, ist für eine Generation schwächer, und er weiß, wem er die Schuld gibt.

**because**  
EN: The secret you planted in Kell surfaced; a devout court with a weak crown answers with fire.  
m: Das Geheimnis, das du in Kell gepflanzt hast, ist ans Licht gekommen; ein frommer Hof mit schwacher Krone antwortet mit Feuer.

**variant.agent.body**  
EN: Aldric has Marta's tutors in chains and the archive under seal. The charge is dark craft at the throne, and this time the proof is not thin. The mages' work has stopped. He has not been named yet.  
m: Aldric hat die Lehrer von Marta in Ketten und das Archiv unter Siegel. Der Vorwurf lautet auf dunkle Künste am Thron, und diesmal ist der Beweis nicht dünn. Die Arbeit der Magier ruht. Er ist noch nicht genannt worden.  
f: Aldric hat die Lehrer von Marta in Ketten und das Archiv unter Siegel. Der Vorwurf lautet auf dunkle Künste am Thron, und diesmal ist der Beweis nicht dünn. Die Arbeit der Magier ruht. Sie ist noch nicht genannt worden.

**option.gift**  
EN: Send the priest a relic to look at instead  
m: Dem Priester stattdessen ein Relikt zum Ansehen schicken  
f: Der Priesterin stattdessen ein Relikt zum Ansehen schicken

**option.judge**  
EN: Descend and spare the accused in judgement  
m: Hinabsteigen und die Angeklagten im Urteil verschonen

**option.incite**  
EN: Set the neighbour on them while they purge  
m: Den Nachbarn auf sie hetzen, solange sie säubern

**option.ignore**  
EN: Let them burn their own  
m: Sie die eigenen Leute verbrennen lassen


## thief_with_the_cup

**headline**  
EN: The cup is on their table  
m: Der Kelch steht auf ihrem Tisch

**body**  
EN: The Ashen Cup that lay under your left wing is now the centrepiece of Kell's feast table, and the whole city has been quieter since. The thief has a street named after the walk back down. The city is rich, pleased with itself, and watching the sky more than it used to.  
m: Der Aschenkelch lag unter deinem linken Flügel. Jetzt ist er der Mittelpunkt der Festtafel von Kell, und die ganze Stadt ist seither stiller geworden. Der Dieb hat eine Gasse, die nach seinem Rückweg benannt ist. Die Stadt ist reich, mit sich zufrieden und schaut häufiger zum Himmel als früher.  
f: Die Aschenschale lag unter deinem linken Flügel. Jetzt ist sie der Mittelpunkt der Festtafel von Kell, und die ganze Stadt ist seither stiller geworden. Der Dieb hat eine Gasse, die nach seinem Rückweg benannt ist. Die Stadt ist reich, mit sich zufrieden und schaut häufiger zum Himmel als früher.

**because**  
EN: A court that does not fear you sent a thief while you slept; it remembers the theft with pride.  
m: Ein Hof, der dich nicht fürchtet, hat einen Dieb geschickt, während du schliefst; er erinnert sich mit Stolz an den Diebstahl.

**variant.thief.body**  
EN: Aldric came out of your mountain with the Ashen Cup and has not bought a drink since. He stands on Kell's feast table and the city watches the sky more than it used to.  
m: Aldric kam mit dem Aschenkelch aus deinem Berg und hat seither keinen Schluck mehr bezahlt. Er steht auf der Festtafel von Kell, und die Stadt schaut häufiger zum Himmel als früher.  
f: Aldric kam mit der Aschenschale aus deinem Berg und hat seither keinen Schluck mehr bezahlt. Sie steht auf der Festtafel von Kell, und die Stadt schaut häufiger zum Himmel als früher.

**option.take**  
EN: Take the Ashen Cup back, and more  
m: Den Aschenkelch zurückholen, und mehr  
f: Die Aschenschale zurückholen, und mehr

**option.demand**  
EN: Demand the thief as hostage  
m: Den Dieb als Geisel fordern

**option.gift**  
EN: Send them exactly what else was taken  
m: Ihnen genau das schicken, was sonst noch genommen wurde

**option.ignore**  
EN: Let them keep the Ashen Cup  
m: Sie den Aschenkelch behalten lassen  
f: Sie die Aschenschale behalten lassen

**reveal.gift**  
EN: Generosity after a theft is remembered as a riddle; the court will send someone to ask what you meant.  
m: Großzügigkeit nach einem Diebstahl bleibt als Rätsel in Erinnerung; der Hof wird jemanden schicken, um zu fragen, was du gemeint hast.


## two_crowns

**headline**  
EN: Two crowns and an heir  
m: Zwei Kronen und ein Erbe  
f: Zwei Kronen und eine Erbin

**body**  
EN: Kell has no one for the throne but Aldric, asleep in your hoard, and Dorun has a widowed queen and a long border. Both courts have written to each other; neither has written to you. A marriage would make one realm of two, and the mountain would stand between it and nothing.  
m: Für den Thron von Kell bleibt nur Aldric, und er schläft in deinem Hort. Dorun hat eine verwitwete Königin und eine lange Grenze. Beide Höfe haben einander geschrieben, keiner hat dir geschrieben. Eine Heirat würde aus zwei Reichen eines machen, und nur der Berg stünde dann noch zwischen diesem Reich und dem Nichts.  
f: Für den Thron von Kell bleibt nur Aldric, und sie schläft in deinem Hort. Dorun hat eine verwitwete Königin und eine lange Grenze. Beide Höfe haben einander geschrieben, keiner hat dir geschrieben. Eine Heirat würde aus zwei Reichen eines machen, und nur der Berg stünde dann noch zwischen diesem Reich und dem Nichts.

**because**  
EN: You hold Aldric; a marriage between the two courts needs him.  
m: Du hältst Aldric; eine Heirat zwischen den beiden Höfen braucht den Erben in deinem Hort.  
f: Du hältst Aldric; eine Heirat zwischen den beiden Höfen braucht die Erbin in deinem Hort.

**variant.envoy.body**  
EN: Marta of Dorun has come to ask, very carefully, whether Aldric is well, and whether the mountain has views on marriage.  
m: Marta von Dorun ist gekommen, um sehr vorsichtig zu fragen, ob es Aldric gut geht und ob der Berg eine Meinung zur Heirat hat.

**option.broker**  
EN: Broker the marriage yourself  
m: Die Heirat selbst vermitteln

**option.release**  
EN: Send the heir home under oath  
m: Den Erben unter Eid heimschicken  
f: Die Erbin unter Eid heimschicken

**option.ignore**  
EN: Keep the heir and the border open  
m: Den Erben behalten und die Grenze offen halten  
f: Die Erbin behalten und die Grenze offen halten


## pilgrims_in_winter

**headline**  
EN: Pilgrims at the foot of the mountain  
m: Pilger am Fuß des Berges

**body**  
EN: Eleven of Kell's faithful have come up the pass in the snow with a cart of Ashford's last apples and a child who will not stop looking at the cave. They do not ask for anything. They have come to be looked at.  
m: Elf Gläubige aus Kell sind im Schnee den Pass heraufgekommen, mit einem Karren der letzten Äpfel von Ashford und einem Kind, das nicht aufhört, die Höhle anzusehen. Sie bitten um nichts. Sie sind gekommen, um angesehen zu werden.

**because**  
EN: Kell holds you in awe and the road from Ashford is open.  
m: Kell hält dich in Ehrfurcht, und die Straße von Ashford ist offen.

**variant.leader.body**  
EN: Aldric has led eleven of Kell's faithful up the pass in the snow with a cart of Ashford's last apples. They do not ask for anything. They have come to be looked at, and he has brought a book to write down what they see.  
m: Aldric hat elf Gläubige aus Kell im Schnee den Pass hinaufgeführt, mit einem Karren der letzten Äpfel von Ashford. Sie bitten um nichts. Sie sind gekommen, um angesehen zu werden, und er hat ein Buch mitgebracht, um aufzuschreiben, was sie sehen.  
f: Aldric hat elf Gläubige aus Kell im Schnee den Pass hinaufgeführt, mit einem Karren der letzten Äpfel von Ashford. Sie bitten um nichts. Sie sind gekommen, um angesehen zu werden, und sie hat ein Buch mitgebracht, um aufzuschreiben, was sie sehen.

**variant.dread.body**  
EN: Eleven of Kell's faithful have come up the pass in the snow. They walk slowly and keep the children at the back. They have brought the last apples from Ashford, and they have not decided whether this is a gift or a tribute.  
m: Elf Gläubige aus Kell sind im Schnee den Pass heraufgekommen. Sie gehen langsam und halten die Kinder hinten. Sie haben die letzten Äpfel aus Ashford mitgebracht und sind sich nicht einig, ob das ein Geschenk ist oder ein Tribut.

**option.receive**  
EN: Land and receive them  
m: Landen und sie empfangen

**option.bless**  
EN: Bless their fields instead of appearing  
m: Ihre Felder segnen, statt zu erscheinen

**option.eat**  
EN: Eat them  
m: Sie fressen

**option.ignore**  
EN: Let them wait and go home  
m: Sie warten und heimgehen lassen


## lich_crown_speaks

**headline**  
EN: The Ashen Cup speaks  
m: Der Aschenkelch spricht  
f: Die Aschenschale spricht

**body**  
EN: He waits until the cave is quiet. The living will always build cannon, he says, in the voice of the king who drowned wearing him. He can make you something cannon cannot reach. He does not say what that costs. He never does.  
m: Er wartet, bis die Höhle still ist. Die Lebenden werden immer Kanonen bauen, sagt er, mit der Stimme des Königs, der ertrank, als er ihn trug. Er kann dich zu etwas machen, das keine Kanone erreicht. Was das kostet, sagt er nicht. Das tut er nie.  
f: Sie wartet, bis die Höhle still ist. Die Lebenden werden immer Kanonen bauen, sagt sie, mit der Stimme des Königs, der ertrank, als er sie trug. Sie kann dich zu etwas machen, das keine Kanone erreicht. Was das kostet, sagt sie nicht. Das tut sie nie.

**because**  
EN: An artifact with its own purpose has been in the hoard long enough to learn your fears.  
m: Ein Artefakt mit eigenem Willen liegt lange genug im Hort, um deine Ängste zu kennen.

**variant.armed.body**  
EN: He waits until the cave is quiet. Dorun has cannon now, he says, in the voice of the king who drowned wearing him, and the living will always build more. He can make you something cannon cannot reach. He does not say what that costs.  
m: Er wartet, bis die Höhle still ist. Dorun hat jetzt Kanonen, sagt er, mit der Stimme des Königs, der ertrank, als er ihn trug, und die Lebenden werden immer mehr bauen. Er kann dich zu etwas machen, das keine Kanone erreicht. Was das kostet, sagt er nicht.  
f: Sie wartet, bis die Höhle still ist. Dorun hat jetzt Kanonen, sagt sie, mit der Stimme des Königs, der ertrank, als er sie trug, und die Lebenden werden immer mehr bauen. Sie kann dich zu etwas machen, das keine Kanone erreicht. Was das kostet, sagt sie nicht.

**variant.dead.body**  
EN: He waits until the cave is quiet. The dead of Kell have priests now, he says, and the priests have a rite. He can make you something the living cannot kill. He does not say what you would be afterwards.  
m: Er wartet, bis die Höhle still ist. Die Toten von Kell haben jetzt Priester, sagt er, mit der Stimme des Königs, der ertrank, als er ihn trug, und die Priester haben einen Ritus. Er kann dich zu etwas machen, das die Lebenden nicht töten können. Was du danach wärst, sagt er nicht.  
f: Sie wartet, bis die Höhle still ist. Die Toten von Kell haben jetzt Priester, sagt sie, mit der Stimme des Königs, der ertrank, als er sie trug, und die Priester haben einen Ritus. Sie kann dich zu etwas machen, das die Lebenden nicht töten können. Was du danach wärst, sagt sie nicht.

**option.ask**  
EN: Ask what he wants  
m: Fragen, was er will  
f: Fragen, was sie will

**option.accept**  
EN: Accept the crown's bargain  
m: Den Handel der Krone annehmen

**option.refuse**  
EN: Refuse him  
m: Ihn ablehnen  
f: Sie ablehnen

**option.gift**  
EN: Give him to the dead and be rid of him  
m: Ihn den Toten geben und ihn los sein  
f: Sie den Toten geben und sie los sein

**option.ignore**  
EN: Say nothing  
m: Nichts sagen


## door_in_the_temple

**headline**  
EN: A door in the temple of Ashford  
m: Eine Tür im Tempel von Ashford

**body**  
EN: The cult of Kell has built a door in the temple at Ashford that opens onto nowhere yet. The priests say it opens for you. The neighbours say it opens for something else, and have sent soldiers to find out. Within two wakings it will open one way or the other, whether you are there or not.  
m: Der Kult von Kell hat im Tempel von Ashford eine Tür gebaut, die noch ins Nichts führt. Die Priester sagen, sie öffne sich für dich. Die Nachbarn sagen, sie öffne sich für etwas anderes, und haben Soldaten geschickt, um es herauszufinden. Innerhalb von zwei Erwachen öffnet sie sich so oder so, ob du dort bist oder nicht.

**because**  
EN: Faith, magic and time have met in one court; a door is the shape they take.  
m: Glaube, Magie und Zeit sind in einem Hof zusammengetroffen; eine Tür ist die Gestalt, die sie annehmen.

**variant.zealot.body**  
EN: The cult of Kell has built a door at Ashford that opens onto nowhere yet. Dorun's soldiers are already in the valley below it with torches. Within two wakings it opens one way or the other, whether you are there or not.  
m: Der Kult von Kell hat bei Ashford eine Tür gebaut, die noch ins Nichts führt. Die Soldaten von Dorun stehen schon mit Fackeln im Tal darunter. Innerhalb von zwei Erwachen öffnet sie sich so oder so, ob du dort bist oder nicht.

**option.consecrate**  
EN: Consecrate the temple as your own  
m: Den Tempel als deinen eigenen weihen

**option.destroy**  
EN: Break the altar and the door with it  
m: Den Altar zerbrechen, und die Tür mit ihm

**option.defend**  
EN: Hold the pass against the soldiers  
m: Den Pass gegen die Soldaten halten

**option.ignore**  
EN: Let the door decide  
m: Die Tür entscheiden lassen

**reveal.ignore**  
EN: If the door opens without you, something may come the other way.  
m: Öffnet sich die Tür ohne dich, kann etwas von der anderen Seite kommen.


## dwarves_ask_for_fire

**headline**  
EN: The dwarves ask for fire  
m: Die Zwerge bitten um Feuer

**body**  
EN: Aldric of Kell has brought a block of the hold's best steel as a gift and a request written in the Book: the orcs of Dorun have the lower galleries, and the hold's forges cannot make a heat that holds them. The dwarves do not ask you to burn anyone. They ask how fire is made to stay.  
m: Aldric von Kell hat einen Block des besten Stahls der Festung als Geschenk mitgebracht, dazu eine Bitte, im Buch niedergeschrieben: Die Orks von Dorun halten die unteren Stollen, und die Schmieden der Festung können keine Hitze erzeugen, die sie aufhält. Die Zwerge bitten dich nicht, jemanden zu verbrennen. Sie fragen, wie man Feuer zum Bleiben bringt.

**because**  
EN: Kell is at war with Dorun and open enough to ask a dragon for craft.  
m: Kell führt Krieg gegen Dorun und ist offen genug, einen Drachen um Handwerk zu bitten.

**option.teach**  
EN: Teach their smiths the craft of lasting fire  
m: Ihre Schmiede die Kunst des beständigen Feuers lehren

**option.gift**  
EN: Give them a fire-touched relic  
m: Ihnen ein feuergeküsstes Relikt geben

**option.burn**  
EN: Burn the orcs out of the galleries yourself  
m: Die Orks selbst aus den Stollen brennen

**option.ignore**  
EN: Keep the steel, give nothing  
m: Den Stahl behalten und nichts geben

**reveal.teach**  
EN: Dwarves who learn fire from a dragon will one day sell engines that reach the clouds.  
m: Zwerge, die von einem Drachen das Feuer lernen, werden eines Tages Maschinen verkaufen, die bis an die Wolken reichen.


## quiet_valley

**headline**  
EN: Nobody is looking at the mountain  
m: Niemand sieht den Berg an

**body**  
EN: Three wakings, three hooks left where they hung. In Kell the children are told the mountain has a dragon in it the way they are told the lake has a woman in it. A shepherd grazes goats on the lower slopes. The valley has stopped watching the sky, and it has stopped building things that point at it, too.  
m: Drei Erwachen, drei Aufhänger, die dort hängen blieben, wo sie hingen. In Kell erzählt man den Kindern, im Berg wohne ein Drache, so wie man ihnen erzählt, im See wohne eine Frau. Ein Hirte weidet Ziegen an den unteren Hängen. Das Tal schaut nicht mehr zum Himmel, und es baut auch nichts mehr, das darauf zeigt.

**because**  
EN: You have answered nothing for three wakings; what is not seen is not remembered.  
m: Du hast drei Erwachen lang nichts beantwortet; was nicht gesehen wird, wird nicht erinnert.

**variant.third.body**  
EN: Three wakings, three hooks left where they hung. Kell tells its children the mountain has a dragon in it the way it tells them the lake has a woman in it. A shepherd grazes goats on your slopes. Nobody is building anything that points at the sky.  
m: Drei Erwachen, drei Aufhänger, die dort hängen blieben, wo sie hingen. Kell erzählt den Kindern, im Berg wohne ein Drache, so wie man ihnen erzählt, im See wohne eine Frau. Ein Hirte weidet Ziegen an deinen Hängen. Niemand baut etwas, das zum Himmel zeigt.

**variant.late_era.body**  
EN: Three wakings, three hooks left where they hung. Kell has stone walls and a clock tower now and does not look up at the mountain at all. Nobody is building anything that points at the sky. The engineers have other work.  
m: Drei Erwachen, drei Aufhänger, die dort hängen blieben, wo sie hingen. Kell hat jetzt Steinmauern und einen Uhrturm und schaut gar nicht mehr zum Berg hinauf. Niemand baut etwas, das zum Himmel zeigt. Die Ingenieure haben anderes zu tun.

**option.show**  
EN: Land in their square at noon  
m: Mittags auf ihrem Platz landen

**option.preach**  
EN: Walk among them and start a rumour  
m: Unter ihnen gehen und ein Gerücht streuen

**option.ignore**  
EN: Let them forget  
m: Sie vergessen lassen

**reveal.ignore**  
EN: A forgotten dragon's next act is read as a first act; the forgetting is also an opening.  
m: Die nächste Tat eines vergessenen Drachen gilt als erste Tat; das Vergessen ist auch eine Gelegenheit.


## the_mountain

**headline**  
EN: The slopes are your shoulders  
m: Die Hänge sind deine Schultern

**body**  
EN: You have not fitted through the cave mouth for a hundred years. Snow stays on your back between wakings now. Goats graze on your tail. If you slept the long sleep here, the valley would build on you and call the mountain by your name, and no engine they ever make would reach your heart.  
m: Seit hundert Jahren passt du nicht mehr durch den Höhleneingang. Schnee bleibt zwischen den Erwachen auf deinem Rücken liegen. Ziegen weiden auf deinem Schwanz. Wenn du hier den langen Schlaf schlafen würdest, würde das Tal auf dir bauen und den Berg bei deinem Namen nennen, und keine Maschine, die es je baut, würde dein Herz erreichen.

**because**  
EN: Might past a threshold and a long sleep make a dragon into a mountain.  
m: Macht über einer Schwelle und ein langer Schlaf machen aus einem Drachen einen Berg.

**variant.settlers.body**  
EN: You have not fitted through the cave mouth for a hundred years. Kell has a village on your lower slopes already; they think it is a hill. If you slept the long sleep here they would build a city on you and call the mountain by your name.  
m: Seit hundert Jahren passt du nicht mehr durch den Höhleneingang. Kell hat schon ein Dorf an deinen unteren Hängen; man hält es dort für einen Hügel. Wenn du hier den langen Schlaf schlafen würdest, würden sie eine Stadt auf dir bauen und den Berg bei deinem Namen nennen.

**option.terraform**  
EN: Raise the ground around you and settle in  
m: Den Boden um dich erheben und dich niederlassen

**option.demand**  
EN: Demand that they come as pilgrims, not squatters  
m: Fordern, dass sie als Pilger kommen und nicht als Besetzer

**option.sleep**  
EN: Sleep the long sleep, here, now  
m: Den langen Schlaf schlafen, hier, jetzt

**option.ignore**  
EN: Not yet  
m: Noch nicht


## the_consort

**headline**  
EN: Aldric has followed you up the mountain  
m: Aldric ist dir auf den Berg gefolgt

**body**  
EN: You walked Kell's streets in a borrowed shape and Aldric walked with you, and knew, and came anyway. He is standing at the cave mouth now, with a travelling bag and the expression of someone who has already decided. Mortals age while you sleep. He knows that too.  
m: Du bist in geborgter Gestalt durch die Straßen von Kell gegangen, und Aldric ging mit dir, wusste es und kam trotzdem. Er steht jetzt am Höhleneingang, mit einer Reisetasche und dem Ausdruck eines Menschen, der sich längst entschieden hat. Sterbliche altern, während du schläfst. Das weiß er auch.  
f: Du bist in geborgter Gestalt durch die Straßen von Kell gegangen, und Aldric ging mit dir, wusste es und kam trotzdem. Sie steht jetzt am Höhleneingang, mit einer Reisetasche und dem Ausdruck eines Menschen, der sich längst entschieden hat. Sterbliche altern, während du schläfst. Das weiß sie auch.

**because**  
EN: You went among them in guise this waking; one of them saw through it and chose you.  
m: Du bist dieses Mal in Verkleidung unter ihnen gegangen; einer von ihnen hat dich durchschaut und dich gewählt.

**option.keep**  
EN: Keep Aldric as your consort  
m: Aldric als deinen Gefährten behalten  
f: Aldric als deine Gefährtin behalten

**option.dismiss**  
EN: Send him down the mountain, kindly  
m: Ihn freundlich den Berg hinunterschicken  
f: Sie freundlich den Berg hinunterschicken

**option.release**  
EN: Send him home under your protection  
m: Ihn unter deinem Schutz heimschicken  
f: Sie unter deinem Schutz heimschicken

**option.ignore**  
EN: Leave him at the cave mouth  
m: Ihn am Höhleneingang stehen lassen  
f: Sie am Höhleneingang stehen lassen


## false_dragon

**headline**  
EN: Aldric speaks for the mountain  
m: Aldric spricht für den Berg

**body**  
EN: Aldric of Kell has announced that the mountain speaks to him alone, and has raised the tithe in its name. The shrine's offerings go to the palace now. The faithful are uneasy and quiet. The sky has not confirmed or denied anything, which Aldric calls agreement.  
m: Aldric von Kell hat verkündet, der Berg spreche zu ihm allein, und hat den Zehnt in seinem Namen erhöht. Die Gaben des Schreins gehen nun an den Palast. Die Gläubigen sind unruhig und still. Der Himmel hat nichts bestätigt oder bestritten, was Aldric Zustimmung nennt.  
f: Aldric von Kell hat verkündet, der Berg spreche zu ihr allein, und hat den Zehnt in seinem Namen erhöht. Die Gaben des Schreins gehen nun an den Palast. Die Gläubigen sind unruhig und still. Der Himmel hat nichts bestätigt oder bestritten, was Aldric Zustimmung nennt.

**because**  
EN: A long absence and a faithful court leave room for someone to hold your worship for you.  
m: Eine lange Abwesenheit und ein gläubiger Hof lassen Raum dafür, dass jemand deine Verehrung für dich verwaltet.

**option.judge**  
EN: Descend and judge the pretender  
m: Hinabsteigen und den Hochstapler richten  
f: Hinabsteigen und die Hochstaplerin richten

**option.preach**  
EN: Walk the city and correct the story  
m: Durch die Stadt gehen und die Geschichte richtigstellen

**option.miracle**  
EN: Answer the faithful with a miracle  
m: Die Gläubigen mit einem Wunder antworten

**option.burn**  
EN: Burn the palace that collects your tithe  
m: Den Palast niederbrennen, der deinen Zehnt einsammelt

**option.ignore**  
EN: Let the pretender keep the tithe  
m: Dem Hochstapler den Zehnt lassen  
f: Der Hochstaplerin den Zehnt lassen


## envoy_who_lies

**headline**  
EN: Aldric brings news of a plot  
m: Aldric bringt Nachricht von einem Komplott

**body**  
EN: Aldric of Kell has climbed the pass to warn you: Dorun is forging engines to slay you and has sworn it at its altar. Kell asks only that you strike first, and offers forty gold a year in gratitude. He does not meet your eye when naming the altar.  
m: Aldric von Kell ist den Pass heraufgestiegen, um dich zu warnen: Dorun schmiedet Maschinen, um dich zu töten, und hat es am eigenen Altar geschworen. Kell bittet nur darum, dass du zuerst zuschlägst, und bietet vierzig Gold im Jahr als Dank. Er weicht deinem Blick aus, wenn der Altar genannt wird.  
f: Aldric von Kell ist den Pass heraufgestiegen, um dich zu warnen: Dorun schmiedet Maschinen, um dich zu töten, und hat es am eigenen Altar geschworen. Kell bittet nur darum, dass du zuerst zuschlägst, und bietet vierzig Gold im Jahr als Dank. Sie weicht deinem Blick aus, wenn der Altar genannt wird.

**because**  
EN: A court at war with a stronger neighbour has a reason to want you angry at it.  
m: Ein Hof im Krieg mit einem stärkeren Nachbarn hat einen Grund, dich gegen diesen Nachbarn aufzubringen.

**variant.keen_eyes.body**  
EN: Aldric of Kell warns you that Dorun is forging engines to slay you. The envoy's eyes say otherwise: Dorun has no slayers and no engines. Kell is losing a war and would like you to win it.  
m: Aldric von Kell warnt dich, dass Dorun Maschinen schmiedet, um dich zu töten. Die Augen des Gesandten sagen etwas anderes: Dorun hat weder Drachentöter noch Maschinen. Kell verliert einen Krieg und möchte, dass du ihn gewinnst.  
f: Aldric von Kell warnt dich, dass Dorun Maschinen schmiedet, um dich zu töten. Die Augen der Gesandten sagen etwas anderes: Dorun hat weder Drachentöter noch Maschinen. Kell verliert einen Krieg und möchte, dass du ihn gewinnst.

**variant.dread.body**  
EN: Aldric of Kell has climbed the pass and stands as far from the cave as the ledge allows. Dorun is forging engines to slay you, the envoy says, too quickly, and Kell would pay forty gold a year to see them burned first. The hands shake. The story does not.  
m: Aldric von Kell ist den Pass heraufgestiegen und steht so weit von der Höhle entfernt, wie der Felsvorsprung es zulässt. Dorun schmiede Maschinen, um dich zu töten, sagt der Gesandte, zu schnell, und Kell würde vierzig Gold im Jahr zahlen, um sie zuerst brennen zu sehen. Die Hände zittern. Die Geschichte nicht.  
f: Aldric von Kell ist den Pass heraufgestiegen und steht so weit von der Höhle entfernt, wie der Felsvorsprung es zulässt. Dorun schmiede Maschinen, um dich zu töten, sagt die Gesandte, zu schnell, und Kell würde vierzig Gold im Jahr zahlen, um sie zuerst brennen zu sehen. Die Hände zittern. Die Geschichte nicht.

**option.swear**  
EN: Swear to burn the rival for forty a year  
m: Schwören, den Rivalen für vierzig im Jahr zu verbrennen

**option.reveal**  
EN: Name the lie to the envoy's face  
m: Dem Gesandten die Lüge ins Gesicht sagen  
f: Der Gesandten die Lüge ins Gesicht sagen

**option.eat**  
EN: Eat the liar  
m: Den Lügner fressen  
f: Die Lügnerin fressen

**option.ignore**  
EN: Send the envoy home with no answer  
m: Den Gesandten ohne Antwort heimschicken  
f: Die Gesandte ohne Antwort heimschicken


## refugee_column

**headline**  
EN: A column on the pass road  
m: Ein Zug auf der Passstraße

**body**  
EN: Two hundred of Kell's people are on the road below with carts and no soldiers. Dorun has their valley. They have come to the mountain because the mountain is the one thing in the valley Dorun is afraid of. They will plant anywhere you point.  
m: Zweihundert Menschen aus Kell sind mit Karren und ohne Soldaten auf der Straße unterhalb. Dorun hält ihr Tal. Sie sind zum Berg gekommen, weil der Berg das Einzige im Tal ist, wovor sich Dorun fürchtet. Sie werden anpflanzen, wo immer du hinzeigst.

**because**  
EN: Kell lost its land to Dorun and holds you in some awe; the mountain is the last door.  
m: Kell hat das eigene Land an Dorun verloren und hält dich in einiger Ehrfurcht; der Berg ist die letzte Tür.

**variant.elder.body**  
EN: Two hundred of Kell's people are on the road below, led by Aldric, who remembers when you last flew over Kell and has been telling the children about it for a week to keep them walking. They will plant anywhere you point.  
m: Zweihundert Menschen aus Kell sind auf der Straße unterhalb, angeführt von Aldric, der sich erinnert, wann du zuletzt über Kell geflogen bist, und es den Kindern seit einer Woche erzählt, damit sie weitergehen. Sie werden anpflanzen, wo immer du hinzeigst.  
f: Zweihundert Menschen aus Kell sind auf der Straße unterhalb, angeführt von Aldric, die sich erinnert, wann du zuletzt über Kell geflogen bist, und es den Kindern seit einer Woche erzählt, damit sie weitergehen. Sie werden anpflanzen, wo immer du hinzeigst.

**variant.orc_taker.body**  
EN: Two hundred of Kell's people are on the road below with carts and no soldiers. The orcs of Dorun have their valley and, by the smoke, their harvest. They have come to the mountain because the orcs will not. They will plant anywhere you point.  
m: Zweihundert Menschen aus Kell sind mit Karren und ohne Soldaten auf der Straße unterhalb. Die Orks von Dorun haben ihr Tal und, nach dem Rauch zu urteilen, ihre Ernte. Sie sind zum Berg gekommen, weil die Orks es nicht tun. Sie werden anpflanzen, wo immer du hinzeigst.

**option.receive**  
EN: Let them settle your slopes  
m: Sie deine Hänge besiedeln lassen

**option.land**  
EN: Give them the empty vale  
m: Ihnen das leere Tal geben

**option.defend**  
EN: Take their valley back for them  
m: Ihr Tal für sie zurückholen

**option.ignore**  
EN: Turn them away  
m: Sie abweisen

**option.skip**  
EN: Do nothing; they will move on  
m: Nichts tun; sie ziehen weiter


## prophet_with_the_prophecy

**headline**  
EN: Aldric has read the lines  
m: Aldric hat die Zeilen gelesen

**body**  
EN: Aldric of Kell has walked up the mountain with the book open. He has read the prophecy forty times and believes it is about you and about now. He wants to tell you what the four peoples and the house of stone mean, and what Kell must build. He also wants to be the one living voice.  
m: Aldric von Kell ist mit aufgeschlagenem Buch den Berg heraufgekommen. Er hat die Prophezeiung vierzigmal gelesen und glaubt, dass sie von dir handelt und von jetzt. Er will dir sagen, was die vier Völker und das Haus aus Stein bedeuten und was Kell bauen muss. Er will außerdem die eine lebende Stimme sein.  
f: Aldric von Kell ist mit aufgeschlagenem Buch den Berg heraufgekommen. Sie hat die Prophezeiung vierzigmal gelesen und glaubt, dass sie von dir handelt und von jetzt. Sie will dir sagen, was die vier Völker und das Haus aus Stein bedeuten und was Kell bauen muss. Sie will außerdem die eine lebende Stimme sein.

**because**  
EN: Kell holds the prophecy and holds you in awe; a devout priest draws the obvious conclusion.  
m: Kell besitzt die Prophezeiung und hält dich in Ehrfurcht; wer fromm ist, zieht den naheliegenden Schluss.

**option.receive**  
EN: Hear the prophet out  
m: Den Propheten anhören  
f: Die Prophetin anhören

**option.teach**  
EN: Teach the prophet what dragons really are  
m: Dem Propheten beibringen, was Drachen wirklich sind  
f: Der Prophetin beibringen, was Drachen wirklich sind

**option.keep**  
EN: Keep the prophet as your priest  
m: Den Propheten als deinen Priester behalten  
f: Die Prophetin als deine Priesterin behalten

**option.ignore**  
EN: Send the book and the reader home  
m: Das Buch und den Leser heimschicken  
f: Das Buch und die Leserin heimschicken

**reveal.receive**  
EN: The prophet's reading of the four conditions is shown on the Endings page from now on, with his bias.  
m: Die Deutung des Propheten zu den vier Bedingungen steht von nun an auf der Seite „Enden“, mit seiner Voreingenommenheit.  
f: Die Deutung der Prophetin zu den vier Bedingungen steht von nun an auf der Seite „Enden“, mit ihrer Voreingenommenheit.


## kin_of_a_hostage

**headline**  
EN: Marta has come for Aldric  
m: Marta ist gekommen, um Aldric zu holen

**body**  
EN: Marta of Kell has come up the pass alone, which took some courage, and stands at the ledge asking to see Aldric. He has brought a ransom and a speech. The speech is about how long Aldric has been gone and how the court needs the heir. The ransom is less than you would have asked.  
m: Marta von Kell ist allein den Pass heraufgekommen, was einigen Mut verlangte, und steht am Felsvorsprung und bittet, Aldric zu sehen. Er hat ein Lösegeld und eine Rede mitgebracht. Die Rede handelt davon, wie lange Aldric schon fort ist und dass der Hof den Erben braucht. Das Lösegeld ist geringer, als du verlangt hättest.  
f: Marta von Kell ist allein den Pass heraufgekommen, was einigen Mut verlangte, und steht am Felsvorsprung und bittet, Aldric zu sehen. Sie hat ein Lösegeld und eine Rede mitgebracht. Die Rede handelt davon, wie lange Aldric schon fort ist und dass der Hof die Erbin braucht. Das Lösegeld ist geringer, als du verlangt hättest.

**because**  
EN: Marta knows where you are and is not too afraid to come.  
m: Marta weiß, wo du bist, und fürchtet sich nicht zu sehr, um zu kommen.

**variant.bold.body**  
EN: Marta of Kell has come up the pass alone and stands at the ledge demanding, not asking, to see Aldric. He has brought a ransom and no speech. The ransom is fair.  
m: Marta von Kell ist allein den Pass heraufgekommen und verlangt, nicht bittet, Aldric zu sehen. Er hat ein Lösegeld mitgebracht und keine Rede. Das Lösegeld ist fair.  
f: Marta von Kell ist allein den Pass heraufgekommen und verlangt, nicht bittet, Aldric zu sehen. Sie hat ein Lösegeld mitgebracht und keine Rede. Das Lösegeld ist fair.

**variant.awe.body**  
EN: Marta of Kell has come up the pass with a small offering for the shrine and a larger one for you, and asks to see Aldric before anything is said about ransom. He kneels at the ledge, which Aldric finds very funny.  
m: Marta von Kell ist den Pass heraufgekommen, mit einer kleinen Gabe für den Schrein und einer größeren für dich, und bittet darum, Aldric zu sehen, bevor vom Lösegeld die Rede ist. Er kniet am Felsvorsprung, was Aldric sehr komisch findet.  
f: Marta von Kell ist den Pass heraufgekommen, mit einer kleinen Gabe für den Schrein und einer größeren für dich, und bittet darum, Aldric zu sehen, bevor vom Lösegeld die Rede ist. Sie kniet am Felsvorsprung, was Aldric sehr komisch findet.

**option.release**  
EN: Take the ransom and release the hostage  
m: Das Lösegeld nehmen und die Geisel freilassen

**option.demand**  
EN: Name a higher price  
m: Einen höheren Preis nennen

**option.keep**  
EN: Keep the hostage; let the kinsman see him  
m: Die Geisel behalten; dem Verwandten erlauben, ihn zu sehen  
f: Die Geisel behalten; der Verwandten erlauben, sie zu sehen

**option.ignore**  
EN: Send the kinsman away without seeing Aldric  
m: Den Verwandten wegschicken, ohne dass er Aldric sieht  
f: Die Verwandte wegschicken, ohne dass sie Aldric sieht


## slayer_at_the_ledge

**headline**  
EN: Aldric asks for single combat  
m: Aldric fordert den Zweikampf

**body**  
EN: Aldric of the order at Kell has walked up to the ledge in full plate with a sealed letter from the grandmaster and a challenge: fight here, now, one against one, and whoever lives keeps the mountain. He has a burn scar down one side that did not come from you. Not yet.  
m: Aldric vom Orden in Kell ist in voller Rüstung zum Felsvorsprung gegangen, mit einem versiegelten Brief des Großmeisters und einer Herausforderung: hier und jetzt, eins gegen eins, und wer überlebt, behält den Berg. Er trägt eine Brandnarbe an einer Seite, die nicht von dir stammt. Noch nicht.  
f: Aldric vom Orden in Kell ist in voller Rüstung zum Felsvorsprung gegangen, mit einem versiegelten Brief des Großmeisters und einer Herausforderung: hier und jetzt, eins gegen eins, und wer überlebt, behält den Berg. Sie trägt eine Brandnarbe an einer Seite, die nicht von dir stammt. Noch nicht.

**because**  
EN: An order with a grievance and your exact location sends its best before it sends everyone.  
m: Ein Orden mit einer Beschwerde und deinem genauen Aufenthaltsort schickt seinen Besten, bevor er alle schickt.

**variant.order_ready.body**  
EN: Aldric of the order at Kell has walked up to the ledge in full plate while the rest of the order waits a day's march back, ready. The challenge is single combat, here, now. If you refuse, he says, they all come, and soon.  
m: Aldric vom Orden in Kell ist in voller Rüstung zum Felsvorsprung gegangen, während der Rest des Ordens einen Tagesmarsch entfernt bereitsteht. Die Herausforderung lautet auf Zweikampf, hier und jetzt. Lehnst du ab, sagt er, kommen sie alle, und bald.  
f: Aldric vom Orden in Kell ist in voller Rüstung zum Felsvorsprung gegangen, während der Rest des Ordens einen Tagesmarsch entfernt bereitsteht. Die Herausforderung lautet auf Zweikampf, hier und jetzt. Lehnst du ab, sagt sie, kommen sie alle, und bald.

**variant.wounded.body**  
EN: Aldric of the order at Kell has walked up to the ledge in full plate and has clearly been told you are hurt. The challenge is single combat, here, now. He looks at the way you hold your wing and smiles.  
m: Aldric vom Orden in Kell ist in voller Rüstung zum Felsvorsprung gegangen und hat offenbar gehört, dass du verletzt bist. Die Herausforderung lautet auf Zweikampf, hier und jetzt. Er sieht, wie du den Flügel hältst, und lächelt.  
f: Aldric vom Orden in Kell ist in voller Rüstung zum Felsvorsprung gegangen und hat offenbar gehört, dass du verletzt bist. Die Herausforderung lautet auf Zweikampf, hier und jetzt. Sie sieht, wie du den Flügel hältst, und lächelt.

**option.fight**  
EN: Accept  
m: Annehmen

**option.haunt**  
EN: Fill the slayer's night with what fire does  
m: Die Nacht des Drachentöters mit dem füllen, was Feuer anrichtet  
f: Die Nacht der Drachentöterin mit dem füllen, was Feuer anrichtet

**option.reveal**  
EN: Tell the slayer what the order plans for him  
m: Dem Drachentöter sagen, was der Orden mit ihm vorhat  
f: Der Drachentöterin sagen, was der Orden mit ihr vorhat

**option.ignore**  
EN: Leave the challenge on the ledge  
m: Die Herausforderung auf dem Felsvorsprung liegen lassen


## the_walls_went_up

**headline**  
EN: Kell has walls now  
m: Kell hat jetzt Mauern

**body**  
EN: Where you burned Ashford 40 years ago there is stone to the height of a house and towers at the corners, each with a slot cut for a bolt-thrower that does not exist yet. The quarry is three valleys away. They have spent a generation's wealth on this, and they will tell you so if you land.  
m: Wo du vor 40 Jahren Ashford verbrannt hast, steht jetzt Stein in Haushöhe, mit Türmen an den Ecken, jeder mit einer Scharte für ein Bolzengeschütz, das es noch nicht gibt. Der Steinbruch liegt drei Täler weit entfernt. Sie haben den Reichtum einer Generation dafür ausgegeben, und sie werden es dir sagen, wenn du landest.

**because**  
EN: A court that remembers a burning and has reached the age of castles builds against the sky.  
m: Ein Hof, der sich an einen Brand erinnert und das Zeitalter der Burgen erreicht hat, baut gegen den Himmel.

**variant.mason.body**  
EN: Aldric of Kell designed the new walls at Ashford, stone to the height of a house with a slot in every tower for a bolt-thrower that does not exist yet. He has written a treatise on dragon fire. It is mostly wrong.  
m: Aldric von Kell hat die neuen Mauern bei Ashford entworfen, Stein in Haushöhe, mit einer Scharte in jedem Turm für ein Bolzengeschütz, das es noch nicht gibt. Er hat eine Abhandlung über Drachenfeuer geschrieben. Sie ist größtenteils falsch.  
f: Aldric von Kell hat die neuen Mauern bei Ashford entworfen, Stein in Haushöhe, mit einer Scharte in jedem Turm für ein Bolzengeschütz, das es noch nicht gibt. Sie hat eine Abhandlung über Drachenfeuer geschrieben. Sie ist größtenteils falsch.

**option.walls**  
EN: Burn the walls while they are new  
m: Die Mauern verbrennen, solange sie neu sind

**option.demand**  
EN: Demand they tear the towers down  
m: Fordern, dass sie die Türme abreißen

**option.show**  
EN: Land inside the walls, in the square  
m: Innerhalb der Mauern landen, auf dem Platz

**option.ignore**  
EN: Let them have their walls  
m: Ihnen ihre Mauern lassen

**reveal.ignore**  
EN: Walls that stand for an age become a court's pride; pride founds slayer orders.  
m: Mauern, die ein Zeitalter lang stehen, werden zum Stolz eines Hofes; aus Stolz entstehen Drachentöterorden.


## the_gold_you_took

**headline**  
EN: An empty treasury and a full neighbour  
m: Eine leere Schatzkammer und ein voller Nachbar

**body**  
EN: The gold you took from Kell 40 years ago was the gold that paid its soldiers. The soldiers have gone to Dorun, which pays. Kell is now a city that cannot defend its own gates, and Dorun has noticed.  
m: Das Gold, das du vor 40 Jahren aus Kell genommen hast, war das Gold, das die Soldaten bezahlte. Die Soldaten sind zu Dorun gegangen, wo man zahlt. Kell ist jetzt eine Stadt, die ihre eigenen Tore nicht verteidigen kann, und Dorun hat es bemerkt.

**because**  
EN: You took a third of Kell's wealth; a poor court cannot keep an army.  
m: Du hast ein Drittel des Reichtums von Kell genommen; ein armer Hof kann kein Heer halten.

**option.give**  
EN: Return a third of it, quietly  
m: Ein Drittel davon zurückgeben, in aller Stille

**option.defend**  
EN: Promise to stand between them  
m: Versprechen, zwischen ihnen zu stehen

**option.incite**  
EN: Point Dorun at the open gates  
m: Dorun auf die offenen Tore lenken

**option.ignore**  
EN: A poor court is a quiet court  
m: Ein armer Hof ist ein stiller Hof


## the_army_you_broke

**headline**  
EN: A road of cairns  
m: Eine Straße der Steinmale

**body**  
EN: The road from Ashford to Kell's capital is lined with cairns now, one for each company you burned on the march. Marta rules a court of widows and boys. The boys are training with spears in the square, and the drill-master was there that day.  
m: Die Straße von Ashford zur Hauptstadt von Kell ist jetzt von Steinmalen gesäumt, eines für jede Kompanie, die du auf dem Marsch verbrannt hast. Marta herrscht über einen Hof aus Witwen und Knaben. Die Knaben üben mit Speeren auf dem Platz, und der Waffenmeister war an jenem Tag dabei.

**because**  
EN: You killed a third of Kell's army at Ashford; a martial court remembers its dead in stone.  
m: Du hast bei Ashford ein Drittel des Heeres von Kell getötet; ein kriegerischer Hof gedenkt seiner Toten in Stein.

**variant.general.body**  
EN: Aldric survived the march you burned at Ashford and has not stopped talking about it. The road is lined with cairns. The boys drilling in Kell's square call him teacher.  
m: Aldric hat den Marsch überlebt, den du bei Ashford verbrannt hast, und hört nicht auf, davon zu reden. Die Straße ist von Steinmalen gesäumt. Die Knaben, die auf dem Platz von Kell üben, nennen ihn Lehrer.  
f: Aldric hat den Marsch überlebt, den du bei Ashford verbrannt hast, und hört nicht auf, davon zu reden. Die Straße ist von Steinmalen gesäumt. Die Knaben, die auf dem Platz von Kell üben, nennen sie Lehrerin.

**option.crush**  
EN: Finish the garrison before the boys grow  
m: Die Besatzung vernichten, bevor die Knaben groß sind

**option.judge**  
EN: Land and judge the drill-master  
m: Landen und den Waffenmeister richten  
f: Landen und die Waffenmeisterin richten

**option.give**  
EN: Send grain for the widows  
m: Getreide für die Witwen schicken

**option.ignore**  
EN: Let them drill  
m: Sie üben lassen


## the_city_you_saved

**headline**  
EN: A feast day in Kell  
m: Ein Festtag in Kell

**body**  
EN: Kell keeps the day you broke the siege at Ashford as a holiday now. Aldric has had your likeness carved over the gate, a little too thin and with too many teeth. Children leave bread on the walls. The priests are arguing about whether bread is enough.  
m: Kell begeht den Tag, an dem du die Belagerung bei Ashford gebrochen hast, jetzt als Feiertag. Aldric hat dein Bild über dem Tor einmeißeln lassen, ein wenig zu dünn und mit zu vielen Zähnen. Kinder legen Brot auf die Mauern. Die Priester streiten, ob Brot genug ist.

**because**  
EN: You defended Kell 40 years ago; a saved city turns gratitude into ritual within a generation.  
m: Du hast Kell vor 40 Jahren verteidigt; eine gerettete Stadt macht aus Dankbarkeit innerhalb einer Generation ein Ritual.

**variant.attacker.body**  
EN: Kell keeps the day you broke Dorun's siege at Ashford as a holiday. Aldric has had your likeness carved over the gate, too thin and with too many teeth. Dorun keeps the day too, differently.  
m: Kell begeht den Tag, an dem du die Belagerung durch Dorun bei Ashford gebrochen hast, als Feiertag. Aldric hat dein Bild über dem Tor einmeißeln lassen, zu dünn und mit zu vielen Zähnen. Auch Dorun begeht den Tag, auf andere Weise.

**option.consecrate**  
EN: Make the gate a shrine  
m: Das Tor zum Schrein machen

**option.show**  
EN: Fly over the feast at noon  
m: Mittags über das Fest fliegen

**option.demand**  
EN: Ask for tribute in place of bread  
m: Tribut statt Brot verlangen

**option.ignore**  
EN: Let them keep their day  
m: Ihnen ihren Tag lassen


## the_oath_they_remember

**headline**  
EN: The oath is in the book  
m: Der Eid steht im Buch

**body**  
EN: Aldric of Kell has read the oath he inherited: tribute to the mountain, sworn 40 years ago by a terrified ancestor. The book also records why. Aldric is not terrified. The tribute has arrived this year, exactly, with a note asking whether the mountain is still there.  
m: Aldric von Kell hat den Eid gelesen, den er geerbt hat: Tribut an den Berg, vor 40 Jahren von einem verängstigten Vorfahren geschworen. Das Buch hält auch fest, warum. Aldric hat keine Angst. Der Tribut ist dieses Jahr eingetroffen, auf den Tag genau, mit einer Nachricht, die fragt, ob der Berg noch da ist.  
f: Aldric von Kell hat den Eid gelesen, den sie geerbt hat: Tribut an den Berg, vor 40 Jahren von einem verängstigten Vorfahren geschworen. Das Buch hält auch fest, warum. Aldric hat keine Angst. Der Tribut ist dieses Jahr eingetroffen, auf den Tag genau, mit einer Nachricht, die fragt, ob der Berg noch da ist.

**because**  
EN: A written oath outlives the dread that made it; the heir keeps the letter and tests the spirit.  
m: Ein geschriebener Eid überdauert die Furcht, aus der er entstand; der Erbe hält sich an den Buchstaben und prüft den Geist.  
f: Ein geschriebener Eid überdauert die Furcht, aus der er entstand; die Erbin hält sich an den Buchstaben und prüft den Geist.

**option.show**  
EN: Answer the note in person  
m: Die Nachricht persönlich beantworten

**option.release**  
EN: Release them from the oath, with grace  
m: Sie in Gnaden vom Eid entbinden

**option.demand**  
EN: Raise the tribute  
m: Den Tribut erhöhen

**option.ignore**  
EN: Take the gold and say nothing  
m: Das Gold nehmen und nichts sagen


## the_pupils

**headline**  
EN: What the pupils did with it  
m: Was die Schüler daraus gemacht haben

**body**  
EN: You taught Kell 40 years ago. Aldric runs a school now where that lesson is the first week. The second week is about dragons. The school has a tower, the tower has a telescope, and the telescope points at the mountain every clear night.  
m: Du hast Kell vor 40 Jahren unterrichtet. Aldric leitet jetzt eine Schule, in der diese Lektion die erste Woche ist. Die zweite Woche handelt von Drachen. Die Schule hat einen Turm, der Turm hat ein Fernrohr, und das Fernrohr zeigt in jeder klaren Nacht auf den Berg.

**because**  
EN: A court you taught teaches itself; what it learns next is not yours to choose.  
m: Ein Hof, den du unterrichtet hast, unterrichtet sich selbst; was er als Nächstes lernt, wählst nicht du.

**option.teach**  
EN: Teach them something harder  
m: Ihnen etwas Schwereres beibringen

**option.burn**  
EN: Burn the tower  
m: Den Turm verbrennen

**option.keep**  
EN: Take the sage as your historian  
m: Den Weisen als deinen Chronisten nehmen  
f: Die Weise als deine Chronistin nehmen

**option.ignore**  
EN: Let them look  
m: Sie schauen lassen


## the_one_you_spared

**headline**  
EN: Aldric rules now  
m: Aldric herrscht jetzt

**body**  
EN: You judged Aldric and let him live, in front of the whole square, 40 years ago. Aldric rules Kell now and has never once mentioned it in public. In private, every envoy who leaves for the mountain is briefed by Aldric personally.  
m: Du hast Aldric gerichtet und ihn leben lassen, vor dem ganzen Platz, vor 40 Jahren. Aldric herrscht jetzt über Kell und hat es nie in der Öffentlichkeit erwähnt. Im Verborgenen weist Aldric jeden Gesandten, der zum Berg aufbricht, persönlich ein.  
f: Du hast Aldric gerichtet und sie leben lassen, vor dem ganzen Platz, vor 40 Jahren. Aldric herrscht jetzt über Kell und hat es nie in der Öffentlichkeit erwähnt. Im Verborgenen weist Aldric jeden Gesandten, der zum Berg aufbricht, persönlich ein.

**because**  
EN: A public mercy left Aldric owing you, and unable to say so.  
m: Eine öffentliche Gnade hat Aldric zu deinem Schuldner gemacht, und er kann es nicht sagen.  
f: Eine öffentliche Gnade hat Aldric zu deiner Schuldnerin gemacht, und sie kann es nicht sagen.

**variant.vengeful.body**  
EN: You judged Aldric and let him live, in front of the whole square. Aldric rules Kell now and has never forgiven the square for watching. Every envoy who leaves for the mountain is briefed personally, and the brief is not kind.  
m: Du hast Aldric gerichtet und ihn leben lassen, vor dem ganzen Platz. Aldric herrscht jetzt über Kell und hat dem Platz nie verziehen, dass er zusah. Jeder Gesandte, der zum Berg aufbricht, wird persönlich eingewiesen, und die Einweisung ist nicht freundlich.  
f: Du hast Aldric gerichtet und sie leben lassen, vor dem ganzen Platz. Aldric herrscht jetzt über Kell und hat dem Platz nie verziehen, dass er zusah. Jeder Gesandte, der zum Berg aufbricht, wird persönlich eingewiesen, und die Einweisung ist nicht freundlich.

**option.broker**  
EN: Use the debt: broker a peace through him  
m: Die Schuld nutzen: durch ihn einen Frieden vermitteln  
f: Die Schuld nutzen: durch sie einen Frieden vermitteln

**option.haunt**  
EN: Remind him, in a dream  
m: Ihn im Traum erinnern  
f: Sie im Traum erinnern

**option.show**  
EN: Visit the square again  
m: Den Platz noch einmal besuchen

**option.ignore**  
EN: Let the debt lie  
m: Die Schuld ruhen lassen


## the_road_of_your_shadow

**headline**  
EN: They saw you pass  
m: Sie haben dich vorüberziehen sehen

**body**  
EN: You flew low over Kell 40 years ago and burned nothing. The road your shadow crossed is called the dragon's road now and the farmers along it leave the first sheaf at the milestone. Nobody has asked you for anything. They might, if you landed.  
m: Du bist vor 40 Jahren tief über Kell geflogen und hast nichts verbrannt. Die Straße, die dein Schatten kreuzte, heißt jetzt die Drachenstraße, und die Bauern an ihr legen die erste Garbe am Meilenstein ab. Niemand hat dich um etwas gebeten. Vielleicht täten sie es, wenn du landen würdest.

**because**  
EN: A dragon seen and not feared becomes a story the fields tell.  
m: Ein Drache, den man sieht und nicht fürchtet, wird zu einer Geschichte, die die Felder erzählen.

**variant.child.body**  
EN: You flew low over Kell and burned nothing. Aldric, the heir, was on the road and has drawn you on every wall of the palace since. The farmers leave the first sheaf at the milestone where your shadow crossed.  
m: Du bist tief über Kell geflogen und hast nichts verbrannt. Aldric, der Erbe, war auf der Straße und hat dich seither an jede Wand des Palastes gemalt. Die Bauern legen die erste Garbe am Meilenstein ab, wo dein Schatten die Straße kreuzte.  
f: Du bist tief über Kell geflogen und hast nichts verbrannt. Aldric, die Erbin, war auf der Straße und hat dich seither an jede Wand des Palastes gemalt. Die Bauern legen die erste Garbe am Meilenstein ab, wo dein Schatten die Straße kreuzte.

**variant.late_era.body**  
EN: You flew low over Kell and burned nothing. The astronomers logged the hour and the heading. The farmers leave the first sheaf at the milestone anyway; the astronomers find this embarrassing.  
m: Du bist tief über Kell geflogen und hast nichts verbrannt. Die Sterndeuter haben Stunde und Richtung verzeichnet. Die Bauern legen die erste Garbe trotzdem am Meilenstein ab; den Sterndeutern ist das peinlich.

**option.land**  
EN: Land at the milestone  
m: Am Meilenstein landen

**option.bless**  
EN: Bless the fields along the road  
m: Die Felder entlang der Straße segnen

**option.ignore**  
EN: Leave the story alone  
m: Die Geschichte in Ruhe lassen


## the_heir_who_grew_up_here

**headline**  
EN: Aldric asks to go home a ruler  
m: Aldric will als Herrscher heimkehren  
f: Aldric will als Herrscherin heimkehren

**body**  
EN: Aldric was a child when you took him from Kell 40 years ago, and is not a child now. Marta rules in Aldric's name and would rather not stop. Aldric asks for the throne, and for you to be seen at the coronation, and says it as someone who knows what the request costs you.  
m: Aldric war ein Kind, als du ihn vor 40 Jahren aus Kell geholt hast, und ist kein Kind mehr. Marta herrscht im Namen von Aldric und würde ungern damit aufhören. Aldric bittet um den Thron und darum, dass du bei der Krönung gesehen wirst, und sagt es wie jemand, der weiß, was die Bitte dich kostet.  
f: Aldric war ein Kind, als du sie vor 40 Jahren aus Kell geholt hast, und ist kein Kind mehr. Marta herrscht im Namen von Aldric und würde ungern damit aufhören. Aldric bittet um den Thron und darum, dass du bei der Krönung gesehen wirst, und sagt es wie jemand, der weiß, was die Bitte dich kostet.

**because**  
EN: Aldric, raised in the hoard, comes of age with a claim, a debt and a view of the valley from above.  
m: Aldric, im Hort aufgewachsen, wird mündig mit einem Anspruch, einer Schuld und einem Blick von oben auf das Tal.

**option.release**  
EN: Send him home with your sign on him  
m: Ihn mit deinem Zeichen heimschicken  
f: Sie mit deinem Zeichen heimschicken

**option.sponsor**  
EN: Back the claim with a war  
m: Den Anspruch mit einem Krieg unterstützen

**option.keep**  
EN: Keep him as your consort instead  
m: Ihn stattdessen als deinen Gefährten behalten  
f: Sie stattdessen als deine Gefährtin behalten

**option.ignore**  
EN: Not yet  
m: Noch nicht


## the_dead_you_raised

**headline**  
EN: The dead have built a chapel  
m: Die Toten haben eine Kapelle gebaut

**body**  
EN: The host you raised at Ashford did not fall when the war ended. It stayed, and the fen it holds has a chapel in it now, roofed with shields. Dorun says the dead are praying. Nobody has gone close enough to hear to whom.  
m: Das Heer, das du bei Ashford erweckt hast, fiel nicht, als der Krieg endete. Es blieb, und im Moor, das es hält, steht jetzt eine Kapelle, mit Schilden gedeckt. Dorun sagt, die Toten beten. Niemand ist nah genug herangegangen, um zu hören, zu wem.

**because**  
EN: You raised the dead 40 years ago; what you raise and do not dismiss finds its own reasons.  
m: Du hast die Toten vor 40 Jahren erweckt; was du erweckst und nicht entlässt, findet eigene Gründe.

**variant.priest.body**  
EN: The host you raised at Ashford has a priest now, Aldric, who was alive when you raised the others and is not quite alive now. Dorun says the dead are praying and that the prayers name the mountain.  
m: Das Heer, das du bei Ashford erweckt hast, hat jetzt einen Priester, Aldric, der lebte, als du die anderen erwecktest, und jetzt nicht mehr ganz lebt. Dorun sagt, die Toten beten und die Gebete nennen den Berg.  
f: Das Heer, das du bei Ashford erweckt hast, hat jetzt eine Priesterin, Aldric, die lebte, als du die anderen erwecktest, und jetzt nicht mehr ganz lebt. Dorun sagt, die Toten beten und die Gebete nennen den Berg.

**option.destroy**  
EN: Break the chapel  
m: Die Kapelle zerbrechen

**option.commune**  
EN: Hear what the priest offers  
m: Hören, was der Priester anbietet  
f: Hören, was die Priesterin anbietet

**option.defend**  
EN: Stand with the living against them  
m: Mit den Lebenden gegen sie stehen

**option.ignore**  
EN: Let the dead pray  
m: Die Toten beten lassen

