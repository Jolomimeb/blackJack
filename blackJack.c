#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/mutex.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oritsejolomisan");
MODULE_DESCRIPTION("A kernel module that can be used to play a Blackjack game");
MODULE_VERSION("0.1");

//define the device name
#define DEVICE_NAME "blackJack"
//functions definition
static int device_driver_open(struct inode *inode, struct file *file);
static int device_driver_close(struct inode *inode, struct file *file);
static ssize_t device_driver_read(struct file *file, char *buff, size_t len, loff_t *offset);
static ssize_t device_driver_write(struct file *file, const char *buff, size_t len, loff_t *offset);


static const char nogame[] = "No game in progress\n";
static char * msg_ptr = (char * ) nogame;
static const char ok[] = "OK\n";
static const char invalid[] = "INVALID COMMAND\n";
static const char noMore[] = "No more cards in the deck.\n";
static const char handFull[] = "Hand full.\n";
static const char here[] = "HERE\n";
static const char player_win[] = "player has won\n";
static const char dealer_win[] = "dealer has won\n";
static const char more_cards[] = "Does Player want another card? If so, respond with Hit or No for hold.\n";
static const char continue_game[] = "Would you like to continue using the same deck?\n";
static const char invalidR[] = "INVALID COMMAND, must Reset first\n";
static const char invalidS[] = "INVALID COMMAND, must Shuffle first\n";
static const char dealerMore[] = "Dealer takes another card,\n";

#define MAX_HAND_SIZE 11
#define BUFF_LEN 80
#define DECK_SIZE 52
static char command[BUFF_LEN];
#define BUFFER_SIZE 256
static char msg_buffer[BUFFER_SIZE];
static char msg_buffer2[BUFFER_SIZE];
static DEFINE_MUTEX(blackJack_mutex);

typedef enum {
    HEARTS, DIAMONDS, CLUBS, SPADES
} Suit;

typedef struct {
    Suit suit;
    // 1 for Ace, 2-10 for numbers, 11 for Jack, 12 for Queen, 13 for King
    int value;
} Card;

typedef struct {
    Card hand[MAX_HAND_SIZE];
    int num_cards; // Number of cards in hand
    int score; // Total score based on the hand

} Player;

// Initialize game components
Card deck[DECK_SIZE];
Player player, dealer;

//helper functions
// Function declarations
void resetGame(Player *player, Player *dealer);
void shuffleDeck(Card *deck);
void dealCard(Player *player, Card *deck) ;
void player_play(Player *player, Player *dealer, Card *deck);
void dealer_play(Player *dealer, Card *deck);
bool determineWinner(Player *player, Player *dealer);
char* getSuitName(Suit suit);
void displayPlayerHand(const char *entity, Player *player);
void displayDealerHand(const char *entity, Player *player);

static struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = device_driver_open,
        .release = device_driver_close,
        .write = device_driver_write,
        .read = device_driver_read,
};

static struct miscdevice blackJack_driver = {
        .name = DEVICE_NAME,
        .minor = MISC_DYNAMIC_MINOR,
        .fops = &fops,
        .mode = 0666,
};

static int device_driver_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "device open for blackJack was called\n");
    return 0;
}

static int device_driver_close(struct inode *inode, struct file *file) {
    printk(KERN_INFO "device close for blackJack was called\n");
    return 0;
}

static ssize_t device_driver_read(struct file * file, char * buffer, size_t length, loff_t * offset) {

    //mutex_lock(&blackJack_mutex);
    //read from device
    int bytes_to_read = 0;

    if ( * msg_ptr == 0) {

        return 0;
    }
    while (length && * msg_ptr) {
        put_user( * (msg_ptr++), buffer++);
        bytes_to_read++;
        length--;
    }
    //mutex_unlock(&blackJack_mutex);
    return bytes_to_read;

}

