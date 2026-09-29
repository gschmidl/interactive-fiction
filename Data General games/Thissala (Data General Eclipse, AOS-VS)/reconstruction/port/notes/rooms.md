# THISSALA — sections and rooms

`EXPRESS <section>/<room>` takes a room number **relative to its section**;
`:` works in place of `/`. The absolute room number the game uses elsewhere
(and that the emulator's `#goto` writes) is `SECTION[s] + room`.

This list was produced by running `EXPRESS s/r` for every valid pair and
recording what the game itself printed, so it is the game's own answer, not
a reading of the database.

    section   base   rooms   absolute
    1         0      125     1..125
    2         125    167     126..292
    3         292    90      293..382
    4         382    128     383..510
    5         510    140     511..650
    6         650    60      651..710

`ASSIST:19` reports the highest room as 710, `ASSIST:11` prints the SECTION
array, and `ASSIST:10` reports CSEC/CRM/CBASE for where you are standing.


## Landmarks

Codes for the places that matter, including the end-game chain the 1985
players never finished:

    1/42    Inside the local small bank      ring the bell with the withdrawal slip
    1/43    Bank Vault                       the gnome opens it
    6/49    Rangarok                         reached through the tunnel the blast opens
    6/51    Large field                      the oak flute is here
    1/113   Inside the Chevy Van             load the treasures, white START then black FORWARD
    2/13    Library                          HOOK CURTAIN TO HOOK
    1/66    Major mountain ledge             where DRAW.TH starts
    1/41    Local bank                       outside
    1/1     Middle of the parking lot        the start

`ASSIST:28` sets the end game up directly, which is faster than walking it.


## Section 1 — Surface — parking lot, river, mountains, town

125 rooms, absolute 1..125.

    1/1     1     Middle of the parking lot
    1/2     2     Parking lot (S)
    1/3     3     Parking lot (SE)
    1/4     4     Parking lot (E)
    1/5     5     Parking lot (NE)
    1/6     6     Parking lot (N)
    1/7     7     Parking lot (NW)
    1/8     8     Parking lot (W)
    1/9     9     Parking lot (SW)
    1/10    10    Dirt trail
    1/11    11    Junction in the paths
    1/12    12    West of house
    1/13    13    Behind house
    1/14    14    South of House
    1/15    15    Kitchen
    1/16    16    Living room
    1/17    17    You are at a 'Y' in the path
    1/18    18    Non-descript building
    1/19    19    You are in a hall way of the non-descript building.
    1/20    20    You are in a hall way of the non-descript building.
    1/21    21    You are in a hall way of the non-descript building.
    1/22    22    You are in a hall way of the non-descript building.
    1/23    23    You are in a hallway of the non-descript building.
    1/24    24    Strange room
    1/25    25    Silver Cube
    1/26    26    You are in a hall way of the non-descript building.
    1/27    27    You are at a 'T' in the path
    1/28    28    Riverview
    1/29    29    Southeast edge of the town
    1/30    30    Middle of the road.  You are in the middle of a road in a small town.
    1/31    31    Old Post Office
    1/32    32    Inside the Old Post Office
    1/33    33    Hardware Store
    1/34    34    Inside the hardware store
    1/35    35    Middle of the road.  You are in the middle of a road in a small town.
    1/36    36    Restaurant
    1/37    37    Inside the restaurant
    1/38    38    Old Church
    1/39    39    Inside the old church
    1/40    40    Middle of the road.  You are in the middle of a road in a small town.
    1/41    41    Local bank
    1/42    42    Inside the local small bank
    1/43    43    Bank Vault
    1/44    44    Modern building
    1/45    45    Inside the Bura di change
    1/46    46    You are at the northwest edge of the town
    1/47    47    Path through a field
    1/48    48    Outside of the old shack
    1/49    49    Inside the little shack
    1/50    50    Bend in the river
    1/51    51    Foot of the mountain
    1/52    52    Low plateau
    1/53    53    Smaller plateau
    1/54    54    Highest plateau
    1/55    55    Top of the mountain
    1/56    56    Eastern edge of the caldera
    1/57    57    Caldera (east)
    1/58    58    Caldera (west)
    1/59    59    Western edge of the caldera
    1/60    60    Acme of the minor mountain
    1/61    61    Ledge in the clouds
    1/62    62    The white ramp
    1/63    63    In the small room
    1/64    64    On the bed
    1/65    65    North of house
    1/66    66    Major mountain ledge
    1/67    67    Cave entrance
    1/68    68    Clearing in the woods
    1/69    69    Outside of the Hen House
    1/70    70    Inside the hen house
    1/71    71    You are in some woods
    1/72    72    You are in some woods
    1/73    73    You are in some woods
    1/74    74    You are in some woods
    1/75    75    You are in some woods
    1/76    76    You are in some woods
    1/77    77    You are in some woods
    1/78    78    You are in some woods
    1/79    79    You are in some woods
    1/80    80    You are in some woods
    1/81    81    You are in some woods
    1/82    82    You are in some woods
    1/83    83    You are in some woods
    1/84    84    You are in some woods
    1/85    85    You are in some woods
    1/86    86    You are in some woods
    1/87    87    You are in some woods
    1/88    88    You are in some woods
    1/89    89    You are in some woods
    1/90    90    On the bank of the river Dipestia
    1/91    91    On the bank of the river Dipestia
    1/92    92    On the bank of the river Dipestia
    1/93    93    On the bank of the river Dipestia, next to a shack
    1/94    94    Small ledge on the cliff
    1/95    95    Directly under the waterfall
    1/96    96    Middle of nowhere
    1/97    97    In the ventilation shaft
    1/98    98    Junction in the shafts
    1/99    99    Narrowing shaft
    1/100   100   You are in a plain aluminum ventilation shaft
    1/101   101   You are in a plain aluminum ventilation shaft
    1/102   102   You are in a plain aluminum ventilation shaft
    1/103   103   You are in a plain aluminum ventilation shaft
    1/104   104   You are in a plain aluminum ventilation shaft
    1/105   105   Steep incline
    1/106   106   A small room
    1/107   107   Small opening in Minor Mountain
    1/108   108   At the edge of the cliff
    1/109   109   At the edge of the cliff
    1/110   110   At the edge of the cliff
    1/111   111   On the western bank of the Great Dipestia
    1/112   112   Inside the silver room
    1/113   113   Inside the Chevy Van
    1/114   114   Hen house corner
    1/115   115   Behind the hen house
    1/116   116   Hen house corner
    1/117   117   North of hen house
    1/118   118   Hen house corner
    1/119   119   North shore
    1/120   120   Ground keeper's hut
    1/121   121   Inside the ground keeper's hut
    1/122   122   South of the hut
    1/123   123   Worn grass
    1/124   124   You are hopelessly lost in the woods
    1/125   125   You are lost hopelessly in the woods

## Section 2 — Caves and tunnels under Major mountain

167 rooms, absolute 126..292.

    2/1     126   Sloping tunnel
    2/2     127   Tunnel junction
    2/3     128   Tunnel junction
    2/4     129   Curving passage
    2/5     130   Southwest end of the great dining hall
    2/6     131   In the large dining hall
    2/7     132   In the large dining hall
    2/8     133   Northeast end of the great dining hall
    2/9     134   South chamber
    2/10    135   North chamber
    2/11    136   Pantry
    2/12    137   Beachview
    2/13    138   Library
    2/14    139   Low stone crawl
    2/15    140   Hidden room
    2/16    141   Main station
    2/17    142   In the railroad tunnel
    2/18    143   In a notch in the wall
    2/19    144   Main station (aboard the VS+EL)
    2/20    145   Main station (aboard the VS+EL)
    2/21    146   In the tunnel (aboard the VS+EL)
    2/22    147   Emerging from the tunnel (aboard the VS+EL)
    2/23    148   Gliding through the field (aboard the VS+EL)
    2/24    149   Approaching River's End station
    2/25    150   River's End station
    2/26    151   Departing River's End station
    2/27    152   Crossing over the Dipestia
    2/28    153   Field becoming a desert
    2/29    154   Approaching Dry Gulch
    2/30    155   Dry Gulch station
    2/31    156   Dry Gulch station
    2/32    157   Departing Dry Gulch
    2/33    158   Entering mountain region
    2/34    159   Dead Man Junction
    2/35    160   Passing through the mountains
    2/36    161   Approaching Stark Beach station
    2/37    162   Stark Beach Station
    2/38    163   Departing Stark Beach Station
    2/39    164   Dead Woman Junction
    2/40    165   Entering the tunnel again
    2/41    166   In the tunnel
    2/42    167   Approaching Main Station
    2/43    168   Stark Beach Station
    2/44    169   Stark Beach
    2/45    170   The VS+EL RR Lost and Found Office.
    2/46    171   Long passageway
    2/47    172   Inside Main Station
    2/48    173   The Pirates Den
    2/49    174   Dusty Passage
    2/50    175   Ore Car Room
    2/51    176   In the ore car
    2/52    177   This slot is available
    2/53    178   Wooden staircase
    2/54    179   Kitchen
    2/55    180   Servants quarters
    2/56    181   Utility room
    2/57    182   Supply room
    2/58    183   Living room
    2/59    184   T
    2/60    185   The room of the caged comedian
    2/61    186   The music room
    2/62    187   Detritus room
    2/63    188   Top of a moist sloping path
    2/64    189   Moist sloping path
    2/65    190   Bottom of a moist sloping path
    2/66    191   Rock ledge
    2/67    192   Rock precipice
    2/68    193   Cave entrance
    2/69    194   Cave passage
    2/70    195   Opening
    2/71    196   On the rocks
    2/72    197   Straight up
    2/73    198   Dead end
    2/74    199   On the Great Dipestia
    2/75    200   In sight of a small clearing
    2/76    201   Small clearing
    2/77    202   Path into the rainforest
    2/78    203   Deep in the rainforest
    2/79    204   Deeper in the rainforest
    2/80    205   Souvenir Stand
    2/81    206   Large kettle
    2/82    207   Big Tent
    2/83    208   In the big tent
    2/84    209   Junction in the paths of the rainforest
    2/85    210   Small cliff
    2/86    211   Bottom of a small cliff
    2/87    212   Viewing Pond
    2/88    213   Natural Bridge
    2/89    214   Top of a low cliff
    2/90    215   Bottom of a low cliff
    2/91    216   South end of the Tribal Burial Ground
    2/92    217   North end of the Tribal Burial Ground
    2/93    218   Rope bridge
    2/94    219   Under a rope bridge
    2/95    220   Under the natural bridge
    2/96    221   Portal Junction
    2/97    222   Top of a High Cliff
    2/98    223   Bottom of a High Cliff
    2/99    224   On the eastern shore of the Dipestia
    2/100   225   Twist in the river
    2/101   226   River in the Chasm
    2/102   227   Large underground lake
    2/103   228   Eastern landing
    2/104   229   Eastern landing
    2/105   230   Sloping tunnel
    2/106   231   Tunnel bend
    2/107   232   Dipestia continuation
    2/108   233   Large tunnel opening
    2/109   234   White water
    2/110   235   Under the VS+EL trestle
    2/111   236   Above Klein Falls
    2/112   237   Small Landing
    2/113   238   Small Landing
    2/114   239   Klein Falls
    2/115   240   Path Junction
    2/116   241   Long north/south path
    2/117   242   Break in the fence
    2/118   243   Inside the fence by the southern break.
    2/119   244   Walking along the VS+EL mainline
    2/120   245   Walking along the VS+EL mainline
    2/121   246   Walking along the VS+EL mainline
    2/122   247   Walking along the VS+EL mainline tressle
    2/123   248   Walking along the VS+EL mainline tressle
    2/124   249   Walking along the VS+EL mainline
    2/125   250   Walking along the VS+EL mainline
    2/126   251   Walking along the VS+EL mainline
    2/127   252   Walking along the VS+EL mainline
    2/128   253   Break in the northern fence
    2/129   254   Walking along the VS+EL mainline
    2/130   255   Walking along the VS+EL mainline
    2/131   256   Walking along the VS+EL mainline
    2/132   257   Walking along the VS+EL mainline
    2/133   258   Break in the fence
    2/134   259   Long path northeast/southwest
    2/135   260   Outside the castle
    2/136   261   Lower landing
    2/137   262   Lower landing
    2/138   263   Lower Dipestia
    2/139   264   Mouth of the Dispesia
    2/140   265   Milestone Cave
    2/141   266   Outside the VS+EL signal tower
    2/142   267   Inside the VS+EL signal tower
    2/143   268   South end of the castle hall
    2/144   269   Center of the castle hall
    2/145   270   North end of the castle hall
    2/146   271   In front of door one
    2/147   272   Inside room one
    2/148   273   In front of door two
    2/149   274   Inside room two
    2/150   275   In front of door three
    2/151   276   Inside room three
    2/152   277   In front of door four
    2/153   278   Inside room four
    2/154   279   In front of door five
    2/155   280   Inside room five
    2/156   281   In front of door six
    2/157   282   Inside room six
    2/158   283   In front of door seven
    2/159   284   Inside room seven
    2/160   285   Oceanside beach
    2/161   286   Oceanside beach
    2/162   287   Winding path
    2/163   288   Access trail
    2/164   289   Oceanside station
    2/165   290   Shoreline view
    2/166   291   You are adrift on the raft on the Ocean
    2/167   292   Phone booth

## Section 3 — The city (Stone Ring, rings and ramps)

90 rooms, absolute 293..382.

    3/1     293   The city's periphery (Stone Ring) and the white ramp
    3/2     294   The city's periphery (Stone Ring)
    3/3     295   The city's periphery (Stone Ring)
    3/4     296   The city's periphery (Stone Ring)
    3/5     297   The city's periphery (Stone Ring)
    3/6     298   The city's periphery (Stone Ring)
    3/7     299   The city's periphery (Stone Ring)
    3/8     300   The city's periphery (Stone Ring)
    3/9     301   On the walkway.
    3/10    302   On the walkway.
    3/11    303   The inner ring (Soil Ring)
    3/12    304   The inner ring (Soil Ring)
    3/13    305   The inner ring (Soil Ring)
    3/14    306   The inner ring (Soil Ring)
    3/15    307   The inner ring (Soil Ring)
    3/16    308   The inner ring (Soil Ring)
    3/17    309   The inner ring (Soil Ring)
    3/18    310   The inner ring (Soil Ring)
    3/19    311   Gateway to the City in the Sky
    3/20    312   The City in the Sky
    3/21    313   Outside the loquanta
    3/22    314   Inside the loquanta
    3/23    315   Park
    3/24    316   Fountain
    3/25    317   Overview
    3/26    318   MacDonalds
    3/27    319   Inside MacDonalds
    3/28    320   Poopskiers
    3/29    321   Inside the Poopskiers
    3/30    322   Walkway of the Gods
    3/31    323   The Center of the City
    3/32    324   Special room
    3/33    325   Crystal Maze
    3/34    326   Crystal Maze
    3/35    327   Crystal Maze
    3/36    328   Crystal Maze
    3/37    329   Crystal Maze
    3/38    330   Crystal Maze
    3/39    331   Crystal Maze
    3/40    332   Crystal Maze
    3/41    333   Crystal Maze
    3/42    334   Crystal Maze
    3/43    335   Crystal Maze
    3/44    336   Crystal Maze
    3/45    337   Crystal Maze
    3/46    338   Crystal Maze
    3/47    339   Crystal Maze
    3/48    340   Crystal Maze
    3/49    341   Crystal Maze
    3/50    342   Crystal Maze
    3/51    343   Crystal Maze
    3/52    344   Crystal Maze
    3/53    345   Crystal Maze
    3/54    346   Crystal Maze
    3/55    347   Crystal Maze
    3/56    348   Crystal Maze
    3/57    349   Crystal Maze
    3/58    350   Crystal Maze
    3/59    351   Crystal Maze
    3/60    352   Crystal Maze
    3/61    353   Crystal Maze
    3/62    354   Crystal Maze
    3/63    355   Crystal Maze
    3/64    356   Crystal Maze
    3/65    357   Crystal Maze
    3/66    358   Crystal Maze
    3/67    359   Crystal Maze
    3/68    360   Crystal Maze
    3/69    361   Crystal Maze
    3/70    362   Crystal Maze
    3/71    363   Crystal Maze
    3/72    364   Crystal Maze
    3/73    365   Crystal Maze
    3/74    366   Crystal Maze
    3/75    367   Crystal Maze
    3/76    368   Crystal Maze
    3/77    369   Crystal Maze
    3/78    370   Crystal Maze
    3/79    371   Crystal Maze
    3/80    372   Crystal Maze
    3/81    373   Crystal Maze
    3/82    374   Crystal Maze
    3/83    375   Crystal Maze
    3/84    376   Crystal Maze
    3/85    377   Crystal Maze
    3/86    378   Crystal Maze
    3/87    379   Crystal Maze
    3/88    380   Crystal Maze
    3/89    381   Exit from Crystal Maze
    3/90    382   MacDonald's Alcove

## Section 4 — Railroad, station and the alpine country

128 rooms, absolute 383..510.

    4/1     383   Railroad station
    4/2     384   By the railroad tracks
    4/3     385   Shade tree
    4/4     386   Path in the middle of the Desert
    4/5     387   Path in the middle of the Desert
    4/6     388   Path in the middle of the Desert
    4/7     389   Somewhere in the desert
    4/8     390   Entrance to sandstone Canyon
    4/9     391   Trail at foot of mountains
    4/10    392   Junction in trail at foot of Mountains
    4/11    393   Junction in path at foot of Mountains
    4/12    394   Junction in track at foot of Mountains
    4/13    395   Coloured rock canyon
    4/14    396   Canyon spring
    4/15    397   Sign junction
    4/16    398   Bowl in the mountains
    4/17    399   Box canyon
    4/18    400   Bottom of slide
    4/19    401   Top of slide
    4/20    402   Sweetwater spring
    4/21    403   In front of a mine
    4/22    404   Ruined cabin
    4/23    405   Campsite bend
    4/24    406   Dry Gulch Ghost Town
    4/25    407   In front of store
    4/26    408   At the north side of the store
    4/27    409   At the south side of the store
    4/28    410   Behind the store
    4/29    411   In front of the bank
    4/30    412   North side of the bank
    4/31    413   Behind the bank
    4/32    414   South side of the bank
    4/33    415   Fork
    4/34    416   Under the arch
    4/35    417   Dead end
    4/36    418   Long canyon
    4/37    419   Big Sign
    4/38    420   Guardpost
    4/39    421   Goblin mine entrance
    4/40    422   Jumbled rock bottom
    4/41    423   Lone Pine Promentory
    4/42    424   End of a gully
    4/43    425   Breezy View
    4/44    426   Red Rimrock
    4/45    427   Hut
    4/46    428   West end of arch
    4/47    429   East end of arch
    4/48    430   Windy view
    4/49    431   You are lost amongst the giant boulders on a mountain plateau.
    4/50    432   You are lost amongst the giant boulders on a mountain plateau.
    4/51    433   You are lost amongst the giant boulders on a mountain plateau.
    4/52    434   You are lost amongst the giant boulders on a mountain plateau.
    4/53    435   You are lost amongst the giant boulders on a mountain plateau.
    4/54    436   You are lost amongst the giant boulders on a mountain plateau.
    4/55    437   You are lost in the middle of some boulders on a mountain plateau.
    4/56    438   Top of jumbled rock pile
    4/57    439   South rim above mine
    4/58    440   Front of Goblin Mine Manager's house
    4/59    441   Canyon rim above ghost town
    4/60    442   High up on Black Rock Ridge
    4/61    443   Ridge path above mine
    4/62    444   Canyon edge overlooking cabin
    4/63    445   Lookout over Verdant Canyon
    4/64    446   Gully edge above rockslide
    4/65    447   Small path in the mountains
    4/66    448   Aspen Glade
    4/67    449   Marshy Valley Junction
    4/68    450   Sloping path on Pine ridge
    4/69    451   Winding path in mountains
    4/70    452   Dry hillside
    4/71    453   Rock notch
    4/72    454   Rock Ridge trail
    4/73    455   Valley floor trail
    4/74    456   Where two paths meet before a ruined tower .....
    4/75    457   Inside roofless tower of broken stone ......
    4/76    458   Inside the store
    4/77    459   Behind the counter
    4/78    460   Hut
    4/79    461   Guard shack
    4/80    462   Entrance hall
    4/81    463   Elevator
    4/82    464   Study
    4/83    465   Trophy room
    4/84    466   Kitchen
    4/85    467   Inside mine entrance
    4/86    468   First side passage
    4/87    469   Curved Passage
    4/88    470   Hole chamber
    4/89    471   Bottom of hole
    4/90    472   Angled junction
    4/91    473   Sideways T-junction
    4/92    474   Sideways T-junction
    4/93    475   Rocky chamber
    4/94    476   Behind big rock
    4/95    477   Narrow Squeeze Passage
    4/96    478   Fourways junction
    4/97    479   Elbow passage room
    4/98    480   Broad chamber
    4/99    481   Faint Breeze Passage
    4/100   482   View
    4/101   483   Hole in floor
    4/102   484   Hole in roof
    4/103   485   High passage bend
    4/104   486   Triangle junction
    4/105   487   End diggins
    4/106   488   Where the *%&$ am I ?
    4/107   489   Dusty chamber
    4/108   490   Sloping chamber
    4/109   491   Deep Tight Crawl
    4/110   492   Hard Place
    4/111   493   Hallway
    4/112   494   End of hallway
    4/113   495   Desert encampment
    4/114   496   East tent
    4/115   497   West tent
    4/116   498   Desert cheiftan's tent
    4/117   499   Sandy hills
    4/118   500   Rock Dome
    4/119   501   Woody valley
    4/120   502   Scrub bushes
    4/121   503   Grassy knoll
    4/122   504   Marshy dell
    4/123   505   Rocky plateau
    4/124   506   Piney flats
    4/125   507   Alpine meadow
    4/126   508   Alpine meadow
    4/127   509   Alpine meadow
    4/128   510   Alpine meadow

## Section 5 — The Pyramid

140 rooms, absolute 511..650.

    5/1     511   Entrance to the Pyramid
    5/2     512   Pit Room
    5/3     513   Above the Pit
    5/4     514   Rounded Archway
    5/5     515   Anteroom
    5/6     516   Slaves' chamber
    5/7     517   Embalming Chamber
    5/8     518   Small Room
    5/9     519   Burial Chamber
    5/10    520   Treasure Room
    5/11    521   Northern Wall
    5/12    522   Long Corridor
    5/13    523   Long Corridor
    5/14    524   Long Corridor
    5/15    525   Long Corridor
    5/16    526   Long Corridor
    5/17    527   Long Corridor
    5/18    528   Long Corridor
    5/19    529   Long Corridor
    5/20    530   Display Room
    5/21    531   Escape passage
    5/22    532   Junction in the escape passage
    5/23    533   Subterranean Jungle Path
    5/24    534   Subterranean Field
    5/25    535   Ancient Ziggurat
    5/26    536   Musty Hole
    5/27    537   Hittite Room
    5/28    538   Babalonian Room
    5/29    539   Prayer Room
    5/30    540   Museum-display
    5/31    541   Exchange room
    5/32    542   Scroll Room
    5/33    543   Sacrificial Chamber
    5/34    544   Priests' Chamber
    5/35    545   Warrior's Chamber
    5/36    546   Emperor's Room
    5/37    547   Emperor's Bedroom
    5/38    548   Children's Room
    5/39    549   Harem room West
    5/40    550   Harem Room East
    5/41    551   Number 1 Wife's Chamber
    5/42    552   Royal Escape Passage
    5/43    553   Damp Passage
    5/44    554   Storage cave
    5/45    555   Empty Cave
    5/46    556   Magnificent Cavern
    5/47    557   Stream
    5/48    558   Hot spring
    5/49    559   Lava Bowl
    5/50    560   Lava Flow
    5/51    561   Pool Room
    5/52    562   Bottom of the Pool
    5/53    563   Underground Lake
    5/54    564   Southeast Lakeshore
    5/55    565   Mouth of a Stream
    5/56    566   Stalagmite Cavern
    5/57    567   Stream Path
    5/58    568   Flint Cave
    5/59    569   Tool Cavern
    5/60    570   Wood Archway
    5/61    571   Paint-mixing Pit
    5/62    572   Gaming Room
    5/63    573   Brush Passage
    5/64    574   Cave Paintings
    5/65    575   Skin Room
    5/66    576   Bone Pit
    5/67    577   Burial Area
    5/68    578   Sacrificial pit
    5/69    579   Ledgge in the Pit
    5/70    580   Bottom of the Pit
    5/71    581   Gravel Room
    5/72    582   Australopithicine crawl
    5/73    583   Neanderthal Grotto
    5/74    584   Cro-Magnon Hollow
    5/75    585   Amer-Indian Chamber
    5/76    586   Glacier Room
    5/77    587   Toltec Cavern
    5/78    588   Aztec Cavity
    5/79    589   Inca Chapel
    5/80    590   Strange Tunnel
    5/81    591   Machine Cave
    5/82    592   Plain Cavern
    5/83    593   Air Lock
    5/84    594   Outside the Spaceship
    5/85    595   Starboard Side
    5/86    596   Antenna Array
    5/87    597   Port Side
    5/88    598   Outside Viewing Area
    5/89    599   Main Engine Cowl
    5/90    600   Port Engine Cowl
    5/91    601   Starbord Engine Cowl
    5/92    602   Out In Space
    5/93    603   Bunker Entrance
    5/94    604   Intelligence Room
    5/95    605   Main Rotunda
    5/96    606   Ammunition Room
    5/97    607   Machine gunner's pit
    5/98    608   Howitzer Room
    5/99    609   Mined Area
    5/100   610   Museum Entrance
    5/101   611   Hall of Tranquility
    5/102   612   Orchestra Pit
    5/103   613   Air Pocket
    5/104   614   Great Rotunda
    5/105   615   Routunda South
    5/106   616   Rotunda East
    5/107   617   Rotunda North
    5/108   618   Center of the Rotunda
    5/109   619   Gallery of Games
    5/110   620   Adventure Room
    5/111   621   ZORK Room
    5/112   622   Thissala Room
    5/113   623   Hall of Natural Science
    5/114   624   Androvian War Room
    5/115   625   Hittite War Room
    5/116   626   Babalonian War Room
    5/117   627   Room of Devestation
    5/118   628   Reptile Room
    5/119   629   Zoo Room
    5/120   630   Hall of Music
    5/121   631   Room of Strings
    5/122   632   Room of the String Section
    5/123   633   Woodwind Room
    5/124   634   Chamber of sound
    5/125   635   Brass Room
    5/126   636   Percussion Room
    5/127   637   Michaelangelo Room
    5/128   638   British Room
    5/129   639   Greece Room
    5/130   640   Rome Room
    5/131   641   Room of Modern Art
    5/132   642   Hall of Science.
    5/133   643   Astronomy Room
    5/134   644   Hall of Oceanography
    5/135   645   Meteorology Room
    5/136   646   Hall of Physics
    5/137   647   Rest Area
    5/138   648   Da Vinci Display Room
    5/139   649   Hall of Mathematics
    5/140   650   Room of Silence

## Section 6 — Lower river and the checkpoint

60 rooms, absolute 651..710.

    6/1     651   Lower river along the shore
    6/2     652   Fossils of bones
    6/3     653   Thick mist
    6/4     654   Western extreme of the bridge
    6/5     655   Darkness dwell
    6/6     656   Upper river
    6/7     657   Rocky terrain
    6/8     658   Riverstart
    6/9     659   Lakeview
    6/10    660   Dead end
    6/11    661   Lower river east
    6/12    662   Foggy bank
    6/13    663   Eastern extreme of the bridge
    6/14    664   Windy cross
    6/15    665   Rocky terrain
    6/16    666   Riverstart
    6/17    667   Lakeview
    6/18    668   Tunnel entrance
    6/19    669   In the tunnel
    6/20    670   Shore of the lake
    6/21    671   Bridge over the river
    6/22    672   The Silver tower
    6/23    673   Base of the iron Ladder
    6/24    674   Lake level platform
    6/25    675   Base of a stainless steel ladder
    6/26    676   Top of the stainless steel ladder
    6/27    677   Silver curtain of light
    6/28    678   Inside the silver room.
    6/29    679   Inside the little boat
    6/30    680   Warning room
    6/31    681   Whoosh ...
    6/32    682   The Adytum
    6/33    683   Ice castle room
    6/34    684   Gelid cave
    6/35    685   Divagaters Rest
    6/36    686   The Calefactory
    6/37    687   Travertine Cavern
    6/38    688   Hall of the Fountains
    6/39    689   Rainbow Chasm
    6/40    690   Map room
    6/41    691   Weapons room
    6/42    692   New tunnel
    6/43    693   Steep incline
    6/44    694   Base of the stone steps
    6/45    695   On the stone steps
    6/46    696   Top of the stone steps
    6/47    697   Eerie tunnel
    6/48    698   Small room
    6/49    699   Rangarok
    6/50    700   Warm tunnel
    6/51    701   Large field
    6/52    702   You're in Debris Room.
    6/53    703   You're at "Y2".
    6/54    704   You're in Plover Room.
    6/55    705   You're inside building.
    6/56    706   Monster home
    6/57    707   Monster home 2
    6/58    708   Above the iron ladder
    6/59    709   On the upper platform
    6/60    710   Checkpoint


## Index by name

Where a name occurs more than a few times it is a maze; only the first
codes are listed, with the count.

    A small room                                               1/106
    Above Klein Falls                                          2/111
    Above the iron ladder                                      6/58
    Above the Pit                                              5/3
    Access trail                                               2/163
    Acme of the minor mountain                                 1/60
    Adventure Room                                             5/110
    Air Lock                                                   5/83
    Air Pocket                                                 5/103
    Alpine meadow                                              4/125, 4/126, 4/127, 4/128
    Amer-Indian Chamber                                        5/75
    Ammunition Room                                            5/96
    Ancient Ziggurat                                           5/25
    Androvian War Room                                         5/114
    Angled junction                                            4/90
    Antenna Array                                              5/86
    Anteroom                                                   5/5
    Approaching Dry Gulch                                      2/29
    Approaching Main Station                                   2/42
    Approaching River's End station                            2/24
    Approaching Stark Beach station                            2/36
    Aspen Glade                                                4/66
    Astronomy Room                                             5/133
    At the edge of the cliff                                   1/108, 1/109, 1/110
    At the north side of the store                             4/26
    At the south side of the store                             4/27
    Australopithicine crawl                                    5/72
    Aztec Cavity                                               5/78
    Babalonian Room                                            5/28
    Babalonian War Room                                        5/116
    Bank Vault                                                 1/43
    Base of a stainless steel ladder                           6/25
    Base of the iron Ladder                                    6/23
    Base of the stone steps                                    6/44
    Beachview                                                  2/12
    Behind big rock                                            4/94
    Behind house                                               1/13
    Behind the bank                                            4/31
    Behind the counter                                         4/77
    Behind the hen house                                       1/115
    Behind the store                                           4/28
    Bend in the river                                          1/50
    Big Sign                                                   4/37
    Big Tent                                                   2/82
    Bone Pit                                                   5/66
    Bottom of a High Cliff                                     2/98
    Bottom of a low cliff                                      2/90
    Bottom of a moist sloping path                             2/65
    Bottom of a small cliff                                    2/86
    Bottom of hole                                             4/89
    Bottom of slide                                            4/18
    Bottom of the Pit                                          5/70
    Bottom of the Pool                                         5/52
    Bowl in the mountains                                      4/16
    Box canyon                                                 4/17
    Brass Room                                                 5/125
    Break in the fence                                         2/117, 2/133
    Break in the northern fence                                2/128
    Breezy View                                                4/43
    Bridge over the river                                      6/21
    British Room                                               5/128
    Broad chamber                                              4/98
    Brush Passage                                              5/63
    Bunker Entrance                                            5/93
    Burial Area                                                5/67
    Burial Chamber                                             5/9
    By the railroad tracks                                     4/2
    Caldera (east)                                             1/57
    Caldera (west)                                             1/58
    Campsite bend                                              4/23
    Canyon edge overlooking cabin                              4/62
    Canyon rim above ghost town                                4/59
    Canyon spring                                              4/14
    Cave entrance                                              1/67, 2/68
    Cave Paintings                                             5/64
    Cave passage                                               2/69
    Center of the castle hall                                  2/144
    Center of the Rotunda                                      5/108
    Chamber of sound                                           5/124
    Checkpoint                                                 6/60
    Children's Room                                            5/38
    Clearing in the woods                                      1/68
    Coloured rock canyon                                       4/13
    Cro-Magnon Hollow                                          5/74
    Crossing over the Dipestia                                 2/27
    Crystal Maze                                               3/33, 3/34, 3/35 ... (56 rooms)
    Curved Passage                                             4/87
    Curving passage                                            2/4
    Da Vinci Display Room                                      5/138
    Damp Passage                                               5/43
    Darkness dwell                                             6/5
    Dead end                                                   2/73, 4/35, 6/10
    Dead Man Junction                                          2/34
    Dead Woman Junction                                        2/39
    Deep in the rainforest                                     2/78
    Deep Tight Crawl                                           4/109
    Deeper in the rainforest                                   2/79
    Departing Dry Gulch                                        2/32
    Departing River's End station                              2/26
    Departing Stark Beach Station                              2/38
    Desert cheiftan's tent                                     4/116
    Desert encampment                                          4/113
    Detritus room                                              2/62
    Dipestia continuation                                      2/107
    Directly under the waterfall                               1/95
    Dirt trail                                                 1/10
    Display Room                                               5/20
    Divagaters Rest                                            6/35
    Dry Gulch Ghost Town                                       4/24
    Dry Gulch station                                          2/30, 2/31
    Dry hillside                                               4/70
    Dusty chamber                                              4/107
    Dusty Passage                                              2/49
    East end of arch                                           4/47
    East tent                                                  4/114
    Eastern edge of the caldera                                1/56
    Eastern extreme of the bridge                              6/13
    Eastern landing                                            2/103, 2/104
    Eerie tunnel                                               6/47
    Elbow passage room                                         4/97
    Elevator                                                   4/81
    Embalming Chamber                                          5/7
    Emerging from the tunnel (aboard the VS+EL)                2/22
    Emperor's Bedroom                                          5/37
    Emperor's Room                                             5/36
    Empty Cave                                                 5/45
    End diggins                                                4/105
    End of a gully                                             4/42
    End of hallway                                             4/112
    Entering mountain region                                   2/33
    Entering the tunnel again                                  2/40
    Entrance hall                                              4/80
    Entrance to sandstone Canyon                               4/8
    Entrance to the Pyramid                                    5/1
    Escape passage                                             5/21
    Exchange room                                              5/31
    Exit from Crystal Maze                                     3/89
    Faint Breeze Passage                                       4/99
    Field becoming a desert                                    2/28
    First side passage                                         4/86
    Flint Cave                                                 5/58
    Foggy bank                                                 6/12
    Foot of the mountain                                       1/51
    Fork                                                       4/33
    Fossils of bones                                           6/2
    Fountain                                                   3/24
    Fourways junction                                          4/96
    Front of Goblin Mine Manager's house                       4/58
    Gallery of Games                                           5/109
    Gaming Room                                                5/62
    Gateway to the City in the Sky                             3/19
    Gelid cave                                                 6/34
    Glacier Room                                               5/76
    Gliding through the field (aboard the VS+EL)               2/23
    Goblin mine entrance                                       4/39
    Grassy knoll                                               4/121
    Gravel Room                                                5/71
    Great Rotunda                                              5/104
    Greece Room                                                5/129
    Ground keeper's hut                                        1/120
    Guard shack                                                4/79
    Guardpost                                                  4/38
    Gully edge above rockslide                                 4/64
    Hall of Mathematics                                        5/139
    Hall of Music                                              5/120
    Hall of Natural Science                                    5/113
    Hall of Oceanography                                       5/134
    Hall of Physics                                            5/136
    Hall of Science.                                           5/132
    Hall of the Fountains                                      6/38
    Hall of Tranquility                                        5/101
    Hallway                                                    4/111
    Hard Place                                                 4/110
    Hardware Store                                             1/33
    Harem Room East                                            5/40
    Harem room West                                            5/39
    Hen house corner                                           1/114, 1/116, 1/118
    Hidden room                                                2/15
    High passage bend                                          4/103
    High up on Black Rock Ridge                                4/60
    Highest plateau                                            1/54
    Hittite Room                                               5/27
    Hittite War Room                                           5/115
    Hole chamber                                               4/88
    Hole in floor                                              4/101
    Hole in roof                                               4/102
    Hot spring                                                 5/48
    Howitzer Room                                              5/98
    Hut                                                        4/45, 4/78
    Ice castle room                                            6/33
    In a notch in the wall                                     2/18
    In front of a mine                                         4/21
    In front of door five                                      2/154
    In front of door four                                      2/152
    In front of door one                                       2/146
    In front of door seven                                     2/158
    In front of door six                                       2/156
    In front of door three                                     2/150
    In front of door two                                       2/148
    In front of store                                          4/25
    In front of the bank                                       4/29
    In sight of a small clearing                               2/75
    In the big tent                                            2/83
    In the large dining hall                                   2/6, 2/7
    In the ore car                                             2/51
    In the railroad tunnel                                     2/17
    In the small room                                          1/63
    In the tunnel                                              2/41, 6/19
    In the tunnel (aboard the VS+EL)                           2/21
    In the ventilation shaft                                   1/97
    Inca Chapel                                                5/79
    Inside MacDonalds                                          3/27
    Inside Main Station                                        2/47
    Inside mine entrance                                       4/85
    Inside roofless tower of broken stone ......               4/75
    Inside room five                                           2/155
    Inside room four                                           2/153
    Inside room one                                            2/147
    Inside room seven                                          2/159
    Inside room six                                            2/157
    Inside room three                                          2/151
    Inside room two                                            2/149
    Inside the Bura di change                                  1/45
    Inside the Chevy Van                                       1/113
    Inside the fence by the southern break.                    2/118
    Inside the ground keeper's hut                             1/121
    Inside the hardware store                                  1/34
    Inside the hen house                                       1/70
    Inside the little boat                                     6/29
    Inside the little shack                                    1/49
    Inside the local small bank                                1/42
    Inside the loquanta                                        3/22
    Inside the old church                                      1/39
    Inside the Old Post Office                                 1/32
    Inside the Poopskiers                                      3/29
    Inside the restaurant                                      1/37
    Inside the silver room                                     1/112
    Inside the silver room.                                    6/28
    Inside the store                                           4/76
    Inside the VS+EL signal tower                              2/142
    Intelligence Room                                          5/94
    Jumbled rock bottom                                        4/40
    Junction in path at foot of Mountains                      4/11
    Junction in the escape passage                             5/22
    Junction in the paths                                      1/11
    Junction in the paths of the rainforest                    2/84
    Junction in the shafts                                     1/98
    Junction in track at foot of Mountains                     4/12
    Junction in trail at foot of Mountains                     4/10
    Kitchen                                                    1/15, 2/54, 4/84
    Klein Falls                                                2/114
    Lake level platform                                        6/24
    Lakeview                                                   6/9, 6/17
    Large field                                                6/51
    Large kettle                                               2/81
    Large tunnel opening                                       2/108
    Large underground lake                                     2/102
    Lava Bowl                                                  5/49
    Lava Flow                                                  5/50
    Ledge in the clouds                                        1/61
    Ledgge in the Pit                                          5/69
    Library                                                    2/13
    Living room                                                1/16, 2/58
    Local bank                                                 1/41
    Lone Pine Promentory                                       4/41
    Long canyon                                                4/36
    Long Corridor                                              5/12, 5/13, 5/14 ... (8 rooms)
    Long north/south path                                      2/116
    Long passageway                                            2/46
    Long path northeast/southwest                              2/134
    Lookout over Verdant Canyon                                4/63
    Low plateau                                                1/52
    Low stone crawl                                            2/14
    Lower Dipestia                                             2/138
    Lower landing                                              2/136, 2/137
    Lower river along the shore                                6/1
    Lower river east                                           6/11
    MacDonald's Alcove                                         3/90
    MacDonalds                                                 3/26
    Machine Cave                                               5/81
    Machine gunner's pit                                       5/97
    Magnificent Cavern                                         5/46
    Main Engine Cowl                                           5/89
    Main Rotunda                                               5/95
    Main station                                               2/16
    Main station (aboard the VS+EL)                            2/19, 2/20
    Major mountain ledge                                       1/66
    Map room                                                   6/40
    Marshy dell                                                4/122
    Marshy Valley Junction                                     4/67
    Meteorology Room                                           5/135
    Michaelangelo Room                                         5/127
    Middle of nowhere                                          1/96
    Middle of the parking lot                                  1/1
    Middle of the road.  You are in the middle of a road in a  1/30, 1/35, 1/40
    Milestone Cave                                             2/140
    Mined Area                                                 5/99
    Modern building                                            1/44
    Moist sloping path                                         2/64
    Monster home                                               6/56
    Monster home 2                                             6/57
    Mouth of a Stream                                          5/55
    Mouth of the Dispesia                                      2/139
    Museum Entrance                                            5/100
    Museum-display                                             5/30
    Musty Hole                                                 5/26
    Narrow Squeeze Passage                                     4/95
    Narrowing shaft                                            1/99
    Natural Bridge                                             2/88
    Neanderthal Grotto                                         5/73
    New tunnel                                                 6/42
    Non-descript building                                      1/18
    North chamber                                              2/10
    North end of the castle hall                               2/145
    North end of the Tribal Burial Ground                      2/92
    North of hen house                                         1/117
    North of house                                             1/65
    North shore                                                1/119
    North side of the bank                                     4/30
    Northeast end of the great dining hall                     2/8
    Northern Wall                                              5/11
    Number 1 Wife's Chamber                                    5/41
    Oceanside beach                                            2/160, 2/161
    Oceanside station                                          2/164
    Old Church                                                 1/38
    Old Post Office                                            1/31
    On the bank of the river Dipestia                          1/90, 1/91, 1/92
    On the bank of the river Dipestia, next to a shack         1/93
    On the bed                                                 1/64
    On the eastern shore of the Dipestia                       2/99
    On the Great Dipestia                                      2/74
    On the rocks                                               2/71
    On the stone steps                                         6/45
    On the upper platform                                      6/59
    On the walkway.                                            3/9, 3/10
    On the western bank of the Great Dipestia                  1/111
    Opening                                                    2/70
    Orchestra Pit                                              5/102
    Ore Car Room                                               2/50
    Out In Space                                               5/92
    Outside of the Hen House                                   1/69
    Outside of the old shack                                   1/48
    Outside the castle                                         2/135
    Outside the loquanta                                       3/21
    Outside the Spaceship                                      5/84
    Outside the VS+EL signal tower                             2/141
    Outside Viewing Area                                       5/88
    Overview                                                   3/25
    Paint-mixing Pit                                           5/61
    Pantry                                                     2/11
    Park                                                       3/23
    Parking lot (E)                                            1/4
    Parking lot (N)                                            1/6
    Parking lot (NE)                                           1/5
    Parking lot (NW)                                           1/7
    Parking lot (S)                                            1/2
    Parking lot (SE)                                           1/3
    Parking lot (SW)                                           1/9
    Parking lot (W)                                            1/8
    Passing through the mountains                              2/35
    Path in the middle of the Desert                           4/4, 4/5, 4/6
    Path into the rainforest                                   2/77
    Path Junction                                              2/115
    Path through a field                                       1/47
    Percussion Room                                            5/126
    Phone booth                                                2/167
    Piney flats                                                4/124
    Pit Room                                                   5/2
    Plain Cavern                                               5/82
    Pool Room                                                  5/51
    Poopskiers                                                 3/28
    Port Engine Cowl                                           5/90
    Port Side                                                  5/87
    Portal Junction                                            2/96
    Prayer Room                                                5/29
    Priests' Chamber                                           5/34
    Railroad station                                           4/1
    Rainbow Chasm                                              6/39
    Rangarok                                                   6/49
    Red Rimrock                                                4/44
    Reptile Room                                               5/118
    Rest Area                                                  5/137
    Restaurant                                                 1/36
    Ridge path above mine                                      4/61
    River in the Chasm                                         2/101
    River's End station                                        2/25
    Riverstart                                                 6/8, 6/16
    Riverview                                                  1/28
    Rock Dome                                                  4/118
    Rock ledge                                                 2/66
    Rock notch                                                 4/71
    Rock precipice                                             2/67
    Rock Ridge trail                                           4/72
    Rocky chamber                                              4/93
    Rocky plateau                                              4/123
    Rocky terrain                                              6/7, 6/15
    Rome Room                                                  5/130
    Room of Devestation                                        5/117
    Room of Modern Art                                         5/131
    Room of Silence                                            5/140
    Room of Strings                                            5/121
    Room of the String Section                                 5/122
    Rope bridge                                                2/93
    Rotunda East                                               5/106
    Rotunda North                                              5/107
    Rounded Archway                                            5/4
    Routunda South                                             5/105
    Royal Escape Passage                                       5/42
    Ruined cabin                                               4/22
    Sacrificial Chamber                                        5/33
    Sacrificial pit                                            5/68
    Sandy hills                                                4/117
    Scroll Room                                                5/32
    Scrub bushes                                               4/120
    Servants quarters                                          2/55
    Shade tree                                                 4/3
    Shore of the lake                                          6/20
    Shoreline view                                             2/165
    Sideways T-junction                                        4/91, 4/92
    Sign junction                                              4/15
    Silver Cube                                                1/25
    Silver curtain of light                                    6/27
    Skin Room                                                  5/65
    Slaves' chamber                                            5/6
    Sloping chamber                                            4/108
    Sloping path on Pine ridge                                 4/68
    Sloping tunnel                                             2/1, 2/105
    Small clearing                                             2/76
    Small cliff                                                2/85
    Small Landing                                              2/112, 2/113
    Small ledge on the cliff                                   1/94
    Small opening in Minor Mountain                            1/107
    Small path in the mountains                                4/65
    Small Room                                                 5/8
    Small room                                                 6/48
    Smaller plateau                                            1/53
    Somewhere in the desert                                    4/7
    South chamber                                              2/9
    South end of the castle hall                               2/143
    South end of the Tribal Burial Ground                      2/91
    South of House                                             1/14
    South of the hut                                           1/122
    South rim above mine                                       4/57
    South side of the bank                                     4/32
    Southeast edge of the town                                 1/29
    Southeast Lakeshore                                        5/54
    Southwest end of the great dining hall                     2/5
    Souvenir Stand                                             2/80
    Special room                                               3/32
    Stalagmite Cavern                                          5/56
    Starboard Side                                             5/85
    Starbord Engine Cowl                                       5/91
    Stark Beach                                                2/44
    Stark Beach Station                                        2/37, 2/43
    Steep incline                                              1/105, 6/43
    Storage cave                                               5/44
    Straight up                                                2/72
    Strange room                                               1/24
    Strange Tunnel                                             5/80
    Stream                                                     5/47
    Stream Path                                                5/57
    Study                                                      4/82
    Subterranean Field                                         5/24
    Subterranean Jungle Path                                   5/23
    Supply room                                                2/57
    Sweetwater spring                                          4/20
    T                                                          2/59
    The Adytum                                                 6/32
    The Calefactory                                            6/36
    The Center of the City                                     3/31
    The City in the Sky                                        3/20
    The city's periphery (Stone Ring)                          3/2, 3/3, 3/4 ... (7 rooms)
    The city's periphery (Stone Ring) and the white ramp       3/1
    The inner ring (Soil Ring)                                 3/11, 3/12, 3/13 ... (8 rooms)
    The music room                                             2/61
    The Pirates Den                                            2/48
    The room of the caged comedian                             2/60
    The Silver tower                                           6/22
    The VS+EL RR Lost and Found Office.                        2/45
    The white ramp                                             1/62
    Thick mist                                                 6/3
    This slot is available                                     2/52
    Thissala Room                                              5/112
    Toltec Cavern                                              5/77
    Tool Cavern                                                5/59
    Top of a High Cliff                                        2/97
    Top of a low cliff                                         2/89
    Top of a moist sloping path                                2/63
    Top of jumbled rock pile                                   4/56
    Top of slide                                               4/19
    Top of the mountain                                        1/55
    Top of the stainless steel ladder                          6/26
    Top of the stone steps                                     6/46
    Trail at foot of mountains                                 4/9
    Travertine Cavern                                          6/37
    Treasure Room                                              5/10
    Triangle junction                                          4/104
    Trophy room                                                4/83
    Tunnel bend                                                2/106
    Tunnel entrance                                            6/18
    Tunnel junction                                            2/2, 2/3
    Twist in the river                                         2/100
    Under a rope bridge                                        2/94
    Under the arch                                             4/34
    Under the natural bridge                                   2/95
    Under the VS+EL trestle                                    2/110
    Underground Lake                                           5/53
    Upper river                                                6/6
    Utility room                                               2/56
    Valley floor trail                                         4/73
    View                                                       4/100
    Viewing Pond                                               2/87
    Walking along the VS+EL mainline                           2/119, 2/120, 2/121 ... (11 rooms)
    Walking along the VS+EL mainline tressle                   2/122, 2/123
    Walkway of the Gods                                        3/30
    Warm tunnel                                                6/50
    Warning room                                               6/30
    Warrior's Chamber                                          5/35
    Weapons room                                               6/41
    West end of arch                                           4/46
    West of house                                              1/12
    West tent                                                  4/115
    Western edge of the caldera                                1/59
    Western extreme of the bridge                              6/4
    Where the *%&$ am I ?                                      4/106
    Where two paths meet before a ruined tower .....           4/74
    White water                                                2/109
    Whoosh ...                                                 6/31
    Winding path                                               2/162
    Winding path in mountains                                  4/69
    Windy cross                                                6/14
    Windy view                                                 4/48
    Wood Archway                                               5/60
    Wooden staircase                                           2/53
    Woodwind Room                                              5/123
    Woody valley                                               4/119
    Worn grass                                                 1/123
    You are adrift on the raft on the Ocean                    2/166
    You are at a 'T' in the path                               1/27
    You are at a 'Y' in the path                               1/17
    You are at the northwest edge of the town                  1/46
    You are hopelessly lost in the woods                       1/124
    You are in a hall way of the non-descript building.        1/19, 1/20, 1/21 ... (5 rooms)
    You are in a hallway of the non-descript building.         1/23
    You are in a plain aluminum ventilation shaft              1/100, 1/101, 1/102 ... (5 rooms)
    You are in some woods                                      1/71, 1/72, 1/73 ... (19 rooms)
    You are lost amongst the giant boulders on a mountain plat 4/49, 4/50, 4/51 ... (6 rooms)
    You are lost hopelessly in the woods                       1/125
    You are lost in the middle of some boulders on a mountain  4/55
    You're at "Y2".                                            6/53
    You're in Debris Room.                                     6/52
    You're in Plover Room.                                     6/54
    You're inside building.                                    6/55
    Zoo Room                                                   5/119
    ZORK Room                                                  5/111
