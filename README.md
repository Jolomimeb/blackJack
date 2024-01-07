 Developed a kernel module that can be used to play a Blackjack game.
This game contains a standard deck with 52 cards. Each card has a suit and value. For example, 7 of clubs, Ace of spades, queen of hearts. In Blackjack, the dealer will deal two cards to each player. Each player will then have an option of taking more cards and attempt to get as close to 21 without going over.
An Ace will have a value of 11 unless that would give a player or the dealer a score more than 21; in which case, it has a value of 1. Face cards are worth 10 and number cards are worth the number printed on the card.


Below is a sample run of how my project works. 

$echo "Reset" > /dev/blackJack 
$ cat /dev/blackJack 
OK
$ echo "Shuffle" > /dev/blackJack 
$ cat /dev/blackJack 
OK
$ echo "Deal" > /dev/blackJack 
$ cat /dev/blackJack 
Player has 7 of Spades and 11 of Diamonds for a total of 17.
$ echo "NEXT" > /dev/blackJack 
$ cat /dev/blackJack 
Does Player want another card? If so, respond with Hit or No for hold.
$ echo "No" > /dev/blackJack 
$ cat /dev/blackJack 
Dealer has 4 of Clubs and 13 of Diamonds and 2 of Clubs and 8 of Hearts for a total of 24.
$ echo "WIN" > /dev/blackJack 
$ cat /dev/blackJack 
player has won
$ echo "CONTINUE" > /dev/blackJack 
$ cat /dev/blackJack 
Would you like to continue using the same deck?
$ echo "no" > /dev/blackJack 
$ echo "Reset" > /dev/blackJack 
$ cat /dev/blackJack 
OK
