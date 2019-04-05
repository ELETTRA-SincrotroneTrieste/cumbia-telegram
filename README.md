## cumbia-telegram bot

Sign in the *cumbia-telegram* bot to chat with a [Tango](http://www.tango-controls.org) or [EPICS](https://epics.anl.gov/)
control system from *everywhere*.

With the *cumbia-telegram* bot you can read values from the aforementioned control systems, monitor them
and receive alerts when something special happens. You text the bot, the bot texts you back.
It's simple, fast, intuitive and it's *everywhere*. A telegram account and a client are all you need to
start. [Telegram](https://telegram.org/) is a  cloud-based mobile and desktop messaging app focused
on security and speed. It is available for *Android*, *iPhone/iPad*, *Windows*, *macOS*, *Linux*... as
well as a *web application*.


### Safety first

On the *server side*, *cumbia-telegram* provides the administrator full control over the allocation of resources,
the network load and the clients authorized to chat with the bot. Additionally, the access on the systems is
*read only*.

### Simplicity second

On the *client side*, the bot has been meticulously crafted to make interaction easy and fast: *history*,
*bookmarks* and *alias* plugins pare texting down to the bone. Preferred and most frequent
operations are accessible by simple *taps* on special *command links*!


### Extensible

Need to extend the bot functionalities?
Write new *plugins*!

The bot relies on *modules* and *plugins* to provide each of its operations.
The former are built in, the second are add ons, but they are treated the same way by the bot, that loads
them in a given order (determined by either the module or plugin) at startup.

## Installation

### Part one. Build the bot application.

#### Prerequisites

*cumbia-libs* are a requirement and they must be properly installed before attempting to build the *cumbia-telegram*
bot. The fundamental libraries are *cumbia* and *cumbia-qtcontrols*. The *cumbia-qtcontrols* installation implies
*qwt* libraries must be installed in your system too.

The *cumbia-telegram* bot installation relies on *pkg-config*. See comments below.
If Tango support is desired, all the Tango dependencies must be found within the *pkg-config* path.
If Tango support is desired, *cumbia-tango* must have been previously installed alongside qumbia-tango-controls.
If their installation succeeded, *pkg-config* files will be ready in their place.

Refer to the [cumbia-libs](https://github.com/ELETTRA-SincrotroneTrieste/cumbia-libs) github page for library installation.


#### Clone the *cumbia-telegram* project

```
git clone https://github.com/ELETTRA-SincrotroneTrieste/cumbia-telegram.git
```

#### Check dependency and define installation destination

Change directory into *cumbia-telegram* and edit *cumbia-telegram.pri*.
Pay attention to all directives within the *customization section start* and *customization section end*
block.

```
#                                                         customization section start
#
INSTALL_ROOT = /usr/local/cumbia-telegram
#
SHAREDIR = $${INSTALL_ROOT}/share
DOCDIR = $${SHAREDIR}/doc

# where to look for telegram plugins
CUMBIA_TELEGRAM_PLUGIN_PATH=$${INSTALL_ROOT}/lib/plugins
DEFINES += CUMBIA_TELEGRAM_PLUGIN_DIR=\"\\\"$${CUMBIA_TELEGRAM_PLUGIN_PATH}\\\"\"

# [..]
QWT_HOME=

# DEPENDENCIES

# cumbia plugin directory.
# Please check your cumbia installation and modify
# CU_QTC_PLUGINDIR accordingly
#
CU_QTC_PLUGINDIR = $${INSTALL_ROOT}/lib/qumbia-plugins
#
DEFINES += CUMBIA_QTCONTROLS_PLUGIN_DIR=\"\\\"$${CU_QTC_PLUGINDIR}\\\"\"
#
#                                                       customization section end
```

Make sure the PKG_CONFIG_PATH includes cumbia, cumbia-qtcontrols, cumbia-tango [cumbia-epics],
cumbia-tango-controls, [cumbia-epics-controls], tango, zeromq; in general, all the dependencies
needed by cumbia.

The INSTALL_ROOT and CUMBIA_TELEGRAM_PLUGIN_PATH define where the bot will be installed


#### Build and install the bot

From within the *cumbia-telegram* directory, execute

```
qmake
```
and then
```
make -j9
```
and  ```make install```

Please note that the software is organized into three source trees:

- src/lib/
- src/cumbia-telegram-app
- cumbia-telegram-control

built in that order by the top level *cumbia-telegram.pro*  *Qt project* file.

The last is an *command line interface* to communicate with the bot and is used to
authorize users and get statistics.

### Part two. Build the plugins.

#### Clone the *cumbia-telegram-plugins* project

The *cumbia-telegram-plugins* provide extensions to *cumbia-telegram*.

```
https://github.com/ELETTRA-SincrotroneTrieste/cumbia-telegram-plugins.git
```

#### Edit cumbia-telegram-plugins.pri

cd into cumbia-telegram-plugins dir and edit the .pri file.
Check and fix the directives needed to locate the installation of the *cumbia-telegram* bot.
The relevant ones are delimited by a *customization section*:

```
INSTALL_ROOT = /usr/local/cumbia-telegram
CUMBIA_TELEGRAM_ROOT = /usr/local/cumbia-telegram
PLUGIN_TARGET_DIR = $${INSTALL_ROOT}/lib/plugins
```

#### Build and install plugins

```
qmake && make -j9 && make install
```

#### CUMBIA_TELEGRAM_PLUGIN_DIR

The  *CUMBIA_TELEGRAM_PLUGIN_DIR* that had been defined in *cumbia-telegram.pri* determines the path from which
plugins are loaded.

*Double check it before starting the bot later on*.

An alternative is exporting CUMBIA_PLUGIN_PATH so that it points to *cumbia-telegram-plugins* directory.

### Part three. Bot creation.

Following the [how do I create a bot?](https://core.telegram.org/bots#3-how-do-i-create-a-bot) guide, you
text to the [BotFather](https://telegram.me/botfather). and follow a few simple steps. Once you've created
a bot and received your *authorization token*, the *cumbia-telegram* bot will be given that token at
startup. Following the *BotFather* instructions is very easy and at last you should end up with a new *bot*
with a *name*, a username, a description, a short description, a profile picture.

To make client interaction quick and easy, we suggest imparting the */setcommands* command to the *BotFather* and
send him the list of the most frequent and useful commands that are understood by the *cumbia-telegram* bot.
The recommended list is the following:

last - Repeat last successful command
reads - Read history
alerts - Alert history
monitors - Monitor history
bookmarks - Your bookmarks
bookmark - Add a bookmark to last successful command
alias - List your alias
modules - List loaded modules and plugins and read their documentation
help - Read essential documentation to start

You can copy and paste it as a reply to the */setcommands* message.

Once the *bot setup* is complete, just start the application as described in the next paragraph.

### Part four. Start the bot!

The bot saves its configuration on a file representing a *sqlite* database.
The database stores user *access and limit* privileges as well as client *history, bookmarks and alias*.
The file must be passed as command line argument to the bot and must be regularly backed up by
the administrator.

The second necessary command line argument is the *authorization token* provided by the *BotFather* at
bot registration time.

If compilation was successful, you should find a *bin* folder under *cumbia-telegram*.
An example of command line to start the *cumbia-telegram* bot is the following:

```
./bin/cumbia-telegram --token=bot123456789:AABBCCDDeeFFggHHiiLLMMnnOOPPQQrrSSt --db=tg_botdb.dat
```

#### Parameters explained

- *--db=/path/to/sqliteDB.sql* (a file where the sqlite db will be stored)

- *--token=telegram_bot_token* (as given by botfather at bot creation)

### Part five. Connect to the *bot* with your telegram client.

Go to your telegram client and join the bot with the name previously registered with the *BotFather*.
If you type the */help* command, you will notice that you are not authorized yet.
By default, no client (*user*) is authorized. The administrator must explicitly  give access to the bot
(see part five).

### Part five. Authorize new users

Security policies prevent any user to chat with the bot without authorization.
Go into the *cumbia-telegram-control* folder and type

```
./bin/cumbia-telegram-control --dbfile ../elettra_botdb.dat -u
```

This command will list the users into the database.

To authorize a user, use the -a switch followed by the user id:

```
./bin/cumbia-telegram-control --dbfile ../elettra_botdb.dat -a 11223344
```

The user with that id will receive a *congratulations* message and will be able to chat with the bot
with the default limits and options.

### Part six. Play!

Once authorized, type */help* to start using the bot!

#### Some read examples:

```
sys/tg_test/1/double_scalar
```

```
sys/tg_test/1/long_scalar > 10
```

#### Alerts

```
alert sys/tg_test/1/long_scalar > 10
```

#### Alias
```
alias  sys/tg_test/1/double_scalar d1
```

#### Monitor + Alias
```
monitor d1 < 200
```









