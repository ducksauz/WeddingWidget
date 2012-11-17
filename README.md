WeddingWidget
=============

A blinkenlights toy for m33p and den's wedding favor.

So, my friends m33p and dennis are having an 8-bit themed wedding. They're both geeks and gamers, so it makes total sense and is awesome. They asked me for some help coming up with a wedding favor that has some sort of blinkenlights to it. I was given a budget of $5-10 per unit, the lower the better. A month or so beforehand I had seen [TinyMatrix](https://sites.google.com/site/tinymatrix/) by TigerUp pop up in my feeds and figured that had the right balance of blinking and cost. They loved it.

Since we need 50-75 of these, doing it deadbug style like TigerUp did was out of the question. PCBs and surface mount is the way to go on this one. I decided that the ATtiny88 was going to be our best value for the microconroller. So, I ordered some parts so I could start prototyping and got to work on the PCB design.

Once the breadboard prototype was built, I went to work on the minimal porting required to make the ATtiny2313 code run on an ATtiny88.