static ssize_t device_driver_write(struct file *file, const char *buffer, size_t len, loff_t *offset) {
    //mutex_lock(&blackJack_mutex);

    //  struct to hold state checks
    static struct {
        bool is_reset;
        bool is_shuffle;
        bool is_deal;
        bool check_win;
        bool is_next;
        bool game_over;
    } checks = {false, false, false, false, false, false};

    // Check for buffer size
    if (len > sizeof(command) - 1) {
        //mutex_unlock(&blackJack_mutex);
        return -EINVAL;
    }

    // Copy data from user
    if (copy_from_user(command, buffer, len)) {
        //mutex_unlock(&blackJack_mutex);
        return -EFAULT;
    }
    command[len] = '\0'; // Null-terminate the command

    // Process "Reset" command
    if (strncmp(command, "Reset", 5) == 0) {
        resetGame(&player, &dealer);
        checks.is_reset = true;
        checks.is_shuffle = false;
        checks.is_deal = false;
        checks.check_win = false;
        checks.is_next = false;
        checks.game_over = false;
        printk(KERN_INFO "Game reset\n");
    }

    // Process "Shuffle" command
    if (strncmp(command, "Shuffle", 7) == 0) {
        if (checks.is_reset) {
            shuffleDeck(deck);
            checks.is_shuffle = true;
            printk(KERN_INFO "shuffled\n");
        } else {
            msg_ptr = (char * ) invalidR;
            printk(KERN_INFO "Invalid command, Reset first\n");
            //mutex_unlock(&blackJack_mutex);
            return len;
        }
    }

    // Process "Deal" command
    if (strncmp(command, "Deal", 4) == 0) {
        if (checks.is_shuffle) {
            //deal 2 cards to the player
            dealCard(&player, deck);
            dealCard(&player, deck);

            //deal 2 cards to the dealer
            dealCard(&dealer, deck);
            dealCard(&dealer, deck);

            //display the player and dealers cards
            displayPlayerHand("Player", &player);
            //displayDealerHand("Dealer", &dealer);
            checks.is_deal = true;
        } else {
            msg_ptr = (char * ) invalidS;
            printk(KERN_INFO "Invalid command, Shuffle first\n");
            //mutex_unlock(&blackJack_mutex);
            return len;
        }
    }
    //added this command to fix error I was getting
    if (strncmp(command, "NEXT", 4) == 0) {
        if (checks.is_deal){
            msg_ptr = (char * )  more_cards;
            checks.is_next = true;
        }

    }
    if (strncmp(command, "Hit", 3) == 0) {
        if (checks.is_next){
            dealCard(&player, deck);
            displayPlayerHand("Player", &player);
        }
    }
    else if (strncmp(command, "No", 2) == 0) {
        if (checks.is_next) {
            while (dealer.score < 17) {
                //add more cards to dealer if < 17
                displayDealerHand("Dealer", &dealer);
                msg_ptr = (char *) dealerMore;
                dealCard(&dealer, deck);
            }
            displayDealerHand("Dealer", &dealer);
            checks.check_win = true;
        }
    }
    //added this command to check winner as I was having trouble checking the winner
    if (strncmp(command, "WIN", 3) == 0) {
        if (checks.check_win) {
            if (determineWinner(&player, &dealer)) {
                checks.game_over = true;
            }
        }
    }
    //ask to continue with same deck
    if (strncmp(command, "CONTINUE", 8) == 0) {
        if (checks.game_over) {
            msg_ptr = (char *) continue_game;
            if (strncmp(command, "Yes", 3) == 0) {
                player.num_cards = 0;
                player.score = 0;

                dealer.num_cards = 0;
                dealer.score = 0;
                shuffleDeck(deck);
            } else if (strncmp(command, "no", 2) == 0) {
                resetGame(&player, &dealer);
            }
        }
    }

    //mutex_unlock(&blackJack_mutex);
    return len;
}


void resetGame(Player *player, Player *dealer) {
    //reset the game
    player->num_cards = 0;
    player->score = 0;

    dealer->num_cards = 0;
    dealer->score = 0;
    msg_ptr = (char * ) ok;
}

void shuffleDeck(Card *deck) {
    //mutex_lock(&blackJack_mutex);
    // Initialize the deck
    int suit;
    for (suit = 0; suit < 4; suit++) {
        int value;
        for (value = 1; value <= 13; value++) {
            int index = suit * 13 + value - 1;
            deck[index].suit = suit;
            deck[index].value = value;
        }
    }

    // Shuffling the deck
    int i;
    for (i = DECK_SIZE - 1; i > 0; i--) {
        int j = get_random_int() % (i + 1);

        // Swap elements
        Card temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }

    msg_ptr = (char * ) ok;
    //mutex_unlock(&blackJack_mutex);
}

