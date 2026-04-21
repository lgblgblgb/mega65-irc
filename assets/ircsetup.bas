10 rem copyright (c)2026 lgb (gabor lenart) <lgblgblgb@gmail.com>
11 rem
12 rem this program is free software; you can redistribute it and/or modify
13 rem it under the terms of the gnu general public license as published by
14 rem the free software foundation; either version 2 of the license, or
15 rem (at your option) any later version.
16 rem
17 rem this program is distributed in the hope that it will be useful,
18 rem but without any warranty; without even the implied warranty of
19 rem merchantability or fitness for a particular purpose.  see the
20 rem gnu general public license for more details.
21 rem
22 rem you should have received a copy of the gnu general public license
23 rem along with this program; if not, write to the free software
24 rem foundation, inc., 59 temple place, suite 330, boston, ma  02111-1307 usa

50 print chr$(14) : scnclr

100 va=0 : ad=$1f700
102 poke ad,0
105 trap 130
110 bload"irc.cfg,s",p(ad),r
115 trap
120 if peek(ad)>0 then va=1
130 trap
140 if va then print "Found valid configuration on disk" : else print"No valid configuration found on disk"

200 rem *** setup ip address ***
210 if va then print "IRC server IP address: ";mid$(str$(peek(ad)),2);".";mid$(str$(peek(ad+1)),2);".";mid$(str$(peek(ad+2)),2);".";mid$(str$(peek(ad+3)),2)
220 gosub 9000 : if ch=0 then 1000
230 input "New IRC server IP address";i$
240 a=1:a$="":b=0:c=0
250 c$=mid$(i$,a,1) : if c$="" or c$="." then 270
260 a$=a$+c$ : a=a+1 : goto 250
270 rem print a$;" ";a
271 if a$ = "" then print "Bad IP (empty octet)" : goto 230
272 o=int(val(a$)) : o$=str$(o) : if left$(o$,1) = " " then o$=mid$(o$,2)
273 if o$<>a$ then print "Bad IP (non-integer octet): [";a$;"] vs [";o$;"]" : goto 230
274 if o < 0 or o > 255 or (o = 0 and c = 0) then print "Bad IP (octet)" : goto 230
279 poke ad+c,o
280 a$="" : a=a+1 : c=c+1
290 if c$="." and c=4 then print "Bad IP (too long)" : goto 230
300 if c$="" and c<4 then print "Bad IP (too short)" : goto 230
310 if c$<>"" then 250

1000 rem *** setup port number ***
1010 if va then print "IRC port: ";peek(ad+4) + 256*peek(ad+5)
1020 gosub 9000 : if ch=0 then 2000
1030 input "New IRC port";a
1040 a=int(a) : if a <= 0 or a >= 65535 then "Bad port number" : goto 1030
1050 poke ad+4, a and 255
1060 poke ad+5, int(a/256)

2000 rem *** setup irc nick ***
2005 if va=0 then 2100
2010 a=0 : n$=""
2020 a$=chr$(peek(ad+6+a)) : gosub 9400 : if x=0 then 2050
2030 if x>32 and x<127 then n$=n$+chr$(x)
2040 a=a+1 : goto 2020
2050 print "Current nick: [";n$;"]"
2060 if n$="" then 2110
2100 gosub 9000 : if ch=0 then 8000
2110 rem *** really change nick ***
2120 input "New IRC nick";nn$
2130 a=0
2140 a$=mid$(nn$,a+1,1) : if a$="" then poke ad+6+a,0 : goto 2300
2150 gosub 9400 : rem print "char-asc-to-store ";x
2160 if x>32 and x<127 then poke ad+6+a,x
2170 a=a+1 : goto 2140
2300 if a=0 then print "Bad nick (too short)" : goto 2120
2310 if a>15 then print "Bad nick (too long)" : goto 2120

8000 goto 9200

9000 rem *** ask if we should change ***
9002 if va=0 then ch=1 : return
9005 print "Change (y/n)? ";
9010 cursor on
9020 get a$ : ch=0
9030 if a$="y" or a$="Y" then ch=1 : goto 9060
9040 if a$="n" or a$="N" then 9060
9050 goto 9020
9060 print a$ : cursor off : return

9200 rem *** end of stuff, saving config ***
9202 if va=0 then 9220
9205 print "Save new config (y/n)? "; : gosub 9010
9210 if ch=0 then print "not saving config!!" : end
9220 print "Saving config ..."
9230 bsave"@irc.cfg,s",p(ad) to p(ad+128),r
9240 print "Configuration has been saved."
9250 end

9400 rem *** toggle character upper/lower case letters to bridge petscii vs ascii differences ***
9405 x=asc(a$)
9408 rem print "char-asc ";x
9410 if x>= 65 and x<=90  then x=x+32 : return
9420 if x>= 97 and x<=122 then x=x-32 : return
9420 if x>=193 and x<=218 then x=x-128 : return
9430 return