int currentCardIndex = 0;
void dealCard(Player *player, Card *deck) {
    //mutex_lock(&blackJack_mutex);
    if (currentCardIndex >= 52) {
        msg_ptr = (char * ) noMore;
        //mutex_unlock(&blackJack_mutex);
        return;
    }
    if (player->num_cards >= MAX_HAND_SIZE) {
        msg_ptr = (char * ) handFull;
        //mutex_unlock(&blackJack_mutex);
        return;
    }
    // Deal the next card from the deck
    Card card = deck[currentCardIndex++];
    player->hand[player->num_cards++] = card;

    //case of Ace
    if (card.value == 1) {
        if (player->score + 11 <= 21) {
            player->score += 11;
        } else {
            player->score += 1;
        }
    // Face cards (Jack, Queen, King)
    } else if (card.value > 10) {
        player->score += 10;
    } else { // Regular cards
        player->score += card.value;
    }
    msg_ptr = (char *)ok;
    //mutex_unlock(&blackJack_mutex);
}

char* getSuitName(Suit suit) {
    if (suit == HEARTS){
        return "Hearts";
    }
    else if (suit == DIAMONDS){
        return "Diamonds";
    }
    else if (suit == CLUBS){
        return "Clubs";
    }
    else if (suit == SPADES){
        return "Spades";
    }
    return "unknown";
}

void displayPlayerHand(const char *entity, Player *player) {
    //mutex_lock(&blackJack_mutex);
    int len = 0;
    len += snprintf(msg_buffer + len, BUFFER_SIZE - len, "%s has ", entity);
    int i;
    for (i = 0; i < player->num_cards; i++) {
        Card *card = &player->hand[i];
        len += snprintf(msg_buffer + len, BUFFER_SIZE - len, "%d of %s", card->value, getSuitName(card->suit));
        if (i < player->num_cards - 1) {
            len += snprintf(msg_buffer + len, BUFFER_SIZE - len, " and ");
        }
    }

    snprintf(msg_buffer + len, BUFFER_SIZE - len, " for a total of %d.\n", player->score);
    msg_ptr = msg_buffer;
    printk(KERN_INFO "%s\n", msg_buffer);
    //mutex_unlock(&blackJack_mutex);
}

void displayDealerHand(const char *entity, Player *player) {
    //mutex_lock(&blackJack_mutex);
    int len = 0;
    len += snprintf(msg_buffer2 + len, BUFFER_SIZE - len, "%s has ", entity);
    int i;
    for (i = 0; i < player->num_cards; i++) {
        Card *card = &player->hand[i];
        len += snprintf(msg_buffer2 + len, BUFFER_SIZE - len, "%d of %s", card->value, getSuitName(card->suit));
        if (i < player->num_cards - 1) {
            len += snprintf(msg_buffer2 + len, BUFFER_SIZE - len, " and ");
        }
    }

    snprintf(msg_buffer2 + len, BUFFER_SIZE - len, " for a total of %d.\n", player->score);
    msg_ptr = msg_buffer2;
    printk(KERN_INFO "%s\n", msg_buffer2);
    //mutex_unlock(&blackJack_mutex);
}

bool determineWinner(Player *player, Player *dealer) {
    //check win conditions
    bool win = false;
    if (player->score > 21) {
        win = true;
        msg_ptr = (char * ) dealer_win;
    } else if (dealer->score > 21) {
        win = true;
        msg_ptr = (char * ) player_win;
    } else if (player->score > dealer->score) {
        win = true;
        msg_ptr = (char * ) player_win;
    } else if (dealer->score > player->score) {
        win = true;
        msg_ptr = (char * ) dealer_win;
    } else {
        win = true;
        msg_ptr = (char * ) dealer_win;
    }
    return win;
}

//enter module
static int __init blackJack_init(void) {
    //register the device name and minor and fops
    int retValue = misc_register(&blackJack_driver);
    if (retValue < 0) {
        return retValue;
    }
    printk(KERN_INFO "device was loaded.\n");
    return 0;
}

//exit module
static void __exit blackJack_exit(void) {
    //unregister the device name and minor and fops
    misc_deregister(&blackJack_driver);
    printk(KERN_INFO "unloading the module\n");
}

module_init(blackJack_init);
module_exit(blackJack_exit);
